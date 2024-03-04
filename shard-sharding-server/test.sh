#!/bin/bash

ERROR_CODE=0

# SNODES = shard-host ip/hostnames
TOTAL_SNODES=12
for i in $(seq $TOTAL_SNODES); do
    SNODES+=(shard-sharding-server-shard-host-$i)
done
# echo "${SNODES[@]}"

# RNODES = race client ip/hostnames
TOTAL_RNODES=4
for i in $(seq $TOTAL_RNODES); do
    RNODES+=(shard-sharding-server-race-client-$i)
done
# RNODES is a list
# echo "${RNODES[@]}"

# dependencies that need to be sharded and stored on SNODES
# so that RNODES can pull and reconstruct them
DEPENDENCIES=(
  zeros_10kB
)

# Set BASEURL if not already set (will be already set in CI)
if ! [ -n "$BASEURL" ]; then
  BASEURL=https://shard-sharding:8000
fi

# Set MOUNT_BASE if not already set (will be already set in CI)
DIR="$( cd "$( dirname "$0" )" >/dev/null 2>&1 && pwd )"
if ! [ -n "$MOUNT_BASE" ]; then
  MOUNT_BASE=$DIR
fi

echo "DIR=${DIR}"
echo "MOUNT_BASE=${MOUNT_BASE}"
echo "BASEURL=${BASEURL}"
# DOCKER_NET=`date +%s | base64 | head -c 15 ; echo`

