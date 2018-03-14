// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

namespace
{
    StringLiteral TraceReplicaUp("ReplicaUp");
}

void Reliability::FailoverManagerComponent::WriteToTextWriter(Common::TextWriter & w, ReplicasUpdateOperation::OperationType::Enum const & val)
{
    switch (val)
    {
    case ReplicasUpdateOperation::OperationType::ReplicaUp:
        w << "ReplicaUp";
        return;
    default: 
        Common::Assert::CodingError("Unknown ReplicasUpdateOperation Type");
    }
}

ReplicasUpdateOperation::ReplicasUpdateOperation(
    FailoverManager & failoverManager,
    OperationType::Enum type,
    NodeInstance const & from)
    :   type_(type),
        from_(from),
        timeoutHelper_(max(FailoverConfig::GetConfig().FMMessageRetryInterval - TimeSpan::FromSeconds(5.0), TimeSpan::FromSeconds(5.0))),
        root_(failoverManager, failoverManager.CreateComponentRoot()),
        completed_(false)
{
}

ReplicasUpdateOperation::~ReplicasUpdateOperation()
{
}

void ReplicasUpdateOperation::OnTimer()
{
    auto holder = root_.lock();
    if (holder)
    {
        FailoverManager & fm = holder->RootedObject;

        if (timeoutHelper_.IsExpired)
        {
            SetError(ErrorCodeValue::Timeout, fm);
        }
        else
        {
            AcquireWriteLock grab(lock_);

            if (completed_)
            {
                return;
            }

            for (auto it = retryList_.begin(); it != retryList_.end(); ++it)
            {
                FailoverUnitId const& failoverUnitId = it->first;
                DynamicStateMachineTaskUPtr & task = it->second;

                bool result = fm.FailoverUnitCacheObj.TryProcessTaskAsync(failoverUnitId, task, from_, false);
                if (!result)
                {
                    fm.WriteWarning(
                        "ReplicaUpdate", wformatString(failoverUnitId),
                        "FailoverUnit not found: {0}", failoverUnitId);
                }
            }

            retryList_.clear();

            timer_->Change(timeoutHelper_.GetRemainingTime());
        }
    }
}

void ReplicasUpdateOperation::SetError(ErrorCode error, FailoverManager & failoverManager)
{
    {
        AcquireWriteLock grab(lock_);
        if (completed_)
        {
            return;
        }

        if (pending_.size() > 0)
        {
            failoverManager.WriteWarning("ReplicaUpdate", "{0} pending for {1}: {2}",
                pending_.size(), *this, *pending_.begin());
        }

        error_ = ErrorCode::FirstError(error_, ErrorCodeValue::Timeout);
        completed_ = true;
    }

    Complete(failoverManager);
}

void ReplicasUpdateOperation::SetError(ErrorCode error, FailoverManager & failoverManager, FailoverUnitId const & failoverUnitId)
{
    {
        AcquireWriteLock grab(lock_);
        if (completed_)
        {
            return;
        }

        error_ = ErrorCode::FirstError(error_, error);
        if (!RemovePending(failoverUnitId))
        {
            return;
        }
    }

    Complete(failoverManager);
}

bool ReplicasUpdateOperation::RemovePending(FailoverUnitId const & failoverUnitId)
{
    auto removed = pending_.erase(failoverUnitId);
    ASSERT_IF(removed == 0,
        "FailoverUnit {0} not found for {1}",
        failoverUnitId, *this);

    if (pending_.size() > 0)
    {
        return false;
    }

    completed_ = true;
    return true;
}

void ReplicasUpdateOperation::Start(
    ReplicasUpdateOperationSPtr const & thisSPtr,
    FailoverManager & failoverManager,
    ReplicaUpMessageBody & replicaUpMessageBody)
{
    for (FailoverUnitInfo const& fuInfo : replicaUpMessageBody.ReplicaList)
    {
        pending_.insert(fuInfo.FailoverUnitDescription.FailoverUnitId);
    }

    for (FailoverUnitInfo const& fuInfo : replicaUpMessageBody.DroppedReplicas)
    {
        pending_.insert(fuInfo.FailoverUnitDescription.FailoverUnitId);
    }

    timer_ = Timer::Create(TimerTagDefault, [thisSPtr](TimerSPtr const&) { thisSPtr->OnTimer(); });
    timer_->Change(timeoutHelper_.GetRemainingTime());

    std::vector<Reliability::FailoverUnitInfo> & replicas = replicaUpMessageBody.GetReplicaList();
    for (auto it = replicas.begin(); it != replicas.end(); ++it)
    {
        ProcessReplica(thisSPtr, failoverManager, move(*it), false);
    }

    std::vector<Reliability::FailoverUnitInfo> & droppedReplicas = replicaUpMessageBody.GetDroppedReplicas();
    for (auto it = droppedReplicas.begin(); it != droppedReplicas.end(); ++it)
    {
        ProcessReplica(thisSPtr, failoverManager, move(*it), true);
    }

    {
        AcquireWriteLock grab(lock_);
        if (completed_ || pending_.size() > 0)
        {
            return;
        }

        completed_ = true;
    }

    Complete(failoverManager);
}

