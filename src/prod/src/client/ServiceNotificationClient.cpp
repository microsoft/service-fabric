// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceGroup;
using namespace ServiceModel;
using namespace Reliability;
using namespace Transport;
using namespace Naming;

using namespace Client;
using namespace ClientServerTransport;

const ClientEventSource ServiceNotificationClient::Trace;

StringLiteral const TraceComponent("ServiceNotificationClient");

// GatewaySynchronizationContext
//
// The gateway keeps empty ("deleted") partitions indexed in its cache
// in order to notify re-connecting clients of partition deletions.
// However, the gateway must eventually clean up empty partition entries
// or state would accumulate unbounded in the face of service deletions.
// The gateway is configured to only keep the last N empty partitions indexed
// and trim the rest (in version order) to avoid this.
//
// Suppose a client with known versions Vc connects to a gateway with known
// versions Vg and the version of the last empty partition deleted by the
// gateway is vd. vd must necessarily be in Vg and all empty partitions
// of version > vd are still indexed in the gateway cache, so the client
// will just receive these as normal notifications upon connecting.
//
// If vd is contained in Vc, then the client has not missed any deleted
// empty partition updates. Otherwise, there could have been an
// empty partition update in Vd = [EndVersion(Vc), vd] that the gateway no
// longer has in its index. The client needs to determine whether
// any non-empty notifications delivered in Vc have a corresponding
// empty update in Vd that was missed.
//
// The protocol for this is to have the client first retrieve vd from
// the gateway. If vd is not contained in Vc, then the client needs to send
// all versions for which a non-empty notification was delivered (Vn). Note
// that there is no benefit to merging this list into version ranges since it
// will contain mostly discrete versions. If the client sends Vn along with
// the CUID associated with each version, then the subset of Vn representing
// missed deletes (Vm) will be all versions for which there is no longer
// an entry at the gateway for the associated CUID. The client can
// raise an empty partition notification for all such CUIDs still at versions
// in Vm.
//
// Note that the client cannot accept changes to Vc until this protocol has
// completed since Vc is used to determine whether the protocol is needed
// at all. This means that the client must accept notifications from
// re-registered filters only after this protocol has completed. Furthermore, 
// this protocol itself also does not change Vc since the gateway does 
// not know the actual version of the empty partition update.
//

