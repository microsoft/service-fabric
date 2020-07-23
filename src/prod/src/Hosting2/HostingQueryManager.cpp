// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Query;

StringLiteral const TraceType("HostingQueryManager");

class HostingQueryManager::ProcessQueryAsyncOperation : public TimedAsyncOperation
{
public:
    ProcessQueryAsyncOperation(
        __in HostingQueryManager &owner,
        Query::QueryNames::Enum queryName,
        ServiceModel::QueryArgumentMap const & queryArgs,
        Common::ActivityId const & activityId,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
        , owner_(owner)
    {
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & asyncOperation,
        __out Transport::MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessQueryAsyncOperation>(asyncOperation);
        reply = move(casted->reply_);
        return casted->Error;
    }

protected:
    void OnStart(Common::AsyncOperationSPtr const& thisSPtr)
    {
        auto jobItem = QueryJobItem(
            [thisSPtr, this](HostingQueryManager &) { this->ProcessQuery(thisSPtr); });
        bool success = owner_.queryJobQueue_->Enqueue(move(jobItem));
        if (!success)
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to enqueue QueryRequest to JobQueue. ActivityId: {0}", activityId_);
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }
    }

private:
    void ProcessQuery(Common::AsyncOperationSPtr const& thisSPtr)
    {
        ErrorCode error = ErrorCodeValue::InvalidConfiguration;
        bool completeAsyncOperation = true;
        QueryResult result;
        switch (queryName_)
        {
        case QueryNames::GetApplicationListDeployedOnNode:
            error = owner_.GetApplicationListDeployedOnNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetApplicationPagedListDeployedOnNode:
            error = owner_.GetApplicationPagedListDeployedOnNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetServiceManifestListDeployedOnNode:
            error = owner_.GetServiceManifestListDeployedOnNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetServiceTypeListDeployedOnNode:
            error = owner_.GetServiceTypeListDeployedOnNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetCodePackageListDeployedOnNode:
            error = owner_.GetCodePackageListDeployedOnNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetDeployedNetworkList:
            error = ProcessGetDeployedNetworkListAsync(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        case QueryNames::GetDeployedNetworkCodePackageList:
            error = ProcessGetDeployedNetworkCodePackageListAsync(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        case QueryNames::GetContainerInfoDeployedOnNode:
            error = ProcessGetContainerInfoAsync(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        case QueryNames::InvokeContainerApiOnNode:
            error = InvokeContainerApiAsync(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        case QueryNames::DeployServicePackageToNode:
            error = ProcessDeployServicePackageAsync(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        case QueryNames::RestartDeployedCodePackage:
            __fallthrough;
        case QueryNames::StopNode:
            __fallthrough;
        case QueryNames::AddUnreliableTransportBehavior:
            __fallthrough;
        case QueryNames::RemoveUnreliableTransportBehavior:
        {
            auto operation = BeginTestabilityOperation(thisSPtr,
                [=](AsyncOperationSPtr const& operation){
                    OnTestabilityOperationComplete(operation, false);
                });
            OnTestabilityOperationComplete(operation, true);
            completeAsyncOperation = false;
            break;
        }
        default:
            error = ErrorCodeValue::InvalidConfiguration;
        }

        if (completeAsyncOperation)
        {
            reply_ = make_unique<Message>(result);
            TryComplete(thisSPtr, error);
        }
    }

    AsyncOperationSPtr BeginTestabilityOperation(Common::AsyncOperationSPtr const& thisSPtr,
        AsyncCallback const & callback)
    {
        ASSERT_IF(owner_.legacyTestabilityRequestHandlerSPtr_ == nullptr, "legacyTestabilityRequestHandlerSPtr_ on hostingSubsystem not available");
        auto operation = owner_.legacyTestabilityRequestHandlerSPtr_->BeginTestabilityOperation(
            queryName_, queryArgs_, activityId_, this->get_RemainingTime(),
            [=](Common::AsyncOperationSPtr const& operation){
                callback(operation);
            },
            thisSPtr);
        return operation;
    }

    void OnTestabilityOperationComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ASSERT_IF(owner_.legacyTestabilityRequestHandlerSPtr_ == nullptr, "legacyTestabilityRequestHandlerSPtr_ on hostingSubsystem not available");
        ErrorCode error = owner_.legacyTestabilityRequestHandlerSPtr_->EndTestabilityOperation(operation, reply_);
        TryComplete(operation->Parent, error);
    }

    ErrorCode ProcessDeployServicePackageAsync(AsyncOperationSPtr const & thisSPtr)
    {
        wstring applicationTypeNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring applicationTypeVersionArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, applicationTypeVersionArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }
        wstring serviceManifestNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }
        wstring sharingPolicy;
        queryArgs_.TryGetValue(QueryResourceProperties::ServiceType::SharedPackages, sharingPolicy);

        PackageSharingPolicyList sharingPolicies;
        auto error = JsonHelper::Deserialize<PackageSharingPolicyList>(sharingPolicies, sharingPolicy);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "Deserializing PackageSharingPolicyList returned error {0}",
            error);
        if (!error.IsSuccess())
        {
            return error;
        }

        auto operation = owner_.hosting_.DownloadManagerObj->BeginDownloadServiceManifestPackages(
            applicationTypeNameArgument,
            applicationTypeVersionArgument,
            serviceManifestNameArgument,
            sharingPolicies,
            [this](AsyncOperationSPtr const & operation){
                OnServiceManifestPackageDownloaded(operation, false);},
            thisSPtr);
        OnServiceManifestPackageDownloaded(operation, true);
        return ErrorCode();
    }

    void OnServiceManifestPackageDownloaded(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.DownloadManagerObj->EndDownloadServiceManifestPackages(operation);
        QueryResult result;
        reply_ = make_unique<Message>(result);
        TryComplete(operation->Parent, error);
    }

    ErrorCode ProcessGetContainerInfoAsync(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "{0}: GetContainerInfoAsync",
            activityId_);

        wstring applicationNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring serviceManifestNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ServiceManifest::ServiceManifestName, serviceManifestNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring codePackageNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::CodePackage::CodePackageName, codePackageNameArgument) ||
            codePackageNameArgument.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring containerInfoTypeArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ContainerInfo::InfoTypeFilter, containerInfoTypeArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring containerInfoArgsArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ContainerInfo::InfoArgsFilter, containerInfoArgsArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        QueryResult result;
        auto error = owner_.GetCodePackageListDeployedOnNode(queryArgs_, activityId_, result);
        if (!error.IsSuccess())
        {
            return error;
        }

