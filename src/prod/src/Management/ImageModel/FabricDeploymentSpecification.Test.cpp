// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Common/Common.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#if defined(PLATFORM_UNIX)
#define PATHSEP L"/"
#else
#define PATHSEP L"\\"
#endif

namespace ImageModelTests
{
    using namespace std;
    using namespace Common;
    using namespace Management;
    using namespace ImageModel;
    
    const StringLiteral TestSource = "ImageModelTest";

    class FabricDeploymentSpecificationTest
    {
    };


    BOOST_FIXTURE_TEST_SUITE(FabricDeploymentSpecificationTestSuite,FabricDeploymentSpecificationTest)

    BOOST_AUTO_TEST_CASE(SpecifyingFabricDataRootOnlyOnConstructor)
    {
        FabricDeploymentSpecification depSpec(L"DataRoot");
        
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.DataRoot, L"DataRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.LogRoot, L"DataRoot" PATHSEP L"Log"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceDataFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"AppInstanceData"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceEtlDataFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"AppInstanceData" PATHSEP L"Etl"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetApplicationCrashDumpsFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"ApplicationCrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCodeDeploymentFolder(L"Node1", L"Fabric"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetConfigurationDeploymentFolder(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Config.1.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCrashDumpsFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"CrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentClusterManifestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentFabricPackageFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetDataDeploymentFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetFabricFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInfrastructureManfiestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data" PATHSEP L"InfrastructureManifest.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstalledBinaryFolder(L"Install", L"Fabric"), L"Install" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
#if defined(PLATFORM_UNIX)          
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"startservicefabricupdater.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"MsiInstaller.Node1.bat"));
#endif        
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetLogFolder(), L"DataRoot" PATHSEP L"Log"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetNodeFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetPerformanceCountersBinaryFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"PerformanceCountersBinary"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTargetInformationFile(), L"DataRoot" PATHSEP L"TargetInformation.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTracesFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedClusterManifestFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedFabricPackageFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetWorkFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"work"));
#if defined(PLATFORM_UNIX)  
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces" PATHSEP L"startservicefabricupdater_Node1_1.0_"));
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetUpgradeScriptFile(L"Node1"), L"DataRoot" PATHSEP L"doupgrade.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces" PATHSEP L"MsiInstaller_Node1_1.0_"));
#endif 
    }

    BOOST_AUTO_TEST_CASE(SpecifyingFabricDataRootAndLogRootOnConstructor)
    {
        FabricDeploymentSpecification depSpec(L"DataRoot", L"LogRoot");

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.DataRoot, L"DataRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.LogRoot, L"LogRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceDataFolder(), L"LogRoot" PATHSEP L"AppInstanceData"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceEtlDataFolder(), L"LogRoot" PATHSEP L"AppInstanceData" PATHSEP L"Etl"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetApplicationCrashDumpsFolder(), L"LogRoot" PATHSEP L"ApplicationCrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCodeDeploymentFolder(L"Node1", L"Fabric"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetConfigurationDeploymentFolder(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Config.1.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCrashDumpsFolder(), L"LogRoot" PATHSEP L"CrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentClusterManifestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentFabricPackageFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetDataDeploymentFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetFabricFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInfrastructureManfiestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data" PATHSEP L"InfrastructureManifest.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstalledBinaryFolder(L"Install", L"Fabric"), L"Install" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
#if defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"startservicefabricupdater.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"MsiInstaller.Node1.bat"));
#endif
        
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetLogFolder(), L"LogRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetNodeFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetPerformanceCountersBinaryFolder(), L"LogRoot" PATHSEP L"PerformanceCountersBinary"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTargetInformationFile(), L"DataRoot" PATHSEP L"TargetInformation.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTracesFolder(), L"LogRoot" PATHSEP L"Traces"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedClusterManifestFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedFabricPackageFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetWorkFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"work"));

