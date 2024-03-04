#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Upload distribution artifacts to the file server
# -----------------------------------------------------------------------------

set -e


###
# Helper functions
###


# Load Helper Functions
BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${BASE_DIR}/helper_functions.sh


###
# Arguments
###


ARTIFACTS_DIR=""
BASE_URL="http://twosix-file-server:8080"

HELP=\
"Upload distribution artifacts to the file server.

Arguments:
--artifacts-dir=DIR
    Directory containing artifacts to be uploaded
--base-url=URL
    Base URL of the file server
-h, --help
    Print help and exit

Example call:
    bash upload_artifacts.sh --artifacts-dir=/path/to/artifacts
"


# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        --artifacts-dir)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing artifacts directory" >&2
            exit 1
        fi
        ARTIFACTS_DIR="$2"
        shift
        shift
        ;;
        --artifacts-dir=*)
        ARTIFACTS_DIR="${1#*=}"
        shift
        ;;
    
        --base-url)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing base URL" >&2
            exit 1
        fi
        BASE_URL="$2"
        shift
        shift
        ;;
        --base-dir=*)
        BASE_URL="${1#*=}"
        shift
        ;;

        -h|--help)
        printf "%s" "${HELP}"
        exit 1;
        ;;
        --config-dir)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "Missing configs directory" >&2
            exit 1
        fi
        # We don't care about this argument
        # CONFIG_DIR="$2"
        shift
        shift
        ;;
        --config-dir=*)
        # We don't care about this argument
        # CONFIG_DIR="${1#*=}"
        shift
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

bash ${BASE_DIR}/shard_plugins.sh --plugin-to-shard-dir "$ARTIFACTS_DIR"
