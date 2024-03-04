import os
import json
import uuid
import glob
import math
import time
import socket
import random
import requests
import ipaddress
import subprocess

from z3 import is_true


def resolve(addr):
    try:
        return socket.gethostbyname(addr)
    except socket.gaierror:
        return "127.0.0.1"


# TODO: improve routability stand-in
# TODO: change to querying actual database?


# def routability(addr1, addr2):
#     return 1


def random_order(seq):
    shuffled = list(seq)
    random.shuffle(shuffled)
    return iter(shuffled)


def routability(addr1, addr2):
    ip1 = resolve(addr1)
    ip2 = resolve(addr2)
    if ip1 == ip2:
        return 1
    ip1_int = int(ipaddress.ip_address(ip1))
    ip2_int = int(ipaddress.ip_address(ip2))
    diff = abs(ip1_int - ip2_int)
    log2_diff = math.log2(diff)
    route_max_distance = 8
    route_distance = max(log2_diff, route_max_distance)
    return float(route_max_distance / route_distance)


def in_subnet(addr1, addr2):
    ip1 = resolve(addr1)
    ip2 = resolve(addr2)
    ip1_subnet = get_subnet(ip1)
    return ipaddress.ip_address(ip2) in ip1_subnet


def get_subnet(addr):
    ip = resolve(addr)
    prefixlen = ipaddress.ip_address(ip).max_prefixlen - 16
    subnet = ipaddress.ip_network(ip + f"/{prefixlen}", strict=False)
    return subnet


def calc_net_spread(network, shard_locations):
    # shards_in_network = filter(lambda x: x in network, shards)
    shards_in_network = [x for x in network if x in shard_locations]
    return len(shards_in_network) / network.num_addresses


def shard_file(file):
    file = os.path.join(os.environ["PWD"], file)
    enc = f"{file}.enc"
    json_file = f"{file}.json"
    shardprefix = f"{json_file}."

    cmd = f"scatter.py encrypt_and_shard {file} \
            --shardprefix {shardprefix} \
            --encrypted {enc} \
            --json {json_file}"
    print(cmd)
    try:
        output = subprocess.check_output(cmd.split())
        print(output)
    except subprocess.CalledProcessError as e:
        if e.returncode != -11:
            raise (e)
        else:
            print("need to fix libsss")

    shard_list = glob.glob("{}[0-9]*".format(shardprefix))
    print(shard_list)
    return enc, shard_list


# TODO: create optimizer for finding N and M
def create_instance(dep):
    inst = dict()
    inst["uuid"] = str(uuid.uuid4())
    inst_path = os.path.join(os.environ["PWD"], "media", dep["uuid"], inst["uuid"])
    os.makedirs(inst_path)
    # TODO: integrate where N and M can be chosen by scatter
    # m_min = 4
    # n_min = 2
    # m_max = (sum(dep['properties']['maximize'].values()) -
    #          sum(dep['properties']['minimize'].values()))
    # inst['m'] = random.randint(m_min, max(int(m_max), m_min))
    # n_mean = (inst['m'] + n_min) / 2
    # n_std = (inst['m'] - n_min) / 3
    # inst['n'] = int(random.gauss(n_mean, n_std))
    # inst['n'] = max(n_min, inst['n'])
    # inst['blob_size'] = 2**24
    # inst['blob'] = f"blob-{str(uuid.uuid4()).split('-')[-1]}"
    # inst['shard_size'] = 2**20
    # inst['shards'] = list()
    # for i in range(inst['m']):
    #     shard_uuid = str(uuid.uuid4()).split("-")[-1]
    #     inst['shards'].append(f"{i+1}-{shard_uuid}")

    # shard_file actually shards dependency
    print("About to shard the file {}".format(dep["filename"]))
    blob, shard_list = shard_file(dep["filename"])
    inst["blob_size"] = os.stat(blob).st_size
    inst["blob"] = f"{uuid.uuid4()}-{blob.split(os.path.sep)[-1]}"
    shard_len_set = set(os.stat(s).st_size for s in shard_list)
    assert len(shard_len_set) == 1 and "shards arent same length!"
    inst["shards"] = [f"{uuid.uuid4()}-{i.split(os.path.sep)[-1]}" for i in shard_list]
    inst["shard_size"] = shard_len_set.pop()
    inst["n"] = 8
    inst["m"] = 16

    # move files into instance path
    os.rename(blob, os.path.join(inst_path, inst["blob"]))
    for i in range(len(shard_list)):
        inst_fn = inst["shards"][i].split(os.path.sep)[-1]
        os.rename(shard_list[i], os.path.join(inst_path, inst_fn))
        print("\t", os.path.join(inst_path, inst_fn))
    dep["instances"].append(inst)
    return dep


