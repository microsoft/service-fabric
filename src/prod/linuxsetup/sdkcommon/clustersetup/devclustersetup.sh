#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

SkipDeleteRoot=false
FabricDataRoot=/home/sfuser/sfdevcluster/data
FabricBinRoot=/opt/microsoft/servicefabric/bin
ClusterManifestPath=/opt/microsoft/sdk/servicefabric/common/clustersetup/nonsecure/servicefabric-scalemin.xml
DeployForMesh=false

for i in "$@"
do
case $i in
    -cdr=*|--clusterdataroot=*)
    FabricDataRoot="${i#*=}"
    ;;
    -sd|--skipdeletedata)
    SkipDeleteRoot=true
    ;;
    --deployForMesh)
    DeployForMesh=true
    ;;
    *)
    echo Error: Unknown option passed in
    exit 1
    ;;
esac
done

if [ "false" = "$SkipDeleteRoot" ]; then
    echo Deleting all old content in FabricDataRoot ${FabricDataRoot}
    rm -rf ${FabricDataRoot}/*
fi

sfctl > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo Could not find sfctl. Installing sfctl...
    apt-get install python-pip
    pip install sfctl
fi

if [ "true" = "$DeployForMesh" ]; then
    ClusterManifestPath=/opt/microsoft/sdk/servicefabric/common/clustersetup/nonsecure_mesh/servicefabric-scalemin.xml
    
    PrimaryInterface=$(route | grep 'default' | grep -o '[^ ]*$')
    sed -i "s/InterfaceNameParameter/${PrimaryInterface}/g" $ClusterManifestPath

    IPAddr=`ifconfig eth0 2>/dev/null|awk '/inet addr:/ {print $2}'|sed 's/addr://'`
    if [ -z "$IPAddr" ]; then
      IPAddr=`ifconfig | perl -ple 'print $_ if /inet addr/ and $_ =~ s/.*inet addr:((?:\d+\.){3}\d+).*/$1/g  ;$_=""' | grep -v ^\s*$ | grep -v 127.0.0.1 | grep -v 172.17.* | grep -v 10.88.* | head -n 1`
    fi
    if [ -z "$IPAddr" ]; then
      IPAddr=`ifconfig | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1' | head -n 1`
    fi

    sed -i "s/localhost/${IPAddr}/g" $ClusterManifestPath
fi

echo Deploying service fabric scale min cluster
${FabricBinRoot}/Fabric/Fabric.Code/deployfabric.sh -fdr=${FabricDataRoot} -cm=${ClusterManifestPath}
if [ $? != 0 ]; then
    echo Error: Deploying cluster manifest failed. Check output for details
    exit 1
fi

echo Starting service
systemctl start servicefabric
if [ $? != 0 ]; then
    echo Error: Starting service failed. Check output for details
    exit 1
fi

echo Fabric service started. Cluster should come up in a few minutes


if [ "true" = "$DeployForMesh" ]; then
  echo Deploying for mesh
  sleepcounter=0

  sfctl > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo Could not find sfctl. Please install sfctl before proceeding. Exiting..
    exit 1
  fi

  clusterconnect="sfctl cluster select --endpoint http://localhost:19080"
  
  $clusterconnect > /dev/null 2>&1
  while [ $? -ne 0 ]; do
    if [ "$sleepcounter" -gt 900 ]; then
      echo Could not connect to cluster. Exiting
      exit 1
    fi
    sleep 10
    sleepcounter=$((sleepcounter+10))
    echo Trying to connect to cluster
    $clusterconnect > /dev/null 2>&1
  done

  echo Cluster is up. Connection established

  ApplicationPackages="$FabricBinRoot/AddonServices/*"

  for ApplicationPackage in $ApplicationPackages; do
    if [[ -d "$ApplicationPackage"  ]]; then
      ApplicationManifest="$ApplicationPackage/ApplicationManifest.xml"
      
      if [ ! -f ${ApplicationManifest} ]; then
        echo "Could not find ApplicationManifest.xml for $ApplicationPackage. Exiting.."
        exit 1
      fi
     
      ApplicationTypeName=$(cat "$ApplicationManifest" | grep "ApplicationTypeName" | cut -d '"' -f2)
      ApplicationName=$(echo "$ApplicationTypeName" | sed 's/Type//')
      ApplicationVersion=$(cat "$ApplicationManifest" | grep "ApplicationTypeVersion" | cut -d '"' -f2)
      
      ListenPort=""
      InstanceCount="1"

      if [ "$ApplicationName" = "AzureFilesVolumePlugin" ]; then
        ListenPort="19100"
      elif [ "$ApplicationName" = "ServiceFabricVolumeDriver" ]; then
        ListenPort="19101"
      fi

      if [ -z "$ListenPort" ]; then
        echo "Could not set ListenPort. Exiting.."
        exit 1
      fi

      ApplicationParameters=$(cat <<EOF
        {"InstanceCount": "$InstanceCount", "ListenPort": "$ListenPort"}
EOF
      )

      echo Uploading ${ApplicationPackage}
      sleepcounter=0
      sfctlappupload="sfctl application upload --path $ApplicationPackage --show-progress"
      $sfctlappupload 2>/dev/null
      while [ $? -ne 0 ]; do
        if [ "$sleepcounter" -gt 900 ]; then
          echo Error: Could not upload package ${ApplicationPackage}. Check output for details
          exit 1
        fi
        sleep 10
        sleepcounter=$((sleepcounter+10))
        echo Trying again to upload ${ApplicationPackage}
        $sfctlappupload 2>/dev/null
      done
      
      echo Provisioning ${ApplicationName}
      sfctl application provision --application-type-build-path ${ApplicationName}
      if [ $? -ne 0 ]; then
        echo Error: Could not provision application ${ApplicationName}. Check output for details
        exit 1
      fi
      
      echo Creating ${ApplicationName} of ${ApplicationTypeName} version ${ApplicationVersion} with parameters ${ApplicationParameters}
      sfctl application create --app-name fabric:/${ApplicationName} --app-type ${ApplicationTypeName} --app-version ${ApplicationVersion} --parameters "$ApplicationParameters"
      if [ $? -ne 0 ]; then
        echo Error: Could not create application ${ApplicationName}. Check output for details
        exit 1
      fi

    fi
  done

  SFCTL_TEMP_DIR=/home/${SUDO_USER}/.sfctl
  if [ -d $SFCTL_TEMP_DIR ]; then
    rm -r /home/${SUDO_USER}/.sfctl
  fi

fi
