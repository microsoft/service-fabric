#!/bin/bash

# Usage : ./<script_name>.sh <path_of_managed_lib_dir> <nuget_packages_files> <is_local_build_packaging_off>
# This script creaes a "external" directory inside publish folder of managed bits
# in a way that is consumable by current linuxsetup scripts.
# "external" folder mimics the folder structure setup by WinFab.CoreClr.Libs.nuproj

MANAGED_LIB_DIR=$1
MANAGED_UBUNTU_LIB_DIR=${MANAGED_LIB_DIR}/netstandard2.0/ubuntu.16.04-x64
MANAGED_PUBLISH_DIR=${MANAGED_LIB_DIR}/publish
MANAGED_PUBLISH_EXTERNAL_DIR=${MANAGED_PUBLISH_DIR}/external

SRC_ROOT=$2
NUGET_PACKAGE_FILES=${SRC_ROOT}/prod/src/managed/Api/CoreCLRDlls/NugetPackageFiles/.

if [ "$3" == "OFF" ]; then
   echo "Not packaging using Managed build"
   exit 0
fi

echo "Packaging using Linux build managed code."

# setup Fabric.Code
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS
cp -r ${MANAGED_PUBLISH_DIR}/azurecliproxy/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/
cp -r ${MANAGED_PUBLISH_DIR}/FabricCAS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/
cp -r ${MANAGED_PUBLISH_DIR}/FabricDeployer/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/
cp -r ${MANAGED_PUBLISH_DIR}/ImageBuilder/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/
cp ${MANAGED_UBUNTU_LIB_DIR}/System.Fabric.Management/System.Fabric.Management.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/
cp ${MANAGED_UBUNTU_LIB_DIR}/Microsoft.ServiceFabric.Data.Impl/Microsoft.ServiceFabric.Data.Impl.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/
cp ${MANAGED_UBUNTU_LIB_DIR}/Microsoft.ServiceFabric.Data.Impl/Microsoft.ServiceFabric.Data.Impl.McgInterop.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/
cp ${MANAGED_UBUNTU_LIB_DIR}/System.Fabric.BackupRestore/System.Fabric.BackupRestore.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/
cp ${MANAGED_UBUNTU_LIB_DIR}/System.Fabric/System.Private.Interop.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/
cp ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/System.Private.CompilerServices.ICastable.dll ${MANAGED_PUBLISH_EXTERNAL_DIR}/Fabric.Code/NS/

# setup FabricUS
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricUS/US.Code.Current
cp -r ${MANAGED_PUBLISH_DIR}/FabricUS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricUS/US.Code.Current/
cp -r ${NUGET_PACKAGE_FILES}/FabricUS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricUS/

# setup FabricTVS
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricTVS/TVS.Code.Current
cp -r ${MANAGED_PUBLISH_DIR}/TokenValidationSvc/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricTVS/TVS.Code.Current/
cp -r ${NUGET_PACKAGE_FILES}/FabricTVS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricTVS/

# setup DCA
cp -r ${MANAGED_PUBLISH_DIR}/FabricDCA/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/DCA.Code/

# setup FabricFAS
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricFAS/FAS.Code.Current
cp -r ${MANAGED_PUBLISH_DIR}/FabricFAS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricFAS/FAS.Code.Current/
cp -r ${NUGET_PACKAGE_FILES}/FabricFAS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricFAS/

# setup FabricGRM
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricGRM/GRM.Code.Current
cp -r ${MANAGED_PUBLISH_DIR}/FabricGRM/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricGRM/GRM.Code.Current/
cp -r ${NUGET_PACKAGE_FILES}/FabricGRM/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricGRM/
mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricGRM/GRM.Data.Current
cp -r ${SRC_ROOT}/prod/src/managed/FabricGRM/src/system/fabric/FabricGRM/GRM.Data.Current/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricGRM/GRM.Data.Current/

# setup FabricIS
# uncomment when FabricIS is compiling on linux.
# mkdir -p ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricIS/IS.Code.Current
# cp -r ${MANAGED_PUBLISH_DIR}/FabricIS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricIS/IS.Code.Current/
# cp -r ${NUGET_PACKAGE_FILES}/FabricIS/. ${MANAGED_PUBLISH_EXTERNAL_DIR}/FabricIS/

# Add version file
cp ${NUGET_PACKAGE_FILES}/VERSION ${MANAGED_PUBLISH_EXTERNAL_DIR}/
