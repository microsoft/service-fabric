// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Reliability;
using namespace Query;
using namespace Naming;

using namespace SystemServices;

// *** Private SystemServiceResolver::Impl

StringLiteral const TraceComponent("SystemServiceResolver");

namespace SystemServices
{
    class Utility
    {
    public:

        static PartitionInfo GetPartitionInfo(ServicePartitionInformation const & info)
        {
            PartitionInfo partitionInfo;
            switch (info.PartitionKind)
            {
            case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                partitionInfo = PartitionInfo(FABRIC_PARTITION_SCHEME_SINGLETON);
                break;

            case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                partitionInfo = PartitionInfo(info.PartitionLowKey, info.PartitionHighKey);
                break;

            case FABRIC_SERVICE_PARTITION_KIND_NAMED:
                partitionInfo = PartitionInfo(info.PartitionName);
                break;

            default:
                Common::Assert::CodingError("Unexpected PartitionKind {0}.", static_cast<unsigned int>(info.PartitionKind));
            }

            return partitionInfo;
        }

        // FabSrvServices will have their endpoints wrapped in json, so parsing needs to be done differently.
        // FabSrvServices can have reserved or dynamic partition IDs 
        static bool IsReservedIdFabSrvService(ConsistencyUnitId& cuid)
        {
            return cuid.IsBackupRestoreService() || cuid.IsFaultAnalysisService() || cuid.IsUpgradeOrchestrationService() || cuid.IsEventStoreService();
        }

        // Check if it is a fabSrvService based on service name.
        // This overload is applicable for system services with dynamic (non-reserved) partition IDs.
        static bool IsDynamicIdFabSrvService(std::wstring& internalServiceName)
        {
            return StringUtility::AreEqualCaseInsensitive(internalServiceName, *SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName);
        }
    };
}
class LocationVersionEntry
{
public:
    explicit LocationVersionEntry(ServiceLocationVersion const & version)
        : version_(version)
        , isStale_(false)
    {
    }

    ServiceLocationVersion const & GetVersion() const { return version_; }
    bool IsStale() const { return isStale_; }
    void MarkStale() { isStale_ = true; }

private:
    ServiceLocationVersion version_;
    bool isStale_;
};

typedef shared_ptr<LocationVersionEntry> LocationVersionEntrySPtr;

class SystemServiceResolver::Impl 
    : public RootedObject
{
public:
    Impl(
        __in QueryGateway & query,
        __in ServiceResolver & resolver,
        ComponentRoot const & root)
        : RootedObject(root)
        , query_(query)
        , resolver_(resolver)
        , cuidsByName_()
        , versionsByCuid_()
        , cuidsLock_()
    {
    }

    AsyncOperationSPtr BeginResolve(
        wstring const & serviceName,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root);

    AsyncOperationSPtr BeginResolve(
        ConsistencyUnitId const & cuid,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root);

    ErrorCode EndResolve(
        AsyncOperationSPtr const & operation, 
        __out SystemServiceLocation & primaryLocation,
        __out vector<wstring> & secondaryLocations);

    ErrorCode EndResolve(
        AsyncOperationSPtr const &,
        __out vector<ServiceTableEntry> & serviceEntries,
        __out PartitionInfo & partitionInfo,
        __out GenerationNumber & generation);

    void UpdateCachedCuid(wstring const & serviceName, pair<ConsistencyUnitId, PartitionInfo> && cuid)
    {
        AcquireWriteLock lock(cuidsLock_);

        cuidsByName_[serviceName] = move(cuid);
    }

    void ClearCachedCuid(wstring const & serviceName)
    {
        AcquireWriteLock lock(cuidsLock_);

        auto it = cuidsByName_.find(serviceName);

        if (it != cuidsByName_.end())
        {
            versionsByCuid_.erase(it->second.first);

            cuidsByName_.erase(it);
        }
    }

    bool TryGetCachedCuid(wstring const & serviceName, __out ConsistencyUnitId & cuid)
    {
        AcquireReadLock lock(cuidsLock_);

        auto it = cuidsByName_.find(serviceName);

        if (it != cuidsByName_.end())
        {
            cuid = it->second.first;
            return true;
        }

        return false;
    }

    bool TrySetLocationVersion(ConsistencyUnitId const & cuid, ServiceLocationVersion const & version)
    {
        AcquireWriteLock lock(cuidsLock_);

        auto it = versionsByCuid_.find(cuid);

        if (it == versionsByCuid_.end() || it->second->GetVersion().CompareTo(version) < 0)
        {
            versionsByCuid_[cuid] = make_shared<LocationVersionEntry>(version);

            return true;
        }

        return false;
    }
    
    ServiceLocationVersion GetLocationVersion(ConsistencyUnitId const & cuid, __out bool & isStale)
    {
        ServiceLocationVersion versionResult;
        isStale = false;

        AcquireReadLock lock(cuidsLock_);

        auto it = versionsByCuid_.find(cuid);

        if (it != versionsByCuid_.end())
        {
            versionResult = it->second->GetVersion();
            isStale = it->second->IsStale();
        }

        return versionResult;
    }

    // Providing functions to mark locations stale allows the resolver
    // to maintain state on behalf of the caller.
    //
    void MarkLocationStale(std::wstring const & serviceName)
    {
        AcquireWriteLock lock(cuidsLock_);

        auto it = cuidsByName_.find(serviceName);

        if (it != cuidsByName_.end())
        {
            this->MarkLocationStaleCallerHoldsLock(it->second.first);
        }
    }

    void MarkLocationStale(ConsistencyUnitId const & cuid)
    {
        AcquireWriteLock lock(cuidsLock_);

        this->MarkLocationStaleCallerHoldsLock(cuid);
    }

private:
    class ResolveAsyncOperation;

    void MarkLocationStaleCallerHoldsLock(ConsistencyUnitId const & cuid)
    {
        auto it = versionsByCuid_.find(cuid);

        if (it != versionsByCuid_.end())
        {
            it->second->MarkStale();
        }
    }

    QueryGateway & query_;
    ServiceResolver & resolver_;

    unordered_map<wstring, pair<ConsistencyUnitId, PartitionInfo>> cuidsByName_;
    unordered_map<ConsistencyUnitId, LocationVersionEntrySPtr, ConsistencyUnitId::Hasher> versionsByCuid_;
    RwLock cuidsLock_;
};

