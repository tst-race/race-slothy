#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Script to configure slothy shard registry
# -----------------------------------------------------------------------------

set -e

CALL_NAME="$0"

###
# Helper functions
###

# BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null2>&1 && pwd)

formatlog() {
    LOG_LEVELS=("DEBUG" "INFO" "WARNING" "ERROR")
    if [ "$1" = "ERROR" ]; then
        RED='\033[0;31m'
        NO_COLOR='\033[0m'
        printf "${RED}%s (%s): %s${NO_COLOR}\n" "$(date +%c)" "${1}" "${2}"
    elif [ "$1" = "WARNING" ]; then
        YELLOW='\033[0;33m'
        NO_COLOR='\033[0m'
        printf "${YELLOW}%s (%s): %s${NO_COLOR}\n" "$(date +%c)" "${1}" "${2}"
    elif [ ! "$(echo "${LOG_LEVELS[@]}" | grep -co "${1}")" = "1" ]; then
        echo "$(date +%c): ${1}"
    else
        echo "$(date +%c) (${1}): ${2}"
    fi
}


###
# Arguments
###

RACE_NODES_CONFIG_FILE=""
SHARD_HOST_NODES_CONFIG_FILE=""
BASE_URL="https://shard-sharding:8000"

HELP=\
"Script to configure slothy shard registry

Arguments:
    --race-version [value], --race-version=[value]
        The version of race to use when creating a deployment. Defaults to ${RACE_VERSION}.
    --deployment-name [value], --deployment-name=[value]
        The name of the test deployment. Defaults to ${DEPLOYMENT_NAME}.
    -h, --help
        Print this message.

Examples:
    N/A - This is meant to be called by RiB
"

while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        --race-nodes-config-file)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing race nodes config file" >&2
            exit 1
        fi
        RACE_NODES_CONFIG_FILE="$2"
        shift
        shift
        ;;
        --race-nodes-config-file=*)
        RACE_NODES_CONFIG_FILE="${1#*=}"
        shift
        ;;

        --shard-host-config-file)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing Shard Hosts Config File" >&2
            exit 1
        fi
        SHARD_HOST_NODES_CONFIG_FILE="$2"
        shift
        shift
        ;;
        --shard-host-config-file=*)
        SHARD_HOST_NODES_CONFIG_FILE="${1#*=}"
        shift
        ;;

        --base-url)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing base url" >&2
        fi
        BASE_URL="$2"
        shift
        shift
        ;;
        --base-url=*)
        BASE_URL="${1#*=}"
        shift
        ;;
        
        -h|--help)
        printf "%s" "${HELP}"
        shift
        exit 1;
        ;;

        "")
        # empty string. do nothing.
        shift
        ;;

        *)
        formatlog "ERROR" "${CALL_NAME} unknown argument \"$1\""
        exit 1
        ;;
    esac
done

if [ -n "${NO_FAIL}" ] ; then
    # unset -e
    set +e
fi


if [ -z "${RACE_NODES_CONFIG_FILE}" ]; then
    formatlog "ERROR" "--race-nodes-config-file required, Exiting"
    exit 1
fi

if [ -z "${SHARD_HOST_NODES_CONFIG_FILE}" ]; then
    formatlog "ERROR" "--shard-host-config-file required, Exiting"
    exit 1
fi

formatlog "INFO" "shard host nodes config file: ${SHARD_HOST_NODES_CONFIG_FILE}"
formatlog "INFO" "race nodes config file: ${RACE_NODES_CONFIG_FILE}"

if [ -n "${TRACE}" ] ; then
    set -x
fi

###
# Main Execution
###
# Get Token
# loop while $TOKEN is not set. It will loop until shard-sharding-server has started serving on localhost:8000
unset TOKEN
while ! [ -n "$TOKEN" ]; do
  curl --insecure -c cookie.txt $BASE_URL || echo 'waiting for shard-sharding-server to start on localhost:8000' && sleep 1
  TOKEN=`grep csrftoken cookie.txt | awk '{print $7}'`
done <<<$(find . -type f)
formatlog "INFO" "Token found. TOKEN=$TOKEN"

# POST conf/snodes.json to update shard-sharding-server's config
URL=$BASE_URL/config/server_nodes/
curl -X POST --insecure \
     -S --cookie cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F "json=`cat $SHARD_HOST_NODES_CONFIG_FILE`" \
     $URL
formatlog "INFO" "posting snodes config file $SHARD_HOST_NODES_CONFIG_FILE"

# POST conf/rnodes.json to update shard-sharding-server's config
URL=$BASE_URL/config/race_nodes/
curl -X POST --insecure \
     -S --cookie cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F "json=`cat $RACE_NODES_CONFIG_FILE`" \
     $URL
formatlog "INFO" "posting rnodes config file $RACE_NODES_CONFIG_FILE"

formatlog "INFO" "removing cookie"
rm cookie.txt
exit 0