        vector<DeployedCodePackageQueryResult> resultList;
        result.MoveList<DeployedCodePackageQueryResult>(resultList);
        int64 codePackageInstanceId;
        wstring servicePackageActivationIdArgument;
        if (!resultList.empty())
        {
            codePackageInstanceId = (*resultList.begin()).EntryPoint.InstanceId;
            servicePackageActivationIdArgument = (*resultList.begin()).ServicePackageActivationId;
            if (servicePackageActivationIdArgument.empty())
            {
                WriteNoise(
                    TraceType,
                    owner_.Root.TraceId,
                    "{0}: GetContainerInfo - ServicePackageActivationId not provided. Using default.",
                    activityId_);

                servicePackageActivationIdArgument = L"";
            }
        }
        else
        {
            return ErrorCodeValue::ContainerNotFound;
        }

        CodePackageSPtr matchingCodePackage;
        error = owner_.GetCodePackage(codePackageInstanceId,
            applicationNameArgument,
            serviceManifestNameArgument,
            servicePackageActivationIdArgument,
            codePackageNameArgument,
            activityId_,
            matchingCodePackage);

        if (!error.IsSuccess())
        {
            return ErrorCodeValue::ContainerNotFound;
        }

        if (matchingCodePackage)
        {
            auto asyncCallback = [this, matchingCodePackage](AsyncOperationSPtr const & operation)
            { OnGetContainerInfoCompleted(operation, false, matchingCodePackage); };

            auto operation = matchingCodePackage->BeginGetContainerInfo(
                codePackageInstanceId,
                containerInfoTypeArgument,
                containerInfoArgsArgument,
                this->get_RemainingTime(),
                asyncCallback,
                thisSPtr);
            OnGetContainerInfoCompleted(operation, true, matchingCodePackage);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "{0}: No code package found running currently or instanceId has changed",
                activityId_);
            return ErrorCode(ErrorCodeValue::ContainerNotFound);
        }

        return ErrorCode();
    }

    void OnGetContainerInfoCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, CodePackageSPtr matchingCodePackage)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring containerInfo;
        auto error = matchingCodePackage->EndGetContainerInfo(operation, containerInfo);
        QueryResult result(make_unique<wstring>(containerInfo));
        reply_ = make_unique<Message>(result);
        TryComplete(operation->Parent, error);
    }

    ErrorCode InvokeContainerApiAsync(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "{0}: InvokeContainerApiAsync",
            activityId_);

        wstring applicationNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring serviceManifestNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring codePackageNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::CodePackage::CodePackageName, codePackageNameArgument) ||
            codePackageNameArgument.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring containerInfoArgsArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ContainerInfo::InfoArgsFilter, containerInfoArgsArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        QueryResult result;
        auto error = owner_.GetCodePackageListDeployedOnNode(queryArgs_, activityId_, result);
        if (!error.IsSuccess())
        {
            return error;
        }

        vector<DeployedCodePackageQueryResult> resultList;
        result.MoveList<DeployedCodePackageQueryResult>(resultList);
        int64 codePackageInstanceId;
        wstring servicePackageActivationIdArgument;
        if (!resultList.empty())
        {
            codePackageInstanceId = (*resultList.begin()).EntryPoint.InstanceId;

            servicePackageActivationIdArgument = (*resultList.begin()).ServicePackageActivationId;
            if (servicePackageActivationIdArgument.empty())
            {
                WriteNoise(
                    TraceType,
                    owner_.Root.TraceId,
                    "{0}: InvokeContainerApiAsync - ServicePackageActivationId not provided. Using default.",
                    activityId_);

                servicePackageActivationIdArgument = L"";
            }
        }
        else
        {
            return ErrorCodeValue::ContainerNotFound;
        }

        CodePackageSPtr matchingCodePackage;
        error = owner_.GetCodePackage(codePackageInstanceId,
            applicationNameArgument,
            serviceManifestNameArgument,
            servicePackageActivationIdArgument,
            codePackageNameArgument,
            activityId_,
            matchingCodePackage);

        if (!error.IsSuccess())
        {
            return error;
        }

        if (matchingCodePackage)
        {
            auto asyncCallback = [this, matchingCodePackage](AsyncOperationSPtr const & operation)
            { OnGetContainerInfoCompleted(operation, false, matchingCodePackage); };

            auto operation = matchingCodePackage->BeginGetContainerInfo(
                codePackageInstanceId,
                StringUtility::ToWString(ContainerInfoType::Enum::RawAPI),
                containerInfoArgsArgument,
                this->get_RemainingTime(),
                asyncCallback,
                thisSPtr);
            OnContainerApiCompleted(operation, true, matchingCodePackage);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "{0}: No code package found running currently or instanceId has changed",
                activityId_);
            return ErrorCode(ErrorCodeValue::CodePackageNotFound);
        }

        return ErrorCode();
    }

    void OnContainerApiCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, CodePackageSPtr matchingCodePackage)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring apiResult;
        auto error = matchingCodePackage->EndGetContainerInfo(operation, apiResult);
        QueryResult result(make_unique<wstring>(apiResult));
        reply_ = make_unique<Message>(result);
        TryComplete(operation->Parent, error);
    }

    ErrorCode ProcessGetDeployedNetworkCodePackageListAsync(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "{0}: GetDeployedNetworkCodePackageList",
            activityId_);

        wstring applicationNameArgument;
        queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument);
        
        wstring serviceManifestNameArgument;
        queryArgs_.TryGetValue(QueryResourceProperties::ServiceManifest::ServiceManifestName, serviceManifestNameArgument);

        Application2SPtr applicationEntry;
        auto error = owner_.GetApplicationEntry(applicationNameArgument, activityId_, applicationEntry);
        if (!error.IsSuccess())
        {
            return error;
        }

        vector<ServicePackage2SPtr> servicePackages;
        error = owner_.GetServicePackages(applicationEntry, serviceManifestNameArgument, activityId_, servicePackages);
        if (!error.IsSuccess())
        {
            return error;
        }

        vector<wstring> servicePackageIds;
        for (auto const & servicePackage : servicePackages)
        {
            servicePackageIds.push_back(servicePackage->Id.ToString());
        }

        wstring codePackageNameArgument;
        queryArgs_.TryGetValue(QueryResourceProperties::CodePackage::CodePackageName, codePackageNameArgument);

        wstring networkNameArgument;
        queryArgs_.TryGetValue(QueryResourceProperties::Network::NetworkName, networkNameArgument);

        vector<VersionedServicePackageSPtr> versionedServicePackages;
        for (auto servicePackageIter = servicePackages.begin(); servicePackageIter != servicePackages.end(); ++servicePackageIter)
        {
            VersionedServicePackageSPtr versionedServicePackage = (*servicePackageIter)->GetVersionedServicePackage();
            if (!versionedServicePackage)
            {
                WriteInfo(
                    TraceType,
                    owner_.Root.TraceId,
                    "{0}: GetVersionedServicePackage for service package {1} failed",
                    activityId_,
                    serviceManifestNameArgument);
                continue;
            }

            versionedServicePackages.push_back(versionedServicePackage);
        }

        std::map<std::wstring, std::wstring> codePackageInstanceAppHostMap;
        std::map<std::wstring, CodePackageSPtr> codePackageInstanceCodePackageMap;
        for (auto versionedServicePackageIter = versionedServicePackages.begin(); versionedServicePackageIter != versionedServicePackages.end(); ++versionedServicePackageIter)
        {
            vector<CodePackageSPtr> codePackages = (*versionedServicePackageIter)->GetCodePackages(codePackageNameArgument);
            for (auto const & codePackage : codePackages)
            {
                auto codePackageInstanceId = codePackage->CodePackageInstanceId.ToString();
                codePackageInstanceCodePackageMap[codePackageInstanceId] = codePackage;

                ApplicationHostProxySPtr hostProxy;
                error = owner_.hosting_.ApplicationHostManagerObj->FindApplicationHost(codePackageInstanceId, hostProxy);
                if (error.IsSuccess())
                {
                    codePackageInstanceAppHostMap[codePackageInstanceId] = hostProxy->HostId;
                }
            }
        }

        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginGetNetworkDeployedPackages(
            servicePackageIds,
            codePackageNameArgument,
            networkNameArgument,
            owner_.hosting_.NodeId,
            codePackageInstanceAppHostMap,
            this->get_RemainingTime(),
            [this, codePackageInstanceCodePackageMap](AsyncOperationSPtr const & operation) {
            OnGetNetworkDeployedPackagesCompleted(operation, codePackageInstanceCodePackageMap, false); },
            thisSPtr);
        OnGetNetworkDeployedPackagesCompleted(operation, codePackageInstanceCodePackageMap, true);

        return ErrorCode();
    }

    void OnGetNetworkDeployedPackagesCompleted(
        AsyncOperationSPtr operation, 
        std::map<std::wstring, CodePackageSPtr> codePackageInstanceCodePackageMap, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> networkReservedCodePackages;
        std::map<std::wstring, std::map<std::wstring, std::wstring>> codePackageInstanceIdentifierContainerInfoMap;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndGetNetworkDeployedPackages(operation, networkReservedCodePackages, codePackageInstanceIdentifierContainerInfoMap);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(GetNetworkDeployedPackagesCompleted): ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
        }

        std::vector<DeployedNetworkCodePackageQueryResult> deployedNetworkCodePackageQueryResultList;
        for (auto const & network : networkReservedCodePackages)
        {
            auto networkName = network.first;
            for (auto const & service : network.second)
            {
                ServicePackageInstanceIdentifier servicePackageInstanceIdentifier;
                auto spError = ServicePackageInstanceIdentifier::FromString(service.first, __out servicePackageInstanceIdentifier);
                if (!spError.IsSuccess())
                {
                    continue;
                }

                for (auto const & cp : service.second)
                {
                    CodePackageInstanceIdentifier codePackageInstanceIdentifier(servicePackageInstanceIdentifier, cp);
                    wstring codePackageInstanceIdentifierStr = codePackageInstanceIdentifier.ToString();

                    wstring containerId = L"";
                    wstring containerAddress = L"";
                    auto cpInstanceIter = codePackageInstanceIdentifierContainerInfoMap.find(codePackageInstanceIdentifierStr);
                    if (cpInstanceIter != codePackageInstanceIdentifierContainerInfoMap.end())
                    {
                        if (!cpInstanceIter->second.empty())
                        {
                            auto containerInfoMap = cpInstanceIter->second.begin();
                            containerId = containerInfoMap->first;
                            containerAddress = containerInfoMap->second;
                        }
                    }

                    wstring applicationName = L"";
                    wstring codePackageName = L"";
                    wstring codePackageVersion = L"";
                    wstring serviceManifestName = L"";
                    wstring servicePackageActivationId = L"";
                    auto cpIter = codePackageInstanceCodePackageMap.find(codePackageInstanceIdentifierStr);
                    if (cpIter != codePackageInstanceCodePackageMap.end())
                    {
                        applicationName = cpIter->second->Context.ApplicationName;
                        codePackageName = cpIter->second->Description.CodePackage.Name;
                        codePackageVersion = cpIter->second->Description.CodePackage.Version;
                        serviceManifestName = cpIter->second->CodePackageInstanceId.ServicePackageInstanceId.ServicePackageName;
                        servicePackageActivationId = cpIter->second->CodePackageInstanceId.ServicePackageInstanceId.PublicActivationId;
                    }

                    DeployedNetworkCodePackageQueryResult deployedNetworkCodePackageQueryResult(
                        applicationName,
                        networkName,
                        codePackageName,
                        codePackageVersion,
                        serviceManifestName,
                        servicePackageActivationId,
                        containerId,
                        containerAddress);

                    deployedNetworkCodePackageQueryResultList.push_back(move(deployedNetworkCodePackageQueryResult));
                }
            }
        }

        wstring resultTraceString;
        for (auto iter = deployedNetworkCodePackageQueryResultList.begin(); iter != deployedNetworkCodePackageQueryResultList.end(); ++iter)
        {
            resultTraceString.append(wformatString("\r\n{0}", iter->ToString()));
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "GetNetworkDeployedPackagesDeployedOnNode[{0}]: Result:{1}",
            activityId_,
            resultTraceString);

        auto queryResult = QueryResult(move(deployedNetworkCodePackageQueryResultList));
        reply_ = make_unique<Message>(queryResult);
        TryComplete(operation->Parent, ErrorCode::Success());
    }

    ErrorCode ProcessGetDeployedNetworkListAsync(AsyncOperationSPtr const & thisSPtr)
    {
        wstring networkTypeArgument;
        queryArgs_.TryGetValue(QueryResourceProperties::Network::NetworkType, networkTypeArgument);

        NetworkType::Enum networkType;
        if (!networkTypeArgument.empty())
        {
            NetworkType::FromString(networkTypeArgument, __out networkType);
        }
        else
        {
            networkType = NetworkType::Enum::Open | NetworkType::Enum::Isolated;
        }

        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginGetDeployedNetworks(
            networkType,
            this->get_RemainingTime(),
            [this](AsyncOperationSPtr const & operation) {
            OnGetDeployedNetworkListCompleted(operation, false); },
            thisSPtr);
        OnGetDeployedNetworkListCompleted(operation, true);

        return ErrorCode();
    }

    void OnGetDeployedNetworkListCompleted(
        AsyncOperationSPtr operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        std::vector<std::wstring> networkNames;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndGetDeployedNetworks(operation, networkNames);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(GetDeployedNetworkListCompleted): ErrorCode={0}",
            error);

        std::vector<DeployedNetworkQueryResult> deployedNetworkQueryResultList;
        for (auto const & networkName : networkNames)
        {
            DeployedNetworkQueryResult result(networkName);
            deployedNetworkQueryResultList.push_back(move(result));
        }

        wstring resultTraceString;
        for (auto iter = deployedNetworkQueryResultList.begin(); iter != deployedNetworkQueryResultList.end(); ++iter)
        {
            resultTraceString.append(wformatString("\r\n{0}", iter->ToString()));
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "GetDeployedNetworkListDeployedOnNode[{0}]: Result:{1}",
            activityId_,
            resultTraceString);

        auto queryResult = QueryResult(move(deployedNetworkQueryResultList));
        reply_ = make_unique<Message>(queryResult);
        TryComplete(operation->Parent, ErrorCode::Success());
    }

    ServiceModel::QueryArgumentMap queryArgs_;
    Query::QueryNames::Enum queryName_;
    Common::ActivityId const & activityId_;
    HostingQueryManager & owner_;
    Transport::MessageUPtr reply_;
};