# refresh docker-compose setup
docker-compose down
docker-compose pull
docker network create racenet || true
# if changing number of shard hosts or clients, you must also update
# the json in shard-sharding-server via json, curl, or web UI.
# don't need to scale race-client because we are creating new containers to test
docker-compose up -d --scale shard-host=${#SNODES[@]} --scale race-client=1 # ${#RNODES[@]}

# loop while $TOKEN is not set. It will loop until shard-sharding-server has started serving on localhost:8000
unset TOKEN
while ! [ -n "$TOKEN" ]; do
  docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
  curl --insecure -c /mounted_folder/cookie.txt $BASEURL || echo 'waiting for shard-sharding-server to start on localhost:8000' && sleep 1
  TOKEN=`grep csrftoken cookie.txt | awk '{print $7}'`
  echo "TOKEN=$TOKEN"
done <<<$(find . -type f)

echo "Token found. TOKEN=$TOKEN"

# create test_data/snodes.json, which tells 
# shard-sharding-server where it can push the shards
python3 test_data/json_generator_snodes.py --outfile $DIR/test_data/snodes.json ${SNODES[@]::${#SNODES[@]}}
# POST test_data/snodes.json to update shard-sharding-server's config
URL=$BASEURL/config/server_nodes/
docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
curl -X POST --insecure \
     -S --cookie /mounted_folder/cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F """json=`cat $DIR/test_data/snodes.json`""" \
     $URL

# create test_data/rnodes.json, which tells 
# shard-sharding-server where it can push the shards
python3 test_data/json_generator_rnodes.py --outfile $DIR/test_data/rnodes.json ${RNODES[@]::${#RNODES[@]}}
# POST test_data/rnodes.json to update shard-sharding-server's config

URL=$BASEURL/config/race_nodes/
docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
curl -X POST --insecure \
     -S --cookie /mounted_folder/cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F "json=`cat $DIR/test_data/rnodes.json`" \
     $URL

# set dependencies json to empty list ("[]")
URL=$BASEURL/config/dependencies/
docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
curl -X POST --insecure \
     -S --cookie /mounted_folder/cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     -F "json=[]" \
     $URL

# This block of code iterates through each dependency and uploads them
for DEPENDENCY in "${DEPENDENCIES[@]}" ; do
  URL=$BASEURL/upload/
  echo "Uploading dependency ${DEPENDENCY}"
  docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
  curl -X POST --insecure \
       -S --cookie /mounted_folder/cookie.txt \
       -H "Referer: $URL" \
       -H "X-CSRFToken: $TOKEN" \
       -F "uuid=`uuidgen || cat /proc/sys/kernel/random/uuid`" \
       -F "deptype=${DEPENDENCY}" \
       -F "stability=1" \
       -F "speed=1" \
       -F "covertness=1" \
       -F "cost=1" \
       -F "filename=@/mounted_folder/test_data/${DEPENDENCY}" \
       $URL
done

# # trigger the solve, shard creation, and shard pushing
# URL=$BASEURL/deploy/
# curl -X GET --insecure \
#      -S --cookie cookie.txt \
#      -H "Referer: $URL" \
#      -H "X-CSRFToken: $TOKEN" \
#      $URL

# trigger the solve, shard creation, and shard pushing
# URL=$BASEURL/deploy/
# URL=$BASEURL/deploy/simple
URL=$BASEURL/randomize/
docker run -i --rm -v $MOUNT_BASE:/mounted_folder --net racenet curlimages/curl \
curl -X GET --insecure \
     -S --cookie /mounted_folder/cookie.txt \
     -H "Referer: $URL" \
     -H "X-CSRFToken: $TOKEN" \
     $URL

# test running a client
# first N should succeed (N=len(RNODES))
mkdir -p deleteme
for RNODE in "${RNODES[@]}" ; do
  # old command for aes_b64
  # (docker run --rm -v $MOUNT_BASE/test_data/test_binary:/root/bin/test_binary --net racenet --platform linux/x86_64 python /bin/bash -c "rm /usr/lib/x86_64-linux-gnu/${DEPENDENCIES[0]} && /root/bin/test_binary" 2>&1 | tee deleteme/$RNODE.txt || (set -e; false; break)) &
  # new command for slothy-http-download
  (docker run --rm \
    -v $MOUNT_BASE/test_data/:/root/bin/ \
    --net racenet \
    --platform linux/x86_64 \
    ghcr.io/tst-race/race-core/race-sdk:2.6.0 \
    /bin/bash -c \
    "LD_LIBRARY_PATH=/root/bin/lib:/usr/local/lib /root/bin/test_binary https://shard-registry ${DEPENDENCIES[0]} /root/bin/${DEPENDENCIES[0]}-downloaded && diff /root/bin/${DEPENDENCIES[0]} /root/bin/${DEPENDENCIES[0]}-downloaded && echo 'Downloaded file matches exactly!'" 2>&1 | tee deleteme/$RNODE.txt || (set -e; false; break)) &
done

for job in `jobs -p`
do
    wait $job
done
echo -e "\n************************ RESULTS ************************"
export NUM_SUCCEEDED=`grep -r "Downloaded file matches exactly!" deleteme/ | wc -l`
echo -e "Succeeded: \t\t\t\t${NUM_SUCCEEDED}/${#RNODES[@]}"
echo -e "Failed by files being different: \t`grep -r "Binary files /root/bin/${DEPENDENCIES[0]} and /root/bin/${DEPENDENCIES[0]}-downloaded differ" deleteme/ | wc -l`/${#RNODES[@]}"
echo -e "Failed by not getting enough shards: \t`grep -r "rs_unshard_fd: validate_shards only validated" deleteme/ | wc -l`/${#RNODES[@]}"
echo -e "Failed by not getting encrypted .so: \t`grep -r "load_diff: download_so" deleteme/ | grep "failed" | wc -l`/${#RNODES[@]}"
echo -e "Failed to get info from registry: \t`grep -r "failed to download dependency info from registry" deleteme/ | wc -l`/${#RNODES[@]}"
echo -e "Registry provided empty dictionary: \t`grep -r "error unpacking JSON provided by registry" deleteme/ | wc -l`/${#RNODES[@]}"
echo -e "Failed with Segmentation fault: \t`grep -r "Segmentation" deleteme/ | wc -l`/${#RNODES[@]}"
echo -e "*********************************************************\n"
rm -rf deleteme/outputfile*.txt
rm -rf deleteme/

if [ $NUM_SUCCEEDED != ${#RNODES[@]} ]; then
    echo "Not all RNODES succeded!"
    ERROR_CODE=$((ERROR_CODE+1))
fi

echo -e "\n************* One more test should fail *************"
# any additional attempts should fail.
# old command for aes_b64
# docker run --rm -v $MOUNT_BASE/test_data/test_binary:/root/bin/test_binary --net racenet --platform linux/x86_64 python /bin/bash -c "rm /usr/lib/x86_64-linux-gnu/${DEPENDENCIES[0]} && /root/bin/test_binary" && \
# "fail! the last run should not succeed" || set -e
# new command for slothy-http-download
docker run --rm \
    -v $MOUNT_BASE/test_data/test_binary:/root/bin/test_binary \
    -v $MOUNT_BASE/test_data/lib/:/root/lib/ \
    --net racenet \
    --platform linux/x86_64 \
    ghcr.io/tst-race/race-core/race-sdk:2.6.0 \
    /bin/bash -c \
    "LD_LIBRARY_PATH=/root/lib:/usr/local/lib /root/bin/test_binary https://shard-registry ${DEPENDENCIES[0]} /root/bin/${DEPENDENCIES[0]}-downloaded && diff /root/bin/${DEPENDENCIES[0]} /root/bin/${DEPENDENCIES[0]}-downloaded" && \
"fail! the last run should not succeed" && ERROR_CODE=$((ERROR_CODE+2))
echo -e "*****************************************************\n"

# cleanup
rm cookie.txt test_data/rnodes.json test_data/snodes.json
docker-compose down

exit ${ERROR_CODE}
