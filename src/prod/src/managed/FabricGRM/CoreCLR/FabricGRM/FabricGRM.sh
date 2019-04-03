#!/usr/bin/env bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


check_errs()
{
  # Function. Parameter 1 is the return code
  if [ "${1}" -ne "0" ]; then
    # make our script exit with the right error code.
    exit ${1}
  fi
}

DIR=`dirname $0`
echo 0x3f > /proc/self/coredump_filter

. /etc/os-release
linuxDistrib=$ID
if [ $linuxDistrib = "rhel" ]; then
  source scl_source enable rh-dotnet20
  exitCode=$?
  if [ $exitCode != 0 ]; then
    echo "Failed: source scl_source enable rh-dotnet20 : ExitCode: $exitCode"
    exit $exitCode
  fi
fi
dotnet $DIR/FabricGRM.dll $@

check_errs $?