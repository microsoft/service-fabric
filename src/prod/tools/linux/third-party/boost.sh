#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=1.61.0
NAME=boost
TAG=boost-1.61.0

REPO="https://github.com/boostorg/boost.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#NUM_PROC=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${BOOST_INSTALL_PREFIX}
#LIBCXX_INSTALL_PREFIX=

# Header files will be installed in ${BOOST_INSTALL_PREFIX}/boost
BOOST_INCLUDE_DIR=${LIB_DIR}

# Lib will be installed in ${BOOST_INSTALL_PREFIX}/lib
BOOST_LIB_DIR=${LIB_DIR}/lib

prune_unused()
{
    pushd `pwd` > /dev/null
    cd ${SRC_DIR}    
    # Remove unnecessary doc files.
    rm -r ./libs/**/doc/*
    popd
}

init()
{
    _init_ ${REPO} ${TAG} ${SRC_DIR}
    cd ${SRC_DIR}
    git submodule update --init --recursive
    prune_unused
}

config()
{
    cd ${SRC_DIR}    
    # lib "serialization" doesn't build with -fshort-wchar
    ./bootstrap.sh -with-libraries="chrono,filesystem,test,random,regex,system,thread,timer"
    ./b2 headers
}

update()
{
    # do nothing. To update, always use a new TAG
    return
}

build()
{
    if [ ! -d ${BIN_DIR} ]; then
        mkdir -p ${BIN_DIR}
    fi
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${SRC_DIR}
    ./b2 --build-dir=${BIN_DIR} -q \
         --includedir=${BOOST_INCLUDE_DIR} \
         --libdir=${BOOST_LIB_DIR} \
         variant=release \
         include=${LIBCXX_INSTALL_PREFIX}/include/c++/v1 \
         cflags="-fPIC" \
         cxxflags="-nostdinc++ -fshort-wchar -fPIC" \
         linkflags="-nodefaultlibs -Wl,-rpath,${BOOST_LIB_DIR} \
                    ${LIBCXX_INSTALL_PREFIX}/lib/libc++.a \
                    ${LIBCXX_INSTALL_PREFIX}/lib/libc++abi.a" \
         install -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build boost"
        exit 1
    fi
}

install()
{
    echo "Run build to install"
    return
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf boost
    rm -rf lib
}

# RUN (function defined in config.sh)
run $@
