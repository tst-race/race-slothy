# Slothy

RACE TA3.1 MICROCON LRDSI - Dependency Shard Collector

# Build

### Linux

```
./prepare_pythong_utils.sh
./build_it_with_docker.sh
```


Output files are 
```
libsss.so: Core sharding library, used in the shard-sharding server
libslothy.so: Slothy generic interfaces into the sharding library
libslothy-race.so: Interface to collect an artifact as a generic RACE client
libslothy-http.so: Interface to collect artifact with no RACE dependance at all
libslothy-amp.so: Interface to load artifacts as a RACE Artifact Management Plugin
```

## Test
Testing the core library is easiest via either the libslothy-race.so or libslothy-http.so interfaces. Demo binaries provide a simple interface to test acquiring an artifact by name from an active network consisiting of a registry, a shard-sharding server, and some number of shard-hosts.

### Usage
Using one of the downloader binaries
```
./slothy-race-download <registry address> <file dependency/artifact name> <location to save>
```

### Creating new interfaces
By extending the `Slothy` class, new interfaces can be created. By overriding the `get_artifact_registry_info()` function, alternative methods for retrieving artifact metadata from the registry can be implemented. In this repo, three such interfaces are provided for demonstration purposes. An HTTP direct interface, a standalone RACE interface, and an interface implemented as a RACE artifact manager plugin (AMP). These are provided as shared objects, and can be freely loaded by any application to provide access to files shared via the sharding distribution channel