void ReplicasUpdateOperation::ProcessReplica(
    ReplicasUpdateOperationSPtr const & thisSPtr,
    FailoverManager & failoverManager,
    FailoverUnitInfo && failoverUnitInfo,
    bool isDropped)
{
    failoverManager.FTEvents.FTReplicaUpdateReceive(failoverUnitInfo.FailoverUnitDescription.FailoverUnitId.Guid, failoverUnitInfo, isDropped);

    ASSERT_IF(
        failoverManager.IsMaster != (failoverUnitInfo.FailoverUnitDescription.FailoverUnitId.Guid == Constants::FMServiceGuid),
        "Invalid FailoverUnit in ReplicaUp: {0}", failoverUnitInfo);

    auto serviceInfo = failoverManager.ServiceCacheObj.GetService(failoverUnitInfo.ServiceDescription.Name);
    if (serviceInfo)
    {
        if (serviceInfo->ServiceDescription.Instance > failoverUnitInfo.ServiceDescription.Instance)
        {
            failoverManager.WriteInfo(
                "ReplicaUpdate", wformatString(failoverUnitInfo.FailoverUnitDescription.FailoverUnitId),
                "Ignoring report for {0} as a newer version of the service exists.\r\nExisting: {1}\r\nIncoming: {2}",
                failoverUnitInfo.FailoverUnitDescription.FailoverUnitId, serviceInfo->ServiceDescription, failoverUnitInfo.ServiceDescription);

            AddResult(failoverManager, move(failoverUnitInfo), true);
            return;
        }
        else if (serviceInfo->ServiceDescription.Instance == failoverUnitInfo.ServiceDescription.Instance &&
                 serviceInfo->ServiceDescription.UpdateVersion < failoverUnitInfo.ServiceDescription.UpdateVersion)
        {
            ServiceDescription serviceDescription = failoverUnitInfo.ServiceDescription;
            ErrorCode error = failoverManager.ServiceCacheObj.UpdateService(move(serviceDescription));
            if (!error.IsSuccess())
            {
                AddRetry(failoverManager, move(failoverUnitInfo));
                return;
            }
        }
    }

    if (isDropped)
    {
        if (failoverUnitInfo.LocalReplica.IsUp || !failoverUnitInfo.LocalReplica.IsDropped)
        {
            TRACE_AND_TESTASSERT(
                failoverManager.WriteError,
                "ReplicaUpdate", wformatString(failoverUnitInfo.FailoverUnitDescription.FailoverUnitId),
                "Invalid Dropped replica from {0}: {1}", from_, failoverUnitInfo);

            return;
        }
    }

    FailoverUnitId failoverUnitId = failoverUnitInfo.FailoverUnitDescription.FailoverUnitId;
    ReplicaUpdateTask* pTask = new ReplicaUpdateTask(thisSPtr, failoverManager, move(failoverUnitInfo), isDropped, from_); 
    DynamicStateMachineTaskUPtr task(pTask);

    while (task)
    {
        bool result = failoverManager.FailoverUnitCacheObj.TryProcessTaskAsync(failoverUnitId, task, from_, false);
        if (!result)
        {
            ErrorCode error = pTask->ProcessMissingFailoverUnit();
            if (!error.IsError(ErrorCodeValue::FMFailoverUnitAlreadyExists))
            {
                if (!error.IsSuccess())
                {
                    SetError(error, failoverManager, failoverUnitId);
                }

                task = nullptr;
            }
        }
    }
}

void ReplicasUpdateOperation::AddResult(
    FailoverManager & failoverManager,
    FailoverUnitInfo && failoverUnitInfo,
    bool dropNeeded)
{
    FailoverUnitId id = failoverUnitInfo.FailoverUnitDescription.FailoverUnitId;

    {
        AcquireWriteLock grab(lock_);
        if (completed_)
        {
            return;
        }

        if (dropNeeded)
        {
            dropped_.push_back(move(failoverUnitInfo));
        }
        else
        {
            processed_.push_back(move(failoverUnitInfo));
        }

        if (!RemovePending(id))
        {
            return;
        }
    }

    Complete(failoverManager);
}

