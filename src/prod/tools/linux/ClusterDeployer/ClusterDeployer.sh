#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null
BuildDir=${ScriptPath}/..

DropPath=$(pwd)/ClusterData
SkipDrop=false

# A subset of this script is used to set up harness runs.  All parts other than the ones that starts the FabricDeployer and FabricHost are run.
SetupHarnessRun="false"
if [ "$1" == "-h" ]
  then
     SetupHarnessRun="true"
fi


if [ "$(id -u)" != "0" ]; then
    echo "ClusterDeployer.sh must be run as root. Please rerun it with sudo."
    exit 1
fi

while (( "$#" )); do
    if [ "$1" == "-path" ]; then
        DropPath=$2
        shift
    elif [ "$1" == "-skipdrop" ]; then
        SkipDrop=true
    fi
    shift
done

DataRoot=${DropPath}/Data
BinPath=${BuildDir}/FabricDrop/bin
CodePath=${BuildDir}/FabricDrop/bin/Fabric/Fabric.Code
IPAddr=`ifconfig eth0 2>/dev/null|awk '/inet addr:/ {print $2}'|sed 's/addr://'`
if [ -z "$IPAddr" ]; then
IPAddr=`ifconfig | perl -ple 'print $_ if /inet addr/ and $_ =~ s/.*inet addr:((?:\d+\.){3}\d+).*/$1/g  ;$_=""' | grep -v ^\s*$ | grep -v 127.0.0.1 | grep -v 172.17.* | grep -v 10.88.* | head -n 1`
fi
if [ -z "$IPAddr" ]; then
IPAddr=`ifconfig | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1' | head -n 1`
fi

echo "Drop folder path is set to '$DropPath'"

DeployFabricBins()
{
echo "Current machine IP Address is $IPAddr"
echo "Deleting old data root content"
rm -rf ${DataRoot}

mkdir -p ${DataRoot}
mkdir -p ${DropPath}/ImageStore

rm ClusterManifest.SingleMachine.Replaced.xml
#cp ClusterManifest.SingleMachine.xml ClusterManifest.SingleMachine.Replaced.xml
cp ClusterManifest.SingleMachineFSS.xml ClusterManifest.SingleMachine.Replaced.xml
#cp ClusterManifest.SingleMachineFSS.X509.xml ClusterManifest.SingleMachine.Replaced.xml
#Uncomment following line to setup cluster with FabricUS
#cp ClusterManifest.SingleMachine.FSS.US.xml ClusterManifest.SingleMachine.Replaced.xml
sed -i "s%REPLACE_IP_1%${IPAddr}%g" ClusterManifest.SingleMachine.Replaced.xml
sed -i "s%REPLACE_DROPPATH%${DropPath}%g" ClusterManifest.SingleMachine.Replaced.xml

echo "Setting environment for FabricHost & FabricDeployer to run"
export LD_LIBRARY_PATH=${CodePath}

if [ ${SetupHarnessRun} == "false" ] ; then
    echo "Running CoreCLR FabricDeployer on ClusterManifest.SingleMachine.xml"
    ${CodePath}/FabricDeployer.sh /operation:Create /fabricBinRoot:${BinPath} /fabricDataRoot:${DataRoot} /cm:ClusterManifest.SingleMachine.Replaced.xml

    if [ $? != 0 ]; then
        echo Error: FabricDeployer.exe failed
        exit 1
    fi
fi
}

mkdir /etc/servicefabric
echo -n "${DataRoot}" > /etc/servicefabric/FabricDataRoot
echo -n "${DataRoot}/log" > /etc/servicefabric/FabricLogRoot
echo -n "${BinPath}" > /etc/servicefabric/FabricBinRoot
echo -n "${CodePath}" > /etc/servicefabric/FabricCodePath

if [ "$SkipDrop" == "false" ]; then
        DeployFabricBins
    fi

# Start tracing
# TODO: Move to TraceSessionManager after figuring out startring lttng as sfuser or root 
# Ex: lttng_handler.sh $serviceFabricVersion $traceDiskBufferSizeInMB $keepExistingSFSessions
echo "Starting tracing session"
traceDiskBufferSizeInMB=5120
keepExistingSFSessions=0
if command -v lttng >/dev/null 2>&1; then
    ./../FabricDrop/bin/Fabric/DCA.Code/lttng_handler.sh $(cat ./../FabricDrop/bin/Fabric/DCA.Code/ClusterVersion) $traceDiskBufferSizeInMB $keepExistingSFSessions
