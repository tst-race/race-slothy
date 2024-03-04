#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Generate docker compose for slothy services
#
# Note: This just calls the generate_docker_compose.py script. This file is a wrapped 
# to have a standardized bash interface with other plugins
#
# Example Call:
#    bash generate_docker_compose.sh {arguments}
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


python3 ${BASE_DIR}/generate_docker_compose.py "$@"
