#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd $SCRIPT_DIR/../

export _NTTREE=$(pwd)
export LD_LIBRARY_PATH="$(pwd)/lib"
export LinuxModeEnabled=true
export MALLOC_CHECK_=2

DotnetTestsRoot=$SCRIPT_DIR/CoreCLRDlls

pushd $DotnetTestsRoot
EXITCODE=$?

if [ $EXITCODE -ne 0 ]
then
  echo "Unable to find DotnetTestsRoot directory."
  exit $EXITCODE
fi

# Since there are many non-test dlls in test folder, we look for deps.json file
# and replace extension with dll to get test file name.

# Ignore if no deps.json file is present.
shopt -s nullglob

EXITCODE=0

for depsFileName in *.deps.json; do
    testName=${depsFileName/.deps.json/.dll}
    echo "Running : dotnet vstest $testName"
    dotnet vstest $testName
    DOTNET_EXITCODE=$?
    if [ $DOTNET_EXITCODE -ne 0 ]
    then
      EXITCODE=$DOTNET_EXITCODE
    fi
done

popd
popd
exit $EXITCODE