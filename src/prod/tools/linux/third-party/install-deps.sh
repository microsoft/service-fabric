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