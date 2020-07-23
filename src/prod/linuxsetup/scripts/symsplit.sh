#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


droproot="$1"
filepath="${droproot}/$2"
symroot="$3/.build-id"
strip_cmd="$4"

filedir=`dirname "$2"`
filename=`basename "$2"`


# skip files that have already been split.  Files that have already
# been split will have the string "not stripped" when running the file command.

echo "running: file \"${filepath}\" | grep -F 'not stripped' | wc -l"
ret="$(file ${filepath} | grep -F 'not stripped' | wc -l)"
if [ "${ret}" == 0 ]; then
    echo "    Skipping ${filepath} - it is already stripped"
    exit 0
fi


buildid=`readelf -n ${filepath} | grep "Build ID" | cut -d : -f 2 | sed -e 's/^[[:space:]]*//'`
buildidhdr=`echo ${buildid} | cut -c1-2`
buildidtail=`echo ${buildid} | cut -c3-`

symdir=${symroot}/${buildidhdr}
if [ ! -d "${symdir}" ] ; then
    echo "creating symbol dir ${symdir}"
    mkdir -p "${symdir}"
fi

sympath="${symdir}/${buildidtail}.debug"

objcopy_cmd=$(echo ${strip_cmd} | sed 's/strip/objcopy/g')

echo "stripping ${filepath}, symbols stored at ${symdir}"
${objcopy_cmd} --only-keep-debug "${filepath}" "${sympath}"
${strip_cmd} --strip-debug --strip-unneeded "${filepath}"
${objcopy_cmd} --add-gnu-debuglink="${sympath}" "${filepath}"

chmod -x "${sympath}"
