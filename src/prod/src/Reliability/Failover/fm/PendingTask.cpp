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

PendingTask::PendingTask(FailoverManager & fm)
    : StateMachineTask(), fm_(fm)
{
}

void PendingTask::CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, vector<StateMachineActionUPtr> & actions)
{
    FailoverUnit & failoverUnit = *lockedFailoverUnit;
    FailoverConfig const & config = FailoverConfig::GetConfig();

    if (lockedFailoverUnit->IsStateful &&
        lockedFailoverUnit->NoData &&
        static_cast<int>(lockedFailoverUnit->CurrentConfiguration.ReplicaCount) >= lockedFailoverUnit->MinReplicaSetSize)
    {
        lockedFailoverUnit.EnableUpdate();
        lockedFailoverUnit->NoData = false;
    }

    if (failoverUnit.IsToBeDeleted)
    {
        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            if (!it->IsDeleted && it->IsNodeUp && (it->IsUp || it->IsDropped || failoverUnit.ServiceInfoObj->IsForceDelete))
            {
                if (failoverUnit.IsStateful)
                {
                    it->GenerateAction<DeleteReplicaAction>(actions, *it);
                }
                else
                {
                    it->GenerateAction<RemoveInstanceAction>(actions, *it);
                }
            }
        }

        if (failoverUnit.IsOrphaned)
        {
            if ((failoverUnit.HealthSequence + 1 < fm_.FailoverUnitCacheObj.AckedHealthSequence) &&
                !failoverUnit.HasPendingUpgradeOrDeactivateNodeReplica &&
                ((failoverUnit.DeletedReplicaCount == failoverUnit.ReplicaCount &&
                  failoverUnit.GetUpdateTime() + config.DeletedReplicaKeepDuration < Stopwatch::Now()) ||
                 (failoverUnit.GetUpdateTime() + config.OfflineReplicaKeepDuration < Stopwatch::Now())))
            {
                if (fm_.FailoverUnitCacheObj.IsSafeToRemove(*lockedFailoverUnit))
                {
                    lockedFailoverUnit.EnableUpdate();
                    failoverUnit.PersistenceState = PersistenceState::ToBeDeleted;
                }
                else
                {
                    actions.push_back(make_unique<SaveLookupVersionAction>(lockedFailoverUnit->LookupVersion));
                }
            }
        }
        else if (failoverUnit.UpReplicaCount == 0)
        {
            if (!failoverUnit.HasPersistedState ||
                ((failoverUnit.CurrentConfiguration.IsEmpty || 
                  failoverUnit.CurrentConfiguration.DeletedCount >= failoverUnit.CurrentConfiguration.ReadQuorumSize) &&
                 (failoverUnit.PreviousConfiguration.IsEmpty || 
                  failoverUnit.PreviousConfiguration.DeletedCount >= failoverUnit.PreviousConfiguration.ReadQuorumSize)))
            {
                bool isFTMarkedForRepartitionRemove = false;
                if (failoverUnit.ServiceInfoObj->RepartitionInfo)
                {
                    // Service is under repartition. Check if the current failover unit is being removed.
                    auto it = failoverUnit.ServiceInfoObj->RepartitionInfo->Removed.find(failoverUnit.Id);
                    if (it != failoverUnit.ServiceInfoObj->RepartitionInfo->Removed.end())
                    {
                        isFTMarkedForRepartitionRemove = true;
                    }
                }

                bool partitionCountsMatch = failoverUnit.ServiceInfoObj->FailoverUnitIds.size() == failoverUnit.ServiceInfoObj->ServiceDescription.PartitionCount;

                if(partitionCountsMatch || isFTMarkedForRepartitionRemove || failoverUnit.ServiceInfoObj->IsForceDelete)
                {
                    actions.push_back(make_unique<DeleteServiceAction>(failoverUnit.ServiceInfoObj, failoverUnit.Id));

                    lockedFailoverUnit.EnableUpdate();
                    failoverUnit.SetOrphaned(fm_);
                }
            }
            else if (failoverUnit.CurrentConfiguration.DroppedCount == failoverUnit.CurrentConfiguration.ReplicaCount)
            {
                lockedFailoverUnit.EnableUpdate();
                failoverUnit.ClearConfiguration();
            }
            else 
            {
                fm_.FTEvents.FTQuorumLossDuringDelete(failoverUnit.Id.Guid, failoverUnit);

                if (fm_.BackgroundManagerObj.IsAdminTraceEnabled)
                {
                    fm_.FTEvents.FTQuorumLossDuringDeleteAdmin(failoverUnit.IdString, wformatString(failoverUnit));
                }
            }
        }

        return;
    }

    if (failoverUnit.HasPersistedState && failoverUnit.IsStable)
    {
        ProcessExtraStandByReplicas(lockedFailoverUnit);
    }

    bool isPrimaryAvailable = failoverUnit.IsReconfigurationPrimaryAvailable();

    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
    {
        if (it->IsPendingRemove && !it->IsInConfiguration && isPrimaryAvailable)
        {
            failoverUnit.ReconfigurationPrimary->GenerateAction<RemoveReplicaAction>(actions, *it, failoverUnit.ReconfigurationPrimary->FederationNodeInstance);
        }
    }

    if (failoverUnit.IsInReconfiguration)
    {
        if (failoverUnit.IsReconfigurationPrimaryActive() && failoverUnit.ShouldGenerateReconfigurationAction())
        {
            actions.push_back(make_unique<DoReconfigurationAction>(*lockedFailoverUnit.Current));
        }
    }
    else
    {
        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            Replica & replica = *it;

            if (failoverUnit.IsStateful)
            {
                if (replica.IsDropped && !replica.IsDeleted)
                {
                    if (replica.IsNodeUp)
                    {
                        replica.GenerateAction<DeleteReplicaAction>(actions, replica);
                    }
                }
                else if (replica.IsToBeDropped && !replica.IsInConfiguration)
                {
                    if (replica.IsUp)
                    {
                        replica.GenerateAction<DeleteReplicaAction>(actions, replica);
                    }
                }
                else if (replica.IsInBuild && replica.IsUp)
                {
                    if (replica.CurrentConfigurationRole == ReplicaRole::Primary && failoverUnit.IsCreatingPrimary)
                    {
                        replica.GenerateAction<AddPrimaryAction>(actions, failoverUnit);
                    }
                    else if (isPrimaryAvailable &&
                             ((replica.CurrentConfigurationRole == ReplicaRole::Secondary) ||
                              (!failoverUnit.IsChangingConfiguration && !replica.IsToBeDropped)))
                    {
                        replica.GenerateAction<AddReplicaAction>(actions, failoverUnit, replica);
                    }
                }
            }
            else
            {
                if (replica.IsToBeDropped)
                {
                    replica.GenerateAction<RemoveInstanceAction>(actions, replica);
                }
                else if (replica.IsDropped && !replica.IsDeleted)
                {
                    if (replica.IsNodeUp)
                    {
                        replica.GenerateAction<RemoveInstanceAction>(actions, replica);
                    }
                }
                else if (replica.IsInBuild)
                {
                    replica.GenerateAction<AddInstanceAction>(actions, replica);
                }
            }
        }
    }
}

