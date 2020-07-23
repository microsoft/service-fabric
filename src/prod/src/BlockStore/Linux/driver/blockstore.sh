#!/bin/bash

# Not supported on Redhat
if [[ -f /etc/os-release ]]; then 
  . /etc/os-release
  linuxDistrib=$ID
  if [ $linuxDistrib = "rhel" ]; then
    echo "Service Fabric doesn't support BlockStore on RHEL yet."
    exit 1
  fi
fi

BlockStoreFeatureEnableFile="/etc/servicefabric/IsSFVolumeDiskServiceEnabled"

# Check if BlockStore is enabled on cluster node
if [[ -f $BlockStoreFeatureEnableFile ]]; then
  if ! grep -iFxq "1" $BlockStoreFeatureEnableFile; then
    echo "Service Fabric BlockStore disabled."
    exit 1
  fi
else
  echo "Service Fabric BlockStore disabled."
  exit 1
fi

ModuleDirPath="../kxm"
ModuleName="sfblkdevice"
DeviceFileName="/dev/sfblkdevctrl"
_UnloadModule=0
_LoadModule=0

ShowHelp()
{
    echo "./blockstore.sh"
    echo "-l : load blockstore driver"
    echo "-u : unload blockstore driver."
    echo "-s <blockstore_driver_path> : blockstore driver source location."
}

CheckError()
{
    if [ $1 != 0 ]; then
        echo "$2" 1>&2
        exit -1
    fi
}

BuildModule()
{
    #Go to Blockstore directory
    pushd $ModuleDirPath
    ExitIfError $?  "Error@$LINENO: Blockstore source directory not found."

    #Clean
    make clean 1>/dev/null 

    #Build module.
    make 1>/dev/null

    ExitIfError $?  "Error@$LINENO: Could not build BlockStore module using make."

    #Leave Blockstore directory
    popd
}

AclAllSFUsers()
{
    #Set permission.
    echo "Setting permission for ServiceFabricAllowedUsers group to access blockstore control device."
    setfacl -m "g:ServiceFabricAllowedUsers:rw-" $DeviceFileName
}

AclCurrentUser()
{
    if [ $SUDO_USER ]; then user=$SUDO_USER; else user=`whoami`; fi
    #Set permission.
    echo "Setting permission for $user user to access blockstore control device."
    setfacl -m "u:$user:rw-" $DeviceFileName
}

LoadModule()
{
    #Enter into Blockstore source directory.
    pushd $ModuleDirPath
    if [ $? != 0 ]; then 
        echo "Line: $LINENO Blockstore source directory not found."
        return -1 
    fi

    #Clean
    make clean 1>/dev/null 

    #Build module.
    make 1>/dev/null
    if [ $? != 0 ]; then
        popd
        return -1
    fi 

    #Load module.
    insmod $ModuleName".ko"
    if [ $? != 0 ]; then
        popd
        return -1
    fi
 
    popd

    #Acl file for all servicefabric allowed other users.
    AclAllSFUsers

    #Acl current user.
    AclCurrentUser

    return 0
}

UnloadModule()
{
    ispresent=`awk '$1=="sfblkdevice"' /proc/modules`
    if [[ $ispresent ]]; then
        rmmod -f $ModuleName 2>/dev/null
        if [ $? != 0 ]; then
            return -1
        fi
    fi
    return 0
}

while getopts "lus:" o; do
    case "${o}" in
        l)
            _LoadModule=1
            ;;
        u)
            _UnloadModule=1
            ;;
        s)
            ModuleDirPath=${OPTARG}
            ;;
        *)
            ShowHelp
            exit 0 
            ;;
    esac
done


if [ $_UnloadModule -eq 1 ]; then
    UnloadModule
    CheckError $? "Coudln't unload blockstore driver."
fi

if [ $_LoadModule -eq 1 ]; then
    LoadModule    
    CheckError $? "Coudln't load blockstore driver."
fi

exit 0
