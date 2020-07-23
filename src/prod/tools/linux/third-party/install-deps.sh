#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


GetOs()
{
    local __resultvar=$1

    . /etc/os-release
    local linuxDistrib=$ID

    if [ $linuxDistrib = "ubuntu" ]; then
        eval $__resultvar="DEBIAN"
    elif [ $linuxDistrib = "rhel" ] || [ $linuxDistrib = "centos" ]; then
        eval $__resultvar="REDHAT"
    else
        eval $__resultvar="UNKNOWN"
    fi
}

GetOs LINUX_DISTRIBUTION
if [ $LINUX_DISTRIBUTION = "REDHAT" ]; then
    echo "ERROR: REDHAT not supported yet"
    exit 1
fi

# Check if GO is installed and version is greater than 1.10
# How to install go 1.10 --> https://github.com/golang/go/wiki/Ubuntu
command -v go >/dev/null 2>&1
if [ $? != 0 ]; then
    echo "ERROR: Please install golang 1.10"
    exit -1
fi
version=$(go version)
regex="([0-9]*\.[0-9]*)"
if [[ $version =~ $regex ]]; then
    version=${BASH_REMATCH[1]}
    nums=( ${version//./ } )
else
    echo "Unable to parse go version number."
    exit -1
fi
if !([ ${nums[0]} -ge "1" ] && [ ${nums[1]} -ge "10" ]); then
    echo "ERROR: Your GO version is too old. cri-o needs at least GO 1.10"
    exit -1
fi

deps=(
   # jemalloc
   autoconf

   # babeltrace
   libtool
   libglib2.0-0
   libglib2.0-dev
   bison
   flex
   libpopt-dev

   # llvm
   libncurses5-dev
   libncursesw5-dev
   swig
   libedit-dev

   #cri-o
   btrfs-tools
   git
   libassuan-dev
   libdevmapper-dev
   libglib2.0-dev
   libc6-dev
   libgpgme11-dev
   libgpg-error-dev
   libseccomp-dev
   libselinux1-dev
   pkg-config
   go-md2man
   runc
   libostree-dev # libostree-dev can be found in the flatpak(https://launchpad.net/~alexlarsson/+archive/ubuntu/flatpak) PPA

   #libcurl
   libcurl4-openssl-dev
)

for pkg in "${deps[@]}"
do
    echo "Installing dependency: $pkg"
    # NOTE: The dpkg -s command returns 0 if a package is installed or WAS installed in the past.
    # dpkg -W return code is also not well defined, and the status code is not consistent with 
    # whether it is installed or not.  For example, both dpkg -W autoconf and dpkg -W bash 
    # return 0, when autoconf is not installed, and bash IS installed.  So ultimately, we need 
    # to explicitly check the output for the message that the package IS installed.  
    # Note this returns 1 if the package IS installed.
    $(dpkg-query -W -f='${Status}' $pkg 2>/dev/null | grep -c "ok installed") &> /dev/null
    if [ $? -eq 1 ]; then
        echo "$pkg already installed on this machine. Skipping..."
        continue
    fi

    sudo apt-get install -y $pkg
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install $pkg."
        echo "Please check your system settings and run this script again."
        exit -1
    fi
done
