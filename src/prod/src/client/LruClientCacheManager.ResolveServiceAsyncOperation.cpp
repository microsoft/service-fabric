// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Client;
using namespace Transport;
using namespace Reliability;
using namespace std;
using namespace Naming;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("CacheManager.ResolveServiceAsyncOperation");

LruClientCacheManager::ResolveServiceAsyncOperation::ResolveServiceAsyncOperation(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    ServiceResolutionRequestData const & request,
    ResolvedServicePartitionMetadataSPtr const & previousRspMetadata,
    FabricActivityHeader && activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(owner, name, move(activityHeader), timeout, callback, parent)
  , request_(request)
  , previousRspMetadata_(previousRspMetadata)
  , currentRsp_()
{
}

ErrorCode LruClientCacheManager::ResolveServiceAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out ResolvedServicePartitionSPtr & result,
    __out Common::ActivityId & activityId)
{
    auto casted = AsyncOperation::End<ResolveServiceAsyncOperation>(operation);

    activityId = casted->ActivityId;

    if (casted->Error.IsSuccess())
    {
        result = move(casted->currentRsp_);
    }

    return casted->Error;
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->InitializeRequestVersion();

    this->GetCachedPsd(thisSPtr);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::InitializeRequestVersion()
{
    if (previousRspMetadata_)
    {
        request_.Version = ServiceLocationVersion(
            previousRspMetadata_->FMVersion,
            previousRspMetadata_->Generation,
            previousRspMetadata_->StoreVersion);
    }
}

Naming::ResolvedServicePartitionSPtr && LruClientCacheManager::ResolveServiceAsyncOperation::TakeRspResult()
{
    return move(currentRsp_);
}

// From this point on, use cacheEntry->Name instead of this->FullName or this->GetNameWithoutMembers().
// When this is a prefix resolution request, the latter two properties will be the original long
// name being resolved while cacheEntry->Name will be the name of the actual prefix match.
//
void LruClientCacheManager::ResolveServiceAsyncOperation::OnProcessCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheProcessing(
        this->ClientContextId,
        this->ActivityId, 
        this->GetCacheEntryName(cacheEntry),
        cacheEntry->Description.Version,
        request_.Version.StoreVersion);

    if (!previousRspMetadata_ || previousRspMetadata_->StoreVersion == cacheEntry->Description.Version)
    {
        this->GetRsp(cacheEntry, thisSPtr);
    }
    else
    {
        // Only invalidate cached PSD when the application tries to refresh with
        // an RSP resolved from the new service version. This implies that there can
        // be a window where the old PSD is still cached upon first resolving
        // (and using) an RSP from the new service, which is okay.
        //
        this->InvalidateCachedPsd(cacheEntry, thisSPtr);
    }
}

