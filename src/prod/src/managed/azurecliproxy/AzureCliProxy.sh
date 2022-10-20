#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

#
check_errs()
{
  # Function. Parameter 1 is the return code
  if [ "${1}" -ne "0" ]; then
    exit ${1}
  fi
}

DIR=`dirname $0`

$DIR/runtimes/dnx-coreclr-linux-x64.1.0.0-rc1-update1/bin/dnx $DIR/azurecliproxy.dll "$@"
check_errs $?