// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ClientServerTransport;
using namespace Naming;

StringLiteral const TraceComponent("ServiceNotificationManager");
    
// This class is used to batch all matching ServiceTableEntry updates
// for each processed broadcast update. This allows batching of
// multiple ServiceTableEntry updates into a single notification
// for each registered client.
//
class MatchedClientRegistration
{
public:
    MatchedClientRegistration(
        ClientRegistrationSPtr const & registration,
        GenerationNumber const & cacheGeneration,
        VersionRangeCollection const & cacheVersions,
        VersionRangeCollection const & updateVersions)
        : registration_(registration)
        , cacheGeneration_(cacheGeneration)
        , cacheVersions_(make_shared<VersionRangeCollection>(cacheVersions))
        , updateVersions_(make_shared<VersionRangeCollection>(updateVersions))
        , matchedPartitions_()
    {
    }

    MatchedClientRegistration(
        ClientRegistrationSPtr const & registration,
        GenerationNumber const & cacheGeneration,
        VersionRangeCollection const & cacheVersions,
        MatchedServiceTableEntryMap && partitions)
        : registration_(registration)
        , cacheGeneration_(cacheGeneration)
        , cacheVersions_(make_shared<VersionRangeCollection>(cacheVersions))
        , updateVersions_(make_shared<VersionRangeCollection>(cacheVersions))
        , matchedPartitions_(move(partitions))
    {
    }

    void AddMatchedPartition(MatchedServiceTableEntrySPtr const & matchedPartition)
    {
        // Sorted by lookup version for two purposes:
        //
        // 1) calculating version ranges when paging
        //
        // 2) duplicate detection when a partition update matches
        //    multiple filters
        //
        auto findIt = matchedPartitions_.find(matchedPartition->Version);
        if (findIt == matchedPartitions_.end())
        {
            matchedPartitions_.insert(make_pair(matchedPartition->Version, matchedPartition));
        }
        else
        {
            // If a notification matches two filters, with one specifying
            // primary-only changes but not the other, then we ignore
            // the primary-only filter and deliver the notification.
            //
            findIt->second->UpdateCheckPrimaryChangeOnly(
                matchedPartition->CheckPrimaryChangeOnly);
        }
    }

    void CreateAndEnqueueNotification(ActivityId const & notificationId)
    {
        registration_->CreateAndEnqueueNotification(
            notificationId,
            cacheGeneration_,
            move(cacheVersions_),
            move(matchedPartitions_),
            move(updateVersions_));
    }

    void CreateAndEnqueueNotificationForClientSynchronization(ActivityId const & notificationId)
    {
        registration_->CreateAndEnqueueNotificationForClientSynchronization(
            notificationId,
            cacheGeneration_,
            move(cacheVersions_),
            move(matchedPartitions_),
            move(updateVersions_));
    }

private:

    ClientRegistrationSPtr registration_;
    GenerationNumber cacheGeneration_;

    // These version ranges must be deep copies since they may change
    // once the update handler is complete. Furthermore, we will be
    // modifying these ranges if the notification gets paged.
    //
    VersionRangeCollectionSPtr cacheVersions_;
    VersionRangeCollectionSPtr updateVersions_;

    MatchedServiceTableEntryMap matchedPartitions_;
};

typedef shared_ptr<MatchedClientRegistration> MatchedClientRegistrationSPtr;

class MatchedClientRegistrationsTable : public TextTraceComponent<TraceTaskCodes::NamingGateway>        
{
public:
    MatchedClientRegistrationsTable()
        : matchedRegistrations_()
    {
    }

    bool HasMatchedRegistrations()
    {
        return !matchedRegistrations_.empty();
    }

