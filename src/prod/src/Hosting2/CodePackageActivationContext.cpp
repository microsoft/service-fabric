// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType("CodePackageActivationContext");

// ********************************************************************************************************************
// CodePackageActivationContext::WorkDirectories Implementation
//
class CodePackageActivationContext::WorkDirectories
{
    DENY_COPY(WorkDirectories);

public:
    WorkDirectories(CodePackageActivationContext & owner)
        : workDirectoriesMap_(),
        owner_(owner)
    {
    }

    void Add(wstring const& directoryName, wstring const& directoryPath)
    {
        {
            auto iter = workDirectoriesMap_.find(directoryName);

            ASSERT_IF(iter != workDirectoriesMap_.end(), "Directory Already Present: Name={0}, Path={1}", directoryName, iter->second);

            workDirectoriesMap_.insert(
              make_pair(
              directoryName,
              directoryPath));
        }
    }

    ErrorCode Get(wstring const& directoryName, __out wstring & directoryPath)
    {
        {
            auto iter = workDirectoriesMap_.find(directoryName);
            if (iter != workDirectoriesMap_.end())
            {
                directoryPath = iter->second;
            }
            else
            {
                directoryPath = L"";
                WriteError(
                    TraceType,
                    owner_.ContextId,
                    "Directory Notfound: Name={0}",
                    directoryName);

                return ErrorCodeValue::DirectoryNotFound;
            }

        }

        return ErrorCodeValue::Success;
    }

private:
    std::map<wstring, wstring> workDirectoriesMap_;
    CodePackageActivationContext & owner_;
};


// ********************************************************************************************************************
// CodePackageActivationContext Implementation
//


ErrorCode CodePackageActivationContext::CreateCodePackageActivationContext(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    ServiceModel::RolloutVersion const & applicationRolloutVersion,
    wstring const & deploymentRoot,
    Client::IpcHealthReportingClientSPtr & ipcHealthReportingClient,
    FabricNodeContextSPtr & nodeContext,
    bool isContainerHost,
    CodePackageActivationContextSPtr & codePackageActivationContextSPtr)
{
    CodePackageActivationContextSPtr temp = CodePackageActivationContextSPtr(new CodePackageActivationContext(
        codePackageInstanceId,
        applicationRolloutVersion,
        deploymentRoot,
        ipcHealthReportingClient,
        nodeContext,
        isContainerHost));
    auto error = temp->EnsureData();
    if (error.IsSuccess())
    {
        codePackageActivationContextSPtr = move(temp);
    }

    return error;
}

CodePackageActivationContext::CodePackageActivationContext(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    ServiceModel::RolloutVersion const & applicationRolloutVersion,
    wstring const & deploymentRoot,
    Client::IpcHealthReportingClientSPtr & ipcHealthReportingClient,
    FabricNodeContextSPtr & nodeContext,
    bool isContainerHost)
    : handlersLock_(),
    dataLock_(),
    codeChangeHandlers_(),
    configChangeHandlers_(),
    dataChangeHandlers_(),
    sourcePointers_(),
    nextHandlerId_(0),
    runLayout_(deploymentRoot),
    codePackageInstanceIdentifier_(codePackageInstanceId),
    applicationRolloutVersion_(applicationRolloutVersion.ToString()),
    codePackageVersion_(),
    applicationId_(codePackageInstanceIdentifier_.ServicePackageInstanceId.ApplicationId.ToString()),
    applicationName_(),
    applicationTypeName_(),
    contextId_(codePackageInstanceIdentifier_.ServicePackageInstanceId.ToString()),
    workingDirectory_(runLayout_.GetApplicationWorkFolder(applicationId_)),
    logDirectory_(runLayout_.GetApplicationLogFolder(applicationId_)),
    tempDirectory_(runLayout_.GetApplicationTempFolder(applicationId_)),
    heap_(),
    endpointResourceList_(),
    serviceTypeList_(),
    serviceGroupTypeList_(),
    applicationPrincipals_(),
    servicePackage_(make_shared<ServicePackageDescription>()),
    serviceManifest_(make_shared<ServiceManifestDescription>()),
    configSettingsMap_(),
    ipcHealthReportingClient_(ipcHealthReportingClient),
    nodeContext_(move(nodeContext)),
    isContainerHost_(isContainerHost),
    serviceListenAddress_(),
    servicePublishAddress_()
{
    auto workDirectoriesUPtr = make_unique<WorkDirectories>(*this);
    this->workDirectoriesUPtr_ = move(workDirectoriesUPtr);

    WriteNoise(
        TraceType,
        contextId_,
        "CodePackageActivationContext Ctor: DeploymentRoot=[{0}], Context=[{1}]",
        runLayout_.Root,
        codePackageInstanceId);
}