class ServiceNotificationClient::GatewaySynchronizationContext
    : public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
{
public:
    GatewaySynchronizationContext(__in ServiceNotificationClient & owner)
        : owner_(owner)
        , undeletedVersionsByCuid_()
        , undeletedPartitionsByVersion_()
        , lastSynchronizedVersion_(0)
        , lock_()
    {
    }

    void Clear()
    {
        undeletedVersionsByCuid_.clear();
        undeletedPartitionsByVersion_.clear();
        lastSynchronizedVersion_ = 0;
    }

    std::map<uint64, Naming::NotificationClientSynchronizationRequestBody> GetSynchronizationRequestPages(
        Common::ActivityId const & activityId,
        int64 lastDeletedEmptyPartitionVersion)
    {
        auto const & clientId = owner_.clientId_;
        auto const & clientGeneration = owner_.generation_;
        auto const & clientVersions = owner_.versions_;

        std::map<uint64, Naming::NotificationClientSynchronizationRequestBody> pages;

        auto pageSizeLimit = static_cast<size_t>(
            ServiceModel::ServiceModelConfig::GetConfig().MaxMessageSize * ServiceModel::ServiceModelConfig::GetConfig().MessageContentBufferRatio);

        size_t baseSizeEstimate = Naming::NotificationClientConnectionRequestBody::SerializationOverheadEstimate
            + (clientId.size() * sizeof(wchar_t));

        size_t currentPageSize = baseSizeEstimate;

        uint64 pageIndex = 0;
        std::vector<Reliability::VersionedConsistencyUnitId> undeletedPartitions;

        for (auto const & partition : this->GetUndeletedPartitionsToCheck(
            clientVersions,
            lastDeletedEmptyPartitionVersion))
        {
            auto partitionSizeEstimate = partition.EstimateSize();
            if (currentPageSize + partitionSizeEstimate > pageSizeLimit)
            {
                if (!undeletedPartitions.empty())
                {
                    auto index = ++pageIndex;

                    WriteNoise(
                        TraceComponent,
                        owner_.TraceId,
                        "{0}: paging synchronization: page={1} size={2}B limit={3}B next={4}B [{5}, {6}]({7})",
                        activityId,
                        index,
                        currentPageSize,
                        pageSizeLimit,
                        partitionSizeEstimate,
                        undeletedPartitions.front().Version,
                        undeletedPartitions.back().Version,
                        undeletedPartitions.size());

                    pages.insert(std::make_pair<uint64, Naming::NotificationClientSynchronizationRequestBody>(
                        std::move(index),
                        std::move(Naming::NotificationClientSynchronizationRequestBody(
                            clientId,
                            clientGeneration,
                            move(undeletedPartitions)))));

                    currentPageSize = baseSizeEstimate;
                }
            }

            currentPageSize += partitionSizeEstimate;
            undeletedPartitions.push_back(partition);
        }

        if (!undeletedPartitions.empty())
        {
            pages.insert(std::make_pair<uint64, Naming::NotificationClientSynchronizationRequestBody>(
                std::move(++pageIndex),
                std::move(Naming::NotificationClientSynchronizationRequestBody(
                    clientId,
                    clientGeneration,
                    move(undeletedPartitions)))));
        }

        return pages;
    }

    Reliability::ServiceTableEntrySPtr CreateEmptyServiceTableEntry(
        Common::ActivityId const & activityId, 
        uint64 pageIndex,
        int64 deletedVersion)
    {
        Reliability::ServiceTableEntrySPtr result;

        Common::AcquireExclusiveLock lock(lock_);

        if (lastSynchronizedVersion_ < deletedVersion)
        {
            lastSynchronizedVersion_ = deletedVersion;
        }

        auto findIt = undeletedPartitionsByVersion_.find(deletedVersion);
        if (findIt != undeletedPartitionsByVersion_.end())
        {
            result = findIt->second->CreateEmptyServiceTableEntry(deletedVersion);

            WriteInfo(
                TraceComponent, 
                owner_.TraceId,
                "{0}: notifying synchronized delete (page={1}): {2}",
                activityId,
                pageIndex,
                *result);

            undeletedPartitionsByVersion_.erase(deletedVersion);
            undeletedVersionsByCuid_.erase(result->ConsistencyUnitId);
        }
        
        return result;
    }

    bool TryUpdateUndeletedVersion(Reliability::ServiceTableEntry const & entry, bool isMatchedPrimaryOnly)
    {
        bool updated = true;

        Common::AcquireExclusiveLock lock(lock_);

        auto findIt = undeletedVersionsByCuid_.find(entry.ConsistencyUnitId);

        if (entry.ServiceReplicaSet.IsEmpty())
        {
            if (findIt != undeletedVersionsByCuid_.end())
            {
                if (findIt->second.Version < entry.Version)
                {
                    undeletedPartitionsByVersion_.erase(findIt->second.Version);
                    undeletedVersionsByCuid_.erase(findIt);
                }
                else
                {
                    // 1) A delete notification will or has already been fired
                    //    for this partition during synchronization.
                    //
                    // 2) This version is stale for this partition (but was
                    //    not covered in known versions).
                    // 
                    updated = false;
                }
            }
            else
            {
                // Intentional no-op (updated = true):
                //
                // Since newly registered filters are not retroactively applied 
                // to old cache entries, there is a special case where a filter is registered
                // after a service is created and that service never reconfigures.
                // If this client never sees a notification for the partition, then undeletedVersionsByCuid_ 
                // will not contain an entry. Fire the notification anyway in this case
                // to avoid missing delete notifications. Duplicates will be suppressed by versions_.
            }
        }
        else
        {
            if (findIt == undeletedVersionsByCuid_.end())
            {
                undeletedPartitionsByVersion_.insert(std::make_pair<int64, UndeletedPartitionDataSPtr>(
                    entry.Version,
                    std::make_shared<UndeletedPartitionData>(entry.ServiceName, entry.ConsistencyUnitId)));

                VersionData data(
                    entry.Version,
                    (isMatchedPrimaryOnly && entry.ServiceReplicaSet.IsPrimaryLocationValid)
                        ? entry.ServiceReplicaSet.PrimaryLocation
                        : L"");

                undeletedVersionsByCuid_.insert(std::make_pair<Reliability::ConsistencyUnitId, VersionData>(
                    entry.ConsistencyUnitId, 
                    std::move(data)));
            }
            else
            {
                int64 newVersion = entry.Version;
                UndeletedPartitionDataSPtr partitionData;

                auto it = undeletedPartitionsByVersion_.find(findIt->second.Version);
                if (it == undeletedPartitionsByVersion_.end())
                {
                    partitionData = std::make_shared<UndeletedPartitionData>(entry.ServiceName, entry.ConsistencyUnitId);
                }
                else
                {
                    partitionData = it->second;
                    undeletedPartitionsByVersion_.erase(it->first);
                }

                undeletedPartitionsByVersion_.insert(make_pair(move(newVersion), move(partitionData)));

                findIt->second.UpdateVersion(entry.Version);

                if (isMatchedPrimaryOnly && entry.ServiceReplicaSet.IsStateful)
                {
                    updated = findIt->second.TryUpdatePrimaryEndpoint(entry.ServiceReplicaSet.IsPrimaryLocationValid 
                            ? entry.ServiceReplicaSet.PrimaryLocation 
                            : L"");
                }
            }
        } 

        return updated;
    }

    int64 GetLastSynchronizedVersion()
    {
        return lastSynchronizedVersion_;
    }

    std::wstring GetUndeletedVersionsTrace()
    {
        return undeletedPartitionsByVersion_.empty() 
            ? L"[]" 
            : Common::wformatString("[{0}, {1}] ({2})", 
                    undeletedPartitionsByVersion_.begin()->first, 
                    undeletedPartitionsByVersion_.rbegin()->first,
                    undeletedPartitionsByVersion_.size());
    }

private:
    class UndeletedPartitionData : public Serialization::FabricSerializable
    {
    public:
        UndeletedPartitionData(
            std::wstring const & name, 
            Reliability::ConsistencyUnitId const & cuid)
            : name_(name)
            , cuid_(cuid)
        {
        }

        __declspec (property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return name_; }

        __declspec (property(get=get_Cuid)) Reliability::ConsistencyUnitId const & Cuid;
        Reliability::ConsistencyUnitId const & get_Cuid() const { return cuid_; }

        Reliability::ServiceTableEntrySPtr CreateEmptyServiceTableEntry(int64 deletedVersion)
        {
            // Note that the gateway no longer knows the actual version of
            // the delete so we use the same version as the update being
            // deleted.
            //
            Reliability::ServiceReplicaSet replicaSet(
                false, // isStateful: irrelevant
                false, // isPrimaryValid
                L"", // primaryLocation
                std::vector<std::wstring>(), // locations
                deletedVersion);

            return std::make_shared<Reliability::ServiceTableEntry>(
                cuid_,
                name_,
                std::move(replicaSet));
        }

    private:
        std::wstring name_;
        Reliability::ConsistencyUnitId cuid_;
    };

    class VersionData
    {
    public:
        VersionData(int64 version, std::wstring const & primaryEndpoint)
            : version_(version)
            , primaryEndpoint_(primaryEndpoint)
        {
        }

        VersionData(VersionData && other)
            : version_(std::move(other.version_))
            , primaryEndpoint_(move(other.primaryEndpoint_))
        {
        }

        __declspec (property(get=get_Version)) int64 Version;
        int64 get_Version() const { return version_; }

        void UpdateVersion(int64 version)
        {
            version_ = version;
        }

        bool TryUpdatePrimaryEndpoint(std::wstring const & endpoint)
        {
            if (primaryEndpoint_ != endpoint)
            {
                primaryEndpoint_ = endpoint;

                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        int64 version_;
        std::wstring primaryEndpoint_;
    };

    typedef std::shared_ptr<UndeletedPartitionData> UndeletedPartitionDataSPtr;
    typedef std::unordered_map<Reliability::ConsistencyUnitId, VersionData, Reliability::ConsistencyUnitId::Hasher> CuidVersionHash;
    typedef std::pair<int64, UndeletedPartitionDataSPtr> VersionedPartitionDataPair;
    typedef std::map<int64, UndeletedPartitionDataSPtr> VersionedPartitionDataMap;

    std::vector<Reliability::VersionedConsistencyUnitId> GetUndeletedPartitionsToCheck(
        Common::VersionRangeCollectionSPtr const & clientVersions,
        int64 lastDeletedEmptyPartitionVersion)
    {
        std::vector<Reliability::VersionedConsistencyUnitId> results;

        // Client is up-to-date with deleted partitions. No synchronization needed.
        //
        if (lastDeletedEmptyPartitionVersion == 0 || clientVersions->Contains(lastDeletedEmptyPartitionVersion))
        {
            // Tracking the last synchronized version is for debugging only
            //
            lastSynchronizedVersion_ = lastDeletedEmptyPartitionVersion;

            return results;
        }

        Common::AcquireExclusiveLock lock(lock_);

        for (auto const & it : undeletedPartitionsByVersion_)
        {
            results.push_back(Reliability::VersionedConsistencyUnitId(
                it.first,
                it.second->Cuid));
        }

        return results;
    }

    ServiceNotificationClient & owner_;
    CuidVersionHash undeletedVersionsByCuid_;
    VersionedPartitionDataMap undeletedPartitionsByVersion_;
    int64 lastSynchronizedVersion_;
    Common::ExclusiveLock lock_;
};

//
// RequestAsyncOperationBase - Scheduling retries and helpers for tracing
//

class ServiceNotificationClient::RequestAsyncOperationBase
    : public AsyncOperation
    , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
{
public:

    RequestAsyncOperationBase(
        __in ServiceNotificationClient & owner,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , activityId_(activityId)
        , timeoutHelper_(timeout)
        , retryTimer_()
        , timerLock_()
    {
    }

    __declspec (property(get=get_Owner)) ServiceNotificationClient & Owner;
    ServiceNotificationClient & get_Owner() const { return owner_; }

    __declspec (property(get=get_OwnerId)) std::wstring const & OwnerId;
    std::wstring const & get_OwnerId() const { return owner_.TraceId; }

    __declspec (property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
    Common::ActivityId const & get_ActivityId() const { return activityId_; }

    __declspec (property(get=get_Timeout)) TimeSpan RemainingTimeout;
    TimeSpan get_Timeout() const { return timeoutHelper_.GetRemainingTime(); }

protected:

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    void ScheduleRetryOrTimeout(
        AsyncOperationSPtr const & thisSPtr,
        wstring const & action,
        RetryCallback const & retryCallback,
        TimeSpan delay)
    {
        if (this->RemainingTimeout <= TimeSpan::Zero)
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
        else
        {
            if (delay > this->RemainingTimeout)
            {
                delay = this->RemainingTimeout;
            }

            WriteInfo(
                TraceComponent, 
                this->OwnerId,
                "{0}: scheduling {1} retry in {2}: remaining={3}",
                this->ActivityId,
                action,
                delay,
                this->RemainingTimeout);

            AcquireExclusiveLock lock(timerLock_);

            if (retryTimer_)
            {
                retryTimer_->Cancel();
            }

            retryTimer_ = Timer::Create("ServiceNotificationClientRetry", [thisSPtr, retryCallback](TimerSPtr const & timer)
            {
                timer->Cancel();

                retryCallback(thisSPtr);
            });
            retryTimer_->Change(delay);
        }
    }

private:
    ServiceNotificationClient & owner_;
    Common::ActivityId activityId_;

    TimeoutHelper timeoutHelper_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
};

//
// SynchronizeSessionAsyncOperation - client/gateway reconnection protocol for service notifications
//
// Two phases (short summary). See GatewaySynchronizationContext for details:
//
// 1) client sends registered filters to gateway
//      - gateway processes filters to generate missing notifications
//      - gateway replies with deleted cache entry watermark
//
// 2) if needed, client sends versions of undeleted CUIDs
//      - gateway replies with versions that should be deleted (no longer in cache)
//      - client synthesizes any required delete notifications
//

class ServiceNotificationClient::SynchronizeSessionAsyncOperation : public RequestAsyncOperationBase
{
public:
    SynchronizeSessionAsyncOperation(
        __in ServiceNotificationClient & owner,
        GatewayDescription const & connectedGateway,
        Common::ActivityId const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestAsyncOperationBase(owner, activityId, TimeSpan::MaxValue, callback, parent)
        , synchronizationContext_(*owner.gatewaySynchronizationContext_)
        , targetConnectedGateway_(connectedGateway)
        , actualConnectedGateway_()
        , sendStopwatch_()
        , synchronizationPages_()
        , synchronizationPagesLock_()
        , pendingSynchronizationPageCount_(0)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out GatewayDescription & targetConnectedGateway,
        __out GatewayDescription & actualConnectedGateway)
    {
        auto casted = AsyncOperation::End<SynchronizeSessionAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            targetConnectedGateway = casted->targetConnectedGateway_;
            actualConnectedGateway = casted->actualConnectedGateway_;
        }
        
        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->SendFilters(thisSPtr);
    }

private:

    bool TryCancelIfStaleGateway(AsyncOperationSPtr const & thisSPtr)
    {
        GatewayDescription currentGateway;
        if (!this->Owner.clientConnection_.IsCurrentGateway(targetConnectedGateway_, currentGateway))
        {
            // If the gateway from the connection event no longer matches
            // the currently connected gateway, then stop this connection
            // operation.
            //
            // A new operation will start (or has already started) against
            // the up-to-date gateway.
            //
            // In very tight connect-disconnect-connect events, we may
            // still end up with multiple connection protocols against the
            // same gateway, but that is handled on the gateway side.
            //
            WriteInfo(
                TraceComponent, 
                this->OwnerId,
                "{0}: gateway {1} not current {2}",
                this->ActivityId,
                targetConnectedGateway_,
                currentGateway);

            this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);

            return true;
        }

        return false;
    }

    void SendFilters(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->TryCancelIfStaleGateway(thisSPtr)) { return; }

        WriteInfo(
            TraceComponent, 
            this->OwnerId,
            "{0}: collecting synchronization filters",
            this->ActivityId);

        vector<uint64> filterIds;
        vector<ServiceNotificationFilterSPtr> filters;
        GenerationNumber generation;
        shared_ptr<VersionRangeCollection> versions;
        {
            AcquireReadLock lock(this->Owner.lifecycleLock_);

            generation = this->Owner.generation_;

            for (auto const & it : this->Owner.registeredFilters_)
            {
                filterIds.push_back(it.first);
                filters.push_back(it.second);
            }

            versions = this->Owner.versions_;
        }

        //
        // Send connection request even if we have no filters
        // so that the gateway can no-op the synchronization
        // and put the client into the synchronized state.
        //
        
        WriteInfo(
            TraceComponent, 
            this->OwnerId,
            "{0}: connecting to target gateway {1} with: filters={2} gen={3} vers={4} priority={5}",
            this->ActivityId,
            targetConnectedGateway_,
            filterIds,
            generation,
            *versions,
            TransportPriority::High);

        auto body = Common::make_unique<NotificationClientConnectionRequestBody>(
            this->Owner.ClientId,
            generation,
            move(versions),
            move(filters));

        sendStopwatch_.Restart();

        auto request = NamingTcpMessage::GetNotificationClientConnectionRequest(std::move(body));
        request->Headers.Replace(FabricActivityHeader(this->ActivityId));

        auto operation = this->Owner.clientConnection_.BeginSendHighPriorityToGateway( 
            move(request),
            this->Owner.ClientSettings->NotificationGatewayConnectionTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnConnectionRequestComplete(operation, false); },
            thisSPtr);
        this->OnConnectionRequestComplete(operation, true);
    }

    void OnConnectionRequestComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ClientServerReplyMessageUPtr reply;
        auto error = this->Owner.clientConnection_.EndSendToGateway(operation, reply);

        if (error.IsSuccess())
        {
            NotificationClientConnectionReplyBody body;
            if (reply->GetBody(body))
            {
                auto cacheGeneration = body.GetGeneration();
                auto lastDeletedEmptyPartitionVersion = body.GetLastDeletedEmptyPartitionVersion();
                actualConnectedGateway_ = body.GetGatewayDescription();

                //
                // The underlying connection manager will ping and connect to the send target on a
                // lower priority transport connection. Ideally, the high priority transport connection
                // on the same send target should end up at the same node. However, with our current
                // TCP transport implementation (which uses a separate TCP connection to separate
                // priorities), the Azure load balancer will likely balance the high priority
                // connection to a different actual node. Keep track of the actual node
                // for debugging purposes.
                //

                WriteInfo(
                    TraceComponent, 
                    this->OwnerId,
                    "{0}: connected to actual gateway {1}: priority={2}",
                    this->ActivityId,
                    actualConnectedGateway_,
                    TransportPriority::High);

                this->StartSynchronization(thisSPtr, cacheGeneration, lastDeletedEmptyPartitionVersion);
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        if (!error.IsSuccess())
        {
            this->ScheduleSendFilters(thisSPtr);
        }
    }

    void StartSynchronization(
        AsyncOperationSPtr const & thisSPtr,
        GenerationNumber const & cacheGeneration,
        int64 lastDeletedEmptyPartitionVersion)
    {
        WriteInfo(
            TraceComponent, 
            this->OwnerId,
            "{0}: synchronizing: gen={1} lastDelete={2} lastSync={3} undeleted={4}",
            this->ActivityId,
            cacheGeneration,
            lastDeletedEmptyPartitionVersion,
            synchronizationContext_.GetLastSynchronizedVersion(),
            synchronizationContext_.GetUndeletedVersionsTrace());

        if (cacheGeneration == this->Owner.generation_)
        {
            {
                AcquireExclusiveLock lock(synchronizationPagesLock_);

                map<uint64, NotificationClientSynchronizationRequestBody> temp(move(synchronizationContext_.GetSynchronizationRequestPages(
                    this->ActivityId,
                    lastDeletedEmptyPartitionVersion)));
                synchronizationPages_.swap(temp);
            }

            this->SendSynchronizationPages(thisSPtr);
        }
        else
        {
            this->OnSynchronizationComplete(thisSPtr);
        }
    }

    void SendSynchronizationPages(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->TryCancelIfStaleGateway(thisSPtr)) { return; }

        vector<pair<uint64,shared_ptr<ClientServerRequestMessageUPtr>>> requests;
        {
            AcquireExclusiveLock lock(synchronizationPagesLock_);

            pendingSynchronizationPageCount_.store(static_cast<LONG>(synchronizationPages_.size()));

            for (auto const & it : synchronizationPages_)
            {
                auto pageIndex = it.first;
                auto const & page = it.second;

                auto request = make_shared<ClientServerRequestMessageUPtr>(
                    NamingTcpMessage::GetNotificationClientSynchronizationRequest(
                        make_unique<NotificationClientSynchronizationRequestBody>(page)));

                requests.push_back(make_pair(pageIndex, move(request)));
            }
        }

        WriteInfo(
            TraceComponent, 
            this->OwnerId,
            "{0}: sending {1} synchronization pages",
            this->ActivityId,
            requests.size());

        if (!requests.empty())
        {
            sendStopwatch_.Restart();

            for (auto const & it : requests)
            {
                auto pageIndex = it.first;
                auto const & requestSPtr = it.second;
                auto & request = *requestSPtr;

                request->Headers.Replace(FabricActivityHeader(this->ActivityId));

                auto operation = this->Owner.clientConnection_.BeginSendHighPriorityToGateway( 
                    move(request),
                    this->Owner.ClientSettings->NotificationGatewayConnectionTimeout,
                    [this, pageIndex](AsyncOperationSPtr const & operation) { this->OnSynchronizationRequestComplete(pageIndex, operation, false); },
                    thisSPtr);
                this->OnSynchronizationRequestComplete(pageIndex, operation, true);
            }
        }
        else
        {
            this->OnSynchronizationComplete(thisSPtr);
        }
    }

    void OnSynchronizationRequestComplete(
        uint64 pageIndex,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ClientServerReplyMessageUPtr reply;
        auto error = this->Owner.clientConnection_.EndSendToGateway(operation, reply);

        if (error.IsSuccess())
        {
            NotificationClientSynchronizationReplyBody body;
            if (reply->GetBody(body))
            {
                auto deletedVersions = body.TakeVersions();

                vector<ServiceNotificationResultSPtr> updates;

                for (auto version : deletedVersions)
                {
                    auto entry = synchronizationContext_.CreateEmptyServiceTableEntry(
                        this->ActivityId, 
                        pageIndex,
                        version);

                    if (entry)
                    {
                        updates.push_back(make_shared<ServiceNotificationResult>(
                            move(entry),
                            this->Owner.generation_));
                    }
                }

                this->Owner.PostNotifications(move(updates));

                AcquireExclusiveLock lock(synchronizationPagesLock_);

                synchronizationPages_.erase(pageIndex);
            }
        }

        if (--pendingSynchronizationPageCount_ == 0)
        {
            auto hasMorePages = false;
            {
                AcquireExclusiveLock lock(synchronizationPagesLock_);
                
                hasMorePages = !synchronizationPages_.empty();
            }

            if (hasMorePages)
            {
                this->ScheduleSendSynchronizationPages(thisSPtr);
            }
            else
            {
                this->OnSynchronizationComplete(thisSPtr);
            }
        }
    }

    void OnSynchronizationComplete(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->Owner.TryDrainPendingNotificationsOnSynchronized(this->ActivityId, targetConnectedGateway_))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        }
    }

    void ScheduleSendFilters(AsyncOperationSPtr const & thisSPtr)
    {
        this->ScheduleRetryOrTimeout(
            thisSPtr,
            L"SendFilters",
            [this](AsyncOperationSPtr const & thisSPtr)
            {
                this->SendFilters(thisSPtr);
            },
            this->GetRetryDelay());
    }

    void ScheduleSendSynchronizationPages(AsyncOperationSPtr const & thisSPtr)
    {
        this->ScheduleRetryOrTimeout(
            thisSPtr,
            L"SendSynchronizationPages",
            [this](AsyncOperationSPtr const & thisSPtr)
            {
                this->SendSynchronizationPages(thisSPtr);
            },
            this->GetRetryDelay());
    }

    TimeSpan GetRetryDelay()
    {
        auto delay = this->Owner.ClientSettings->NotificationGatewayConnectionTimeout;
        delay = delay.SubtractWithMaxAndMinValueCheck(sendStopwatch_.Elapsed);
        if (delay < TimeSpan::Zero) { delay = TimeSpan::Zero; }

        return delay;
    }

    GatewaySynchronizationContext & synchronizationContext_;
    GatewayDescription targetConnectedGateway_;
    GatewayDescription actualConnectedGateway_;

    Stopwatch sendStopwatch_;

    map<uint64, NotificationClientSynchronizationRequestBody> synchronizationPages_;
    ExclusiveLock synchronizationPagesLock_;
    Common::atomic_long pendingSynchronizationPageCount_;
};

