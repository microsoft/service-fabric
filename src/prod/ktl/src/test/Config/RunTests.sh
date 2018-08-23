#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KXMDIR=$SCRIPTDIR/../kxm
#disable kxm loading by default.
skipkxm=1
#Unload module if script was terminated due to ctrl+c
trap UnloadKxm INT

pushd ../
export _NTTREE=$(pwd)
export LD_LIBRARY_PATH="$(pwd)/lib"
export LinuxModeEnabled=true
export MALLOC_CHECK_=2

RecreateDir()
{
  if [ -d "$1" ]; then
      'rm' -rf $1
  fi
  mkdir $1
}

SetupKxm()
{
    if [ -x $KXMDIR/InstallKxm.sh ]; then
      if [ "$EUID" -ne 0 ]; then
        echo "Please run as root or sudo"
        echo "KXM Setup failed."
        exit -1
      fi

      #Invoke InstallKxm script to build & install KXM module. 
      $KXMDIR/InstallKxm.sh -u -l -s $KXMDIR/ -o
      if [ $? != 0 ]; then
        echo "KXMSetup failed."
        exit -1
      fi
    fi
}

UnloadKxm()
{
  #Unload KXM module.
    if [ -x $KXMDIR/InstallKxm.sh ]; then
        $KXMDIR/InstallKxm.sh -u > /dev/null
    fi
}

#Skip driver loading if /skipkxm param is passed.
for arg do
  shift
  if [ "$arg" = "/skipkxm" ]; then
    skipkxm=1
    continue;
  fi
  set -- "$@" "$arg"
done
if [ $skipkxm -eq 0 ]; then
   SetupKxm
else
   UnloadKxm
   echo "Skipping Kxm setup."
   echo "WARNING: Ktl Iocp and blockfile operations would run with user emulation code."
fi

ktlObjDir=/tmp/.ktl
RecreateDir $ktlObjDir

if [ $SUDO_USER ]; then user=$SUDO_USER; else user=`whoami`; fi

chown $user:$user $ktlObjDir
setfacl -m "d:u:$user:rwx,u:$user:rwx" $ktlObjDir
setfacl -R -m "d:g:KtlAllowedUsers:rwx,g:KtlAllowedUsers:rwx" $ktlObjDir
setfacl -R -m "d:g:users:rwx,g:users:rwx,d:g:KtlAllowedUsers:rwx,g:KtlAllowedUsers:rwx" $_NTTREE/test

echo $_NTTREE
popd
mono RunTests.exe $@
EXITCODE=$?

#Unload KXM module.
UnloadKxm

exit $EXITCODE
