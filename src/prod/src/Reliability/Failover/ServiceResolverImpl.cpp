// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"
#include "Reliability/Failover/ServiceResolverImpl.h"

using namespace std;
using namespace Common;
using namespace Transport;

namespace Reliability
{
    class ServiceResolverImpl::LookupRequestLinkableAsyncOperation : public LinkableAsyncOperation
    {
    public:
        LookupRequestLinkableAsyncOperation(
            __in ServiceResolverImpl & resolver,
            vector<VersionedCuid> const partitions,
            FabricActivityHeader const & activityHeader,
            TimeSpan const timeout,
            AsyncOperationSPtr const & primary,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
          : LinkableAsyncOperation(primary, callback, parent)
          , resolver_(resolver)
          , partitions_(partitions)
          , activityHeader_(activityHeader)
          , timeoutHelper_(timeout)
        {
        }

        LookupRequestLinkableAsyncOperation(
            __in ServiceResolverImpl & resolver,
            vector<VersionedCuid> const partitions,
            FabricActivityHeader const & activityHeader,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
          : LinkableAsyncOperation(callback, parent)
          , resolver_(resolver)
          , partitions_(partitions)
          , activityHeader_(activityHeader)
          , timeoutHelper_(timeout)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & asyncOperation, __out ServiceTableUpdateMessageBody & body)
        {
            auto casted = AsyncOperation::End<LookupRequestLinkableAsyncOperation>(asyncOperation);
            
            // On secondaries, copy the result from the primary;
            // this state can't be set in OnCompleted, as End may execute before the OnCompleted method
            // in CompletedSynchronously case
            casted->TrySetResultOnSecondary();

            // Do not move the body, as it may be needed on primary to set the secondaries result
            body = casted->body_;
            return casted->Error;
        }

    protected:
        void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
        {
            vector<ConsistencyUnitId> cuids;
            for (auto iter = partitions_.begin(); iter != partitions_.end(); ++iter)
            {
                cuids.push_back(iter->Cuid);
            }

            MessageUPtr message = RSMessage::GetServiceTableRequest().CreateMessage<ServiceTableRequestMessageBody>(
                ServiceTableRequestMessageBody(
                    resolver_.cache_.Generation, 
                    resolver_.cache_.GetKnownVersions(),
                    move(cuids)));

            RSMessage::AddActivityHeader(*message, activityHeader_);
            auto operation = resolver_.transport_.BeginRequest(
                move(message),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnLookupRequestComplete(operation, false); },
                thisSPtr);

            this->OnLookupRequestComplete(operation, true);
        }

        void OnNewPrimaryPromoted(Common::AsyncOperationSPtr const & promotedPrimary)
        {
            ASSERT_IFNOT(promotedPrimary, "{0}: OnNewPrimaryPromoted: promotedPrimary should be set", activityHeader_.Guid);
            WriteNoise(
                Constants::ServiceResolverSource,
                resolver_.traceId_,
                "{0}-{1}: Primary completed with {2}, new primary chosen.",
                activityHeader_.Guid,
                resolver_.tag_,
                this->Error);
            
            { 
                AcquireExclusiveLock lock(resolver_.pendingCommunicationLock_);
                resolver_.primaryPendingLookupRequestOperation_ = promotedPrimary;
            } 
        }

        bool IsErrorRetryable(ErrorCode const error)
        {
            if (error.IsError(ErrorCodeValue::Timeout))
            {
                return !timeoutHelper_.IsExpired;
            }

            return false;
        }

    private:
        
        void OnLookupRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            MessageUPtr reply;
            ErrorCode error = resolver_.transport_.EndRequest(operation, reply);
            if (error.IsSuccess())
            {
                if (!reply->GetBody<ServiceTableUpdateMessageBody>(body_))
                {
                    WriteWarning(
                        Constants::ServiceResolverSource,
                        resolver_.traceId_,
                        "{0}-{1}: Invalid message: {2}. Error={3:x}", 
                        activityHeader_.Guid,
                        resolver_.tag_,
                        reply->MessageId, 
                        reply->Status);
                    error = ErrorCodeValue::InvalidMessage;
                }
                else
                {
                    WriteNoise(
                        Constants::ServiceResolverSource,
                        resolver_.traceId_,
                        "{0}-{1}: Got lookup response: IsFromFMM={2}, Generation={3}, VersionRangeCollection={4}, entry number={5}",
                        activityHeader_.Guid,
                        resolver_.tag_,
                        body_.IsFromFMM,
                        body_.Generation,
                        body_.VersionRangeCollection,
                        body_.ServiceTableEntries.size());

                    // Process the updates, without calling refresh
                    resolver_.ProcessServiceTableUpdate(body_, activityHeader_, nullptr);
                }
            }

