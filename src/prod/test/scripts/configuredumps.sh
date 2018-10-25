#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

ulimit -c unlimited
ulimit -a

# This dir might already exist which is ok
mkdir "$(pwd)/RunTests/CrashDumps"
chmod 777 "$(pwd)/RunTests/CrashDumps"

# Delete dumps so they don't mix from previous runs in the same group
rm -f $(pwd)/RunTests/CrashDumps/*

echo "$(pwd)/RunTests/CrashDumps/core.%e.%p.%h.%t" > /proc/sys/kernel/core_pattern
echo "Printing contents of /proc/sys/kernel/core_pattern:"
more /proc/sys/kernel/core_pattern
echo "pwd is $(pwd)"


