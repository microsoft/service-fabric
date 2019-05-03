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
        logger -s -tFabricUpgrade "Line read from target information file- ${targetInfoLine}"
        if [[ "$targetInfoLine" == *"CurrentInstallation"* ]]; then
            logger -s -tFabricUpgrade "Found CurrentInstallation line"
            Current_Version_From_Info_File=`echo $targetInfoLine | sed 's/.*TargetVersion=\"\([^\"]*\).*/\1/'`
            Current_Version_Package_Location=`echo $targetInfoLine | sed 's/.*MSILocation=\"\([^\"]*\).*/\1/'`
        elif [[ "$targetInfoLine" == *"TargetInstallation"* ]]; then
            logger -s -tFabricUpgrade "Found TargetInstallation line"
            Target_Version_From_Info_File=`echo $targetInfoLine | sed 's/.*TargetVersion=\"\([^\"]*\).*/\1/'`
            Target_Version_Package_Location=`echo $targetInfoLine | sed 's/.*MSILocation=\"\([^\"]*\).*/\1/'`
        fi
    done < "$PathToTargetInfoFile"

    logger -s -tFabricUpgrade "Current version from TargetInfo file ${Current_Version_From_Info_File}"
    logger -s -tFabricUpgrade "Current version package location from TargetInfo file ${Current_Version_Package_Location}"
    logger -s -tFabricUpgrade "Target version from TargetInfo file ${Target_Version_From_Info_File}"
    logger -s -tFabricUpgrade "Target version package location from TargetInfo file ${Target_Version_Package_Location}"
}

###########################################################################################################
# Function: disable_updater_and_exit - Disables the service and exits with success
###########################################################################################################
disable_updater_and_exit()
{
    logger -s -tFabricUpgrade "Disabling the service fabric updater service and exiting upgrade process"
    systemctl disable servicefabricupdater
    exit 0
}

