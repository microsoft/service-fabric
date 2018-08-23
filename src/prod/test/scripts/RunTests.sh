#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


pushd ../
export _NTTREE=$(pwd)

if [ "$1" == "-harness" ]
   then
     export LD_LIBRARY_PATH="/opt/microsoft/servicefabric/bin/Fabric/Fabric.Code"
     shift
     ulimit -c unlimited
   else
     export LD_LIBRARY_PATH="$(pwd)/lib"
fi

echo "Running as user $(whoami)"
echo "LB_LIBRARY_PATH IS $LD_LIBRARY_PATH"

export LinuxModeEnabled=true
export MALLOC_CHECK_=2

ModuleName="sfkxm"
DeviceFileName="/dev/sfkxm"

if [ "$(id -u)" == "0" ]; then
  echo "40000 65535" | sudo tee /proc/sys/net/ipv4/ip_local_port_range
fi

RecreateDir()
{
  if [ -d "$1" ]; then
      'rm' -rf $1
  fi
  mkdir $1
}

UnloadKxm()
{
  ispresent=`awk '$1=="sfkxm"' /proc/modules`
  if [[ $ispresent ]]; then
      sudo rmmod -f $ModuleName
  fi
  sudo rm -f $DeviceFileName
}

#Unload KXM module.
UnloadKxm

sfObjDir=/tmp/.service.fabric
RecreateDir $sfObjDir

if [ $SUDO_USER ]; then user=$SUDO_USER; else user=`whoami`; fi
chown $user:$user $sfObjDir
setfacl -m "d:u:$user:rwx,u:$user:rwx" $sfObjDir
setfacl -R -m "d:g:ServiceFabricAllowedUsers:rwx,g:ServiceFabricAllowedUsers:rwx" $sfObjDir

setfacl -R -m "d:g:users:rwx,g:users:rwx,d:g:ServiceFabricAllowedUsers:rwx,g:ServiceFabricAllowedUsers:rwx" $_NTTREE/test
setfacl -R -m "d:g:users:rwx,g:users:rwx,d:g:ServiceFabricAllowedUsers:rwx,g:ServiceFabricAllowedUsers:rwx" $_NTTREE/RunTests


waagentDir=/var/lib/sfcerts
if [ ! -d "${waagentDir}" ]; then
    sudo mkdir ${waagentDir}
    sudo chmod a+rwx ${waagentDir}
fi
waagentDirPerm=$(stat -c "%a" ${waagentDir})
if [ "$waagentDirPerm" -ne "777" ]; then
    sudo chmod a+rwx ${waagentDir}
fi

echo "Your $_NTTREE is: $_NTTREE"
popd
if [ $# -eq 0 ]; then
    dotnet RunTests.dll /sourcefiles:SingleMachine-CIT.json /types:FabricTest,FabricTest_Sequential,FederationTest,ExeTest,ExeTest_Sequential
elif [ "$1" == "/cloud" ]; then
    dotnet RunTests.dll /sourcefiles:SingleMachine-CIT.json /types:FabricTest,FabricTest_Sequential,FederationTest,ExeTest,ExeTest_Sequential /cloud
else 
    dotnet RunTests.dll $@ 
fi

EXITCODE=$?

#full CIT run requires sudo, we need to change permission to allow current user
chmod -R a+rw .

exit $EXITCODE