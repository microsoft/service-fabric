# Running a test using Cluster Deployer in a Docker image

## Prerequisites

You will need to setup Docker as instructed here for [mac](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-get-started-mac) or see below for the Linux instructions.

### Linux Setup

---
#### Note ####
You can't run the local cluster and the Docker daemon concurrently. If you followed the [instructions]  (install_packages_and_deploy_cluster.md) to setup a local cluster, then in order to use use this test you will need to stop the local cluster and start the Docker daemon.

```
sudo systemctl stop servicefabric
sudo systemctl start docker
```
---

1. Edit the Daemon configuration (normally `/etc/docker/daemon.json`)

```json
{
  "ipv6": true,
  "fixed-cidr-v6": "fd00::/64"
}
```
2. Expose a tcp listener for your Docker daemon. For Ubuntu edit `/etc/systemd/system/docker.service.d/docker.conf`

```
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -H fd:// -H tcp://0.0.0.0:2376
```

3. Reload Docker. On Ubuntu
```
sudo systemctl daemon-reload
sudo systemctl restart docker
```
## Running the test

Build Service Fabric
```
  $ ./runbuild.sh
```
Run the test
```
  $ cd out/build.prod/ClusterDeployerTest/
  $ ./runtest.sh
```
## Details

This test first builds a Docker image (service-cluster-run-ubuntu) locally, this image contains the necessary packages to run Service Fabric. The test then runs Docker, mounting the FabricDrop folder in your output directory. Then a [sample application](https://github.com/Azure-Samples/service-fabric-dotnet-core-getting-started) is downloaded and built inside the container. The application is installed, and if the http endpoint can be hit then the test passes.