def write_updated_json(old_fn, new_obj):
    fn, ext = os.path.splitext(old_fn)
    new_fn = f"{fn}_updated{ext}"
    os.path.exists(new_fn)
    os.path.isfile(new_fn)
    with open(new_fn, "w") as f:
        json.dump(new_obj, f, indent=4)


def is_shard_new_globally(nodes, shard_dict):
    for node in nodes:
        for i in node["shards"]:
            if (
                shard_dict["instance_uuid"] == i["instance_uuid"]
                and shard_dict["shard_uuid"] == i["shard_uuid"]
            ):
                return False
    return True


def is_shard_new_to_node(node, shard_dict):
    for i in node["shards"]:
        if (
            shard_dict["instance_uuid"] == i["instance_uuid"]
            and shard_dict["shard_uuid"] == i["shard_uuid"]
        ):
            return False
    return True


def push_to_snode_https(n):
    host = n["address"]
    # Rename & push shards
    for i in range(len(n["shards"])):
        if n["shards"][i]["pushed"]:
            continue

        # Rename shard
        dep_uuid = n["shards"][i]["dep_uuid"]
        instance_uuid = n["shards"][i]["instance_uuid"]
        shard_uuid = n["shards"][i]["shard_uuid"]
        shard_location = f"media/{dep_uuid}/{instance_uuid}/{shard_uuid}"
        shard_url = f"https://{host}/{shard_uuid}"
        # Push shard
        with open(shard_location, "rb") as f:
            # TODO: push experation
            requests.post(shard_url, data=f.read(), verify=False)
        # set pushed to true
        n["shards"][i]["pushed"] = True

    return n


def push_to_registry_https(registry, rnode, duplicate=False):
    for instance in set(shard["instance_uuid"] for shard in rnode["shards"]):
        if instance in registry["instances"] and (not duplicate):

            # already pushed to registry, dont have to do anything
            # if duplicate is set, it will push again anyways
            continue
        else:
            # print(rnode)
            # collect all shards for instance
            inst_name = ""
            inst_shards = []
            inst_blobs = []
            for i in range(len(rnode["shards"])):
                if instance == rnode["shards"][i]["instance_uuid"] and (
                    rnode["shards"][i]["registered"] is False or duplicate
                ):
                    inst_name = rnode["shards"][i]["deptype"]
                    shard_location = rnode["shards"][i]["location"]
                    
                    # print("Checking push method {}".format(rnode["shards"][i]["method"]))
                    if(rnode["shards"][i]["method"] == "ipfs"):
                        ipfs_uuid = rnode["shards"][i]["ipfs_uuid"]
                        loc = f"https://{shard_location}/ipfs/{ipfs_uuid}"
                    elif(rnode["shards"][i]["method"] == "https"):
                        shard_uuid = rnode["shards"][i]["shard_uuid"]
                        loc = f"https://{shard_location}/{shard_uuid}"

                    if "blob" in rnode["shards"][i]:
                        inst_blobs.append(loc)
                    else:
                        inst_shards.append(loc)
                    rnode["shards"][i]["registered"] = True

            # Send JSON to registry
            dep_json = {
                "name": inst_name,
                "instance_uuid": instance,
                "so_sharded": False,
                "so": inst_blobs,
                "json_sharded": True,
                "json": inst_shards,
            }

        registry_url = f"https://{registry['address']}/{instance}"
        requests.post(registry_url, json=dep_json, verify=False)

        registry["instances"][instance] = dep_json

    return registry, rnode


