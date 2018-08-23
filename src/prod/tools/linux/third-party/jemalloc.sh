#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=5.0.1
NAME=jemalloc
TAG=5.0.1

REPO="https://github.com/jemalloc/jemalloc.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIBCXX_INSTALL_PREFIX}

init()
{
    _init_ ${REPO} ${TAG} ${SRC_DIR}
}

config()
{
    cd ${SRC_DIR}
    autoconf

    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    CC=${CC} CXX=${CXX} \
       CFLAGS="-fPIC" CXXFLAGS="-fPIC" \
       CPPFLAGS="-nostdinc++ -I${LIBCXX_INSTALL_PREFIX}/include/c++/v1" \
       LD_LIBRARY_PATH="${LIBCXX_INSTALL_PREFIX}/lib" \
       LDFLAGS="-stdlib=libc++" \
       ${SRC_DIR}/configure --prefix=${LIBCXX_INSTALL_PREFIX} --with-rpath=${LIBCXX_INSTALL_PREFIX}/lib
}

update()
{
    return
}

build()
{
    if [ ! -d ${BIN_DIR} ]; then
        config
    fi
    cd ${BIN_DIR}
    make build_lib_static -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build jemalloc."
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cp -p ${BIN_DIR}/lib/libjemalloc.a ${LIB_DIR}/lib
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf lib/libjemalloc*
}

# RUN (function defined in config.sh)
run $@