###########################################################################################################
# Function: disable_servicefabric - Disables the service fabric runtime service
###########################################################################################################
disable_servicefabric()
{
    logger -s -tFabricUpgrade "Stopping service fabric service"
    systemctl stop servicefabric 2>&1

    logger -s -tFabricUpgrade "Disabling service fabric service"
    systemctl disable servicefabric 2>&1
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
# Function: remove_conflicting_packages - Removes conflicting packages for upgrade
###########################################################################################################
remove_conflicting_packages()
{
    local packagePath=$1

    if [ $DistroInstallerType == DISTRO_DEB ]; then
        packageInfo=$(dpkg --info $packagePath)
        error=$?
        echo "$packageInfo"
        if [ $error != 0 ]; then
            echo "Error ${error} getting package info from ${packagePath}"
            return $error
        fi

        echo "$packageInfo" | grep Depends:.*[[:space:]]docker-ce
        error=$?
        if [ $error == 0 ]; then
            logger -s -tFabricUpgrade "Backward compatibility scenario; Depends contains docker-ce. Will be removing moby-engine & moby-ci if present."

            mobyEngineInstallation=($(dpkg -l moby-engine | grep moby-engine)[0])
            mobyEngineInstallationPresent=$?

            mobyCliInstallation=($(dpkg -l moby-cli | grep moby-cli)[0])
            mobyCliInstallationPresent=$?

            if [ $mobyEngineInstallationPresent == 0 ] || [ $mobyCliInstallationPresent == 0 ]; then
                disable_servicefabric

                if [ $mobyEngineInstallationPresent == 0 ]; then
                    logger -s -tFabricUpgrade "Removing moby-engine. Current installation status ${mobyEngineInstallation}"
                    apt -o Debug::pkgProblemResolver=yes purge moby-engine -fy
                    error=$?
                    if [ $error != 0 ]; then
                        dpkg --configure -a
                        apt -o Debug::pkgProblemResolver=yes purge moby-engine -fy
                    fi
                fi

                if [ $mobyCliInstallationPresent == 0 ]; then
                    logger -s -tFabricUpgrade "Removing moby-cli. Current installation status ${mobyCliInstallationPresent}"
                    apt -o Debug::pkgProblemResolver=yes purge moby-cli -fy
                    error=$?
                    if [ $error != 0 ]; then
                        dpkg --configure -a
                        apt -o Debug::pkgProblemResolver=yes purge moby-cli -fy
                    fi
                fi

                apt autoremove -y
            fi
        fi
    elif [ $DistroInstallerType == DISTRO_RPM ]; then
        local conflictingPackages=( $(rpm -qp --conflicts ${packagePath}) )

        logger -s -tFabricUpgrade "Removing conflicting packages"

        for conflictingPackageName in "${conflictingPackages[@]}"
        do
            if [ $conflictingPackageName = "" ]; then
                break
            fi
            
            rpm -q $conflictingPackageName >/dev/null 2>&1
            if [ $? -ne 0 ]; then
                # package not installed
                continue
            fi

            # remove package
            logger -s -tFabricUpgrade "Removing conflicting package: ${conflictingPackageName}"
            output=$(rpm -e $conflictingPackageName 2>&1)
            err=$?
            if [ $err -ne 0 ]; then
                logger -s -tFabricUpgrade "Command \"rpm -e ${conflictingPackageName} failed\" with ${err}"
                logger -s -tFabricUpgrade "Output: ${output}"
                return 1
            fi
        done
    else
        logger -s -tFabricUpgrade "Error: remove_conflicting_packages not implemented for ${DistroInstallerType}"
        return 1
    fi

    logger -s -tFabricUpgrade "Done removing conflicting packages"
    return 0
}

###########################################################################################################
# Function: check_package_rollback_status_and_exit - Confirms package correctly rolled back and if not exits with 1
###########################################################################################################
check_package_rollback_status_and_exit()
{
    set_installation_status_and_version
    if [ "$Installation_Status" != "installed" ]; then
        logger -s -tFabricUpgrade "ServiceFabric rollback of package failed. Will exit to restart state machine"
        exit $DEPLOY_ROLLBACK
    elif [ "$Installed_Version" != "$Current_Version_From_Info_File" ]; then
        logger -s -tFabricUpgrade "Did not successfully rollback to old version. Will exit to restart machine"
        exit $INSTALL_TARGET_PACKAGE
    fi
}

###########################################################################################################
# Function: is_reboot_pending - Checks if a reboot is currently pending; used to prevent dpkg/rpm db corruption 
###########################################################################################################
is_reboot_pending()
{
    if [ -f /run/nologin ]; then
        return 1
    fi

    return 0
}

###########################################################################################################
# Function: exit_if_reboot_pending - Used if a reboot is pending in intermediate states to terminate the script and avoid prematurely advancing to next state
###########################################################################################################
exit_if_reboot_pending()
{
  local exitCode=$1
  is_reboot_pending
  if [ $? -ne 0 ]; then
        logger -s -tFabricUpgrade "Reboot pending. Exit code: ${exitCode}."
        exit $exitCode
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
        logger -s -tFabricUpgrade "Invalid upgrade path"
        return
    fi

    if [ $versionCompare -eq 255 ]; then
        logger -s -tFabricUpgrade "One or both of the versions was in an unexpected format"
        return
    fi

    if [ $DistroInstallerType == DISTRO_DEB ]; then
        exit_if_reboot_pending $NextState
        logger -s -tFabricUpgrade "Installing debian file at ${packagePath}"
        apt-get update
        
        exit_if_reboot_pending $NextState
        remove_conflicting_packages ${packagePath}
        if [ $? -ne 0 ]; then
            logger -s -tFabricUpgrade "remove_conflicting_packages failed"
            exit 1
        fi

        exit_if_reboot_pending $NextState
        dpkg -i ${packagePath}
        
        exit_if_reboot_pending $NextState
        apt-get update
        
        exit_if_reboot_pending $NextState
        apt -o Debug::pkgProblemResolver=yes install -fy
        error=$?

        apt clean -y

    elif [ $DistroInstallerType == DISTRO_RPM ]; then
        if [ $versionCompare -eq 0 ]; then
            logger -s -tFabricUpgrade "Versions are the same, not taking any action"
            return
        elif [ $versionCompare -ne -1 ] && [ $versionCompare -ne 1 ]; then
            logger -s -tFabricUpgrade "compare_versions had invalid result ${versionCompare}"
            exit 1
        fi

        exit_if_reboot_pending $NextState
        remove_conflicting_packages ${packagePath}
        if [ $? -ne 0 ]; then
            logger -s -tFabricUpgrade "remove_conflicting_packages failed"
            exit 1
        fi
        
        exit_if_reboot_pending $NextState
        pendingYumTransactions=$(find /var/lib/yum -maxdepth 1 -type f -name 'transaction-all*' -not -name '*disabled' -printf . | wc -c)
        if [ $pendingYumTransactions -ne 0 ]; then
            logger -s -tFabricUpgrade "Running pending yum transactions. Count: ${pendingYumTransactions}"
            yum-complete-transaction
            error = $?
            if [ $error != 0 ]; then
                logger -s -tFabricUpgrade "Completing yum transactions exited with $error. Proceeding anyway."
            fi
        else
            logger -s -tFabricUpgrade "No pending yum transactions."
        fi

        exit_if_reboot_pending $NextState
        if [ $versionCompare -eq -1 ]; then # first version smaller
            logger -s -tFabricUpgrade "Upgrading rpm file at ${packagePath}"
            yum upgrade -y ${packagePath}
        elif [ $versionCompare -eq 1 ]; then
            logger -s -tFabricUpgrade "Downgrading rpm file at ${packagePath}"
            yum downgrade -y ${packagePath}
        fi
        error = $?
    fi

    if [ $error != 0 ]; then
        logger -s -tFabricUpgrade "Error: Package installation failed. Error: ${error}"
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
            logger -s -tFabricUpgrade "Did not find service fabric installation."
            Installation_Status=""
            Installed_Version=""
        else
            Installation_Status=${SFDebStatusTokens[0]}
            if [ $Installation_Status = "ii" ]; then
                Installation_Status="installed"
            fi
            Installed_Version=${SFDebStatusTokens[2]}
            logger -s -tFabricUpgrade "Current Service Fabric debian status ${Installed_Version} and ${Installation_Status}"
        fi
    elif [ $DistroInstallerType == DISTRO_RPM ]; then
        # Get status of installation
        SFRpmStatusString="$(rpm -qi servicefabric | grep -w Version)"
        SFRpmStatusTokens=( $SFRpmStatusString )

        # Rollback if the installed package has any issues
        if [ "${SFRpmStatusTokens[0]}" != "Version" ]; then
            logger -s -tFabricUpgrade "Did not find service fabric installation."
            Installation_Status=""
            Installed_Version=""
        else
            Installation_Status="installed"
            Installed_Version=${SFRpmStatusTokens[2]}
            logger -s -tFabricUpgrade "Current Service Fabric rpm status ${Installed_Version} and ${Installation_Status}"
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
   logger -s -tFabricUpgrade "Script doupgrade.sh must be run as root"
   exit 1
fi

if [ "$#" -ne 1 ]; then
    logger -s -tFabricUpgrade "Illegal number of arguments passed in to doupgrade script"
    exit 1
fi

get_distro_installer DistroInstallerType
if [ $DistroInstallerType == DISTRO_UNKNOWN ]; then
    logger -s -tFabricUpgrade "Distro unknown for upgrade"
    exit 1
fi

PathToTargetInfoFile=$1

NextState=READ_TARGET_INFO

while :; do
    while :; do 
        is_reboot_pending
        if [ $? -ne 0 ]; then
            logger -s -tFabricUpgrade "Halting upgrade process due to pending reboot to avoid package DB corruption."
            sleep 60
        else
            break;
        fi
    done

    if [ $NextState == READ_TARGET_INFO ]; then
        read_target_info_file
        if [ "$Current_Version_From_Info_File" == "" ]; then
            logger -s -tFabricUpgrade "Current Version in TargetInformation file not found. Upgrade cannot be performed"
            NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
        elif [ "$Target_Version_From_Info_File" == "" ]; then
            logger -s -tFabricUpgrade "Target version in TargetInformation file not found. Upgrade cannot be performed"
            NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
        elif [ "$Current_Version_From_Info_File" == "$Target_Version_From_Info_File" ]; then
            logger -s -tFabricUpgrade "TargetInformation file Current and Target versions are the same. Invalid configuration; exiting."
            NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
        else
            logger -s -tFabricUpgrade "Target versions set. Will move on to installation step"
            NextState=CHECK_CURRENT_VERSION
        fi  
    elif [ $NextState == CHECK_CURRENT_VERSION ]; then
        set_installation_status_and_version
        if [ "$Installation_Status" != "installed" ]; then
            logger -s -tFabricUpgrade "ServiceFabric not fully installed. Will rollback"
            NextState=ROLLBACK_PACKAGE
        elif [ "$Installed_Version" == "$Current_Version_From_Info_File" ]; then
            logger -s -tFabricUpgrade "Current version installed. Will move to install target package"
            NextState=INSTALL_TARGET_PACKAGE
        elif [ "$Installed_Version" == "$Target_Version_From_Info_File" ]; then
            logger -s -tFabricUpgrade "Target package version achieved. Will move to Deployment step"
            NextState=DEPLOY_FABRIC
        else
            logger -s -tFabricUpgrade "Current version is unexpected. Will rollback to last known good version"
            NextState=ROLLBACK_PACKAGE
        fi
    elif [ $NextState == INSTALL_TARGET_PACKAGE ]; then
        logger -s -tFabricUpgrade "Installing target version of the package"
        upgrade_package "install" $Target_Version_Package_Location
        NextState=CHECK_CURRENT_VERSION
    elif [ $NextState == ROLLBACK_PACKAGE ]; then
        logger -s -tFabricUpgrade "Rolling back to current version of the package"
        upgrade_package "rollback" $Current_Version_Package_Location
        check_package_rollback_status_and_exit
        NextState=DEPLOY_ROLLBACK
    elif [ $NextState == DEPLOY_FABRIC ]; then
        pushd .
        cd /opt/microsoft/servicefabric/bin/Fabric/Fabric.Code
        ./FabricDeployer.sh
        if [ "$?" -ne "0" ]; then
            logger -s -tFabricUpgrade "FabricDeployer.sh failed. Will do a rollback"
            NextState=ROLLBACK_PACKAGE
        else
            logger -s -tFabricUpgrade "Deployment succeeded. Will move on to starting SF"
            NextState=START_SF_SERVICE  
        fi
        popd
    elif [ $NextState == DEPLOY_ROLLBACK ]; then
        pushd . 
        cd /opt/microsoft/servicefabric/bin/Fabric/Fabric.Code
        ./FabricDeployer.sh /operation:Rollback
        if [ "$?" -ne "0" ]; then
            popd
            logger -s -tFabricUpgrade "FabricDeployer rollback failed. Will exit to restart state machine"
            exit $DEPLOY_FABRIC
        else
            logger -s -tFabricUpgrade "Deployment rollback succeeded. Will move on to starting SF"
            NextState=START_SF_SERVICE  
        fi
        popd
    elif [ $NextState == START_SF_SERVICE ]; then
        logger -s -tFabricUpgrade "Enable service fabric service"
        systemctl enable servicefabric
        if [ $? != 0 ]; then
            logger -s -tFabricUpgrade "Error: Failed to enable servicefabric daemon. Will exit to restart state machine"
            exit $DEPLOY_ROLLBACK
        fi
        
        logger -s -tFabricUpgrade "Start Service Fabric Service"
        systemctl start servicefabric
        if [ $? != 0 ]; then
            logger -s -tFabricUpgrade "Error: Failed to start service fabric service. Will exit to restart state machine"
            exit $START_SF_SERVICE
        fi
        NextState=DISABLE_UPDATER_SERVICE_AND_EXIT
    elif [ $NextState == DISABLE_UPDATER_SERVICE_AND_EXIT ]; then
        disable_updater_and_exit
    fi
done

logger -s -tFabricUpgrade "Unreachable code"
exit 8