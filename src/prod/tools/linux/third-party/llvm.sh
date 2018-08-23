#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=3.6
NAME=llvm
BRANCH=release_36

LLVM_REPO="https://github.com/llvm-mirror/llvm.git"
LIBCXX_REPO="https://github.com/llvm-mirror/libcxx.git"
LIBCXXABI_REPO="https://github.com/llvm-mirror/libcxxabi.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIBCXX_INSTALL_PREFIX}

LLVM_SRC_DIR=${SRC_DIR}
LIBCXX_SRC_DIR=${LLVM_SRC_DIR}/projects/libcxx
LIBCXXABI_SRC_DIR=${LLVM_SRC_DIR}/projects/libcxxabi

prune_unused()
{
    pushd `pwd` > /dev/null
    cd ${SRC_DIR}    
    rm -rf ./docs/_themes
    rm -rf ./test/CodeGen
    popd > /dev/null
}

init()
{
    _init_ ${LLVM_REPO} ${BRANCH} ${LLVM_SRC_DIR}
    _init_ ${LIBCXX_REPO} ${BRANCH} ${LIBCXX_SRC_DIR}
    _init_ ${LIBCXXABI_REPO} ${BRANCH} ${LIBCXXABI_SRC_DIR}
    prune_unused
}

config()
{
    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    cmake -G 'Unix Makefiles' ${LLVM_SRC_DIR} \
          -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} \
          -DCMAKE_CXX_FLAGS='-fPIC -fshort-wchar' \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=${LIB_DIR} \
          -DLIBCXX_ENABLE_SHARED=OFF \
          -DLIBCXXABI_ENABLE_SHARED=OFF
}

update()
{
    cd ${LLVM_SRC_DIR}
    git pull

    cd ${LIBCXX_SRC_DIR}
    git pull

    cd ${LIBCXXABI_SRC_DIR}
    git pull
}

build()
{
    if [ ! -d ${BIN_DIR} ]; then
        config
    fi
    cd ${BIN_DIR}
    make cxx -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build libcxx"
        exit 1
    fi
    make cxxabi -j${NUM_RPOC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build libcxxabi"
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${BIN_DIR}/projects
    make install
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf include/c++
    rm -rf lib/libc++*
}

# RUN (function defined in config.h)
run $@
