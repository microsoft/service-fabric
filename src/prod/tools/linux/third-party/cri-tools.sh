#!/bin/bash

VERSION=1.9
NAME=cri-tools
BRANCH=release-1.9

REPO="https://github.com/kubernetes-incubator/cri-tools.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
SRC_DIR=${GO_PATH}/src/github.com/kubernetes-incubator/cri-tools
#BIN_DIR=
LIB_DIR=${LIB_DIR}/cri-o

clean()
{
    cd ${SRC_DIR}
    make clean
}

init()
{
    _init_ ${REPO} ${BRANCH} ${SRC_DIR}
}

config()
{
    return
}

update()
{
    return
}

build()
{
    cd ${SRC_DIR}
    make
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build cri-tools."
        exit 1
    fi
}

install()
{
    cd ${SRC_DIR}
    make BINDIR=${LIB_DIR} install
}

uninstall()
{
    cd ${SRC_DIR}
    make BINDIR=${LIB_DIR} uninstall
}

# RUN (function defined in config.sh)
run $@
