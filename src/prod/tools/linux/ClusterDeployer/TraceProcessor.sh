#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


lttng stop
lttng destroy
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="${SCRIPT_DIR}"
TRACE_DIR="$(ls -t -d ${SCRIPT_DIR}/ClusterData/Data/log/lttng-traces/*/ | head -1)"
DATE_NOW="$(date +%Y-%m-%d-%H-%M-%S)"
SCRIPT_PATH="${SCRIPT_DIR}/sftrace"
if [ -n "$1" ]; then
    ${SCRIPT_PATH} -f loglevel --clock-date --clock-gmt --no-delta "${TRACE_DIR}" > "$1.trace";
    echo "Written trace to file $1.trace"
else
    ${SCRIPT_PATH} -f loglevel --clock-date --clock-gmt --no-delta "${TRACE_DIR}" > "${DATE_NOW}.trace";
    echo "Written trace to file ${DATE_NOW}.trace"
fi