void LruClientCacheManager::ResolveServiceAsyncOperation::GetRsp(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    auto operation = cacheEntry->BeginTryGetRsp(
        request_.Key,
        this->RemainingTimeout,
        [this, cacheEntry](AsyncOperationSPtr const & operation) { this->OnGetCachedRspComplete(cacheEntry, operation, false); },
        thisSPtr);
    this->OnGetCachedRspComplete(cacheEntry, operation, true);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnGetCachedRspComplete(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    bool isFirstWaiter;
    ResolvedServicePartitionSPtr rsp;

    auto error = cacheEntry->EndTryGetRsp(
        this->GetCacheEntryName(cacheEntry),
        operation, 
        isFirstWaiter, 
        rsp);

    if (error.IsSuccess())
    {
        if (isFirstWaiter)
        {
            this->OnRspCacheMiss(cacheEntry, thisSPtr);
        }
        else
        {
            this->ProcessCachedRsp(cacheEntry, rsp, thisSPtr);
        }
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: reading RSP cache failed: {1} error = {2}", 
            this->TraceId, 
            this->GetCacheEntryName(cacheEntry),
            error);

        this->Cache.TryRemove(cacheEntry->Name);

        this->TryComplete(thisSPtr, error);
    }
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnRspCacheMiss(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.RspCacheMiss(
        this->ClientContextId,
        this->ActivityId, 
        this->GetCacheEntryName(cacheEntry),
        request_.Key,
        request_.Version);

    // This resolution protocol is compatible with old cluster versions that
    // did not support including the PSD in resolution requests. The prefix
    // resolution feature has added this capability, which can benefit
    // the exact service resolution protocol as well in the future.
    // 
    request_.SetIncludePsd(false);

    auto request = NamingTcpMessage::GetResolveService(
        Common::make_unique<ServiceResolutionMessageBody>(
            cacheEntry->Name,
            request_));

    this->SendRequest(cacheEntry, move(request), thisSPtr);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::SendRequest(
    LruClientCacheEntrySPtr const & cacheEntry,
    Client::ClientServerRequestMessageUPtr && request,
    AsyncOperationSPtr const & thisSPtr)
{
    this->AddActivityHeader(request);

    auto operation = AsyncOperation::CreateAndStart<FabricClientImpl::RequestReplyAsyncOperation>(
        this->Client,
        // cacheEntry is null when sending a prefix resolution request
        cacheEntry ? cacheEntry->Name : this->GetNameWithoutMembers(),
        std::move(request),
        this->RemainingTimeout,
        [this, cacheEntry] (AsyncOperationSPtr const & operation) { this->OnRequestComplete(cacheEntry, operation, false); },
        thisSPtr);
    this->OnRequestComplete(cacheEntry, operation, true);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnRequestComplete(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    ClientServerReplyMessageUPtr reply;
    auto error = FabricClientImpl::RequestReplyAsyncOperation::End(operation, reply);

    this->ProcessReply(error, move(reply), cacheEntry, thisSPtr);
}

// Overridden in LruClientCacheManager.PrefixResolveServiceAsyncOperation
//
void LruClientCacheManager::ResolveServiceAsyncOperation::ProcessReply(
    __in ErrorCode & error,
    ClientServerReplyMessageUPtr && reply,
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    if (error.IsSuccess())
    {
        ResolvedServicePartition body;
        if (reply->GetBody(body))
        {
            error = this->ProcessRspReply(move(body), cacheEntry, thisSPtr);
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());

            this->WriteInfo(
                TraceComponent, 
                "{0}: reading RSP body failed: {1} error = {2}", 
                this->TraceId, 
                this->GetCacheEntryName(cacheEntry),
                error);
        }
    }

    if (!error.IsSuccess())
    {
        this->OnRspErrorAndComplete(cacheEntry, error, thisSPtr);
    }
}

ErrorCode LruClientCacheManager::ResolveServiceAsyncOperation::ProcessRspReply(
    ResolvedServicePartition && body,
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    auto rsp = make_shared<ResolvedServicePartition>(move(body));
    
    auto error = cacheEntry->TryPutOrGetRsp(
        this->GetCacheEntryName(cacheEntry),
        rsp, 
        this->GetCacheCallback(cacheEntry->Name));

    if (error.IsSuccess())
    {
        this->ProcessCachedRsp(cacheEntry, rsp, thisSPtr);
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: failed to process RSP reply: {1} error = {2}", 
            this->TraceId, 
            *rsp,
            error);

        this->Cache.TryRemove(cacheEntry->Name);
    }

    return error;
}

void LruClientCacheManager::ResolveServiceAsyncOperation::ProcessCachedRsp(
    LruClientCacheEntrySPtr const & cacheEntry,
    ResolvedServicePartitionSPtr const & rsp,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.RspCacheProcessing(
        this->ClientContextId,
        this->ActivityId, 
        rsp->Locations.ConsistencyUnitId.Guid,
        rsp->Generation,
        rsp->FMVersion,
        rsp->StoreVersion,
        request_.Key,
        request_.Version); 

    if (!previousRspMetadata_)
    {
        // PSD cache may actually be stale here if we
        // invalidated the RSP. Return the newly fetched
        // RSP and delay PSD invalidation until the RSP
        // is next invalidated again, at which point we will
        // notice the mismatched PSD version and fetch the
        // new PSD.
        //
        this->OnSuccess(rsp, thisSPtr);
    }
    else
    {
        LONG result;
        auto error = rsp->CompareVersion(*previousRspMetadata_, result);
        
        // Break RSP invalidation loop
        //
        previousRspMetadata_.reset();

        if (error.IsSuccess())
        {
            if (result > 0)
            {
                this->OnSuccess(rsp, thisSPtr);
            }
            else
            {
                this->InvalidateCachedRsp(cacheEntry, rsp, thisSPtr);
            }
        }
        else
        {
            // Only invalidate the PSD here (on error) and not other 
            // paths since we are able to break the invalidation loop
            // if the resolve request itself is actually bad
            // (e.g. an incorrect caller-supplied key).
            //
            this->InvalidateCachedPsd(cacheEntry, thisSPtr);
        }
    }
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnProcessRefreshedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    Common::AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheRefreshed(
        this->ClientContextId,
        this->ActivityId, 
        this->GetCacheEntryName(cacheEntry),
        cacheEntry->Description.Version,
        request_.Version.StoreVersion); 

    this->GetRsp(cacheEntry, thisSPtr);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::InvalidateCachedRsp(
    LruClientCacheEntrySPtr const & cacheEntry,
    ResolvedServicePartitionSPtr const & rsp,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.RspCacheInvalidation(
        this->ClientContextId,
        this->ActivityId, 
        rsp->Locations.ConsistencyUnitId.Guid,
        rsp->Generation,
        rsp->FMVersion,
        rsp->StoreVersion,
        request_.Key,
        request_.Version); 
    
    auto operation = cacheEntry->BeginTryInvalidateRsp(
        rsp,
        this->RemainingTimeout,
        [this, cacheEntry](AsyncOperationSPtr const & operation) { this->OnInvalidateCachedRspComplete(cacheEntry, operation, false); },
        thisSPtr);
    this->OnInvalidateCachedRspComplete(cacheEntry, operation, true);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnInvalidateCachedRspComplete(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    bool isFirstWaiter;
    ResolvedServicePartitionSPtr rsp;

    auto error = cacheEntry->EndTryInvalidateRsp(
        this->GetCacheEntryName(cacheEntry),
        operation, 
        isFirstWaiter, 
        rsp);

    if (error.IsSuccess())
    {
        if (isFirstWaiter)
        {
            this->OnRspCacheMiss(cacheEntry, thisSPtr);
        }
        else
        {
            this->ProcessCachedRsp(cacheEntry, rsp, thisSPtr);
        }
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: invalidating RSP cache failed: {1} error = {2}", 
            this->TraceId, 
            this->GetCacheEntryName(cacheEntry),
            error);

        this->TryComplete(thisSPtr, error);
    }
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnRspErrorAndComplete(
    LruClientCacheEntrySPtr const & cacheEntry,
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr)
{
    this->OnRspError(cacheEntry, error, thisSPtr);

    this->TryComplete(thisSPtr, error);
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnRspError(
    LruClientCacheEntrySPtr const & cacheEntry,
    ErrorCode const & error,
    AsyncOperationSPtr const &)
{
    this->WriteInfo(
        TraceComponent, 
        "{0}: Get RSP failed: {1} error = {2}", 
        this->TraceId, 
        this->GetCacheEntryName(cacheEntry),
        error);

    if (LruClientCacheManager::ErrorIndicatesInvalidPartition(error))
    {
        cacheEntry->TryRemoveRsp(request_.Key);

        cacheEntry->FailWaiters(request_.Key, error);
    }
    else
    {
        cacheEntry->CancelWaiters(request_.Key);
    }
}

void LruClientCacheManager::ResolveServiceAsyncOperation::OnSuccess(
    ResolvedServicePartitionSPtr const & rsp,
    AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (rsp->IsServiceGroup && this->FullName.Fragment.empty())
    {
        error = ErrorCodeValue::AccessDenied;

        this->WriteWarning(
            TraceComponent, 
            "{0}: only service group members can be resolved: {1} error = {2}", 
            this->TraceId, 
            this->FullName, 
            error);
    }
    else
    {
        currentRsp_ = rsp;
    }

    this->TryComplete(thisSPtr, error);
}