// ***  ResolveAsyncOperation
// SystemServiceResolver can handle both regular system services and fabsrv services (services that publish JSON endpoint)
class SystemServiceResolver::Impl::ResolveAsyncOperation 
    : public AsyncOperation
    , public TextTraceComponent<TraceTaskCodes::SystemServices>
{
public:
    ResolveAsyncOperation(
        __in SystemServiceResolver::Impl & owner,
        wstring const & serviceName,
        ConsistencyUnitId const & cuid,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , owner_(owner)
        , serviceName_(serviceName)
        , cuid_(cuid)
        , activityId_(activityId)
        , traceId_(wformatString("{0}", activityId_))
        , timeoutHelper_(timeout)
        , primaryLocation_()
        , secondaryLocations_()
        , serviceEntries_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out SystemServiceLocation & primaryLocation,
        __out vector<wstring> & secondaryLocations)
    {
        auto casted = AsyncOperation::End<ResolveAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            primaryLocation = casted->primaryLocation_;
            secondaryLocations = casted->secondaryLocations_;
        }

        return casted->Error;
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceTableEntry> & serviceEntries,
        __out PartitionInfo & partitionInfo,
        __out GenerationNumber & generation)
    {
        auto casted = AsyncOperation::End<ResolveAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            serviceEntries = casted->serviceEntries_;
            partitionInfo = casted->partitionInfo_;
            generation = casted->generation_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (cuid_.IsInvalid)
        {
            if (owner_.TryGetCachedCuid(serviceName_, cuid_))
            {
                WriteNoise(
                    TraceComponent, 
                    "{0}: using cached cuid: service={1} cuid={2}",
                    this->TraceId, 
                    serviceName_, 
                    cuid_);

                this->ResolvePrimaryLocation(thisSPtr);
            }
            else
            {
                this->QueryForCuid(thisSPtr);
            }
        }
        else
        {
            this->ResolvePrimaryLocation(thisSPtr);
        }
    }