ErrorCode CodePackageActivationContext::EnsureData()
{
    AcquireWriteLock lock(dataLock_);
    wstring containerHost;
    ApplicationPackageDescription applicationPackageDescription;
    auto error = ReadData(applicationPackageDescription, *servicePackage_, *serviceManifest_, configSettingsMap_);
    if (!error.IsSuccess()) { return error; }

    if (StringUtility::AreEqualCaseInsensitive(this->CodePackageName, Constants::ImplicitTypeHostCodePackageName)||
        StringUtility::AreEqualCaseInsensitive(this->CodePackageName, Constants::BlockStoreServiceCodePackageName))
    {
        codePackageVersion_ = servicePackage_->ManifestVersion;
    }
    else
    {
        for (auto iter = servicePackage_->DigestedCodePackages.begin();
            iter != servicePackage_->DigestedCodePackages.end();
            ++iter)
        {
            if (iter->CodePackage.Name == this->CodePackageName)
            {
                codePackageVersion_ = iter->CodePackage.Version;
            }
        }
    }

    ASSERT_IF(codePackageVersion_.empty(), "CodePackage {0} not found", CodePackageName);

    if (this->isContainerHost_)
    {
        wstring networkingMode;
        bool envVarFound = Environment::GetEnvironmentVariable(Common::ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable, networkingMode, Common::NOTHROW());
        ASSERT_IF(!envVarFound, "Networking mode not found for container");

        WriteInfo(TraceType, contextId_, "NetworkingMode for container : {0}", networkingMode);

        // Networking mode can indicate multiple networks, so need to do a contains check.
        bool hasOpen = StringUtility::ContainsCaseInsensitive(networkingMode, NetworkType::OpenStr);
        bool hasIsolated = StringUtility::ContainsCaseInsensitive(networkingMode, NetworkType::IsolatedStr);
        bool hasOther = StringUtility::ContainsCaseInsensitive(networkingMode, NetworkType::OtherStr);

        ASSERT_IF(serviceManifest_->Resources.Endpoints.size() < 1, "Atleast one endpoint must be specified in serviceManifest_->Resources.Endpoints");

        if (hasIsolated)
        {
            // Use the first isolated network
            auto endpointIsolatedNetworkMap = servicePackage_->GetEndpointNetworkMap(NetworkType::IsolatedStr);
            for (auto iter = serviceManifest_->Resources.Endpoints.begin(); iter != serviceManifest_->Resources.Endpoints.end(); ++iter)
            {
                auto networkFromEndpoint = endpointIsolatedNetworkMap.find(iter->Name);
                if (networkFromEndpoint != endpointIsolatedNetworkMap.end())
                {
                    serviceListenAddress_ = iter->IpAddressOrFqdn;
                    servicePublishAddress_ = iter->IpAddressOrFqdn;
                }
            }
        }
        else if (hasOpen)
        {
            serviceListenAddress_ = serviceManifest_->Resources.Endpoints.begin()->IpAddressOrFqdn;
            servicePublishAddress_ = serviceManifest_->Resources.Endpoints.begin()->IpAddressOrFqdn;
        }
        else if (hasOther)
        {
            // Use NAT
            map<wstring, vector<Common::IPPrefix>> ipAddressPerAdapter;
            error = IpUtility::GetIpAddressesPerAdapter(ipAddressPerAdapter);

            ASSERT_IF(!error.IsSuccess(), "Getting container IP failed - {0}", error);
            ASSERT_IF(ipAddressPerAdapter.size() > 2, "Found more than two adapters in container");

            wstring localIp = L"";
            error = IpUtility::GetFirstNonLoopbackAddress(ipAddressPerAdapter, localIp);

            ASSERT_IF(!error.IsSuccess(), "Getting container IP failed - {0}", error);
            ASSERT_IF(localIp.empty(), "By this time we should have an IP.");

            serviceListenAddress_ = localIp;
            servicePublishAddress_ = this->nodeContext_->IPAddressOrFQDN;
        }
        else
        {
            // This is not a valid situation to be in
            ASSERT_IFNOT(
                hasIsolated || hasOpen || hasOther,
                "CodePackageActivationContext::EnsureData has reached an invalid state. Network is neither Isolated, Open, nor Other. networkingMode={0}",
                networkingMode);
        }
    }
    else
    {
        serviceListenAddress_ = this->nodeContext_->IPAddressOrFQDN;
        servicePublishAddress_ = this->nodeContext_->IPAddressOrFQDN;
    }

    WriteInfo(TraceType, contextId_, "ServiceListenAddress is {0} and ServicePublishAddress is {1}", serviceListenAddress_, servicePublishAddress_);

    applicationTypeName_ = applicationPackageDescription.ApplicationTypeName;
    applicationName_ = applicationPackageDescription.ApplicationName;

    endpointResourceList_ = heap_.AddItem<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST>();
    EndpointDescription::ToPublicApi(heap_, serviceManifest_->Resources.Endpoints, *endpointResourceList_);

    serviceTypeList_ = heap_.AddItem<FABRIC_SERVICE_TYPE_DESCRIPTION_LIST>();
    ServiceTypeDescription::ToPublicApi(heap_, serviceManifest_->ServiceTypeNames, *serviceTypeList_);

    serviceGroupTypeList_ = heap_.AddItem<FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST>();
    ServiceGroupTypeDescription::ToPublicApi(heap_, serviceManifest_->ServiceGroupTypes, *serviceGroupTypeList_);

    map<wstring, wstring> sids;
    wstring sidsFileName = runLayout_.GetPrincipalSIDsFile(applicationId_);
    bool success = PrincipalsDescription::ReadSIDsFromFile(sidsFileName, sids);
    if (!success)
    {
        WriteWarning(
            TraceType,
            contextId_,
            "Error Reading SID's from file:{0}",
            sidsFileName);
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    applicationPrincipals_ = heap_.AddItem<FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION>();
    applicationPackageDescription.DigestedEnvironment.Principals.ToPublicApi(heap_, sids, *applicationPrincipals_);

    if (nodeContext_)
    {
        auto const & logicalApplicationDirectories = nodeContext_->get_LogicalApplicationDirectories();

        std::wstring directoryName;
        std::wstring directoryPath;

        for (auto iter = logicalApplicationDirectories.begin(); iter != logicalApplicationDirectories.end(); ++iter)
        {
            directoryName = iter->first;
            directoryPath = Path::Combine(workingDirectory_, directoryName);

            if (StringUtility::AreEqualCaseInsensitive(applicationId_, ApplicationIdentifier::FabricSystemAppId->ToString()))
            {
                //If CodePackageActivationContext is for Fabric we don't setup the directories
                //inside work folder. So, we will return the work directory.
                workDirectoriesUPtr_->Add(directoryName, workingDirectory_);
                continue;
            }

            workDirectoriesUPtr_->Add(directoryName, directoryPath);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode CodePackageActivationContext::ReadData(
    __inout ApplicationPackageDescription & applicationPackageDescriptionResult,
    __inout ServicePackageDescription & servicePackageResult,
    __inout ServiceManifestDescription & serviceManifestResult,
    __inout ConfigSettingsMap & configSettingsMapResult)
{
    wstring applicationPackageFileName = runLayout_.GetApplicationPackageFile(
        applicationId_,
        applicationRolloutVersion_);

    ErrorCode error = Parser::ParseApplicationPackage(applicationPackageFileName, applicationPackageDescriptionResult);
    if (!error.IsSuccess()) { return error; }

    WriteNoise(
        TraceType,
        contextId_,
        "ParseApplicationPackage {0} at {1}",
        applicationPackageDescriptionResult,
        applicationPackageFileName);

    return ReadData(servicePackageResult, serviceManifestResult, configSettingsMapResult);
}

ErrorCode CodePackageActivationContext::ReadData(
    __inout ServicePackageDescription & servicePackageResult,
    __inout ServiceManifestDescription & serviceManifestResult,
    __inout ConfigSettingsMap & configSettingsMapResult)
{

    wstring currentServicePackageFileName = runLayout_.GetCurrentServicePackageFile(
        applicationId_,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.PublicActivationId);

    ErrorCode error = Parser::ParseServicePackage(currentServicePackageFileName, servicePackageResult);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        contextId_,
        "ParseServicePackage from {0}. ErrorCode={1}",
        currentServicePackageFileName,
        error);
    if (!error.IsSuccess()) { return error; }


    wstring manifestFilePath = runLayout_.GetServiceManifestFile(
        applicationId_,
        servicePackageResult.ManifestName,
        servicePackageResult.ManifestVersion);

    error = Parser::ParseServiceManifest(manifestFilePath, serviceManifestResult);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        contextId_,
        "ParseServiceManifest from {0}. ErrorCode={1}",
        manifestFilePath,
        error);
    if (!error.IsSuccess()) { return error; }

    // update the endpoint information from the endpoints file
    wstring endpointDescriptionsFile = runLayout_.GetEndpointDescriptionsFile(
        applicationId_,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.PublicActivationId);


    serviceManifestResult.Resources.Endpoints.clear();
    bool success = EndpointDescription::ReadFromFile(endpointDescriptionsFile, serviceManifestResult.Resources.Endpoints);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        contextId_,
        "ParseEndpoints from {0}. Success? = {1}",
        endpointDescriptionsFile,
        success);
    if (!success) { return ErrorCode(ErrorCodeValue::OperationFailed); }

    auto digestedEndpoints = servicePackage_->DigestedResources.DigestedEndpoints;
    for (auto epIter = serviceManifestResult.Resources.Endpoints.begin();
        epIter != serviceManifestResult.Resources.Endpoints.end();
        ++epIter)
    {
        auto dEpIter = digestedEndpoints.find(epIter->Name);
        if (dEpIter != digestedEndpoints.end())
        {
            dEpIter->second.Endpoint = *epIter;
        }
    }

    // load settings for the configuration package
    for (auto iter = servicePackageResult.DigestedConfigPackages.begin();
        iter != servicePackageResult.DigestedConfigPackages.end();
        ++iter)
    {
        wstring configPackageFolder = runLayout_.GetConfigPackageFolder(
            applicationId_,
            codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
            iter->ConfigPackage.Name,
            iter->ConfigPackage.Version,
            iter->IsShared);

        wstring settingsFile = runLayout_.GetSettingsFile(configPackageFolder);

        ConfigSettings configSettings;
        error = Parser::ParseConfigSettings(settingsFile, configSettings);
        if (!error.IsSuccess()) { return error; }

        if (!iter->ConfigOverrides.ConfigPackageName.empty())
        {
            ASSERT_IFNOT(
                StringUtility::AreEqualCaseInsensitive(iter->ConfigPackage.Name, iter->ConfigOverrides.ConfigPackageName),
                "Error in DigestedConfigPackage element. The ConfigPackage Name '{0}' does not match the ConfigOverrides Name '{1}'.",
                iter->ConfigPackage.Name,
                iter->ConfigOverrides.ConfigPackageName);

            configSettings.ApplyOverrides(iter->ConfigOverrides.SettingsOverride);
        }

        configSettingsMapResult[iter->ConfigPackage.Name] = move(configSettings);
    }

    return ErrorCode::Success();
}

void CodePackageActivationContext::GetCodePackageNames(StringCollection & names)
{
    AcquireReadLock lock(dataLock_);
    for (auto iter = serviceManifest_->CodePackages.begin();
        iter != serviceManifest_->CodePackages.end();
        ++iter)
    {
        CodePackageDescription const& codePackageDescription = *iter;
        names.push_back(codePackageDescription.Name);
    }
}

void CodePackageActivationContext::GetConfigurationPackageNames(StringCollection & names)
{
    AcquireReadLock lock(dataLock_);
    for (auto iter = serviceManifest_->ConfigPackages.begin();
        iter != serviceManifest_->ConfigPackages.end();
        ++iter)
    {
        ConfigPackageDescription const& configPackageDescription = *iter;
        names.push_back(configPackageDescription.Name);
    }
}

void CodePackageActivationContext::GetDataPackageNames(StringCollection & names)
{
    AcquireReadLock lock(dataLock_);
    for (auto iter = serviceManifest_->DataPackages.begin();
        iter != serviceManifest_->DataPackages.end();
        ++iter)
    {
        DataPackageDescription const& dataPackageDescription = *iter;
        names.push_back(dataPackageDescription.Name);
    }
}

void CodePackageActivationContext::GetServiceManifestName(wstring & serviceManifestName)
{
    AcquireReadLock lock(dataLock_);
    serviceManifestName = serviceManifest_->Name;
}

void CodePackageActivationContext::GetServiceManifestVersion(wstring & serviceManifestVersion)
{
    AcquireReadLock lock(dataLock_);
    serviceManifestVersion = serviceManifest_->Version;
}

ErrorCode CodePackageActivationContext::GetCodePackage(wstring const& codePackageName, ComPointer<IFabricCodePackage> & codePackage)
{
    {
        AcquireReadLock lock(dataLock_);
        for (auto iter = servicePackage_->DigestedCodePackages.begin();
            iter != servicePackage_->DigestedCodePackages.end();
            ++iter)
        {
            DigestedCodePackageDescription const& digestedCodePackageDescription = *iter;
            if (StringUtility::AreEqualCaseInsensitive(codePackageName, digestedCodePackageDescription.Name))
            {
                codePackage = CreateComPackage<IFabricCodePackage, DigestedCodePackageDescription>(
                    *iter,
                    serviceManifest_->Version,
                    configSettingsMap_);

                return ErrorCode::Success();
            }
        }
    }

    return ErrorCode(ErrorCodeValue::CodePackageNotFound);
}

ErrorCode CodePackageActivationContext::GetConfigurationPackage(wstring const& configPackageName, ComPointer<IFabricConfigurationPackage> & configPackage)
{
    {
        AcquireReadLock lock(dataLock_);
        for (auto iter = servicePackage_->DigestedConfigPackages.begin();
            iter != servicePackage_->DigestedConfigPackages.end();
            ++iter)
        {
            DigestedConfigPackageDescription const& digestedConfigPackageDescription = *iter;
            if (StringUtility::AreEqualCaseInsensitive(configPackageName, digestedConfigPackageDescription.Name))
            {
                configPackage = CreateComPackage<IFabricConfigurationPackage, DigestedConfigPackageDescription>(
                    digestedConfigPackageDescription,
                    serviceManifest_->Version,
                    configSettingsMap_);
                return ErrorCode::Success();
            }
        }
    }

    return ErrorCode(ErrorCodeValue::ConfigurationPackageNotFound);
}

ErrorCode CodePackageActivationContext::GetDataPackage(wstring const& dataPackageName, ComPointer<IFabricDataPackage> & dataPackage)
{
    {
        AcquireReadLock lock(dataLock_);
        for (auto iter = servicePackage_->DigestedDataPackages.begin();
            iter != servicePackage_->DigestedDataPackages.end();
            ++iter)
        {
            DigestedDataPackageDescription const& digestedDataPackageDescription = *iter;
            if (StringUtility::AreEqualCaseInsensitive(dataPackageName, digestedDataPackageDescription.Name))
            {
                dataPackage = CreateComPackage<IFabricDataPackage, DigestedDataPackageDescription>(
                    digestedDataPackageDescription,
                    serviceManifest_->Version,
                    configSettingsMap_);
                return ErrorCode::Success();
            }
        }
    }

    return ErrorCode(ErrorCodeValue::DataPackageNotFound);
}

ErrorCode CodePackageActivationContext::GetServiceManifestDescription(
    __out std::shared_ptr<ServiceModel::ServiceManifestDescription> & serviceManifest)
{
    {
        AcquireReadLock lock(dataLock_);
        serviceManifest = make_shared<ServiceModel::ServiceManifestDescription>(*serviceManifest_);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode CodePackageActivationContext::ReportApplicationHealth(
    ServiceModel::HealthInformation && healthInfoObj,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    auto healthReport = HealthReport::GenerateApplicationHealthReport(move(healthInfoObj), applicationName_);
    ASSERT_IFNOT(ipcHealthReportingClient_, "IpcHealthReporting client cannot be null");
    return ipcHealthReportingClient_->AddReport(move(healthReport), move(sendOptions));
}

Common::ErrorCode CodePackageActivationContext::ReportDeployedApplicationHealth(
    ServiceModel::HealthInformation && healthInfoObj,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    LargeInteger nodeId;
    ASSERT_IF(!LargeInteger::TryParse(nodeContext_->NodeId, nodeId), "Could not parse node id");
    auto healthReport = HealthReport::GenerateDeployedApplicationHealthReport(move(healthInfoObj), applicationName_, nodeId, nodeContext_->NodeName);
    ASSERT_IFNOT(ipcHealthReportingClient_, "IpcHealthReporting client cannot be null");
    return ipcHealthReportingClient_->AddReport(move(healthReport), move(sendOptions));
}

Common::ErrorCode CodePackageActivationContext::ReportDeployedServicePackageHealth(
    ServiceModel::HealthInformation && healthInfoObj,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    LargeInteger nodeId;
    ASSERT_IF(!LargeInteger::TryParse(nodeContext_->NodeId, nodeId), "Could not parse node id");
    auto healthReport = HealthReport::GenerateDeployedServicePackageHealthReport(
        move(healthInfoObj),
        applicationName_,
        serviceManifest_->Name,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.PublicActivationId,
        nodeId,
        nodeContext_->NodeName);
    ASSERT_IFNOT(ipcHealthReportingClient_, "IpcHealthReporting client cannot be null");
    return ipcHealthReportingClient_->AddReport(move(healthReport), move(sendOptions));
}

Common::ErrorCode CodePackageActivationContext::GetDirectory(
    std::wstring const& logicalDirectoryName,
    __out std::wstring & directoryPath)
{
    return workDirectoriesUPtr_->Get(logicalDirectoryName, directoryPath);
}

void CodePackageActivationContext::RegisterITentativeCodePackageActivationContext(
    std::wstring const& sourceId,
    ITentativeCodePackageActivationContext const & source)
{
    AcquireWriteLock lock(handlersLock_);
    auto iter = sourcePointers_.find(sourceId);
    ASSERT_IFNOT(iter == sourcePointers_.end(), "Source {0} already exists", sourceId);
    ReferencePointer<ITentativeCodePackageActivationContext> sourcePointer = ReferencePointer<ITentativeCodePackageActivationContext>(const_cast<ITentativeCodePackageActivationContext *>(&source));
    sourcePointers_.insert(make_pair(sourceId, sourcePointer));
}

void CodePackageActivationContext::UnregisterITentativeCodePackageActivationContext(
    std::wstring const& sourceId)
{
    AcquireWriteLock lock(handlersLock_);
    auto iter = sourcePointers_.find(sourceId);
    ASSERT_IF(iter == sourcePointers_.end(), "Source {0} does not exist", sourceId);
    sourcePointers_.erase(iter);
}

LONGLONG CodePackageActivationContext::RegisterCodePackageChangeHandler(
    ComPointer<IFabricCodePackageChangeHandler> const & handlerCPtr,
    wstring const & sourceId)
{
    LONGLONG handleId;

    {
        AcquireWriteLock lock(handlersLock_);

        handleId = nextHandlerId_++;

        codeChangeHandlers_.push_back(make_shared<PackageChangeHandler<IFabricCodePackageChangeHandler>>(
            handleId,
            handlerCPtr,
            sourceId));
    }

    return handleId;
}

void CodePackageActivationContext::UnregisterCodePackageChangeHandler(LONGLONG handlerId)
{
    AcquireWriteLock lock(handlersLock_);

    codeChangeHandlers_.erase(remove_if(codeChangeHandlers_.begin(), codeChangeHandlers_.end(),
        [handlerId](shared_ptr<PackageChangeHandler<IFabricCodePackageChangeHandler>> const & handler)
    {
        return (handler->Id == handlerId);
    }),
        codeChangeHandlers_.end());
}

LONGLONG CodePackageActivationContext::RegisterConfigurationPackageChangeHandler(
    ComPointer<IFabricConfigurationPackageChangeHandler> const & handlerCPtr,
    wstring const & sourceId)
{
    LONGLONG handleId;

    {
        AcquireWriteLock lock(handlersLock_);

        handleId = nextHandlerId_++;

        configChangeHandlers_.push_back(make_shared<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>>(
            handleId,
            handlerCPtr,
            sourceId));
    }

    return handleId;
}

void CodePackageActivationContext::UnregisterConfigurationPackageChangeHandler(LONGLONG handlerId)
{
    AcquireWriteLock lock(handlersLock_);

    configChangeHandlers_.erase(remove_if(configChangeHandlers_.begin(), configChangeHandlers_.end(),
        [handlerId](shared_ptr<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>> const & handler)
    {
        return (handler->Id == handlerId);
    }),
        configChangeHandlers_.end());
}

LONGLONG CodePackageActivationContext::RegisterDataPackageChangeHandler(
    ComPointer<IFabricDataPackageChangeHandler> const & handlerCPtr,
    wstring const & sourceId)
{
    LONGLONG handleId;

    {
        AcquireWriteLock lock(handlersLock_);

        handleId = nextHandlerId_++;

        dataChangeHandlers_.push_back(make_shared<PackageChangeHandler<IFabricDataPackageChangeHandler>>(
            handleId,
            handlerCPtr,
            sourceId));
    }

    return handleId;
}

void CodePackageActivationContext::UnregisterDataPackageChangeHandler(LONGLONG handlerId)
{
    AcquireWriteLock lock(handlersLock_);

    dataChangeHandlers_.erase(remove_if(dataChangeHandlers_.begin(), dataChangeHandlers_.end(),
        [handlerId](shared_ptr<PackageChangeHandler<IFabricDataPackageChangeHandler>> const & handler)
    {
        return (handler->Id == handlerId);
    }),
        dataChangeHandlers_.end());
}

void CodePackageActivationContext::OnServicePackageChanged()
{
    auto updatedServiceManifest = make_shared<ServiceManifestDescription>();
    auto updatedServicePackage = make_shared<ServicePackageDescription>();
    ConfigSettingsMap updatedConfigSettingsMap;
    auto error = ReadData(*updatedServicePackage, *updatedServiceManifest, updatedConfigSettingsMap);
    if (error.IsSuccess())
    {
        vector<PackageChangeDescription<IFabricCodePackage>> changedCodePackages;
        vector<PackageChangeDescription<IFabricConfigurationPackage>> changedConfigPackages;
        vector<PackageChangeDescription<IFabricDataPackage>> changedDataPackages;
        {
            AcquireWriteLock lock(dataLock_);
            changedCodePackages = ComputePackageChanges<IFabricCodePackage, DigestedCodePackageDescription>(
                servicePackage_->DigestedCodePackages,
                updatedServicePackage->DigestedCodePackages,
                configSettingsMap_,
                updatedConfigSettingsMap,
                serviceManifest_->Version,
                updatedServiceManifest->Version);

            changedConfigPackages = ComputePackageChanges<IFabricConfigurationPackage, DigestedConfigPackageDescription>(
                servicePackage_->DigestedConfigPackages,
                updatedServicePackage->DigestedConfigPackages,
                configSettingsMap_,
                updatedConfigSettingsMap,
                serviceManifest_->Version,
                updatedServiceManifest->Version);

            changedDataPackages = ComputePackageChanges<IFabricDataPackage, DigestedDataPackageDescription>(
                servicePackage_->DigestedDataPackages,
                updatedServicePackage->DigestedDataPackages,
                configSettingsMap_,
                updatedConfigSettingsMap,
                serviceManifest_->Version,
                updatedServiceManifest->Version);

            // update current data members after computing the changes
            serviceManifest_.swap(updatedServiceManifest);
            servicePackage_.swap(updatedServicePackage);
            configSettingsMap_.swap(updatedConfigSettingsMap);
        }

        this->NotifyAllPackageChangeHandlers(changedCodePackages, changedConfigPackages, changedDataPackages);
    }
    else
    {
        WriteError(
            TraceType,
            contextId_,
            "Could not read new manifest because of error {0}. Killing process",
            error);
        ExitProcess(ErrorCodeValue::UpgradeFailed & 0x0000FFFF);
    }
}

void CodePackageActivationContext::NotifyAllPackageChangeHandlers(
    vector<PackageChangeDescription<IFabricCodePackage>> const & changedCodePackages,
    vector<PackageChangeDescription<IFabricConfigurationPackage>> const & changedConfigPackages,
    vector<PackageChangeDescription<IFabricDataPackage>> const & changedDataPackages)
{
    NotifyChangeHandlers<IFabricCodePackage, IFabricCodePackageChangeHandler>(changedCodePackages);
    NotifyChangeHandlers<IFabricConfigurationPackage, IFabricConfigurationPackageChangeHandler>(changedConfigPackages);
    NotifyChangeHandlers<IFabricDataPackage, IFabricDataPackageChangeHandler>(changedDataPackages);
}

template <>
Common::ComPointer<IFabricCodePackage> CodePackageActivationContext::CreateComPackage(
    ServiceModel::DigestedCodePackageDescription const & packageDescription,
    std::wstring const& serviceManifestVersion,
    ConfigSettingsMap const & configSettingsMap)
{
    UNREFERENCED_PARAMETER(configSettingsMap);

    std::wstring packageFolder;
    packageFolder = runLayout_.GetCodePackageFolder(
        applicationId_,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
        packageDescription.CodePackage.Name,
        packageDescription.CodePackage.Version,
        packageDescription.IsShared);

    return Common::make_com<Hosting2::ComCodePackage, IFabricCodePackage>(
        packageDescription.CodePackage,
        ServicePackageName,
        serviceManifestVersion,
        packageFolder,
        packageDescription.SetupRunAsPolicy,
        packageDescription.RunAsPolicy);
}

template <>
Common::ComPointer<IFabricConfigurationPackage> CodePackageActivationContext::CreateComPackage(
    ServiceModel::DigestedConfigPackageDescription const & packageDescription,
    std::wstring const& serviceManifestVersion,
    ConfigSettingsMap const & configSettingsMap)
{
    std::wstring packageFolder;

    packageFolder = runLayout_.GetConfigPackageFolder(
        applicationId_,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
        packageDescription.ConfigPackage.Name,
        packageDescription.ConfigPackage.Version,
        packageDescription.IsShared);

    Common::ConfigSettings configSettings;
    auto iter = configSettingsMap.find(packageDescription.ConfigPackage.Name);
    if (iter != configSettingsMap.end())
    {
        configSettings = iter->second;
    }
    else
    {
        WriteWarning(
            "CodePackageActivationContext",
            contextId_,
            "config settings not found for config package {0}:{1}",
            packageDescription.ConfigPackage.Name,
            packageDescription.ConfigPackage.Version);
    }

    return Common::make_com<ComConfigPackage, IFabricConfigurationPackage>(
        packageDescription.ConfigPackage,
        ServicePackageName,
        serviceManifestVersion,
        packageFolder,
        configSettings);
}

template <>
Common::ComPointer<IFabricDataPackage> CodePackageActivationContext::CreateComPackage(
    ServiceModel::DigestedDataPackageDescription const & packageDescription,
    std::wstring const& serviceManifestVersion,
    ConfigSettingsMap const & configSettingsMap)
{
    UNREFERENCED_PARAMETER(configSettingsMap);

    std::wstring packageFolder;

    packageFolder = runLayout_.GetDataPackageFolder(
        applicationId_,
        codePackageInstanceIdentifier_.ServicePackageInstanceId.ServicePackageName,
        packageDescription.DataPackage.Name,
        packageDescription.DataPackage.Version,
        packageDescription.IsShared);

    return Common::make_com<ComDataPackage, IFabricDataPackage>(
        packageDescription.DataPackage,
        ServicePackageName,
        serviceManifestVersion,
        packageFolder);
}

template <>
std::vector<std::shared_ptr<PackageChangeHandler<IFabricCodePackageChangeHandler>>> CodePackageActivationContext::CopyHandlers()
{
    std::vector<std::shared_ptr<PackageChangeHandler<IFabricCodePackageChangeHandler>>> snap;

    {
        Common::AcquireReadLock lock(handlersLock_);
        snap = codeChangeHandlers_;
    }

    return std::move(snap);
}

template <>
std::vector<std::shared_ptr<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>>> CodePackageActivationContext::CopyHandlers()
{
    std::vector<std::shared_ptr<PackageChangeHandler<IFabricConfigurationPackageChangeHandler>>> snap;

    {
        Common::AcquireReadLock lock(handlersLock_);
        snap = configChangeHandlers_;
    }

    return std::move(snap);
}

template <>
std::vector<std::shared_ptr<PackageChangeHandler<IFabricDataPackageChangeHandler>>> CodePackageActivationContext::CopyHandlers()
{
    std::vector<std::shared_ptr<PackageChangeHandler<IFabricDataPackageChangeHandler>>> snap;

    {
        Common::AcquireReadLock lock(handlersLock_);
        snap = dataChangeHandlers_;
    }

    return std::move(snap);
}
