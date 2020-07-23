#!/bin/bash

print_help()
{
    cat <<EOF
Usage:
builddocker.sh <ubuntu|ubuntu18|centos> [release]
    -h, --help: Displays this help text

Creates the docker image used in running the build.  Optionally, specify "release" to push the image to DockerHub (credentials required)
EOF
}

if [ "$#" -eq 0 ] || [ "$#" -gt 3 ]; then
    echo -e "Invalid parameters supplied.\n"
    print_help
    exit -1
fi

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    print_help
    exit 0
fi

CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT=$CDIR/../../

REGISTRY=microsoft

TARGET_OS="$1"
CAPS_OS_NAME=`echo $TARGET_OS | awk '{print toupper($0)}'`
IMAGE_VERSION=`cat $REPO_ROOT/tools/build/${CAPS_OS_NAME}IMAGE_VERSION`
IMAGE_NAME=service-fabric-build-${TARGET_OS}
BUILD_IMAGE_PATH=$REPO_ROOT/tools/build/Dockerfile.${TARGET_OS}

docker build -t $REGISTRY/$IMAGE_NAME:latest -f $BUILD_IMAGE_PATH $REPO_ROOT
docker tag $REGISTRY/$IMAGE_NAME:latest $REGISTRY/$IMAGE_NAME:$IMAGE_VERSION

# Release the official images to the registry.  Credentials required
# Check explicitly for ubuntu, as centos shouldn't be on DockerHub right now.
if [ "$2" == "release" ] && [ "$TARGET_OS" == "ubuntu" ]; then
    docker push $REGISTRY/$IMAGE_NAME:latest
    docker push $REGISTRY/$IMAGE_NAME:$IMAGE_VERSION
fi