    void UpdateMatchingRegistrations(
        ServiceNotificationManager::MatchResultList const & matches,
        GenerationNumber const & cacheGeneration,
        VersionRangeCollection const & cacheVersions,
        VersionRangeCollection const & updateVersions,
        unique_ptr<ClientRegistrationTable> const & clientTable)
    {
        for (auto const & match : matches)
        {
            auto const & regId = match.first;
            auto const & matchedPartition = match.second;

            MatchedClientRegistrationSPtr matchedRegistration;

            auto it = matchedRegistrations_.find(regId);
            if (it == matchedRegistrations_.end())
            {
                auto registration = clientTable->TryGetRegistration(regId, L"");
                if (registration)
                {
                    matchedRegistration = make_shared<MatchedClientRegistration>(
                        registration, 
                        cacheGeneration, 
                        cacheVersions,
                        updateVersions);

                    matchedRegistrations_.insert(make_pair(regId, matchedRegistration));
                }
                else
                {
                    TRACE_ERROR_AND_TESTASSERT(
                        TraceComponent,
                        "registration does not exist for matched id={0}",
                        regId);
                }
            }
            else
            {
                matchedRegistration = it->second;
            }

            if (matchedRegistration)
            {
                matchedRegistration->AddMatchedPartition(matchedPartition);
            }
        }
    }

    void CreateAndEnqueueNotifications(ActivityId const & notificationId)
    {
        for (auto const & it : matchedRegistrations_)
        {
            it.second->CreateAndEnqueueNotification(notificationId);
        }
    }

private:
    map<ClientRegistrationId, MatchedClientRegistrationSPtr> matchedRegistrations_;
};

class NameFilterIndexEntry
    : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
{
    DENY_COPY(NameFilterIndexEntry)

public:

    // Do not expect large numbers of clients per name. Use map to optimize for space.
    //
    typedef std::pair<ClientRegistrationId, ServiceNotificationFilterSPtr> FilterPair;
    typedef std::map<ClientRegistrationId, ServiceNotificationFilterSPtr> FiltersMap;

public:

    NameFilterIndexEntry() : filtersByRegistrationId_() { }

    __declspec (property(get=get_FiltersMap)) FiltersMap const & FiltersByRegistrationId;
    FiltersMap const & get_FiltersMap() const { return filtersByRegistrationId_; }

    bool IsEmpty() { return filtersByRegistrationId_.empty(); }

    ErrorCode TryAddFilter(ClientRegistrationId const & registrationId, ServiceNotificationFilterSPtr const & filter)
    {
        auto result = filtersByRegistrationId_.insert(FilterPair(registrationId, filter));
        if (!result.second)
        {
            WriteInfo(
                TraceComponent,
                "{0}: filter name already exists: existing={1} incoming={2}",
                registrationId,
                *(result.first),
                *filter);

            // This client already has a filter registered on this name.
            // Consider checking for prefix coverage as well.
            //
            return ErrorCode(
                ErrorCodeValue::DuplicateServiceNotificationName,
                wformatString("{0} {1}", StringResource::Get(IDS_NAMING_Duplicate_Notification_Filter_Name), filter->Name));
        }

        return ErrorCodeValue::Success;
    }

    bool TryRemoveFilter(ClientRegistrationId const & registrationId)
    {
        auto it = filtersByRegistrationId_.find(registrationId);

        if (it != filtersByRegistrationId_.end())
        {
            filtersByRegistrationId_.erase(it);

            return true;
        }
        else
        {
            return false;
        }
    }

private:

    FiltersMap filtersByRegistrationId_;
};

typedef std::shared_ptr<NameFilterIndexEntry> NameFilterIndexEntrySPtr;

class ServiceNotificationManager::NameFilterIndex
{
    DENY_COPY(NameFilterIndex)

private:

    typedef std::pair<NamingUri, NameFilterIndexEntrySPtr> FilterPair;
    typedef std::unordered_map<NamingUri, NameFilterIndexEntrySPtr, NamingUri::Hasher> FiltersHash;

public:

    NameFilterIndex() : filtersByName_() { }

    bool IsEmpty() { return filtersByName_.empty(); }

    void Clear() { filtersByName_.clear(); }

