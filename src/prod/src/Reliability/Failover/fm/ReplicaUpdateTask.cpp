// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const ReplicaUpdateTaskTag("ReplicaUpdateTask");

ReplicaUpdateTask::ReplicaUpdateAction::ReplicaUpdateAction(
    ReplicaUpdateTask & task,
    bool dropNeeded,
    bool isFullyProcessed)
    : task_(task),
      dropNeeded_(dropNeeded),
      isFullyProcessed_(isFullyProcessed)
{
}

int ReplicaUpdateTask::ReplicaUpdateAction::OnPerformAction(FailoverManager & fm)
{
    ReplicasUpdateOperationSPtr operation = move(task_.operation_);

    if (isFullyProcessed_)
    {
        operation->AddResult(fm, move(task_.failoverUnitInfo_), dropNeeded_);
    }
    else
    {
        operation->AddRetry(fm, move(task_.failoverUnitInfo_));
    }

    return 0;
}

void ReplicaUpdateTask::ReplicaUpdateAction::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ReplicaUpdate:{0}", task_.failoverUnitInfo_.LocalReplica.FederationNodeId);
}

void ReplicaUpdateTask::ReplicaUpdateAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, wformatString(*this));
}

ReplicaUpdateTask::ReplicaUpdateTask(
    ReplicasUpdateOperationSPtr const & operation,
    FailoverManager & fm,
    FailoverUnitInfo && failoverUnitInfo,
    bool isDropped,
    Federation::NodeInstance const & from)
    : fm_(fm),
      operation_(operation),
      failoverUnitInfo_(move(failoverUnitInfo)),
      isDropped_(isDropped),
      from_(from)
{
}

void ReplicaUpdateTask::CheckFailoverUnit(
    LockedFailoverUnitPtr & lockedFailoverUnit,
    vector<StateMachineActionUPtr> & actions)
{
    fm_.FTEvents.FTReplicaUpdateProcess(lockedFailoverUnit->Id.Guid, from_);

    ASSERT_IF(!operation_, "Task already completed");

    bool dropNeeded = false;
    bool isFullyProcessed = true;
    if (!isDropped_)
    {
        ProcessNormalReplica(lockedFailoverUnit, dropNeeded, isFullyProcessed);
    }
    else
    {
        ProcessDroppedReplica(lockedFailoverUnit);
    }

    ServiceDescription const& targetServiceDescription = lockedFailoverUnit->ServiceInfoObj->ServiceDescription;
    if (targetServiceDescription.UpdateVersion > failoverUnitInfo_.ServiceDescription.UpdateVersion)
    {
        failoverUnitInfo_.ServiceDescription = targetServiceDescription;
    }

    actions.push_back(make_unique<ReplicaUpdateAction>(*this, dropNeeded, isFullyProcessed));
}

