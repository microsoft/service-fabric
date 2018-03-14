#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


./ClusterData/Data/N0010/Fabric/DCA.Code/lttng_handler.sh -c

for file in ./ClusterData/Data/log/Traces/*.dtr; do
    name=${file##*/}
    mv "$file" "./ClusterData/Data/log/work/WFab/0/0/Log/${name}"
done