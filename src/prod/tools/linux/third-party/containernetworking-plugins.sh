#!/bin/bash

VERSION=0.7.0
NAME=containernetworking-plugins
BRANCH=v0.7.0

REPO="https://github.com/containernetworking/plugins.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
SRC_DIR=${GO_PATH}/src/github.com/containernetworking/plugins
#BIN_DIR=
LIB_DIR=${LIB_DIR}/cri-o/cni

plugins=(
    bridge
    dhcp
    flannel
    host-device
    host-local
    ipvlan
    loopback
    macvlan
    portmap
    ptp
    sample
    tuning
    vlan
)

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
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build containernetworking-plugins."
        exit 1
    fi
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    for plugin in "${plugins[@]}"
    do
        cp -p ${SRC_DIR}/bin/${plugin} ${LIB_DIR}/${plugin}
    done
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi

    for plugin in "${plugins[@]}"
    do
        rm -f ${LIB_DIR}/${plugin}
    done
}

# RUN (function defined in config.sh)
run $@