#if defined(PLATFORM_UNIX)  
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"LogRoot" PATHSEP L"Traces" PATHSEP L"startservicefabricupdater_Node1_1.0_"));
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetUpgradeScriptFile(L"Node1"), L"DataRoot" PATHSEP L"doupgrade.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"LogRoot" PATHSEP L"Traces" PATHSEP L"MsiInstaller_Node1_1.0_"));
#endif 
    }

    BOOST_AUTO_TEST_CASE(EmptyConstructorUsingFabricDataRootOnly)
    {
        FabricDeploymentSpecification depSpec;
        depSpec.DataRoot = L"DataRoot";

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.DataRoot, L"DataRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.LogRoot, L"DataRoot" PATHSEP L"Log"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceDataFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"AppInstanceData"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceEtlDataFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"AppInstanceData" PATHSEP L"Etl"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetApplicationCrashDumpsFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"ApplicationCrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCodeDeploymentFolder(L"Node1", L"Fabric"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetConfigurationDeploymentFolder(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Config.1.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCrashDumpsFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"CrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentClusterManifestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentFabricPackageFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetDataDeploymentFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetFabricFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInfrastructureManfiestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data" PATHSEP L"InfrastructureManifest.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstalledBinaryFolder(L"Install", L"Fabric"), L"Install" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
#if defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"startservicefabricupdater.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"MsiInstaller.Node1.bat"));
#endif
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetLogFolder(), L"DataRoot" PATHSEP L"Log"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetNodeFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetPerformanceCountersBinaryFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"PerformanceCountersBinary"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTargetInformationFile(), L"DataRoot" PATHSEP L"TargetInformation.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTracesFolder(), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedClusterManifestFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedFabricPackageFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetWorkFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"work"));

#if defined(PLATFORM_UNIX)  
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces" PATHSEP L"startservicefabricupdater_Node1_1.0_"));
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetUpgradeScriptFile(L"Node1"), L"DataRoot" PATHSEP L"doupgrade.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Log" PATHSEP L"Traces" PATHSEP L"MsiInstaller_Node1_1.0_"));
#endif 
    }

    BOOST_AUTO_TEST_CASE(EmptyConstructorUsingFabricDataRootAndLogRoot)
    {
        FabricDeploymentSpecification depSpec;
        depSpec.DataRoot = L"DataRoot";
        depSpec.LogRoot = L"LogRoot";

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.DataRoot, L"DataRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.LogRoot, L"LogRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceDataFolder(), L"LogRoot" PATHSEP L"AppInstanceData"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetAppInstanceEtlDataFolder(), L"LogRoot" PATHSEP L"AppInstanceData" PATHSEP L"Etl"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetApplicationCrashDumpsFolder(), L"LogRoot" PATHSEP L"ApplicationCrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCodeDeploymentFolder(L"Node1", L"Fabric"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetConfigurationDeploymentFolder(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Config.1.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCrashDumpsFolder(), L"LogRoot" PATHSEP L"CrashDumps"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentClusterManifestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetCurrentFabricPackageFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.Current.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetDataDeploymentFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetFabricFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInfrastructureManfiestFile(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Data" PATHSEP L"InfrastructureManifest.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstalledBinaryFolder(L"Install", L"Fabric"), L"Install" PATHSEP L"Fabric" PATHSEP L"Fabric.Code"));
#if defined(PLATFORM_UNIX)
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"startservicefabricupdater.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetInstallerScriptFile(L"Node1"), L"DataRoot" PATHSEP L"MsiInstaller.Node1.bat"));
#endif
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetLogFolder(), L"LogRoot"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetNodeFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetPerformanceCountersBinaryFolder(), L"LogRoot" PATHSEP L"PerformanceCountersBinary"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTargetInformationFile(), L"DataRoot" PATHSEP L"TargetInformation.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetTracesFolder(), L"LogRoot" PATHSEP L"Traces"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedClusterManifestFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"ClusterManifest.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetVersionedFabricPackageFile(L"Node1", L"1.0"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"Fabric.Package.1.0.xml"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(depSpec.GetWorkFolder(L"Node1"), L"DataRoot" PATHSEP L"Node1" PATHSEP L"Fabric" PATHSEP L"work"));

#if defined(PLATFORM_UNIX)  
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"LogRoot" PATHSEP L"Traces" PATHSEP L"startservicefabricupdater_Node1_1.0_"));
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetUpgradeScriptFile(L"Node1"), L"DataRoot" PATHSEP L"doupgrade.Node1.sh"));
#else
        VERIFY_IS_TRUE(StringUtility::StartsWithCaseInsensitive<wstring>(depSpec.GetInstallerLogFile(L"Node1", L"1.0"), L"LogRoot" PATHSEP L"Traces" PATHSEP L"MsiInstaller_Node1_1.0_"));
#endif 
    }

    BOOST_AUTO_TEST_SUITE_END()
}
