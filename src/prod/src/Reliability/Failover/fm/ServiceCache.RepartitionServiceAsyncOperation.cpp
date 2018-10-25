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

ServiceCache::RepartitionServiceAsyncOperation::RepartitionServiceAsyncOperation(
    ServiceCache & serviceCache,
    LockedServiceInfo && lockedServiceInfo,
    ServiceDescription && serviceDescription,
    vector<ConsistencyUnitDescription> && added,
    vector<ConsistencyUnitDescription> && removed,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , serviceCache_(serviceCache)
    , lockedServiceInfo_(move(lockedServiceInfo))
    , serviceDescription_(move(serviceDescription))
    , newServiceInfo_()
    , failoverUnits_()
{
    for (auto i = 0; i < added.size(); i++)
    {
        added_[FailoverUnitId(added[i].ConsistencyUnitId.Guid)] = move(added[i]);
    }

    for (auto i = 0; i < removed.size(); i++)
    {
        removed_[FailoverUnitId(removed[i].ConsistencyUnitId.Guid)] = move(removed[i]);
    }
}

ServiceCache::RepartitionServiceAsyncOperation::~RepartitionServiceAsyncOperation()
{
}

ErrorCode ServiceCache::RepartitionServiceAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<RepartitionServiceAsyncOperation>(operation);
    return thisPtr->Error;
}

void ServiceCache::RepartitionServiceAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    ASSERT_IF(
        serviceDescription_.PartitionCount == lockedServiceInfo_->ServiceDescription.PartitionCount,
        "PartitionCount not changed for repartition service: {0}", serviceDescription_);

    ASSERT_IF(
        added_.empty() && removed_.empty(),
        "Both added and removed partitions are empty: {0}", serviceDescription_);

    ASSERT_IF(
        !added_.empty() && !removed_.empty(),
        "Both added and removed partitions are non-empty: {0}, Added={1}, Removed={2}", serviceDescription_, added_.size(), removed_.size());

    RepartitionType::Enum repartitionType = RepartitionType::Invalid;
    if (!added_.empty())
    {
        repartitionType = RepartitionType::Add;

        ASSERT_IF(
            lockedServiceInfo_->ServiceDescription.PartitionCount + added_.size() != serviceDescription_.PartitionCount,
            "Partition count mismatch: Current={0}, Added={1}, New={2}",
            lockedServiceInfo_->ServiceDescription.PartitionCount,
            added_.size(),
            serviceDescription_.PartitionCount);
    }
    else if (!removed_.empty())
    {
        repartitionType = RepartitionType::Remove;

        ASSERT_IF(
            lockedServiceInfo_->ServiceDescription.PartitionCount - removed_.size() != serviceDescription_.PartitionCount,
            "Partition count mismatch: Current={0}, Removed={1}, New={2}",
            lockedServiceInfo_->ServiceDescription.PartitionCount,
            removed_.size(),
            serviceDescription_.PartitionCount);
    }
    else
    {
        serviceCache_.fm_.WriteWarning(Constants::ServiceSource, lockedServiceInfo_->Name, "{0}", repartitionType);
        TryComplete(thisSPtr, ErrorCodeValue::NotImplemented);
        return;
    }

    RepartitionInfoUPtr repartitionInfo = make_unique<FailoverManagerComponent::RepartitionInfo>(
        repartitionType,
        move(added_),
        move(removed_));

    newServiceInfo_ = make_shared<ServiceInfo>(*lockedServiceInfo_, move(serviceDescription_));
    newServiceInfo_->RepartitionInfo = move(repartitionInfo);
    newServiceInfo_->IsServiceUpdateNeeded = 
        (repartitionType == RepartitionType::Add || repartitionType == RepartitionType::Remove);

    if (repartitionType == RepartitionType::Add)
    {
        AddPartitions(thisSPtr);
    }
    else if (repartitionType == RepartitionType::Remove)
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
            RemovePartitions(thisSPtr);
        }
    }
}