class HostingQueryManager::RestartDeployedCodePackageAsyncOperation : public TimedAsyncOperation
{
public:
    RestartDeployedCodePackageAsyncOperation(
        __in HostingQueryManager &owner,
        int64 codePackageInstanceId,
        wstring const & applicationNameArgument,
        wstring const & serviceManifestNameArgument,
        wstring const & servicePackageActivationId,
        wstring const & codePackageNameArgument,
        ActivityId const activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , codePackageInstanceId_(codePackageInstanceId)
        , applicationNameArgument_(applicationNameArgument)
        , serviceManifestNameArgument_(serviceManifestNameArgument)
        , servicePackageActivationId_(servicePackageActivationId)
        , codePackageNameArgument_(codePackageNameArgument)
        , activityId_(activityId)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & asyncOperation, __out MessageUPtr& reply)
    {
        auto casted = AsyncOperation::End<RestartDeployedCodePackageAsyncOperation>(asyncOperation);
        QueryResult result;
        reply = make_unique<Message>(result);
        return casted->Error;
    }

protected:
    void OnStart(Common::AsyncOperationSPtr const& thisSPtr)
    {
        auto error = this->HandleRestartDeployedCodePackageAsync(thisSPtr);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to enqueue QueryRequest to JobQueue. ActivityId: {0}", activityId_);
            TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }
    }