def update_rnodes(rnodes, snodes, deps, model=None, opt=None, simple=False):
    print("Updating RNODES")
    # update self.rnodes based on solving
    # doesnt add collisions. shards are deleted from self.rnodes and snodes at start
    for n in range(len(rnodes)):
        for i in range(len(deps)):
            for j in range(len(deps[i]["instances"])):
                # update blobs
                if simple or is_true(model[opt.rnode_has_dep_blob[n][i][j]]):
                    instance_uuid = deps[i]["instances"][j]["uuid"]
                    blob_uuid = deps[i]["instances"][j]["blob"]
                    for sn in random_order(range(len(snodes))):
                        # TODO: random_order picks a random snode of the snodes
                        # that have the shard. this should be optimized
                        if simple or is_true(model[opt.snode_has_dep_blob[sn][i][j]]):
                            # found server node that holds this shard
                            # break so sn is the correct index
                            break
                    d = {
                        "deptype": deps[i]["deptype"],
                        "location": f"{snodes[sn]['address']}",
                        "method": f"{snodes[sn]['method']}",
                        # "dep_uuid": deps[i]["uuid"],
                        "instance_uuid": instance_uuid,
                        "shard_uuid": blob_uuid,
                        "blob": True,
                        "pushed": False,
                        "registered": False,
                        "expiration": int(time.time()) + 1000000,
                    }
                    if is_shard_new_to_node(rnodes[n], d):
                        rnodes[n]["shards"].append(d)
                # update shards
                for k in range(len(deps[i]["instances"][j]["shards"])):
                    if simple or is_true(model[opt.rnode_has_dep_shard[n][i][j][k]]):
                        instance_uuid = deps[i]["instances"][j]["uuid"]
                        shard_uuid = deps[i]["instances"][j]["shards"][k]
                        for sn in random_order(range(len(snodes))):
                            # TODO: random_order picks a random snode of the snodes
                            # that have the shard. this should be optimized
                            if simple or is_true(model[opt.snode_has_dep_shard[sn][i][j][k]]):
                                # found server node that holds this shard
                                # break so sn is the correct index
                                break
                        d = {
                            "deptype": deps[i]["deptype"],
                            "location": f"{snodes[sn]['address']}",
                            "method": f"{snodes[sn]['method']}",
                            # "dep_uuid": deps[i]["uuid"],
                            "instance_uuid": instance_uuid,
                            "shard_uuid": shard_uuid,
                            "pushed": False,
                            "registered": False,
                            "expiration": int(time.time()) + 1000000,
                        }
                        if is_shard_new_to_node(rnodes[n], d):
                            rnodes[n]["shards"].append(d)


def update_snodes(snodes, deps, model=None, opt=None, simple=False):
    # update anodes based on solving
    # doesnt add collisions. shards are deleted from rnodes and snodes at start
    for n in range(len(snodes)):
        for i in range(len(deps)):
            for j in range(len(deps[i]["instances"])):
                # update blobs
                if simple or is_true(model[opt.snode_has_dep_blob[n][i][j]]):
                    dep_uuid = deps[i]["uuid"]
                    instance_uuid = deps[i]["instances"][j]["uuid"]
                    blob_uuid = deps[i]["instances"][j]["blob"]
                    d = {
                        "dep_uuid": dep_uuid,
                        "instance_uuid": instance_uuid,
                        "shard_uuid": blob_uuid,
                        "blob": True,
                        "pushed": False,
                        "registered": False,
                        "expiration": int(time.time()) + 1000000,
                    }
                    if is_shard_new_to_node(snodes[n], d):
                        snodes[n]["shards"].append(d)
                # update shards
                for k in range(len(deps[i]["instances"][j]["shards"])):
                    if simple or is_true(model[opt.snode_has_dep_shard[n][i][j][k]]):
                        dep_uuid = deps[i]["uuid"]
                        instance_uuid = deps[i]["instances"][j]["uuid"]
                        shard_uuid = deps[i]["instances"][j]["shards"][k]
                        d = {
                            "dep_uuid": dep_uuid,
                            "instance_uuid": instance_uuid,
                            "shard_uuid": shard_uuid,
                            "pushed": False,
                            "registered": False,
                            "expiration": int(time.time()) + 1000000,
                        }
                        if is_shard_new_to_node(snodes[n], d):
                            snodes[n]["shards"].append(d)