void ReplicaUpdateTask::ProcessNormalReplica(LockedFailoverUnitPtr & failoverUnit, __out bool & isDropped, __out bool & isFullyProcessed)
{
    if (failoverUnitInfo_.ReplicaCount == 0)
    {
        TRACE_AND_TESTASSERT(
            fm_.WriteError,
            "ReplicaUpdate", failoverUnit->IdString,
            "ReplicaUp message contains empty FailoverUnit info: {0}", failoverUnitInfo_);

        return;
    }

    isDropped = false;
    isFullyProcessed = true;

    ReplicaDescription const & incomingReplica = failoverUnitInfo_.LocalReplica;

    // TODO: Get the Replica by ReplicaId.
    auto replica = failoverUnit->GetReplica(from_.Id);
    if (replica == nullptr)
    {
        if (failoverUnit->CurrentConfiguration.DroppedCount < failoverUnit->CurrentConfiguration.ReplicaCount)
        {
            NodeInfoSPtr nodeInfo = fm_.NodeCacheObj.GetNode(from_.Id);

            failoverUnit.EnableUpdate();
            auto it = failoverUnit->AddReplica(
                move(nodeInfo),
                incomingReplica.ReplicaId,
                incomingReplica.InstanceId,
                incomingReplica.State,
                ReplicaFlags::None,
                ReplicaRole::None,
                ReplicaRole::Idle,
                incomingReplica.IsUp,
                failoverUnitInfo_.ServiceDescription.UpdateVersion,
                incomingReplica.PackageVersionInstance,
                PersistenceState::ToBeUpdated,
                DateTime::Now());

            replica = &(*it);
        }
        else
        {
            isDropped = true;
            return;
        }
    }
    else if (replica->IsDropped)
    {
        isDropped = true;
        return;
    }
    
    if (failoverUnitInfo_.CCEpoch > failoverUnit->CurrentConfigurationEpoch)
    {
        if (replica->FederationNodeInstance.InstanceId < from_.InstanceId)
        {
            failoverUnit.EnableUpdate();
            replica->FederationNodeInstance = from_;
        }

        if (failoverUnit->IsQuorumLost())
        {
            fm_.WriteWarning(
                "ReplicaUpdate", failoverUnit->IdString,
                "ReplicaUp message received for FailoverUnit reporting a higher epoch: {0}", failoverUnitInfo_);
    
            isFullyProcessed = false;
            return;        
        }
        else
        {
            if (!replica->IsDropped)
            {
                failoverUnit.EnableUpdate();
                failoverUnit->OnReplicaDown(*replica, true);
            }

            isDropped = true;
        }
    }
    else if (replica->InstanceId < incomingReplica.InstanceId)
    {
        if (incomingReplica.IsUp && !failoverUnit->HasPersistedState)
        {
            TRACE_AND_TESTASSERT(
                fm_.WriteError,
                "ReplicaUpdate", failoverUnit->IdString,
                "ReplicaUp message recieved for FailoverUnit that is not persisted: {0}", failoverUnitInfo_);

            isFullyProcessed = false;
            return;
        }

        failoverUnit.EnableUpdate();

        failoverUnit->OnReplicaDown(*replica, false);

        if (replica->IsPendingRemove && !replica->IsInConfiguration)
        {
            isFullyProcessed = false;
        }
        else
        {
            replica->ReplicaId = incomingReplica.ReplicaId;
            replica->InstanceId = incomingReplica.InstanceId;
            replica->ReplicaState = incomingReplica.State;
            replica->FederationNodeInstance = from_;
            replica->IsUp = incomingReplica.IsUp;
            if (replica->IsInConfiguration && !failoverUnit->IsToBeDeleted)
            {
                replica->IsToBeDroppedByFM = false;
            }

            if (incomingReplica.IsUp)
            {
                fm_.Events.EndpointsUpdated(
                    replica->FailoverUnitObj.Id.Guid, 
                    replica->FederationNodeInstance,
                    incomingReplica.ReplicaId,
                    incomingReplica.InstanceId,
                    ReplicaUpdateTaskTag,
                    L"",
                    incomingReplica.ReplicationEndpoint);

                replica->ReplicationEndpoint = incomingReplica.ReplicationEndpoint;
            }

            // After a rebuild, FM may not have the most recent information about replicas that are down.
            // If some replica reports a higher instance for a remote down replica, FM should update it.
            for (auto incoming = failoverUnitInfo_.Replicas.begin(); incoming != failoverUnitInfo_.Replicas.end(); incoming++)
            {
                auto existing = failoverUnit->GetReplica(incoming->Description.FederationNodeId);

                if (existing && existing->InstanceId < incoming->Description.InstanceId)
                {
                    if (existing->IsUp)
                    {
                        TRACE_AND_TESTASSERT(
                            fm_.WriteError,
                            "ReplicaUpdate", failoverUnit->IdString,
                            "Replica must be down on node {0}: {1}", existing->FederationNodeId, *failoverUnit);

                        isFullyProcessed = false;
                        return;
                    }

                    existing->InstanceId = incoming->Description.InstanceId;
                }
            }
        }

        if (isFullyProcessed)
        {
            replica->VersionInstance = incomingReplica.PackageVersionInstance;
        }
    }
    else if (replica->InstanceId == incomingReplica.InstanceId)
    {
        if (replica->FederationNodeInstance.InstanceId < from_.InstanceId)
        {
            if (incomingReplica.IsUp)
            {
                TRACE_AND_TESTASSERT(
                    fm_.WriteError,
                    "ReplicaUpdate", failoverUnit->IdString,
                    "Local replica should be Down: {0}", failoverUnitInfo_);

                isFullyProcessed = false;
                return;
            }

            failoverUnit.EnableUpdate();
            replica->FederationNodeInstance = from_;
            failoverUnit->OnReplicaDown(*replica, false);
        }

        if (!incomingReplica.IsUp && replica->IsUp)
        {
            failoverUnit.EnableUpdate();
            failoverUnit->OnReplicaDown(*replica, false);
        }

        if (replica->VersionInstance.InstanceId < incomingReplica.PackageVersionInstance.InstanceId)
        {
            failoverUnit.EnableUpdate();
            replica->VersionInstance = incomingReplica.PackageVersionInstance;
        }
    }
    else
    {
        fm_.WriteInfo("ReplicaUpdate", failoverUnit->IdString, "Ignoring stale replica: {0}", failoverUnitInfo_);
    }

    if (failoverUnit->IsOrphaned)
    {
        if (!replica->IsDropped)
        {
            failoverUnit.EnableUpdate();
            failoverUnit->OnReplicaDown(*replica, true);
        }

        isDropped = true;
    }
}

void ReplicaUpdateTask::ProcessDroppedReplica(LockedFailoverUnitPtr & failoverUnit)
{
    auto replica = failoverUnit->GetReplica(from_.Id);
    if (replica)
    {
        if  (replica->ReplicaId <= failoverUnitInfo_.LocalReplica.ReplicaId)
        {
            if (!replica->IsDropped)
            {
                failoverUnit.EnableUpdate();
                failoverUnit->OnReplicaDown(*replica, true);
            }
            if (replica->ReplicaId < failoverUnitInfo_.LocalReplica.ReplicaId)
            {
                failoverUnit.EnableUpdate();
                replica->ReplicaId = failoverUnitInfo_.LocalReplica.ReplicaId;
            }
            if (replica->InstanceId < failoverUnitInfo_.LocalReplica.InstanceId)
            {
                failoverUnit.EnableUpdate();
                replica->InstanceId = failoverUnitInfo_.LocalReplica.InstanceId;
            }
        }

        if (replica->FederationNodeInstance.InstanceId < from_.InstanceId)
        {
            failoverUnit.EnableUpdate();

            if (!replica->IsDropped)
            {
                failoverUnit.EnableUpdate();
                failoverUnit->OnReplicaDown(*replica, true);
            }

            replica->FederationNodeInstance = from_;
        }
    }
}

ErrorCode ReplicaUpdateTask::ProcessMissingFailoverUnit()
{
    ErrorCode error = ErrorCode::Success();

    if (!failoverUnitInfo_.IsLocalReplicaDeleted)
    {
        error = fm_.InBuildFailoverUnitCacheObj.AddFailoverUnitReport(failoverUnitInfo_, from_);
    }

    if (error.IsSuccess())
    {
        operation_->AddResult(fm_, move(failoverUnitInfo_), false);
    }
    else if (error.ReadValue() == ErrorCodeValue::FMServiceDeleteInProgress)
    {
        operation_->AddResult(fm_, move(failoverUnitInfo_), !isDropped_);
        error = ErrorCode::Success();
    }

    return error;
}
