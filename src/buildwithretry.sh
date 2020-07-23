#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Script to retry build.sh until it succeeds

COUNTER=0
EXITCODE=1
while [ $COUNTER -lt 15 ] && [ $EXITCODE -ne 0 ]; do
    $(dirname $0)/build.sh "$@"
    EXITCODE=$?
    let COUNTER=COUNTER+1
done
exit $ret_code