    ErrorCode TryAddFilter(ClientRegistrationSPtr const & registration, ServiceNotificationFilterSPtr const & filter)
    {
        auto it = filtersByName_.find(filter->Name);

        if (it == filtersByName_.end())
        {
            it = filtersByName_.insert(FilterPair(filter->Name, make_shared<NameFilterIndexEntry>())).first;
        }

        auto indexEntry = it->second;

        return indexEntry->TryAddFilter(registration->Id, filter);
    }

    bool TryRemoveFilter(ClientRegistrationSPtr const & registration, ServiceNotificationFilterSPtr const & filter)
    {
        auto it = filtersByName_.find(filter->Name);

        if (it == filtersByName_.end())
        {
            return false;
        }

        auto indexEntry = it->second;
        
        bool removed = indexEntry->TryRemoveFilter(registration->Id);
        if (removed && indexEntry->IsEmpty())
        {
            filtersByName_.erase(it);
        }

        return removed;
    }

    bool TryGetFilters(NamingUri const & name, __out NameFilterIndexEntrySPtr & result)
    {
        auto it = filtersByName_.find(name);

        if (it != filtersByName_.end())
        {
            result = it->second;

            return true;
        }

        return false;
    }

private:

    FiltersHash filtersByName_;
};

//
// ServiceNotificationManager
//

ServiceNotificationManager::ServiceNotificationManager(
    Federation::NodeInstance const & nodeInstance,
    std::wstring const & nodeName,
    IServiceTableEntryCache & serviceTableEntryCache, 
    GatewayEventSource const & trace,
    ComponentRoot const & componentRoot)
    : RootedObject(componentRoot)
    , nodeInstance_(nodeInstance)
    , nodeName_(nodeName)
    , traceId_(wformatString(nodeInstance))
    , serviceTableEntryCache_(serviceTableEntryCache)
    , trace_(trace)
    , transport_()
    , clientTable_(make_unique<ClientRegistrationTable>())
    , exactFilterIndex_(make_unique<NameFilterIndex>())
    , prefixFilterIndex_(make_unique<NameFilterIndex>())
    , registrationsLock_()
{
}

ServiceNotificationManager::~ServiceNotificationManager()
{
}

ErrorCode ServiceNotificationManager::Open(EntreeServiceTransportSPtr const & transport)
{
    ServiceNotification::InitializeSerializationOverheadEstimate();
    ServiceNotificationRequestBody::InitializeSerializationOverheadEstimate();

    transport_ = transport;

    return FabricComponent::Open();
}

ErrorCode ServiceNotificationManager::OnOpen()
{
    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "opening");

    if (!transport_)
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} must initialize transport", this->TraceId);
        return ErrorCodeValue::OperationFailed;
    }

    auto root = this->Root.CreateComponentRoot();

    serviceTableEntryCache_.SetUpdateHandler([this, root](
        ActivityId const & a,
        GenerationNumber const& g, 
        VersionRangeCollection const& cacheVersions, 
        vector<CachedServiceTableEntrySPtr> const& p,
        VersionRangeCollection const& updateVersions)
    {
        this->ProcessServiceTableEntries(a, g, cacheVersions, p, updateVersions);
    });

    transport_->SetConnectionFaultHandler([this, root](ErrorCode err)
        {
            // EntreeServiceProxy is down.
            this->OnConnectionFault(err);
        });

    return ErrorCodeValue::Success;
}

ErrorCode ServiceNotificationManager::OnClose()
{
    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "closing");

    transport_->RemoveConnectionFaultHandler();

    serviceTableEntryCache_.ClearUpdateHandler();

    {
        AcquireWriteLock lock(registrationsLock_);

        exactFilterIndex_->Clear();

        prefixFilterIndex_->Clear();

        clientTable_->Clear();
    }

    return ErrorCodeValue::Success;
}

void ServiceNotificationManager::OnAbort()
{
    this->OnClose();
}

