#!/usr/bin/env python3
"""
Purpose:
    Generate the slothy docker-compose based on number of shard hosts.

    Will take in --range-config arguments to generate 
    configs against.

    --local will ignore hostnames in range-config

    Note: if config is not empty, --override will need
    to be run to remove old configs

Steps:
    - Parse CLI args
    - Load templates for shard host, registry, and sharding servers
    - Generate docker-compose data
    - Store docker-compose in same directory

usage:
    generate_configs.py \
        [-h] \
        --shard-host-count NUMBER_OF_SHARD_HOSTS \
        [--overwrite] \

example call:
    ./generate_docker_compose.py \
        --shard-host-count=3
"""

# Python Library Imports
import argparse
import copy
import json
import logging
import math
import os
import sys
from typing import Any, Dict, List, Optional, Tuple

# Local Lib Imports
import slothy_config_utils

###
# Main Execution
###


def main() -> None:
    """
    Purpose:
        Generate configs for Slothy
    Args:
        N/A
    Returns:
        N/A
    Raises:
        Exception: Config Generation fails
    """
    logging.info("Starting Process To Generate RACE Configs")

    # Parsing Configs
    cli_args = get_cli_arguments()

    # Generate Docker Compose File
    generate_shard_hosts_docker_compose(shard_host_count=cli_args.shard_host_count)
    
    logging.info("Process To Generate Slothy docker-compose Complete")

###
# Generate Docker Compose Functions
###


def generate_shard_hosts_docker_compose(
    shard_host_count: int,
) -> None:
    """
    Purpose:
        Generate docker compose for x number of shard hosts
    Args:
        shard_host_count: number of shard hosts
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    # Template Dirs
    compose_template_dir = f"{os.path.dirname(os.path.realpath(__file__))}/../runtime-scripts"

    # Create Base Compose Data
    docker_compose_data = {
        "version": '3.7',
        "networks":{
            "rib-overlay-network":{
                "external": True
            },
        },
        "x-logging": {
            "driver": "json-file",
            "options":{
                "max-file": "5",
                "max-size": "1m"
            }
        },
        "services": {},
    }

    # Add Shard Registry Server (Always only 1)
    slothy_registry_template= (
        f"{compose_template_dir}/slothy-shard-registry.yml"
    )
    slothy_registry_service = slothy_config_utils.load_file_into_memory(
        slothy_registry_template, data_format="yaml"
    )
    docker_compose_data["services"].update(slothy_registry_service)

    # Add Shard Sharding Server (Always only 1)
    slothy_sharding_template = (
        f"{compose_template_dir}/slothy-shard-sharding.yml"
    )
    slothy_sharding_service = slothy_config_utils.load_file_into_memory(
        slothy_sharding_template, data_format="yaml"
    )
    docker_compose_data["services"].update(slothy_sharding_service)

    shard_hosts_hostnames = slothy_config_utils.get_shard_host_hostnames(shard_host_count=shard_host_count)
    for hostname in shard_hosts_hostnames:
        # Add Shard Hosts
        slothy_host_template = f"{compose_template_dir}/slothy-shard-host.yml"
        slothy_host_template_configs = {
            "shard_host_name": hostname,
        }
        slothy_host_service = slothy_config_utils.format_yaml_template(
            slothy_host_template, slothy_host_template_configs
        )
        docker_compose_data["services"].update(slothy_host_service)
    slothy_config_utils.write_yaml(filename=f"{compose_template_dir}/docker-compose.yml", data=docker_compose_data)


###
# Helper Functions
###


def get_cli_arguments() -> argparse.Namespace:
    """
    Purpose:
        Parse CLI arguments for script
    Args:
        N/A
    Return:
        cli_arguments (ArgumentParser Obj): Parsed Arguments Object
    """
    logging.info("Getting and Parsing CLI Arguments")

    parser = argparse.ArgumentParser(description="Generate RACE Config Files")
    required = parser.add_argument_group("Required Arguments")
    optional = parser.add_argument_group("Optional Arguments")

    # Optional Arguments
    required.add_argument(
        "--shard-host-count",
        dest="shard_host_count",
        help="Number of shard hosts",
        required=False,
        default=3,
        type=int,
    )
    return parser.parse_args()


###
# Entrypoint
###


if __name__ == "__main__":

    LOG_LEVEL = logging.INFO
    logging.getLogger().setLevel(LOG_LEVEL)
    logging.basicConfig(
        stream=sys.stdout,
        level=LOG_LEVEL,
        format="[generate_configs] %(asctime)s.%(msecs)03d %(levelname)s %(message)s",
        datefmt="%a, %d %b %Y %H:%M:%S",
    )

    try:
        main()
    except Exception as err:
        print(f"{os.path.basename(__file__)} failed due to error: {err}")
        raise err
