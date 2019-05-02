#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


PrintUsage()
{
    cat << EOF
Usage: ./build.sh [-c] [-d] [-pe] [-pd] [-n] [-createdeb] [-createinstaller] [-ktlonly] [-upgradetestdeb] [-upgradetestinstaller] [-skipbuild] [-s] [-st] [-release] [-debug] [-clang50] [-j<#>] [-all] [-v] [-nocoreclrbuild] [-packagelocalcoreclrbuild]
    -c: Clean build
    -d: Set project binary root to current working directory
    -pd: Disable precompiled headers
    -pe: Enable precompiled headers
    -n: Use ninja to build
    -createdeb -createinstaller: Create installer package
    -ktlonly - only build prod/ktl
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
    -nocoreclrbuild: Does not build manage coreclr build
    -packagelocalcoreclrbuild: [Deprecated : Always true now] Package locally build manage coreclr binaries in installer
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
DisablePrecompile="-pd"
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
BuildThirdPartyLib="OFF"
VERBOSE=0
DoKtlOnlyBuild="OFF"
CLEAN_DEPS=0
BUILD_MANAGED=1
PACKAGE_LOCAL_CORECLR_BUILD="ON"
ArmCrossCompile="OFF"
ArmCrossCompileTriple="aarch64-linux-gnu"
SkipDepCheck="false"
# Ninja build binary has a different default name on CentOS based distros than Ubuntu
NinjaBuildBinaryName="ninja"

#WinFab.Linux.Libs library version
LinuxLibVersion=$(cat ${ProjRoot}/src/prod/linuxsetup/versions/linuxlibs.version)

if [ $LINUX_DISTRIBUTION = "REDHAT" ]; then
    LinuxLibVersion=$(cat ${ProjRoot}/src/prod/linuxsetup/versions/linuxlibscentos.version)
    NinjaBuildBinaryName="ninja-build"
fi
CoreCLRLibVersion=$(cat ${ProjRoot}/src/prod/linuxsetup/versions/coreclrlibs.version)
SFXLibVersion=$(cat ${ProjRoot}/src/prod/linuxsetup/versions/sfxlibs.version)
SFUpgradeTestVersion=$(cat ${ProjRoot}/src/prod/linuxsetup/versions/upgradetestlibs.version)
SFResgenVersion="ServiceFabric.ResGen.NetStandard.1.0.1"
CoreclrBuildArtifacts="ServiceFabric.Linux.Coreclr.BuildArtifacts.1.0.1"
MCGLinux="Microsoft.DotNet.Interop.1.0.0.7171701"
DataExtensions="Microsoft.ServiceFabric.Data.Extensions.1.4.4"
SFAspNetCoreInternal="SF.AspNetCore.Internal.3.3.1"
SFActorsServicesInternal="SF.ActorsServices.Internal.3.3.9"
SFArmCrossCompileSysRoot="SF.ArmCrossCompileSysRoot.Internal.1.0.0"
export CMAKE_NO_VERBOSE=1

# Install any required packages that docker image don't have yet
# After the docker image has this, this should be removed.
InstallPkgs()
{
    ${ProjRoot}/src/prod/tools/linux/init.sh ${LinuxLibVersion} ${CoreCLRLibVersion} ${SFXLibVersion} ${SFUpgradeTestVersion} ${SFResgenVersion} ${CoreclrBuildArtifacts} ${DataExtensions} ${MCGLinux} ${SFAspNetCoreInternal} ${SFActorsServicesInternal} ${SFLinuxLibssh2} ${SFArmCrossCompileSysRoot} ${CloudBuild} 
    if [ $? != 0 ]; then
        return 1
    fi

    if [ ! -L ${LibPath}/grpc/include/grpc++ ]; then
        ln ${LibPath}/grpc/include/grpcxx ${LibPath}/grpc/include/grpc++ -rs
        ln ${LibPath}/grpc/include/grpcxx/grpcxx.h ${LibPath}/grpc/include/grpcxx/grpc++.h -rs
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
        if [ "-pd" = ${DisablePrecompile} ]; then
            echo -e "\e[0;32mPrecompilation is disabled as requested.\e[0m"
            DisablePrecompileFlag="-DSF_DISABLE_PRECOMPILE=ON"
        else
            echo -e "\e[0;32mPrecompilation is enabled.\e[0m"
        fi
        echo -e "\e[0;32mBuild Type: ${BuildType} \e[0m"

        if [ "ON" = ${DoKtlOnlyBuild} ]; then
            echo -e "\e[0;32mKTL Only Build \e[0m"
        fi
        
        DARM_CMAKE_FLAG=""
        if [ "ON" = ${ArmCrossCompile} ]; then
            echo -e "\e[0;32mCross compile for AArch64.\e[0m"
            echo -e "\e[0;32mNote that cross compiling for ARM does NOT support packaging or building coreclr or third-party lib from source NOW \e[0m"
            if [ ! -f /usr/bin/${ArmCrossCompileTriple}-ld ]; then
                echo "Error: please install binutils-$ArmCrossCompileTriple for cross compile."
                exit -1
            fi
            SYS_ROOT_DIR="${ScriptPath}/../external/SF.ArmCrossCompileSysRoot.Internal"
            SYS_ROOT_TAR="${SYS_ROOT_DIR}/raspberrypi-rootfs.tar.gz"
            SYS_ROOT="${SYS_ROOT_DIR}/rootfs"
            mkdir -p ${SYS_ROOT} && tar -xvzf ${SYS_ROOT_TAR} -C ${SYS_ROOT_DIR}  > /dev/null;
            if [ $? != 0 ]; then
              echo -e '\e[01;31m tar failed. Package $SYS_ROOT_TAR may be incomplete or corrupted.  Try deleting package and trying again. \e[0m'
              exit -1
            fi 
            DARM_CMAKE_FLAG="DARM_CROSS_COMPILE=${ArmCrossCompile} \
                             -DCMAKE_TOOLCHAIN_FILE=${ScriptPath}/cross-compile.cmake \ 
                             -DARM_SYS_ROOT=${SYS_ROOT} \
                             -DARM_TARGET_TRIPLE=${ArmCrossCompileTriple}"          
        fi

        cmake ${CMakeGenerator} \
              -DCMAKE_C_COMPILER=${CC} \
              -DCMAKE_CXX_COMPILER=${CXX} \
              -DCMAKE_BUILD_TYPE=${BuildType} \
              -DBUILD_THIRD_PARTY=${BuildThirdPartyLib} \
              -DBUILD_KTL_ONLY=${DoKtlOnlyBuild} \
              -DPACKAGE_LOCAL_BUILD_MANAGED=${PACKAGE_LOCAL_CORECLR_BUILD} \
              -${DARM_CMAKE_FLAG} \
              ${DisablePrecompileFlag} ${ScriptPath}/$DirName
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../$1
            exit ${TotalErrors}
        fi
    fi

    if [ -f ${ProjBinRoot}/build.${DirName}/build.ninja ]; then
        BuildSystem=$NinjaBuildBinaryName
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

        if [ ${BUILD_MANAGED} == 1 ]; then
            ${BuildSystem} ManagedCoreClrBuild
            if [ $? != 0 ]; then
               let TotalErrors+=1
               echo Error: cmd failed: make ManagedCoreClrBuild
               exit ${TotalErrors}
            fi
        fi

        if [ "OFF" = ${DoKtlOnlyBuild} ]; then
            ${BuildSystem} install
            if [ $? != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make install
                exit ${TotalErrors}
            fi
        else
            echo Attempting to install -ktlonly
            pushd ktl/src >/dev/null
            ${BuildSystem} install
            installStatus=$?
            popd >/dev/null
            if [ ${installStatus} != 0 ]; then
                let TotalErrors+=1
                echo Error: cmd failed: make install -ktlonly
                exit ${TotalErrors}
            fi
        fi
    fi

    if [ "OFF" = ${DoKtlOnlyBuild} ]; then
        ${BuildSystem} fabricdrop
        if [ $? != 0 ]; then
            let TotalErrors+=1
            echo Error: cmd failed: make fabricdrop
            exit ${TotalErrors}
        fi
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
    elif [ "$1" == "-ktlonly" ]; then
        DoKtlOnlyBuild="ON"
    elif [ "$1" == "-d" ]; then
        ProjBinRoot=$(pwd)
    elif [ "$1" == "-pe" ] || [ "$1" == "-pd" ]; then
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
    elif [ "$1" == "-clang60" ]; then
        ClangVersion="6.0-sf"
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
    elif [ "$1" == "-nocoreclrbuild" ]; then
        BUILD_MANAGED=0
    elif [ "$1" == "-packagelocalcoreclrbuild" ]; then
        PACKAGE_LOCAL_CORECLR_BUILD="ON"
    elif [ "$1" == "-arm" ]; then
        ArmCrossCompile="ON"
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

if [ "ON" = ${ArmCrossCompile} ]; then
     BuildThirdPartyLib="OFF"
    if [ "ON" == ${BuildThirdPartyLib} ]; then
        echo Error: Cross compiling ARM build does NOT support packaging or building coreclr or third-party lib from source NOW.
        exit -1
    fi
else
    SFArmCrossCompileSysRoot=""
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
