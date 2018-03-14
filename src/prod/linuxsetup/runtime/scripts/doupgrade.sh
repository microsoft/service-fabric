#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


######################
# State machine states
######################
READ_TARGET_INFO=0
DISABLE_UPDATER_SERVICE_AND_EXIT=1
CHECK_CURRENT_VERSION=2
ROLLBACK_PACKAGE=3
INSTALL_TARGET_PACKAGE=4
DEPLOY_FABRIC=5
DEPLOY_ROLLBACK=6
START_SF_SERVICE=7

######################
# Distro types
######################
DISTRO_DEB="DEB"
DISTRO_RPM="RPM"
DISTRO_UNKNOWN="UNKNOWN"

######################
# Global Variables
######################
Current_Version_From_Info_File=""
Current_Version_Package_Location=""
Target_Version_From_Info_File=""
Target_Version_Package_Location=""
Installed_Version=""
Installation_Status=""

###########################################################################################################
# Function: read_target_info_file - Reads the Target info file and assigns expected versions and package file 
# locations. Sets to empty if file is not found
###########################################################################################################
read_target_info_file()
{
    while read -r line
    do
        targetInfoLine="$line"
        echo Line read from target information file- $targetInfoLine
        if [[ "$targetInfoLine" == *"CurrentInstallation"* ]]; then
            echo Found CurrentInstallation line
        	Current_Version_From_Info_File=`echo $targetInfoLine | sed 's/.*TargetVersion=\"\([^\"]*\).*/\1/'`
        	Current_Version_Package_Location=`echo $targetInfoLine | sed 's/.*MSILocation=\"\([^\"]*\).*/\1/'`
        elif [[ "$targetInfoLine" == *"TargetInstallation"* ]]; then
        	echo Found TargetInstallation line
        	Target_Version_From_Info_File=`echo $targetInfoLine | sed 's/.*TargetVersion=\"\([^\"]*\).*/\1/'`
        	Target_Version_Package_Location=`echo $targetInfoLine | sed 's/.*MSILocation=\"\([^\"]*\).*/\1/'`
        fi
    done < "$PathToTargetInfoFile"

    echo Current version from TargetInfo file $Current_Version_From_Info_File
    echo Current version package location from TargetInfo file $Current_Version_Package_Location
    echo Target version from TargetInfo file $Target_Version_From_Info_File
    echo Target version package location from TargetInfo file $Target_Version_Package_Location
}

###########################################################################################################
# Function: disable_updater_and_exit - Disables the service and exits with success
###########################################################################################################
disable_updater_and_exit()
{
    echo Disabling the service fabric updater service and exiting upgrade process
    systemctl disable servicefabricupdater
    exit 0
}

