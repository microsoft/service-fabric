
# How to install the packages and deploy local cluster on Linux

To deploy and run a cluster on your Linux development machine, install the runtime and common SDK generated as the build output. 

## Prerequisites

The following operating system versions are supported for development:

* Ubuntu 16.04 (`Xenial Xerus`)

Ensure you have run the build using the `-createinstaller` option to generate the debian files
```
./runbuild.sh -createinstaller
```

## Installation steps

### Update your APT sources
To install the SDK and the associated runtime package via the apt-get command-line tool, you must first update your Advanced Packaging Tool (APT) sources.

1. Open a terminal.

2. Add the `dotnet` repo to your sources list.

    ```bash
    sudo sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list'
    ```

3. Add the new Gnu Privacy Guard (GnuPG, or GPG) key to your APT keyring.

    ```bash
    sudo apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 417A0893
    ```

4. Add the official Docker GPG key to your APT keyring.

    ```bash
    sudo apt-get install curl
    sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
    ```

5. Set up the Docker repository.

    ```bash
    sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
    ```

6. Refresh your package lists based on the newly added repositories.

    ```bash
    sudo apt-get install apt-transport-https
    sudo apt-get update
    ```

### Install and set up the Service Fabric Debians for local cluster setup

After you have updated your sources, you can install the runtime and SDK, confirm the installation, and agree to the license agreement. Please cd to the location in the output directory where the debians are placed i.e. `out/build.prod/FabricDrop/deb/`. Install the runtime and sdk Debian packages named as `servicefabric_x.x.x.x.deb and servicefabric_sdkcommon_y.y.y.deb`, where `x.x.x.x` and `y.y.y` is the actual version numbers in the same of the file. Please replace the commands below with the actual files names in the folder 

```bash
sudo apt-get install -fy && sudo dpkg -i out/build.prod/FabricDrop/deb/servicefabric_6*.deb
sudo apt-get install -fy && sudo dpkg -i out/build.prod/FabricDrop/deb/servicefabric_sdkcommon_*.deb
```

>   The following commands automate accepting the license for Service Fabric packages:
>   ```bash
>   echo "servicefabric servicefabric/accepted-eula-ga select true" | sudo debconf-set-selections
>   echo "servicefabricsdkcommon servicefabricsdkcommon/accepted-eula-ga select true" | sudo debconf-set-selections
>   ```

## Set up a local cluster
  Once the installation completes, you should be able to start a local cluster.

  1. Run the cluster setup script.

      ```bash
      sudo /opt/microsoft/sdk/servicefabric/common/clustersetup/devclustersetup.sh
      ```

  2. Open a web browser and go to [Service Fabric Explorer](http://localhost:19080/Explorer). If the cluster has started, you should see the Service Fabric Explorer dashboard.

  At this point, you can deploy pre-built Service Fabric application packages or new ones based on guest containers or guest executables. Please check out more details at [How to run some sample projects](https://azure.microsoft.com/resources/samples/?sort=0&service=service-fabric).


>   If you have an SSD disk available, it is recommended to pass an SSD folder path using `--clusterdataroot` with devclustersetup.sh for superior performance.

## Set up the Service Fabric CLI

The [Service Fabric CLI](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cli) has commands for interacting with Service Fabric entities,
including clusters and applications.
Follow the instructions at [Service Fabric CLI](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cli) to install the CLI.
