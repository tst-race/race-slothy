#!/bin/bash
BASENAME="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
BUILD_DIR=$BASENAME/build
PLUGIN_DIR=kit
CLIENT_PLUGIN_DIR=$PLUGIN_DIR/artifacts/linux-x86_64-client/PluginArtifactManagerSlothy/
SERVER_PLUGIN_DIR=$PLUGIN_DIR/artifacts/linux-x86_64-server/PluginArtifactManagerSlothy/

# Clean first
$BASENAME/clean_artifacts.sh

mkdir -p $BUILD_DIR
mkdir -p $CLIENT_PLUGIN_DIR
mkdir -p $SERVER_PLUGIN_DIR


BASE_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) >/dev/null 2>&1 && pwd)
. ${BASE_DIR}/helper_functions.sh

cmake --preset=LINUX_x86_64 -Wno-dev $BASENAME
cmake --build --preset=LINUX_x86_64 $CMAKE_ARGS --target=install

mkdir -p $CLIENT_PLUGIN_DIR/
mkdir -p $SERVER_PLUGIN_DIR/


cp $BUILD_DIR/LINUX_x86_64/src/libsss.so $CLIENT_PLUGIN_DIR
cp $BUILD_DIR/LINUX_x86_64/src/libslothy.so $CLIENT_PLUGIN_DIR
cp $BUILD_DIR/LINUX_x86_64/src/libslothy-amp.so $CLIENT_PLUGIN_DIR
cp -r $BASENAME/rib-resources/manifest.json $CLIENT_PLUGIN_DIR


cp $BUILD_DIR/LINUX_x86_64/src/libsss.so $SERVER_PLUGIN_DIR
cp $BUILD_DIR/LINUX_x86_64/src/libslothy.so $SERVER_PLUGIN_DIR
cp $BUILD_DIR/LINUX_x86_64/src/libslothy-amp.so $SERVER_PLUGIN_DIR
cp -r $BASENAME/rib-resources/manifest.json $SERVER_PLUGIN_DIR

cp -r $BASENAME/rib-resources/config-generator $PLUGIN_DIR
cp -r $BASENAME/rib-resources/runtime-scripts $PLUGIN_DIR
cp -r $BASENAME/rib-resources/race-python-utils $PLUGIN_DIR
