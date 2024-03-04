#!/usr/bin/env python3
# -------------------------------------------------------------------------------
# Copyright (c) 2019, Cybertron LLC
# All rights reserved.
# -------------------------------------------------------------------------------
import os
import json
import time
import argparse
import ipaddress

from z3 import Int, Optimize, Sum, And, Or, If, Implies, Bool, set_param

from optimize.util import (
    routability,
    get_subnet,
    create_instance,
    write_updated_json,
    resolve,
    update_rnodes,
    update_snodes,
)

# -------------------------------------------------------------------------------
__author__ = "Orion Pax"
__email__ = "opax@cybertron.com"

"""
1. HOW many members of how many parts should a dependency be sharded
populate deps shards

2. WHERE to distribute dependency shards given a collection of service nodes
populate snodes shards

3. WHICH dependencies and shards to provide WHICH endpoint
populate self.rnodes shards<->snodes
"""
# -------------------------------------------------------------------------------

set_param("parallel.enable", True)  # enable parallelization for Z3


class Optimizer(object):
    def __init__(self, deps, rnodes, snodes, args):
        self.deps = deps
        self.rnodes = rnodes
        self.snodes = snodes
        self.args = args
        self.start_time = time.time()

    def reset(self):
        self.opt = Optimize()

    def create_opt_model(self, deps=None, rnodes=None, snodes=None):
        if self.args.verbose:
            print("create_opt_model:", round(time.time() - self.start_time, 5))
        if deps:
            self.deps = deps
        if rnodes:
            self.rnodes = rnodes
        if snodes:
            self.snodes = snodes

        self.reset()

        if self.args.simple:
            return self.create_opt_model_simple()

        # setup variables
        self.setup_bool_vars()
        # self.setup_sum_vars()

        # rules
        # self.rule_sum_vars()
        self.rule_one_snode_must_have_shard_if_rnode_has_shard()
        self.rule_one_snode_must_have_blob_if_rnode_has_blob()
        self.rule_sum_shards_between_n_and_m()
        self.rule_rnode_has_blob_if_has_shards()
        # self.rule_shard_routability()
        # self.rule_blob_routability()
        self.rule_blob_matches_inst()
        self.rule_snode_capacity()
        self.rule_only_one_instance_per_rnode()
        self.rule_one_instance_per_rnode()
        self.rule_no_duplicate_shards_no_extra_shards()
        # self.rule_no_duplicate_blobs_no_extra_blobs()
        self.rule_rnode_has_dependencies()

        return self.opt

    def create_opt_model_simple(self):
        if self.args.verbose:
            print("create_opt_model_simple:", round(time.time() - self.start_time, 5))
        # setup variables
        self.setup_bool_vars()
        # self.setup_sum_vars()

        # rules
        # self.rule_sum_vars()
        self.rule_all_snodes_have_all_shards_and_blobs()
        self.rule_one_snode_must_have_shard_if_rnode_has_shard()
        self.rule_one_snode_must_have_blob_if_rnode_has_blob()
        self.rule_sum_shards_between_n_and_m()
        self.rule_sum_shards_at_least(12)
        # self.rule_at_least_all_shards_per_dep_across_snodes(len(self.snodes))
        self.rule_rnode_has_blob_if_has_shards()
        # # self.rule_shard_routability()
        # # self.rule_blob_routability()
        self.rule_blob_matches_inst()
        # self.rule_snode_capacity()
        # self.rule_only_one_instance_per_rnode()
        self.rule_one_instance_per_rnode()
        # # self.rule_no_duplicate_shards_no_extra_shards()
        # # self.rule_no_duplicate_blobs_no_extra_blobs()
        self.rule_rnode_has_dependencies()

        # self.optimize_functions()

        return self.opt

    def optimize_functions(self):
        # optimize
        # self.opt_min_subnet_spread()
        # self.opt_max_internet_spread()
        # self.opt_min_shards_rnodes()
        # self.opt_max_shards_snodes()

        self.offset = -5
        self.opt_properties_deps()
        self.opt_properties_rnodes()
        self.opt_properties_snodes()

    def setup_bool_vars(self):
        if self.args.verbose:
            print("setup_bool_vars:", round(time.time() - self.start_time, 5))
        # Create server node shard tracking.
        # naming convention: servernode_has_dependency#_instance#_shard#
        self.snode_has_dep_shard = dict()
        self.snode_has_dep_blob = dict()
        for n in range(len(self.snodes)):
            # Create list of existing shards in node
            has_shards = [
                i["shard_uuid"]
                for i in snodes[n].setdefault("shards", list())
                if i["pushed"] is True and i["expiration"] > time.time()
            ]
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    # setup blob
                    self.snode_has_dep_blob.setdefault(n, dict()).setdefault(i, dict())
                    if "blob" in self.deps[i]["instances"][j]:
                        var = Bool(f"s{n}_has_d{i}_i{j}_b")
                        self.snode_has_dep_blob.setdefault(n, dict()).setdefault(i, dict())[j] = var
                        if self.deps[i]["instances"][j]["blob"] in has_shards:
                            self.opt.add(var is True)
                    # setup shards
                    for k in range(len(self.deps[i]["instances"][j]["shards"])):
                        shard_uuid = self.deps[i]["instances"][j]["shards"][k]
                        var = Bool(f"s{n}_has_d{i}_i{j}_s{k}_{shard_uuid}")
                        self.snode_has_dep_shard.setdefault(n, dict()).setdefault(
                            i, dict()
                        ).setdefault(j, dict())[k] = var

                        # if shard already exists, set to true
                        if shard_uuid in has_shards:
                            self.opt.add(var is True)

        # Create race node shard tracking.
        # naming convention: racenode_has_dependency#_instance#_shard#
        self.rnode_has_dep_shard = dict()
        self.rnode_has_dep_blob = dict()
        for n in range(len(self.rnodes)):
            # Create list of existing shards in node
            has_shards = [
                i["shard_uuid"]
                for i in self.rnodes[n].setdefault("shards", list())
                if i["expiration"] > time.time()
            ]
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    # setup blob
                    self.rnode_has_dep_blob.setdefault(n, dict()).setdefault(i, dict())
                    if "blob" in self.deps[i]["instances"][j]:
                        var = Bool(f"r{n}_has_d{i}_i{j}_b")
                        self.rnode_has_dep_blob.setdefault(n, dict()).setdefault(i, dict())[j] = var
                        if self.deps[i]["instances"][j]["blob"] in has_shards:
                            self.opt.add(var is True)
                    # setup shards
                    for k in range(len(self.deps[i]["instances"][j]["shards"])):
                        shard_uuid = self.deps[i]["instances"][j]["shards"][k]
                        var = Bool(f"r{n}_has_d{i}_i{j}_s{k}_{shard_uuid}")
                        self.rnode_has_dep_shard.setdefault(n, dict()).setdefault(
                            i, dict()
                        ).setdefault(j, dict())[k] = var

                        # if shard already exists, set to true
                        if shard_uuid in has_shards:
                            self.opt.add(var is True)

    def setup_sum_vars(self):
        if self.args.verbose:
            print("setup_sum_vars:", round(time.time() - self.start_time, 5))
        # Create server node dependency tracking for total shards
        # naming convention: servernode_has_dependency#_instance#_sum
        self.snode_has_dep_sum = dict()
        for n in range(len(self.snodes)):
            for i in range(len(self.deps)):
                self.snode_has_dep_sum.setdefault(n, dict()).setdefault(i, dict())
                for j in range(len(self.deps[i]["instances"])):
                    v = Int("s{0}_has_d{1}_i{2}_sum".format(n, i, j))
                    self.snode_has_dep_sum.setdefault(n, dict()).setdefault(i, dict())[j] = v

        # Create race node dependency tracking for total shards
        # naming convention: servernode_has_dependency#_instance#_sum
        self.rnode_has_dep_sum = dict()
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                self.rnode_has_dep_sum.setdefault(n, dict()).setdefault(i, dict())
                for j in range(len(self.deps[i]["instances"])):
                    v = Int("r{0}_has_d{1}_i{2}_sum".format(n, i, j))
                    self.rnode_has_dep_sum.setdefault(n, dict()).setdefault(i, dict())[j] = v

        self.rnode_shard_sum = dict()
        for n in range(len(self.rnodes)):
            v = Int("r{0}_shard_sum".format(n))
            self.rnode_shard_sum[n] = v

        self.snode_shard_sum = dict()
        for n in range(len(self.snodes)):
            v = Int("s{0}_shard_sum".format(n))
            self.snode_shard_sum[n] = v

    def rule_sum_vars(self):
        if self.args.verbose:
            print("rule_sum_vars:", round(time.time() - self.start_time, 5))
        # computed sum of shards per race node, dependency, and instance
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    s = Sum([If(x, 1, 0) for x in self.rnode_has_dep_shard[n][i][j].values()])
                    self.opt.add(self.rnode_has_dep_sum[n][i][j] == s)

        # computed sum of shards per server node, dependency, and instance
        for n in range(len(self.snodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    s = Sum([If(x, 1, 0) for x in self.snode_has_dep_shard[n][i][j].values()])
                    self.opt.add(self.snode_has_dep_sum[n][i][j] == s)

        # computed sum of shards per server node overall
        for n in range(len(self.snodes)):
            s = Sum(
                [
                    Sum(
                        [
                            Sum([If(x, 1, 0) for x in self.snode_has_dep_shard[n][i][j].values()])
                            for j in range(len(self.deps[i]["instances"]))
                        ]
                    )
                    for i in range(len(self.deps))
                ]
            )
            self.opt.add(self.snode_shard_sum[n] == s)

        # computed sum of shards per server node overall
        for n in range(len(self.rnodes)):
            s = Sum(
                [
                    Sum(
                        [
                            Sum([If(x, 1, 0) for x in self.rnode_has_dep_shard[n][i][j].values()])
                            for j in range(len(self.deps[i]["instances"]))
                        ]
                    )
                    for i in range(len(self.deps))
                ]
            )
            self.opt.add(self.rnode_shard_sum[n] == s)

    def rule_all_snodes_have_all_shards_and_blobs(self):
        for n in range(len(self.snodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    self.opt.add(self.snode_has_dep_blob[n][i][j] is True)
                    for k in range(len(self.deps[i]["instances"][j]["shards"])):
                        self.opt.add(self.snode_has_dep_shard[n][i][j][k] is True)

    def rule_one_snode_must_have_shard_if_rnode_has_shard(self):
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    for k in range(len(self.deps[i]["instances"][j]["shards"])):
                        any_snode = Or(
                            [
                                self.snode_has_dep_shard[sn][i][j][k]
                                for sn in range(len(self.snodes))
                            ]
                        )
                        self.opt.add(Implies(self.rnode_has_dep_shard[n][i][j][k], any_snode))

    def rule_one_snode_must_have_blob_if_rnode_has_blob(self):
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    any_snode = Or(
                        [self.snode_has_dep_blob[sn][i][j] for sn in range(len(self.snodes))]
                    )
                    self.opt.add(Implies(self.rnode_has_dep_blob[n][i][j], any_snode))

    def rule_sum_shards_between_n_and_m(self):
        if self.args.verbose:
            print("rule_sum_shards_between_n_and_m:", round(time.time() - self.start_time, 5))
        # sum of shards should equal 0 or n<=x<=m for each instance
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    eq_zero = sum_shards == 0
                    gt_n = sum_shards >= deps[i]["instances"][j]["n"]
                    lt_m = sum_shards <= deps[i]["instances"][j]["m"]
                    self.opt.add(Or(eq_zero, And(gt_n, lt_m)))

    def rule_sum_shards_at_least(self, num_shards):
        if self.args.verbose:
            print("rule_sum_shards_at_least:", round(time.time() - self.start_time, 5))
        # sum of shards should equal 0 or x>=num_shards
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    eq_zero = sum_shards == 0
                    self.opt.add(Or(eq_zero, sum_shards >= num_shards))

    def rule_at_most_one_shard_per_dep_per_snode(self):
        if self.args.verbose:
            print("rule_one_shard_per_dep_per_snode:", round(time.time() - self.start_time, 5))
        for n in range(len(self.snodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    snode_shard_list = [
                        If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                        for k in range(len(self.deps[i]["instances"][j]["shards"]))
                    ]
                    self.opt.add(Sum(snode_shard_list) <= 1)

    def rule_at_least_all_shards_per_dep_across_snodes(self, copies=1):
        if self.args.verbose:
            print(
                "rule_at_least_all_shards_per_dep_across_snodes:",
                round(time.time() - self.start_time, 5),
            )
        for i in range(len(self.deps)):
            for j in range(len(self.deps[i]["instances"])):
                for k in range(len(self.deps[i]["instances"][j]["shards"])):
                    snode_shard_list = [
                        If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                        for n in range(len(self.snodes))
                    ]
                    print("copies:", copies)
                    self.opt.add(Sum(snode_shard_list) >= copies)

    def rule_max_shards_per_snode(self, num_shards):
        for n in range(len(self.snodes)):
            snode_shard_list = [
                If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            self.opt.add(Sum(snode_shard_list) <= num_shards)

    def rule_rnode_has_blob_if_has_shards(self):
        if self.args.verbose:
            print("rule_rnode_has_blob_if_has_shards:", round(time.time() - self.start_time, 5))
        # self.rnode_has_dep_blob must be true if sum_shards > 0
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    no_dep = sum_shards == 0
                    blob_list = [
                        # if shard is on rnode and snode we are looking at
                        And(self.rnode_has_dep_blob[n][i][j], self.snode_has_dep_blob[sn][i][j])
                        # loop through all server nodes
                        for sn in range(len(self.snodes))
                    ]
                    # if eq_zero, it means that the rnode doesn't have the dependency
                    # which means that it doesnt need the blob.
                    self.opt.add(Or(no_dep, Or(blob_list)))

    def rule_shard_routability(self):
        if self.args.verbose:
            print("rule_shard_routability:", round(time.time() - self.start_time, 5))
        # rnode routability must equal greater than n
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    # sum up the routability from rnode to snode if the shard is shared
                    rout_list = [
                        If(
                            # if shard is on rnode and snode we are looking at
                            And(
                                self.rnode_has_dep_shard[n][i][j][k],
                                self.snode_has_dep_shard[sn][i][j][k],
                            ),
                            # if true, use routability score
                            routability(self.rnodes[n]["address"], self.snodes[sn]["address"]),
                            # if false, use zero
                            0,
                        )
                        # loop through all shards
                        for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        # loop through all server nodes
                        for sn in range(len(self.snodes))
                    ]
                    # this calculation won't matter if the dependency isn't on rnode
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    no_dep = sum_shards == 0
                    # routability should be at least equal to n for the instance
                    routable_gt_n = Sum(rout_list) >= deps[i]["instances"][j]["n"]
                    # require either not existing or the sum > n
                    self.opt.add(Or(no_dep, routable_gt_n))
                    # self.opt.maximize(Sum(routability_list))

    def rule_blob_routability(self):
        if self.args.verbose:
            print("rule_blob_routability:", round(time.time() - self.start_time, 5))
        # rnode routability must equal greater than n
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    # sum up the routability from rnode to snode if the shard is shared
                    rout_list = [
                        If(
                            # if shard is on rnode and snode we are looking at
                            And(
                                self.rnode_has_dep_blob[n][i][j], self.snode_has_dep_blob[sn][i][j]
                            ),
                            # if true, use routability score
                            routability(self.rnodes[n]["address"], self.snodes[sn]["address"]),
                            # if false, use zero
                            0,
                        )
                        # loop through all server nodes
                        for sn in range(len(self.snodes))
                    ]
                    # this calculation won't matter if the dependency isn't on rnode
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    no_dep = sum_shards == 0
                    # routability should be at least equal to 1 for the instance
                    routable_gt_1 = Sum(rout_list) >= 1
                    # require either not existing or the sum > n
                    self.opt.add(Or(no_dep, routable_gt_1))
                    # self.opt.maximize(Sum(routability_list))

    def rule_blob_matches_inst(self):
        if self.args.verbose:
            print("rule_blob_matches_inst:", round(time.time() - self.start_time, 5))
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                for j in range(len(self.deps[i]["instances"])):
                    blob = If(self.rnode_has_dep_blob[n][i][j], 1, 0)
                    # if shards have been distributed, make sure blob is too
                    # if shards aren't distributed, ensure blob isn't
                    sum_shards = Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    eq = If(sum_shards > 0, blob == 1, blob == 0)
                    self.opt.add(eq)

    def rule_snode_capacity(self):
        if self.args.verbose:
            print("rule_snode_capacity:", round(time.time() - self.start_time, 5))
        for n in range(len(self.snodes)):
            shard_sizes = [
                self.deps[i]["instances"][j]["shard_size"]
                * If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            shard_sizes.extend(
                [
                    If(
                        self.snode_has_dep_blob[n][i][j],
                        self.deps[i]["instances"][j]["blob_size"],
                        0,
                    )
                    for i in range(len(self.deps))
                    for j in range(len(self.deps[i]["instances"]))
                ]
            )
            self.opt.add(Sum(shard_sizes) <= self.snodes[n]["capacity"])

    def rule_only_one_instance_per_rnode(self):
        if self.args.verbose:
            print("rule_only_one_instance_per_rnode:", round(time.time() - self.start_time, 5))
        # a dependency should have one or less instances of a dependency in one rnode
        for n in range(len(self.rnodes)):
            for i in range(len(self.deps)):
                sum_instances = Sum(
                    [
                        If(
                            Sum(
                                [
                                    If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                                    for k in range(len(self.deps[i]["instances"][j]["shards"]))
                                ]
                            )
                            > 0,
                            1,
                            0,
                        )
                        for j in range(len(self.deps[i]["instances"]))
                    ]
                )

                self.opt.add(sum_instances <= 1)

    def rule_one_instance_per_rnode(self):
        if self.args.verbose:
            print("rule_one_instance_per_rnode:", round(time.time() - self.start_time, 5))
        # a dependency instance should exist once across all self.rnodes
        # a dependency instance should exist once across all snodes
        for i in range(len(self.deps)):
            for j in range(len(self.deps[i]["instances"])):
                rnode_total_instances = [
                    If(
                        Sum(
                            [
                                If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                                for k in range(len(self.deps[i]["instances"][j]["shards"]))
                            ]
                        )
                        > 0,
                        1,
                        0,
                    )
                    for n in range(len(self.rnodes))
                ]
                self.opt.add(Sum(rnode_total_instances) <= 1)

    def rule_one_instance_per_snode(self):
        if self.args.verbose:
            print("rule_one_instance_per_snode:", round(time.time() - self.start_time, 5))
        # a dependency instance should exist once across all self.rnodes
        # a dependency instance should exist once across all snodes
        for i in range(len(self.deps)):
            for j in range(len(self.deps[i]["instances"])):
                snode_total_instances = [
                    If(
                        Sum(
                            [
                                If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                                for k in range(len(self.deps[i]["instances"][j]["shards"]))
                            ]
                        )
                        > 0,
                        1,
                        0,
                    )
                    for n in range(len(self.snodes))
                ]
                self.opt.add(Sum(snode_total_instances) <= 1)

    def rule_no_duplicate_shards_no_extra_shards(self):
        if self.args.verbose:
            print(
                "rule_no_duplicate_shards_no_extra_shards:", round(time.time() - self.start_time, 5)
            )
        # Shard should be put on only one rnode and snode globally
        for i in range(len(self.deps)):
            for j in range(len(self.deps[i]["instances"])):
                for k in range(len(self.deps[i]["instances"][j]["shards"])):
                    # share on only one rnode globally
                    rnode_s_list = [
                        If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                        for n in range(len(self.rnodes))
                    ]
                    self.opt.add(Sum(rnode_s_list) <= 1)
                    # share on only one snode globally
                    snode_shard_list = [
                        If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
                        for n in range(len(self.snodes))
                    ]
                    self.opt.add(Sum(snode_shard_list) <= 1)
                    # shard on snodes should match shards on rnodes
                    self.opt.add(Sum(snode_shard_list) == Sum(rnode_s_list))

    def rule_no_duplicate_blobs_no_extra_blobs(self):
        if self.args.verbose:
            print(
                "rule_no_duplicate_blobs_no_extra_blobs:", round(time.time() - self.start_time, 5)
            )
        # Shard should be put on only one rnode and snode globally
        for i in range(len(self.deps)):
            for j in range(len(self.deps[i]["instances"])):
                # share on only one rnode globally
                rnode_b_list = [
                    If(self.rnode_has_dep_blob[n][i][j], 1, 0) for n in range(len(self.rnodes))
                ]
                self.opt.add(Sum(rnode_b_list) <= 1)
                # share on only one snode globally
                snode_b_list = [
                    If(self.snode_has_dep_blob[n][i][j], 1, 0) for n in range(len(self.snodes))
                ]
                self.opt.add(Sum(snode_b_list) <= 1)
                # shard on snodes should match shards on rnodes
                self.opt.add(Sum(snode_b_list) == Sum(rnode_b_list))

    def rule_rnode_has_dependencies(self):
        if self.args.verbose:
            print("rule_rnode_has_dependencies:", round(time.time() - self.start_time, 5))
        # each race node must have one instance of each type of dependency
        # generate list of module types
        deptypes = set(dep["deptype"] for dep in deps)
        for deptype in deptypes:
            for n in range(len(self.rnodes)):
                d = [i for i in range(len(self.deps)) if deps[i]["deptype"] == deptype]
                # get all sum variables for instances of modules of deptype
                all_sum_vars = [
                    Sum(
                        [
                            If(
                                Sum(
                                    [
                                        If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                                        for k in range(len(self.deps[i]["instances"][j]["shards"]))
                                    ]
                                )
                                >= self.deps[i]["instances"][j]["n"],
                                1,
                                0,
                            )
                            for j in range(len(self.deps[i]["instances"]))
                        ]
                    )
                    for i in d
                ]
                # sum up number of instances of deptype for current rnode
                num_instances = Sum([If(i > 0, 1, 0) for i in all_sum_vars])
                self.opt.add(num_instances == 1)

    # def opt_subnet(self):
    #     total_shards = Sum([n for n in self.rnode_shard_sum.values()])

    #     for n in range(len(self.rnodes)):
    #         s_in_sub = Sum([self.rnode_shard_sum[rn]
    #                         for rn in range(len(self.rnodes))
    #                         if in_subnet(self.rnodes[n]['address'], self.rnodes[rn]['address'])])
    #         addresses_in_subnet = get_subnet(self.rnodes[n]['address']).num_addresses
    #         self.opt.maximize(
    #             s_in_sub / )

    def opt_min_subnet_spread(self):
        if self.args.verbose:
            print("opt_min_subnet_spread:", round(time.time() - self.start_time, 5))

        for n in range(len(self.rnodes)):
            subnet = get_subnet(self.rnodes[n]["address"])
            addresses_in_subnet = subnet.num_addresses
            addresses_used_in_subnet = Sum(
                [
                    If(
                        Sum(
                            [
                                If(self.rnode_has_dep_shard[rn][i][j][k], 1, 0)
                                for i in range(len(self.deps))
                                for j in range(len(self.deps[i]["instances"]))
                                for k in range(len(self.deps[i]["instances"][j]["shards"]))
                            ]
                        )
                        > 0,
                        1,
                        0,
                    )
                    for rn in range(len(self.rnodes))
                    if ipaddress.ip_address(resolve(self.rnodes[rn]["address"])) in subnet
                ]
            )
            self.opt.minimize(addresses_used_in_subnet / addresses_in_subnet)

    def opt_max_internet_spread(self):
        if self.args.verbose:
            print("opt_max_internet_spread:", round(time.time() - self.start_time, 5))

        addresses_in_internet = 2**32
        addresses_used = Sum(
            [
                If(
                    Sum(
                        [
                            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
                            for i in range(len(self.deps))
                            for j in range(len(self.deps[i]["instances"]))
                            for k in range(len(self.deps[i]["instances"][j]["shards"]))
                        ]
                    )
                    > 0,
                    1,
                    0,
                )
                for n in range(len(self.rnodes))
            ]
        )
        self.opt.maximize(addresses_used / addresses_in_internet)

    def opt_min_shards_rnodes(self):
        if self.args.verbose:
            print("opt_min_shards_rnodes:", round(time.time() - self.start_time, 5))
        # minimize shards held by race nodes
        rnode_shard_list = [
            If(self.rnode_has_dep_shard[n][i][j][k], 1, 0)
            for n in range(len(self.rnodes))
            for i in range(len(self.deps))
            for j in range(len(self.deps[i]["instances"]))
            for k in range(len(self.deps[i]["instances"][j]["shards"]))
        ]
        if rnode_shard_list:
            self.opt.minimize(Sum(rnode_shard_list))

    def opt_max_shards_snodes(self):
        if self.args.verbose:
            print("opt_max_shards_snodes:", round(time.time() - self.start_time, 5))
        # maximize shards held by server nodes
        snode_shard_list = [
            If(self.snode_has_dep_shard[n][i][j][k], 1, 0)
            for n in range(len(self.snodes))
            for i in range(len(self.deps))
            for j in range(len(self.deps[i]["instances"]))
            for k in range(len(self.deps[i]["instances"][j]["shards"]))
        ]
        if snode_shard_list:
            self.opt.maximize(Sum(snode_shard_list))

    def opt_properties_deps(self):
        if self.args.verbose:
            print("opt_properties_deps:", round(time.time() - self.start_time, 5))
        if len(self.deps) == 0:
            return
        # minimize/maximize properties of shards
        # TODO minimize for blobs too
        # for rnode_has_dep_sum
        for prop in self.deps[0]["properties"]["maximize"]:
            weight_list = [
                If(
                    self.rnode_has_dep_shard[n][i][j][k],
                    (self.deps[i]["properties"]["maximize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.rnodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.maximize(Sum(weight_list))

        for prop in self.deps[0]["properties"]["minimize"]:
            weight_list = [
                If(
                    self.rnode_has_dep_shard[n][i][j][k],
                    (self.deps[i]["properties"]["minimize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.rnodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.minimize(Sum(weight_list))

        # for snode_has_dep_sum
        for prop in self.deps[0]["properties"]["maximize"]:
            weight_list = [
                If(
                    self.snode_has_dep_shard[n][i][j][k],
                    (self.deps[i]["properties"]["maximize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.snodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.maximize(Sum(weight_list))

        for prop in self.deps[0]["properties"]["minimize"]:
            if self.args.verbose:
                print(f"\tminimize: {prop}")
            weight_list = [
                If(
                    self.snode_has_dep_shard[n][i][j][k],
                    (self.deps[i]["properties"]["minimize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.snodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.minimize(Sum(weight_list))

    def opt_properties_rnodes(self):
        if self.args.verbose:
            print("opt_properties_rnodes:", round(time.time() - self.start_time, 5))
        if len(self.rnodes) == 0:
            return
        # minimize/maximize properties of race nodes
        for prop in self.rnodes[0]["properties"]["maximize"]:
            if self.args.verbose:
                print(f"\tmaximize: {prop}")
            weight_list = [
                If(
                    self.rnode_has_dep_shard[n][i][j][k],
                    (self.rnodes[i]["properties"]["maximize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.rnodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.maximize(Sum(weight_list))

        for prop in self.rnodes[0]["properties"]["minimize"]:
            if self.args.verbose:
                print(f"\tminimize: {prop}")
            weight_list = [
                If(
                    self.rnode_has_dep_shard[n][i][j][k],
                    (self.rnodes[i]["properties"]["minimize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.rnodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.minimize(Sum(weight_list))

    def opt_properties_snodes(self):
        if self.args.verbose:
            print("opt_properties_snodes:", round(time.time() - self.start_time, 5))
        if len(self.snodes) == 0:
            return
        # # minimize/maximize properties of shard nodes
        for prop in self.snodes[0]["properties"]["maximize"]:
            if self.args.verbose:
                print(f"\tmaximize: {prop}")
            weight_list = [
                If(
                    self.snode_has_dep_shard[n][i][j][k],
                    (self.snodes[i]["properties"]["maximize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.snodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.maximize(Sum(weight_list))

        for prop in self.snodes[0]["properties"]["minimize"]:
            if self.args.verbose:
                print(f"\tminimize: {prop}")
            weight_list = [
                If(
                    self.snode_has_dep_shard[n][i][j][k],
                    (self.snodes[i]["properties"]["minimize"][prop] + self.offset),
                    0,
                )
                for n in range(len(self.snodes))
                for i in range(len(self.deps))
                for j in range(len(self.deps[i]["instances"]))
                for k in range(len(self.deps[i]["instances"][j]["shards"]))
            ]
            if weight_list:
                self.opt.minimize(Sum(weight_list))


# -------------------------------------------------------------------------------
def random_spread(deps, rnodes, snodes, args):
    for n in rnodes:
        for d in deps:
            create_instance(d)
    update_rnodes(rnodes, snodes, deps, simple=True)
    update_snodes(snodes, deps, simple=True)


def optimize(deps, rnodes, snodes, args):
    opt = Optimizer(deps, rnodes, snodes, args)
    m = opt.create_opt_model()
    while not (m.check() and m.model()):
        print("m.mode():", m.model())
        print("Satisfiability Check FAILED")
        # new_deps = list()
        for n in rnodes:
            for d in deps:
                create_instance(d)
            # if args.simple:
            #     # if simple, only need one instance of each dependency
            #     break
        #                new_deps.append(create_instance(d))
        #        deps = new_deps
        m = opt.create_opt_model(deps, rnodes, snodes)
        print("Checking satisfiability...")

    print("Satisfiability Check PASSED")

    # pre_opt_model = m.model()
    # print("Model:", pre_opt_model)
    # print("Adding optimization constraints to model...")
    # opt.optimize_functions()
    # m.check()
    # print("Optimizing model...")
    # print("Model optimized")

    model = m.model()
    print("Model:", model)

    # print(model == pre_opt_model)

    # m_sorted = sorted([(str(d), str(m[d]))
    #                    for d in m if "sum" in str(d)], key=lambda x: str(x[0]))
    # print(json.dumps(m_sorted, indent=4))

    update_rnodes(rnodes, snodes, deps, model=model, opt=opt)
    update_snodes(snodes, deps, model=model, opt=opt)
    print(json.dumps(rnodes, indent=4))
    print(json.dumps(snodes, indent=4))


# -------------------------------------------------------------------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-s", "--simple", action="store_true")
    parser.add_argument("regs")
    parser.add_argument("deps")
    parser.add_argument("rnodes")
    parser.add_argument("snodes")
    args = parser.parse_args()
    # args = parser.parse_args(
    #     ["data/regs.json", "data/deps.json", "data/rnodes.json", "data/snodes.json"])
    # ---------------------------------------
    os.path.exists(args.regs)
    os.path.isfile(args.regs)
    regs = json.load(open(args.regs, "r"))
    if args.verbose:
        print(json.dumps(regs, indent=4))

    os.path.exists(args.deps)
    os.path.isfile(args.deps)
    deps = json.load(open(args.deps, "r"))
    if args.verbose:
        print(json.dumps(deps, indent=4))

    os.path.exists(args.rnodes)
    os.path.isfile(args.rnodes)
    rnodes = json.load(open(args.rnodes, "r"))
    if args.verbose:
        print(json.dumps(rnodes, indent=4))

    os.path.exists(args.snodes)
    os.path.isfile(args.snodes)
    snodes = json.load(open(args.snodes, "r"))
    if args.verbose:
        print(json.dumps(snodes, indent=4))

    if args.simple:
        random_spread(deps, rnodes, snodes, args)

    else:
        optimize(deps, rnodes, snodes, args)

    write_updated_json(args.regs, regs)
    write_updated_json(args.deps, deps)
    write_updated_json(args.rnodes, rnodes)
    write_updated_json(args.snodes, snodes)

    # ---------------------------------------
    # TODO integrate registry tracking/tasking
    # TODO push shards as necessary
    # push_shards(rnodes)

    # ---------------------------------------
    # sys.exit()
# -------------------------------------------------------------------------------