//
// FilterAsyncOperationBase - Detects whether the gateway connection protocol needs
//                            to be run on-demand. Triggered lazily by first
//                            registered filter.
//

class ServiceNotificationClient::FilterAsyncOperationBase : public RequestAsyncOperationBase
{
public:
    FilterAsyncOperationBase(
        __in ServiceNotificationClient & owner,
        Common::ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestAsyncOperationBase(owner, activityId, timeout, callback, parent)
    {
    }

protected:

    bool ShouldWaitForSynchronization()
    {
        bool isSynchronized = false;
        bool startSynchronizing = false;
        GatewayDescription currentConnectedGateway;

        {
            AcquireWriteLock lock(this->Owner.lifecycleLock_);

            isSynchronized = this->Owner.isSynchronized_;

            // Trigger synchronization if this happens to be the first filter
            // we are registering. We can also just start synchronizing
            // unconditionally (OnGatewayConnected), but that would unnecessarily 
            // create an extra TCP connection for clients that may never
            // use the notification feature.
            //
            if (!isSynchronized && this->Owner.registeredFilters_.empty())
            {
                if (this->Owner.clientConnection_.TryGetCurrentGateway(currentConnectedGateway))
                {
                    if (this->Owner.targetGateway_ != currentConnectedGateway)
                    {
                        this->Owner.targetGateway_ = currentConnectedGateway;

                        startSynchronizing = true;
                    }
                }
            }
        }

        if (startSynchronizing)
        {
            this->Owner.StartSynchronizingSession(currentConnectedGateway, this->ActivityId);
        }

        return !isSynchronized;
    }
};

//
// RegisterFilterAsyncOperation
//

class ServiceNotificationClient::RegisterFilterAsyncOperation : public FilterAsyncOperationBase
{
public:
    RegisterFilterAsyncOperation(
        __in ServiceNotificationClient & owner,
        Common::ActivityId const & activityId,
        ServiceNotificationFilterSPtr const & filter,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : FilterAsyncOperationBase(owner, activityId, timeout, callback, parent)
        , filter_(filter)
        , filterId_(0)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out uint64 & filterId)
    {
        auto casted = AsyncOperation::End<RegisterFilterAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            filterId = casted->filterId_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->CheckGatewaySynchronization(thisSPtr);
    }