else
    echo "LTTng is not installed." >&2;
fi

echo "Cleaning up accounts and groups"
sudo userdel -rf G-FSSGroupffffffff > /dev/null 2>&1
sudo userdel -rf P_FSSUserffffffff  > /dev/null 2>&1
sudo userdel -rf S_FSSUserffffffff  > /dev/null 2>&1
for THIS in `awk -F':' '{ print $1}' /etc/passwd | grep WF-`
do
    sudo userdel -rf $THIS      > /dev/null 2>&1
done
sudo groupdel FSSMetaGroup      > /dev/null 2>&1
sudo groupdel FSSGroupffffffff  > /dev/null 2>&1
sudo groupdel P_FSSUserffffffff > /dev/null 2>&1
sudo groupdel S_FSSUserffffffff > /dev/null 2>&1
for THIS in `awk -F':' '{ print $1}' /etc/group | grep WF-`
do
    sudo groupdel $THIS         > /dev/null 2>&1
done

echo "Setting up accounts and groups"
sfuser_exists=false
getent passwd sfuser >/dev/null 2>&1 && sfuser_exists=true
if $sfuser_exists; then
   echo "sfuser exists..."
   usermod -g sfuser sfuser
else
   useradd sfuser -d /home/sfuser
   mkdir /home/sfuser
   chown sfuser:sfuser /home/sfuser
fi
chown sfuser:sfuser ${DropPath} -R

sfappsuser_exists=false
getent passwd sfappsuser >/dev/null 2>&1 && sfappsuser_exists=true
if $sfappsuser_exists; then
   echo "sfappsuser exists..."
   usermod -g sfuser sfappsuser
else
   useradd sfappsuser -d /home/sfappsuser -g sfuser
   mkdir /home/sfappsuser
   chown sfappsuser:sfappsuser /home/sfappsuser
fi

sfgrp_exists=false
getent group ServiceFabricAllowedUsers >/dev/null 2>&1 && sfgrp_exists=true
if $sfgrp_exists; then
   echo "ServiceFabricAllowedUsers exists..."
else
   groupadd ServiceFabricAllowedUsers
   groupadd ServiceFabricAdministrators
fi
usermod -a -G ServiceFabricAllowedUsers sfuser
usermod -a -G ServiceFabricAllowedUsers sfappsuser

if [ ! -f /etc/rssh.conf ]; then
   echo "Error: rssh not installed. Please run 'apt-get install rssh' to install, and restart the onebox."
   exit -1
fi

if [ ! -f "/usr/bin/cgget" ]; then
    echo "Error: cgroup-bin is not installed. Please run apt-get install cgroup-bin to install cgroup support"
    exit -1
fi

if ! grep -q "^allowscp" /etc/rssh.conf ; then
   echo "allowscp" >> /etc/rssh.conf
fi
if ! grep -q "^allowsftp" /etc/rssh.conf ; then
   echo "allowsftp" >> /etc/rssh.conf
fi

RecreateDir()
{
  if [ -d "$1" ]; then
      'rm' -rf $1
  fi
  mkdir $1
}

sfObjDir=/tmp/.service.fabric
RecreateDir $sfObjDir

if [ ! -d /var/lib/waagent ]; then
    sudo mkdir /var/lib/waagent
    chmod og+rx /var/lib/waagent
fi
if [ ! -d /var/lib/sfcerts ]; then
    sudo mkdir /var/lib/sfcerts
    chmod og+rx /var/lib/sfcerts
fi

if [ $SUDO_USER ]; then user=$SUDO_USER; else user=`whoami`; fi
chown $user:$user $sfObjDir
setfacl -m "d:u:$user:rwx,u:$user:rwx" $sfObjDir
setfacl -R -m "d:g:ServiceFabricAllowedUsers:rwx,g:ServiceFabricAllowedUsers:rwx" $sfObjDir

find ${DataRoot} -name libFabricCommon.so -exec rm -f {} \;

if [ ${SetupHarnessRun} == "false" ] ; then
    echo "Running FabricDeployer"
    cd $CodePath
    ./FabricDeployer.sh

    echo "Starting FabricHost"
    cd $BinPath
    ./FabricHost -c -skipfabricsetup
fi

exit
