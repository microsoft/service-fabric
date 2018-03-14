#!/bin/bash
set -ex

CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT=$CDIR/..
DOCKER_VERSION=`cat $REPO_ROOT/DOCKER_VERSION`
REGISTRY=microsoft
IMAGE_NAME=service-fabric-build-ubuntu


BUILD_IMAGE_PATH=$REPO_ROOT/Dockerfile

docker build -t $REGISTRY/$IMAGE_NAME:latest -f $BUILD_IMAGE_PATH $REPO_ROOT 
docker tag $REGISTRY/$IMAGE_NAME:latest $REGISTRY/$IMAGE_NAME:$DOCKER_VERSION

# Release the official images to the registry.  Credentials required
if [ "$1" == "release" ]; then
    docker push $REGISTRY/$IMAGE_NAME:latest
    docker push $REGISTRY/$IMAGE_NAME:$DOCKER_VERSION
fi