    void CheckGatewaySynchronization(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->ShouldWaitForSynchronization())
        {
            this->ScheduleRetryOrTimeout(
                thisSPtr,
                L"Register-CheckGateway",
                [this](AsyncOperationSPtr const & thisSPtr) 
                { 
                    this->CheckGatewaySynchronization(thisSPtr); 
                },
                this->Owner.ClientSettings->ConnectionInitializationTimeout);
        }
        else
        {
            this->RegisterFilter(thisSPtr);
        }
    }

    void RegisterFilter(AsyncOperationSPtr const & thisSPtr)
    {
        filterId_ = this->Owner.GetNextFilterId();

        filter_->SetFilterId(filterId_);

        WriteInfo(
            TraceComponent,
            this->OwnerId,
            "{0}: registering filter ({1}) at {2}",
            this->ActivityId,
            *filter_,
            this->Owner.CurrentGateway);

        auto body = Common::make_unique<RegisterServiceNotificationFilterRequestBody>(
            this->Owner.ClientId,
            filter_);

        auto request = NamingTcpMessage::GetRegisterServiceNotificationFilterRequest(std::move(body));
        request->Headers.Replace(FabricActivityHeader(this->ActivityId));

        auto operation = this->Owner.clientConnection_.BeginSendHighPriorityToGateway( 
            move(request),
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnSendToGatewayComplete(operation, false); },
            thisSPtr);
        this->OnSendToGatewayComplete(operation, true);
    }