            TryComplete(operation->Parent, error);
        }
        
        void TrySetResultOnSecondary()
        {
            if (IsSecondary)
            {
                auto primary = GetPrimary<LookupRequestLinkableAsyncOperation>();
                WriteNoise(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Secondary LookupRequest completed with {2} (primary {3}).",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    Error,
                    primary->activityHeader_.Guid);
 
                if (Error.IsSuccess())
                {
                    body_ = primary->body_;
                }
            }
            else
            {
                 WriteNoise(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Primary LookupRequest callback called with {2}.",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    Error); 
            }
        }

        ServiceResolverImpl & resolver_;
        vector<VersionedCuid> const partitions_;
        ServiceTableUpdateMessageBody body_;        
        FabricActivityHeader activityHeader_;
        TimeoutHelper timeoutHelper_;
    };

    class ServiceResolverImpl::ResolveServiceAsyncOperation : public Common::TimedAsyncOperation
    {
    public:
        ResolveServiceAsyncOperation(
            std::vector<VersionedCuid> const & partitions,
            CacheMode::Enum refreshMode,
            __in ServiceResolverImpl & resolver,
            FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        
        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out std::vector<ServiceTableEntry> & serviceLocations,
            __out GenerationNumber & generation);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Transport::FabricActivityHeader & activityHeader,
            __out Common::AsyncOperationSPtr & root);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void OnLookupRequestComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
        void RetrieveDataFromCacheAndComplete(Common::AsyncOperationSPtr const & thisSPtr);
        
