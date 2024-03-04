#!/usr/bin/env python3
"""
Purpose:
    Generate the slothy configs based on a provided range config and save 
    config files to the specified config directory.

    Will take in --range-config arguments to generate 
    configs against.

    --local will ignore hostnames in range-config

    Note: if config is not empty, --override will need
    to be run to remove old configs

Steps:
    - Parse CLI args
    - Check for existing configs
        - remove if --override is set and configs exist
        - fail if --override is not set and configs exist
    - Load and Parse Range Config File
    - Generate configs for the plugin
    - Store configs in the specified config directory

usage:
    generate_configs.py \
        [-h] \
        --range RANGE_CONFIG_FILE \
        [--overwrite] \
        [--config-dir CONFIG_DIR]

example call:
    ./generate_plugin_configs.py \
        --range=./2x2.json \
        --config-dir=./config \
        --overwrite
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

RACE_UTILS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/../race-python-utils"
sys.path.insert(0, RACE_UTILS_PATH)
from race_python_utils import file_utils, range_config_utils


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

    # Load and validate Range Config
    range_config = file_utils.read_json(cli_args.range_config_file)
    range_config_utils.validate_range_config(range_config)

    # Generate shard host hostnames config and race nodes config
    generate_configs(range_config=range_config, shard_host_count=cli_args.shard_host_count, config_dir=cli_args.config_dir, local_override=cli_args.local_override)

    logging.info("Process To Generate Slothy Plugin Configs Complete")


###
# Generate Config Functions
###

def generate_configs(
    range_config: Dict[str, Any],
    shard_host_count: int,
    config_dir: str,
    local_override: bool,
) -> None:
    """
    Purpose:
        Generate the plugin configs based on range config
    Args:
        range_config: range config to generate against
        shard_host_count: number of shard hosts
        config_dir: where to store configs
        local_override: Ignore range config services.
    Raises:
        Exception: generation fails
    Returns:
        None
    """

    # create directory if it doesn't exist
    if not os.path.isdir(config_dir):
        os.makedirs(config_dir)

    # Get RACE Nodes
    race_clients = range_config_utils.get_client_details_from_range_config(range_config=range_config)
    race_servers = range_config_utils.get_server_details_from_range_config(range_config=range_config)
    all_race_nodes = set(
            list(race_clients.keys())
            + list(race_servers.keys())
        )
    # Generate RACE Nodes config file
    slothy_race_nodes_config_file = f"{config_dir}/slothy_race_nodes_config_file.json"
    slothy_config_utils.generate_slothy_race_nodes_config(race_nodes=all_race_nodes, outfile=slothy_race_nodes_config_file)

    shard_host_hostnames = slothy_config_utils.get_shard_host_hostnames(shard_host_count=shard_host_count, range_config=range_config, local_override=local_override)

    # Generate Shard Host Hostnames config file
    slothy_hosts_hostnames_config_file = f"{config_dir}/shard_hosts_hostnames.json"
    slothy_config_utils.generate_slothy_shard_hosts_config(shard_host_nodes=shard_host_hostnames, outfile=slothy_hosts_hostnames_config_file)


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
        "--range",
        dest="range_config_file",
        help="Range config of the physical network",
        required=False,
        default="",
        type=str,
    )
    required.add_argument(
        "--shard-host-count",
        dest="shard_host_count",
        help="Number of shard hosts",
        required=False,
        default=3,
        type=int,
    )
    optional.add_argument(
        "--overwrite",
        dest="overwrite",
        help="Overwrite configs if they exist",
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--local",
        dest="local_override",
        help=(
            "Ignore range config service connectivity, utilized "
            "local configs (e.g. local hostname/port vs range services fields). "
            "Does nothing for Direct Links at the moment"
        ),
        required=False,
        default=False,
        action="store_true",
    )
    optional.add_argument(
        "--config-dir",
        dest="config_dir",
        help="Where should configs be stored",
        required=False,
        default="./configs",
        type=str,
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
