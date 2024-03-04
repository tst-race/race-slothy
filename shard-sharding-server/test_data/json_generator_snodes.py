#!/usr/bin/env python

import uuid
import json

from jinja2 import Template
from argparse import ArgumentParser


def generate_json(args):
    snode_template = Template(
        """{
        "uuid": "{{ uuid }}",
        "address": "{{ snode_address }}",
        "properties": {
            "maximize": {
                "stability": {{ range(0, 101) | random  / 10}},
                "speed": {{ range(0, 101) | random  / 10}}
            },
            "minimize": {
                "scrutiny": {{ range(0, 101) | random  / 10}},
                "oppression": {{ range(0, 101) | random  / 10}}
            }
        },
        "method": "https",
        "capacity": {{ range(1000000, 1000000000) | random }},
        "shards": []
    }"""
    )

    snodes_json = []

    for snode_address in args.snode_addresses:
        snode_item = snode_template.render(uuid=uuid.uuid4(), snode_address=snode_address)
        snodes_json.append(json.loads(snode_item))

    if args.verbose:
        print(json.dumps(snodes_json, indent=4))

    with open(args.outfile, "w") as f:
        json.dump(snodes_json, f, indent=4)


parser = ArgumentParser()
parser.add_argument(
    "snode_addresses", nargs="*", help="IPs/hostnames of race server nodes (shard hosts)"
)
parser.add_argument("--outfile", "-o", default="snodes.json")
parser.add_argument("--verbose", "-v", action="store_true")
args = parser.parse_args()

if args.verbose:
    print(args)

generate_json(args)
