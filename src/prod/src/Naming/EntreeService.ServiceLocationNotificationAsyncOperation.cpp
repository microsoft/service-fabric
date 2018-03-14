// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Reliability;

    StringLiteral const TraceComponent("ServiceLocationNotification");
   
    EntreeService::ServiceLocationNotificationAsyncOperation::ServiceLocationNotificationAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, std::move(receivedMessage), TimeoutWithoutSendBuffer(timeout), callback, parent)
      , requestData_()
      , requestDataIndex_(0)
      , replyBuilder_()
      , broadcastRequests_()
      , broadcastRegisteredNames_()
      , firstBroadcastReceived_(false)
      , broadcastLock_()
      , waitingForBroadcast_(false)
      , noNewerData_(false)
    {
        replyBuilder_ = make_unique<GatewayNotificationReplyBuilder>(this->Properties.Trace, this->TraceId);
    }

    EntreeService::ServiceLocationNotificationAsyncOperation::~ServiceLocationNotificationAsyncOperation()
    {
    }

    TimeSpan EntreeService::ServiceLocationNotificationAsyncOperation::TimeoutWithoutSendBuffer(Common::TimeSpan const timeout)
    {
        return timeout.SubtractWithMaxAndMinValueCheck(CommonConfig::GetConfig().SendReplyBufferTimeout);
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        ServiceLocationNotificationRequestBody body;
        if (this->ReceivedMessage->GetBody(body))
        {
            requestData_ = body.TakeRequestData();
            if (requestData_.empty())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0}: Cannot receive notifications for an empty service list.", 
                    TraceId);

                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidNamingAction);
            }
            else
            {
                TimedAsyncOperation::OnStart(thisSPtr);

                this->ProcessNextRequestData(thisSPtr);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCode::FromNtStatus(this->ReceivedMessage->GetStatus()));
        }
    }

    // Avoid hitting the Naming Service with a high volume of parallel PSD requests
    // when the cache is cold. This adds a bit of latency for large notification poll
    // requests in the cold cache case, but also avoids fetching PSDs that may not
    // be needed yet due to paging.
    //
    void EntreeService::ServiceLocationNotificationAsyncOperation::ProcessNextRequestData(
        AsyncOperationSPtr const & thisSPtr)
    {
        auto const & requestData = requestData_[requestDataIndex_];

        auto operation = GetServiceDescriptionAsyncOperation::BeginGetCached(
            this->Properties,
            requestData.Name, 
            this->ActivityHeader,
            TimeoutWithoutSendBuffer(this->RemainingTime),
            [this](AsyncOperationSPtr const & operation) { this->OnGetServiceDescriptionComplete(operation, false); },
            thisSPtr);
        this->OnGetServiceDescriptionComplete(operation, true);
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::OnGetServiceDescriptionComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        PartitionedServiceDescriptorSPtr psd;
        __int64 storeVersion;
        auto error = GetServiceDescriptionAsyncOperation::EndGetCached(operation, psd, storeVersion);

        this->ProcessPsdForCurrentRequestData(
            thisSPtr,
            psd,
            storeVersion,
            error);
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::ProcessPsdForCurrentRequestData(
        AsyncOperationSPtr const & thisSPtr,
        PartitionedServiceDescriptorSPtr const & psd,
        __int64 storeVersion,
        ErrorCode const & error)
    {
        bool continueProcessing = true;

        auto const & requestData = requestData_[requestDataIndex_];

        if (error.IsSuccess())
        {
            if (!psd)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent,
                   "{0}: skipping invalid PSD: name={1}",
                    TraceId,
                    requestData.Name);
            }
            else if (requestData.IsWholeService)
            {
                for (auto const & cuid : psd->Cuids)
                {
                    if (!this->TryAddCachedServicePartitionToReply(requestData, psd, cuid))
                    {
                        this->AddBroadcastRequest(requestData.Name, cuid);
                    }
                }
            }
            else
            {
                ConsistencyUnitId cuid;
                if (psd->TryGetCuid(requestData.Partition, cuid))
                {
                    if (!this->TryAddCachedServicePartitionToReply(requestData, psd, cuid))
                    {
                        this->AddBroadcastRequest(requestData.Name, cuid);
                    }
                }
                else
                {
                    this->Properties.Trace.NotificationInvalidatePsd(
                        TraceId,
                        requestData.Name,
                        psd->Version,
                        requestData.Partition);

                    this->TryAddFailureToReply(requestData, ErrorCodeValue::InvalidServicePartition, psd->Version);

                    this->Properties.PsdCache.TryRemove(requestData.Name.Name);
                }
            }
        }
        else if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            this->Properties.Trace.NotificationServiceNotFound(
                TraceId,
                requestData.Name,
                storeVersion);

            this->TryAddFailureToReply(requestData, ErrorCodeValue::UserServiceNotFound, storeVersion);

            this->AddBroadcastRequest(requestData.Name, ConsistencyUnitId(Guid::Empty()));
        }
        else // error
        {
            this->Properties.Trace.NotificationPsdFetchFailed(
                TraceId,
                requestData.Name,
                error);

            continueProcessing = false;
        }

        if (continueProcessing && !replyBuilder_->MessageSizeLimitEncountered)
        {
            if (++requestDataIndex_ < requestData_.size())
            {
                Threadpool::Post([this, thisSPtr] { this->ProcessNextRequestData(thisSPtr); });
            }
            else if (replyBuilder_->HasResults)
            {
                this->CompleteWithCurrentResults(thisSPtr);
            }
            else
            {
                this->WaitForNextBroadcast(thisSPtr);
            }
        }
        else
        {
            this->CompleteWithCurrentResultsOrError(thisSPtr, error);
        }
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::WaitForNextBroadcast(
        AsyncOperationSPtr const & thisSPtr)
    {
        AcquireExclusiveLock lock(broadcastLock_);

        this->Properties.Trace.NotificationWaitBroadcast(
            this->TraceId, 
            broadcastRequests_.Entries);

        waitingForBroadcast_ = true;

        auto error = this->Properties.BroadcastEventManager.AddRequests(    
            thisSPtr,
            [this] (UserServiceCuidMap const & changedCuids, AsyncOperationSPtr const & thisSPtr) 
            {
                this->OnBroadcastNotification(changedCuids, thisSPtr);
            },
            move(broadcastRequests_));

        broadcastRequests_.Clear();
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::OnBroadcastNotification(
        UserServiceCuidMap const & changedCuids,
        AsyncOperationSPtr const & thisSPtr)
    {
        UNREFERENCED_PARAMETER(changedCuids);

        firstBroadcastReceived_ = true;

        if (this->TryUnregisterBroadcastRequests(thisSPtr))
        {
            requestDataIndex_ = 0;

            this->ProcessNextRequestData(thisSPtr);
        }
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::AddBroadcastRequest(
        NamingUri const & name,
        Reliability::ConsistencyUnitId const & cuid)
    {
        if (broadcastRequests_.TryAdd(name, cuid))
        {
            broadcastRegisteredNames_.insert(name);
        }
    }

    bool EntreeService::ServiceLocationNotificationAsyncOperation::TryUnregisterBroadcastRequests(
        AsyncOperationSPtr const & thisSPtr)
    {
        AcquireExclusiveLock lock(broadcastLock_);

        if (waitingForBroadcast_)
        {
            this->Properties.Trace.NotificationUnregisterBroadcast(this->TraceId);

            waitingForBroadcast_ = false;

            this->Properties.BroadcastEventManager.RemoveRequests(
                thisSPtr,
                move(broadcastRegisteredNames_));

            broadcastRegisteredNames_.clear();

            return true;
        }
        else
        {
            return false;
        }
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::OnCompleted()
    {
        this->TryUnregisterBroadcastRequests(shared_from_this());

        RequestAsyncOperationBase::OnCompleted();
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::OnTimeout(Common::AsyncOperationSPtr const &)
    {
        noNewerData_ = true;

        // OnTimeout is called after TryStartComplete returns true, so changing the Reply is safe
        this->Reply = NamingMessage::GetServiceLocationChangeListenerReply(ServiceLocationNotificationReplyBody());
    }

    bool EntreeService::ServiceLocationNotificationAsyncOperation::CanConsiderSuccess()
    {
        if (RequestAsyncOperationBase::CanConsiderSuccess())
        {
            return true;
        }

        return noNewerData_;
    }

    bool EntreeService::ServiceLocationNotificationAsyncOperation::TryAddCachedServicePartitionToReply(
        ServiceLocationNotificationRequestData const & request,
        PartitionedServiceDescriptorSPtr const & psd,
        ConsistencyUnitId const & cuid)
    {
        auto previous = request.PreviousResolves.find(cuid);
        bool updated = false;
        if (previous == request.PreviousResolves.end() ||
            previous->second.StoreVersion <= psd->Version)
        {
            ServiceTableEntry entry;
            GenerationNumber generation;
            if (this->Properties.Resolver.TryGetCachedEntry(cuid, entry, generation))
            {
                if (previous != request.PreviousResolves.end())
                {
                    ServiceLocationVersion const & previousVersion = previous->second;
                    if ((previousVersion.Generation < generation) ||
                        ((previousVersion.Generation == generation) && (previousVersion.FMVersion < entry.Version)) ||
                        (previousVersion.StoreVersion < psd->Version))
                    {
                        updated = true;
                    }
                }
                else
                {
                    updated = true;
                }

                this->Properties.Trace.NotificationProcessRequest(
                    this->TraceId, 
                    request.Name,
                    (previous != request.PreviousResolves.end()) ? previous->second.ToString() : L"[]", 
                    cuid.Guid, 
                    generation.Value,
                    entry.Version,
                    psd->Version,
                    updated);

                if (updated)
                {
                    ResolvedServicePartition newRsp(
                        psd->IsServiceGroup,
                        entry,
                        psd->GetPartitionInfo(cuid),
                        generation,
                        psd->Version,
                        nullptr);

                    return replyBuilder_->TryAdd(request, move(newRsp));
                }
            }
            else
            {
                this->Properties.Trace.NotificationRspNotFound(
                    TraceId,
                    request.Name,
                    cuid.Guid);

                // Do not return ServiceOffline until we've tried waiting
                // for a broadcast first
                //
                if (firstBroadcastReceived_)
                {
                    this->TryAddFailureToReply(request, ErrorCodeValue::ServiceOffline, psd->Version);
                }

                this->Properties.PsdCache.TryRemove(request.Name.Name);
            }
        }

        return updated;
    }
    
    bool EntreeService::ServiceLocationNotificationAsyncOperation::TryAddFailureToReply(
        ServiceLocationNotificationRequestData const & request,
        ErrorCodeValue::Enum const newError,
        __int64 storeVersion)
    {
        if (storeVersion == ServiceModel::Constants::UninitializedVersion)
        {
            TRACE_WARNING_AND_TESTASSERT(
                TraceComponent,
                "{0}: TryAddFailureToReply: New error should have a valid version: error={1}, version={2}, name={3}", 
                TraceId,
                newError, 
                storeVersion,
                request.Name);
        }

        AddressDetectionFailure failure(
            request.Name, 
            request.Partition, 
            newError, 
            storeVersion);
        return replyBuilder_->TryAdd(request, move(failure));
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::CompleteWithCurrentResultsOrError(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (replyBuilder_->HasResults)
        {
            this->CompleteWithCurrentResults(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::ServiceLocationNotificationAsyncOperation::CompleteWithCurrentResults(
        AsyncOperationSPtr const & thisSPtr)
    {
        this->SetReplyAndComplete(
            thisSPtr,
            replyBuilder_->CreateMessage(requestDataIndex_),
            ErrorCodeValue::Success);
    }
}