private:
    void OnSendToGatewayComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ClientServerReplyMessageUPtr reply;
        auto error = this->Owner.clientConnection_.EndSendToGateway(operation, reply);

        if (error.IsError(ErrorCodeValue::ServiceNotificationFilterAlreadyExists))
        {
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            this->Owner.AddFilter(filter_);
        }

        this->TryComplete(operation->Parent, error);
    }

    ServiceNotificationFilterSPtr filter_;
    uint64 filterId_;
};

//
// UnregisterFilterAsyncOperation
//

class ServiceNotificationClient::UnregisterFilterAsyncOperation : public FilterAsyncOperationBase
{
public:
    UnregisterFilterAsyncOperation(
        __in ServiceNotificationClient & owner,
        Common::ActivityId const & activityId,
        uint64 filterId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : FilterAsyncOperationBase(owner, activityId, timeout, callback, parent)
        , filterId_(filterId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UnregisterFilterAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->CheckGatewaySynchronization(thisSPtr);
    }

    void CheckGatewaySynchronization(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->ShouldWaitForSynchronization())
        {
            this->ScheduleRetryOrTimeout(
                thisSPtr,
                L"Unregister-CheckGateway",
                [this](AsyncOperationSPtr const & thisSPtr) 
                { 
                    this->CheckGatewaySynchronization(thisSPtr); 
                },
                this->Owner.ClientSettings->ConnectionInitializationTimeout);
        }
        else
        {
            this->UnregisterFilter(thisSPtr);
        }
    }

