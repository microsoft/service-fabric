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

StringLiteral const TraceComponent("CacheManager.ParallelUpdateCacheEntriesAsyncOperation");

LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation::ParallelUpdateCacheEntriesAsyncOperation(
    __in LruClientCacheManager & owner,
    multimap<NamingUri, ResolvedServicePartitionSPtr> && rsps,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(owner, NamingUri(), FabricActivityHeader(activityId), timeout, callback, parent)
  , rsps_(move(rsps))
  , pendingCount_(0)
  , successRsps_()
  , successRspsLock_()
{
}

ErrorCode LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out vector<ResolvedServicePartitionSPtr> & result)
{
    auto casted = AsyncOperation::End<ParallelUpdateCacheEntriesAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        AcquireExclusiveLock lock(casted->successRspsLock_);

        result = move(casted->successRsps_);
    }

    return casted->Error;
}

void LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->UpdateCacheEntries(thisSPtr);
}

void LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation::UpdateCacheEntries(AsyncOperationSPtr const & thisSPtr)
{
    if (rsps_.empty())
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        pendingCount_.store(static_cast<LONG>(rsps_.size()));

        for (auto const & pair : rsps_)
        {
            auto const & name = pair.first;
            auto const & rsp = pair.second;

            auto operation = AsyncOperation::CreateAndStart<UpdateCacheEntryAsyncOperation>(
                this->Owner,
                name,
                rsp,
                this->ActivityHeader,
                this->RemainingTimeout,
                [this, name, rsp](AsyncOperationSPtr const & operation) { this->OnUpdateCacheEntryComplete(name, rsp, operation, false); },
                thisSPtr);
            this->OnUpdateCacheEntryComplete(name, rsp, operation, true);
        }
    }
}

void LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation::OnUpdateCacheEntryComplete(
    NamingUri const & name,
    ResolvedServicePartitionSPtr const & rsp,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = UpdateCacheEntryAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        AcquireExclusiveLock lock(successRspsLock_);

        successRsps_.push_back(rsp);
    }
    else
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Failed to update entry: {1}, {2}, {3}",
            this->TraceId,
            name,
            rsp,
            error);

        // Intentional no-op: Cache update here is best effort. Let the
        //                    normal V1 notification failure detection
        //                    path handle notification errors.
    }

    auto count = --pendingCount_;
    if (count < 0)
    {
        this->WriteWarning(
            TraceComponent,
            "{0} Invalid pending cache update ref-count on ({1}, {2}): count={3}",
            this->TraceId,
            name,
            rsp,
            count);
        Assert::TestAssert();
    }

    if (count <= 0)
    {

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
}