private:
    ErrorCode HandleRestartDeployedCodePackageAsync(AsyncOperationSPtr const& thisSPtr)
    {
        CodePackageSPtr matchingCodePackage;
        auto error = owner_.GetCodePackage(codePackageInstanceId_,
            applicationNameArgument_,
            serviceManifestNameArgument_,
            servicePackageActivationId_,
            codePackageNameArgument_,
            activityId_,
            matchingCodePackage);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (matchingCodePackage)
        {
            auto asyncCallback = [this, matchingCodePackage](AsyncOperationSPtr const & operation)
            { OnRestartCompleted(operation, false, matchingCodePackage); };

            auto operation = matchingCodePackage->BeginRestartCodePackageInstance(
                codePackageInstanceId_,
                this->get_RemainingTime(),
                asyncCallback,
                thisSPtr);
            OnRestartCompleted(operation, true, matchingCodePackage);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "{0}: No code package found running currently or instanceId has changed",
                activityId_);
            return ErrorCode(ErrorCodeValue::CodePackageNotFound);
        }

        return ErrorCode();
    }

    void OnRestartCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, CodePackageSPtr matchingCodePackage)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = matchingCodePackage->EndRestartCodePackageInstance(operation);
        TryComplete(operation->Parent, error);
    }

private:
    HostingQueryManager & owner_;
    int64 codePackageInstanceId_;
    wstring applicationNameArgument_;
    wstring serviceManifestNameArgument_;
    std::wstring servicePackageActivationId_;
    wstring codePackageNameArgument_;
    ActivityId const activityId_;
};

