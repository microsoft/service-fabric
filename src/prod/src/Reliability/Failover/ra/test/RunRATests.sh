#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


ScriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
pushd $ScriptDir
${ScriptDir}/RA.Test.exe "$@"
popd