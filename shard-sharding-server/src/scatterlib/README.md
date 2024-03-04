# scatter

RACE TA3.1 MICROCON shard distributer

## System Dependencies

* Ensure that you've built the Slothy shared objects
* Install libsodium (apt-get install libsodium libsodium-dev)


## Python API

The Python wrapper API to SSS can be installed via:

```bash
python setup.py install
```

## CLI

Set the path to libsodium and libsss shared objects, e.g.

```bash
export LIBSODIUM=/usr/lib/x86_64-linux-gnu/libsodium.so.18
export LIBSSS=/home/branden/projects/vencorelabs/race/slothy/src/libsss.so
```

```bash
./scatter.py --help
```

### Shard

```bash
scatter.py shard sss/helloworld.txt
```

Specify custom shard naming prefix and output for encrypted file:

```bash
./scatter.py shard -o helloworldenc.dat -s chunk sss/helloworld.txt
```

```bash
./scatter.sh shard /root/slothy/so/libb64.so.0d --shardprefix /root/slothy/so/libb64.so.0d.json. --output /root/slothy/so/libb64.so.0d.enc
./scatter.sh shard /root/slothy/so/libcrypto.so.1.1 --shardprefix /root/slothy/so/libcrypto.so.1.1.json. --output /root/slothy/so/libcrypto.so.1.1.enc
```

#### move and shard two-face
```bash
cp ../slothy/libs/armeabi-v7a/two-face ../slothy/so/armeabi-v7a/two-face.so && ./scatter.sh shard /root/slothy/so/armeabi-v7a/two-face.so --shardprefix /root/slothy/so/armeabi-v7a/two-face.so.json. --output /root/slothy/so/armeabi-v7a/two-face.so.enc
```

### optim.py

#### Testing
```bash
docker run -it --rm -v $PWD/../slothy/so/libb64.so.0d:/tmp/libb64.so.0d -v $PWD/../slothy/so/libcrypto.so.1.1:/tmp/libcrypto.so.1.1 -v $PWD/data:/root/scatter/data -v $PWD/optim.py:/root/scatter/optim.py -v $PWD/../microcontainers/shard-sharding-server/scatter_server:/root/scatter_server -p 8000:8000 pldc.peratonlabs.com:5000/microcon/shard-sharding-server bash
```


then inside the container:

```bash
cd /root/scatter
python optim.py --simple data/regs_racenet.json data/deps_testing.json data/rnodes_racenet.json data/snodes_racenet.json
```

### scatter.py

```bash
docker run -it --rm -v $PWD/../slothy/so/:/tmp/so  pldc.peratonlabs.com:5000/microcon/shard-sharding-server bash
docker run -it --rm -v $PWD/../slothy/so/x86_64/libb64.so:/tmp/so -v $PWD/../slothy/libs/linux-x86_64/lib/libsss.so:/usr/lib/x86_64-linux-gnu/libsss.so pldc.peratonlabs.com:5000/microcon/shard-sharding-server bash

export FILE=/tmp/so/x86_64/libb64.so
export FILE=/tmp/so
python3 scatter.py encrypt_and_shard $FILE --shardprefix $FILE.json. --encrypted $FILE.enc --json $FILE.json
python3 scatter.py reconstruct_and_decrypt $FILE.json. $FILE.enc --json $FILE.json --decrypted $FILE.decrypted
diff $FILE $FILE.decrypted
```


Encrypt/Decrypt
```bash
python3 scatter.py encrypt sss/libsss.so --json sss/libsss.so.json --encrypted sss/libsss.so.enc
python3 scatter.py decrypt sss/libsss.so.enc --json sss/libsss.so.json --decrypted sss/libsss.so.decrypted
```

Shard/Reconstruct
```bash
python3 scatter.py shard sss/libsss.so --shardprefix sss/libsss.so.
python3 scatter.py reconstruct  sss/libsss.so. --reconstructed sss/libsss.so.reconstructed
```
Encrypt->Shard/Reconstruct->Decrypt
```bash
python3 scatter.py encrypt_and_shard sss/libsss.so --shardprefix sss/libsss.so.json. --encrypted sss/libsss.so.enc --json sss/libsss.so.json
python3 scatter.py reconstruct_and_decrypt sss/libsss.so.json. sss/libsss.so.enc --json sss/libsss.so.json --decrypted sss/libsss.so.decrypted
```
