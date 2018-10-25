#!/bin/bash

# get the current directory
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DOCKERIMAGE_VERSION=`cat $CDIR/tools/build/DOCKERIMAGE_VERSION`
FULL_IMAGE_NAME=microsoft/service-fabric-build-ubuntu

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

echo "Checking locally for required image: $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION..."
if [ "$(docker images -q $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION 2> /dev/null)" == "" ]; then
    echo "Image not found locally... Checking Docker Hub."
    if [ ! $(curl --silent -f -lSL https://index.docker.io/v1/repositories/$FULL_IMAGE_NAME/tags/$DOCKERIMAGE_VERSION 2> /dev/null) ]; then
        echo "Image $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION not found on DockerHub."
        echo "Building image locally..."
        eval $CDIR/tools/build/builddocker.sh
        echo "NOTE: Your local container image is now out of sync with the container on Docker Hub!"
    else
        echo "Image found on Docker Hub! Pulling $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION from Docker Hub."
        docker pull $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION
    fi

fi

echo "Running a build with container $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION..."

# Note: Only debug builds work right now with Clang 6.0.
docker run \
    --rm \
    --net=host \
    --cap-add=NET_ADMIN \
    -ti \
    -v "$OUT_DIR":/out \
    -v "$CDIR"/deps:/deps \
    -v "$CDIR"/external:/external \
    -v "$CDIR"/src:/src \
    -v "$CDIR"/.config:/.config \
    -e "BUILD_PARAMS=$BUILD_PARAMS" \
    $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION \
    bash -c 'echo $BUILD_PARAMS && cd /out/ && /src/build.sh -all -d $BUILD_PARAMS'
