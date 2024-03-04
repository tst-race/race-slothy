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

PLUGIN_ZIPS_DIR=""
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
        --plugin-to-shard-dir)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing directors of plugins to shard" >&2
            exit 1
        fi
        PLUGIN_ZIPS_DIR="$2"
        shift
        shift
        ;;
        --plugin-to-shard-dir=*)
        PLUGIN_ZIPS_DIR="${1#*=}"
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


if [ -z "${PLUGIN_ZIPS_DIR}" ]; then
    formatlog "ERROR" "--plugin-to-shard-dir required, Exiting"
    exit 1
fi


formatlog "INFO" "plugin zips dir: ${PLUGIN_ZIPS_DIR}"

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

# Clear existing plugins (if any)
URL=$BASE_URL/config/dependencies/
curl -X POST --insecure \
     -S --cookie cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F "json=[]" \
     $URL

# This block of code iterates through each dependency and uploads them
for PLUGIN in "$PLUGIN_ZIPS_DIR"/*
do
  FILENAME="$(basename ${PLUGIN})"
  URL=$BASE_URL/upload/
  formatlog "INFO" "Uploading Plugin ${FILENAME} from path ${PLUGIN}"
  curl -X POST --insecure \
       -S --cookie cookie.txt \
       -H "Referer: $URL" \
       -H "X-CSRFToken: $TOKEN" \
       -F "uuid=`uuidgen 2>/dev/null || cat /proc/sys/kernel/random/uuid`" \
       -F "deptype=$FILENAME" \
       -F "stability=1" \
       -F "speed=1" \
       -F "covertness=1" \
       -F "cost=1" \
       -F "filename=@$PLUGIN" \
       $URL
done

# trigger the solve, shard creation, and shard pushing
URL=$BASE_URL/deploy/ # for deploying with optimization
# URL=$BASE_URL/randomize/ # for deploying fast with randomized shard placement
curl -X GET --insecure \
     -S --cookie cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     $URL

rm cookie.txt

exit 0