ErrorCode ServiceNotificationManager::ProcessRequest(
    MessageUPtr const & request,
    __out MessageUPtr & reply)
{
    ErrorCode error(ErrorCodeValue::Success);
    
    ActivityId activityId;
    FabricActivityHeader activityHeader;
    if (request->Headers.TryReadFirst(activityHeader))
    {
        activityId = activityHeader.ActivityId;
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            this->TraceId,
            "request {0} missing FabricActivityHeader: using={1}",
            request->MessageId,
            activityId);
    }

    ClientIdentityHeader clientRegn;
    if (!request->Headers.TryReadFirst(clientRegn))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "{0} Client identity header missing in request",
            activityId);
        return ErrorCodeValue::InvalidMessage;
    }

    if (request->Action == NamingTcpMessage::RegisterServiceNotificationFilterRequestAction)
    {
        RegisterServiceNotificationFilterRequestBody body;
        if (request->GetBody(body))
        {
            auto clientId = body.TakeClientId();
            auto filter = body.TakeFilter();

            // Notifications can flow to client as soon as registrations complete, which
            // implies that the client may start seeing notifications before the registration
            // even returns
            //
            error = this->RegisterFilter(
                activityId, 
                clientRegn, 
                move(clientId), 
                move(filter));

            if (error.IsSuccess())
            {
                GenerationNumber cacheGeneration;
                VersionRangeCollectionSPtr cacheVersions;

                serviceTableEntryCache_.GetCurrentVersions(cacheGeneration, cacheVersions);

                if (!cacheVersions)
                {
                    cacheVersions = make_shared<VersionRangeCollection>();
                }

                reply = NamingTcpMessage::GetRegisterServiceNotificationFilterReply()->GetTcpMessage();
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(request->GetStatus());
        }
    }
    else if (request->Action == NamingTcpMessage::UnregisterServiceNotificationFilterRequestAction)
    {
        UnregisterServiceNotificationFilterRequestBody body;
        if (request->GetBody(body))
        {
            auto clientId = body.TakeClientId();
            auto filterId = body.GetFilterId();

            if (this->UnregisterFilter(
                activityId, 
                clientRegn,
                clientId, 
                filterId))
            {
                reply = NamingTcpMessage::GetUnregisterServiceNotificationFilterReply()->GetTcpMessage();
            }
            else
            {
                error = ErrorCodeValue::ServiceNotificationFilterNotFound;
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(request->GetStatus());
        }
    }
    else if (request->Action == NamingTcpMessage::NotificationClientConnectionRequestAction)
    {
        NotificationClientConnectionRequestBody body;
        if (request->GetBody(body))
        {
            auto clientId = body.GetClientId();
            auto generation = body.GetGeneration();
            auto versions = body.TakeVersions();
            auto filters = body.TakeFilters();

            GenerationNumber cacheGeneration;
            int64 lastDeletedEmptyPartitionVersion;

            error = this->ConnectClient(
                activityId,
                clientRegn, 
                move(clientId), 
                generation, 
                move(versions),
                move(filters),
                cacheGeneration,
                lastDeletedEmptyPartitionVersion);

            if (error.IsSuccess())
            {
                reply = NamingTcpMessage::GetNotificationClientConnectionReply(
                    Common::make_unique<NotificationClientConnectionReplyBody>(
                        cacheGeneration, 
                        lastDeletedEmptyPartitionVersion,
                        this->CreateGatewayDescription()))->GetTcpMessage();
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(request->GetStatus());
        }
    }
    else if (request->Action == NamingTcpMessage::NotificationClientSynchronizationRequestAction)
    {
        NotificationClientSynchronizationRequestBody body;
        if (request->GetBody(body))
        {
            auto clientId = body.GetClientId();
            auto generation = body.GetGeneration();
            auto partitions = body.TakePartitions();

            GenerationNumber cacheGeneration;
            vector<int64> deletedVersions;

            error = this->SynchronizeClient(
                activityId,
                clientRegn, 
                move(clientId),
                generation, 
                move(partitions),
                cacheGeneration,
                deletedVersions);

            if (error.IsSuccess())
            {
                reply = NamingTcpMessage::GetNotificationClientSynchronizationReply(
                    Common::make_unique<NotificationClientSynchronizationReplyBody>(cacheGeneration, move(deletedVersions)))->GetTcpMessage();
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(request->GetStatus());
        }
    }
    else if (request->Action == NamingMessage::DisconnectClientAction)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Processing client disconnection for {0}",
            clientRegn.TargetId);

        this->RemoveClientRegistration(clientRegn);
        reply = NamingMessage::GetOperationSuccess(Actor::EntreeServiceProxy);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "{0} Invalid action {1}",
            activityId,
            request->Action);

        error = ErrorCodeValue::InvalidMessage;
    }

    return error;
}

