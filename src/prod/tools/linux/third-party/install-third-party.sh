#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Usage:
#   To build/install all the libs:
#
#       ./install-third-party.sh
#
#   Verbosity can be set by passing option -v:
#
#       ./install-third-party.sh -v
#
#   To clean bin output:
#
#       ./install-third-party.sh -c
#
#   To clean and uninstall all the libs:
#
#       ./install-third-party.sh --cleanall


# All third-party libs
# The order matters because some libs depends on previous libs
libs=(
  clang
  llvm
  minizip
  boost
  jemalloc
  babeltrace
  cpprest
  protobuf
  grpc
#  cri-o
#  cri-tools
  containernetworking-plugins
  )

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null

# Clean up old log file:
rm -f ${ScriptPath}/../../../../../build.3rdparty.log

# Run all scripts
for lib in "${libs[@]}"
do
    ${ScriptPath}/$lib.sh $@
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install $lib"
        echo "See ${ROOT}/build.3rdparty.log"
        exit -1
    fi
done

