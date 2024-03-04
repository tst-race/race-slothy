# **Resilient Anonymous Communication for Everyone (RACE) Slothy (Artifact Manager) Guide**

## **Table of Contents**
  * [**Introduction**](#introduction)
    + [**Design Goals**](#design-goals)
    + [**Security Considerations**](#security-considerations)
  * [**Scope**](#scope)
    + [**Audience**](#audience)
    + [**Environment**](#environment)
    + [**License**](#license)
    + [**Additional Reading**](#additional-reading)
  * [**Implementation Overview**](#implementation-overview)
  * [**Implementation Organization**](#implementation-organization)
    + [artifact-manager-plugin](#artifact-manager-plugin)
    + [registry-client](#registry-client)
    + [shard-host-server](#shard-host-server)
    + [shard-sharding-server](#shard-sharding-server)
    
<br></br>

## **Introduction**
Slothy is an Artifact Manager Plugin (AMP) designed to use cryptographic sharding techniques to facilitate secure distribution of RACE artifacts (i.e. runtime code). The current shard-pushing logic does not function properly.

</br>

### **Design Goals**
Slothy has two primary goals: providing a distribution method for encrypted artifacts and using cryptographic sharding to enable the keys for decrypting the artifacts to be split into _n_ shards, wherein the decryptor must obtain _k_ of these _n_ shards to decrypt the artifacts.

### **Security Considerations**
This plugin is a research prototype and has not been the subject of an independent security audit or extensive external testing.


<br></br>

## **Scope**
This developer guide covers the  development model, building artifacts, running, and troubleshooting.  It is structured this way to first provide context, associate attributes/features with source files, then guide a developer through the build process.  

</br>

### **Audience**
Technical/developer audience.

### **Environment**
Works on x86 and arm64 Linux hosts, does not support Android clients.


### **License**
Licensed under the APACHE 2.0 license, see LICENSE file for more information.

### **Additional Reading**
* [RACE Quickstart Guide](https://github.com/tst-race/race-quickstart/blob/main/README.md)

* [What is RACE: The Longer Story](https://github.com/tst-race/race-docs/blob/main/what-is-race.md)

* [Developer Documentation](https://github.com/tst-race/race-docs/blob/main/RACE%20developer%20guide.md)

* [RIB Documentation](https://github.com/tst-race/race-in-the-box/tree/2.6.0/documentation)

<br></br>

## **Implementation Overview**
Slothy consists of an Artifact Manage Plugin (AMP) which runs as a plugin in every RACE node; it also has a _registry-client_ which is a special type of RACE client that automatically responds to requests for artifacts (which are automatically sent by AMPs on other nodes when they need to fetch artifacts). Outside the network of RACE nodes, Slothy has a set of shard-host-servers which function as simple storage mechanisms (analogous to cloud storage providers in the real world) and is where the actual encrypted artifacts and sharded keys are stored. Finally, there is a shard-sharding server which ingests artifacts and produces randomly-padded encrypted copies and sharded keys, pushes these to different shard-host-servers, and then informs the registry-client about the locations of these keys and artifacts.

<br></br>

## **Implementation Organization**
The main repository is split into four separate directories for these four separates components:

### artifact-manager-plugin
This directory contains the actual AMP code that is built and runs on each RACE node.

### registry-client
This directory contains code for building a special refinement of the RACE client that ingests information about shards from the shard-sharding-server and automatically distributes information about encrypted artifacts in response to queries from other RACE nodes. All these requests and responses occur over the RACE anonymous messaging network.

### shard-host-server
This is a simple HTTP server that allows encrypted artifacts and sharded keys to be pushed to it and then later fetched. Neither of these actions require authentication.

### shard-sharding-server
This server ingests artifacts and produced encrypted versions along with sharded copies of the encryption keys. It then pushes both of these to shard-host-servers and then informs the registry-client about the locations of these and their metadata.

<br></br>

## **How To Build and Run**
In __artifact-manager-plugin and __registry-client__ run `build_it_with_docker.sh` to produce a `kit` directory to be included in a RACE deployment with the following `deployment create` arguments:

```
--artifact-manager-kit=local=<path/to/artifact-manager-plugin/kit> \
--registry-app=local=<path/to/registry-client/kit>
```


In __shard-host-server__ run `docker build . -t ghcr.io/tst-race/race-slothy/shard-host-server:1.0.3`

In __shard-sharding-server__ run `docker build . -t ghcr.io/tst-race/race-slothy/shard-sharding-server:1.0.4`


</br>
