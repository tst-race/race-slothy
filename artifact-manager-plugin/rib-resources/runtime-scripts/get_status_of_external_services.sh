#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Get status of external (not running in a RACE node) services required by plugin.
# Intent is to ensure that the channel will be functional if a RACE deployment
# were to be started and connections established
#
# Note: For Two Six Exemplar, need to check the status of the two six file server
#
# Arguments:
# -h, --help
#     Print help and exit
#
# Example Call:
#    bash get_status_of_external_services.sh \
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
        echo "Example Call: bash get_status_of_external_services.sh"
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


formatlog "INFO" "Printing compose status."
docker-compose -p slothy-services -f "${BASE_DIR}/docker-compose.yml" ps

formatlog "INFO" "Getting Expected Container Statuses. slothy-services needs to be running"
# TODO, once we have health checks, we need to add '--filter "health=healthy"' to the status commands

SERVICES_ALL=$(docker-compose -p slothy-services -f "${BASE_DIR}/docker-compose.yml" ps -aq)
SERVICES_RUNNING=$(docker-compose -p slothy-services -f "${BASE_DIR}/docker-compose.yml" ps -aq --filter "status=running")
if [ "$SERVICES_RUNNING" != "$SERVICES_ALL" || "$SERVICES_ALL" == "" ]; then
    echo "Slothy services are not running, Exit 1"
    # exit 1
else 
    echo "Slothy services are running, Exit 0"
    # exit 0
fi

