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

StringLiteral const TraceComponent("CacheManager.PrefixResolveServiceAsyncOperation");

LruClientCacheManager::PrefixResolveServiceAsyncOperation::PrefixResolveServiceAsyncOperation(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    ServiceResolutionRequestData const & request,
    ResolvedServicePartitionMetadataSPtr const & previousRspMetadata,
    bool bypassCache,
    FabricActivityHeader && activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ResolveServiceAsyncOperation(
        owner, 
        name, 
        request,
        previousRspMetadata,
        move(activityHeader), 
        timeout, 
        callback, 
        parent)
    , prefixCache_(owner.prefixCache_)
    , bypassCache_(bypassCache)
{
}

ErrorCode LruClientCacheManager::PrefixResolveServiceAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out ResolvedServicePartitionSPtr & result,
    __out Common::ActivityId & activityId)
{
    auto casted = AsyncOperation::End<PrefixResolveServiceAsyncOperation>(operation);

    activityId = casted->ActivityId;

    if (casted->Error.IsSuccess())
    {
        result = casted->TakeRspResult();
    }

    return casted->Error;
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->InitializeRequestVersion();

    if (bypassCache_)
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: prefix resolve bypass cache: {1}",
            this->TraceId, 
            this->FullName);

        this->OnPsdCacheMiss(thisSPtr);
    }
    else
    {
        this->GetPrefixCachedPsd(thisSPtr);
    }
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::GetPrefixCachedPsd(
    AsyncOperationSPtr const & thisSPtr)
{
    auto operation = prefixCache_.BeginTryGet(
        this->GetNameWithoutMembers(),
        this->RemainingTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnGetCachedPsdComplete(operation, false); },
        thisSPtr);
    this->OnGetCachedPsdComplete(operation, true);
}

ErrorCode LruClientCacheManager::PrefixResolveServiceAsyncOperation::EndGetCachedPsd(
    AsyncOperationSPtr const & operation,
    __out bool & isFirstWaiter,
    __out LruClientCacheEntrySPtr & cacheEntry)
{
    return prefixCache_.EndTryGet(operation, isFirstWaiter, cacheEntry);
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::OnPsdCacheMiss(AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheMiss(
        this->ClientContextId,
        this->ActivityId, 
        this->FullName,
        true); // prefix 

    this->RequestData.SetIncludePsd(true);

    auto request = NamingTcpMessage::GetPrefixResolveService(
        Common::make_unique<ServiceResolutionMessageBody>(
            this->GetNameWithoutMembers(),
            this->RequestData,
            bypassCache_));

    this->SendRequest(LruClientCacheEntrySPtr(), move(request), thisSPtr);
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::ProcessReply(
    __in ErrorCode & error,
    ClientServerReplyMessageUPtr && reply,
    LruClientCacheEntrySPtr const & existingCacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    if (!error.IsSuccess())
    {
        // In RSP invalidation path, existingCacheEntry will be non-null
        //
        this->OnError(existingCacheEntry, error, thisSPtr);

        return;
    }

    ResolvedServicePartition body;
    if (!reply->GetBody(body))
    {
        error = ErrorCode::FromNtStatus(reply->GetStatus());

        this->WriteError(
            TraceComponent, 
            "{0}: reading RSP body failed: {1} error = {2}", 
            this->TraceId, 
            this->FullName, 
            error);

        this->OnError(existingCacheEntry, error, thisSPtr);

        return;
    }

    LruClientCacheEntrySPtr cacheEntry;

    if (existingCacheEntry)
    {
        cacheEntry = existingCacheEntry;
    }
    else
    {
        if (!body.Psd)
        {
            auto msg = wformatString(
                "{0}: RSP reply missing PSD for {1}",
                this->TraceId, 
                this->FullName); 

            this->WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            this->OnError(existingCacheEntry, ErrorCodeValue::OperationFailed, thisSPtr);

            return;
        }

        NamingUri resolvedName;
        if (!NamingUri::TryParse(body.Psd->Service.Name, resolvedName))
        {
            auto msg = wformatString(
                "{0}: invalid service name {1}",
                this->TraceId, 
                body.Psd->Service.Name);

            this->WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            this->OnError(existingCacheEntry, ErrorCodeValue::OperationFailed, thisSPtr);

            return;
        }

        cacheEntry = make_shared<LruClientCacheEntry>(resolvedName, *body.Psd);
    }

    bool updatedCache = prefixCache_.TryPutOrGet(this->GetNameWithoutMembers(), cacheEntry);

    if (updatedCache || existingCacheEntry)
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: {1} prefix cache: {2} : {3}",
            this->TraceId, 
            updatedCache ? "updated" : "existing",
            cacheEntry->GetKey(),
            cacheEntry->Description);

        error = this->ProcessRspReply(move(body), cacheEntry, thisSPtr);

        if (!error.IsSuccess())
        {
            this->OnRspErrorAndComplete(cacheEntry, error, thisSPtr);
        }
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: re-reading prefix cache: {1} : {2}",
            this->TraceId, 
            cacheEntry->GetKey(),
            cacheEntry->Description);

        this->GetPrefixCachedPsd(thisSPtr);
    }
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::InvalidateCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    prefixCache_.InnerCache.TryRemove(cacheEntry->GetKey());

    this->GetPrefixCachedPsd(thisSPtr);
}

void LruClientCacheManager::PrefixResolveServiceAsyncOperation::OnError(
    LruClientCacheEntrySPtr const & cacheEntry,
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr)
{
    this->WriteInfo(TraceComponent, "{0}: Prefix resolution failed: {1} error = {2}", this->TraceId, this->FullName, error);

    if (cacheEntry)
    {
        this->OnRspError(cacheEntry, error, thisSPtr);
    }

    if (LruClientCacheManager::ErrorIndicatesInvalidService(error))
    {
        prefixCache_.FailWaiters(this->GetNameWithoutMembers(), error);
    }
    else
    {
        prefixCache_.CancelWaiters(this->GetNameWithoutMembers());
    }

    this->TryComplete(thisSPtr, error);
}
