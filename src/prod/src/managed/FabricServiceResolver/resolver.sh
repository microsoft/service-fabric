#!/usr/bin/env bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

echo 0x3f > /proc/self/coredump_filter

. /etc/os-release
linuxDistrib=$ID
if [ $linuxDistrib = "rhel" ]; then
  source scl_source enable rh-dotnet20
  exitCode=$?
  if [ $exitCode != 0 ]; then
  	echo "Failed: source scl_source enable rh-dotnet20 : ExitCode: $exitCode"
  	exit $exitCode
  fi
fi
exec dotnet FabricServiceResolver.dll "$@"