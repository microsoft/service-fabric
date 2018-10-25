#!/bin/bash

VERSION=1.9.7
NAME=cri-o
BRANCH=v1.9.7

REPO="https://github.com/kubernetes-incubator/cri-o.git"

# Import variables and functions from config.sh
ScriptPath=`dirname $0`
source ${ScriptPath}/config.sh

# Uncomment variables below to overwrite global settings
#CC=
#CXX=
SRC_DIR=${GO_PATH}/src/github.com/kubernetes-incubator/cri-o
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
        echo "ERROR: Failed to build cri-o."
        exit 1
    fi
}

generateMyNetConf()
{
echo '{
    "cniVersion": "0.2.0",
    "name": "mynet",
    "type": "bridge",
    "bridge": "cni0",
    "isGateway": true,
    "ipMasq": true,
    "ipam": {
        "type": "host-local",
        "subnet": "10.88.0.0/16",
        "routes": [
            { "dst": "0.0.0.0/0"  }
        ]
    }
}
' > ${LIB_DIR}/cni/10-mynet.conf
}

install()
{
    if [ ! -d ${LIB_DIR} ]; then
        mkdir -p ${LIB_DIR}
    fi
    cp -p ${SRC_DIR}/bin/pause ${LIB_DIR}/pause
    cp -p ${SRC_DIR}/bin/conmon ${LIB_DIR}/conmon
    cp -p ${SRC_DIR}/bin/crio ${LIB_DIR}/crio
    cp -p ${SRC_DIR}/crio.conf ${LIB_DIR}/crio.conf
    cp -p ${SRC_DIR}/seccomp.json ${LIB_DIR}/seccomp.json

    if [ ! -d ${LIB_DIR}/cni ]; then
        mkdir -p ${LIB_DIR}/cni
    fi
    cp -p ${SRC_DIR}/contrib/cni/99-loopback.conf ${LIB_DIR}/cni/99-loopback.conf
    generateMyNetConf
}

uninstall()
{
    if [ ! -d ${LIB_DIR} ]; then
        return
    fi

    rm -f ${LIB_DIR}/crio
    rm -f ${LIB_DIR}/pause
    rm -f ${LIB_DIR}/conmon
    rm -f ${LIB_DIR}/crio.conf
    rm -f ${LIB_DIR}/seccomp.json
    rm -f ${LIB_DIR}/cni/99-loopback.conf
    rm -f ${LIB_DIR}/cni/10-mynet.conf
}

# RUN (function defined in config.sh)
run $@
