#!/usr/bin/env bash

# 
# Copyright 2023 Two Six Technologies
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 

# -----------------------------------------------------------------------------
# Push a set of artifacts to jfog.
#
# Note, it is expected that you have Jfrog already configured to interact
# with https://jfrog.race.twosixlabs.com/. please run `jr
#
# Arguments:
# --race-version [value], --race=[value]
#     Specify the RACE version of the build to push
# --app-revision [value], --app-revision=[value]
#     Specify the revision of the build to push
# --env [value], --env=[value]
#     Env (dev|prod) to push artifacts to in artifactory
# -h, --help
#     Print help and exit
#
# Example Call:
#    ./push_app_to_artifactory.sh \
#        --race-version=1.4.0 \
#        --app-revision=r1 \
#        {--help}
# -----------------------------------------------------------------------------


set -e


###
# Helper functions
###


# Load Helper Functions
CURRENT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${CURRENT_DIR}/helper_functions.sh



###
# Arguments
###


# Artifactory Values
PERFORMER="ta31_perspecta"
ENV="dev"
BUILD_NUMBER="1.0.4"

# App Value
APP_ID="SlothyRegistry"

# Version values
RACE_VERSION="2.4.0"
APP_REVISION="r1"

# Parse CLI Arguments
while [ $# -gt 0 ]
do
    key="$1"

    case $key in
        --race-version)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing RACE version number" >&2
            exit 1
        fi
        RACE_VERSION="$2"
        shift
        shift
        ;;
        --race-version=*)
        RACE_VERSION="${1#*=}"
        shift
        ;;
        
        --app-revision)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing revision number" >&2
            exit 1
        fi
        APP_REVISION="$2"
        shift
        shift
        ;;
        --app-revision=*)
        APP_REVISION="${1#*=}"
        shift
        ;;
        
        --build-number)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing build number" >&2
            exit 1
        fi
        BUILD_NUMBER="$2"
        shift
        shift
        ;;
        --build-number=*)
        BUILD_NUMBER="${1#*=}"
        shift
        ;;

        --app-id)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing app ID" >&2
            exit 1
        fi
        APP_ID="$2"
        shift
        shift
        ;;
        --app-id=*)
        APP_ID="${1#*=}"
        shift
        ;;

        --env)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing env" >&2
            exit 1
        fi
        ENV="$2"
        shift
        shift
        ;;
        --env=*)
        ENV="${1#*=}"
        shift
        ;;

        -h|--help)
        echo "Example Call: bash push_app_to_artifactory.sh --race=2.1.0 --revision=r1 {--help}"
        exit 1;
        ;;

        *)
        formatlog "ERROR" "unknown argument \"$1\""
        exit 1
        ;;
    esac
done

if [ -z "${APP_REVISION}" ]; then
    formatlog "ERROR" "app revision must be a non-empty string"
    exit 1
fi

ARTIFACTORY_REPO="${PERFORMER}-generic-${ENV}-local"
ARTIFACTORY_APP_PATH="${ARTIFACTORY_REPO}/race/${RACE_VERSION}/ta3/${APP_ID}/${APP_REVISION}"

BUILD_NAME="${ARTIFACTORY_APP_PATH}"
if [ -z "${BUILD_NUMBER}" ]; then
    BUILD_NUMBER="0"
fi

###
# Main Execution
###


# Verify Jfrog Configuration (Fails if Not Configured)
jfrog config use race

# Verify Artifacts Exist
if [ -z "$(ls ${CURRENT_DIR}/out/app/artifacts/linux-x86_64-client/registry/install.sh)" ]; then
    formatlog "ERROR" "Missing install.sh script, please add it"
    exit 1
fi

if [ -z "$(ls ${CURRENT_DIR}/out/app/artifacts/linux-x86_64-client/registry/bin/server.key)" ] ||
    [ -z "$(ls ${CURRENT_DIR}/out/app/artifacts/linux-x86_64-client/registry/bin/server.cert)" ]; then
    formatlog "ERROR" "Missing server self-signed certs, please add them"
    exit 1
fi
if [ -z "$(ls ${CURRENT_DIR}/out/app/artifacts/linux-x86_64-client/registry/bin/raceregistry)" ]; then
    formatlog "ERROR" "Missing Linux Artifacts; Please build the artifacts"
    exit 1
fi
if [ -z "$(ls ${CURRENT_DIR}/out/app/app-manifest.json)" ]; then
    formatlog "ERROR" "Missing App Manifest; Please add the artifact"
    exit 1
fi

# Navigate to App Dir
cd ${CURRENT_DIR}/out/app

# Upload the app dir to the specified location in Artifactory
jfrog rt upload \
    "./*" \
    "${ARTIFACTORY_APP_PATH}/" \
    --build-name="${BUILD_NAME}" \
    --build-number="${BUILD_NUMBER}" \
    --flat=false \
    --recursive \
    --exclusions="*DS_Store;*gitignore"

if [[ "$BUILD_NUMBER" != "" ]]; then
    # Collect environment variables and add them to the build info:
    jfrog rt build-collect-env $BUILD_NAME $BUILD_NUMBER

    # Publish the build info to Artifactory:
    jfrog rt build-publish $BUILD_NAME $BUILD_NUMBER
fi
