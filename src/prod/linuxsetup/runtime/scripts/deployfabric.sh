#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

FabricDataRoot=""
FabricBinRoot=/opt/microsoft/servicefabric/bin
ClusterManifest=""
InfraManifest=""

for i in "$@"
do
case $i in
    -fdr=*|--fabricdataroot=*)
    FabricDataRoot="${i#*=}"
    ;;
    -cm=*|--clustermanifest=*)
    ClusterManifest="${i#*=}"
    ;;
    -im=*|--inframanifest=*)
    InfraManifest="${i#*=}"
    ;;    
    *)
    echo Error: Unknown option passed in
    exit 1
	;;
esac
done

# Remove trailing slash
FabricDataRoot=$(echo $FabricDataRoot | sed 's:/*$::')


echo FabricDataRoot = ${FabricDataRoot}
echo FabricBinRoot = ${FabricBinRoot}
echo ClusterManifest = ${ClusterManifest}
echo InfraManifest = ${InfraManifest}

if [ "" = "$FabricDataRoot" ]; then
    echo FabricDataRoot is invalid. Please provide a valid path
    exit 1
fi

if [ "" = "$ClusterManifest" ]; then
    echo ClusterManifest path is invalid. Please provide a valid path
    exit 1
fi

FabricCodePath=${FabricBinRoot}/Fabric/Fabric.Code
FabricLogRoot=${FabricDataRoot}/log
echo "Setting environment for FabricDeployer to run"
if [ ! -d /etc/servicefabric ]; then
    mkdir /etc/servicefabric
fi
echo -n ${FabricDataRoot} > /etc/servicefabric/FabricDataRoot
echo -n ${FabricDataRoot}/log > /etc/servicefabric/FabricLogRoot
echo -n ${FabricBinRoot} > /etc/servicefabric/FabricBinRoot
echo -n ${FabricCodePath} > /etc/servicefabric/FabricCodePath

export LD_LIBRARY_PATH=${FabricCodePath}

mkdir -p ${FabricLogRoot}/Traces

echo Running FabricDeployer on ${ClusterManifest}
if [ "" = "$InfraManifest" ]; then
    ${FabricCodePath}/FabricDeployer.sh /operation:Create /fabricBinRoot:${FabricBinRoot} /fabricDataRoot:${FabricDataRoot} /cm:${ClusterManifest}
else
    ${FabricCodePath}/FabricDeployer.sh /operation:Create /fabricBinRoot:${FabricBinRoot} /fabricDataRoot:${FabricDataRoot} /cm:${ClusterManifest} /im:${InfraManifest}
fi

if [ $? != 0 ]; then
    echo Error: FabricDeployer.exe failed
    exit 1
fi

echo Setting sfuser as owner for FabricDataRoot
chown -R sfuser:sfuser ${FabricDataRoot} || exit 1
echo Setting sfuser as owner of FabricBinRoot
chown -R sfuser:sfuser ${FabricBinRoot} || exit 1

echo Enable service fabric service
systemctl enable servicefabric
if [ $? != 0 ]; then
    echo Error: Failed to enable servicefabric daemon
    exit 1
fi