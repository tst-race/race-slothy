#!/usr/bin/env python3
"""
    Purpose:
        Slothy Utilities for config generation
"""

import uuid
import json
import yaml

from jinja2 import Template
from typing import Any, Dict, Union
from yaml import Loader as YamlLoader, Dumper as YamlDumper


def generate_slothy_shard_hosts_config(shard_host_nodes: list, outfile: str):
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

    for snode_address in shard_host_nodes:
        snode_item = snode_template.render(
            uuid=uuid.uuid4(), snode_address=snode_address
        )
        snodes_json.append(json.loads(snode_item))

    with open(outfile, "w") as f:
        json.dump(snodes_json, f, indent=4)


def generate_slothy_race_nodes_config(race_nodes: list, outfile: str):
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

    for rnode_address in race_nodes:
        rnode_item = rnode_template.render(
            uuid=uuid.uuid4(), rnode_address=rnode_address
        )
        rnodes_json.append(json.loads(rnode_item))

    with open(outfile, "w") as f:
        json.dump(rnodes_json, f, indent=4)


def get_shard_host_hostnames(shard_host_count: int, range_config: Dict[str, Any] = None, local_override: bool = True):
    shard_host_hostnames = []
    
    if local_override:
        for shard_host_index in range(shard_host_count):
            shard_host_hostnames.append(f"shard-host-{shard_host_index}")
    else:
        # Get hostnames from range config if not local (local means AWS or Local deployments, not T&E)
        shard_host_index = 0
        for service in range_config["range"]["services"]:
            if service["type"] == "shard-host":
               shard_host_hostnames.append(service["access"][0]["url"])
               shard_host_index += 1
               if shard_host_index >= shard_host_count:
                   break
    
    return shard_host_hostnames


def write_yaml(
    filename: str, data: Any
) -> None:
    """
    Purpose:
        Write given yaml data to a specified file
    Args:
        filename: Filename to write to
        data: Data to write to file
    Return:
        N/A
    Raises:
        N/A
    """

    with open(filename, "w") as file_obj:
        file_obj.write(yaml.dump(data, default_flow_style=False))


###
# Docker Compose Functions
###

def format_yaml_template(
    file_path: str, format_data: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Purpose:
        Open a yaml template file, format the contents with the provided data, and
            convert and return it as a dict.
    Args:
        file_path (str): The path of the template file.
        format_data (Dict[str, Any], optional): The data used to format the template.
            Defaults to None.

    Returns:
        Dict[str, Any]: A dict representation of the formatted yaml template.
    """

    yml_as_string = load_file_into_memory(file_path, data_format="string")
    try:
        service_data = yml_as_string.format_map(format_data)  # type: ignore
    except Exception as err:
        raise err

    return yaml.load(service_data, Loader=YamlLoader)


def load_file_into_memory(
    filename: str, data_format: str = "string"
) -> Union[Dict[str, Any], list, bytes, str]:
    """Open a file, read the contents, optionally parse the data depending on the format,
    and return it.

    Args:
        filename (str): The name of the file to open, read, and parse.Any
        data_format (str, optional): The format of the data: "json", "yaml", bytes", or "string". Defaults to "string".

    Raises:
        error_utils.RIB006: Failed to open or parse the files for some reason

    Returns:
        Union[Dict[str, Any], list, bytes, str]: The formatted contents of the file.
            Varies based on data format.
                "json" : dict representation of the json.
                "yaml" : dict, list, or some other representation of the yaml depending on the contents of the yaml file.
                "bytes" : bytes
                "string" : str contents of the file.
    """
    file_mode = "rb" if data_format == "bytes" else "r"

    with open(filename, file_mode) as file_obj:
        if data_format == "json":
            # return type is a dict.
            return json.load(file_obj)
        elif data_format == "yaml":
            # return type is a dict, list, or whatever the yaml file happens to contain.
            return yaml.load(file_obj.read(), Loader=YamlLoader)
        elif data_format == "bytes":
            # return type is bytes
            return file_obj.read()
        elif data_format == "string":
            # return type is str.
            return file_obj.read()