###########################################################################################################
# Function: compare_versions - Compares Fabric versions
###########################################################################################################
compare_versions()
{
    local firstVersion=${1}
    local secondVersion=${2}

    IFS='.' read -ra firstVersion <<< "$firstVersion"
    IFS='.' read -ra secondVersion <<< "$secondVersion"

    if [ ${#firstVersion[@]} -ne 4 ] || [ ${#secondVersion[@]} -ne 4 ]; then
        echo 255
    fi

    for i in {0..3}
    do
        local n1=${firstVersion[i]}
        if [[ ! $n1 =~ ^[0-9]+$ ]]; then
            echo 255
            return
        fi

        local n2=${secondVersion[i]}
        if [[ ! $n1 =~ ^[0-9]+$ ]]; then
            echo 255
            return
        fi

        if [ $n1 -lt $n2 ]; then
            echo -1
            return
        elif [ $n1 -gt $n2 ]; then
            echo 1
            return
        fi
    done

    echo 0
}

###########################################################################################################
# Function: check_package_rollback_status_and_exit - Confirms package correctly rolled back and if not exits with 1
###########################################################################################################
check_package_rollback_status_and_exit()
{
    set_installation_status_and_version
    if [ "$Installation_Status" != "installed" ]; then
        echo ServiceFabric rollback of package failed. Will exit to restart state machine
	    exit 3
    elif [ "$Installed_Version" != "$Current_Version_From_Info_File" ]; then
    	echo Did not successfully rollback to old version. Will exit to restart machine
    	exit 4
    fi
}

###########################################################################################################
# Function: upgrade_package - Installs the specified version
###########################################################################################################
upgrade_package()
{
    local operation=$1
    local packagePath=$2

    local versionCompare
    if [ $operation = "install" ]; then
        versionCompare=$(compare_versions $Current_Version_From_Info_File $Target_Version_From_Info_File)
    elif [ $operation = "rollback" ]; then
        versionCompare=$(compare_versions $Target_Version_From_Info_File $Current_Version_From_Info_File)
    else
        echo "Invalid upgrade path"
        return
    fi

    if [ $versionCompare -eq 255 ]; then
        echo "One or both of the versions was in an unexpected format"
        return
    fi

    if [ $DistroInstallerType == DISTRO_DEB ]; then
        echo "Installing debian file at ${packagePath}"
        apt-get update
        dpkg -i ${packagePath}
        apt-get install -fy
    elif [ $DistroInstallerType == DISTRO_RPM ]; then
        if [ $versionCompare -eq -1 ]; then # first version smaller
            echo "Upgrading rpm file at ${packagePath}"
            yum upgrade -y ${packagePath}
        elif [ $versionCompare -eq 1 ]; then
            echo Downgrading rpm file at ${packagePath}
            yum downgrade -y ${packagePath}
        elif [ $versionCompare -eq 0 ]; then
            echo "Versions are the same, not taking any action"
        else
            echo "compare_versions had invalid result ${versionCompare}"
            exit 1
        fi
    fi

    if [ $? != 0 ]; then
        echo Error: Package resolution failed.        
    fi
}

###########################################################################################################
# Function: set_installation_status_and_version - Reads the curent installed version for SF from environment
# and sets the variables. If no installation found sets version and status to empty
###########################################################################################################
set_installation_status_and_version()
{
    if [ $DistroInstallerType == DISTRO_DEB ]; then
        # Get status of installation
        SFDebStatusString="$(dpkg -l | grep -w servicefabric)"
        SFDebStatusTokens=( $SFDebStatusString )

        # Rollback if the installed package has any issues
        if [ "${SFDebStatusTokens[1]}" != "servicefabric" ]; then
            echo Did not find service fabric installation.
            Installation_Status=""
            Installed_Version=""
        else
            Installation_Status=${SFDebStatusTokens[0]}
            if [ $Installation_Status = "ii" ]; then
                Installation_Status="installed"
            fi
            Installed_Version=${SFDebStatusTokens[2]}
            echo Current Service Fabric debian status $Installed_Version and $Installation_Status
        fi
    elif [ $DistroInstallerType == DISTRO_RPM ]; then
        # Get status of installation
        SFRpmStatusString="$(rpm -qi servicefabric | grep -w Version)"
        SFRpmStatusTokens=( $SFRpmStatusString )

        # Rollback if the installed package has any issues
        if [ "${SFRpmStatusTokens[0]}" != "Version" ]; then
            echo Did not find service fabric installation.
            Installation_Status=""
            Installed_Version=""
        else
            Installation_Status="installed"
            Installed_Version=${SFRpmStatusTokens[2]}
            echo Current Service Fabric rpm status $Installed_Version and $Installation_Status
        fi
    fi
}

###########################################################################################################
# Function: get_distro_installer - Determines appropriate installer type based on OS
# and sets the input variable as distro type.
###########################################################################################################
get_distro_installer()
{
    local __resultvar=$1

    /usr/bin/dpkg --search /usr/bin/dpkg >/dev/null 2>&1
    if [ $? = 0 ]; then
        eval $__resultvar=DISTRO_DEB
    else
        /usr/bin/rpm -q -f /usr/bin/rpm >/dev/null 2>&1
        if [ $? = 0 ]; then
            eval $__resultvar=DISTRO_RPM
        else
            eval $__resultvar=DISTRO_UNKNOWN
        fi
    fi
}

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Illegal number of arguments passed in to doupgrade script"
    exit 2
fi

get_distro_installer DistroInstallerType
if [ $DistroInstallerType == DISTRO_UNKNOWN ]; then
    echo "Distro unknown for upgrade"
    exit 1
fi

PathToTargetInfoFile=$1

NextState=READ_TARGET_INFO

while :; do
    if [ $NextState == READ_TARGET_INFO ]; then
        read_target_info_file
	    if [ "$Current_Version_From_Info_File" == "" ]; then
            echo Current Version in TargetInformation file not found. Upgrade cannot be performed
	        NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
	    elif [ "$Target_Version_From_Info_File" == "" ]; then
            echo Target version in TargetInformation file not found. Upgrade cannot be performed
	        NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
	    else
            echo Target versions set. Will move on to installation step
	        NextState=CHECK_CURRENT_VERSION
        fi	
    elif [ $NextState == CHECK_CURRENT_VERSION ]; then
        set_installation_status_and_version
	    if [ "$Installation_Status" != "installed" ]; then
            echo ServiceFabric not fully installed. Will rollback
	        NextState=ROLLBACK_PACKAGE
	    elif [ "$Installed_Version" == "$Current_Version_From_Info_File" ]; then
            echo Current version installed. Will move to install target package
	        NextState=INSTALL_TARGET_PACKAGE
	    elif [ "$Installed_Version" == "$Target_Version_From_Info_File" ]; then
            echo Target package version achieved. Will move to Deployment step
	        NextState=DEPLOY_FABRIC
	    else
            echo Current version is unexpected. Will rollback to last known good version
	        NextState=ROLLBACK_PACKAGE
        fi
    elif [ $NextState == INSTALL_TARGET_PACKAGE ]; then
	    echo Installing target version of the package
	    upgrade_package "install" $Target_Version_Package_Location
        NextState=CHECK_CURRENT_VERSION
    elif [ $NextState == ROLLBACK_PACKAGE ]; then
	    echo Rolling back to current version of the package
	    upgrade_package "rollback" $Current_Version_Package_Location
	    check_package_rollback_status_and_exit
	    NextState=DEPLOY_ROLLBACK
    elif [ $NextState == DEPLOY_FABRIC ]; then
	    pushd .
        cd /opt/microsoft/servicefabric/bin/Fabric/Fabric.Code
        ./FabricDeployer.sh
        if [ "$?" -ne "0" ]; then
            echo FabricDeployer.sh failed. Will do a rollback
	        NextState=ROLLBACK_PACKAGE
        else
            echo Deployment succeeded. Will move on to starting SF
            NextState=START_SF_SERVICE	
        fi
        popd
    elif [ $NextState == DEPLOY_ROLLBACK ]; then
	    pushd . 
        cd /opt/microsoft/servicefabric/bin/Fabric/Fabric.Code
        ./FabricDeployer.sh /operation:Rollback
	    if [ "$?" -ne "0" ]; then
            popd
            echo FabricDeployer rollback failed. Will exit to restart state machine
	        exit 5
        else
            echo Deployment rollback succeeded. Will move on to starting SF
            NextState=START_SF_SERVICE	
        fi
        popd
    elif [ $NextState == START_SF_SERVICE ]; then
	    echo Enable service fabric service
        systemctl enable servicefabric
        if [ $? != 0 ]; then
            echo Error: Failed to enable servicefabric daemon. Will exit to restart state machine
            exit 6
        fi
		
		echo Start Service Fabric Service
 	    systemctl start servicefabric
        if [ $? != 0 ]; then
            echo Error: Failed to start service fabric service. Will exit to restart state machine
            exit 7
        fi
	    NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
    elif [ $NextState == DISABLE_UPDATER_SERVICE_AND_EXIT ]; then
        disable_updater_and_exit
    fi
done

echo Unreachable code
exit 8