ErrorCode ServiceNotificationManager::RegisterFilters(
    ActivityId const & activityId,
    ClientRegistrationSPtr const & registration,
    vector<ServiceNotificationFilterSPtr> const & filters)
{
    AcquireWriteLock lock(registrationsLock_);

    for (auto const & filter : filters)
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId,
            "{0}: filter re-registration from {1} (client={2}): {3}",
            activityId,
            registration->Id,
            registration->ClientId,
            *filter);

        auto error = this->RegisterFilterCallerHoldsLock(
            registration,
            filter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceNotificationManager::RegisterFilter(
    ActivityId const & activityId,
    ClientIdentityHeader const & clientRegn,
    wstring && clientId,
    ServiceNotificationFilterSPtr const & filterSPtr)
{
    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "{0}: filter registration from {1} (client={2}): {3}",
        activityId,
        clientRegn.TargetId,
        clientId,
        *filterSPtr);

    auto registration = make_shared<ClientRegistration>(
        this->TraceId,
        this->Transport,
        clientRegn,
        move(clientId),
        make_shared<VersionRangeCollection>(),
        [this](ClientIdentityHeader const & clientRegn) { this->RemoveClientRegistration(clientRegn); });

    AcquireWriteLock lock(registrationsLock_);

    clientTable_->TryAddOrGetExistingRegistration(registration);

    return this->RegisterFilterCallerHoldsLock(
        registration,
        filterSPtr);
}

ErrorCode ServiceNotificationManager::RegisterFilterCallerHoldsLock(
    ClientRegistrationSPtr const & registration,
    ServiceNotificationFilterSPtr const & filterSPtr)
{
    auto error = registration->TryAddFilter(filterSPtr);

    if (error.IsSuccess()) 
    { 
        error = this->GetNameFilterIndex(*filterSPtr)->TryAddFilter(registration, filterSPtr);
    }

    return error;
}

bool ServiceNotificationManager::UnregisterFilter(
    ActivityId const & activityId,
    ClientIdentityHeader const &clientIdentity,
    wstring const & clientId,
    uint64 filterId)
{
    AcquireWriteLock lock(registrationsLock_);

    auto registration = clientTable_->TryGetRegistration(clientIdentity, clientId);

    if (registration)
    {
        auto filter = registration->TryRemoveFilter(filterId);

        if (filter)
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId, 
                "{0}: unregistered filter: id={1} client={2} filter={3}",
                activityId,
                registration->Id,
                registration->ClientId,
                *filter);

            return this->GetNameFilterIndex(*filter)->TryRemoveFilter(registration, filter);
        }
    }

    return false;
}

