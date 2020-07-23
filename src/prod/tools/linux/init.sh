#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

# Replace "PkgName" and "PkgVer"
# NugetRepo="https://msazure.pkgs.visualstudio.com/_packaging/ManualMirror/nuget/v2?id=PkgName&version=PkgVer"

PrebuiltBlobStore="https://sfprebuilt.blob.core.windows.net/binaries"

PkgsList=()

paramNum=1
for param in "$@"
do
    if [ $paramNum -lt $# ]; then
        PkgsList+=($param)
    else
        IsJenkins=$param
    fi
    paramNum=$((paramNum+1))
done

pushd `dirname $0` > /dev/null
ScriptPath=`pwd -P`
popd > /dev/null

ExtLibPath=${ScriptPath}/../../../../external

SFPkgCache="/tmp/SFPkgCache"
if [ ! -d "$SFPkgCache" ]; then
  mkdir "$SFPkgCache";
fi;

ParsePkgNameVersionSuf(){
    local pkgName
    local pkgVer
    local pkgSuf
    IFS='.|-' read -ra tokens <<< "$1"
    for tok in "${tokens[@]}"
    do
       if [[ "$tok" = "nupkg" ]]; then
           break
       fi
       if [[ "$tok" =~ ^[^0-9]+$ ]]; then
           if [ ! -z "$pkgVer" ]; then
               pkgSuf=$tok
           elif [ ! -z "$pkgName" ]; then
               pkgName="${pkgName}.${tok}"
           else
               pkgName=$tok
           fi
       else
           if [ ! -z "$pkgVer" ]; then
               pkgVer="${pkgVer}.${tok}"
           else
               pkgVer=$tok
           fi
       fi
    done
    echo "$pkgName,$pkgVer,$pkgSuf"
}

DownloadPkg() {
    local pkgName=$1
    local pkgVer=$2
    local pkgSuf=$3
    local pkgVerSuf=${pkgVer}
    if [ ! -z "$pkgSuf" ]; then
        pkgVerSuf=${pkgVerSuf}-${pkgSuf}
    fi
    local pkgPath=/tmp/${pkgName}.${pkgVerSuf}.nupkg

    if [ -f $pkgPath ]; then
        echo "prebuilt package already downloaded: ${pkgPath}"
        return
    fi

    shopt -u nullglob dotglob #Disable nullglob dotglob

    local url=${PrebuiltBlobStore}
    url="${url}/${pkgName}.${pkgVerSuf}.nupkg"
    wget $url -O $pkgPath

    if [ $? != 0 ]; then
        echo "Failed to download ${pkgName}. Aborting."
        exit 1
    fi
}

ExtractPkg() {
    local forceInstall=$1
    local pkgName=$2
    local pkgVer=$3
    local pkgSuf=$4
    local pkgFullName=${pkgName}.${pkgVer}
    if [ ! -z "$pkgSuf" ]; then
        pkgFullName=${pkgFullName}-${pkgSuf}
    fi
    local pkgPath="/tmp/${pkgFullName}.nupkg"
    local pkgDestPath=${ExtLibPath}/${pkgFullName}
    local pkgPathlink=${ExtLibPath}/${pkgName}

    if [ "$forceInstall" = "true" ] || [ ! -d $pkgDestPath ]; then
        echo "Extracting ${pkgPath}..."
        local pkgCachePath=$SFPkgCache/$pkgFullName
        shopt -s nullglob dotglob
        local cacheFiles=($pkgCachePath/*)
        if [ ! -d $pkgCachePath ] || [ ${#cacheFiles[@]} -eq 0 ]; then
          mkdir -p $pkgCachePath;
          unzip $pkgPath -d $pkgCachePath > /dev/null;
          if [ $? != 0 ]; then
              echo "unzip failed. Package $pkgPath may be incomplete or corrupted.  Try deleting package and trying again."
              cacheFiles=($pkgCachePath/*)
              if [ ${#cacheFiles[@]} -eq 0 ]; then
                  rm -rf $pkgCachePath
              fi
              exit 1
          fi
        fi;

        rm -rf $pkgDestPath
        mkdir $pkgDestPath;

        # Historically, some nupkgs had content/Content folder : give preference to these folders.
        if [ -d $pkgCachePath/content ]; then
            cp $pkgCachePath/content/* $pkgDestPath -r -f
        elif [ -d $pkgCachePath/Content ]; then
            cp $pkgCachePath/Content/* $pkgDestPath -r -f
        else
            countNuspec=`ls -1 $pkgCachePath/*.nuspec 2>/dev/null | wc -l`
            if [ $countNuspec == 1 ]; then
              # This is an actual nupkg, copy its content directly.
              cp $pkgCachePath/* $pkgDestPath -r -f
              # Copy nupkg to destination folder so that it can work for PackageReference
              cp ${pkgPath} ${pkgDestPath}
            else
              echo "Bad nupkg format : $pkgPath"
              exit 1
            fi
        fi
    fi

    if [ -d $pkgPathlink ]; then
        rm -r ${pkgPathlink}
    fi
    ln $pkgDestPath $pkgPathlink -srf
}

CheckVersion() {
    local pkgName=$1
    local pkgVer=$2
    local pkgSuf=$3

    local pkgFullName=${pkgName}.${pkgVer}
    if [ ! -z "$pkgSuf" ]; then
        pkgFullName=${pkgFullName}-${pkgSuf}
    fi
    if [ -d ${ExtLibPath}/${pkgFullName} ]; then
        if [ -L ${ExtLibPath}/${pkgName} ] && [ "$(basename $(readlink -- "${ExtLibPath}/${pkgName}"))" != ${pkgFullName} ] ; then
            rm ${ExtLibPath}/${pkgName}
            ln ${ExtLibPath}/${pkgFullName} ${ExtLibPath}/${pkgName} -srf
        fi
        return 0
    else
        return 1
    fi
}

for pkg in "${PkgsList[@]}"
do
    if [ -z ${pkg} ]; then 
        continue
    fi 
    echo -n "Checking ${pkg}:"
    pkgNameVerSuf=$(ParsePkgNameVersionSuf $pkg)
    IFS=',' read -ra pkgNameVerSuf <<< "${pkgNameVerSuf}"
    pkgName=${pkgNameVerSuf[0]}
    pkgVer=${pkgNameVerSuf[1]}
    pkgSuf=${pkgNameVerSuf[2]}
    CheckVersion $pkgName $pkgVer $pkgSuf
    if [ $? != 0 ]; then
        echo "not found. Prepare to download."
        DownloadPkg ${pkgName} ${pkgVer} ${pkgSuf}
        ExtractPkg "true" ${pkgName} ${pkgVer} ${pkgSuf}
    else
        echo "found."
        ExtractPkg "false" ${pkgName} ${pkgVer} ${pkgSuf}
    fi
done

LinuxPkgDestPath=${ExtLibPath}/WinFab.Linux.Libs
if [ $IsJenkins != "true" ]; then
    boostTestLibLink="/usr/lib/libboost_unit_test_framework.so"
    boostTestLibFile="/usr/lib/libboost_unit_test_framework.so.1.61.0"
    if [ ! -L $boostTestLibLink ] || [ "$(readlink $boostTestLibLink)" != "$boostTestLibFile" ] || [ ! -f $boostTestLibFile ] ; then
        echo "Installing $LinuxPkgDestPath/Boost_1_61_0/lib/libboost_unit_test_framework.so.1.61.0 into /usr/lib"
        sudo cp $LinuxPkgDestPath/Boost_1_61_0/lib/libboost_unit_test_framework.so.1.61.0 /usr/lib/libboost_unit_test_framework.so.1.61.0 -f --remove-destination
        sudo unlink /usr/lib/libboost_unit_test_framework.so > /dev/null 2>&1
        sudo ln /usr/lib/libboost_unit_test_framework.so.1.61.0 -s /usr/lib/libboost_unit_test_framework.so
    fi
fi
