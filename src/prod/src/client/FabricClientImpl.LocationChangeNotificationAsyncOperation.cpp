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
    using namespace ClientServerTransport;

    StringLiteral const TraceComponent("FabricClient.ServiceLocationNotification");

    FabricClientImpl::LocationChangeNotificationAsyncOperation::LocationChangeNotificationAsyncOperation(
        __in FabricClientImpl & client,
        vector<ServiceLocationNotificationRequestData> && requests,
        FabricActivityHeader && activityHeader,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        __in_opt ErrorCode && passThroughError)
      : ClientAsyncOperationBase(client, move(activityHeader), move(passThroughError), TimeSpan::MaxValue, callback, parent)
      , settingsHolder_(client)
      , requests_(std::move(requests))
      , partitions_()
      , failures_()
      , firstNonProcessedRequest_()
      , updateNonProcessedRequest_(false)
      , currentCuids_()
    {
    }

    ErrorCode FabricClientImpl::LocationChangeNotificationAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out vector<ResolvedServicePartitionSPtr> & partitions,
        __out vector<AddressDetectionFailureSPtr> & failures,
        __out bool & updateNonProcessedRequest,
        __out std::unique_ptr<NameRangeTuple> & firstNonProcessedRequest,
        __out FabricActivityHeader & activityHeader)
    {
        auto casted = AsyncOperation::End<FabricClientImpl::LocationChangeNotificationAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            partitions = std::move(casted->partitions_);
            failures = move(casted->failures_);
        }

        // Update this first non-processed in all cases (error or not).
        // This way, if an error occured, it will point to the first request, 
        // which was non-processed.
        firstNonProcessedRequest = move(casted->firstNonProcessedRequest_);
        updateNonProcessedRequest = casted->updateNonProcessedRequest_;

        activityHeader = casted->ActivityHeader;
        return casted->Error;
    }

    void FabricClientImpl::LocationChangeNotificationAsyncOperation::OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if (requests_.empty())
        {
            WriteWarning(
                TraceComponent,
                "{0}: Can't start poll with empty request list", 
                this->ActivityId);
            Assert::TestAssert();
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        vector<ResolvedServicePartitionSPtr> cachedEntriesToReturn;
        for (auto it = requests_.begin(); it != requests_.end(); ++it)
        {
            ServiceLocationNotificationRequestData const & request = *it;
            if (request.IsWholeService)
            {
                vector<ResolvedServicePartitionSPtr> partitions;
                this->Client.Cache.GetAllEntriesForName(request.Name, /*out*/partitions);
                for (auto itCache = partitions.begin(); itCache != partitions.end(); ++itCache)
                {
                    TryAddToReply(move(*itCache), request, cachedEntriesToReturn);
                }
            }
            else 
            {
                ResolvedServicePartitionSPtr cachedPartition;
                if (this->Client.Cache.TryGetEntry(request.Name, request.Partition, /*out*/ cachedPartition))
                {
                    TryAddToReply(move(cachedPartition), request, cachedEntriesToReturn);
                }
            }
        }

        if (!cachedEntriesToReturn.empty())
        {
            Trace.NotificationUseCached(this->TraceContext, this->ActivityId);
            partitions_ = std::move(cachedEntriesToReturn);
            // Since the cached values may not cover all entries, keep the previous lastProcessed value
            // for the request that will go to the gateway.
            updateNonProcessedRequest_ = false;
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            Trace.NotificationPollGateway(this->TraceContext, this->ActivityId);
            auto message = NamingTcpMessage::GetServiceLocationChangeListenerMessage(Common::make_unique<ServiceLocationNotificationRequestBody>(requests_)); // todo: refactor this to remove need for copy
            message->Headers.Replace(this->ActivityHeader);

            auto inner = AsyncOperation::CreateAndStart<FabricClientImpl::RequestReplyAsyncOperation>(
                this->Client,
                NamingUri::RootNamingUri,
                std::move(message),
                settingsHolder_.Settings->ServiceChangePollInterval,
                [this] (AsyncOperationSPtr const & requestOp)
                {
                    OnRequestComplete(requestOp, false /*expectedCompletedSynchronously*/);
                },
                thisSPtr);
            OnRequestComplete(inner, true /*expectedCompletedSynchronously*/);
        }
    }

    void FabricClientImpl::LocationChangeNotificationAsyncOperation::OnRequestComplete(
        AsyncOperationSPtr const & requestOp,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != requestOp->CompletedSynchronously)
        {
            return;
        }

        auto const & thisSPtr = requestOp->Parent;

        ClientServerReplyMessageUPtr reply;
        ErrorCode error = FabricClientImpl::RequestReplyAsyncOperation::End(requestOp, reply);
        if (!error.IsSuccess())
        {        
            // Remember the first non-processed request as the first passed in request
            // (there must be at least one request in poll)
            firstNonProcessedRequest_ = requests_[0].GetNameRangeTuple();
            updateNonProcessedRequest_ = true;
            TryComplete(thisSPtr, error);
            return;
        }

        ServiceLocationNotificationReplyBody body;
        if (!reply->GetBody(body))
        {
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }

        vector<ResolvedServicePartition> partitions = body.MovePartitions();
        vector<AddressDetectionFailure> failures = body.MoveFailures();

        // Assumption: The reply must match the request sent.
        // Therefore, a delayed reply will not be matched to this request.
        // Otherwise, the requests will not match, so the index will refer to a different name / partition key
        if (body.FirstNonProcessedRequestIndex == 0)
        {
            // The value is not set, no information about the last processed
            updateNonProcessedRequest_ = false;
        }
        else 
        {
            updateNonProcessedRequest_ = true;
            if (body.FirstNonProcessedRequestIndex < requests_.size())
            {
                // If the index is valid, it shows that 
                // some requests couldn't be added to the reply, so they should be retried.
                // Mark the first request that was missed.
                // Otherwise, all requests were successfully processed
                firstNonProcessedRequest_ = requests_[body.FirstNonProcessedRequestIndex].GetNameRangeTuple();
            }
        }

        for (auto itFailures = failures.begin(); itFailures != failures.end(); ++itFailures)
        {
            AddressDetectionFailureSPtr entry = make_shared<AddressDetectionFailure>(move(*itFailures));
            this->Client.Cache.UpdateCacheOnError(
                entry->Name, 
                entry->PartitionData, 
                this->ActivityHeader.ActivityId, 
                entry->Error);
            failures_.push_back(move(entry));
        }
        
        // multimap since a single notification reply may contain multiple RSPs
        // for the same service
        //
        multimap<NamingUri, ResolvedServicePartitionSPtr> rsps;
        for (auto it = partitions.begin(); it != partitions.end(); ++it)
        {
            NamingUri serviceName;
            if (it->TryGetName(serviceName)) // TryGetName should return true for all user services
            {                    
                rsps.insert(make_pair<NamingUri, ResolvedServicePartitionSPtr>(
                    move(serviceName), 
                    make_shared<ResolvedServicePartition>(move(*it))));
            }
            else
            {
                this->Trace.RSPHasNoName(
                    this->TraceContext,
                    this->ActivityId, 
                    *it);
                Assert::TestAssert("{0}: RSP has no name", this->ActivityId);
            }
        }  
       
        auto operation = this->Client.Cache.BeginUpdateCacheEntries(
            move(rsps),
            this->ActivityId,
            // Timeout only affects resolution of PSDs when they are not cached
            // or need refreshing
            //
            settingsHolder_.Settings->ServiceChangePollInterval,
            [this](AsyncOperationSPtr const & operation) { this->OnUpdateCacheEntriesComplete(operation, false); },
            thisSPtr);
        this->OnUpdateCacheEntriesComplete(operation, true);
    }

    void FabricClientImpl::LocationChangeNotificationAsyncOperation::OnUpdateCacheEntriesComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Client.Cache.EndUpdateCacheEntries(operation, partitions_);

        this->TryComplete(thisSPtr, error);
    }
    
    bool FabricClientImpl::LocationChangeNotificationAsyncOperation::ShouldAddEntry(
        ResolvedServicePartitionSPtr const & entry,
        ServiceLocationNotificationRequestData const & request)
    {
        if (!request.IsWholeService && !entry->PartitionData.RangeContains(request.Partition))
        {
            return false;
        }

        auto it = request.PreviousResolves.find(entry->Locations.ConsistencyUnitId);
        if (it != request.PreviousResolves.end())
        {
            if (entry->FMVersion > it->second.FMVersion ||
                entry->StoreVersion > it->second.StoreVersion)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else 
        {
            return true;
        }
    }

    bool FabricClientImpl::LocationChangeNotificationAsyncOperation::TryAddToReply(
        ResolvedServicePartitionSPtr && entry,
        ServiceLocationNotificationRequestData const & request,
        __in vector<ResolvedServicePartitionSPtr> & partitions)
    {
        if (!ShouldAddEntry(entry, request))
        {
            return false;
        }

        // If the entry is already in the vector, do not add it again
        if (currentCuids_.insert(entry->Locations.ConsistencyUnitId).second)
        {
            // Insert succeeded, so the entry wasn't already in the cuids list
            partitions.push_back(move(entry));
            return true;
        }

        return false;
    }
}
