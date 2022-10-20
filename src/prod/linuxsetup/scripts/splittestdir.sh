#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


rootdir="$1"
#symroot="$2"
symroot="$2/.build-id"

echo "$1"
echo "$2"

#1 is root of where the target binaries are

echo "rootdir is ${rootdir}"
for f in ${rootdir}/*.exe ${rootdir}/*.so; do
    echo "$f -"

    filedir=`dirname "$f"`
    filename=`basename "$f"`


    # skip files that have already been split.  Files that have already
    # been split will have the string "not stripped" when running the file command.
    echo "Running: file \"${f}\" | grep -F 'not stripped' | wc -l"
    ret="$(file ${f} | grep -F 'not stripped' | wc -l)"
    
    if [ "${ret}" == 0 ]; then
        echo "    Skipping ${f} - it is stripped"
        continue
    fi


    # this is managed
    if [ "${filename}" == "RunTests.exe" ]; then
        continue
    fi


    buildid=`readelf -n ${f} | grep "Build ID" | cut -d : -f 2 | sed -e 's/^[[:space:]]*//'`
    buildidhdr=`echo ${buildid} | cut -c1-2`
    buildidtail=`echo ${buildid} | cut -c3-`

    symdir=${symroot}/${buildidhdr}
    if [ ! -d "${symdir}" ] ; then
        echo "creating symbol dir ${symdir}"
        mkdir -p "${symdir}"
    fi

    sympath="${symdir}/${buildidtail}.debug"

    echo "    stripping ${f}, symbols stored at ${symdir}"
    objcopy --only-keep-debug "${f}" "${sympath}"
    strip --strip-debug --strip-unneeded "${f}"
    objcopy --add-gnu-debuglink="${sympath}" "${f}"

    chmod -x "${sympath}"
    
done