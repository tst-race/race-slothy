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
CURRENT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${CURRENT_DIR}/helper_functions.sh

# Artifactory Values
PERFORMER="ta31_perspecta"
ENV="dev"
BUILD_NUMBER="1.0.4"

# App Value
PLUGIN_ID="PluginArtifactManagerSlothy"

# Version values
RACE_VERSION="2.4.0"
PLUGIN_REVISION="latest"

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
        
        --plugin-revision)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing revision number" >&2
            exit 1
        fi
        PLUGIN_REVISION="$2"
        shift
        shift
        ;;
        --plugin-revision=*)
        PLUGIN_REVISION="${1#*=}"
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

        --plugin-id)
        if [ $# -lt 2 ]; then
            formatlog "ERROR" "missing pluginId" >&2
            exit 1
        fi
        PLUGIN_ID="$2"
        shift
        shift
        ;;
        --plugin-id=*)
        PLUGIN_ID="${1#*=}"
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
        echo "Example Call: bash push_plugin_to_artifactory.sh --race-version=2.4.0 --plugin-revision=r1 {--help}"
        exit 1;
        ;;

        *)
        formatlog "ERROR" "unknown argument \"$1\""
        exit 1
        ;;
    esac

done

if [ -z "${PLUGIN_REVISION}" ]; then
    formatlog "ERROR" "app revision must be a non-empty string"
    exit 1
fi

ARTIFACTORY_REPO="${PERFORMER}-generic-${ENV}-local"
ARTIFACTORY_PLUGIN_PATH="${ARTIFACTORY_REPO}/race/${RACE_VERSION}/artifact-manager/${PLUGIN_ID}/${PLUGIN_REVISION}"

BUILD_NAME="${ARTIFACTORY_PLUGIN_PATH}"
if [ -z "${BUILD_NUMBER}" ]; then
    BUILD_NUMBER="0"
fi

###
# Main Execution
###


# Verify Jfrog Configuration (Fails if Not Configured)
jfrog config use race


if [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-client/PluginArtifactManagerSlothy/lib/libsss.so)" ] || 
   [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-client/PluginArtifactManagerSlothy/lib/libslothy.so)" ] ||
   [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-client/PluginArtifactManagerSlothy/lib/libslothy-amp.so)" ]; then
    formatlog "ERROR" "Missing client libraries; Please build the artifacts"
    exit 1
fi


if [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-server/PluginArtifactManagerSlothy/lib/libsss.so)" ] || 
   [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-server/PluginArtifactManagerSlothy/lib/libslothy.so)" ] ||
   [ -z "$(ls ${CURRENT_DIR}/out/plugin/artifacts/linux-x86_64-server/PluginArtifactManagerSlothy/lib/libslothy-amp.so)" ]; then
    formatlog "ERROR" "Missing server libraries; Please build the artifacts"
    exit 1
fi


if [ -z "$(ls ${CURRENT_DIR}/out/plugin/runtime-scripts)" ]; then
    formatlog "ERROR" "Missing runtime scripts; Please build the artifacts"
    exit 1
fi


if [ -z "$(ls ${CURRENT_DIR}/out/plugin/config-generator)" ]; then
    formatlog "ERROR" "Missing config generator scripts; Please build the artifacts"
    exit 1
fi


# Navigate to App Dir
cd ${CURRENT_DIR}/out/plugin

jfrog rt upload \
    "artifacts/*" \
    "${ARTIFACTORY_PLUGIN_PATH}/" \
    --build-name="${BUILD_NAME}" \
    --build-number="${BUILD_NUMBER}" \
    --flat=false \
    --recursive \
    --exclusions="*DS_Store;*gitignore"


# Upload the dependencies to the specified location in Artifactory
jfrog rt upload \
    "config-generator/*" \
    "${ARTIFACTORY_PLUGIN_PATH}/" \
    --build-name="${BUILD_NAME}" \
    --build-number="${BUILD_NUMBER}" \
    --flat=false \
    --recursive \
    --exclusions="*DS_Store;*gitignore;channels"

# Upload the scripts to the specified location in Artifactory
jfrog rt upload \
    "runtime-scripts/*" \
    "${ARTIFACTORY_PLUGIN_PATH}/" \
    --build-name="${BUILD_NAME}" \
    --build-number="${BUILD_NUMBER}" \
    --flat=false \
    --recursive \
    --exclusions="*DS_Store;*gitignore;channels"

# Upload race-python-utils (used to generate configs)
jfrog rt upload \
    "race-python-utils/*" \
    "${ARTIFACTORY_PLUGIN_PATH}/" \
    --build-name="${BUILD_NAME}" \
    --build-number="${BUILD_NUMBER}" \
    --flat=false \
    --exclusions="*.eggs*;*.pytest.*;*reports*;*DS_Store;*gitignore;*__pycache__*;*.git*;*scripts*;*.blackrc.toml;*.coveragerc;*.pycodestylerc;*.pylintrc;*Makefile;*README.md;*setup.cfg;*setup.py;*race_python_utils/tests/*;*race_python_utils/ta1/tests/*"



if [[ "$BUILD_NUMBER" != "" ]]; then
    # Collect environment variables and add them to the build info:
    jfrog rt build-collect-env $BUILD_NAME $BUILD_NUMBER

    # Publish the build info to Artifactory:
    jfrog rt build-publish $BUILD_NAME $BUILD_NUMBER
fi
