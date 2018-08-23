#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=1.5
NAME=babeltrace
BRANCH=sf-1.5

REPO="https://github.com/jeffreychenmsft/babeltrace.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIB_DIR}/babeltrace

init()
{
    _init_ ${REPO} ${BRANCH} ${SRC_DIR}
}

config()
{
    cd ${SRC_DIR}
    ./bootstrap

    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    CC=${CC} CFLAGS="-fPIC" ${SRC_DIR}/configure \
        --enable-static=no \
        --enable-silent-rules \
        --disable-debug-info \
        --prefix=${LIB_DIR} \
        --program-transform-name="s/babeltrace/sftrace/g"
}

update()
{
    cd ${SRC_DIR}
    git pull
}

build()
{
    if [ ! -d ${BIN_DIR} ]; then
        config
    fi
    cd ${BIN_DIR}
    make V=${VERBOSE} -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build babeltrace"
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${BIN_DIR}
    make V=${VERBOSE} install
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install babeltrace"
        exit 1
    fi
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf libbabeltrace*
    rm -rf sftrace*
}

# RUN (function defined in config.sh)
clean_all
run $@