    void UnregisterFilter(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            this->OwnerId,
            "{0}: unregistering filter ({1}) at {2}",
            this->ActivityId,
            filterId_,
            this->Owner.CurrentGateway);

        auto body = Common::make_unique<UnregisterServiceNotificationFilterRequestBody>(
            this->Owner.ClientId,
            filterId_);

        auto request = NamingTcpMessage::GetUnregisterServiceNotificationFilterRequest(std::move(body));
        request->Headers.Replace(FabricActivityHeader(this->ActivityId));

        auto operation = this->Owner.clientConnection_.BeginSendHighPriorityToGateway( 
            move(request),
            this->RemainingTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnSendToGatewayComplete(operation, false); },
            thisSPtr);
        this->OnSendToGatewayComplete(operation, true);
    }

private:
    void OnSendToGatewayComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ClientServerReplyMessageUPtr unusedReply;
        auto error = this->Owner.clientConnection_.EndSendToGateway(operation, unusedReply);

        if (error.IsError(ErrorCodeValue::ServiceNotificationFilterNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            this->Owner.RemoveFilter(filterId_);
        }

        this->TryComplete(operation->Parent, error);
    }

    uint64 filterId_;
};

//
// ProcessNotificationAsyncOperation
//

class ServiceNotificationClient::ProcessNotificationAsyncOperation : public AsyncOperation
{
public:
    ProcessNotificationAsyncOperation(
        __in ServiceNotificationClient & owner,
        __in Message & request,
        __in ReceiverContextUPtr & receiverContext,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , receiverContext_(move(receiverContext))
        , notification_()
        , error_()
    {
        ServiceNotificationRequestBody body;
        if (request.GetBody(body))
        {
            notification_ = body.TakeNotification();
        }
        else
        {
            error_ = ErrorCode::FromNtStatus(request.GetStatus()); 
        }
    }

    static void End(AsyncOperationSPtr const & operation)
    {
        AsyncOperation::End<ProcessNotificationAsyncOperation>(operation);
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!error_.IsSuccess())
        {
            owner_.WriteError(
                TraceComponent,
                owner_.TraceId,
                "failed to deserialize body - dropping notification: messageId={0} error={1}",
                receiverContext_->MessageId,
                error_);

            this->SendReplyAndComplete(thisSPtr);
        }
        else
        {
            auto operation = owner_.clientCache_.BeginUpdateCacheEntries(
                *notification_,
                owner_.ClientSettings->NotificationCacheUpdateTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnUpdateCacheEntriesComplete(operation, false); },
                thisSPtr);
            this->OnUpdateCacheEntriesComplete(operation, true);
        }
    }

private:

    void OnUpdateCacheEntriesComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        // The client cache will need to build an RSP for the cache entry, so re-use that
        // RSP here for the notification. In the future, if we support disabling the cache
        // independent of notifications, then all we have to do is build the
        // ServiceNotificationResults here without the partition info.
        //
        // Processing the notification through the cache will clear the list of STEs
        // from the notification itself since we no longer need them, we only need
        // the processed results. The only purpose for using the resulting RSP
        // is to include PartitionInfo whenever possible in the notification itself.
        //
        // PartitionInfo is not guaranteed to be available in all cases. For example,
        // the PSD no longer exists after delete.
        //
        vector<ServiceNotificationResultSPtr> notificationResults;
        error_ = owner_.clientCache_.EndUpdateCacheEntries(operation, notificationResults);

        if (error_.IsSuccess())
        {
            owner_.AcceptAndPostNotificationsIfSynchronized(
                move(notification_),
                move(notificationResults));
        }
        
        this->SendReplyAndComplete(thisSPtr);
    }

    void SendReplyAndComplete(AsyncOperationSPtr const & thisSPtr)
    {
        receiverContext_->Reply(
            NamingTcpMessage::GetServiceNotificationReply(
                Common::make_unique<ServiceNotificationReplyBody>(error_))->GetTcpMessage(),
            TransportPriority::High);

        // Notification processing happens in the background - no one
        // will observe the completion error.
        //
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    ServiceNotificationClient & owner_;
    ReceiverContextUPtr receiverContext_;

    ServiceNotificationSPtr notification_;
    ErrorCode error_;
};

//
// ServiceNotificationClient
//

ServiceNotificationClient::ServiceNotificationClient(
    wstring const & clientId,
    INotificationClientSettingsUPtr && settings,
    __in ClientConnectionManager & clientConnection,
    __in INotificationClientCache & clientCache,
    NotificationHandler const & handler,
    ComponentRoot const & root)
    : RootedObject(root)
    , clientId_(clientId)
    , settings_(move(settings))
    , clientConnection_(clientConnection)
    , clientCache_(clientCache)
    , notificationHandler_(handler)
    , handlerLock_()
    , targetGateway_()
    , currentGateway_()
    , nextFilterId_(1)
    , registeredFilters_()
    , generation_()
    , versions_(make_shared<VersionRangeCollection>())
    , gatewaySynchronizationContext_()
    , pendingNotifications_()
    , isSynchronized_(false)
    , lifecycleLock_()
    , connectionHandlersId_(0)
{
    gatewaySynchronizationContext_ = make_unique<GatewaySynchronizationContext>(*this);
}

ServiceNotificationClient::~ServiceNotificationClient()
{
}

