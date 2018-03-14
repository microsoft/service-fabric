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

StringLiteral const TraceComponent("CacheManager.ProcessServiceNotificationAsyncOperation");

LruClientCacheManager::ProcessServiceNotificationAsyncOperation::ProcessServiceNotificationAsyncOperation(
    __in LruClientCacheManager & owner,
    __in ServiceNotification & notification,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : CacheAsyncOperationBase(
      owner, 
      NamingUri(), 
      FabricActivityHeader(notification.GetNotificationPageId().NotificationId), 
      timeout, 
      callback, 
      parent)
  , steNotifications_()
  , generation_(notification.GetGenerationNumber())
  , pendingCount_(0)
  , notificationResults_()
  , error_()
  , resultsLock_()
{
    this->InitializeStes(notification);
}

ErrorCode LruClientCacheManager::ProcessServiceNotificationAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceNotificationResultSPtr> & results)
{
    auto casted = AsyncOperation::End<ProcessServiceNotificationAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        AcquireExclusiveLock lock(casted->resultsLock_);

        results = move(casted->notificationResults_);
    }

    return casted->Error;
}

void LruClientCacheManager::ProcessServiceNotificationAsyncOperation::InitializeStes(__in ServiceNotification & notification)
{
    auto partitions = notification.TakePartitions();

    for (auto const & steNotification : partitions)
    {
        NamingUri uri;
        
        if (NamingUri::TryParse(steNotification->GetPartition()->ServiceName, uri))
        {
            steNotifications_.insert(make_pair(
                uri, 
                steNotification));
        }
    }
}

void LruClientCacheManager::ProcessServiceNotificationAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->UpdateCacheEntries(thisSPtr);
}

void LruClientCacheManager::ProcessServiceNotificationAsyncOperation::UpdateCacheEntries(AsyncOperationSPtr const & thisSPtr)
{
    if (steNotifications_.empty())
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        pendingCount_.store(static_cast<LONG>(steNotifications_.size()));

        for (auto const & pair : steNotifications_)
        {
            auto const & name = pair.first;
            auto const & steNotification = pair.second;

            auto operation = AsyncOperation::CreateAndStart<UpdateServiceTableEntryAsyncOperation>(
                this->Owner,
                name,
                steNotification->GetPartition(),
                generation_,
                this->ActivityHeader,
                this->RemainingTimeout,
                [this, name, steNotification](AsyncOperationSPtr const & operation) { this->OnUpdateCacheEntryComplete(name, steNotification, operation, false); },
                thisSPtr);
            this->OnUpdateCacheEntryComplete(name, steNotification, operation, true);
        }
    }
}

void LruClientCacheManager::ProcessServiceNotificationAsyncOperation::OnUpdateCacheEntryComplete(
    NamingUri const & name,
    ServiceTableEntryNotificationSPtr const & steNotification,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    ServiceNotificationResultSPtr result;
    auto error = UpdateServiceTableEntryAsyncOperation::End(operation, result);

    if (error.IsSuccess())
    {
        if (result->IsEmpty)
        {
            this->Owner.ClearCacheEntriesWithName(
                name,
                this->ActivityHeader.ActivityId);
        }

        if (steNotification->IsMatchedPrimaryOnly())
        {
            result->SetMatchedPrimaryOnly();
        }

        AcquireExclusiveLock lock(resultsLock_);

        notificationResults_.push_back(move(result));
    }
    else
    {
        this->WriteInfo(
            TraceComponent,
            "{0} Failed to update entry: {1}, {2}, {3}",
            this->TraceId,
            name,
            *(steNotification->GetPartition()),
            error);

        AcquireExclusiveLock lock(resultsLock_);

        error_ = ErrorCode::FirstError(error_, error);
    }

    auto count = --pendingCount_;
    if (count < 0)
    {
        this->WriteWarning(
            TraceComponent,
            "{0} Invalid pending cache update ref-count on ({1}, {2}): count={3}",
            this->TraceId,
            name,
            *(steNotification->GetPartition()),
            count);
        Assert::TestAssert();
    }

    if (count <= 0)
    {
        this->TryComplete(thisSPtr, error_);
    }
}