HostingQueryManager::HostingQueryManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root)
    , hosting_(hosting)
    , queryMessageHandler_(make_unique<QueryMessageHandler>(root, QueryAddresses::HostingAddressSegment))
{
}

HostingQueryManager::~HostingQueryManager()
{
}

ErrorCode HostingQueryManager::OnOpen()
{
    queryJobQueue_ = make_unique<QueryJobQueue>(
        this->Root.TraceId,
        *this,
        false,
        HostingConfig::GetConfig().MaxQueryOperationThreads);

    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
    {
        return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
    },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        return this->EndProcessQuery(operation, reply);
    });

    return queryMessageHandler_->Open();
}

Common::AsyncOperationSPtr HostingQueryManager::BeginProcessIncomingQueryMessage(
    Transport::Message & message,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return queryMessageHandler_->BeginProcessQueryMessage(message, timeout, callback, parent);
}

Common::ErrorCode HostingQueryManager::EndProcessIncomingQueryMessage(
    Common::AsyncOperationSPtr const & asyncOperation,
    _Out_ Transport::MessageUPtr & reply)
{
    return queryMessageHandler_->EndProcessQueryMessage(asyncOperation, reply);
}

Common::AsyncOperationSPtr HostingQueryManager::BeginProcessQuery(
    Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode HostingQueryManager::EndProcessQuery(
    Common::AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    return ProcessQueryAsyncOperation::End(operation, reply);
}

Common::AsyncOperationSPtr HostingQueryManager::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    std::wstring applicationNameArgument,
    std::wstring serviceManifestNameArgument,
    std::wstring codePackageNameArgument,
    Common::ActivityId const activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RestartDeployedCodePackageAsyncOperation>(
        *this,
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        L"", /* Default ServicePackageActivationId */
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr HostingQueryManager::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    wstring const & applicationNameArgument,
    wstring const & serviceManifestNameArgument,
    wstring const & servicePackageActivationId,
    wstring const & codePackageNameArgument,
    ActivityId const activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RestartDeployedCodePackageAsyncOperation>(
        *this,
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        servicePackageActivationId,
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode HostingQueryManager::EndRestartDeployedPackage(
    Common::AsyncOperationSPtr const &operation,
    __out Transport::MessageUPtr& reply)
{
    return RestartDeployedCodePackageAsyncOperation::End(operation, reply);
}

Common::ErrorCode HostingQueryManager::GetAggregatedApplicationsDeployedOnNode(
    wstring const & applicationNameArgument,
    ActivityId const & activityId,
    wstring const & continuationToken,
    __out map<wstring, DeployedApplicationQueryResult> & deployedApplicationsMap)
{
    QueryResult queryResult;

    // Get status of already active Applications
    vector<Application2SPtr> applications;
    auto error = hosting_.ApplicationManagerObj->GetApplications(applications, applicationNameArgument, continuationToken);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itResult = applications.begin(); itResult != applications.end(); ++itResult)
    {
        if (applicationNameArgument.empty() && (*itResult)->AppName == SystemServiceApplicationNameHelper::SystemServiceApplicationName)
        {
            // Skip system application unless explicitly queried
            continue;
        }

        auto deployedApplicationQueryResult = (*itResult)->GetDeployedApplicationQueryResult();
        deployedApplicationsMap.insert(
            make_pair(
                deployedApplicationQueryResult.ApplicationName,
                move(deployedApplicationQueryResult)));
    }

    if (!applicationNameArgument.empty() && !deployedApplicationsMap.empty())
    {
        TraceDeployedApplicationQueryResult(activityId, applicationNameArgument, deployedApplicationsMap);
        queryResult = QueryResult(move(GetValues(deployedApplicationsMap)));
        return ErrorCodeValue::Success;
    }

    // Get status of Application being Activated
    vector<DeployedApplicationQueryResult> applicationActivations;
    error = hosting_.ApplicationManagerObj->ActivatorObj->GetApplicationsQueryResult(applicationNameArgument, applicationActivations, continuationToken);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itResult = applicationActivations.begin(); itResult != applicationActivations.end(); ++itResult)
    {
        if (applicationNameArgument.empty() && itResult->ApplicationName == SystemServiceApplicationNameHelper::SystemServiceApplicationName)
        {
            // Skip system application unless explicitly queried
            continue;
        }

        if (deployedApplicationsMap.find(itResult->ApplicationName) == deployedApplicationsMap.end())
        {
            deployedApplicationsMap.insert(make_pair(itResult->ApplicationName, *itResult));
        }
    }

    if (!applicationNameArgument.empty() && !deployedApplicationsMap.empty())
    {
        TraceDeployedApplicationQueryResult(activityId, applicationNameArgument, deployedApplicationsMap);
        queryResult = QueryResult(move(GetValues(deployedApplicationsMap)));
        return ErrorCodeValue::Success;
    }

    // Get status of Application being Downloaded
    vector<DeployedApplicationQueryResult> applicationDownloads;
    error = hosting_.DownloadManagerObj->GetApplicationsQueryResult(applicationNameArgument, applicationDownloads, continuationToken);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itResult = applicationDownloads.begin(); itResult != applicationDownloads.end(); ++itResult)
    {
        if (applicationNameArgument.empty() && itResult->ApplicationName == SystemServiceApplicationNameHelper::SystemServiceApplicationName)
        {
            // Skip system application unless explicitly queried
            continue;
        }

        if (deployedApplicationsMap.find(itResult->ApplicationName) == deployedApplicationsMap.end())
        {
            deployedApplicationsMap.insert(make_pair(itResult->ApplicationName, *itResult));
        }
    }

    TraceDeployedApplicationQueryResult(activityId, applicationNameArgument, deployedApplicationsMap);

    return ErrorCodeValue::Success;
}

