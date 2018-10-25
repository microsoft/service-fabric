#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# This file is to be used by other script files to import global variables and
# functions. Before sourcing this file, the script file needs to have variable
# NAME and VERSION defined.

# Sanity check
# NAME and VERSION of the third-party library. These two variables needs to be
# set in each script file.
if [ -z ${NAME+x} ]; then
    echo "ERROR: NAME is not set in $0"
    exit -1
fi
if [ -z ${VERSION+x} ]; then
    echo "ERROR: VERSION is not set in $0"
    exit -1
fi

DIR_NAME=${NAME}-${VERSION}

# Setting global variables below

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null
ROOT=${ScriptPath}/../../../../../

# Number of processors on this machine
NUM_PROC=`nproc`

DEPS_ROOT=${ROOT}/deps
# Directory for the source code
SRC_DIR=${DEPS_ROOT}/third-party/src/${NAME}

# Root directory for go project
GO_PATH=${DEPS_ROOT}/third-party/src/go

# Directory for the binary output
BIN_DIR=${DEPS_ROOT}/third-party/build/${NAME}

# Default install prefix
LIB_DIR=${DEPS_ROOT}/third-party/bin/

# libc++ install prefix
LIBCXX_INSTALL_PREFIX=${LIB_DIR}/Cxx

# boost install prefix
BOOST_INSTALL_PREFIX=${LIB_DIR}/Boost_1_61_0

# clang install prefix
CLANG_INSTALL_PREFIX=${LIB_DIR}/clang

# Compiler
if [ -z ${CC} ]; then
    CC=${CLANG_INSTALL_PREFIX}/bin/clang
fi
if [ -z ${CXX} ]; then
    CXX=${CLANG_INSTALL_PREFIX}/bin/clang++
fi

# Variables that control build
# Verbosity can be set by passing option -v in the command line
VERBOSE=0
LOG=${ROOT}/build.3rdparty.log


# Logic for install/update third-party library. Script sourcing this file needs
# to define these functions: init, update, build, intall, config, uninstall
# This function checks if directory ${DIR_NAME} exists (e.g. llvm-release_50,
# boost-1.61.1). If ${DIR_NAME} does not exist, pull and build the library.
main()
{
    if [ ! -d ${SRC_DIR} ]; then
        clean_bin
        init
        config
    fi
    build
    install
}

# clone specified 'repo' to the specified 'src_dir' and checkout to the
# specified 'branch/tag'
# Args:
#   repo
#   branch/tag
#   src_dir
_init_()
{
    local repo=$1
    local branch=$2
    local src_dir=$3
    mkdir -p ${src_dir}
    git clone ${repo} ${src_dir}
    cd ${src_dir}
    git checkout ${branch}
}

# Delete the binary output dir.
clean_bin()
{
    echo "Removing output dir: ${BIN_DIR}"
    rm -rf ${BIN_DIR}
}

# Uninstall the lib, delete both source dir and bin output dir. Can be used to
# start a fresh new clone and build.
clean_all()
{
    echo "Uninstalling lib"
    uninstall

    echo "Removing output dir: ${BIN_DIR}"
    rm -rf ${BIN_DIR}

    echo "Removing source dir: ${SRC_DIR}"
    rm -rf ${SRC_DIR}
}

print_help()
{
    echo "Usage: $0 [-options]"
    echo "Options:"
    echo "-h|--help: print help message"
    echo "-v|--verbose: verbose build"
    echo "-c|--clean: delete binary output"
    echo "-j: parallel build"
    echo "--build-dir: directory for build output. Recommended"
    echo "--install-prefix: directory for installation"
    echo "--cleanall: uninstall lib, delete both source directory and binary"\
         "output directory"
    echo "--uninstall: remove installed files"
    exit 1
}

run()
{
    local CLEAN_BIN=0
    local CLEAN_ALL=0
    local UNINSTALL=0

    while [[ $# -gt 0 ]]
    do
    case "$1" in
        -h|--help)
        print_help
        ;;
        -v|--verbose)
        VERBOSE=1
        shift
        ;;
        -c|--clean)
        CLEAN_BIN=1
        shift
        ;;
        --cleanall)
        CLEAN_ALL=1
        shift
        ;;
        --build-dir)
        BIN_DIR=$2/${DIR_NAME}
        shift
        shift
        ;;
        --install-prefix)
        LIB_DIR=$2
        shift
        shift
        ;;
        --uninstall)
        UNINSTALL=1
        shift
        ;;
        -j)
        if [ $2 -gt 0 ]; then
            NUM_PROC=$2
        fi
        shift
        shift
        ;;
        *)
        echo $0 $@
        echo "ERROR: Unkown option $1"
        print_help
        ;;
    esac
    done

    if [ ${CLEAN_ALL} -eq 1 ]; then
        clean_all
        exit 0
    fi
    if [ ${CLEAN_BIN} -eq 1 ]; then
        clean_bin
        exit 0
    fi
    if [ ${UNINSTALL} -eq 1 ]; then
        uninstall
        exit 0
    fi

    echo "Building and installing ${NAME}..."
    if [ ${VERBOSE} -eq 1 ]; then
        # Write both stderr and stdout to console AND the log file.
        main |& tee -a ${LOG}
    else
        # Write only the error to stdout, and write both stderr and stdout to the file.
        # Note that the ordering is not guaranteed because there's a race condition between pipe to tee
        # and redirect to file.         
        main 2>&1 >>${LOG} | tee --append ${LOG}
    fi
    if [ $? != 0 ]; then
        echo "ERROR: Failed to build/install ${NAME}"
        exit 1
    fi
}