ErrorCode ServiceNotificationManager::ConnectClient(
    ActivityId const & activityId,
    ClientIdentityHeader const & clientIdentity,
    wstring && clientId,
    GenerationNumber const & clientGeneration,
    VersionRangeCollectionSPtr && clientVersions,
    vector<ServiceNotificationFilterSPtr> && filters,
    __out GenerationNumber & cacheGeneration,
    __out int64 & lastDeletedEmptyPartitionVersion)
{
    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "{0}: connection from {1} (client={2}): gen={3} vers={4}",
        activityId,
        clientIdentity.TargetId,
        clientId,
        clientGeneration,
        (clientVersions ? *clientVersions : VersionRangeCollection()));

    ErrorCode error(ErrorCodeValue::Success);

    ClientRegistrationSPtr registration;

    if (this->TryRegisterNewClient(clientIdentity, move(clientId), registration))
    {
        // Must register filters before synchronizing with the cache since 
        // the cache might get updated with new entries immediately after
        // it's read for synchronization. Registering filters first will 
        // ensure that we do not miss these new incoming entries.
        //
        // However, note that any notifications created from such incoming
        // entries cannot be stamped with the full version ranges of the cache
        // because synchronization notifications may not have been delivered yet.
        //
        // To avoid blocking such notifications behind synchronization
        // notifications, we can stamp them with the actual versions
        // of the update instead. The trade-off is that the client will
        // observe more version holes until synchronization notifications
        // are received. ServiceNotificationSender encapsulates this logic.
        //
        // Filters must be re-registered atomically since an update
        // can come in as the filters are being re-registered.
        //
        error = this->RegisterFilters(activityId, registration, filters);
        if (error.IsSuccess())
        {
            this->ProcessServiceNotificationFilters(
                activityId,
                registration,
                clientGeneration,
                clientVersions,
                filters,
                cacheGeneration,
                lastDeletedEmptyPartitionVersion);

            registration->SetConnectionProcessingComplete();
        }
        else
        {
            this->RemoveClientRegistration(clientIdentity);
        }
    }
    else
    {
        if (registration->IsConnectionProcessingComplete())
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: connection protocol already complete from {1}: client={2}",
                activityId,
                clientIdentity.TargetId,
                clientId);

            // This could be a retry from the client. Resend the update metadata
            // but do not process the filters again. Any resulting notifications
            // from the previous connection process will either be sent
            // and received by client eventually or this registration will expire.
            //
            serviceTableEntryCache_.GetUpdateMetadata(
                cacheGeneration,
                lastDeletedEmptyPartitionVersion);
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: connection protocol still pending from {1}: client={2}",
                activityId,
                clientIdentity.TargetId,
                clientId);

            error = ErrorCodeValue::OperationCanceled;
        }
    }

    return error;
}

ErrorCode ServiceNotificationManager::SynchronizeClient(
    ActivityId const & activityId,
    ClientIdentityHeader const &clientIdentity,
    wstring && clientId,
    GenerationNumber const & clientGeneration,
    vector<VersionedConsistencyUnitId> && partitionsToCheck,
    __out GenerationNumber & cacheGeneration,
    __out vector<int64> & deletedVersions)
{
    serviceTableEntryCache_.GetDeletedVersions(
        clientGeneration,
        partitionsToCheck,
        cacheGeneration,
        deletedVersions);

    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "{0}: synchronizing {1} (client={2}): gen={3} deleted={4}",
        activityId,
        clientIdentity.TargetId,
        clientId,
        cacheGeneration,
        deletedVersions.empty() 
            ? L"[]"
            : wformatString("[{0}, {1}] ({2})", 
                deletedVersions.front(), 
                deletedVersions.back(), 
                deletedVersions.size()));

    return ErrorCodeValue::Success;
}

void ServiceNotificationManager::RemoveClientRegistration(ClientIdentityHeader const & client)
{
    AcquireWriteLock lock(registrationsLock_);

    RemoveClientRegistrationCallerHoldsLock(client);
}

void ServiceNotificationManager::RemoveClientRegistrationCallerHoldsLock(ClientIdentityHeader const & client)
{
    auto registration = clientTable_->TryRemoveRegistration(client, L"");

    if (registration)
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId, 
            "removing registration: id={0} client={1}",
            registration->Id,
            registration->ClientId);

        for (auto it : registration->Filters)
        {
            this->GetNameFilterIndex(*(it.second))->TryRemoveFilter(registration, it.second);
        }
    }
}