ErrorCode HostingQueryManager::GetApplicationListDeployedOnNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    wstring applicationNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument);

    map<wstring, DeployedApplicationQueryResult> deployedApplicationsMap;

    auto error = GetAggregatedApplicationsDeployedOnNode(applicationNameArgument, activityId, wstring(), deployedApplicationsMap);

    queryResult = QueryResult(move(GetValues(deployedApplicationsMap)));

    return error;
}

// We don't worry about the node name since it's already used in getting to this particular replica.
ErrorCode HostingQueryManager::GetApplicationPagedListDeployedOnNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    // This gets some arguments that we don't need. However, this is necessary because we need to check somewhere that all given parameters are correct,
    // and we can't do it at query specification level for IncludeHealthState.
    DeployedApplicationQueryDescription queryDescription;
    auto error = queryDescription.GetDescriptionFromQueryArgumentMap(queryArgument);

    // If we weren't able to get the QueryDescription, then trace the error message
    if (!error.IsSuccess())
    {
        // If parsing fails, return error
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetApplicationPagedListDeployedOnNode failed because the QueryDescription object is invalid or could not be parsed: {1}",
            activityId,
            error.Message);

        return error;
    }

    QueryPagingDescriptionUPtr pagingDescription;
    queryDescription.TakePagingDescription(pagingDescription);

    // Create list to store only the results we want to return
    ListPager<DeployedApplicationQueryResult> deployadeApplicationResultList;

    // When we call "TryAdd", this will make sure to respect the max results.
    deployadeApplicationResultList.SetMaxResults(pagingDescription->MaxResults);

    // Get a list of all deployed applications in active, activating, and downloading states
    // We ignore those in deactivating states
    map<wstring, DeployedApplicationQueryResult> deployedApplicationsMap;

    wstring continuationToken;
    if (pagingDescription != nullptr)
    {
        continuationToken = pagingDescription->ContinuationToken;
    }

    error = GetAggregatedApplicationsDeployedOnNode(queryDescription.GetApplicationNameString(), activityId, continuationToken, deployedApplicationsMap);

    if (!error.IsSuccess())
    {
        // If parsing fails, return error
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetApplicationPagedListDeployedOnNode failed retrieving deployed apps: {1}",
            activityId,
            error.Message);

        return error;
    }

    for (auto app : deployedApplicationsMap)
    {
        // note: we don't worry about the continuation token here because when we get this map, we already passed
        // through the continuation token to method GetAggregatedApplicationsDeployedOnNode
        // For the continuation token comparison that the method calls,
        // we use the > operator here, because that's what the map would be using, so we guarantee the ordering
        // Add only if ListPager determines there is enough room
        error = deployadeApplicationResultList.TryAdd(move(app.second));

        if (!error.IsSuccess()) // unexpected error
        {
            deployadeApplicationResultList.TraceErrorFromTryAdd(error, TraceType, activityId.ToString(), L"DeployedApplicationQueryResult");
            if (!error.IsError(ErrorCodeValue::EntryTooLarge) && !error.IsError(ErrorCodeValue::MaxResultsReached))
            {
                return error;
            }

            break;
        }
    }

    queryResult = move(QueryResult(move(deployadeApplicationResultList)));

    return ErrorCodeValue::Success;
}

ErrorCode HostingQueryManager::GetServiceManifestListDeployedOnNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    wstring applicationNameArgument;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring serviceManifestNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument);

    Application2SPtr applicationEntry;
    auto error = GetApplicationEntry(applicationNameArgument, activityId, applicationEntry);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<ServicePackage2SPtr> servicePackageInstances;
    error = GetServicePackages(applicationEntry, serviceManifestNameArgument, activityId, servicePackageInstances);
    if (!error.IsSuccess())
    {
        return error;
    }

    map<wstring, DeployedServiceManifestQueryResult> deployedServiceManifestsMap;
    for (auto const & svcPkgInstance : servicePackageInstances)
    {
        auto serviceManifestQueryResult = svcPkgInstance->GetDeployedServiceManifestQueryResult();

        auto key = wformatString("{0}_{1}", svcPkgInstance->ServicePackageName, svcPkgInstance->Id.PublicActivationId);
        deployedServiceManifestsMap.insert(make_pair(key, move(serviceManifestQueryResult)));
    }

    if (!serviceManifestNameArgument.empty() && !deployedServiceManifestsMap.empty())
    {
        TraceDeployedServiceManifestQueryResult(activityId, applicationNameArgument, serviceManifestNameArgument, deployedServiceManifestsMap);
        queryResult = QueryResult(move(GetValues(deployedServiceManifestsMap)));
        return ErrorCodeValue::Success;
    }

    // Get status of servicePackageInstances being Activated
    vector<DeployedServiceManifestQueryResult> servicePackageActivations;
    error = hosting_.ApplicationManagerObj->ActivatorObj->GetServiceManifestQueryResult(applicationNameArgument, serviceManifestNameArgument, servicePackageActivations);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itResult = servicePackageActivations.begin(); itResult != servicePackageActivations.end(); ++itResult)
    {
        auto key = wformatString("{0}_{1}", itResult->ServiceManifestName, itResult->ServicePackageActivationId);

        if (deployedServiceManifestsMap.find(key) == deployedServiceManifestsMap.end())
        {
            deployedServiceManifestsMap.insert(make_pair(key, *itResult));
        }
    }

    if (!serviceManifestNameArgument.empty() && !deployedServiceManifestsMap.empty())
    {
        TraceDeployedServiceManifestQueryResult(activityId, applicationNameArgument, serviceManifestNameArgument, deployedServiceManifestsMap);
        queryResult = QueryResult(move(GetValues(deployedServiceManifestsMap)));
        return ErrorCodeValue::Success;
    }

    // Get status of ServicePackages being Downloaded
    vector<DeployedServiceManifestQueryResult> servicePackageDownloads;
    error = hosting_.DownloadManagerObj->GetServiceManifestQueryResult(applicationNameArgument, serviceManifestNameArgument, servicePackageDownloads);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itResult = servicePackageDownloads.begin(); itResult != servicePackageDownloads.end(); ++itResult)
    {
        auto key = wformatString("{0}_{1}", itResult->ServiceManifestName, itResult->ServicePackageActivationId);

        if (deployedServiceManifestsMap.find(key) == deployedServiceManifestsMap.end())
        {
            deployedServiceManifestsMap.insert(make_pair(key, *itResult));
        }
    }

    TraceDeployedServiceManifestQueryResult(activityId, applicationNameArgument, serviceManifestNameArgument, deployedServiceManifestsMap);
    queryResult = QueryResult(move(GetValues(deployedServiceManifestsMap)));
    return ErrorCodeValue::Success;
}

