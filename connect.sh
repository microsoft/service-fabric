#!/bin/bash
# Creates a new container from the build environment image
# and drops you into an interactive prompt.
# From there you can run a build via the build script directly
# by going to /out and running /src/build.sh.

PrintUsage()
{
    cat <<EOF
This tool will launch a build container and drop you into a bash prompt.

connect.sh [-h]

  -h, --help: Show this help screen and exit
EOF
}

while (( "$#" )); do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
        PrintUsage
        exit -1
    else
        echo "Unexpected option $1"
        PrintUsage
        exit -2
    fi
done

# change directory to the one containing this script and
# record this directories full path.
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DOCKERIMAGE_VERSION=`cat "$CDIR"/tools/build/DOCKERIMAGE_VERSION`
OUT_DIR=$CDIR/out
echo "Running container version $DOCKERIMAGE_VERSION"
# net=host for connection on corpnet
# cap-add SYS_PTRACE for strace to work.
# Note that this has a relative path.  It assumes that
# the script is two folders from the root directory.
# the CDIR above will always make this script execute
# from the directory the script lives in.
docker run \
    --net=host \
    --cap-add NET_ADMIN \
    -ti \
    -v "$OUT_DIR":/out \
    -v "$CDIR"/deps:/deps \
    -v "$CDIR"/src:/src \
    microsoft/service-fabric-build-ubuntu:$DOCKERIMAGE_VERSION \
    bash -i