ErrorCode ServiceNotificationClient::OnOpen()
{
    NotificationClientSynchronizationRequestBody::InitializeSerializationOverheadEstimate();

    clientConnection_.UpdateTraceId(this->TraceId);

    auto root = this->Root.CreateComponentRoot();

    connectionHandlersId_ = clientConnection_.RegisterConnectionHandlers(
        [this, root](ClientConnectionManager::HandlerId h, ISendTarget::SPtr const & st, GatewayDescription const & g)
        {
            this->OnGatewayConnected(h, st, g);
        },
        [this, root](ClientConnectionManager::HandlerId, ISendTarget::SPtr const &, GatewayDescription const &, ErrorCode const &)
        {
            // intentional no-op
        },
        [this, root](ClientConnectionManager::HandlerId, shared_ptr<ClaimsRetrievalMetadata> const &, __out wstring &)
        {
            return ErrorCodeValue::NotImplemented;
        });

    auto error = clientConnection_.Open();

    if (error.IsSuccess())
    {
        error = clientConnection_.SetNotificationHandler(
            [this, root](Message & request, ReceiverContextUPtr & receiverContext)
            {
                this->OnNotificationReceived(request, receiverContext);
            });
    }

    return error;
}

ErrorCode ServiceNotificationClient::OnClose()
{
    clientConnection_.RemoveNotificationHandler();

    auto error = clientConnection_.Close();

    clientConnection_.UnregisterConnectionHandlers(connectionHandlersId_);

    AcquireWriteLock lock(handlerLock_);

    notificationHandler_ = nullptr;

    return error;
}

void ServiceNotificationClient::OnAbort()
{
    this->OnClose();
}

AsyncOperationSPtr ServiceNotificationClient::BeginRegisterFilter(
    ActivityId const & activityId,
    ServiceNotificationFilterSPtr const & filter,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RegisterFilterAsyncOperation>(
        *this,
        activityId,
        filter,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceNotificationClient::EndRegisterFilter(
    AsyncOperationSPtr const & operation,
    __out uint64 & filterId)
{
    return RegisterFilterAsyncOperation::End(operation, filterId);
}

AsyncOperationSPtr ServiceNotificationClient::BeginUnregisterFilter(
    ActivityId const & activityId,
    uint64 filterId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UnregisterFilterAsyncOperation>(
        *this,
        activityId,
        filterId,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceNotificationClient::EndUnregisterFilter(
    AsyncOperationSPtr const & operation)
{
    return UnregisterFilterAsyncOperation::End(operation);
}

uint64 ServiceNotificationClient::GetNextFilterId()
{
    AcquireWriteLock lock(lifecycleLock_);

    return nextFilterId_++;
}

void ServiceNotificationClient::AddFilter(
    ServiceNotificationFilterSPtr const & filter)
{
    AcquireWriteLock lock(lifecycleLock_);
    registeredFilters_.insert(make_pair(
        filter->FilterId,
        filter));
}

void ServiceNotificationClient::RemoveFilter(uint64 filterId)
{
    AcquireWriteLock lock(lifecycleLock_);

    registeredFilters_.erase(filterId);
}

void ServiceNotificationClient::UpdateVersionsCallerHoldsLock(
    GenerationNumber const & generation,
    VersionRangeCollectionSPtr && versions)
{
    if (generation_ == generation)
    {
        versions_->Merge(*versions);
    }
    else
    {
        generation_ = generation;
        versions_.swap(versions);
        gatewaySynchronizationContext_->Clear();
    }
}

void ServiceNotificationClient::AcceptAndPostNotificationsIfSynchronized(
    ServiceNotificationSPtr && notification,
    vector<ServiceNotificationResultSPtr> && notificationResults)
{
    vector<ServiceNotificationResultSPtr> updates;
    {
        auto pendingNotification = make_shared<PendingServiceNotificationResultsItem>(
            move(notification),
            move(notificationResults));

        AcquireWriteLock lock(lifecycleLock_);

        if (isSynchronized_)
        {
            updates = this->ComputeAcceptedNotificationsCallerHoldsLock(move(pendingNotification));
        }
        else
        {
            WriteNoise(
                TraceComponent, 
                this->TraceId,
                "[{0}]: buffering notification during synchronization: count={1}",
                notification->GetNotificationPageId(),
                pendingNotifications_.size());

            pendingNotifications_.push_back(move(pendingNotification));
        }
    }

    this->PostNotifications(move(updates));
}

bool ServiceNotificationClient::TryDrainPendingNotificationsOnSynchronized(
    ActivityId const & activityId,
    GatewayDescription const &targetConnectedGateway)
{
    bool synchronized = false;

    vector<ServiceNotificationResultSPtr> updates;
    {
        AcquireWriteLock lock(lifecycleLock_);

        if (targetGateway_ == targetConnectedGateway)
        { 
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: synchronized: draining={1}",
                activityId,
                pendingNotifications_.size());

            for (auto & it : pendingNotifications_)
            {
                auto temp = this->ComputeAcceptedNotificationsCallerHoldsLock(move(it));

                updates.insert(updates.end(), temp.begin(), temp.end());
            }

            pendingNotifications_.clear();

            isSynchronized_ = true;

            synchronized = true;
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: aborting synchronization on gateway mismatch: synchronized={1} expected target={2}",
                activityId,
                targetConnectedGateway,
                targetGateway_);
        }
    }

    if (synchronized)
    {
        this->PostNotifications(move(updates));
    }

    return synchronized;
}

vector<ServiceNotificationResultSPtr> ServiceNotificationClient::ComputeAcceptedNotificationsCallerHoldsLock(
    PendingServiceNotificationResultsItemSPtr && pendingNotification)
{
    vector<ServiceNotificationResultSPtr> updates;

    auto notificationPageId = pendingNotification->GetNotificationPageId();
    auto generation = pendingNotification->GetGenerationNumber();
    auto versions = pendingNotification->TakeVersions();
    auto notificationResults = pendingNotification->TakeNotificationResults();

    if (!versions)
    {
        versions = make_shared<VersionRangeCollection>();
    }

    // Stale generation - drop all incoming notifications
    //
    if (generation < generation_)
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId,
            "[{0}]: dropping old generation notifications: {1} < {2}", 
            notificationPageId,
            generation,
            generation_); 

        return move(updates);
    }

    // New generation - reset all known version information
    // an deliver all notifications
    //
    if (generation > generation_)
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId,
            "[{0}]: updating generation: {1} -> {2}, versions: {3} -> {4}", 
            notificationPageId,
            generation_,
            generation,
            *versions_,
            *versions); 

        this->UpdateVersionsCallerHoldsLock(generation, move(versions));

        for (auto const & notificationResult : notificationResults)
        {
            if (!notificationResult) { continue; }

            this->TryAcceptNotification(
                updates,
                notificationPageId,
                notificationResult);
        }

        return move(updates);
    }

    // Generation is current - filter notifications
    // based on current known versions, which represent
    // delivered notifications
    //
    for (auto const & notificationResult : notificationResults)
    {
        if (!notificationResult) { continue; }

        auto const & partition = notificationResult->Partition;

        if (!versions_->Contains(partition.Version))
        {
            this->TryAcceptNotification(
                updates,
                notificationPageId,
                notificationResult);
        }
        else
        {
            WriteNoise(
                TraceComponent, 
                this->TraceId,
                "[{0}]: dropped service notification: {1}",
                notificationPageId,
                partition);
        }
    }

    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "[{0}]: updating versions: gen={1} vers={2}+{3} incoming={4} accepted={5}",
        notificationPageId,
        generation_,
        *versions_,
        *versions,
        notificationResults.size(),
        updates.size());

    this->UpdateVersionsCallerHoldsLock(generation, move(versions));

    return move(updates);
}