private:

    __declspec (property(get=get_TraceId)) wstring const & TraceId;
    wstring const & get_TraceId() const { return traceId_; }

    void QueryForCuid(AsyncOperationSPtr const & thisSPtr)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Service::ServiceName,
            serviceName_);

        // TODO: Optimize query to fetch FM data only
        //       (i.e. do not issue parallel query to HM for health)
        //
        auto body = Common::make_unique<QueryRequestMessageBody>(
            QueryNames::ToString(QueryNames::GetServicePartitionList),
            argMap);

        auto request = ClientServerTransport::QueryTcpMessage::GetQueryRequest(std::move(body))->GetTcpMessage();

        auto operation = owner_.query_.BeginProcessIncomingQuery(
            *request,
            FabricActivityHeader(activityId_),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnQueryCompleted(operation, false); },
            thisSPtr);
        this->OnQueryCompleted(operation, true);
    }

    void OnQueryCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        MessageUPtr reply;
        auto error = owner_.query_.EndProcessIncomingQuery(operation, reply);

        if (error.IsSuccess())
        {
            QueryResult result;
            if (reply->GetBody(result))
            {
                vector<ServicePartitionQueryResult> partitions;
                error = result.MoveList<ServicePartitionQueryResult>(partitions);

                if (!error.IsSuccess())
                {
                    WriteWarning(TraceComponent, "{0}: failed to read partitions list: error={1}", this->TraceId, error);
                }
                else if (partitions.empty())
                {
                    WriteWarning(TraceComponent, "{0}: no partitions found", this->TraceId);
                    error = ErrorCodeValue::SystemServiceNotFound;
                }
                else
                {
                    cuid_ = ConsistencyUnitId(partitions[0].PartitionId);
                    partitionInfo_ = Utility::GetPartitionInfo(partitions[0].PartitionInformation);
                    pair<ConsistencyUnitId, PartitionInfo> servicePartitionInfo(cuid_, partitionInfo_);
                    owner_.UpdateCachedCuid(serviceName_, move(servicePartitionInfo));

                    WriteInfo(TraceComponent, "{0}: cached cuid mapping: service={1} cuid={2}", this->TraceId, serviceName_, cuid_);

                    if (partitions.size() > 1)
                    {
                        auto msg = wformatString("{0}: unexpected partition count: {1}", this->TraceId, partitions);
                        WriteError(TraceComponent, "{0}", msg);
                        Assert::TestAssert("{0}", msg);
                    }
                }
            }
            else
            {
                WriteWarning(TraceComponent, "{0}: invalid query reply body", this->TraceId);
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        if (error.IsSuccess())
        {
            this->ResolvePrimaryLocation(thisSPtr);
        }
        else
        {
            WriteWarning(TraceComponent, "{0}: query failed: error={1}", this->TraceId, error);

            this->TryComplete(thisSPtr, error);
        }
    }

    void ResolvePrimaryLocation(AsyncOperationSPtr const & thisSPtr)
    {
        bool isStale = false;
        auto version = owner_.GetLocationVersion(cuid_, isStale);

        WriteNoise(
            TraceComponent, 
            "{0}: resolving: service={1} cuid={2} version={3} stale={4}", 
            this->TraceId, 
            serviceName_, 
            cuid_,
            version,
            isStale);

        vector<VersionedCuid> cuids;
        cuids.push_back(VersionedCuid(cuid_, version.FMVersion, version.Generation));

        auto operation = owner_.resolver_.BeginResolveServicePartition(
            cuids,
            isStale ? CacheMode::Refresh : CacheMode::UseCached,
            FabricActivityHeader(activityId_),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
            thisSPtr);
        this->OnResolveCompleted(operation, true);
    }

    void OnResolveCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.resolver_.EndResolveServicePartition(operation, serviceEntries_, generation_);

        if (error.IsSuccess())
        {
            bool isHttpOnly = false;
            bool parseSuccess = false;

            auto first = serviceEntries_.front();
            if (first.ServiceReplicaSet.IsPrimaryLocationValid)
            {
                bool parseWasSuccessful = false;
                if (cuid_.IsEventStoreService())
                {
                    // A Temp solution, skip as parser always expects a tcp endpoint to exist,
                    // however EventStoreService has only a http/https endpoint.
                    isHttpOnly = true;
                }
                else
                {
                    // Check if it is a fabSrvService, based on fixed partition ID or by service name (when using dynamic partition IDs)
                    bool isFabSrvService = Utility::IsReservedIdFabSrvService(cuid_) || Utility::IsDynamicIdFabSrvService(serviceName_);
                    parseWasSuccessful = SystemServiceLocation::TryParse(first.ServiceReplicaSet.PrimaryLocation, isFabSrvService, primaryLocation_); 
                }

                if (parseWasSuccessful)
                {
                    secondaryLocations_ = first.ServiceReplicaSet.SecondaryLocations;

                    ServiceLocationVersion locationVersion(first.Version, generation_, 0);
                    if (owner_.TrySetLocationVersion(cuid_, locationVersion))
                    {
                        WriteInfo(
                            TraceComponent, 
                            "{0}: updated location version: service={1} cuid={2} version={3}",
                            this->TraceId, 
                            serviceName_, 
                            cuid_,
                            locationVersion);
                    }
                }

                if (isHttpOnly || parseWasSuccessful)
                {
                    parseSuccess = true;
                }
            }
            
            if (!parseSuccess)
            {
                // retryable by Naming Gateway
                error = ErrorCodeValue::SystemServiceNotFound;
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} could not resolve {1} ({2}): error = {3}",
                this->TraceId,
                serviceName_,
                cuid_,
                error);

            // most errors from the FM on system service resolution are not retryable 
            // with the following exceptions (convert these to retryable errors)
            //
            switch (error.ReadValue())
            {
            case ErrorCodeValue::FMFailoverUnitNotFound:
            case ErrorCodeValue::PartitionNotFound:

                WriteInfo(TraceComponent, "{0}: clear cuid mapping: service={1} cuid={2}", this->TraceId, serviceName_, cuid_);

                owner_.ClearCachedCuid(serviceName_);

                error = ErrorCodeValue::FMFailoverUnitNotFound;
                break;

            case ErrorCodeValue::ServiceOffline:

                error = ErrorCodeValue::SystemServiceNotFound;
                break;
            }
        }

        this->TryComplete(thisSPtr, error);
    }

    SystemServiceResolver::Impl & owner_;
    wstring serviceName_;
    ConsistencyUnitId cuid_;
    ActivityId activityId_;

    wstring traceId_;
    TimeoutHelper timeoutHelper_;

    SystemServiceLocation primaryLocation_;
    vector<wstring> secondaryLocations_;
    vector<ServiceTableEntry> serviceEntries_;
    GenerationNumber generation_;
    PartitionInfo partitionInfo_;
};

