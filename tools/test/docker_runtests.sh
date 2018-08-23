#!/bin/bash
# Wrapper for running tests inside a docker container.

CDIR_RUNTESTS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOTDIR=$CDIR_RUNTESTS/../../

OUTPUT_DIR="/out/build.prod/"
RUNTESTS_SH="RunTests.sh"
cmd="cd $OUTPUT_DIR/RunTests/ && ./RunTests.sh $@"
$ROOTDIR/tools/docker/run_in_docker.sh "$cmd" 

