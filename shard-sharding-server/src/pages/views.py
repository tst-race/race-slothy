import os
import json
import uuid
import pprint
import subprocess
from multiprocessing.pool import ThreadPool
from django.shortcuts import render
import asyncio
import aioipfs

# from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage

from pages.models import *
from pages.forms import UploadDepForm

# optimize is from scatter package
from scatterlib.optimize.util import (
    push_to_snode_https,
    push_to_registry_https,
    create_instance,
    update_rnodes,
    update_snodes,
)

# global deploy semaphor
deploy_lock = False


def index(request):
    context = {}
    context["documents"] = Document.objects.all()
    config(request)
    return render(request, "pages/index.html", context)


def upload(request):
    config()
    context = {}
    if request.method == "POST":
        form = UploadDepForm(request.POST, request.FILES)
        # check whether it's valid:
        if form.is_valid():
            myfile = request.FILES["filename"]
            fs = FileSystemStorage()
            filename = fs.save(myfile.name, myfile)
            myfile_full_path = fs.path(myfile.name)
            context["uploaded_file_url"] = myfile.name
            Document.objects.create(document=filename)

            deps_json = json.loads(Dependencies.objects.all()[0].json)
            if form["name"].value():
                name = form["name"].value()
            else:
                name = filename
            new_dep = {}
            new_dep["uuid"] = form["uuid"].value()
            new_dep["name"] = name
            new_dep["filename"] = myfile_full_path
            new_dep["deptype"] = form["deptype"].value()
            new_dep["properties"] = {
                "maximize": {
                    "stability": float(form["stability"].value()),
                    "speed": float(form["speed"].value()),
                    "covertness": float(form["covertness"].value()),
                },
                "minimize": {"cost": float(form["cost"].value())},
            }
            new_dep["instances"] = []
            deps_json.append(new_dep)
            Dependencies.objects.all().update(json=json.dumps(deps_json, indent=4))
            # optimize()
            # push_to_snodes()
            # push_to_registries()

    context["documents"] = Document.objects.all()
    context["form"] = UploadDepForm(initial={"uuid": str(uuid.uuid4())}).as_p()
    return render(request, "pages/upload.html", context)


def shard(request):
    # if request.method == "POST":
    #     print(request.POST)
    #     file = request.POST["file"]
    #     enc, shard_list = shard_file(file)
    #     EncryptedFile.objects.create(document=enc, pushed=False)
    #     for shard in shard_list:
    #         print(shard)
    #         Shard.objects.create(document=shard, pushed=False)

    context = {}
    context["documents"] = Document.objects.all()
    context["shards"] = Shard.objects.all()
    context["encrypted_files"] = EncryptedFile.objects.all()

    # push(request)

    return render(request, "pages/shard.html", context)


def push(request):
    context = {}

    # if request.method == "POST":
    #     print(request.POST)
    #     file_path = request.POST["file"]
    #     file_path = file_path.strip(os.sep)
    #     file_path = os.path.join(os.environ["PWD"], file_path)
    #     file = os.path.basename(file_path)
    #     enc = f"{file_path}.enc"
    #     json_file = f"{file_path}.json"
    #     shardprefix = f"{json_file}."

    #     registries = json.loads(Registries.objects.all()[0].json)
    #     registry = registries["address"]
    #     hosts = " ".join(config["hosts"])

    #     cmd = f"scatter.py push_dependencies {file} \
    #             --shardprefix {shardprefix} \
    #             --encrypted {enc} \
    #             --registry {registry} \
    #             --hosts {hosts}"
    #     print(cmd)
    #     subprocess.check_output(cmd.split())

    return render(request, "pages/push.html", context)


def deploy_simple(request, simple="simple"):
    return deploy(request, simple=simple)


