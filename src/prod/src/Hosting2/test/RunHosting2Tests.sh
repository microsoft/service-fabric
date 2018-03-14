#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


ScriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
export TestRootDir=${ScriptDir}
pushd $ScriptDir
pushd ../
export _NTTREE=$(pwd)
pushd $ScriptDir
${ScriptDir}/Hosting2.Test.exe "$@"
popd