#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

echo Stopping service
systemctl stop servicefabric
if [ $? != 0 ]; then
    echo Error: Stopping service failed. Check output for details
    exit 1
fi

echo Fabric service stopped

echo Disable service fabric service
systemctl disable servicefabric
if [ $? != 0 ]; then
    echo Error: Disable of servicefabric service failed. Check output for details
    exit 1
fi

SFCTL_TEMP_DIR=/home/${SUDO_USER}/.sfctl
if [ -d $SFCTL_TEMP_DIR ]; then
    rm -r /home/${SUDO_USER}/.sfctl
fi
