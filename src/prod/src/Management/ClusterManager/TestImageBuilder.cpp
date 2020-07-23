// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestImageBuilder.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("CM.TestImageBuilder");

unique_ptr<TestImageBuilder> TestImageBuilder::Clone()
{
    return make_unique<TestImageBuilder>(this->sharedState_);
}

void TestImageBuilder::MockUnprovisionApplicationType(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion)
{
    vector<StoreDataServiceManifest> manifestsForAllVersions;
    StoreDataServiceManifest manifestForUnprovisionedVersion;

    // Mimic unprovision cleanup logic to test ref-counting logic
    //
    auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
    if (itName == sharedState_->AppTypeServiceManifests.end()) { return; }

    for (auto itVersion = itName->second.begin(); itVersion != itName->second.end(); ++itVersion)
    {
        auto findType = sharedState_->ProvisionedAppTypes.find(typeName);
        if (findType != sharedState_->ProvisionedAppTypes.end())
        {
            auto findVersion = findType->second.find(itVersion->first);
            if (findVersion != findType->second.end())
            {
                vector<ServiceModelServiceManifestDescription> manifestDescriptions = itVersion->second;

                StoreDataServiceManifest manifestData(
                    typeName,
                    itVersion->first,
                    move(manifestDescriptions));

                manifestsForAllVersions.push_back(manifestData);
            }
        }

        if (itVersion->first == typeVersion)
        {
            vector<ServiceModelServiceManifestDescription> manifestDescriptions = itVersion->second;

            manifestForUnprovisionedVersion = StoreDataServiceManifest(
                typeName,
                itVersion->first,
                move(manifestDescriptions));
        }
    }

    // core ref-counting logic in TrimDuplicateManifestsAndPackages()
    //
    manifestForUnprovisionedVersion.TrimDuplicateManifestsAndPackages(manifestsForAllVersions);

    Trace.WriteNoise(
        TraceComponent,
        "MockUnprovisionApplicationType({0}, {1}): existing({2}) = {3} cleanup = {4}",
        typeName,
        typeVersion,
        manifestsForAllVersions.size(),
        manifestsForAllVersions,
        manifestForUnprovisionedVersion.ServiceManifests);

    this->CleanupApplicationType(
        typeName,
        typeVersion,
        manifestForUnprovisionedVersion.ServiceManifests,
        (manifestsForAllVersions.size() == 1),
        TimeSpan::MaxValue);

    auto findType = sharedState_->ProvisionedAppTypes.find(typeName);
    if (findType != sharedState_->ProvisionedAppTypes.end())
    {
        findType->second.erase(typeVersion);
    }

}

ErrorCode TestImageBuilder::GetApplicationTypeInfo(
    std::wstring const &,
    TimeSpan const,
    __out ServiceModelTypeName &,
    __out ServiceModelVersion &)
{
    return GetNextError();
}

ErrorCode TestImageBuilder::BuildManifestContexts(
    vector<ApplicationTypeContext> const& applicationTypeContexts,
    TimeSpan const timeout,
    __out std::vector<StoreDataApplicationManifest> &,
    __out std::vector<StoreDataServiceManifest> &)
{
    UNREFERENCED_PARAMETER(applicationTypeContexts);
    UNREFERENCED_PARAMETER(timeout);

    return ErrorCode::Success();
}

ErrorCode TestImageBuilder::Test_BuildApplicationType(
    Common::ActivityId const & activityId,
    std::wstring const & buildPath,
    std::wstring const & downloadPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    TimeSpan const,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring &,
    __out ApplicationHealthPolicy &,
    __out map<wstring, wstring> &)
{
    UNREFERENCED_PARAMETER(activityId);
    UNREFERENCED_PARAMETER(buildPath);
    UNREFERENCED_PARAMETER(downloadPath);

    auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
    if (itName != sharedState_->AppTypeServiceManifests.end())
    {
        auto itVersion = itName->second.find(typeVersion);
        if (itVersion != itName->second.end())
        {
            serviceManifests = itVersion->second;
        }
    }

    return GetNextError();
}

