#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null

echo "LinuxRunAsAcct.sh is starting 'LinuxRunAsTest.exe acct' on ${Fabric_NodeName}"
${ScriptPath}/LinuxRunAsTest.exe acct