def deploy(request, simple=""):
    global deploy_lock
    config(request)
    context = {}
    if deploy_lock:
        context["message"] = "Deploying already in progress."
        return render(request, "pages/deploy.html", context)

    deploy_lock = True
    optimize(simple=simple)
    push_to_snodes()

    # Ugly, but gets the job done
    rnodes = json.loads(RaceNodes.objects.all()[0].json)
    snodes = json.loads(ServerNodes.objects.all()[0].json)
    for rn in rnodes:
        for sn in snodes:
            for rshard in rn["shards"]:
                if rshard["method"] == "ipfs":
                    for sshard in sn["shards"]:
                        if sshard["shard_uuid"] == rshard["shard_uuid"]:
                            rshard["ipfs_uuid"] = sshard["ipfs_uuid"]

    RaceNodes.objects.all().update(json=json.dumps(rnodes, indent=4))
    ServerNodes.objects.all().update(json=json.dumps(snodes, indent=4))

    push_to_registries()
    deploy_lock = False
    context["message"] = "Successfully deployed!"
    return render(request, "pages/deploy.html", context)


def config(request=None):
    # initialize all JSON configs by calling functions
    registries(request)
    dependencies(request)
    race_nodes(request)
    server_nodes(request)

    context = {}
    return render(request, "pages/config.html", context)


def registries(request):
    f = "./configs/regs.json"
    context = handle_config_request(request, Registries, f)
    context["config_type"] = "Registries"
    return render(request, "pages/config_base.html", context)


def dependencies(request):
    f = "./configs/deps.json"
    context = handle_config_request(request, Dependencies, f)
    context["config_type"] = "Dependencies"
    return render(request, "pages/config_base.html", context)


def race_nodes(request):
    f = "./configs/rnodes.json"
    context = handle_config_request(request, RaceNodes, f)
    context["config_type"] = "Race Nodes"
    return render(request, "pages/config_base.html", context)


def server_nodes(request):
    f = "./configs/snodes.json"
    context = handle_config_request(request, ServerNodes, f)
    context["config_type"] = "Server Nodes"
    return render(request, "pages/config_base.html", context)


################################################################################
# Helper Functions
################################################################################
def handle_config_request(request, model, file):
    context = {}
    if len(model.objects.all()) < 1:
        with open(file) as f:
            default = json.load(f)
        model.objects.create(json=json.dumps(default, indent=4))

    if request and request.method == "POST":
        try:
            json_validated = json.loads(request.POST["json"])
            model.objects.all().update(json=json.dumps(json_validated, indent=4))
        except ValueError as e:
            print(e)
            pass

    context["json"] = model.objects.all()[0].json
    return context


def randomize(request):
    global deploy_lock
    config(request)
    context = {}
    if deploy_lock:
        context["message"] = "Deploying already in progress."
        return render(request, "pages/deploy.html", context)

    deploy_lock = True

    regs = json.loads(Registries.objects.all()[0].json)
    deps = json.loads(Dependencies.objects.all()[0].json)
    rnodes = json.loads(RaceNodes.objects.all()[0].json)
    snodes = json.loads(ServerNodes.objects.all()[0].json)

    for d in deps:
        create_instance(d)

    update_rnodes(rnodes, snodes, deps, simple=True)
    update_snodes(snodes, deps, simple=True)

    Registries.objects.all().update(json=json.dumps(regs, indent=4))
    Dependencies.objects.all().update(json=json.dumps(deps, indent=4))
    RaceNodes.objects.all().update(json=json.dumps(rnodes, indent=4))
    ServerNodes.objects.all().update(json=json.dumps(snodes, indent=4))

    push_to_snodes()

    # push as many sets of dependencies to registry as there are race nodes
    # push_to_registries will automatically push rnodes times
    push_to_registries(duplicate=True)

    deploy_lock = False
    context["message"] = "Successfully deployed!"
    return render(request, "pages/deploy.html", context)


