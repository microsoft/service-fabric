#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Illegal number of arguments passed in to startservicefabricupdater script"
    exit 1
fi

PathToTargetInfoFile=$1

echo updating servicefabricupdater.conf file with arguments
echo PathToTargetInfoFile=${PathToTargetInfoFile} > /etc/servicefabric/servicefabricupdater.config
if [ $? != 0 ]; then
    echo Error: Failed to write to servicefabricupdater.config
    exit 1
fi

echo Enable the service fabric updater service before starting
systemctl enable servicefabricupdater
if [ $? != 0 ]; then
    echo Error: Failed to enable servicefabricupdater service
    exit 1
fi

echo Starting the service fabric updater service to run through the upgrade state machine
systemctl start servicefabricupdater
if [ $? != 0 ]; then
    echo Error: Failed to start servicefabricupdater service
    exit 1
fi

echo Blocking the script since the upgrade script will kill fabric host which will kill this script 
sleep infinity