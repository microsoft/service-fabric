#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


NugetRepoList=("http://wanuget.corp.microsoft.com/ManualMirror/api/v2/package/" "http://wanuget.corp.microsoft.com/Dev/api/v2/package/")

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

ExtLibPath=${ScriptPath}/../../External/

CxCache="/tmp/CxCache"
if [ ! -d "$CxCache" ]; then
  mkdir "$CxCache";
fi;

AcctDomain=""
AcctName=""

PromptForCreds() {
    if [ $IsJenkins == "true" ]; then
      return
    fi
    if [ -z $AcctName ]; then
        read -rp "NTLM User(e.g. redmond\user): " domainAccount
        IFS='\\' read -ra domainAccount <<< "$domainAccount"
        AcctDomain=${domainAccount[0]}
        AcctName=${domainAccount[1]}
    fi
}

ParsePkgNameVersion(){
    local pkgName
    local pkgVer
    IFS='.' read -ra tokens <<< "$1"
    for tok in "${tokens[@]}"
    do
       if [[ "$tok" = "nupkg" ]]; then
           break
       fi
       if [[ "$tok" =~ ^[^0-9]+$ ]]; then
           if [ ! -z "$pkgName" ]; then
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
    echo "$pkgName,$pkgVer"
}

DownloadPkg() {
    pkgName=$1
    pkgVer=$2
    pkgPath=/tmp/${pkgName}.${pkgVer}.nupkg
    httpResp=
    found=0
    for nugetRepo in "${NugetRepoList[@]}"
    do
        echo Connecting to $nugetRepo..
        if [ $IsJenkins == "true" ]; then
            httpResp=$(curl -s -w %{http_code} --ntlm -u ${AZURE_CREDENTIALS} $nugetRepo/${pkgName}/${pkgVer} -o ${pkgPath})
        else
            httpResp=$(curl -s -w %{http_code} --ntlm -u ${AcctDomain}\\${AcctName} $nugetRepo/${pkgName}/${pkgVer} -o ${pkgPath})
        fi
        if [ "$httpResp" -eq "200" ]; then
            found=1 
            break
        else
            echo "HTTP $httpResp"
        fi
    done
   
    if [ "$found" -ne "1" ]; then
        echo "Download failed. Aborting."
        exit -1
    fi
}

ExtractPkg() {
    local pkgName=$1
    local pkgVer=$2
    local pkgPath=/tmp/${pkgName}.${pkgVer}.nupkg

    echo "Extracting ${pkgPath}..."
    local pkgNameVer=$(basename "$pkgPath")
    local pkgCachePath=$CxCache/$pkgNameVer
    if [ ! -d $pkgCachePath ]; then
      mkdir $pkgCachePath;
      if [ $? -ne 0 ]; then
        echo Directory $pkgCachePath creation failed. Using root to create.
        sudo chown $USER:$USER $CxCache -R >/dev/null 2>&1
        rm $pkgCachePath -r
        mkdir "$pkgCachePath"
      fi
      unzip $pkgPath -d $pkgCachePath > /dev/null;
    fi;

    local pkgDestPath=$ExtLibPath/${pkgName}.${pkgVer}
    local pkgPathlink=$ExtLibPath/${pkgName}
    rm -rf $pkgDestPath
    mkdir $pkgDestPath;
    if [ -d $pkgCachePath/content ]; then
        cp $pkgCachePath/content/* $pkgDestPath -r -f
    fi
    if [ -d $pkgCachePath/Content ]; then
        cp $pkgCachePath/Content/* $pkgDestPath -r -f
    fi
    if [ -d $pkgPathlink ]; then
        rm -r ${pkgPathlink}
    fi
    ln $pkgDestPath $pkgPathlink -srf
}

CheckVersion() {
    local pkgName=$1
    local pkgVer=$2

    if [ -d ${ExtLibPath}/${pkgName}.${pkgVer} ]; then
        return 0
    else
        return 1
    fi
}

for pkg in "${PkgsList[@]}"
do
    echo -n "Checking ${pkg}:"
    pkgNameVer=$(ParsePkgNameVersion $pkg)
    IFS=',' read -ra pkgNameVer <<< "${pkgNameVer}"
    pkgName=${pkgNameVer[0]}
    pkgVer=${pkgNameVer[1]}
    CheckVersion $pkgName $pkgVer
    if [ $? != 0 ]; then
        echo "not found. Prepare to download."
        PromptForCreds
        DownloadPkg ${pkgName} ${pkgVer}
        ExtractPkg ${pkgName} ${pkgVer}
    else
        echo "found."
    fi
done

# LinuxPkgDestPath=$ScriptPath/../../External/Ktl.Linux.Libs
# if [ $IsJenkins != "true" ]; then
#     boostTestLibLink="/usr/lib/libboost_unit_test_framework.so"
#     boostTestLibFile="/usr/lib/libboost_unit_test_framework.so.1.61.0"
#     if [ ! -L $boostTestLibLink ] || [ "$(readlink $boostTestLibLink)" != "$boostTestLibFile" ] || [ ! -f $boostTestLibFile ] ; then
#         echo "Installing $LinuxPkgDestPath/Boost_1_61_0/lib/libboost_unit_test_framework.so.1.61.0 into /usr/lib"
#         sudo cp $LinuxPkgDestPath/Boost_1_61_0/lib/libboost_unit_test_framework.so /usr/lib/libboost_unit_test_framework.so.1.61.0 -f --remove-destination
#         sudo unlink /usr/lib/libboost_unit_test_framework.so > /dev/null 2>&1
#         sudo ln /usr/lib/libboost_unit_test_framework.so.1.61.0 -s /usr/lib/libboost_unit_test_framework.so
#     fi
# fi