// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

FabricDeploymentSpecification::FabricDeploymentSpecification()
    : dataRoot_()
{
}

FabricDeploymentSpecification::FabricDeploymentSpecification(wstring const & dataRoot)
    : dataRoot_(dataRoot)
{
    logRoot_ = Path::Combine(dataRoot_, L"Log");
}

FabricDeploymentSpecification::FabricDeploymentSpecification(wstring const & dataRoot, wstring const & logRoot)
    : dataRoot_(dataRoot),
    logRoot_(logRoot)
{    
}

FabricDeploymentSpecification::FabricDeploymentSpecification(FabricDeploymentSpecification const & other)
    : dataRoot_(other.dataRoot_),
    logRoot_(other.logRoot_)
{
}

FabricDeploymentSpecification& FabricDeploymentSpecification::operator =(FabricDeploymentSpecification const & other)
{
    if (this != &other)
    {
        dataRoot_ = other.dataRoot_;
        logRoot_ = other.logRoot_;
    }

    return *this;
}

void FabricDeploymentSpecification::set_DataRoot(wstring const & value) 
{ 
    dataRoot_.assign(value); 
    if (logRoot_ == L"")
    {
        logRoot_ = Path::Combine(dataRoot_, L"log");
    }
}

wstring FabricDeploymentSpecification::GetLogFolder() const
{
    return logRoot_;
}

wstring FabricDeploymentSpecification::GetTracesFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"Traces");
}

wstring FabricDeploymentSpecification::GetCrashDumpsFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"CrashDumps");
}

wstring FabricDeploymentSpecification::GetApplicationCrashDumpsFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"ApplicationCrashDumps");
}

wstring FabricDeploymentSpecification::GetAppInstanceDataFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"AppInstanceData");
}

wstring FabricDeploymentSpecification::GetAppInstanceEtlDataFolder() const
{
    wstring appInstanceDataFolder = GetAppInstanceDataFolder();
    return Path::Combine(appInstanceDataFolder, L"Etl");
}

wstring FabricDeploymentSpecification::GetPerformanceCountersBinaryFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"PerformanceCountersBinary");
}

wstring FabricDeploymentSpecification::GetTargetInformationFile() const
{    
    return Path::Combine(dataRoot_, L"TargetInformation.xml");
}

wstring FabricDeploymentSpecification::GetNodeFolder(wstring const & nodeName) const
{
    return Path::Combine(dataRoot_, nodeName);
}

wstring FabricDeploymentSpecification::GetFabricFolder(wstring const & nodeName) const
{
    wstring nodeFolder = GetNodeFolder(nodeName);
    return Path::Combine(nodeFolder, L"Fabric");
}


wstring FabricDeploymentSpecification::GetCurrentClusterManifestFile(wstring const & nodeName) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    return Path::Combine(fabricFolder, L"ClusterManifest.current.xml");
}

wstring FabricDeploymentSpecification::GetVersionedClusterManifestFile(wstring const & nodeName, wstring const & version) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    wstring fileName = wformatString("ClusterManifest.{0}.xml", version);
    return Path::Combine(fabricFolder, fileName);
}

wstring FabricDeploymentSpecification::GetInstallerScriptFile(wstring const & nodeName) const
{
#if defined(PLATFORM_UNIX)
    wstring fileName = wformatString("startservicefabricupdater.{0}.sh", nodeName);
#else
    wstring fileName = wformatString("MSIInstaller.{0}.bat", nodeName);
#endif
    return Path::Combine(dataRoot_, fileName);
}

wstring FabricDeploymentSpecification::GetInstallerLogFile(wstring const & nodeName, wstring const & codeVersion) const
{
    wstring logFolder = GetTracesFolder();
#if defined(PLATFORM_UNIX)
    wstring logFileName = wformatString("startservicefabricupdater_{0}_{1}_{2}.log", nodeName, codeVersion, DateTime::Now().Ticks);
#else
    wstring logFileName = wformatString("MsiInstaller_{0}_{1}_{2}.log", nodeName, codeVersion, DateTime::Now().Ticks);
#endif
    return Path::Combine(logFolder, logFileName);
}

#if defined(PLATFORM_UNIX)
wstring FabricDeploymentSpecification::GetUpgradeScriptFile(wstring const & nodeName) const
{
    wstring fileName = wformatString("doupgrade.{0}.sh", nodeName);
    return Path::Combine(dataRoot_, fileName);
}
#endif

wstring FabricDeploymentSpecification::GetInfrastructureManfiestFile(wstring const & nodeName)
{
    wstring dataFolder = GetDataDeploymentFolder(nodeName);
    return Path::Combine(dataFolder, L"InfrastructureManifest.xml");
}

wstring FabricDeploymentSpecification::GetConfigurationDeploymentFolder(wstring const & nodeName, wstring const & configVersion) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    wstring configFolderName = wformatString("Fabric.Config.{0}", configVersion);
    return Path::Combine(fabricFolder, configFolderName);
}

wstring FabricDeploymentSpecification::GetDataDeploymentFolder(wstring const & nodeName) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    return Path::Combine(fabricFolder, L"Fabric.Data");
}

wstring FabricDeploymentSpecification::GetCodeDeploymentFolder(wstring const & nodeName, wstring const & service) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    wstring codeFolderName = wformatString("{0}.Code", service);
    return Path::Combine(fabricFolder, codeFolderName);
}

wstring FabricDeploymentSpecification::GetWorkFolder(wstring const & nodeName) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    return Path::Combine(fabricFolder, L"work");
}

wstring FabricDeploymentSpecification::GetInstalledBinaryFolder(wstring const & installationDirectory, wstring const & service) const
{
    wstring codeFolderName = wformatString("{0}.Code", service);
    return Path::Combine(Path::Combine(installationDirectory, L"Fabric"), codeFolderName);
}

wstring FabricDeploymentSpecification::GetCurrentFabricPackageFile(wstring const & nodeName) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    return Path::Combine(fabricFolder, L"Fabric.Package.current.xml");
}

wstring FabricDeploymentSpecification::GetVersionedFabricPackageFile(wstring const & nodeName, wstring const & version) const
{
    wstring fabricFolder = GetFabricFolder(nodeName);
    wstring fileName = wformatString("Fabric.Package.{0}.xml", version);
    return Path::Combine(fabricFolder, fileName);
}

wstring FabricDeploymentSpecification::GetQueryTraceFolder() const
{
    wstring logFolder = GetLogFolder();
    return Path::Combine(logFolder, L"QueryTraces");
}