void PendingTask::ProcessExtraStandByReplicas(LockedFailoverUnitPtr & failoverUnit)
{
    vector<ReplicaIterator> standByReplicas;
    for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; ++replica)
    {
        if (replica->PreviousConfigurationRole == ReplicaRole::None &&
            replica->CurrentConfigurationRole == ReplicaRole::Idle && 
            !replica->IsToBeDropped &&
            replica->IsStandBy &&
            replica->IsUp)
        {
            standByReplicas.push_back(replica);
        }
    }

    if (!standByReplicas.empty())
    {
        size_t idleStandByReplicaCount = standByReplicas.size();
        size_t maxStandByReplicaCount = static_cast<size_t>(ServiceModel::SystemServiceApplicationNameHelper::IsSystemServiceName(failoverUnit->ServiceName) ?
            ServiceModelConfig::GetConfig().SystemMaxStandByReplicaCount : ServiceModelConfig::GetConfig().UserMaxStandByReplicaCount);
        size_t count = idleStandByReplicaCount > maxStandByReplicaCount ? idleStandByReplicaCount - maxStandByReplicaCount : 0;

        if (count > 0)
        {
            sort(
                standByReplicas.begin(),
                standByReplicas.end(),
                [](ReplicaIterator r1, ReplicaIterator r2) { return r1->LastUpTime < r2->LastUpTime; });

            failoverUnit.EnableUpdate();

            for (size_t i = 0; i < count; ++i)
            {
                standByReplicas[i]->IsToBeDroppedByFM = true;
            }
        }
    }
}
