#!/usr/bin/env python

import uuid
import json

from jinja2 import Template
from argparse import ArgumentParser


def generate_json(args):
    dep_template = Template(
        """{
        "uuid": "{{ uuid }}",
        "name": "{{ dep_name }}",
        "filename": "/root/scatter_server/media/{{ dep_name }}",
        "deptype": "{{ dep_name }}",
        "properties": {
            "maximize": {
                "stability": {{ range(0, 101) | random  / 10}},
                "speed": {{ range(0, 101) | random  / 10}},
                "covertness": {{ range(0, 101) | random  / 10}}
            },
            "minimize": {
                "cost": {{ range(0, 101) | random  / 10}}
            }
        },
        "instances": []
    }"""
    )

    deps_json = []

    for dep in args.deps:
        dep_item = dep_template.render(uuid=uuid.uuid4(), dep_name=dep)
        deps_json.append(json.loads(dep_item))

    if args.verbose:
        print(json.dumps(deps_json, indent=4))

    with open(args.outfile, "w") as f:
        json.dump(deps_json, f, indent=4)


parser = ArgumentParser()
parser.add_argument("deps", nargs="*", help="names of dependencies")
parser.add_argument("--outfile", "-o", default="deps.json")
parser.add_argument("--verbose", "-v", action="store_true")
args = parser.parse_args()

if args.verbose:
    print(args)

generate_json(args)
