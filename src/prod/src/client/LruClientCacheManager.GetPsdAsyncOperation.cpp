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

StringLiteral const TraceComponent("CacheManager.GetPsdAsyncOperation");

LruClientCacheManager::GetPsdAsyncOperation::GetPsdAsyncOperation(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(owner, name, activityHeader, timeout, callback, parent)
  , psd_()
{
}

ErrorCode LruClientCacheManager::GetPsdAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out PartitionedServiceDescriptor & result)
{
    auto casted = AsyncOperation::End<GetPsdAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        result = move(casted->psd_);
    }

    return casted->Error;
}

void LruClientCacheManager::GetPsdAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->GetCachedPsd(thisSPtr);
}

void LruClientCacheManager::GetPsdAsyncOperation::OnProcessCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheProcessing(
        this->ClientContextId,
        this->ActivityId, 
        this->GetCacheEntryName(cacheEntry),
        cacheEntry->Description.Version,
        -1); // request store version

    psd_ = cacheEntry->Description;

    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
}

void LruClientCacheManager::GetPsdAsyncOperation::OnProcessRefreshedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    Common::AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheRefreshed(
        this->ClientContextId,
        this->ActivityId, 
        this->GetCacheEntryName(cacheEntry),
        cacheEntry->Description.Version,
        -1); // request store version

    psd_ = cacheEntry->Description;

    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
}
