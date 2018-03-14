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

    StringLiteral const TraceComponent("PrefixResolveServiceAsyncOperation");

    EntreeService::PrefixResolveServiceAsyncOperation::PrefixResolveServiceAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : ServiceResolutionAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnTimeout(
        AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.PrefixPsdCache.CancelWaiters(this->Name);

        __super::OnTimeout(thisSPtr);
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->RemainingTime > TimeSpan::Zero)
        {
            this->GetPsd(thisSPtr);
        }
        else
        {
            this->TryCompleteAndCancelCacheWaiters(thisSPtr, ErrorCodeValue::Timeout);
        }
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::GetPsd(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->RequestBody.BypassCache)
        {
            this->FetchPsdFromNaming(thisSPtr);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: prefix resolve from cache: {1}",
                this->TraceId,
                this->Name);

            auto operation = this->Properties.PrefixPsdCache.BeginTryGet(
                this->Name,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnGetPsdComplete(operation, false); },
                thisSPtr);
            this->OnGetPsdComplete(operation, true);
        }
    }
    
    void EntreeService::PrefixResolveServiceAsyncOperation::OnGetPsdComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        
        auto const & thisSPtr = operation->Parent;

        bool isFirstWaiter = false;
        GatewayPsdCacheEntrySPtr cacheEntry;
        auto error = this->Properties.PrefixPsdCache.EndTryGet(operation, isFirstWaiter, cacheEntry);

        if (error.IsSuccess())
        {
            if (isFirstWaiter)
            {
                this->FetchPsdFromNaming(thisSPtr);
            }
            else if (cacheEntry->HasPsd)
            {
                this->Properties.Trace.ReadCachedPsd(
                    this->TraceId,
                    *cacheEntry->Psd,
                    cacheEntry->Psd->Version);

                this->ResolveLocations(thisSPtr, cacheEntry->Psd);
            }
            else
            {
                this->Properties.Trace.PsdNotFound(
                    this->TraceId,
                    this->Name,
                    0);

                this->TryComplete(thisSPtr, ErrorCodeValue::UserServiceNotFound);
            }
        }
        else if (error.IsError(ErrorCodeValue::OperationCanceled))
        {
            this->Properties.Trace.RetryPsdCacheRead(
                this->TraceId,
                this->Name);

            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->Properties.Trace.PsdCacheReadFailed(
                this->TraceId,
                this->Name,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::FetchPsdFromNaming(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.FetchPsdFromNaming(
            this->TraceId,
            this->Name,
            true); // prefix

        this->StartStoreCommunication(thisSPtr, this->Name.GetAuthorityName());
    }

    MessageUPtr EntreeService::PrefixResolveServiceAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPrefixResolveRequest(ServiceCheckRequestBody(true, this->Name)); // isPrefixMatch
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnStoreCommunicationFinished(
        AsyncOperationSPtr const & thisSPtr, 
        MessageUPtr && reply)
    {
        ServiceCheckReplyBody body;
        if (!reply->GetBody(body))
        {
            this->TryCompleteAndFailCacheWaiters(thisSPtr, ErrorCode::FromNtStatus(reply->GetStatus()));

            return;
        }

        if (!body.DoesExist)
        {
            this->TryCompleteAndFailCacheWaiters(thisSPtr, ErrorCodeValue::UserServiceNotFound);

            return;
        }

        auto psdCacheEntry = make_shared<GatewayPsdCacheEntry>(move(body.TakePsd()));

        if (this->Properties.PrefixPsdCache.TryPutOrGet(this->Name, psdCacheEntry))
        {
            this->Properties.Trace.UpdatedPsdCache(
                this->TraceId,
                *psdCacheEntry->Psd,
                psdCacheEntry->Psd->Version);
        }
        else
        {
            // If the refreshing entry is older than the current cache entry for
            // whatever reason, then just complete all waiters with the current cache
            // entry.
            //
            this->Properties.PrefixPsdCache.CompleteWaitersWithMockEntry(this->Name, psdCacheEntry);
        }

        this->ResolveLocations(thisSPtr, psdCacheEntry->Psd);
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnResolveNamingServiceRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const &)
    {
        this->Properties.PrefixPsdCache.CancelWaiters(this->Name);
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnResolveNamingServiceNonRetryableError(
        AsyncOperationSPtr const &,
        ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            this->Properties.PrefixPsdCache.CancelWaiters(this->Name);
        }
        else
        {
            this->Properties.PrefixPsdCache.FailWaiters(this->Name, error);
        }
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnRouteToNodeFailedRetryableError(
            AsyncOperationSPtr const & thisSPtr,
            MessageUPtr & reply,
            ErrorCode const & error)
    {
        // Unblock all waiters in preparation for retry
        //
        this->Properties.PrefixPsdCache.CancelWaiters(this->Name);

        __super::OnRouteToNodeFailedRetryableError(
            thisSPtr,
            reply,
            error);
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::OnRouteToNodeFailedNonRetryableError(
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

    void EntreeService::PrefixResolveServiceAsyncOperation::TryCompleteAndFailCacheWaiters(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        this->Properties.PrefixPsdCache.FailWaiters(this->Name, error);

        this->TryComplete(thisSPtr, error);
    }

    void EntreeService::PrefixResolveServiceAsyncOperation::TryCompleteAndCancelCacheWaiters(
        AsyncOperationSPtr const & thisSPtr, 
        ErrorCode const & error)
    {
        this->Properties.PrefixPsdCache.CancelWaiters(this->Name);

        this->TryComplete(thisSPtr, error);
    }
}