def optimize(simple=""):
    file_list = [
        ("/tmp/regs.json", Registries),
        ("/tmp/deps.json", Dependencies),
        ("/tmp/rnodes.json", RaceNodes),
        ("/tmp/snodes.json", ServerNodes),
    ]
    updated_list = [("_updated".join(os.path.splitext(i)), j) for (i, j) in file_list]

    for i, j in file_list:
        with open(i, "w+") as f:
            if len(j.objects.all()) == 0:
                return
            jsonified = json.loads(j.objects.all()[0].json)
            json.dump(jsonified, f)

    flags = "--simple" if (simple.lower() == "simple" or simple.lower() == "true") else ""

    cmd = f"scatterlib/optim.py {flags} {file_list[0][0]} \
                     {file_list[1][0]} \
                     {file_list[2][0]} \
                     {file_list[3][0]}"
    # print(cmd)
    output = subprocess.check_output(cmd.split())
    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint("\t-> {}".format(output))

    for i, j in updated_list:
        with open(i) as f:
            updated_json = json.load(f)
            j.objects.all().update(json=json.dumps(updated_json, indent=4))

    # for i, _ in file_list:
    #     os.remove(i)
    # for i, _ in updated_list:
    #     os.remove(i)


def push_to_snode_ipfs(n):
    # print("Begin push to IPFS")
    to_push = {}

    if len(list(filter(lambda x: not x["pushed"], n["shards"]))) == 0:
        print("No new shards to push")
        return n

    for shard_index in range(len(n["shards"])):
        shard = n["shards"][shard_index]
        if shard["pushed"]:
            continue

        # Rename shard
        dep_uuid = shard["dep_uuid"]
        instance_uuid = shard["instance_uuid"]
        shard_uuid = shard["shard_uuid"]
        shard_location = f"media/{dep_uuid}/{instance_uuid}/{shard_uuid}"
        to_push[shard_uuid] = (shard_index, shard_location)

    async def f(file_index_pairs):
        client = aioipfs.AsyncIPFS()

        # print(list(map(lambda x: x[1], to_push.values())))

        async for added_file in client.add(
            list(map(lambda x: x[1], file_index_pairs.values())), recursive=True
        ):
            print(
                "Uploaded file to IPFS {0}, CID: {1} (shard path: {2})".format(
                    added_file["Name"],
                    added_file["Hash"],
                    to_push[added_file["Name"]][1],
                )
            )

            n["shards"][file_index_pairs[added_file["Name"]][0]]["pushed"] = True
            n["shards"][file_index_pairs[added_file["Name"]][0]]["ipfs_uuid"] = added_file["Hash"]

        await client.close()

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(f(to_push))
    loop.close()

    return n


def push_to_snodes():
    snodes = json.loads(ServerNodes.objects.all()[0].json)
    with ThreadPool() as p:
        results = [None] * len(snodes)
        for n in range(len(snodes)):
            if snodes[n]["method"] == "https":
                results[n] = p.apply_async(push_to_snode_https, (snodes[n],))
            elif snodes[n]["method"] == "ipfs":
                results[n] = p.apply_async(push_to_snode_ipfs, (snodes[n],))
        for n in range(len(snodes)):
            snodes[n] = results[n].get()

    ServerNodes.objects.all().update(json=json.dumps(snodes, indent=4))


def push_to_registries(duplicate=False):
    rnodes = json.loads(RaceNodes.objects.all()[0].json)

    regs = json.loads(Registries.objects.all()[0].json)
    for n in range(len(rnodes)):
        for r in range(len(regs)):
            if regs[r]["uuid"] == rnodes[n]["registry"]:
                break
            else:
                # didn't find registry for this rnode
                # skip to next rnode
                continue

        if regs[r]["method"] == "https":
            regs[r], rnodes[n] = push_to_registry_https(regs[r], rnodes[n], duplicate=duplicate)

    RaceNodes.objects.all().update(json=json.dumps(rnodes, indent=4))
    Registries.objects.all().update(json=json.dumps(regs, indent=4))