ErrorCode TestImageBuilder::Test_BuildApplication(
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::map<std::wstring, std::wstring> const & parameters,
    TimeSpan const,
    __out DigestedApplicationDescription & appDescription)
{
    UNREFERENCED_PARAMETER(typeName);
    UNREFERENCED_PARAMETER(typeVersion);
    UNREFERENCED_PARAMETER(parameters);

    {
        auto iter = sharedState_->AppPackageMap.find(appName);
        if (iter != sharedState_->AppPackageMap.end())
        {
            for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
            {
                StoreDataServicePackage package(
                    appId,
                    appName,
                    iter2->first,
                    iter2->second,
                    GetServicePackageVersion(),
                    ServiceModel::ServiceTypeDescription());

                appDescription.ServicePackages.push_back(package);
            }
        }
        else
        {
            Trace.WriteNoise(TraceComponent, "No packages in application '{0}' ({1})", appName, sharedState_->AppPackageMap.size());
        }
    }

    {
        auto iter = sharedState_->AppTemplatesMap.find(appName);
        if (iter != sharedState_->AppTemplatesMap.end())
        {
            for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
            {
                PartitionedServiceDescriptor psd = *iter2;
                StoreDataServiceTemplate serviceTemplate(
                    appId,
                    appName,
                    ServiceModelTypeName(iter2->Service.Type.ServiceTypeName),
                    std::move(psd));
                appDescription.ServiceTemplates.push_back(serviceTemplate);
            }
        }
        else
        {
            Trace.WriteNoise(TraceComponent, "No templates in application '{0}'", appName);
        }
    }

    {
        auto iter = sharedState_->AppDefaultServicesMap.find(appName);
        if (iter != sharedState_->AppDefaultServicesMap.end())
        {
            for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
            {
                appDescription.DefaultServices.push_back(*iter2);
            }
        }
        else
        {
            Trace.WriteNoise(TraceComponent, "No required services in application '{0}'", appName);
        }
    }

    return GetNextError();
}

ErrorCode TestImageBuilder::GetFabricVersionInfo(
    std::wstring const & codeFilepath,
    std::wstring const & clusterManifestFilepath,
    Common::TimeSpan const timeout,
    __out Common::FabricVersion &)
{
    UNREFERENCED_PARAMETER(codeFilepath);
    UNREFERENCED_PARAMETER(clusterManifestFilepath);
    UNREFERENCED_PARAMETER(timeout);

    return GetNextError();
}

ErrorCode TestImageBuilder::ProvisionFabric(
    std::wstring const & codeFilepath,
    std::wstring const & clusterManifestFilepath,
    TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(codeFilepath);
    UNREFERENCED_PARAMETER(clusterManifestFilepath);
    UNREFERENCED_PARAMETER(timeout);

    return GetNextError();
}

ErrorCode TestImageBuilder::GetClusterManifestContents(
    FabricConfigVersion const &,
    TimeSpan const,
    __out std::wstring &)
{
    return GetNextError();
}

ErrorCode TestImageBuilder::UpgradeFabric(
    FabricVersion const & currentVersion,
    FabricVersion const & targetVersion,
    TimeSpan const timeout,
    __out bool & isConfigOnly)
{
    UNREFERENCED_PARAMETER(currentVersion);
    UNREFERENCED_PARAMETER(targetVersion);
    UNREFERENCED_PARAMETER(timeout);
    UNREFERENCED_PARAMETER(isConfigOnly);

    return GetNextError();
}

ErrorCode TestImageBuilder::Test_CleanupApplication(
    ServiceModelTypeName const & appTypeName,
    ServiceModelApplicationId const & appId,
    TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(appTypeName);
    UNREFERENCED_PARAMETER(appId);
    UNREFERENCED_PARAMETER(timeout);

    return GetNextError();
}

ErrorCode TestImageBuilder::Test_CleanupApplicationInstance(
    ApplicationInstanceDescription const &,
    bool,
    vector<ServicePackageReference> const &,
    TimeSpan const)
{
    return GetNextError();
}

ErrorCode TestImageBuilder::CleanupApplicationType(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::vector<ServiceModelServiceManifestDescription> const & manifests,
    bool isLastApplicationType,
    TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(timeout);
        
    for (auto it = manifests.begin(); it != manifests.end(); ++it)
    {
        auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
        if (itName != sharedState_->AppTypeServiceManifests.end())
        {
            auto itVersion = itName->second.find(typeVersion);
            if (itVersion != itName->second.end())
            {
                for (auto itManifest = itVersion->second.begin(); itManifest != itVersion->second.end(); ++itManifest)
                {
                    if (itManifest->Name == it->Name && itManifest->Version == it->Version)
                    {
                        itManifest->TrimDuplicatePackages(*it);
                    }

                    if (itManifest->CodePackages.empty() &&
                        itManifest->ConfigPackages.empty() &&
                        itManifest->DataPackages.empty())
                    {
                        // service manifest contains no packages
                        itManifest = itVersion->second.erase(itManifest);
                        if (itManifest != itVersion->second.begin())
                        {
                            --itManifest;
                        }
                    }

                    if (itManifest == itVersion->second.end())
                    {
                        break;
                    }
                }

                if (itVersion->second.empty())
                {
                    // application type version contains no service manifests
                    itName->second.erase(itVersion);
                }
            }

            if (itName->second.empty())
            {
                // no more versions of this application type
                sharedState_->AppTypeServiceManifests.erase(itName);
            }
        }
    }

    if (isLastApplicationType)
    {
        auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
        if (itName != sharedState_->AppTypeServiceManifests.end())
        {
            sharedState_->AppTypeServiceManifests.erase(itName);
        }
    }

    return GetNextError();
}

