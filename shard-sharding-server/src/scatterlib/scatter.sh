#!/bin/bash

DIR="$( cd "$( dirname "$0" )" >/dev/null 2>&1 && pwd )"
REGISTRY_URL="pldc.peratonlabs.com:5000"

docker run -it --rm -v $DIR:/root/scatter -e LIBSSS=/usr/lib/x86_64-linux-gnu/libsss.so -e LIBSODIUM=/usr/lib/x86_64-linux-gnu/libsodium.so $REGISTRY_URL/microcon/scatter scatter.py $@
