#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# This script uninstalls the deb.  It's executed once per group
set -x

echo "The current directory is $(pwd)"

ulimit -a
ulimit -c 

# Debug tracing
ls -la /mnt/mcroot/BinCache/bins/RunTests/CrashDumps/*
find . -name core*
find . -name *.trace

sudo dpkg -r servicefabric
if [ $? != 0 ]; then
    error=$?
    echo Error: dpkg -r servicefabric failed with $error
    exit $error
fi

echo "Success"


