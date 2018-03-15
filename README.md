# Service Fabric
[![Build Status](https://travis-ci.com/Microsoft/service-fabric.svg?branch=master)](https://travis-ci.com/Microsoft/service-fabric)

Service Fabric is a distributed application platform for packaging, deploying, and managing stateless and stateful distributed applications and containers at large scale. Service Fabric runs on Windows and Linux, on any cloud, any datacenter, across geographic regions, or on your laptop.

## Getting started with Service Fabric
To get started building applications for Service Fabric:

 - Set up your development environment for [Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started), [Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-Linux), or [Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac).
 - [See our documentation](https://docs.microsoft.com/azure/service-fabric/).
 - [Run some sample projects](https://azure.microsoft.com/resources/samples/?sort=0&service=service-fabric).

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


## How to engage, contribute and provide feedback 
During our transition to open development, we are primarily focused on tasks related to building, testing, and developing Service Fabric on GitHub. If you are interested in helping out with this effort, head over to the current set of issues to see what we currently need help with. We will happily work with you and take any contributions that help us move Service Fabric development to GitHub

In the meantime, contributions to other areas of Service Fabric are welcome on a best-effort basis. While the team continues to develop internally, we will integrate the changes into our internal development repo for testing and verification, and then push the merged changes back to GitHub when the change is released. The smaller and more targeted your PRs, the easier it will be for us to review and integrate them. 

For more information on how this process works and how to contribute, provide feedback, and log bugs, please see [Contributing.md](CONTRIBUTING.md).

## Setting up for build
### Get a linux machine
This is the linux version of Service Fabric.  You need a linux machine to build this project.  If you have a linux machine, great! You can get started below.  If not, you can get a [linux machine on Azure](https://azuremarketplace.microsoft.com/en-us/marketplace/apps/Canonical.UbuntuServer?tab=Overview).
### Installing docker
Our build environment depends on Docker.  In order to get started you will need to [install docker](https://docs.docker.com/engine/installation/).  

There are many ways to install docker, here is how to install on Ubuntu:
```
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce
```

## Optional: Enable executing docker without sudo
By default docker requires root privelages to run.  In order to run docker as a regular user (i.e, not root), you need to add the user to the `docker` user group:
```
sudo usermod -aG docker ${USER}
su - ${USER}
```

You do not need to do this, but note that if you skip this step, you must run all docker commands with sudo.
## Build Service Fabric
To start the build inside of a docker container you can clone the repository and run this command from the root directory
```
./runbuild.sh
```
This will do a full build of the project with the output being placed into the `out` directory.  For more options see `runbuild.sh -h`.

#### Optional: Build the container locally
If you would prefer to build the container locally, you can run the following script:
```
sudo ./tools/builddocker.sh
```

Currently, the build container is based off a base image that includes a few service fabric dependencies that have either not yet been open sourced, or must be included due to technical constraints (for example, some .NET files currently only build on windows, but are a dependency of the Linux build).

This will pull all of the required packages, add service fabric internal dependencies, and apply patches.


## Documentation 
Service Fabric conceptual and reference documentation is available at [docs.microsoft.com/azure/service-fabric](https://docs.microsoft.com/azure/service-fabric/). Documentation is also open to your contribution on GitHub at [github.com/Microsoft/azure-docs](https://github.com/Microsoft/azure-docs).
## Samples 
For Service Fabric sample code, check out the [Azure Code Sample gallery](https://azure.microsoft.com/resources/samples/?service=service-fabric) or go straight to [Azure-Samples on GitHub](https://github.com/Azure-Samples?q=service-fabric).
## License 
All Service Fabric open source projects are licensed under the [MIT License](LICENSE.txt).
## Code of Conduct 
All Service Fabric open source projects adopt the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
