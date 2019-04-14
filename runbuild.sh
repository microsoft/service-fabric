#!/bin/bash

# # get the current directory
CDIR_RUNBUILD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BUILD_PARAMS=""

# This string controls the container name that the build will be launched in, 
# e.g, "service-fabric-build-{ubuntu,centos,...}"
TARGET_OS="ubuntu"

# Add all runbuild parameters here.
while (( "$#" )); do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
        cat <<EOF
Usage:
runbuild.sh [build arguments] [-h] [-centos|-ubuntu18]
    -h, --help: Displays this help text
    -centos: Run the build against CentOS instead of the default Ubuntu 16.04
    -ubuntu18: Run the build against Ubuntu 18.04 instead of 16.04.
    -usemake: Run the build using make instead of ninja. Ubuntu only (requires clean build).
This script starts a container then calls src/build.sh within it. It
accepts all build.sh arguments except -d. See below for the arguments
to build.sh.
EOF
        $CDIR_RUNBUILD/src/build.sh -h
        exit -1
    elif [ "$1" == "-centos" ]; then
        TARGET_OS="centos"
    elif [ "$1" == "-ubuntu18" ]; then
        TARGET_OS="ubuntu18"
    elif [ "$1" == "-usemake" ]; then
        NINJA_FLAG=""
    else
        BUILD_PARAMS="$BUILD_PARAMS $1"
    fi
    shift
done

build_command="cd /out/ && /src/build.sh  ${NINJA_FLAG} -d $BUILD_PARAMS"
echo "Running Build for ${TARGET_OS} in docker..."
exec $CDIR_RUNBUILD/tools/docker/run_in_docker.sh $TARGET_OS "$build_command" 
