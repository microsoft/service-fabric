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

ServiceCache::UpdateServiceTypesAsyncOperation::UpdateServiceTypesAsyncOperation(
    ServiceCache & serviceCache,
    vector<ServiceTypeIdentifier> && serviceTypeIds,
    uint64 sequenceNumber,
    Federation::NodeId const& nodeId,
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , serviceTypeIds_(move(serviceTypeIds))
    , sequenceNumber_(sequenceNumber)
    , nodeId_(nodeId)
    , serviceTypeUpdateEvent_(serviceTypeUpdateEvent)
    , pendingCount_()
    , errorTracker_()
{
}

ServiceCache::UpdateServiceTypesAsyncOperation::~UpdateServiceTypesAsyncOperation()
{
}

ErrorCode ServiceCache::UpdateServiceTypesAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<UpdateServiceTypesAsyncOperation>(operation);

    return thisPtr->Error;
}

void ServiceCache::UpdateServiceTypesAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    AcquireReadLock grab(serviceCache_.lock_);

    pendingCount_.store(1);

    for (ServiceTypeIdentifier const& serviceTypeId : serviceTypeIds_)
    {
        auto serviceType = serviceCache_.serviceTypes_.find(serviceTypeId);
        if (serviceType != serviceCache_.serviceTypes_.end())
        {
            ++pendingCount_;

            serviceCache_.BeginUpdateServiceType(
                serviceType->second,
                sequenceNumber_,
                nodeId_,
                serviceTypeUpdateEvent_,
                [this](AsyncOperationSPtr const& operation) mutable
                {
                    ErrorCode error = serviceCache_.EndUpdateServiceType(operation);

                    OnUpdateServiceTypeCompleted(operation, error);
                },
                thisSPtr);
        }
    }

    if (--pendingCount_ == 0)
    {
        TryComplete(thisSPtr, ErrorCode(errorTracker_.ReadValue()));
    }
}

void ServiceCache::UpdateServiceTypesAsyncOperation::OnUpdateServiceTypeCompleted(
    AsyncOperationSPtr const& operation,
    ErrorCode error)
{
    errorTracker_.Update(error);

    if (--pendingCount_ == 0)
    {
        TryComplete(operation->Parent, ErrorCode(errorTracker_.ReadValue()));
    }
}
