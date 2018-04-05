# Service Fabric

[Service Fabric is a distributed systems platform](https://github.com/GitTorre/service-fabric/tree/master/docs/architecture/README.md) for packaging, deploying, and managing stateless and stateful distributed applications and containers at large scale. Service Fabric runs on Windows and Linux, on any cloud, any datacenter, across geographic regions, or on your laptop.

## Getting started with Service Fabric
To get started with building applications for Service Fabric:

 - Set up your development environment for [Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started), [Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-Linux), or [Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac).
 - [See our documentation](https://docs.microsoft.com/azure/service-fabric/).
 - [Run some sample projects](https://azure.microsoft.com/resources/samples/?sort=0&service=service-fabric).
 - [Watch this.](https://www.youtube.com/watch?v=v9WHgjSqwWg) 
 
## Project timeline and development
Service Fabric is currently undergoing a big transition to open development. Our main goal right now is to move the entire build, test, and development process to GitHub. For now the Service Fabric team will continue regular feature development internally while we work on transitioning everything to GitHub.  

We'll be providing frequent updates here and on our [team blog](https://blogs.msdn.microsoft.com/azureservicefabric/) as we work to get situated in our new home.  

### Quick look at our current status
 - [x] Service Fabric build tools for Linux
 - [x] Basic tests for Linux builds available
 - [x] Container image with build tools available to run builds

### Currently in progress
 - [ ] Build tools for Windows
 - [ ] Improve dependency consumption process
 - [ ] Automated CI environment
 - [ ] Migrate complete test infrastructure 


## Build Requirements
The requirements below are based off running clean builds using ninja, with the command

```sh
runbuild.sh –c –n
```

The builds were run on [Azure Linux VMs](https://docs.microsoft.com/en-us/azure/virtual-machines/linux/sizes-general) with added disk capacity. If you want to to build on an Azure machine you need to add approximately 70GB for the source+build outputs. 

These times should be taken as estimates of how long a build will take.

|Machine SKU|Cores|Memory|Build Time|
|-----------|-----|-----------|----------|
|Standard_D8s_v3|8|32GB|~4 hours|
|Standard_D16s_v3|16|64GB|~2 hours|
|Standard_D32s_v3|32|128GB|~1 hour|

On a smaller VM (Standard_D4s_V3 / 4 cores / 16GB) the build may fail. You may be able to build on a machine with less RAM if you limit the parallelism using the `-j` switch.

The build also requires approximately 70GB of disk space.

## Setting up for build
### Get a Linux machine
This is the Linux version of Service Fabric. You need a Linux machine to build this project.  If you already have a Linux machine, great! You can get started below.  If not, you can get a [Linux machine on Azure](https://azuremarketplace.microsoft.com/en-us/marketplace/apps/Canonical.UbuntuServer?tab=Overview).

### Installing docker
Our build environment depends on Docker. In order to get started you will need to [install docker](https://docs.docker.com/engine/installation/).  

There are many ways to install docker. Here is how to install on Ubuntu:

```sh
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce
```

## Optional: Enable executing docker without sudo
By default docker requires root privelages to run. In order to run docker as a regular user (i.e, not root), you need to add the user to the `docker` user group:

```sh
sudo usermod -aG docker ${USER}
su - ${USER}
```

You do not need to do this, but note that if you skip this step, you must run all docker commands with sudo.

## Build Service Fabric
To start the build inside of a docker container you can clone the repository and run this command from the root directory:

```sh
./runbuild.sh
```

This will do a full build of the project with the output being placed into the `out` directory.  For more options see `runbuild.sh -h`.

Additionally in order to build and create the installer packages you can pass in the `-createinstaller` option to the script:

```sh
./runbuild.sh -createinstaller
```

#### Optional: Build the container locally
If you would prefer to build the container locally, you can run the following script:

```sh
sudo ./tools/builddocker.sh
```

Currently, the build container is based off a base image that includes a few Service Fabric dependencies that have either not yet been open sourced, or must be included due to technical constraints (for example, some .NET files currently only build on Windows, but are required for a Linux build).

This will pull all of the required packages, add Service Fabric internal dependencies, and apply patches.

#### Troubleshooting: Internet connectivity when installing local docker containers behind a firewall
A common issue with building a docker container behind a firewall is when the firewall blocks the default DNS used by docker.  This will manifest as packages failing to download during the `docker build` step (such as in the `builddocker.sh` script above).  

To fix this, you need to tell Docker to use an alternative DNS server.  As a root user, create or edit the Docker daemon's config file at `/etc/docker/daemon.json` so that it has an entry that looks like this:

```json
{ 
    "dns": ["<my DNS server IP here>", "<my DNS secondary server IP here>"] 
}
```

Take note to replace the above command with your actual local DNS server, and restart docker:

```sh
service docker restart
```
## Testing a local cluster  
For more details please refer to [Testing using ClusterDeployer](docs/cluster_deployer_test.md).

## Running a local cluster  
For more details please refer [Deploying local cluster from build](docs/install_packages_and_deploy_cluster.md)

## Documentation 
Service Fabric conceptual and reference documentation is available at [docs.microsoft.com/azure/service-fabric](https://docs.microsoft.com/azure/service-fabric/). Documentation is also open to your contribution on GitHub at [github.com/Microsoft/azure-docs](https://github.com/Microsoft/azure-docs).
## Samples 
For Service Fabric sample code, check out the [Azure Code Sample gallery](https://azure.microsoft.com/resources/samples/?service=service-fabric) or go straight to [Azure-Samples on GitHub](https://github.com/Azure-Samples?q=service-fabric).
## Service Fabric - Behind the Curtain  
<a href="https://www.youtube.com/playlist?list=PLlrxD0HtieHh73JryJJ-GWcUtrqpcg2Pb&disable_polymer=true"><strong>Take a virtual tour with us</strong></a> and meet some of the folks who design and implement service fabric. This playlist will grow over time with content describing the inner workings of the product.  
## License 
All Service Fabric open source projects are licensed under the [MIT License](LICENSE).
## Code of Conduct 
All Service Fabric open source projects adopt the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