        std::vector<VersionedCuid> const partitions_;
        CacheMode::Enum refreshMode_;
        std::vector<ServiceTableEntry> serviceLocations_;
        ServiceResolverImpl & resolver_;
        Transport::MessageUPtr reply_;
        FabricActivityHeader activityHeader_;
        GenerationNumber generation_;
    };

    ServiceResolverImpl::ResolveServiceAsyncOperation::ResolveServiceAsyncOperation(
        vector<VersionedCuid> const & partitions,
        CacheMode::Enum refreshMode,
        __in ServiceResolverImpl & resolver,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , partitions_(partitions)
        , refreshMode_(refreshMode)
        , serviceLocations_()
        , resolver_(resolver)
        , activityHeader_(activityHeader)
        , generation_()
    {
    }

    ErrorCode ServiceResolverImpl::ResolveServiceAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation, 
        __out vector<ServiceTableEntry> & serviceLocations,
        __out GenerationNumber & generation) 
    {
        auto casted = AsyncOperation::Get<ResolveServiceAsyncOperation>(asyncOperation);
        if (casted->Error.IsSuccess())
        {
            serviceLocations = casted->serviceLocations_;
            generation = casted->generation_;
        }
        return casted->Error;
    }

    ErrorCode ServiceResolverImpl::ResolveServiceAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out FabricActivityHeader & activityHeader,
        __out AsyncOperationSPtr & root) 
    {
        auto casted = AsyncOperation::End<ResolveServiceAsyncOperation>(asyncOperation);
        activityHeader = casted->activityHeader_;
        root = casted->Parent;
        return casted->Error;
    }         
    
    void ServiceResolverImpl::ResolveServiceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        vector<ServiceTableEntry> cachedEntries;

        bool shouldSendLookupRequest = (refreshMode_ == CacheMode::BypassCache);

        if (!shouldSendLookupRequest)
        {
            for (auto it = partitions_.begin(); it != partitions_.end(); ++it)
            {
                ServiceTableEntry cachedEntry;
                if (resolver_.cache_.TryGetEntryWithCacheMode(*it, refreshMode_, cachedEntry))
                {
                    cachedEntries.push_back(move(cachedEntry));
                }
                else
                {
                    shouldSendLookupRequest = true;
                    break;
                }
            }
        }

        if (!shouldSendLookupRequest)
        {
            WriteNoise(
                Constants::ServiceResolverSource,
                resolver_.traceId_,
                "{0}-{1}: Cached data will be used to resolve CUIDs {2}.",
                activityHeader_.Guid,
                resolver_.tag_,
                partitions_);

            serviceLocations_ = std::move(cachedEntries);
            generation_ = resolver_.cache_.Generation;

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            bool isPrimary = false;
            LONG pendingOperationCount = 0;
            AsyncOperationSPtr operation;

            {
                AcquireExclusiveLock lock(resolver_.pendingCommunicationLock_);

                pendingOperationCount = ++resolver_.pendingCommunicationOperationCount_;

                if (resolver_.primaryPendingLookupRequestOperation_ == nullptr)
                {
                    isPrimary = true;

                    operation = AsyncOperation::CreateAndStart<LookupRequestLinkableAsyncOperation>(
                        resolver_,
                        partitions_,
                        activityHeader_,
                        this->RemainingTime,
                        [this](AsyncOperationSPtr const & a) { this->OnLookupRequestComplete(a, false); },
                        thisSPtr);

                    resolver_.primaryPendingLookupRequestOperation_ = operation;
                }
                else
                {
                    operation = AsyncOperation::CreateAndStart<LookupRequestLinkableAsyncOperation>(
                        resolver_,
                        partitions_,
                        activityHeader_,
                        this->RemainingTime,
                        resolver_.primaryPendingLookupRequestOperation_,
                        [this](AsyncOperationSPtr const & a) { this->OnLookupRequestComplete(a, false); },
                        thisSPtr);
                }
            }

            if (isPrimary)
            {
                WriteInfo(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Primary: Sending lookup request to resolve entries outside from range {2} ({3} pending ops).",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    resolver_.cache_.GetKnownVersions(),
                    pendingOperationCount);
            }
            else
            {
                WriteInfo(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Secondary: Sending lookup request to resolve entries outside from range {2} ({3} pending ops).",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    resolver_.cache_.GetKnownVersions(),
                    pendingOperationCount);
            }

            AsyncOperation::Get<LookupRequestLinkableAsyncOperation>(operation)->ResumeOutsideLock(operation);
            this->OnLookupRequestComplete(operation, true);
        }
    }

    void ServiceResolverImpl::ResolveServiceAsyncOperation::OnLookupRequestComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ServiceTableUpdateMessageBody body;
        ErrorCode error = LookupRequestLinkableAsyncOperation::End(operation, body);
        {
            AcquireExclusiveLock lock(resolver_.pendingCommunicationLock_);
            --resolver_.pendingCommunicationOperationCount_;
            if (resolver_.pendingCommunicationOperationCount_ == 0)
            {
                 WriteInfo(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: {2} pending lookup requests, clear primary.",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    resolver_.pendingCommunicationOperationCount_);
                resolver_.primaryPendingLookupRequestOperation_ = nullptr;
            }
            else
            {
                WriteNoise(
                    Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: {2} pending lookup requests.",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    resolver_.pendingCommunicationOperationCount_);
            }
        }

        if (error.IsSuccess())
        {
            // The primary updated the service table. 
            // All (primary and secondaries) need to check whether the cuid they are interested in is not found.
            for (auto iter = body.ServiceTableEntries.begin(); iter != body.ServiceTableEntries.end(); ++iter)
            {
                if (!iter->IsFound)
                {
                    // A secondary linked to a primary may have specified different CUIDs
                    // Check that the cuid that doesn't exist is actually requested by this async operation
                    auto const & iterCuid = iter->ConsistencyUnitId;
                    auto iterFind = find_if(partitions_.begin(), partitions_.end(), [&iterCuid](VersionedCuid const & cuid) { return cuid.Cuid == iterCuid; });
                    if (iterFind != partitions_.end())
                    {
                        TryComplete(operation->Parent, ErrorCodeValue::PartitionNotFound); 
                        return;
                    }
                }
            }

            RetrieveDataFromCacheAndComplete(operation->Parent);
        }
        else
        {
            WriteInfo(
                Reliability::Constants::ServiceResolverSource,
                resolver_.traceId_,
                "{0}-{1}: Failed to resolve CUIDs: CUID count = {2} error = {3}",
                activityHeader_.Guid,
                resolver_.tag_,
                partitions_.size(),
                error);
            TryComplete(operation->Parent, error);
        }
    }    

    void ServiceResolverImpl::ResolveServiceAsyncOperation::RetrieveDataFromCacheAndComplete(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ServiceTableEntry> cachedEntries;
        for (auto it = partitions_.begin(); it != partitions_.end(); ++it)
        {
            ServiceTableEntry cachedEntry;
            if (resolver_.cache_.TryGetEntry(it->Cuid, cachedEntry))
            {
                WriteNoise(
                    Reliability::Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Resolved service location: {2}",
                    resolver_.tag_,
                    activityHeader_.Guid,
                    cachedEntry);
                cachedEntries.push_back(cachedEntry);
            }
            else
            {
                WriteInfo(
                    Reliability::Constants::ServiceResolverSource,
                    resolver_.traceId_,
                    "{0}-{1}: Unable to resolve CUID '{2}'",
                    activityHeader_.Guid,
                    resolver_.tag_,
                    it->Cuid);
                TryComplete(thisSPtr, ErrorCodeValue::ServiceOffline); 
                return;
            }
        }
            
        serviceLocations_ = std::move(cachedEntries);
        generation_ = resolver_.cache_.Generation;
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    ServiceResolverImpl::ServiceResolverImpl(
        std::wstring const & tag,
        __in ServiceResolverTransport & transport, 
        __in Common::Event & broadcastProcessedEvent,
        ComponentRoot const & root)
      : traceId_(root.TraceId)
      , tag_(tag)
      , transport_(transport)
      , broadcastProcessedEvent_(broadcastProcessedEvent)
      , cache_(root)
      , primaryPendingLookupRequestOperation_(nullptr)
      , pendingCommunicationLock_()
      , pendingCommunicationOperationCount_(0)
      , refreshTimer_()
      , isProcessingMissedBroadcast_(false)
      , isDisposed_(false)
    {
    }

    AsyncOperationSPtr ServiceResolverImpl::BeginResolveServicePartition(
        ConsistencyUnitId const & initialNamingServiceCuid,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        vector<VersionedCuid> partition;
        partition.push_back(VersionedCuid(initialNamingServiceCuid));
        return BeginResolveServicePartition(partition, CacheMode::Refresh, activityHeader, timeout, callback, parent);
    }

    AsyncOperationSPtr ServiceResolverImpl::BeginResolveServicePartition(    
        vector<VersionedCuid> const & partitions,
        CacheMode::Enum refreshMode,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(Constants::ServiceResolverSource, traceId_, "{0}: Resolving CUIDs {1}", activityHeader, partitions);
        return AsyncOperation::CreateAndStart<ResolveServiceAsyncOperation>(
            partitions,
            refreshMode,
            *this,
            activityHeader,
            timeout,
            callback,
            parent);
    }

    ErrorCode ServiceResolverImpl::EndResolveServicePartition(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceTableEntry> & serviceLocations,
        __out GenerationNumber & generation)
    {
        return ResolveServiceAsyncOperation::End(operation, serviceLocations, generation);
    }
    
    bool ServiceResolverImpl::TryGetCachedEntry(
        ConsistencyUnitId const & cuid,
        __out ServiceTableEntry & entry,
        __out GenerationNumber & generation)
    {
        generation = cache_.Generation;
        return cache_.TryGetEntry(cuid, entry);
    }

    void ServiceResolverImpl::ProcessServiceTableUpdate(
        ServiceTableUpdateMessageBody const & body,
        FabricActivityHeader const & activityHeader,
        ComponentRootSPtr const & root)
    {
        bool shouldFireFMChangeEvent = (body.IsFromFMM 
            && !body.ServiceTableEntries.empty() 
            && body.ServiceTableEntries.front().ServiceReplicaSet.IsPrimaryLocationValid);

        bool isUpToDate = false;

        ServiceCuidMap updatedCuids;
        ServiceCuidMap removedCuids;

        {
            AcquireWriteLock lock(cache_.ServiceLookupTableLock);

            if (shouldFireFMChangeEvent)
            {
                shouldFireFMChangeEvent = cache_.IsUpdateNeeded_CallerHoldsLock(
                    body.ServiceTableEntries.front(),
                    body.Generation);
            }

            isUpToDate = cache_.StoreUpdate_CallerHoldsLock(
                body.ServiceTableEntries, 
                body.Generation, 
                body.VersionRangeCollection, 
                body.EndVersion,
                updatedCuids,   // out
                removedCuids);  // out
        }

        if (shouldFireFMChangeEvent)
        {
            this->fmChangeEvent_.Fire(body, false);
        }

        if (!isUpToDate && root)
        {
            this->RefreshCache(activityHeader, root->CreateAsyncOperationRoot());
        }

        if (!updatedCuids.empty() || !removedCuids.empty())
        {
            broadcastProcessedEvent_.Fire(BroadcastProcessedEventArgs(move(updatedCuids), move(removedCuids)));
        }
    }

    void ServiceResolverImpl::RefreshCache(
        FabricActivityHeader const & activityHeader, 
        Common::AsyncOperationSPtr const & root)
    {
        if (isProcessingMissedBroadcast_.exchange(true))
        {
            WriteInfo(
                Constants::ServiceResolverSource,
                traceId_,
                "{0}-{1}: Missed broadcast processing already active.",
                activityHeader,
                tag_);
        }
        else
        {
            WriteInfo(
                Constants::ServiceResolverSource,
                traceId_,
                "{0}-{1}: Resolving from FM on missed broadcast.",
                activityHeader,
                tag_);

            {
                AcquireExclusiveLock acquire(pendingCommunicationLock_);
                if (refreshTimer_)
                {
                    refreshTimer_->Cancel();
                    refreshTimer_.reset();
                }
            }

            AsyncOperationSPtr refreshOp = AsyncOperation::CreateAndStart<ResolveServiceAsyncOperation>(
                vector<VersionedCuid>(),
                CacheMode::BypassCache,
                *this,
                activityHeader,
                FailoverConfig::GetConfig().SendToFMTimeout,
                [this] (AsyncOperationSPtr const & a) { this->OnRefreshComplete(a, false); },
                root);

            this->OnRefreshComplete(refreshOp, true);
        }
    }

    void ServiceResolverImpl::OnRefreshComplete(AsyncOperationSPtr const & refreshOperation, bool expectedCompletedSynchronously)
    {
        if (refreshOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        isProcessingMissedBroadcast_.store(false);

        FabricActivityHeader activityHeader;
        AsyncOperationSPtr root;
        auto error = ResolveServiceAsyncOperation::End(refreshOperation, activityHeader, root);

        if (!error.IsSuccess())
        {
            auto retryDelay = FailoverConfig::GetConfig().SendToFMRetryInterval;

            WriteInfo(
                Constants::ServiceResolverSource,
                traceId_,
                "{0}-{1}: Resolving from FM on missed broadcast failed: error={2} retry delay={3}",
                activityHeader,
                tag_,
                error,
                retryDelay);

            AcquireExclusiveLock acquire(pendingCommunicationLock_);
            if (!refreshTimer_ && !isDisposed_)
            {
                refreshTimer_ = Timer::Create(
                    TimerTagDefault,
                    [root, this, activityHeader](Common::TimerSPtr const &) mutable { this->RefreshCache(move(activityHeader), root); },
                    false); // allow concurrency

                refreshTimer_->Change(retryDelay);
            }
        }
    }

    EventT<ServiceTableEntry>::HHandler ServiceResolverImpl::RegisterFMChangeEvent(EventT<ServiceTableUpdateMessageBody>::EventHandler const & handler)
    {
        return this->fmChangeEvent_.Add(handler);
    }

    bool ServiceResolverImpl::UnRegisterFMChangeEvent(ServiceUpdateEvent::HHandler hhandler)
    {
        return this->fmChangeEvent_.Remove(hhandler);
    }

    void ServiceResolverImpl::Dispose()
    {
        AcquireExclusiveLock acquire(pendingCommunicationLock_);

        isDisposed_ = true;

        if (refreshTimer_)
        {
            refreshTimer_->Cancel();
        }
    }
}
