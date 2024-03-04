#!/bin/bash
BASENAME="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
OUT_DIR=$BASENAME
ALL_ARTIFACTS_DIR=$OUT_DIR/artifacts
BUILD_DIR=$OUT_DIR/build
APP_DIR=$OUT_DIR/kit/

LINUX_X86_64_CLIENT_DIR=$APP_DIR/artifacts/linux-x86_64-client/registry

$BASENAME/clean_artifacts.sh


# mkdir -p $BUILD_DIR
# mkdir -p $OUT_DIR
# mkdir -p $APP_DIR

mkdir -p $LINUX_X86_64_CLIENT_DIR

cp $BASENAME/rib-resources/app-manifest.json $APP_DIR/
cp $BASENAME/rib-resources/install.sh $LINUX_X86_64_CLIENT_DIR

# cd $BUILD_DIR
cmake --preset=LINUX_x86_64 -Wno-dev -DCMAKE_INSTALL_PREFIX:PATH=$ALL_ARTIFACTS_DIR $BASENAME
cmake --build --preset=LINUX_x86_64 --target=install

mkdir -p $LINUX_X86_64_CLIENT_DIR/bin
cp $BASENAME/build/LINUX_x86_64/bin/raceregistry $LINUX_X86_64_CLIENT_DIR/bin/
# cp $ALL_ARTIFACTS_DIR/bin/raceregistry $LINUX_X86_64_CLIENT_DIR/bin/

# Generate some SSL certs for the registry
openssl req -new -x509 -keyout $LINUX_X86_64_CLIENT_DIR/bin/server.key -out $LINUX_X86_64_CLIENT_DIR/bin/server.pem -days 365 -nodes -subj "/C=CN/ST=./L=./O=./OU=../CN=."