void ServiceNotificationManager::OnConnectionFault(ErrorCode const & error)
{
    UNREFERENCED_PARAMETER(error);

    WriteInfo(
        TraceComponent,
        TraceId,
        "Received connection fault from the ipc channel : {0}",
        error);
    this->RemoveClientRegistrations();
}

void ServiceNotificationManager::ProcessServiceTableEntries(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollection const & cacheVersions,
    vector<CachedServiceTableEntrySPtr> const & cachePartitions,
    VersionRangeCollection const & updateVersions)
{
    auto const & notificationId = activityId;

    MatchedClientRegistrationsTable matchedRegistrations;

    {
        AcquireReadLock lock(registrationsLock_);

        if (exactFilterIndex_->IsEmpty() && prefixFilterIndex_->IsEmpty())
        {
            WriteNoise(
                TraceComponent, 
                this->TraceId, 
                "{0}: skipping update: cache.gen={1} cache.vers={2} count={3} update.vers={4}", 
                notificationId,
                cacheGeneration, 
                cacheVersions,
                cachePartitions.size(),
                updateVersions);

            return;
        }

        trace_.PushNotificationProcessing(
            this->TraceId,
            notificationId,
            cacheGeneration, 
            cacheVersions,
            cachePartitions.size(),
            updateVersions);

        for (auto const & cachePartition : cachePartitions)
        {
            NamingUri uri;
            if (!NamingUri::TryParse(cachePartition->ServiceName, uri))
            {
                trace_.PushNotificationSkipped(
                    this->TraceId, 
                    notificationId, 
                    cachePartition->ServiceName);

                continue;
            }

            auto exactMatches = this->GetExactMatches(uri, cachePartition);

            auto prefixMatches = this->GetPrefixMatches(uri, cachePartition);

            if (!exactMatches.empty() || !prefixMatches.empty())
            {
                trace_.PushNotificationMatched(
                    this->TraceId, 
                    notificationId,
                    uri,
                    exactMatches.size(),
                    prefixMatches.size(),
                    *(cachePartition->Partition));

                matchedRegistrations.UpdateMatchingRegistrations(
                    exactMatches, 
                    cacheGeneration, 
                    cacheVersions, 
                    updateVersions,
                    clientTable_);

                matchedRegistrations.UpdateMatchingRegistrations(
                    prefixMatches, 
                    cacheGeneration, 
                    cacheVersions, 
                    updateVersions,
                    clientTable_);
            }
            else
            {
                WriteNoise(
                    TraceComponent, 
                    this->TraceId, 
                    "{0}: unmatched update ({1}) {2}",
                    notificationId,
                    uri,
                    *(cachePartition->Partition));
            }

        } // for each partition
    } // registrations lock 

    // In practice, the notification built here does not need to be paged since
    // it resulted from processing of a single broadcast message.
    //
    matchedRegistrations.CreateAndEnqueueNotifications(notificationId);
}

bool ServiceNotificationManager::TryRegisterNewClient(
    ClientIdentityHeader const &clientIdentity,
    wstring && clientId,
    __out ClientRegistrationSPtr & registration)
{
    registration = make_shared<ClientRegistration>(
        this->TraceId,
        this->Transport,
        clientIdentity,
        move(clientId),
        make_shared<VersionRangeCollection>(),
        [this](ClientIdentityHeader const & clientIdentity) { this->RemoveClientRegistration(clientIdentity); });

    {
        AcquireWriteLock lock(registrationsLock_);

        return clientTable_->TryAddOrGetExistingRegistration(registration);
    }
}

