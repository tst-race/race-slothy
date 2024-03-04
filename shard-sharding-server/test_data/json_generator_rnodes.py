#!/usr/bin/env python

import uuid
import json

from jinja2 import Template
from argparse import ArgumentParser


def generate_json(args):
    rnode_template = Template(
        """{
        "uuid": "{{ uuid }}",
        "address": "{{ rnode_address }}",
        "registry": "7ee9440f-04a6-470b-addb-183f11ef0302",
        "properties": {
            "maximize": {
                "covertness": {{ range(0, 101) | random  / 10}},
                "trust": {{ range(0, 101) | random  / 10}}
            },
            "minimize": {
                "scrutiny": {{ range(0, 101) | random  / 10}},
                "oppression": {{ range(0, 101) | random  / 10}}
            }
        },
        "shards": []
    }"""
    )

    rnodes_json = []

    for rnode_address in args.rnode_addresses:
        rnode_item = rnode_template.render(uuid=uuid.uuid4(), rnode_address=rnode_address)
        rnodes_json.append(json.loads(rnode_item))

    if args.verbose:
        print(json.dumps(rnodes_json, indent=4))

    with open(args.outfile, "w") as f:
        json.dump(rnodes_json, f, indent=4)


parser = ArgumentParser()
parser.add_argument("rnode_addresses", nargs="*", help="IPs/hostnames of race client nodes")
parser.add_argument("--outfile", "-o", default="rnodes.json")
parser.add_argument("--verbose", "-v", action="store_true")
args = parser.parse_args()

if args.verbose:
    print(args)

generate_json(args)
