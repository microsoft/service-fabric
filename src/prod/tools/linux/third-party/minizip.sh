#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


VERSION=1.2.11
NAME=zlib
TAG=v1.2.11

REPO="https://github.com/madler/zlib.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#SRC_DIR=
#BIN_DIR=

LIB_DIR=${LIB_DIR}/minizip

LIBZ_SRC_DIR=${SRC_DIR}
LIBMINIZIP_SRC_DIR=${SRC_DIR}/contrib/minizip

# Binary output directories for zlib and minizip
LIBZ_BIN_DIR=${BIN_DIR}/zlib
LIBMINIZIP_BIN_DIR=${BIN_DIR}/minizip

# Header files will be installed in
LIBZ_INC_DIR=${LIB_DIR}/minizip
LIBMINIZIP_INC_DIR=${LIB_DIR}

prune_unused()
{
    pushd `pwd` > /dev/null
    cd ${SRC_DIR}
    find ./contrib/* | grep -v "minizip/**" | xargs rm -rf
    rm -rf ./amiga
    popd > /dev/null
}
init()
{
    _init_ ${REPO} ${TAG} ${LIBZ_SRC_DIR}
    cd ${LIBMINIZIP_SRC_DIR}
    prune_unused
    autoreconf -i
}

config_libz()
{
    mkdir -p ${LIBZ_BIN_DIR}
    cd ${LIBZ_BIN_DIR}
    CC=${CC} CFLAGS="-fPIC" ${LIBZ_SRC_DIR}/configure --static \
        --includedir=${LIBZ_INC_DIR} --libdir=${LIB_DIR}
}

config_libminizip()
{
    mkdir -p ${LIBMINIZIP_BIN_DIR}
    cd ${LIBMINIZIP_BIN_DIR}
    CC=${CC} CFLAGS="-fPIC" ${LIBMINIZIP_SRC_DIR}/configure --enable-shared=no \
        --includedir=${LIBMINIZIP_INC_DIR} --libdir=${LIB_DIR}
}

config()
{
    config_libz
    config_libminizip
}

update()
{
    return
}

build_libz()
{
    if [ ! -d ${LIBZ_BIN_DIR} ]; then
        config_libz
    fi
    cd ${LIBZ_BIN_DIR}
    make -j${NUM_PROC}
}

build_libminizip()
{
    if [ ! -d ${LIBMINIZIP_BIN_DIR} ]; then
        config_libminizip
    fi
    cd ${LIBMINIZIP_BIN_DIR}
    make V=${VERBOSE} -j${NUM_PROC}
}

build()
{
    build_libz
    build_libminizip
}

install_libz()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cp -p ${LIBZ_BIN_DIR}/libz.a ${LIB_DIR}
}

install_libminizip()
{
    if [ ! -d ${LIB_DIR}/minizip ]; then
        mkdir -p ${LIB_DIR}/minizip
    fi
    cp -p ${LIBMINIZIP_SRC_DIR}/crypt.h ${LIB_DIR}/minizip
    cp -p ${LIBMINIZIP_SRC_DIR}/ioapi.h ${LIB_DIR}/minizip
    cp -p ${LIBMINIZIP_SRC_DIR}/mztools.h ${LIB_DIR}/minizip
    cp -p ${LIBMINIZIP_SRC_DIR}/unzip.h ${LIB_DIR}/minizip
    cp -p ${LIBMINIZIP_SRC_DIR}/zip.h ${LIB_DIR}/minizip

    cp -p ${LIBMINIZIP_BIN_DIR}/.libs/libminizip.a ${LIB_DIR}
}

install()
{
    install_libz
    install_libminizip
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi
    cd ${LIB_DIR}
    rm -rf libminizip*
    rm -rf libz.a
    rm -rf minizip
    rm -rf pkgconfig
}

# RUN (defined in config.sh)
run $@