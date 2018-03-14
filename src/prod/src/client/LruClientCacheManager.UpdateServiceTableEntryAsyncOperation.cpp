// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Client;
using namespace ClientServerTransport;
using namespace Transport;
using namespace Reliability;
using namespace std;
using namespace Naming;

StringLiteral const TraceComponent("CacheManager.UpdateServiceTableEntryAsyncOperation");

LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::UpdateServiceTableEntryAsyncOperation(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    ServiceTableEntrySPtr const & ste,
    GenerationNumber const & generation,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(owner, name, activityHeader, timeout, callback, parent)
  , ste_(ste)
  , generation_(generation)
  , rsp_()
{
}

ErrorCode LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out ServiceNotificationResultSPtr & result)
{
    auto casted = AsyncOperation::End<UpdateServiceTableEntryAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        if (casted->rsp_)
        {
            result = make_shared<ServiceNotificationResult>(casted->rsp_);
        }
        else
        {
            result = make_shared<ServiceNotificationResult>(
                casted->ste_,
                casted->generation_);
        }
    }
    else if (LruClientCacheManager::ErrorIndicatesInvalidService(casted->Error)
        || LruClientCacheManager::ErrorIndicatesInvalidPartition(casted->Error))
    {
        result = make_shared<ServiceNotificationResult>(
            casted->ste_,
            casted->generation_);

        return ErrorCodeValue::Success;
    }

    return casted->Error;
}

void LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (ste_->ServiceReplicaSet.IsEmpty() && !this->HasCachedPsd())
    {
        // Since the PSD is not cached and the entry is empty, the
        // service is likely to have been deleted already. Avoid 
        // attempting to fetch a PSD from the system that probably 
        // no longer exists and just build the notification
        // without detailed partition info.
        //
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        this->GetCachedPsd(thisSPtr);
    }
}

void LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::OnProcessCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.NotificationCacheUpdate(
        this->ClientContextId,
        this->ActivityId, 
        cacheEntry->Description.Version, 
        ste_->ConsistencyUnitId.Guid,
        generation_,
        ste_->Version,
        -1);

    if (this->TryCreateRsp(cacheEntry))
    {
        this->ProcessRsp(cacheEntry, thisSPtr);
    }
    else
    {
        this->InvalidateCachedPsd(cacheEntry, thisSPtr);
    }
}

void LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::OnProcessRefreshedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.NotificationCacheUpdateOnRefresh(
        this->ClientContextId,
        this->ActivityId, 
        cacheEntry->Description.Version, 
        ste_->ConsistencyUnitId.Guid,
        generation_,
        ste_->Version,
        -1);

    if (this->TryCreateRsp(cacheEntry))
    {
        this->ProcessRsp(cacheEntry, thisSPtr);
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: failed to build RSP: {1} for {2}",
            this->TraceId, 
            cacheEntry->Description,
            *ste_);

        // Just ignore the notificaton since it is most likely stale
        //
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}

void LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::ProcessRsp(
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
            "{0}: failed to update RSP: {1} with {2}, {3}",
            this->TraceId, 
            cacheEntry->Description,
            rsp_,
            error);

        this->Cache.TryRemove(this->GetNameWithoutMembers());

        this->TryComplete(thisSPtr, error);
    }
}

bool LruClientCacheManager::UpdateServiceTableEntryAsyncOperation::TryCreateRsp(
    LruClientCacheEntrySPtr const & cacheEntry)
{
    auto const & cachedPsd = cacheEntry->Description;

    PartitionInfo partitionInfo;
    if (cachedPsd.TryGetPartitionInfo(ste_->ConsistencyUnitId, partitionInfo))
    {
        rsp_ = make_shared<ResolvedServicePartition>(
            cachedPsd.IsServiceGroup,
            *ste_,
            partitionInfo,
            generation_,
            cachedPsd.Version,
            nullptr);

        return true;
    }

    return false;
}
