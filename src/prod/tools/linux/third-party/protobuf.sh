#!/bin/bash

VERSION=3.5.1.1
NAME=protobuf
TAG=v3.5.1.1

REPO="https://github.com/google/protobuf.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIB_DIR}/protobuf

init()
{
    _init_ ${REPO} ${TAG} ${SRC_DIR}
    cd ${SRC_DIR}
    ./autogen.sh
}

config()
{
    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    ${SRC_DIR}/configure CC=${CC} CXX=${CXX} \
         CPPFLAGS="-I${LIBCXX_INSTALL_PREFIX}/include/c++/v1" \
         CXXFLAGS="-stdlib=libc++ -fPIC" \
         LDFLAGS="${LIBCXX_INSTALL_PREFIX}/lib/libc++.a ${LIBCXX_INSTALL_PREFIX}/lib/libc++abi.a" \
         --prefix=${LIB_DIR}
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
    make -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build protobuf."
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cd ${BIN_DIR}
    make install -j${NUM_PROC}
}

uninstall()
{
    rm -rf ${LIB_DIR}/bin
    rm -rf ${LIB_DIR}/include
    rm -rf ${LIB_DIR}/lib
}

# RUN (function defined in config.sh)
run $@
