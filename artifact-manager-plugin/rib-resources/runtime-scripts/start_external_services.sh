#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Start external (not running in a RACE node) services required by channel 
#
# Note: For Two Six Indirect Links, need to stand up the two six whiteboard. This
# will include running a docker-compose.yml file to up the whiteboard and API
#
# Arguments:
# -h, --help
#     Print help and exit
#
# Example Call:
#    bash start_external_services.sh \
#        {--help}
# -----------------------------------------------------------------------------


###
# Helper functions
###


# Load Helper Functions
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${BASE_DIR}/helper_functions.sh


###
# Arguments
###


# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        -h|--help)
        echo "Example Call: bash start_external_services.sh"
        exit 1;
        ;;
        *)
        formatlog "ERROR" "unknown argument \"$1\""
        exit 1
        ;;
    esac
done


###
# Main Execution
###


formatlog "INFO" "Starting Slothy Services"
docker-compose -p slothy-services -f "${BASE_DIR}/docker-compose.yml" up -d

PLUGIN_NAME=`ls $BASE_DIR/../../../configs/artifact-manager/ | grep -i slothy`

# TODO: change to use argument passed into this script instead of relative path with $PLUGIN_NAME
bash ${BASE_DIR}/configure_shard_registry.sh \
    --race-nodes-config-file $BASE_DIR/../../../configs/artifact-manager/${PLUGIN_NAME}/slothy_race_nodes_config_file.json \
    --shard-host-config-file $BASE_DIR/../../../configs/artifact-manager/${PLUGIN_NAME}/shard_hosts_hostnames.json