ErrorCode HostingQueryManager::GetServiceTypeListDeployedOnNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    wstring applicationNameArgument;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring serviceManifestNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument);

    wstring serviceTypeNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeNameArgument);

    ApplicationIdentifier id;
    if (StringUtility::Compare(applicationNameArgument, NamingUri::RootNamingUriString) != 0)
    {
        Application2SPtr applicationEntry;
        auto error = GetApplicationEntry(applicationNameArgument, activityId, applicationEntry);
        if (!error.IsSuccess())
        {
            return error;
        }
        id = applicationEntry->Id;
    }

    vector<DeployedServiceTypeQueryResult> serviceTypeQueryResult;
    serviceTypeQueryResult = hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->GetDeployedServiceTypeQueryResult(
        id,
        serviceManifestNameArgument,
        serviceTypeNameArgument);

    wstring resultTraceString;
    for (auto iter = serviceTypeQueryResult.begin(); iter != serviceTypeQueryResult.end(); ++iter)
    {
        resultTraceString.append(wformatString("\r\n{0}", iter->ToString()));
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "GetServiceTypeListDeployedOnNode[{0}]: AppNameFilter: {1}, ServiceManifestNameFiter:{2}, ServiceTypeNameFilter:{3}, Result:{4}",
        activityId,
        applicationNameArgument,
        serviceManifestNameArgument,
        serviceTypeNameArgument,
        resultTraceString);

    queryResult = QueryResult(move(serviceTypeQueryResult));
    return ErrorCode::Success();
}

ErrorCode HostingQueryManager::GetCodePackageListDeployedOnNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    std::vector<DeployedCodePackageQueryResult> codePackageQueryResult;
    wstring applicationNameArgument;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring serviceManifestNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument);

    Application2SPtr applicationEntry;
    auto error = GetApplicationEntry(applicationNameArgument, activityId, applicationEntry);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<ServicePackage2SPtr> servicePackages;
    error = GetServicePackages(applicationEntry, serviceManifestNameArgument, activityId, servicePackages);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<VersionedServicePackageSPtr> versionedServicePackages;
    for (auto servicePackageIter = servicePackages.begin(); servicePackageIter != servicePackages.end(); ++servicePackageIter)
    {
        VersionedServicePackageSPtr versionedServicePackage = (*servicePackageIter)->GetVersionedServicePackage();
        if (!versionedServicePackage)
        {
            WriteInfo(
                TraceType,
                Root.TraceId,
                "{0}: GetVersionedServicePackage for service package {1} failed",
                activityId,
                serviceManifestNameArgument);
            continue;
        }

        versionedServicePackages.push_back(versionedServicePackage);
    }

    wstring codePackageInstanceArgument;
    FABRIC_INSTANCE_ID codePackageInstance{};
    bool codePackageInstanceSpecified = false;
    if (queryArgument.TryGetValue(QueryResourceProperties::CodePackage::InstanceId, codePackageInstanceArgument))
    {
        codePackageInstanceSpecified = StringUtility::TryFromWString(codePackageInstanceArgument, codePackageInstance);
    }

    wstring codePackageNameArgument;
    queryArgument.TryGetValue(QueryResourceProperties::CodePackage::CodePackageName, codePackageNameArgument);
    for (auto versionedServicePackageIter = versionedServicePackages.begin(); versionedServicePackageIter != versionedServicePackages.end(); ++versionedServicePackageIter)
    {
        vector<CodePackageSPtr> codePackages = (*versionedServicePackageIter)->GetCodePackages(codePackageNameArgument);
        for (auto itCodePackage = codePackages.begin(); itCodePackage != codePackages.end(); ++itCodePackage)
        {
            auto deployedCodePackageQueryResult = (*itCodePackage)->GetDeployedCodePackageQueryResult();
            if (!codePackageInstanceSpecified ||
                (deployedCodePackageQueryResult.EntryPoint.InstanceId == (uint64)codePackageInstance))
            {
                deployedCodePackageQueryResult.NodeName = hosting_.NodeName;
                codePackageQueryResult.push_back(move(deployedCodePackageQueryResult));
            }
        }
    }

    wstring resultTraceString;
    for (auto iter = codePackageQueryResult.begin(); iter != codePackageQueryResult.end(); ++iter)
    {
        resultTraceString.append(wformatString("\r\n{0}", iter->ToString()));
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "GetCodePackageListDeployedOnNode[{0}]: AppNameFilter: {1}, ServiceManifestNameFiter:{2}, CodePackageNameFilter:{3}, Result:{4}",
        activityId,
        applicationNameArgument,
        serviceManifestNameArgument,
        codePackageNameArgument,
        resultTraceString);

    queryResult = QueryResult(move(codePackageQueryResult));
    return ErrorCode::Success();
}

