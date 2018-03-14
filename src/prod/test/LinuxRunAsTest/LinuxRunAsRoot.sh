#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null

echo "LinuxRunAsRoot.sh is starting 'LinuxRunAsTest.exe root' on ${Fabric_NodeName}"
${ScriptPath}/LinuxRunAsTest.exe root