#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=28
NAME=cpprest
BRANCH=zhol_ubuntu1804

REPO="https://github.com/kavyako/cpprestsdk.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIB_DIR}/casablanca_v_2
#BOOST_INSTALL_PREFIX=
#LIBCXX_INSTALL_PREFIX=

init()
{
    _init_ ${REPO} ${BRANCH} ${SRC_DIR}
}

config()
{
    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    cmake ${SRC_DIR}/Release \
        -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} \
        -DCMAKE_CXX_FLAGS="-fPIC -fshort-wchar -std=c++11 \
                           -Wno-error=unused-command-line-argument -nostdinc++ \
                           -I${LIBCXX_INSTALL_PREFIX}/include/c++/v1" \
        -DCMAKE_SHARED_LINKER_FLAGS="-stdlib=libc++ -nodefaultlibs" \
        -DCMAKE_CXX_STANDARD_LIBRARIES="${LIBCXX_INSTALL_PREFIX}/lib/libc++.a \
                                        ${LIBCXX_INSTALL_PREFIX}/lib/libc++abi.a" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${LIB_DIR} \
        -DBUILD_SAMPLES=OFF -DBUILD_TESTS=OFF \
        -DBOOST_ROOT=${BOOST_INSTALL_PREFIX} \
        -DBOOST_LIBRARYDIR=${BOOST_INSTALL_PREFIX}/lib
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
    make -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build cpprest."
        exit 1
    fi
}

install() {
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${BIN_DIR}
    make install
    rm -rf ${LIB_DIR}/Binaries
    cp -rfp ${BIN_DIR}/Binaries ${LIB_DIR}
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf Binaries
    rm -rf include/cpprest
    rm -rf include/pplx
    rm -rf lib/libcpprest*
}

# RUN (function defined in config.sh)
run $@
