# shard-server

Server that hosts shards.

Development
```bash
docker run -it --rm -v $PWD:/root/ -p 4443:443 `docker build . -q`
```

Production
```bash
docker run -it --rm -p 4443:443 pldc.peratonlabs.com:5000/microcon/shard-host-server
```