void ServiceNotificationManager::ProcessServiceNotificationFilters(
    ActivityId const & activityId,
    ClientRegistrationSPtr const & registration,
    GenerationNumber const & clientGeneration,
    VersionRangeCollectionSPtr const & clientVersions,
    vector<ServiceNotificationFilterSPtr> const & filters,
    __out GenerationNumber & cacheGeneration,
    __out int64 & lastDeletedEmptyPartitionVersion)
{
    {
        // Searching the cache for missed updates and enqueue of any
        // results must occur under the cache lock to avoid races with
        // cache update processing (ProcessServiceTableEntries).
        //
        // GetUpdates()/Enqueue() and ProcessServiceTableEntries()/Enqueue()
        // cannot be interleaved or the version information sent by the
        // gateway to the client will not be correct.
        //
        AcquireReadLock lock(serviceTableEntryCache_.GetCacheLock());

        VersionRangeCollectionSPtr cacheVersions;
        auto partitionsByLookupVersion = serviceTableEntryCache_.GetUpdatesCallerHoldsCacheLock(
            activityId,
            *registration,
            filters,
            clientGeneration,
            clientVersions,
            cacheGeneration,
            cacheVersions,
            lastDeletedEmptyPartitionVersion);

        if (!cacheVersions)
        {
            cacheVersions = make_shared<VersionRangeCollection>();
        }

        // Always send at least one notification (even if empty) to update the client's
        // known versions during connection
        //
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "{0}: updating connecting client: id={1} gen={2} vers={3} lastDelete={4} matched={5}",
            activityId,
            registration->Id,
            cacheGeneration,
            *cacheVersions,
            lastDeletedEmptyPartitionVersion,
            partitionsByLookupVersion.size());

        auto matchedRegistration = make_shared<MatchedClientRegistration>(
            registration, 
            cacheGeneration,
            *cacheVersions,
            move(partitionsByLookupVersion));

        matchedRegistration->CreateAndEnqueueNotificationForClientSynchronization(activityId);
    }
}

ServiceNotificationManager::MatchResultList ServiceNotificationManager::GetExactMatches(
    NamingUri const & uri,
    CachedServiceTableEntrySPtr const & cachePartition)
{
    MatchResultList results;

    NameFilterIndexEntrySPtr indexEntry;
    if (!exactFilterIndex_->IsEmpty() && exactFilterIndex_->TryGetFilters(uri, indexEntry))
    {
        for (auto const & it : indexEntry->FiltersByRegistrationId)
        {
            results.push_back(make_pair(it.first, MatchedServiceTableEntry::CreateOnUpdateMatch(cachePartition, it.second->IsPrimaryOnly)));
        }
    }

    return move(results);
}

ServiceNotificationManager::MatchResultList ServiceNotificationManager::GetPrefixMatches(
    NamingUri const & uri,
    CachedServiceTableEntrySPtr const & cachePartition)
{
    MatchResultList results;
    auto prefixUri = uri;
    auto parentUri = uri.GetParentName();
    bool done = false;

    while (!done)
    {
        NameFilterIndexEntrySPtr indexEntry;
        if (!prefixFilterIndex_->IsEmpty() && prefixFilterIndex_->TryGetFilters(prefixUri, indexEntry))
        {
            for (auto const & it : indexEntry->FiltersByRegistrationId)
            {
                results.push_back(make_pair(it.first, MatchedServiceTableEntry::CreateOnUpdateMatch(cachePartition, it.second->IsPrimaryOnly)));
            }
        }

        if (prefixUri == parentUri)
        {
            done = true;
        }
        else
        {
            prefixUri = parentUri;
            parentUri = prefixUri.GetParentName();
        }
    }

    return move(results);
}

unique_ptr<ServiceNotificationManager::NameFilterIndex> const & ServiceNotificationManager::GetNameFilterIndex(
    ServiceNotificationFilter const & filter) const
{
    if (filter.IsPrefix)
    {
        return prefixFilterIndex_;
    }
    else
    {
        return exactFilterIndex_;
    }
}

GatewayDescription ServiceNotificationManager::CreateGatewayDescription() const
{
    return GatewayDescription(
        L"", // no public endpoint
        nodeInstance_,
        nodeName_);
}

void ServiceNotificationManager::RemoveClientRegistrations()
{
    AcquireWriteLock lock(registrationsLock_);
    
    // TODO special case passthrough transport
    clientTable_->Clear();
    exactFilterIndex_->Clear();
    prefixFilterIndex_->Clear();
}

