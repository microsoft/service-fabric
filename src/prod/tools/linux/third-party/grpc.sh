#!/bin/bash

VERSION=1.10.0
NAME=grpc
TAG=v1.10.0

REPO="https://github.com/grpc/grpc.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
#SRC_DIR=
#BIN_DIR=
LIB_DIR=${LIB_DIR}/grpc

init()
{
    _init_ ${REPO} ${TAG} ${SRC_DIR}
    cd ${SRC_DIR}
    git submodule update --init

    cd third_party
    rm -rf boringssl* # don't build boringssl, otherwise linking would fail
}

config()
{
    mkdir -p ${BIN_DIR}
    cd ${BIN_DIR}
    cmake -G "Unix Makefiles" ${SRC_DIR} \
        -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} \
        -DCMAKE_CXX_FLAGS="-fPIC -fshort-wchar -std=c++11 \
                           -Wno-error=unused-command-line-argument -nostdinc++ \
                           -I${LIBCXX_INSTALL_PREFIX}/include/c++/v1" \
        -DCMAKE_SHARED_LINKER_FLAGS="-stdlib=libc++ -nodefaultlibs" \
        -DCMAKE_CXX_STANDARD_LIBRARIES="-lpthread -lm -lssl -lcrypto\
                                        ${LIBCXX_INSTALL_PREFIX}/lib/libc++.a \
                                        ${LIBCXX_INSTALL_PREFIX}/lib/libc++abi.a" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${LIB_DIR} \
        -DHAVE_STD_REGEX=TRUE # A hack to bypass STD_REGEX check in benchmark subproject
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
    EMBED_OPENSSL=false make -j${NUM_PROC}
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build gRPC."
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

    cp -p ${BIN_DIR}/libgpr.a ${LIB_DIR}/libgpr.a
    cp -p ${BIN_DIR}/libgrpc.a ${LIB_DIR}/libgrpc.a
    cp -p ${BIN_DIR}/libgrpc++.a ${LIB_DIR}/libgrpcxx.a
    cp -p ${BIN_DIR}/libgrpc_cronet.a ${LIB_DIR}/libgrpc_cronet.a
    cp -p ${BIN_DIR}/libgrpc++_cronet.a ${LIB_DIR}/libgrpcxx_cronet.a
    cp -p ${BIN_DIR}/libgrpc++_error_details.a ${LIB_DIR}/libgrpcxx_error_details.a
    cp -p ${BIN_DIR}/libgrpc_plugin_support.a ${LIB_DIR}/libgrpc_plugin_support.a
    cp -p ${BIN_DIR}/libgrpc++_reflection.a ${LIB_DIR}/libgrpcxx_reflection.a
    cp -p ${BIN_DIR}/libgrpc_unsecure.a ${LIB_DIR}/libgrpc_unsecure.a
    cp -p ${BIN_DIR}/libgrpc++_unsecure.a ${LIB_DIR}/libgrpcxx_unsecure.a

    cp -p ${BIN_DIR}/third_party/cares/cares/lib/libcares.a ${LIB_DIR}/libares.a
    cp -p ${BIN_DIR}/third_party/protobuf/libprotobuf.a ${LIB_DIR}/libprotobuf.a
    cp -p ${BIN_DIR}/third_party/protobuf/libprotobuf-lite.a ${LIB_DIR}/libprotobuf-lite.a
    cp -p ${BIN_DIR}/third_party/protobuf/libprotoc.a ${LIB_DIR}/libprotoc.a
}

uninstall()
{
    rm -rf ${LIB_DIR}/*
}

# RUN (function defined in config.sh)
run $@
