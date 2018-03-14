#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

SkipDeleteRoot=false
FabricDataRoot=/home/sfuser/sfdevcluster/data
FabricBinRoot=/opt/microsoft/servicefabric/bin
ClusterManifestPath=/opt/microsoft/sdk/servicefabric/common/clustersetup/nonsecure/servicefabric-scalemin.xml

for i in "$@"
do
case $i in
    -cdr=*|--clusterdataroot=*)
    FabricDataRoot="${i#*=}"
    ;;
    -sd|--skipdeletedata)
    SkipDeleteRoot=true
    ;;
    *)
    echo Error: Unknown option passed in
    exit 1
    ;;
esac
done

if [ "false" = "$SkipDeleteRoot" ]; then
    echo Deleting all old content in FabricDataRoot
    rm -rf ${FabricDataRoot}/*
fi

echo Deploying service fabric scale min cluster
${FabricBinRoot}/Fabric/Fabric.Code/deployfabric.sh -fdr=${FabricDataRoot} -cm=${ClusterManifestPath}
if [ $? != 0 ]; then
    echo Error: Deploying cluster manifest failed. Check output for details
    exit 1
fi

echo Starting service
systemctl start servicefabric
if [ $? != 0 ]; then
    echo Error: Starting service failed. Check output for details
    exit 1
fi

echo Fabric service started. Cluster should come up in a few minutes

# TODO: Wait until cluster comes up
# test connection using azure CLI