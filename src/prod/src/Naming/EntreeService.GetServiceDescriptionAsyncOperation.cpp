// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Transport;

    StringLiteral const TraceComponent("GetServiceDescription");

    EntreeService::GetServiceDescriptionAsyncOperation::GetServiceDescriptionAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , cachedName_()
      , isSystemService_(false)
      , psd_()
      , storeVersion_(ServiceModel::Constants::UninitializedVersion)
      , refreshCache_(true)
      , expectsReplyMessage_(true)
    {
    }

    EntreeService::GetServiceDescriptionAsyncOperation::GetServiceDescriptionAsyncOperation(
        __in GatewayProperties & properties,
        NamingUri const & name,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, name, activityHeader, timeout, callback, parent)
      , cachedName_()
      , isSystemService_(false)
      , psd_()
      , storeVersion_(ServiceModel::Constants::UninitializedVersion)
      , refreshCache_(false)
      , expectsReplyMessage_(true)
    {
    }

    EntreeService::GetServiceDescriptionAsyncOperation::GetServiceDescriptionAsyncOperation(
        __in GatewayProperties & properties,
        NamingUri const & name,
        FabricActivityHeader const & activityHeader,
        bool expectsReplyMessage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, name, activityHeader, timeout, callback, parent)
      , cachedName_()
      , isSystemService_(false)
      , psd_()
      , storeVersion_(ServiceModel::Constants::UninitializedVersion)
      , refreshCache_(false)
      , expectsReplyMessage_(expectsReplyMessage)
    {
    }

    ErrorCode EntreeService::GetServiceDescriptionAsyncOperation::End(
        AsyncOperationSPtr const & operation, 
        __out PartitionedServiceDescriptorSPtr & psd)
    {
        auto casted = AsyncOperation::End<GetServiceDescriptionAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            psd = move(casted->psd_);
        }

        return casted->Error;
    }

    ErrorCode EntreeService::GetServiceDescriptionAsyncOperation::End(
        AsyncOperationSPtr const & operation, 
        __out PartitionedServiceDescriptorSPtr & psd,
        __out __int64 & storeVersion)
    {
        auto casted = AsyncOperation::End<GetServiceDescriptionAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            psd = move(casted->psd_);
        }

        storeVersion = move(casted->storeVersion_);

        return casted->Error;
    }

    AsyncOperationSPtr EntreeService::GetServiceDescriptionAsyncOperation::BeginGetCached(
        __in GatewayProperties & properties,
        NamingUri const & name,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        //
        // This cached service description api is used as a part of determining the service endpoints(resolve) for a user request
        // or as a part of gateway logic to satisfy some queries. In these cases, we directly need to return
        // the service description object and we dont need to create a transport message that contains the service description.
        //

        auto operation = shared_ptr<AsyncOperation>(new GetServiceDescriptionAsyncOperation(
            properties,
            name,
            activityHeader,
            false,
            timeout,
            callback,
            parent));
        operation->Start(operation);

        return operation;
    }

    ErrorCode EntreeService::GetServiceDescriptionAsyncOperation::EndGetCached(
        AsyncOperationSPtr const & operation, 
        __out PartitionedServiceDescriptorSPtr & psd)
    {
        return GetServiceDescriptionAsyncOperation::End(operation, psd);
    }

    ErrorCode EntreeService::GetServiceDescriptionAsyncOperation::EndGetCached(
        AsyncOperationSPtr const & operation, 
        __out PartitionedServiceDescriptorSPtr & psd,
        __out __int64 & storeVersion)
    {
        return GetServiceDescriptionAsyncOperation::End(operation, psd, storeVersion);
    }

    MessageUPtr EntreeService::GetServiceDescriptionAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerGetServiceDescription(ServiceCheckRequestBody(false, Name));
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->RemainingTime > TimeSpan::Zero)
        {
            this->StartGetServiceDescription(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->TryGetNameFromRequestBody())
        {
            cachedName_ = this->Name.Name;

            TimedAsyncOperation::OnStart(thisSPtr);

            if (SystemServiceApplicationNameHelper::IsSystemServiceName(cachedName_))
            {
                isSystemService_ = true;
            }

            this->StartGetServiceDescription(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::StartGetServiceDescription(
        AsyncOperationSPtr const & thisSPtr)
    {
        if (isSystemService_)
        {
            this->FetchPsdFromFM(thisSPtr);
        }
        else if (refreshCache_)
        {
            this->RefreshPsdCache(thisSPtr);
        }
        else
        {
            this->GetPsdFromCache(thisSPtr);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::RefreshPsdCache(
        AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.TryRefreshPsdCache(
            TraceId,
            this->Name);

        auto operation = this->Properties.PsdCache.BeginTryRefresh(
            cachedName_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnTryRefreshPsdComplete(operation, false); },
            thisSPtr);
        this->OnTryRefreshPsdComplete(operation, true);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnTryRefreshPsdComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        bool isFirstWaiter = false;
        GatewayPsdCacheEntrySPtr cacheEntry;
        auto error = this->Properties.PsdCache.EndTryRefresh(operation, isFirstWaiter, cacheEntry);

        this->ProcessPsdCacheResult(thisSPtr, error, isFirstWaiter, cacheEntry);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::GetPsdFromCache(
        AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.TryGetPsdFromCache(
            TraceId,
            this->Name);

        auto operation = this->Properties.PsdCache.BeginTryGet(
            cachedName_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnTryGetPsdComplete(operation, false); },
            thisSPtr);
        this->OnTryGetPsdComplete(operation, true);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnTryGetPsdComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        bool isFirstWaiter = false;
        GatewayPsdCacheEntrySPtr cacheEntry;
        auto error = this->Properties.PsdCache.EndTryGet(operation, isFirstWaiter, cacheEntry);

        this->ProcessPsdCacheResult(thisSPtr, error, isFirstWaiter, cacheEntry);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::ProcessPsdCacheResult(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error,
        bool isFirstWaiter,
        GatewayPsdCacheEntrySPtr const & cacheEntry)
    {
        if (error.IsSuccess())
        {
            if (isFirstWaiter)
            {
                this->FetchPsdFromNaming(thisSPtr);
            }
            else if (cacheEntry->HasPsd)
            {
                this->Properties.Trace.ReadCachedPsd(
                    TraceId,
                    *cacheEntry->Psd,
                    cacheEntry->Psd->Version);

                psd_ = cacheEntry->Psd;

                this->SetReplyAndCompleteWithCurrentPsd(thisSPtr);
            }
            else
            {
                storeVersion_ = cacheEntry->ServiceNotFoundVersion;

                this->Properties.Trace.PsdNotFound(
                    TraceId,
                    this->Name,
                    storeVersion_);

                this->TryComplete(thisSPtr, ErrorCodeValue::UserServiceNotFound);
            }
        }
        else if (error.IsError(ErrorCodeValue::OperationCanceled))
        {
            this->Properties.Trace.RetryPsdCacheRead(
                TraceId,
                this->Name);

            if (!this->TryHandleRetryStart(thisSPtr))
            {
                // This AsyncOperation may have completed (timed out) just
                // before attempting to read from the cache - the waiter may
                // not have been registered yet when OnCompleted executes.
                //
                // AsyncOperation::Start() will cancel the child, which
                // may be the first waiter.
                //
                this->Properties.PsdCache.CancelWaiters(cachedName_);
            }
        }
        else
        {
            this->Properties.Trace.PsdCacheReadFailed(
                TraceId,
                this->Name,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::FetchPsdFromFM(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.FetchPsdFromFM(
            TraceId,
            this->Name);

        auto operation = this->Properties.AdminClient.BeginGetServiceDescription(
            cachedName_,
            this->ActivityHeader,
            this->RemainingTime,
            [this](AsyncOperationSPtr const& operation) { this->OnRequestToFMComplete(operation, false); },
            thisSPtr);
        this->OnRequestToFMComplete(operation, true);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnRequestToFMComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        PartitionedServiceDescriptor psd;
        Reliability::ServiceDescriptionReplyMessageBody replyBody;

        auto error = this->Properties.AdminClient.EndGetServiceDescription(operation, replyBody);

        error = ErrorCode::FirstError(error, replyBody.Error);

        if (error.IsSuccess())
        {
            auto const & svcDescription = replyBody.ServiceDescription;

            // The Naming Service is the only partitioned system service. The mapping
            // from name to partition is handled internally, so there is no actual
            // range maintained. Just provide fake [low, high] limits when returning
            // partition information.
            //
            if (cachedName_ == *ServiceModel::SystemServiceApplicationNameHelper::PublicNamingServiceName)
            {
                error = PartitionedServiceDescriptor::Create(
                    svcDescription, 
                    numeric_limits<__int64>::min(), // low range
                    numeric_limits<__int64>::max(), // high range
                    psd);
            }
            else
            {
                error = PartitionedServiceDescriptor::Create(
                    svcDescription, 
                    psd);
            }
        }
        
        if (error.IsSuccess())
        {
            wstring applicationName = *SystemServiceApplicationNameHelper::SystemServiceApplicationName;
            psd.MutableService.ApplicationName = move(applicationName);

            wstring serviceName = SystemServiceApplicationNameHelper::GetPublicServiceName(psd.Service.Name);
            psd.MutableService.Name = move(serviceName);

            std::vector<Reliability::ServiceCorrelationDescription> correlations;
            for(auto iter = psd.Service.Correlations.begin(); iter != psd.Service.Correlations.end(); ++iter)
            {                
                auto publicServiceName = SystemServiceApplicationNameHelper::GetPublicServiceName(iter->ServiceName);
                Reliability::ServiceCorrelationDescription desc(publicServiceName, iter->Scheme);
                correlations.push_back(move(desc));                
            }

            psd.MutableService.Correlations = move(correlations);

            psd_ = make_shared<PartitionedServiceDescriptor>(move(psd));

            this->SetReplyAndCompleteWithCurrentPsd(thisSPtr);
        }
        else if (this->IsRetryable(error))
        {
            this->Properties.Trace.RetryFetchPsdFromFM(
                TraceId,
                this->Name,
                error);

            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->Properties.Trace.FetchPsdFromFMFailed(
                TraceId,
                this->Name,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::FetchPsdFromNaming(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.FetchPsdFromNaming(
            TraceId,
            this->Name,
            false); // prefix

        this->StartStoreCommunication(thisSPtr, this->Name);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnStoreCommunicationFinished(
        AsyncOperationSPtr const & thisSPtr, 
        MessageUPtr && reply)
    {
        ServiceCheckReplyBody body;
        if (reply->GetBody(body))
        {
            if (body.DoesExist)
            {
                auto psdCacheEntry = make_shared<GatewayPsdCacheEntry>(move(body.TakePsd()));

                if (this->Properties.PsdCache.TryPutOrGet(psdCacheEntry))
                {
                    this->Properties.Trace.UpdatedPsdCache(
                        TraceId,
                        *psdCacheEntry->Psd,
                        psdCacheEntry->Psd->Version);
                }
                else
                {
                    // If the refreshing entry is older than the current cache entry for
                    // whatever reason, then just complete all waiters with the current cache
                    // entry.
                    //
                    this->Properties.PsdCache.CompleteWaitersWithMockEntry(psdCacheEntry);
                }

                psd_ = psdCacheEntry->Psd;

                this->SetReplyAndCompleteWithCurrentPsd(thisSPtr);
            }
            else
            {
                storeVersion_ = body.StoreVersion;

                this->Properties.Trace.PsdNotFound(
                    TraceId,
                    this->Name,
                    storeVersion_);
                    
                this->TryCompleteOnUserServiceNotFound(thisSPtr);
            }
        }
        else
        {
            this->TryCompleteAndFailCacheWaiters(thisSPtr, ErrorCode::FromNtStatus(reply->GetStatus()));
        }
    }

    // Must handle all cases in base classes where the first waiter can either
    // complete or re-enter the cache. The first waiter must release all other
    // pending waiters on such events.

    void EntreeService::GetServiceDescriptionAsyncOperation::OnCompleted()
    {
        if (this->Error.IsError(ErrorCodeValue::Timeout))
        {
            this->Properties.PsdCache.CancelWaiters(cachedName_);
        }
        else if (!this->Error.IsSuccess())
        {
            this->Properties.PsdCache.FailWaiters(cachedName_, this->Error);
        }

        __super::OnCompleted();
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnResolveNamingServiceRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const &)
    {
        this->Properties.PsdCache.CancelWaiters(cachedName_);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnResolveNamingServiceNonRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            this->Properties.PsdCache.CancelWaiters(cachedName_);
        }
        else
        {
            this->Properties.PsdCache.FailWaiters(cachedName_, error);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnRouteToNodeFailedRetryableError(
            AsyncOperationSPtr const & thisSPtr,
            MessageUPtr & reply,
            ErrorCode const & error)
    {
        // Unblock all waiters in preparation for retry
        //
        this->Properties.PsdCache.CancelWaiters(cachedName_);

        __super::OnRouteToNodeFailedRetryableError(
            thisSPtr,
            reply,
            error);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::OnRouteToNodeFailedNonRetryableError(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            this->TryCompleteAndCancelCacheWaiters(thisSPtr, error);
        }
        else
        {
            this->TryCompleteAndFailCacheWaiters(thisSPtr, error);
        }
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::SetReplyAndCompleteWithCurrentPsd(
        AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr replyMessage = nullptr;
        if (expectsReplyMessage_)
        {
            replyMessage = NamingMessage::GetServiceDescriptionReply(*psd_);
        }

        this->SetReplyAndComplete(thisSPtr, move(replyMessage), ErrorCodeValue::Success);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::TryCompleteOnUserServiceNotFound(
        AsyncOperationSPtr const & thisSPtr)
    {
        auto notFoundEntry = make_shared<GatewayPsdCacheEntry>(cachedName_, storeVersion_);

        if (refreshCache_)
        {
            this->Properties.PsdCache.TryRemove(cachedName_);
        }

        this->Properties.PsdCache.CompleteWaitersWithMockEntry(notFoundEntry);

        this->TryComplete(thisSPtr, ErrorCodeValue::UserServiceNotFound);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::TryCompleteAndFailCacheWaiters(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        WriteInfo(
            TraceComponent,
            "{0}: failed to fetch PSD: name={1} error={2}",
            TraceId,
            this->Name,
            error);

        this->Properties.PsdCache.FailWaiters(cachedName_, error);

        this->TryComplete(thisSPtr, error);
    }

    void EntreeService::GetServiceDescriptionAsyncOperation::TryCompleteAndCancelCacheWaiters(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        WriteInfo(
            TraceComponent,
            "{0}: failed to fetch PSD: name={1} error={2} (cancel waiters)",
            TraceId,
            this->Name,
            error);

        this->Properties.PsdCache.CancelWaiters(cachedName_);

        this->TryComplete(thisSPtr, error);
    }
}
