#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


PrintUsage()
{
    cat << EOF
Usage: ./build.sh [-c] [-d] [-p] [-n] [-createdeb] [-createinstaller] [-upgradetestdeb] [-upgradetestinstaller] [-skipbuild] [-s] [-st] [-release] [-debug] [-clang50] [-j<#>] [-all] [-v]
    -c: Clean build
    -d: Set project binary root to current working directory
    -p: Disable precompiled headers
    -n: Use ninja to build
    -createdeb -createinstaller: Create installer package
    -upgradetestdeb -upgradetestinstalller: Create multiple installer packages (for testing)
    -skipbuild: Skip the build phase (only generate)
    -s: Split symbols
    -st: Split symbols for test directory
    -release: Build release
    -debug: Build debug
    -clang50: Use clang 5.0
    -j<#>: Specify number of build threads
    -v: Verbose
    -all: Build third party libraries
EOF
}

TotalErrors=0

GetOs()
{
    local __resultvar=$1
    
    . /etc/os-release
    local linuxDistrib=$ID

    if [ $linuxDistrib = "ubuntu" ]; then 
        eval $__resultvar="DEBIAN"
    elif [ $linuxDistrib = "rhel" ] || [ $linuxDistrib = "centos" ]; then
        eval $__resultvar="REDHAT"
    else
        eval $__resultvar="UNKNOWN"
    fi
}

GetOs LINUX_DISTRIBUTION

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null
ProjRoot=${ScriptPath}/..
ProjBinRoot=${ProjRoot}
LibPath=${ProjRoot}/external/WinFab.Linux.Libs
CleanBuildDrop="-nc"
DisablePrecompile="-p"
SkipBuild="false"
CreateInstallerPackage="false"
CreateMultipleInstallerPackagesForTesting="false"
SplitSymbols="false"
SplitSymbolsForTestDir="false"
UseNinjaBuild="false"
CloudBuild="false"
BuildType="RelWithDebInfo"
ClangVersion="5.0"
NumProc=0
BuildThirdPartyLib="ON"
VERBOSE=0
CLEAN_DEPS=0
#WinFab.Linux.Libs library version
LinuxLibVersion="WinFab.Linux.Libs.2.2.7"
if [ $LINUX_DISTRIBUTION = "REDHAT" ]; then
    LinuxLibVersion="WinFab.Linux.Libs.2.2.2-centos"
fi
CoreCLRLibVersion="WinFab.CoreCLR.Libs.6.2.183"
SFXLibVersion="ServiceFabric.Explorer.2017.11.13.1"
KtlLibDebugVersion="WinFab.Ktl.Linux.debug.1.1.2.52"
KtlLibRetailVersion="WinFab.Ktl.Linux.retail.1.1.2.52"
SFUpgradeTestVersion="Microsoft.ServiceFabric.Upgrade.Test.Internal.6.0.0.22"

export CMAKE_NO_VERBOSE=1

# Install any required packages that docker image don't have yet
# After the docker image has this, this should be removed.
InstallPkgs()
{
    ${ProjRoot}/src/prod/tools/linux/init.sh ${LinuxLibVersion} ${CoreCLRLibVersion} ${SFXLibVersion} ${KtlLibDebugVersion} ${KtlLibRetailVersion} ${SFUpgradeTestVersion} ${CloudBuild}
    if [ $? != 0 ]; then
        return 1
    fi

    if [ -e ${LibPath}/Boost_1_61_0/lib/libc%2B%2B.so.1 ]
    then
       mv ${LibPath}/Boost_1_61_0/lib/libc%2B%2B.so.1 ${LibPath}/Boost_1_61_0/lib/libc++.so.1
       rm ${LibPath}/Boost_1_61_0/lib/libc%2*.so.1
    fi
    if [ -e ${LibPath}/Boost_1_61_0/lib/libc%25252B%25252B.so.1 ]
    then
       mv ${LibPath}/Boost_1_61_0/lib/libc%25252B%25252B.so.1 ${LibPath}/Boost_1_61_0/lib/libc++.so.1
       rm ${LibPath}/Boost_1_61_0/lib/libc%2*.so.1
    fi
}

InstallDeps()
{
    local InstallScript=${ProjRoot}/src/prod/tools/linux/third-party/install-deps.sh
    ${InstallScript}
    if [ $? != 0 ]; then
        exit 1
    fi
}

BuildLib()
{
    if [ ${BuildThirdPartyLib} != "ON" ]; then
        return 0
    fi
    #InstallDeps

    local bin_output=${ProjBinRoot}/build.prod/third-party
    local BuildScript=${ProjRoot}/src/prod/tools/linux/third-party/install-third-party.sh
    if [ ${CLEAN_DEPS} = 1 ]; then
        ${BuildScript} --uninstall
        ${BuildScript} -c
    fi
    if [ ${VERBOSE} = 1 ]; then
        ${BuildScript} -v -j ${NumProc}
    else
        ${BuildScript} -j ${NumProc}
    fi
    if [ $? != 0 ]; then
        exit 1
    fi
}

BuildDir()
{
    DirName=$1
    Clean=$2
    DisablePrecompile=$3
    
    echo
    echo Start to build $1 directory ...

    pushd .

    BuildSystem="make"

    if [ ${BuildThirdPartyLib} = "OFF" ]; then
        CC=/usr/lib/llvm-${ClangVersion}/bin/clang
        CXX=/usr/lib/llvm-${ClangVersion}/bin/clang++
    else
        CC=${ProjRoot}/deps/third-party/bin/clang/bin/clang
        CXX=${ProjRoot}/deps/third-party/bin/clang/bin/clang++
    fi

    if [ "-c" = ${Clean} ] || [ ! -d ${ProjBinRoot}/build.${DirName} ]; then
        rm -rf ${ProjBinRoot}/build.${DirName} 2> /dev/null
        mkdir ${ProjBinRoot}/build.${DirName}  2> /dev/null
        cd ${ProjBinRoot}/build.${DirName}
        
        CMakeGenerator=""
        if [ ${UseNinjaBuild} = "true" ]; then
            CMakeGenerator="-GNinja"
        fi
        
        DisablePrecompileFlag=""
        if [ "-p" = ${DisablePrecompile} ]; then
            echo -e "\e[0;32mPrecompilation is disabled as requested.\e[0m"
            DisablePrecompileFlag="-DSF_DISABLE_PRECOMPILE=ON"
        else
            echo -e "\e[0;32mPrecompilation is enabled.\e[0m"
        fi
        echo -e "\e[0;32mBuild Type: ${BuildType} \e[0m"
        cmake ${CMakeGenerator} \
              -DCMAKE_C_COMPILER=${CC} \
              -DCMAKE_CXX_COMPILER=${CXX} \
              -DCMAKE_BUILD_TYPE=${BuildType} \
              -DBUILD_THIRD_PARTY=${BuildThirdPartyLib} \
              ${DisablePrecompileFlag} ${ScriptPath}/$DirName
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../$1
            exit ${TotalErrors}
        fi
    fi
    
    if [ -f ${ProjBinRoot}/build.${DirName}/build.ninja ]; then
        BuildSystem="ninja"
    fi
    
    cd ${ProjBinRoot}/build.${DirName}

    if [ "false" = ${SkipBuild} ]; then
        if (( $NumProc <= 0 )); then
            NumProc=$(($(getconf _NPROCESSORS_ONLN)+0))
        fi
        echo -e "\e[0;32mParallel Build: ${NumProc} \e[0m"
        ${BuildSystem} -j $NumProc
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: make
            exit ${TotalErrors}
        fi

        ${BuildSystem} install
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: make install
            exit ${TotalErrors}       
        fi
    fi

    ${BuildSystem} fabricdrop
    if [ $? != 0 ]; then
        let TotalErrors+=1
        echo Error: cmd failed: make fabricdrop
        exit ${TotalErrors}       
    fi
    
    if [ "true" = ${SplitSymbols} ]; then
        ${BuildSystem} symsplit
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: make symsplit
            exit ${TotalErrors}       
        fi
    fi

    if [ "true" = ${SplitSymbolsForTestDir} ]; then
        ${BuildSystem} symsplitfortestdir
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: make symsplitfortestdir
            exit ${TotalErrors}
        fi
    fi

    if [ "true" = ${CreateInstallerPackage} ]; then
        if [ $LINUX_DISTRIBUTION = "DEBIAN" ]; then
            ${BuildSystem} runtimedeb
            if [ $? != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make runtimedeb
                exit ${TotalErrors}       
            fi
            
            ${BuildSystem} sdkdebcommon
            if [ $? != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make sdkdebcommon
                exit ${TotalErrors}       
            fi
            
            if [ "true" = ${CreateMultipleInstallerPackagesForTesting} ]; then
                ${BuildSystem} upgradetestdeb
                if [ $? != 0 ]; then
                    let TotalErrors+=1
                    echo Error: cmd failed: make upgradetestdeb
                    exit ${TotalErrors}       
                fi
            fi
        elif [ $LINUX_DISTRIBUTION = "REDHAT" ]; then
            ${BuildSystem} runtimerpm
            if [ $? != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make runtimerpm
                exit ${TotalErrors}       
            fi
            
            ${BuildSystem} sdkrpmcommon
            if [ $? != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make sdkrpmcommon
                exit ${TotalErrors}       
            fi
            
            if [ "true" = ${CreateMultipleInstallerPackagesForTesting} ]; then
                ${BuildSystem} upgradetestrpm
                if [ $? != 0 ]; then
                    let TotalErrors+=1
                    echo Error: cmd failed: make upgradetestrpm
                    exit ${TotalErrors}       
                fi
            fi
        else
            let TotalErrors+=1
            echo Error: unknown os
            exit ${TotalErrors}   
        fi

        popd  > /dev/null
    fi
}
startTime=`date +%s`
echo Start build at `date`.

echo "Distribution: $LINUX_DISTRIBUTION"
if [ $LINUX_DISTRIBUTION = "UNKNOWN" ]; then
    echo Distribution is not supported for build.
    exit 1
fi

while (( "$#" )); do
    if [ "$1" == "-c" ]; then
        CleanBuildDrop=$1
    elif [ "$1" == "-d" ]; then
        ProjBinRoot=$(pwd)
    elif [ "$1" == "-p" ]; then
        DisablePrecompile=$1
    elif [ "$1" == "-n" ]; then
        UseNinjaBuild="true"
    elif [ "$1" == "-createdeb" ] || [ "$1" == "-createinstaller" ]; then
        CreateInstallerPackage="true"
    elif [ "$1" == "-upgradetestdeb" ] || [ "$1" == "-upgradetestinstaller" ]; then
        CreateMultipleInstallerPackagesForTesting="true"
    elif [ "$1" == "-skipbuild" ]; then
        SkipBuild="true"
    elif [ "$1" == "-s" ]; then
        SplitSymbols="true"
    elif [ "$1" == "-st" ]; then
        SplitSymbolsForTestDir="true"
    #used for official builds only
    elif [ "$1" == "--cloudbuild" ]; then
        CloudBuild="true"
    elif [ "$1" == "-release" ]; then
        BuildType="RelWithDebInfo"
    elif [ "$1" == "-debug" ]; then
        BuildType="Debug"
    elif [ "$1" == "-clang50" ]; then
        ClangVersion="5.0-sf"
        DisablePrecompile=""
    elif [[ "$1" =~ ^-j.* ]]; then
        NumProcStr=${1:2}
        NumProc=$(($NumProcStr + 0))
    elif [ "$1" == "-all" ]; then
        BuildThirdPartyLib="ON"
    elif [ "$1" == "-v" ]; then
        VERBOSE=1
    elif [ "$1" == "--cleandeps" ]; then
	CLEAN_DEPS=1
    elif [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
        PrintUsage
        exit -1
    else
        echo "Unknown option $1"
        PrintUsage
        exit -2
    fi
    shift
done

if [ $CloudBuild == "true" ]; then
  echo "You have enabled the cloud build option. This option is for cloud build machines use not for dev machines."
  if [ -z $AZURE_CREDENTIALS ]; then
    echo "Environment variable AZURE_CREDENTIALS not found."
    exit 1
  fi
fi

mkdir -p ${ProjRoot}/external
InstallPkgs

if [ $? != 0 ]; then
    echo InstallPkgs has failed. Stopping build.
    exit 1
fi

BuildLib

BuildDir prod ${CleanBuildDrop} ${DisablePrecompile}
endTime=`date +%s`
timeTaken=$(($endTime-$startTime))
echo
echo Finished all builds with error = ${TotalErrors}, Time taken = ${timeTaken} seconds.

exit ${TotalErrors}
