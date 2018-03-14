#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


#This script is responsible for the cleanup of the whole cgroup hierarchy created by fabric
#Paths that we will remove is /cpu/fabric and /memory/fabric relative to cgroup mount path

memoryMount=($(findmnt -t cgroup | grep memory))
if [ ! -z $memoryMount ]; then
    fabricMemoryPath=${memoryMount[0]}/fabric
fi
cpuMount=($(findmnt -t cgroup | grep cpu | grep -v cpuset))
if [ ! -z $cpuMount ]; then
    fabricCpuPath=${cpuMount[0]}/fabric
fi

#we need all the child subdirectories removed before we remove the parent
function removeCgroup {
    if [ -d $1 ]; then
        for cgroupPath in $1/*; do
            removeCgroup $cgroupPath
        done
        rmdir $1
    fi
}

if [ ! -z $fabricMemoryPath ]; then
    echo "Removing cgroup" $fabricMemoryPath
    removeCgroup $fabricMemoryPath
fi
if [ ! -z $fabricCpuPath ]; then
    echo "Removing cgroup" $fabricCpuPath
    removeCgroup $fabricCpuPath
fi