void ServiceNotificationClient::TryAcceptNotification(
    __in vector<ServiceNotificationResultSPtr> & updates,
    ServiceNotificationPageId const & pageId,
    ServiceNotificationResultSPtr const & notificationResult)
{
    ServiceTableEntry const & partition = notificationResult->Partition;

    if (gatewaySynchronizationContext_->TryUpdateUndeletedVersion(
        partition, 
        notificationResult->IsMatchedPrimaryOnly))
    {
        vector<ServiceTableEntrySPtr> serviceGroupMembers;
        if (notificationResult->Rsp &&
            notificationResult->Rsp->IsServiceGroup &&
            ServiceGroup::Utility::TryConvertToServiceGroupMembers(partition, serviceGroupMembers))
        {
            for (auto & member : serviceGroupMembers)
            {
                WriteInfo(
                    TraceComponent, 
                    this->TraceId,
                    "[{0}]: accepted service group member '{1}' notification: {2}",
                    pageId,
                    member->ServiceName,
                    *member);

                updates.push_back(
                    make_shared<ServiceNotificationResult>(
                        make_shared<ResolvedServicePartition>(
                            *notificationResult->Rsp,
                            move(*member))));
            }
        }
        else
        {
            Trace.AcceptedNotification(
                this->TraceId,
                pageId,
                partition.ServiceName,
                partition);

            updates.push_back(notificationResult);
        }
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId,
            "[{0}]: dropping duplicate notification ({1}): {2}",
            pageId,
            partition.ServiceName,
            partition);
    }
}

void ServiceNotificationClient::PostNotifications(
    vector<ServiceNotificationResultSPtr> && notificationResults)
{
    if (!notificationResults.empty())
    {
        auto notificationsSPtr = make_shared<vector<ServiceNotificationResultSPtr>>();
        notificationsSPtr->swap(notificationResults);

        auto root = this->Root.CreateComponentRoot();
        Threadpool::Post([this, root, notificationsSPtr]()
        {
            NotificationHandler handler;
            {
                AcquireReadLock lock(handlerLock_);

                handler = notificationHandler_;
            }

            if (handler)
            {
                auto error = handler(*notificationsSPtr);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent, 
                        this->TraceId,
                        "failed to deliver notifications: error={0}",
                        error);
                }
            }
        });
    }
}

void ServiceNotificationClient::OnNotificationReceived(
    __in Message & request,
    __in ReceiverContextUPtr & receiverContext)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessNotificationAsyncOperation>(
        *this,
        request,
        receiverContext,
        [this](AsyncOperationSPtr const & operation) { this->OnProcessNotificationComplete(operation, false); },
        this->Root.CreateAsyncOperationRoot());
    this->OnProcessNotificationComplete(operation, true);
}

void ServiceNotificationClient::OnProcessNotificationComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ProcessNotificationAsyncOperation::End(operation);
}

void ServiceNotificationClient::OnGatewayConnected(
    ClientConnectionManager::HandlerId,
    ISendTarget::SPtr const &,
    GatewayDescription const & connectedGateway)
{
    ActivityId activityId;
    bool startSynchronizing = false;
    {
        AcquireWriteLock lock(lifecycleLock_);

        if (registeredFilters_.empty())
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: ignoring gateway connected event: no registered filters",
                activityId);

            targetGateway_.Clear();

            currentGateway_.Clear();

            isSynchronized_ = false;
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: gateway connected event: {1}",
                activityId,
                connectedGateway);

            targetGateway_ = connectedGateway;

            isSynchronized_ = false;

            startSynchronizing = true;
        }
    }

    if (startSynchronizing)
    {
        this->StartSynchronizingSession(connectedGateway, activityId);
    }
}

void ServiceNotificationClient::StartSynchronizingSession(
    GatewayDescription const & targetGateway,
    ActivityId const & activityId)
{
    auto operation = AsyncOperation::CreateAndStart<SynchronizeSessionAsyncOperation>(
        *this,
        targetGateway,
        activityId,
        [this](AsyncOperationSPtr const & operation) { this->OnSynchronizingSessionComplete(operation, false); },
        this->Root.CreateAsyncOperationRoot());
    this->OnSynchronizingSessionComplete(operation, true);
}

void ServiceNotificationClient::OnSynchronizingSessionComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    GatewayDescription target;
    GatewayDescription actual;
    auto error = SynchronizeSessionAsyncOperation::End(operation, target, actual);

    if (error.IsSuccess())
    {
        AcquireWriteLock lock(lifecycleLock_);

        if (targetGateway_ == target)
        {
            currentGateway_ = actual;
        }
    }
    else if (error.IsError(ErrorCodeValue::Timeout))
    {
        auto msg = wformatString("Synchronization timed out");

        WriteError(TraceComponent, this->TraceId, "{0}", msg);

        Assert::CodingError("{0}", msg);
    }
    else
    {
        WriteInfo(TraceComponent, this->TraceId, "Synchronization failed: error={0}", error);
    }
}
