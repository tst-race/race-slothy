# -----------------------------------------------------------------------------
# Helper functions
#
# Script is used to declare functions for other bash scripts, does nothing 
# when called directly
# -----------------------------------------------------------------------------


#
# Exit if called directly
#
if [ ${0##*/} == ${BASH_SOURCE[0]##*/} ]; then 
    RED='\033[0;31m'
    NO_COLOR='\033[0m'
    printf "${RED}%s (%s): %s${NO_COLOR}\n" "$(date +%c)" "ERROR" "This script cannot be executed directly! Run build.sh instead."
    exit 1
fi


#
# Formatlog wraps print statments with logging level, color, and current time
#
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

