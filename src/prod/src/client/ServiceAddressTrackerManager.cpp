// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Naming;

    ServiceAddressTrackerManager::ServiceAddressTrackerManager(__in FabricClientImpl & client)
        : RootedObject(client)
        , hasPendingRequest_(false)
        , lock_()
        , trackers_()
        , timer_()
        , cancelled_(false)
        , client_(client)
        , settingsHolder_(client)
        , pendingBatchedPollCount_()
        , startPollImmediately_(false)
        , firstNonProcessedRequest_()
    {
    }

    ServiceAddressTrackerManager::~ServiceAddressTrackerManager()
    {
    }

    void ServiceAddressTrackerManager::CacheUpdatedCallback(
        ResolvedServicePartitionSPtr const & partition,
        AddressDetectionFailureSPtr const & failure,
        FabricActivityHeader const & activityHeader)
    {
        AcquireExclusiveLock acquire(lock_);
        if (trackers_.empty())
        {
            return;
        }
        
        if (partition)
        {
            PostPartitionUpdateCallerHoldsLock(partition, activityHeader);
        }

        if (failure)
        {
            PostFailureUpdateCallerHoldsLock(failure, activityHeader);
        }
    }
    
    Common::ErrorCode ServiceAddressTrackerManager::AddTracker(
        Api::ServicePartitionResolutionChangeCallback const &handler,
        NamingUri const& name,
        PartitionKey const& partitionKey,
        __out LONGLONG & handlerId)
    {
        // For service groups, extract the base name which is actually resolved.
        // The full name is used to see whether the locations pertain to this service.
        NamingUri requestDataName = name.Fragment.empty() ? name : NamingUri(name.Path);
        NameRangeTuple tuple(requestDataName, partitionKey);

        ServiceAddressTrackerSPtr tracker;
        ErrorCode error;
        FabricActivityHeader activityHeader;
        {
            AcquireExclusiveLock acquire(lock_);
            auto nameRange = trackers_.equal_range(tuple);
            auto it = find_if(nameRange.first, nameRange.second, 
                [&name] (NameRangeTupleTrackerPair const & pair) { return pair.second->Name == name; });
            
            if (it == nameRange.second)
            {
                // This is the first request for this partition
                tracker = make_shared<ServiceAddressTracker>(
                    client_, 
                    name, 
                    partitionKey, 
                    move(requestDataName));

                // Make sure that the request fits in the max message size,
                // otherwise do not allow the callback to be added
                error = Utility::CheckEstimatedSize(tracker->CreateRequestData().EstimateSize());
                if (!error.IsSuccess())
                {
                    return error;
                }                

                trackers_.insert(pair<NameRangeTuple, ServiceAddressTrackerSPtr>(tuple, tracker));
            }
            else
            {
                tracker = it->second;
            }

            error = tracker->AddRequest(handler, activityHeader, /*out*/handlerId);
        }

        if (error.IsSuccess())
        {
            auto root = this->Root.CreateComponentRoot();
            client_.Cache.EnsureCacheCallback(
                name, 
                [this, root](ResolvedServicePartitionSPtr const & rsp, FabricActivityHeader const & act)
                {
                    AcquireExclusiveLock lock(this->lock_);

                    if (!this->trackers_.empty())
                    {
                        this->PostPartitionUpdateCallerHoldsLock(rsp, act);
                    }
                });

            PollServiceLocations(move(activityHeader));
        }

        return error;
    }

    bool ServiceAddressTrackerManager::RemoveTracker(LONGLONG handlerId)
    {
        AcquireExclusiveLock acquire(lock_);

        for (auto it = trackers_.begin(); it != trackers_.end(); ++it)
        {
            if (it->second->TryRemoveRequest(handlerId))
            {
                client_.Cache.ReleaseCacheCallback(it->second->Name);

                if (it->second->RequestCount == 0)
                {
                    trackers_.erase(it);
                }

                return true;
            }
        }

        return false;
    }

    void ServiceAddressTrackerManager::AddRequestDataCallerHoldsLock(
        ServiceAddressTrackerSPtr const & tracker,
        FabricActivityHeader const & activityHeader,
        __in std::vector<ServiceAddressTrackerPollRequests> & batchedPollRequests)
    {
        ServiceLocationNotificationRequestData pendingRequest = move(tracker->CreateRequestData());
        size_t pendingRequestSize = pendingRequest.EstimateSize();
        size_t messageContentThreshold = Utility::GetMessageContentThreshold();

        // Check that this request has valid size (not bigger than message size)
        auto error = Utility::CheckEstimatedSize(pendingRequestSize, messageContentThreshold);
        if (!error.IsSuccess())
        {
            // Post error to tracker, do not add the request to the list
            tracker->PostError(error, activityHeader);
            return;
        }

        if (batchedPollRequests.empty() ||
            !batchedPollRequests.back().CanFit(pendingRequest, pendingRequestSize))
        {
            uint64 index = static_cast<uint64>(batchedPollRequests.size());
            batchedPollRequests.push_back(ServiceAddressTrackerPollRequests(
                activityHeader, 
                client_.TraceContext, 
                index,
                messageContentThreshold));
        }

        batchedPollRequests.back().AddRequest(move(pendingRequest), pendingRequestSize);
    }

    void ServiceAddressTrackerManager::PollServiceLocations(
        FabricActivityHeader && activityHeader)
    {
        std::vector<ServiceAddressTrackerPollRequests> batchedPollRequests;

        { // lock
            AcquireExclusiveLock acquire(lock_);

            if (!hasPendingRequest_ && !trackers_.empty())
            {
                hasPendingRequest_ = true;

                firstNonProcessedRequest_.reset();
                startPollImmediately_ = false;

                auto itStart = trackers_.begin();
                if (firstNonProcessedRequest_)
                {
                    // Find first non-processed request (if any) and start from there.
                    // This will make sure that updates for non-processed requests will
                    // be processed first.
                    // Note that trackers may have been added and/or removed; that is fine,
                    // they will eventually be processed.
                    itStart =  trackers_.lower_bound(*firstNonProcessedRequest_);
                }

                for (auto it = itStart; it != trackers_.end(); ++it)
                {
                    AddRequestDataCallerHoldsLock(it->second, activityHeader, batchedPollRequests);
                }

                for (auto it = trackers_.begin(); it != itStart; ++it)
                {
                    AddRequestDataCallerHoldsLock(it->second, activityHeader, batchedPollRequests);
                }
            }
        } //endlock

        if (!batchedPollRequests.empty())
        {
            size_t pollCount = batchedPollRequests.size();
            pendingBatchedPollCount_.store(pollCount);
            for (size_t i = 0; i < pollCount; ++i)
            {
                AsyncOperationSPtr inner = client_.BeginInternalLocationChangeNotificationRequest(
                    std::move(batchedPollRequests[i].MoveRequests()),
                    std::move(activityHeader),
                    [this](AsyncOperationSPtr const & a) { FinishPoll(a, false /*expectedCompletedSynchronously*/); },
                    get_Root().CreateAsyncOperationRoot());

                FinishPoll(inner, true /*bool expectedCompletedSynchronously*/);
            }

            CheckIfAllPendingPollRequestsDone(pollCount);
        }
    }

     void ServiceAddressTrackerManager::FinishPoll(
        Common::AsyncOperationSPtr const & pollOperation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != pollOperation->CompletedSynchronously)
        {
            return;
        }

        FabricActivityHeader activityHeader;
        vector<ResolvedServicePartitionSPtr> partitions;
        vector<AddressDetectionFailureSPtr> failures;
        bool updateNonProcessedRequest;
        std::unique_ptr<NameRangeTuple> firstNonProcessedRequest;
        auto error = client_.EndInternalLocationChangeNotificationRequest(
            pollOperation, 
            partitions, 
            failures, 
            updateNonProcessedRequest,
            firstNonProcessedRequest, 
            activityHeader);

        if (!error.IsSuccess())
        {
            client_.Trace.TrackerEndPoll(
                client_.TraceContext,
                activityHeader.ActivityId,
                error);
        }
       
        { // lock
            AcquireExclusiveLock acquire(lock_);
            if (trackers_.empty())
            {
                // Ignore any previously non-processed requests
                firstNonProcessedRequest_.reset();
            }
            else 
            {
                // If one or more of the requests was found in the cache, 
                // the poll didn't go to the gateway. To make sure the non-processed requests are 
                // going to be processed first, do not override the previous firstNonProcessedRequest.
                // Otherwise, change the firstNonProcessedRequest accordingly.
                // If the requests were broken into multiple messages, keep the minimum of the non-processed tuple.
                // Note that this is best effort: it may be that the trackers were removed or the
                // non-processed requests are the one that were updated in the cache.
                // This is fine, as their processing should eventually return no new updates, 
                // so the other entries will be resolved.
                if (updateNonProcessedRequest)
                {
                    bool update = false;
                    if (firstNonProcessedRequest_)
                    {
                        if (firstNonProcessedRequest)
                        {
                            // Compute the minimum unprocessed value from all current polls
                            NameRangeTuple::LessThanComparitor comparitor;
                            if (comparitor(*firstNonProcessedRequest, *firstNonProcessedRequest_))
                            {
                                update = true;
                            }
                        }
                    }
                    else
                    {
                        update = true;
                    }

                    if (update)
                    {
                        firstNonProcessedRequest_ = move(firstNonProcessedRequest);
                    }
                }
                
                if (error.IsSuccess())
                {
                    for (auto iter = partitions.begin(); iter != partitions.end(); ++iter)
                    {
                        PostPartitionUpdateCallerHoldsLock(*iter, activityHeader);
                    }

                    for (auto iter = failures.begin(); iter != failures.end(); ++iter)
                    {
                        PostFailureUpdateCallerHoldsLock(*iter, activityHeader);
                    }
                }
                else if (ShouldRaiseError(error))
                {
                    // Post the failure to all trackers, without passing a version
                    // This means that not only the trackers in this batch will get it.
                    for (auto it = trackers_.begin(); it != trackers_.end(); ++it)
                    {
                        it->second->PostError(error, activityHeader);
                    }
                }
            
                if (IsRetryableError(error))
                {
                    startPollImmediately_ = true;
                }
            }
        } // end lock
        
        uint64 pendingOperationCount = --pendingBatchedPollCount_;
        CheckIfAllPendingPollRequestsDone(pendingOperationCount);
    }

    void ServiceAddressTrackerManager::CheckIfAllPendingPollRequestsDone(uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            AcquireExclusiveLock acquire(lock_);
            hasPendingRequest_ = false;
             
            if (!cancelled_ && !trackers_.empty())
            {
                if (!timer_)
                {
                    auto root = get_Root().CreateComponentRoot();
                    timer_ = Timer::Create(
                        "PollServiceLocations",
                        [this, root] (TimerSPtr const &) 
                        { 
                            PollServiceLocations(move(FabricActivityHeader())); 
                        },
                        true /*allowConcurrency*/);
                }

                // Retry immediately if we got back success or a retryable error. Otherwise wait to retry.
                // Note that this can be called from PollServiceLocations, so don't recurse here.
                //
                // We want the normal state to be that we have a request parked at the service,
                // waiting for a change. That way we can find out immediately about changes.
                // So if we get success or a retryable error, we want to send the next request immediately so that we
                // are ready. 
                // Other errors could indicate the previous request failed immediately, so wait for PollInterval.
                timer_->Change(
                    startPollImmediately_ ? TimeSpan::Zero : settingsHolder_.Settings->ServiceChangePollInterval);
            }
        }
    }

    bool ServiceAddressTrackerManager::ShouldRaiseError(Common::ErrorCode const error)
    {
        switch(error.ReadValue())
        {
        case ErrorCodeValue::GatewayUnreachable:
            return true;
        default:
            return false;
        }
    }

    bool ServiceAddressTrackerManager::IsRetryableError(Common::ErrorCode const error)
    {
        if (error.IsSuccess() || 
            error.IsError(ErrorCodeValue::Timeout)) 
        {
            return true;
        }

        return false;
    }

    void ServiceAddressTrackerManager::PostFailureUpdateCallerHoldsLock(
        AddressDetectionFailureSPtr const & failure,
        Transport::FabricActivityHeader const & activityHeader)
     {
        if (!failure)
        {
            // Functionally okay to continue in production, since we can
            // skip the element
            //
            Assert::TestAssert();
            return;
        }
            
        NameRangeTuple updateTuple(failure->Name, failure->PartitionData);
        auto nameRange = trackers_.equal_range(updateTuple);
        if (nameRange.first == nameRange.second)
        {
            client_.Trace.TrackerADFNotRelevant(
                client_.TraceContext,
                activityHeader.ActivityId, 
                static_cast<uint64>(trackers_.size()),
                *failure);
            return;
        }

        for (auto it = nameRange.first; it != nameRange.second; ++it)
        {
           it->second->PostUpdate(failure, activityHeader);
        }
    }
     
    void ServiceAddressTrackerManager::PostPartitionUpdateCallerHoldsLock(
        ResolvedServicePartitionSPtr const & partition,
        Transport::FabricActivityHeader const & activityHeader)
    {
        if (!partition)
        {
            // Functionally okay to continue in production, since we can
            // skip the element
            //
            Assert::TestAssert();
            return;
        }
            
        // Skip infrastructure services that have no name
        NamingUri serviceName;
        if (!partition->TryGetName(serviceName))
        {
            client_.Trace.RSPHasNoName(
                client_.TraceContext,
                activityHeader.ActivityId, 
                *partition);
            return;
        }

        // The range in the ResolvedServicePartition can refer to multiple trackers
        NameRangeTuple updateTuple(serviceName, partition->PartitionData);
        auto nameRange = trackers_.equal_range(updateTuple);
        if (nameRange.first == nameRange.second)
        {
            client_.Trace.TrackerRSPNotRelevant(
                client_.TraceContext,
                activityHeader.ActivityId, 
                static_cast<uint64>(trackers_.size()),
                *partition);
            return;
        }

        for (auto it = nameRange.first; it != nameRange.second; ++it)
        {
           it->second->PostUpdate(partition, activityHeader);
        }
    }

    void ServiceAddressTrackerManager::CancelPendingRequests()
    {
        NameRangeTupleTrackerMap trackers;

        { // lock
            AcquireExclusiveLock acquire(lock_);

            if (timer_)
            {
                timer_->Cancel();
                timer_ = nullptr;
            }

            // Setting cancelled_ means we will never add another tracker.
            cancelled_ = true;
            trackers.swap(trackers_);
        } // endlock

        if (trackers.empty())
        {
            return;
        }

        client_.Trace.TrackerCancelAll(
            client_.TraceContext,
            static_cast<uint64>(trackers.size()));
        for (auto iter = trackers.begin(); iter != trackers.end(); ++iter)
        {
            iter->second->Cancel();
        }
    }
}