void ServiceCache::RepartitionServiceAsyncOperation::AddPartitions(AsyncOperationSPtr const& thisSPtr)
{
    // Create new partitions
    for (auto const& pair : newServiceInfo_->RepartitionInfo->Added)
    {
        newServiceInfo_->AddFailoverUnitId(pair.first);

        auto failoverUnit = new FailoverUnit(
            pair.first,
            pair.second,
            newServiceInfo_->ServiceDescription.IsStateful,
            newServiceInfo_->ServiceDescription.HasPersistedState,
            newServiceInfo_->ServiceDescription.Name,
            newServiceInfo_);

        ASSERT_IFNOT(
            failoverUnit->PersistenceState == PersistenceState::ToBeInserted,
            "PeristenceState is not ToBeInserted for the newly created FailoverUnit");

        failoverUnit->HealthSequence = serviceCache_.fm_.FailoverUnitCacheObj.GetHealthSequence();
        serviceCache_.fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

        failoverUnits_.push_back(FailoverUnitUPtr(failoverUnit));
    }

    // Forced update of PLB (do not check for capacity).
    int64 dummyDuration = 0;
    serviceCache_.PLBUpdateService(*newServiceInfo_, dummyDuration, true);

    serviceCache_.fm_.Store.BeginUpdateServiceAndFailoverUnits(
        nullptr, // ApplicationInfo
        nullptr, // ServiceType
        *newServiceInfo_,
        failoverUnits_,
        [this](AsyncOperationSPtr const& updateOperation)
        {
            OnStoreUpdateCompleted(updateOperation);
        },
        thisSPtr);
}

void ServiceCache::RepartitionServiceAsyncOperation::RemovePartitions(AsyncOperationSPtr const& thisSPtr)
{
    // Forced update of PLB (do not check for capacity).
    int64 dummyDuration = 0;
    serviceCache_.PLBUpdateService(*newServiceInfo_, dummyDuration, true);
    serviceCache_.fm_.Store.BeginUpdateServiceAndFailoverUnits(
        nullptr, // ApplicationInfo
        nullptr, // ServiceType
        *newServiceInfo_,
        failoverUnits_,
        [this](AsyncOperationSPtr const& updateOperation)
        {
            OnStoreUpdateCompleted(updateOperation);
        },
        thisSPtr);
}

void ServiceCache::RepartitionServiceAsyncOperation::OnTombstoneServiceCreated(AsyncOperationSPtr const& createOperation)
{
    ErrorCode error = serviceCache_.EndCreateService(createOperation);

    if (error.IsSuccess())
    {
        RemovePartitions(createOperation->Parent);
    }
    else
    {
        TryComplete(createOperation->Parent, error);
    }
}

void ServiceCache::RepartitionServiceAsyncOperation::OnStoreUpdateCompleted(AsyncOperationSPtr const& updateOperation)
{
    int64 commitDuration;
    ErrorCode error = serviceCache_.fm_.Store.EndUpdateServiceAndFailoverUnits(
        updateOperation,
        nullptr, // ApplicationInfo
        nullptr, // ServiceType
        *newServiceInfo_,
        failoverUnits_,
        commitDuration);

    for (FailoverUnitUPtr const& failoverUnit : failoverUnits_)
    {
        serviceCache_.fm_.FailoverUnitCacheObj.ReportHealthAfterFailoverUnitUpdate(*failoverUnit, error.IsSuccess());
    }

    if (error.IsSuccess())
    {
        for (auto const& pair : newServiceInfo_->RepartitionInfo->Removed)
        {
            DynamicStateMachineTaskUPtr task;

            serviceCache_.fm_.FailoverUnitCacheObj.TryProcessTaskAsync(
                pair.first,
                task,
                serviceCache_.fm_.Federation.Instance);
        }
        serviceCache_.InternalUpdateServiceInfoCache(lockedServiceInfo_, move(newServiceInfo_), commitDuration, 0);

        for (size_t i = 0; i < failoverUnits_.size(); i++)
        {
            serviceCache_.fm_.FailoverUnitCacheObj.InsertFailoverUnitInCache(move(failoverUnits_[i]));
        }
    }
    else
    {
        // Repartitioning failed: need to revert the update to PLB
        int64 dummyDuration = 0;
        serviceCache_.PLBUpdateService(*lockedServiceInfo_, dummyDuration, true);
    }

    TryComplete(updateOperation->Parent, error);
}