void ReplicasUpdateOperation::AddRetry(
	FailoverManager & failoverManager,
	FailoverUnitInfo && failoverUnitInfo)
{
	AcquireWriteLock grab(lock_);

	if (!timeoutHelper_.IsExpired)
	{
		FailoverUnitId failoverUnitId = failoverUnitInfo.FailoverUnitDescription.FailoverUnitId;
		ReplicaUpdateTask* pTask = new ReplicaUpdateTask(shared_from_this(), failoverManager, move(failoverUnitInfo), false, from_);
		DynamicStateMachineTaskUPtr task(pTask);

		if (retryList_.empty())
		{
			timer_->Change(TimeSpan::FromSeconds(1.0));
		}

		retryList_.insert(pair<FailoverUnitId, DynamicStateMachineTaskUPtr>(failoverUnitId, move(task)));
	}
}

void ReplicasUpdateOperation::Complete(FailoverManager & failoverManager)
{
    AcquireWriteLock grab(lock_);

    if (timer_)
    {
        timer_->Cancel();
        timer_ = nullptr;
    }

    auto thisRoot = shared_from_this();
    auto failoverManagerRoot = failoverManager.shared_from_this();
    Threadpool::Post([thisRoot, failoverManagerRoot, &failoverManager] { thisRoot->OnCompleted(failoverManager); });
}

void ReplicasUpdateOperation::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.Write("{0} from {1}", type_, from_);
}

ReplicaUpProcessingOperation::ReplicaUpProcessingOperation(
    FailoverManager & failoverManager,
    NodeInstance const & from)
    : ReplicasUpdateOperation(failoverManager, OperationType::ReplicaUp, from),
      isLastReplicaUpMessageFromRA_(false)
{
}

void ReplicaUpProcessingOperation::Process(
    ReplicasUpdateOperationSPtr const & thisSPtr,
    FailoverManager & failoverManager,
    ReplicaUpMessageBody & body)
{
    isLastReplicaUpMessageFromRA_ = body.IsLastReplicaUpMessage;

    auto nodeInfo = failoverManager.NodeCacheObj.GetNode(from_.Id);
    if (!nodeInfo || nodeInfo->NodeInstance.InstanceId < from_.InstanceId)
    {
        // Wait for NodeUp message to arrive for this node instance.
        failoverManager.WriteInfo(TraceReplicaUp, wformatString(from_.Id),
            "Ignoring ReplicaUp message from {0} because incoming NodeInstance is greater than existing",
            from_);
    }
    else
    {
        failoverManager.WriteInfo(TraceReplicaUp, wformatString(from_.Id),
            "Processing ReplicaUp message from {0}: {1}",
            from_, body);
        Start(thisSPtr, failoverManager, body);
    }
}

void ReplicaUpProcessingOperation::OnCompleted(FailoverManager & failoverManager)
{
    if (!error_.IsSuccess())
    {
        failoverManager.WriteWarning(TraceReplicaUp, wformatString(from_.Id),
            "Process ReplicaUp from {0} failed with {1}",
            from_, error_);

        SendReplicaUpReply(false, failoverManager);
    }
    else if (isLastReplicaUpMessageFromRA_)
    {
		auto thisRoot = shared_from_this();
        failoverManager.NodeCacheObj.BeginReplicaUploaded(
            from_,
            [this, thisRoot, &failoverManager](AsyncOperationSPtr const & operation) mutable
            {
                ErrorCode error = failoverManager.NodeCacheObj.EndReplicaUploaded(operation);

                if (!error.IsSuccess())
                {
                    failoverManager.WriteWarning(TraceReplicaUp, wformatString(from_.Id),
                        "Process last ReplicaUp from {0} failed with {1}",
                        from_, error);
                    return;
                }

                SendReplicaUpReply(true, failoverManager);
            },
            failoverManager.CreateAsyncOperationRoot());
    }
	else
	{
		SendReplicaUpReply(false, failoverManager);
	}
}

void ReplicaUpProcessingOperation::SendReplicaUpReply(
    bool isLastReplicaUpFromRAProcessedSuccessfully,
    FailoverManager & failoverManager)
{
    if (processed_.size() > 0 || dropped_.size() > 0 || isLastReplicaUpFromRAProcessedSuccessfully)
    {
        ReplicaUpMessageBody body(move(processed_), move(dropped_), isLastReplicaUpFromRAProcessedSuccessfully, failoverManager.IsMaster);
        failoverManager.WriteInfo(TraceReplicaUp, wformatString(from_.Id),
            "Sending ReplicaUpReply message to {0}: {1}",
            from_, body);

        MessageUPtr reply = RSMessage::GetReplicaUpReply().CreateMessage(body);
        reply->Headers.Add(GenerationHeader(failoverManager.Generation, failoverManager.IsMaster));
        failoverManager.SendToNodeAsync(move(reply), from_);
    }
}
