// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

CommitJobItem::CommitJobItem()
{
}

CommitJobItem::~CommitJobItem()
{
}

void CommitJobItem::SynchronizedProcess(FailoverManager & fm)
{
    UNREFERENCED_PARAMETER(fm);
}

void CommitJobItem::Close(FailoverManager & fm)
{
    UNREFERENCED_PARAMETER(fm);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FailoverUnitCommitJobItem::FailoverUnitCommitJobItem(
    LockedFailoverUnitPtr && failoverUnit,
    PersistenceState::Enum persistenceState,
    bool shouldUpdateLooupVersion,
    bool shouldReportHealth,
    std::vector<StateMachineActionUPtr> actions,
    bool isBackground)
    : failoverUnit_(std::move(failoverUnit))
    , persistenceState_(persistenceState)
    , shouldUpdateLooupVersion_(shouldUpdateLooupVersion)
    , shouldReportHealth_(shouldReportHealth)
    , actions_(std::move(actions))
    , replicaDifference_(failoverUnit_->ReplicaDifference)
    , error_(ErrorCodeValue::Success)
    , commitDuration_(-1)
    , isBackground_(isBackground)
{
}

void FailoverUnitCommitJobItem::SetCommitError(ErrorCode error)
{
    error_ = error;
}

void FailoverUnitCommitJobItem::SetCommitDuration(int64 commitDuration)
{
    commitDuration_ = commitDuration;
}

bool FailoverUnitCommitJobItem::ProcessJob(FailoverManager & failoverManager)
{
    int64 plbDuration;
    error_ = failoverManager.FailoverUnitCacheObj.PostUpdateFailoverUnit(failoverUnit_, persistenceState_, shouldUpdateLooupVersion_, shouldReportHealth_, error_, plbDuration);

    if (error_.IsSuccess())
    {
        failoverManager.BackgroundManagerObj.PostProcessFailoverUnit(
            failoverUnit_,
            actions_,
            true, // isUpdated
            replicaDifference_,
            commitDuration_,
            plbDuration,
            isBackground_);
    }

    return true;
}

void FailoverUnitCommitJobItem::SynchronizedProcess(FailoverManager & failoverManager)
{
    if (isBackground_)
    {
        failoverManager.BackgroundManagerObj.ProcessThreadContexts(*(failoverUnit_.Current), error_.IsSuccess());
    }
}

void FailoverUnitCommitJobItem::Close(FailoverManager & failoverManager)
{
    bool success = error_.IsSuccess();
    if (!failoverUnit_.Release(!success, success))
    {
        bool isSync = failoverManager.BackgroundManagerObj.ProcessFailoverUnit(move(failoverUnit_), false);
        if (!isSync)
        {
            return;
        }
    }

    if (isBackground_)
    {
        if (!ResumeBackgroundManager(failoverManager))
        {
            ResumeProcessingQueue(failoverManager);
        }
    }
    else
    {
        if (!ResumeProcessingQueue(failoverManager))
        {
            ResumeBackgroundManager(failoverManager);
        }
    }
}

bool FailoverUnitCommitJobItem::ResumeBackgroundManager(FailoverManager & failoverManager)
{
    return failoverManager.BackgroundManagerObj.IsThrottled() && failoverManager.BackgroundManagerObj.Resume();
}

bool FailoverUnitCommitJobItem::ResumeProcessingQueue(FailoverManager & failoverManager)
{
    return failoverManager.ProcessingQueue.IsThrottled() && failoverManager.ProcessingQueue.Resume();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CreateServiceCommitJobItem::CreateServiceCommitJobItem(
    Common::AsyncOperationSPtr const& operation,
    Common::ErrorCode error,
    int64 commitDuration, 
    int64 plbDuration)
    : operation_(operation)
    , error_(error)
    , commitDuration_(commitDuration)
    , plbDuration_(plbDuration)
{
}

bool CreateServiceCommitJobItem::ProcessJob(FailoverManager & fm)
{
    UNREFERENCED_PARAMETER(fm);

    auto createServiceAsyncOperation = AsyncOperation::Get<ServiceCache::CreateServiceAsyncOperation>(operation_);

    createServiceAsyncOperation->FinishCreateService(operation_, error_, commitDuration_, plbDuration_);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveServiceCommitJobItem::RemoveServiceCommitJobItem(
    LockedServiceInfo && lockedServiceInfo,
    int64 commitDuration)
    : lockedServiceInfo_(move(lockedServiceInfo))
    , commitDuration_(commitDuration)
{
}

bool RemoveServiceCommitJobItem::ProcessJob(FailoverManager & fm)
{
    fm.ServiceCacheObj.FinishRemoveService(lockedServiceInfo_, commitDuration_);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveServiceTypeCommitJobItem::RemoveServiceTypeCommitJobItem(
    LockedServiceType && lockedServiceType,
    int64 commitDuration)
    : lockedServiceType_(move(lockedServiceType))
    , commitDuration_(commitDuration)
{
}

bool RemoveServiceTypeCommitJobItem::ProcessJob(FailoverManager & fm)
{
    fm.ServiceCacheObj.FinishRemoveServiceType(lockedServiceType_, commitDuration_);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveApplicationCommitJobItem::RemoveApplicationCommitJobItem(
    LockedApplicationInfo && lockedApplicationInfo,
    int64 commitDuration,
    int64 plbDuration)
    : lockedApplicationInfo_(move(lockedApplicationInfo))
    , commitDuration_(commitDuration)
    , plbDuration_(plbDuration)
{
}

bool RemoveApplicationCommitJobItem::ProcessJob(FailoverManager & fm)
{
    fm.ServiceCacheObj.FinishRemoveApplication(lockedApplicationInfo_, commitDuration_, plbDuration_);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveNodeCommitJobItem::RemoveNodeCommitJobItem(
    LockedNodeInfo && lockedNodeInfo,
    int64 commitDuration)
    : lockedNodeInfo_(move(lockedNodeInfo))
    , commitDuration_(commitDuration)
{
}

bool RemoveNodeCommitJobItem::ProcessJob(FailoverManager & fm)
{
    fm.NodeCacheObj.FinishRemoveNode(lockedNodeInfo_, commitDuration_);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MarkServiceAsDeletedCommitJobItem::MarkServiceAsDeletedCommitJobItem(const ServiceInfoSPtr & serviceInfo)
    : serviceInfo_(serviceInfo)
{
}

bool MarkServiceAsDeletedCommitJobItem::ProcessJob(FailoverManager & fm)
{
    ErrorCode error = fm.InBuildFailoverUnitCacheObj.DeleteInBuildFailoverUnitsForService(serviceInfo_->Name);
    if (error.IsSuccess())
    {
        fm.ServiceCacheObj.MarkServiceAsDeletedAsync(serviceInfo_->Name, serviceInfo_->Instance);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CommitJobQueue::CommitJobQueue(FailoverManager & fm)
    : JobQueue(
      L"FMCommit." + fm.Id,
      fm, // Root
      true, // ForceEnqueue
      FailoverConfig::GetConfig().CommitQueueThreadCount,
      JobQueuePerfCounters::CreateInstance(L"FMCommit", fm.Id),
      UINT64_MAX,
      DequePolicy::Fifo)
{
}
