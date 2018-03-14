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

ServiceCache::DeleteServiceAsyncOperation::DeleteServiceAsyncOperation(
    ServiceCache & serviceCache,
    wstring const& serviceName,
    bool const isForce,
    uint64 serviceInstance,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , serviceName_(serviceName)
    , isForce_(isForce)
    , serviceInstance_(serviceInstance)
    , newServiceInfo_()
    , lockedServiceInfo_()
{
}

ServiceCache::DeleteServiceAsyncOperation::~DeleteServiceAsyncOperation()
{
}

ErrorCode ServiceCache::DeleteServiceAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<DeleteServiceAsyncOperation>(operation);
    return thisPtr->Error;
}

void ServiceCache::DeleteServiceAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    if (!serviceCache_.ServiceExists(Constants::TombstoneServiceName))
    {
        serviceCache_.BeginCreateService(
            ServiceCache::GetTombstoneServiceDescription(),
            vector<ConsistencyUnitDescription>(),
            true, // IsCreateFromRebuild
            false, // IsServiceUpdateNeeded
            [this](AsyncOperationSPtr const& createOperation) { OnTombstoneServiceCreated(createOperation); },
            thisSPtr);
    }
    else
    {
        StartDeleteService(thisSPtr);
    }
}

void ServiceCache::DeleteServiceAsyncOperation::OnTombstoneServiceCreated(AsyncOperationSPtr const& createOperation)
{
    ErrorCode error = serviceCache_.EndCreateService(createOperation);

    if (error.IsSuccess())
    {
        StartDeleteService(createOperation->Parent);
    }
    else
    {
        TryComplete(createOperation->Parent, error);
    }
}

void ServiceCache::DeleteServiceAsyncOperation::StartDeleteService(AsyncOperationSPtr const& thisSPtr)
{
    ErrorCode error = serviceCache_.GetLockedService(serviceName_, lockedServiceInfo_);
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    if (lockedServiceInfo_->IsDeleted)
    {
        TryComplete(thisSPtr, ErrorCodeValue::FMServiceDoesNotExist);
        return;
    }
    else if (lockedServiceInfo_->IsToBeDeleted)
    {
        if (lockedServiceInfo_->IsForceDelete || !isForce_) // Allow convert normal deletion to forceful one
        {
            lockedServiceInfo_->AddDeleteOperation(thisSPtr);
            lockedServiceInfo_.Release();
            return;
        }
    }
    else if (serviceInstance_ < lockedServiceInfo_->Instance)
    {
        TryComplete(thisSPtr, ErrorCodeValue::StaleRequest);
        return;
    }

    newServiceInfo_ = make_shared<ServiceInfo>(*lockedServiceInfo_);
    newServiceInfo_->IsToBeDeleted = true;
    newServiceInfo_->IsForceDelete = isForce_;
    newServiceInfo_->AddDeleteOperation(thisSPtr);

    serviceCache_.fmStore_.BeginUpdateData(
        *newServiceInfo_,
        [this](AsyncOperationSPtr const& updateOperation) { OnStoreUpdateCompleted(updateOperation); },
        thisSPtr);
}

void ServiceCache::DeleteServiceAsyncOperation::OnStoreUpdateCompleted(AsyncOperationSPtr const& updateOperation)
{
    int64 commitDuration;
    ErrorCode error = serviceCache_.fm_.Store.EndUpdateData(*newServiceInfo_, updateOperation, commitDuration);

    if (error.IsSuccess())
    {
        // Make a copy of the ServiceInfo shared pointer to be used later for processing FailoverUnit tasks
        ServiceInfoSPtr serviceInfo = newServiceInfo_;

        int64 plbDuration(0);
        serviceCache_.InternalUpdateServiceInfoCache(lockedServiceInfo_, move(newServiceInfo_), commitDuration, plbDuration);

        for (auto const& failoverUnitId : serviceInfo->FailoverUnitIds)
        {
            // The default state machine tasks already handle the delete. Hence,
            // no need to create a new dynamic task.
            DynamicStateMachineTaskUPtr task;

            serviceCache_.fm_.FailoverUnitCacheObj.TryProcessTaskAsync(
                failoverUnitId,
                task,
                serviceCache_.fm_.Federation.Instance);
        }
    }

    lockedServiceInfo_.Release();
}
