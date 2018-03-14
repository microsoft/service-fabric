#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

_AclCurrentUser=0
_AclAllSFUser=0
_AclSFAdministrators=0

ShowHelp()
{
    echo "./FaclBlkDev.sh"
    echo "-o : ACL block device files for the current user."
    echo "-g : ACL block device files for ServiceFabricAdministrators group."
    echo "-a : ACL device file for ServiceFabricAllowedUsers group."
}

CheckError()
{
    if [ $1 != 0 ]; then
        echo "$2" 1>&2
        exit -1
    fi
}

ExecuteWithRetry()
{
    i=0
    until [ $i -gt 5 ]
    do
        echo "Executing $@"
        #Execute Command
        $@
        if [ $? == 0 ]; then
            return 0
        fi
        echo "Command failed.. Retrying after 1 sec"
        sleep 1
        i=expr $i +1
    done
    return -1
}

AclAllSFUsers()
{
    #Set permission.
    for i in `lsblk -l | cut -d' ' -f1 |grep -v NAME`
    do 
        setfacl -m "g:ServiceFabricAllowedUsers:rw-" /dev/${i}
    done
}

AclCurrentUser()
{
    if [ $SUDO_USER ]; then user=$SUDO_USER; else user=`whoami`; fi
    #Set permission.
    echo "Setting permission for $user"
    for i in `lsblk -l | cut -d' ' -f1 |grep -v NAME`
    do 
        setfacl -m "u:$user:rw-" /dev/${i}
    done
}

AclSFAdministrators()
{
    #Set permission.
    for i in `lsblk -l | cut -d' ' -f1 |grep -v NAME`
    do 
        setfacl -m "g:ServiceFabricAdministrators:rw-" /dev/${i}
    done
}

while getopts "hoaglius:" o; do
    case "${o}" in
        o)
            _AclCurrentUser=1
            ;;
        a)
            _AclAllSFUser=1
            ;;
        g)
            _AclSFAdministrators=1
            ;;
        *)
            ShowHelp
            exit 0 
            ;;
    esac
done

if [ $_AclCurrentUser -eq 1 ]; then
    AclCurrentUser
fi

if [ $_AclSFAdministrators -eq 1 ]; then
    AclSFAdministrators
fi

if [ $_AclAllSFUser -eq 1 ]; then
    AclAllSFUsers
fi