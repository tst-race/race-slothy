# TA 3.1 Shard Registry

Provides a peristent storage for JSON formatted metadata used for locating shards and encrypted blobs of artifacts. Receives metadata from the shard sharding server via HTTPS and stores it in a small SQLite database. This data can then be retrieved via HTTPS or via the RACE network.

## Building
Project can be built using the included shell script `build_it_with_docker.sh`, which will handle loading a docker enviroment to compile, as well as generate self-signed HTTPS certificates for the web server. 

## Usage
Running the generated binary should be straightforward. During startup, it will handle creation of the database if none exists and set up a RACE client interface using typical RACE configuration files. In the event that no RACE connection is desired, the `norace` option can be passed on startup to skip this step. 

HTTPS interaction is a simple REST API. The process for adding new entries is a PUT request with the appropriate JSON in the body (handled by the shard sharding server) and requestion entries is via a GET request (handled by slothy). 

## Example
```
raceregistry
```

Skipping RACE
```
raceregistry norace
```
