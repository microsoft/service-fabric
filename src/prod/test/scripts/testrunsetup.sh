#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# This script installs the packages, the deb, and runs ClusterDeployer.sh to set permissions.  It's executed once per group
set -x

# Rename this dir.  In a Linux build the dir is called test and "FabricUnitTests" is a symbolic link.  So we upload "test"
# then rename it here, as the test type jsons/definitions themselves expect a dir named FabricUnitTests
#mv -f test FabricUnitTests

# this script should run from ....BinCache/bins so everything should be relative to that
debdirectory="./FabricDrop/deb/"

echo "The current directory is $(pwd)"
workingdirectory="$(pwd)"

sudo sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list'
sudo apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates curl software-properties-common -fy

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

sudo apt-get update

echo "DEBUG, printing processes before uninstall..."
ps aux
ps aux | grep dpkg
ps aux | grep apt

sudo apt-get autoclean
sudo apt-get autoremove
sudo apt-get --force-yes remove servicefabric
uninstallReturnValue=$?
sudo apt-get -f install
sudo dpkg --configure -a

echo "DEBUG - the uninstall above returned ${uninstallReturnValue}"
if [ $? != 0 ]; then
    echo "DEBUG - the command above returned ${uninstallReturnValue} , failing"
    exit $uninstallReturnValue 
fi

echo "servicefabric servicefabric/accepted-all-eula select true" | debconf-set-selections


# find the name of the deb, since the version changes.
cd $debdirectory
pattern="servicefabric*.deb"
files=( $pattern )
thedeb=${files[0]}
echo "the name is $thedeb"

echo "DEBUG, printing processes again..."
ps aux
ps aux | grep dpkg
ps aux | grep apt

sudo DEBIAN_FRONTEND=noninteractive dpkg -i $thedeb <<< y &>  debinstalloutput.txt

echo "dumping contents"
more debinstalloutput.txt
echo "done dumping contents"

grep 'debinstalloutput.txt' -e 'subprocess new pre-installation script returned error exit status 128'

outputDoesNotContain128=$?
if [ 0 == $outputDoesNotContain128 ]; then
    echo "DEBUG - the command above returned an error, failing"
    exit 128
fi

sudo apt-get update
sudo apt-get install -f -y
if [ $? != 0 ]; then
    echo Error: Install failed
    exit 1
fi

#
#Install mono - in the final form this should probably be in machine setup, not run setup

sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
echo "deb http://download.mono-project.com/repo/ubuntu xenial main" | sudo tee /etc/apt/sources.list.d/mono-official.list
sudo apt-get update

sudo apt-get install mono-complete -y


#cd "$workingdirectory/../ClusterDeployer"
cd "$workingdirectory/ClusterDeployer"

# Run ClusterDeployer.sh to do some setup.  The -h omits the parts that call FabricDeployer.sh, and that start the FabricHost.
bash ClusterDeployer.sh -h
if [ $? != 0 ]; then
    error=$?
    echo "Error: ClusterDeployer.sh failed with $error"
    exit $error
fi

systemctl servicefabric disable

# install dotnet 2.  This should be in machine setup
sudo sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list'
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 417A0893
sudo apt-get update

sudo apt-get install dotnet-dev-1.0.4


sudo sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list'
sudo apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893
sudo apt-get update
sudo apt-get install dotnet-runtime-2.0.0 -y

sudo apt-get install dotnet-sdk-2.0.0 -y

mkdir /tmp/azcopyinstall
cd /tmp/azcopyinstall

wget -O azcopy.tar.gz https://aka.ms/downloadazcopyprlinux
tar -xf azcopy.tar.gz
sudo ./install.sh
#if [ $? != 0]; then
#    error=$?
#    echo "Error: installation of azcopy failed with $error"
#    exit $error
#fi

rm -f /mnt/mcroot/BinCache/bins/RunTests/Crashdumps/*core*
echo "The current directory is $(pwd)"


# Relink these so they have the same relationship as on devbox.
ln /mnt/mcroot/BinCache/bins/FabricUnitTests /mnt/mcroot/BinCache/bins/test -sf

echo "Success"



