#!/bin/bash
# Runs a command inside the WindowsFabric docker container
# Usage: ./run_in_docker.sh <command_to_run> 
# Example: ./run_in_docker.sh "/src/build.sh"

# get the current directory
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPOROOT=$CDIR/../../
DOCKERIMAGE_VERSION=`cat $REPOROOT/tools/build/DOCKERIMAGE_VERSION`
FULL_IMAGE_NAME=microsoft/service-fabric-build-ubuntu

OUT_DIR=$REPOROOT/out
mkdir -p $OUT_DIR


echo "Checking locally for required image: $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION..."
if [ "$(docker images -q $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION 2> /dev/null)" == "" ]; then
    echo "Image not found locally... Checking Docker Hub."
    if [ ! $(curl --silent -f -lSL https://index.docker.io/v1/repositories/$FULL_IMAGE_NAME/tags/$DOCKERIMAGE_VERSION 2> /dev/null) ]; then
        echo "Image $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION not found on DockerHub."
        echo "Building image locally..."
        eval $REPOROOT/tools/build/builddocker.sh
        echo "NOTE: Your local container image is now out of sync with the container on Docker Hub!"
    else
        echo "Image found on Docker Hub! Pulling $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION from Docker Hub."
        docker pull $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION
    fi

fi

cmd=$@
echo -e "Running command:\n\t'$cmd'\n"
echo -e "in docker container with image:\n\t$FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION\n"
docker run \
    --rm \
    --net=host \
    --cap-add=NET_ADMIN \
    -ti \
    -v "$OUT_DIR":/out \
    -v "$REPOROOT"/deps:/deps \
    -v "$REPOROOT"/external:/external \
    -v "$REPOROOT"/src:/src \
    -v "$REPOROOT"/.config:/.config \
    $FULL_IMAGE_NAME:$DOCKERIMAGE_VERSION \
    bash -c "$cmd"