ErrorCode HostingQueryManager::GetApplicationEntry(
    wstring const & filterApplicationName,
    ActivityId const & activityId,
    __out Application2SPtr & applicationEntry)
{
    vector<Application2SPtr> applications;
    auto error = hosting_.ApplicationManagerObj->GetApplications(applications, filterApplicationName);
    if (!error.IsSuccess())
    {
        return ReplaceErrorIf(error, ErrorCodeValue::ObjectClosed, ErrorCodeValue::ApplicationNotFound);
    }

    if (applications.empty())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: Application Name {1} not found",
            activityId,
            filterApplicationName);
        return ErrorCodeValue::ApplicationNotFound;
    }

    applicationEntry = applications[0];
    return ErrorCode::Success();
}

ErrorCode HostingQueryManager::GetServicePackages(
    Application2SPtr const & applicationEntry,
    wstring const & filterServiceManifestName,
    ActivityId const & activityId,
    __out vector<ServicePackage2SPtr> & servicePackages)
{
    VersionedApplicationSPtr versionApplication = applicationEntry->GetVersionedApplication();
    if (!versionApplication)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetVersionedApplication for ApplicationName {1} failed",
            activityId,
            applicationEntry->AppName);
        return ErrorCodeValue::ApplicationNotFound;
    }

    ErrorCode error;

    if (filterServiceManifestName.empty())
    {
        error = versionApplication->GetAllServicePackageInstances(servicePackages);
    }
    else
    {
        error = versionApplication->GetInstancesOfServicePackage(filterServiceManifestName, servicePackages);
    }

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetServicePackages for ApplicationName {1} failed with error {2}",
            activityId,
            applicationEntry->AppName,
            error);
        return ReplaceErrorIf(error, ErrorCodeValue::ObjectClosed, ErrorCodeValue::ApplicationNotFound);
    }

    return ErrorCode::Success();
}

void HostingQueryManager::TraceDeployedApplicationQueryResult(
    ActivityId const & activityId,
    wstring const & filterApplicationName,
    map<wstring, ServiceModel::DeployedApplicationQueryResult> const & deployedApplicationQueryResult)
{
    wstring resultTraceString;
    for (auto iter = deployedApplicationQueryResult.begin(); iter != deployedApplicationQueryResult.end(); ++iter)
    {
        resultTraceString.append(wformatString("\r\n{0}", iter->second.ToString()));
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "GetApplicationListDeployedOnNode[{0}]: AppNameFilter: {1}, Result:{2}",
        activityId,
        filterApplicationName,
        resultTraceString);
}

void HostingQueryManager::TraceDeployedServiceManifestQueryResult(
    ActivityId const & activityId,
    wstring const & filterApplicationName,
    wstring const & filterServiceManifestName,
    map<wstring, ServiceModel::DeployedServiceManifestQueryResult> const & deployedServiceManifestQueryResult)
{
    wstring resultTraceString;
    for (auto iter = deployedServiceManifestQueryResult.begin(); iter != deployedServiceManifestQueryResult.end(); ++iter)
    {
        resultTraceString.append(wformatString("\r\n{0}", iter->second.ToString()));
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "GetServiceManifestListDeployedOnNode[{0}]: AppNameFilter: {1}, ServiceManifestName: {2}, Result:{3}",
        activityId,
        filterApplicationName,
        filterServiceManifestName,
        resultTraceString);
}

ErrorCode HostingQueryManager::GetCodePackage(
    int64 codePackageInstanceId,
    wstring const & applicationNameArgument,
    wstring const & serviceManifestNameArgument,
    wstring const & servicePackageActivationId,
    wstring const & codePackageNameArgument,
    ActivityId const activityId,
    __out CodePackageSPtr & matchingCodePackage)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "{0} - GetCodePackage[codePackageInstanceId: {1}, applicationNameArgument: {2}, serviceManifestNameArgument: {3}, servicePackageActivationId: {4}, codePackageNameArgument: {5}]",
        activityId, codePackageInstanceId, applicationNameArgument,	serviceManifestNameArgument, servicePackageActivationId, codePackageNameArgument);

    Application2SPtr applicationEntry;
    auto error = GetApplicationEntry(applicationNameArgument, activityId, applicationEntry);
    if (!error.IsSuccess())
    {
        return error;
    }

    ServicePackage2SPtr servicePackage;

    VersionedApplicationSPtr versionApplication = applicationEntry->GetVersionedApplication();
    if (!versionApplication)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetVersionedApplication for ApplicationName {1} failed",
            activityId,
            applicationEntry->AppName);
        return ErrorCodeValue::ApplicationNotFound;
    }

    error = versionApplication->GetServicePackageInstance(serviceManifestNameArgument, servicePackageActivationId, servicePackage);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetServicePackages for ApplicationName {1} failed with error {2}",
            activityId,
            applicationEntry->AppName,
            error);
        return ErrorCodeValue::ServiceManifestNotFound;
    }

    VersionedServicePackageSPtr versionedServicePackage = servicePackage->GetVersionedServicePackage();
    if (!versionedServicePackage)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: GetVersionedServicePackage for service package {1} failed",
            activityId,
            serviceManifestNameArgument);
        return ErrorCodeValue::ServiceManifestNotFound;
    }

    vector<CodePackageSPtr> codePackages = versionedServicePackage->GetCodePackages(codePackageNameArgument);
    if (codePackages.size() != 1)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: Expected only one code package to be found that matches {1}. Actualy found {2}",
            activityId,
            codePackageNameArgument,
            codePackages.size());
        return ErrorCodeValue::CodePackageNotFound;
    }

    matchingCodePackage = codePackages[0];

    return ErrorCode();
}

ErrorCode HostingQueryManager::ReplaceErrorIf(ErrorCode actualError, ErrorCodeValue::Enum compareWithError, ErrorCodeValue::Enum changeToError)
{
    if (actualError.IsError(compareWithError))
    {
        return changeToError;
    }

    return actualError;
}

ErrorCode HostingQueryManager::OnClose()
{
    queryJobQueue_->Close();
    return queryMessageHandler_->Close();
}

void HostingQueryManager::OnAbort()
{
    queryJobQueue_->Close();
    queryMessageHandler_->Abort();
}

