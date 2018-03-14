#!/bin/bash

VERSION=5.0
NAME=clang
BRANCH="merge0307"

LLVM_REPO="https://github.com/GorNishanov/llvm.git"
CLANG_REPO="https://github.com/GorNishanov/clang.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
#LIB_DIR=

LLVM_SRC_DIR=${SRC_DIR}
CLANG_SRC_DIR=${LLVM_SRC_DIR}/tools/clang

init()
{
    _init_ ${LLVM_REPO} ${BRANCH} ${LLVM_SRC_DIR}
    _init_ ${CLANG_REPO} ${BRANCH} ${CLANG_SRC_DIR}
}

config()
{
    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    cmake -G 'Unix Makefiles' ${LLVM_SRC_DIR} \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=${CLANG_INSTALL_PREFIX}
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
    make clang -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build clang"
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${BIN_DIR}
    make install-clang -j${NUM_PROC}
    make install-clang-headers -j${NUM_PROC}
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf bin
    rm -rf lib
}

# RUN (function defined in config.h)
run $@