ErrorCode TestImageBuilder::CleanupFabricVersion(
    FabricVersion const & version,
    TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(version);
    UNREFERENCED_PARAMETER(timeout);

    return GetNextError();
}

ErrorCode TestImageBuilder::CleanupApplicationPackage(
    std::wstring const & buildPath,
    Common::TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(buildPath);
    UNREFERENCED_PARAMETER(timeout);

    return GetNextError();
}

TestImageBuilder::SharedStateWrapper TestImageBuilder::GetSharedState()
{
    return TestImageBuilder::SharedStateWrapper(sharedState_);
}

bool TestImageBuilder::SharedStateWrapper::TryGetServiceManifest(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::wstring const & name,
    std::wstring const & version,
    __out ServiceModelServiceManifestDescription & result)
{
    auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
    if (itName != sharedState_->AppTypeServiceManifests.end())
    {
        auto itVersion = itName->second.find(typeVersion);
        if (itVersion != itName->second.end())
        {
            for (auto itManifest = itVersion->second.begin(); itManifest != itVersion->second.end(); ++itManifest)
            {
                if (itManifest->Name == name && itManifest->Version == version)
                {
                    result = *itManifest;
                    return true;
                }
            }
        }
    }

    result = ServiceModelServiceManifestDescription();
    return false;
}

void TestImageBuilder::MockProvision(
    std::wstring const & buildPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion)
{
    UNREFERENCED_PARAMETER(buildPath);

    auto findType = sharedState_->ProvisionedAppTypes.find(typeName);
    if (findType == sharedState_->ProvisionedAppTypes.end())
    {
        sharedState_->ProvisionedAppTypes[typeName] = set<ServiceModelVersion>();
    }

    sharedState_->ProvisionedAppTypes[typeName].insert(typeVersion);
}

void TestImageBuilder::SetServiceManifests(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    vector<ServiceModelServiceManifestDescription> && serviceManifests)
{
    auto itName = sharedState_->AppTypeServiceManifests.find(typeName);
    if (itName == sharedState_->AppTypeServiceManifests.end())
    {
        itName = sharedState_->AppTypeServiceManifests.insert(make_pair(typeName, map<ServiceModelVersion, vector<ServiceModelServiceManifestDescription>>())).first;
    }

    auto itVersion = itName->second.find(typeVersion);
    if (itVersion == itName->second.end())
    {
        itVersion = itName->second.insert(make_pair(typeVersion, vector<ServiceModelServiceManifestDescription>())).first;
    }

    itVersion->second = move(serviceManifests);
}

void TestImageBuilder::AddPackage(
    NamingUri const & appName,
    ServiceModelTypeName const & typeName,
    ServiceModelPackageName const & packageName)
{
    auto iter = sharedState_->AppPackageMap.find(appName);
    if (iter == sharedState_->AppPackageMap.end())
    {
        sharedState_->AppPackageMap[appName] = map<ServiceModelTypeName, ServiceModelPackageName>();
        iter = sharedState_->AppPackageMap.find(appName);
    }

    iter->second[typeName] = packageName;
}

void TestImageBuilder::AddTemplate(
    NamingUri const & appName,
    PartitionedServiceDescriptor const & serviceTemplate)
{
    auto iter = sharedState_->AppTemplatesMap.find(appName);
    if (iter == sharedState_->AppTemplatesMap.end())
    {
        sharedState_->AppTemplatesMap[appName] = vector<PartitionedServiceDescriptor>();
    }

    sharedState_->AppTemplatesMap[appName].push_back(serviceTemplate);
}

void TestImageBuilder::AddDefaultService(
    NamingUri const & appName,
    PartitionedServiceDescriptor const & psd)
{
    auto iter = sharedState_->AppDefaultServicesMap.find(appName);
    if (iter == sharedState_->AppDefaultServicesMap.end())
    {
        sharedState_->AppDefaultServicesMap[appName] = vector<PartitionedServiceDescriptor>();
    }

    sharedState_->AppDefaultServicesMap[appName].push_back(psd);
}

void TestImageBuilder::SetNextError(ErrorCodeValue::Enum error, int operationCount)
{
    Trace.WriteNoise(TraceComponent, "Set next error to {0}", error);
    sharedState_->NextError = error;
    sharedState_->NextErrorOperationCount = operationCount;
}

ErrorCode TestImageBuilder::GetNextError()
{
    if (sharedState_->NextErrorOperationCount <= 0)
    {
        auto error = sharedState_->NextError;
        sharedState_->NextError = ErrorCodeValue::Success;
        return error;
    }
    else
    {
        --(sharedState_->NextErrorOperationCount);
        return ErrorCodeValue::Success;
    }
}

ServiceModelVersion TestImageBuilder::GetServicePackageVersion()
{
    return
        ServiceModelVersion(
        ServicePackageVersion(
        ApplicationVersion(RolloutVersion(4, 2)),
        RolloutVersion(3, 7)
        ).ToString()
        );
}
