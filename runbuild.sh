#!/bin/bash

# get the current directory
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DOCKER_VERSION=`cat $CDIR/DOCKER_VERSION`
IMAGE_NAME=microsoft/service-fabric-build-ubuntu

OUT_DIR=$CDIR/out
mkdir -p $OUT_DIR

BUILD_PARAMS=""
while (( "$#" )); do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
        cat <<EOF
Usage:
runbuild.sh [build arguments] [-h]
    -h, --help: Displays this help text

This script starts a container then calls src/build.sh within it. It 
accepts all build.sh arguments except -d. See below for the arguments
to build.sh.
EOF
        $CDIR/src/build.sh -h
        exit -1
    else
        BUILD_PARAMS="$BUILD_PARAMS $1"
    fi
    shift
done

echo "Checking locally for required image: $IMAGE_NAME:$DOCKER_VERSION..."
if [ "$(docker images -q $IMAGE_NAME:$DOCKER_VERSION 2> /dev/null)" == "" ]; then
    echo "Image not found locally... Checking Docker Hub."
    if [ ! $(curl --silent -f -lSL https://index.docker.io/v1/repositories/$IMAGE_NAME/tags/$DOCKER_VERSION 2> /dev/null) ]; then
        echo "Image $IMAGE_NAME:$DOCKER_VERSION not found on DockerHub."
        echo "Building image locally..."
        source $CDIR/tools/builddocker.sh
        echo "NOTE: Your local container image is now out of sync with the container on Docker Hub!"
    else
        echo "Image found on Docker Hub! Pulling $IMAGE_NAME:$DOCKER_VERSION from Docker Hub."
        docker pull $IMAGE_NAME:$DOCKER_VERSION
    fi
    
fi

echo "Running a build with container $IMAGE_NAME:$DOCKER_VERSION..."
docker run \
    --rm \
    --net=host \
    --cap-add=NET_ADMIN \
    -ti \
    -v "$OUT_DIR":/out \
    -v "$CDIR"/deps:/deps \
    -v "$CDIR"/src:/src \
    -e "BUILD_PARAMS=$BUILD_PARAMS" \
    $IMAGE_NAME:$DOCKER_VERSION \
    bash -c 'echo $BUILD_PARAMS && cd /out/ && /src/build.sh -all -d $BUILD_PARAMS'
