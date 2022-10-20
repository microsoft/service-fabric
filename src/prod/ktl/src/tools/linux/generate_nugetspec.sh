#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


src=$1
BuildType=$2
dst=$3

version=$(grep -oP 'value="\K[^"]*' ${src})
echo Nuget Version: ${version}
echo Nuget Build Type: ${BuildType}
if [ ${BuildType} = "Debug" ]; then
    sed -i -e "s/REPLACE_BUILD_TYPE/debug/g" ${dst}
else
    sed -i -e "s/REPLACE_BUILD_TYPE/retail/g" ${dst}
fi
sed -i -e "s/REPLACE_VERSION/${version}/g" ${dst}