#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

if [ "$#" -ne 2 ]; then
    echo "Illegal number of arguments passed in"
    echo "Usage is collecttraces.sh <Path_To_Traces_Folder> <Trace_Output_Folder>"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="${SCRIPT_DIR}"

TRACE_DIR=$(echo $1 | sed 's:/*$::')
TRACE_INPUT_DIR="$(ls -t -d ${TRACE_DIR}/*/ | head -1)"
OUTPUT_DIR=$(echo $2 | sed 's:/*$::')
SFTRACE="${SCRIPT_DIR}/sftrace"
DATE_NOW="$(date +%Y-%m-%d-%H-%M-%S)"

echo Stopping session
lttng stop servicefabric
lttng destroy servicefabric

echo Converting traces from ${TRACE_DIR} and placing at ${OUTPUT_DIR}
${SFTRACE} -f loglevel --clock-date --clock-gmt --no-delta "${TRACE_INPUT_DIR}" > "${OUTPUT_DIR}/${DATE_NOW}.trace";

echo Starting session
lttng create --output ${TRACE_DIR}/${DATE_NOW} servicefabric
lttng enable-channel --userspace --session servicefabric --tracefile-size 1000M --tracefile-count 1 --subbuf-size 5M servicefabric-channel
lttng enable-event --channel servicefabric-channel --userspace "service_fabric:*"
lttng add-context --userspace --channel servicefabric-channel --type vtid
lttng add-context --userspace --channel servicefabric-channel --type vpid
lttng start