// *** Impl

AsyncOperationSPtr SystemServiceResolver::Impl::BeginResolve(
    wstring const & serviceName,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<ResolveAsyncOperation>(
        *this,
        serviceName,
        ConsistencyUnitId::Invalid(),
        activityId,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr SystemServiceResolver::Impl::BeginResolve(
    ConsistencyUnitId const & cuid,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<ResolveAsyncOperation>(
        *this,
        L"",
        cuid,
        activityId,
        timeout,
        callback,
        root);
}

ErrorCode SystemServiceResolver::Impl::EndResolve(
    AsyncOperationSPtr const & operation, 
    __out SystemServiceLocation & primaryLocation,
    __out vector<wstring> & secondaryLocations)
{
    return ResolveAsyncOperation::End(operation, primaryLocation, secondaryLocations);
}

ErrorCode SystemServiceResolver::Impl::EndResolve(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceTableEntry> & serviceEntries,
    __out PartitionInfo & partitionInfo,
    __out GenerationNumber & generation)
{
    return ResolveAsyncOperation::End(operation, serviceEntries, partitionInfo, generation);
}

// *** Public SystemServiceResolver

SystemServiceResolverSPtr SystemServiceResolver::Create(
    __in Query::QueryGateway & query,
    __in Reliability::ServiceResolver & resolver,
    Common::ComponentRoot const & root)
{
    return shared_ptr<SystemServiceResolver>(new SystemServiceResolver(query, resolver, root));
}

SystemServiceResolver::SystemServiceResolver(
    __in Query::QueryGateway & query,
    __in Reliability::ServiceResolver & resolver,
    Common::ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(query, resolver, root))
{
}

Common::AsyncOperationSPtr SystemServiceResolver::BeginResolve(
    std::wstring const & serviceName,
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & root)
{
    return implUPtr_->BeginResolve(serviceName, activityId, timeout, callback, root);
}

Common::AsyncOperationSPtr SystemServiceResolver::BeginResolve(
    Reliability::ConsistencyUnitId const & cuid,
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & root)
{
    return implUPtr_->BeginResolve(cuid, activityId, timeout, callback, root);
}

Common::ErrorCode SystemServiceResolver::EndResolve(
    Common::AsyncOperationSPtr const & operation, 
    __out SystemServiceLocation & primaryLocation,
    __out vector<wstring> & secondaryLocations)
{
    return implUPtr_->EndResolve(operation, primaryLocation, secondaryLocations);
}

Common::ErrorCode SystemServiceResolver::EndResolve(
    Common::AsyncOperationSPtr const & operation,
    __out vector<ServiceTableEntry> & serviceEntries,
    __out PartitionInfo & partitionInfo,
    __out GenerationNumber & generation)
{
    return implUPtr_->EndResolve(operation, serviceEntries, partitionInfo, generation);
}

void SystemServiceResolver::MarkLocationStale(wstring const & serviceName)
{
    implUPtr_->MarkLocationStale(serviceName);
}

void SystemServiceResolver::MarkLocationStale(ConsistencyUnitId const & cuid)
{
    implUPtr_->MarkLocationStale(cuid);
}
