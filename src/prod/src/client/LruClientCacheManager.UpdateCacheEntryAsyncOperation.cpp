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

StringLiteral const TraceComponent("CacheManager.UpdateCacheEntryAsyncOperation");

LruClientCacheManager::UpdateCacheEntryAsyncOperation::UpdateCacheEntryAsyncOperation(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    ResolvedServicePartitionSPtr const & rsp,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(owner, name, activityHeader, timeout, callback, parent)
  , rsp_(rsp)
{
}

ErrorCode LruClientCacheManager::UpdateCacheEntryAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<UpdateCacheEntryAsyncOperation>(operation)->Error;
}

void LruClientCacheManager::UpdateCacheEntryAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->GetCachedPsd(thisSPtr);
}

void LruClientCacheManager::UpdateCacheEntryAsyncOperation::OnProcessCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.NotificationCacheUpdate(
        this->ClientContextId,
        this->ActivityId, 
        cacheEntry->Description.Version, 
        rsp_->Locations.ConsistencyUnitId.Guid,
        rsp_->Generation,
        rsp_->FMVersion,
        rsp_->StoreVersion);

    if (rsp_->StoreVersion == cacheEntry->Description.Version)
    {
        this->ProcessRsp(cacheEntry, thisSPtr);
    }
    else
    {
        this->InvalidateCachedPsd(cacheEntry, thisSPtr);
    }
}

void LruClientCacheManager::UpdateCacheEntryAsyncOperation::OnProcessRefreshedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.NotificationCacheUpdateOnRefresh(
        this->ClientContextId,
        this->ActivityId, 
        cacheEntry->Description.Version, 
        rsp_->Locations.ConsistencyUnitId.Guid,
        rsp_->Generation,
        rsp_->FMVersion,
        rsp_->StoreVersion);

    this->ProcessRsp(cacheEntry, thisSPtr);
}

void LruClientCacheManager::UpdateCacheEntryAsyncOperation::ProcessRsp(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    ResolvedServicePartitionSPtr snap = rsp_;
    auto error = cacheEntry->TryPutOrGetRsp(this->FullName, snap);

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: failed to update PSD: {1} with {2}, {3}",
            this->TraceId, 
            cacheEntry->Description,
            rsp_,
            error);

        this->Cache.TryRemove(this->GetNameWithoutMembers());

        this->TryComplete(thisSPtr, error);
    }
}
