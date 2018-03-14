// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServiceCache::RemoveFromDisabledNodesAsyncOperation::RemoveFromDisabledNodesAsyncOperation(
    ServiceCache & serviceCache,
    uint64 sequenceNumber,
    Federation::NodeId const& nodeId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , sequenceNumber_(sequenceNumber)
    , nodeId_(nodeId)
    , pendingCount_()
    , errorTracker_()
{
}

ServiceCache::RemoveFromDisabledNodesAsyncOperation::~RemoveFromDisabledNodesAsyncOperation()
{
}

ErrorCode ServiceCache::RemoveFromDisabledNodesAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<RemoveFromDisabledNodesAsyncOperation>(operation);

    return thisPtr->Error;
}

void ServiceCache::RemoveFromDisabledNodesAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    AcquireReadLock grab(serviceCache_.lock_);

    pendingCount_.store(1);

    for (auto it = serviceCache_.serviceTypes_.begin(); it != serviceCache_.serviceTypes_.end(); ++it)
    {
        ++pendingCount_;

        serviceCache_.BeginUpdateServiceType(
            it->second,
            sequenceNumber_,
            nodeId_,
            ServiceTypeUpdateKind::Enabled,
            [this](AsyncOperationSPtr const& operation) mutable
            {
                ErrorCode error = serviceCache_.EndUpdateServiceType(operation);

                OnUpdateServiceTypeCompleted(operation, error);
            },
            thisSPtr);
    }

    if (--pendingCount_ == 0)
    {
        TryComplete(thisSPtr, ErrorCode(errorTracker_.ReadValue()));
    }
}

void ServiceCache::RemoveFromDisabledNodesAsyncOperation::OnUpdateServiceTypeCompleted(
    AsyncOperationSPtr const& operation,
    ErrorCode error)
{
    errorTracker_.Update(error);

    if (--pendingCount_ == 0)
    {
        TryComplete(operation->Parent, ErrorCode(errorTracker_.ReadValue()));
    }
}
