#!/bin/bash

print_help()
{
    cat <<EOF
Usage:
run_in_docker.sh [-h] <ubuntu|ubuntu18|centos> <command_to_run> 
    -h, --help: Displays this help text
    
Runs a command inside the WindowsFabric docker container

Example: 
    run_in_docker.sh ubuntu "/src/build.sh"
EOF
}

if [ "$#" -lt 1 ]; then
    echo -e "Missing parameters.\n"
    print_help
    exit -1
fi

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
	print_help
	exit 0
fi

# get the current directory
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

TARGET_OS="$1"
CAPS_OS_NAME=`echo $TARGET_OS | awk '{print toupper($0)}'`

REPOROOT=$CDIR/../../
IMAGE_VERSION=`cat $REPOROOT/tools/build/${CAPS_OS_NAME}IMAGE_VERSION`
FULL_IMAGE_NAME=microsoft/service-fabric-build-${TARGET_OS}

echo "Checking locally for required image: $FULL_IMAGE_NAME:$IMAGE_VERSION..."
if [ "$(docker images -q $FULL_IMAGE_NAME:$IMAGE_VERSION 2> /dev/null)" == "" ]; then
    echo "Image not found locally... Checking Docker Hub."
    if [ ! $(curl --silent -f -lSL https://index.docker.io/v1/repositories/$FULL_IMAGE_NAME/tags/$IMAGE_VERSION 2> /dev/null) ]; then
        echo "Image $FULL_IMAGE_NAME:$IMAGE_VERSION not found on DockerHub."
        echo "Building image locally..."
        eval $REPOROOT/tools/build/builddocker.sh ${TARGET_OS}
        echo "NOTE: Your local container image is now out of sync with the container on Docker Hub!"
    else
        echo "Image found on Docker Hub! Pulling $FULL_IMAGE_NAME:$IMAGE_VERSION from Docker Hub."
        docker pull $FULL_IMAGE_NAME:$IMAGE_VERSION
    fi

fi

if [ "$(docker images -q $FULL_IMAGE_NAME:$IMAGE_VERSION 2> /dev/null)" == "" ]; then
    echo "Docker either failed to build image $FULL_IMAGE_NAME:$IMAGE_VERSION  for target OS ${TARGET_OS} or OS is not currently supported. Exiting."
    exit 1
fi

OUT_DIR=$REPOROOT/out.${TARGET_OS}
mkdir -p $OUT_DIR

cmd=$2
echo -e "Running command:\n\t'$cmd'\n"
echo -e "in docker container with image:\n\t$FULL_IMAGE_NAME:$IMAGE_VERSION\n"
docker run \
    --rm \
    --net=host \
    --cap-add=NET_ADMIN \
    -ti \
    -v "$OUT_DIR":/out \
    -v "$REPOROOT"/external:/external \
    -v "$REPOROOT"/deps:/deps \
    -v "$REPOROOT"/src:/src \
    -v "$REPOROOT"/.config:/.config \
    -v "$REPOROOT"/tools/ci/scripts:/scripts \
    $FULL_IMAGE_NAME:$IMAGE_VERSION \
    bash -c "$cmd"
