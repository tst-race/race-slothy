#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Generate configs for the plugin
#
# Note: This just calls the generate_configs.py script. This file is a wrapped 
# to have a standardized bash interface with other plugins
#
# Example Call:
#    bash generate_configs.sh {arguments}
# -----------------------------------------------------------------------------


set -e


###
# Arguments
###


# Get Path
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)


###
# Main Execution
###


python3 ${BASE_DIR}/generate_configs.py "$@"
ONLY_SHARD_HOST_COUNT_ARG=`echo "$@" | grep -oP '\-\-shard-host-count\s\w+' || echo ""`
bash ${BASE_DIR}/generate_docker_compose.sh $ONLY_SHARD_HOST_COUNT_ARG