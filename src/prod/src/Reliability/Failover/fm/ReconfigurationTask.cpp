// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ReconfigurationTask::ReconfigurationTask(FailoverManager * fm)
    : StateMachineTask(), fm_(fm)
{
}

void ReconfigurationTask::CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, vector<StateMachineActionUPtr> & actions)
{
    FailoverUnit & failoverUnit = *lockedFailoverUnit;

    if (failoverUnit.ReplicaCount == 0 || failoverUnit.IsCreatingPrimary || failoverUnit.IsToBeDeleted)
    {
        return;
    }

    Epoch currentEpoch = failoverUnit.CurrentConfigurationEpoch;

    // If there is a reconfiguration already going on, we only need
    // to react if some node is down.  Since PendingTask will already
    // be sending out DoReconfiguration message to the current primary
    // with the most up-to-date node information, we only need to
    // take care of the case where primary is down.
    // We don't need to separately consider the case where PC primary
    // is down because in such case the DoReconfigurationAction will
    // send to new primary automatically.
    if (failoverUnit.CurrentConfiguration.IsEmpty)
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.RecoverFromDataLoss();
    }
    else if (!failoverUnit.Primary->IsUp || failoverUnit.Primary->IsStandBy || 
        (!failoverUnit.Primary->IsAvailable && !failoverUnit.IsChangingConfiguration))
    {
        ReconfigPrimary(lockedFailoverUnit, *lockedFailoverUnit);
    }
    else if (!failoverUnit.IsChangingConfiguration && !failoverUnit.HasPendingRemoveIdleReplica())
    {
        if (failoverUnit.ToBePromotedSecondaryExists &&
            (failoverUnit.CurrentConfiguration.DownSecondaryCount == 0 ||
            (failoverUnit.CurrentConfiguration.ReplicaCount == static_cast<size_t>(failoverUnit.MinReplicaSetSize) && !failoverUnit.IsQuorumLost())) &&
            failoverUnit.CurrentConfiguration.StableReplicaCount > 0 &&
			failoverUnit.CurrentConfiguration.StandByReplicaCount == 0 &&
            failoverUnit.InBuildReplicaCount == 0 &&
            failoverUnit.IdleReadyReplicaCount == 0)
        {
            // Perform Swap Primary with a Secondary that is marked as ToBePromoted
            ReconfigSwapPrimary(lockedFailoverUnit);
        }
        else if (!failoverUnit.IsStable)
        {
            ReconfigSecondary(lockedFailoverUnit);
        }
    }

    bool buildSecondary;
    if (!failoverUnit.IsChangingConfiguration)
    {
        buildSecondary = failoverUnit.CurrentConfiguration.IsPrimaryAvailable;
    }
    else
    {
        buildSecondary = (failoverUnit.IsReconfigurationPrimaryActive() && failoverUnit.CurrentConfiguration.AvailableCount < failoverUnit.CurrentConfiguration.WriteQuorumSize);
    }

    if (buildSecondary)
    {
        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            if (it->IsInCurrentConfiguration && it->IsUp)
            {
                if (it->IsCreating || it->IsStandBy)
                {
                    lockedFailoverUnit.EnableUpdate();
                    it->ReplicaState = ReplicaStates::InBuild;
                    if (it->IsCreating && !failoverUnit.IsChangingConfiguration)
                    {
                        failoverUnit.StartReconfiguration(false);
                    }
                }
            }
        }
    }

    if (failoverUnit.CurrentConfigurationEpoch.DataLossVersion != currentEpoch.DataLossVersion)
    {
        actions.push_back(make_unique<TraceDataLossAction>(failoverUnit.Id, currentEpoch));

        if (failoverUnit.Id.Guid == Constants::FMServiceGuid)
        {
            failoverUnit.RemoveAllReplicas();

            fm_->FTEvents.FTUpdateDataLossRebuild(lockedFailoverUnit->Id.Guid, *lockedFailoverUnit);
        }
    }

    if (failoverUnit.IsQuorumLost())
    {
        if (lockedFailoverUnit.Old && !lockedFailoverUnit.Old->IsQuorumLost())
        {
            failoverUnit.SetQuorumLossTime(DateTime::Now().Ticks);
        }

        fm_->FTEvents.FTQuorumLoss(failoverUnit.Id.Guid, failoverUnit);

        if (fm_->BackgroundManagerObj.IsAdminTraceEnabled)
        {
            fm_->FTEvents.FTQuorumLossAdmin(failoverUnit.IdString, wformatString(failoverUnit));
        }

        // If a FailoverUnit has exceeded the QuorumLossWaitDuration, then we should
        // drop the Down replica(s) so that the FailoverUnit can come out of quorum loss
        // and make progress. Because this can cause potential data loss, we do not 
        // want to use this feature for the FMService (which comes out of quorum loss by full
        // rebuild if the FailoverUnit is in quorum loss for too long).
        TimeSpan quorumLossWaitDuration = (fm_->IsMaster ? FailoverConfig::GetConfig().FullRebuildWaitDuration : failoverUnit.ServiceInfoObj->ServiceDescription.QuorumLossWaitDuration);
        if (failoverUnit.LastQuorumLossDuration >= quorumLossWaitDuration)
        {
            lockedFailoverUnit.EnableUpdate();
            failoverUnit.DropOfflineReplicas();
        }
    }
    else if (lockedFailoverUnit.Old && lockedFailoverUnit.Old->IsQuorumLost())
    {
        failoverUnit.SetQuorumLossTime(lockedFailoverUnit.Old->LastQuorumLossDuration.Ticks);

        fm_->FTEvents.FTQuorumRestored(failoverUnit.Id.Guid, failoverUnit);
    }
}

int ReconfigurationTask::CompareForPrimary(FailoverUnit const& failoverUnit, Replica const & replica, Replica const & current)
{
    if (replica.IsAvailable != current.IsAvailable)
    {
        return replica.IsAvailable ? -1 : 1;
    }

    if (replica.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        return -1;
    }
    else if (current.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        return 1;
    }

    if (replica.IsInPreviousConfiguration != current.IsInPreviousConfiguration)
    {
        return replica.IsInPreviousConfiguration ? -1 : 1;
    }

    if (replica.IsCreating != current.IsCreating)
    {
        return replica.IsCreating ? 1 : -1;
    }

    if (fm_ != nullptr)
    {
        return fm_->PLB.CompareNodeForPromotion(failoverUnit.ServiceName, failoverUnit.Id.Guid, replica.FederationNodeId, current.FederationNodeId);
    }

    return 0;
}

void ReconfigurationTask::ReconfigPrimary(LockedFailoverUnitPtr & lockedFailoverUnit, FailoverUnit & failoverUnit)
{
    if (failoverUnit.IsSwappingPrimary)
    {
        ASSERT_IFNOT(
            failoverUnit.PreviousConfiguration.Primary->IsAvailable,
            "PC primary not available: {0}", failoverUnit);

        if (lockedFailoverUnit)
        {
            lockedFailoverUnit.EnableUpdate();
        }

        failoverUnit.UpdateEpochForConfigurationChange(true /*isPrimaryChange*/);
        failoverUnit.SwapPrimary(failoverUnit.ConvertIterator(failoverUnit.PreviousConfiguration.Primary));
        failoverUnit.IsSwappingPrimary = false;

        return;
    }

    size_t ccUpCount = 0;
    size_t ccRemainingCount = 0;
    for (ReplicaIterator it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; it++)
    {
        if (!it->IsDropped)
        {
            if (it->IsInCurrentConfiguration)
            {
                ccRemainingCount++;
                if (it->IsUp)
                {
                    ccUpCount++;
                }
            }
        }
    }

    if (!failoverUnit.IsChangingConfiguration)
    {
        if (ccUpCount < failoverUnit.CurrentConfiguration.ReadQuorumSize && ccUpCount < ccRemainingCount)
        {
            return;
        }

        if (ccRemainingCount == 0)
        {
            if (lockedFailoverUnit)
            {
                lockedFailoverUnit.EnableUpdate();
            }

            failoverUnit.ClearConfiguration();

            return;
        }
    }
    else
    {
        size_t pcUpCount = failoverUnit.PreviousConfiguration.UpCount;
        size_t pcRemainingCount = failoverUnit.PreviousConfiguration.ReplicaCount - failoverUnit.PreviousConfiguration.DroppedCount;

        if (pcUpCount < failoverUnit.PreviousConfiguration.ReadQuorumSize && pcUpCount < pcRemainingCount)
        {
            return;
        }

        if (ccUpCount < failoverUnit.CurrentConfiguration.ReadQuorumSize && ccUpCount < ccRemainingCount)
        {
            return;
        }

        if (ccRemainingCount == 0)
        {
            if (lockedFailoverUnit)
            {
                lockedFailoverUnit.EnableUpdate();
            }
            if (pcRemainingCount == 0)
            {
                failoverUnit.ClearConfiguration();
            }
            else
            {
                failoverUnit.ClearCurrentConfiguration();
            }

            return;
        }
    }

    if (lockedFailoverUnit)
    {
        lockedFailoverUnit.EnableUpdate();
    }

    ReplicaIterator newPrimary;
    if (failoverUnit.PreviousConfiguration.IsPrimaryAvailable)
    {
        newPrimary = failoverUnit.ConvertIterator(failoverUnit.PreviousConfiguration.Primary);
    }
    else
    {
        newPrimary = failoverUnit.EndIterator;

        vector<ReplicaIterator> newPrimaryList;

        for (ReplicaIterator it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; it++)
        {
            if (it->IsInCurrentConfiguration && it->IsUp)
            {
                if (newPrimary == failoverUnit.EndIterator)
                {
                    newPrimary = it;
                    newPrimaryList.push_back(it);
                }
                else
                {
                    int compareResult = CompareForPrimary(failoverUnit, *it, *newPrimary);

                    if (compareResult < 0)
                    {
                        newPrimary = it;
                        newPrimaryList.clear();
                        newPrimaryList.push_back(it);
                    }
                    else if (compareResult == 0)
                    {
                        newPrimaryList.push_back(it);
                    }
                }
            }
        }

        // newPrimary should be the first in newPrimaryList now
        ASSERT_IF(newPrimaryList.empty(), "No up replica found in CC: {0}", failoverUnit);

        int newPrimarySize = static_cast<int>(newPrimaryList.size());
        if (newPrimarySize > 1 && !FailoverConfig::GetConfig().IsTestMode)
        {
            int index = failoverUnit.GetRandomInteger(newPrimarySize);

            newPrimary = newPrimaryList[index];
        }
    }

    ASSERT_IF(newPrimary == failoverUnit.EndIterator, "No up replica found in CC: {0}", failoverUnit);

    if (newPrimary->IsStandBy)
    {
        newPrimary->ReplicaState = ReplicaStates::InBuild;
    }

    // If there is no in progress reconfiguration, no need
    // to keep the failed primary.
    // Otherwise it will be swapped to get Secondary Role
    // to keep the CC set unchanged.
    if (!failoverUnit.IsChangingConfiguration)
    {
        // Start a new reconfiguration.
        failoverUnit.StartReconfiguration(true /*isPrimaryChange*/);
        failoverUnit.SwapPrimary(newPrimary);

        // Update other down secondary.
        for (ReplicaIterator it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; it++)
        {
            if (it->IsInCurrentConfiguration && !it->IsUp && failoverUnit.CurrentConfiguration.ReplicaCount > static_cast<size_t>(failoverUnit.MinReplicaSetSize))
            {
                failoverUnit.RemoveFromCurrentConfiguration(it);
            }
        }
    }
    else
    {
        // Primary changed, update epoch but can't change PC
        // since we are already in reconfiguration.
        failoverUnit.UpdateEpochForConfigurationChange(true /*isPrimaryChange*/);
        failoverUnit.SwapPrimary(newPrimary);
    }
}

void ReconfigurationTask::ReconfigSecondary(LockedFailoverUnitPtr & lockedFailoverUnit)
{
    FailoverUnit & failoverUnit = *lockedFailoverUnit;

    size_t downSecondaryCount = 0;
    size_t readyIdleCount = 0;
    bool hasPendingRemoveSecondary = false;
    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
    {
        if (it->IsCurrentConfigurationSecondary && !it->IsUp)
        {
            downSecondaryCount++;
        }
        else if (!it->IsInCurrentConfiguration && it->IsAvailable && !it->IsToBeDropped)
        {
            readyIdleCount++;
        }

        hasPendingRemoveSecondary = hasPendingRemoveSecondary || (it->IsPendingRemove && it->IsInConfiguration);
    }

    if (downSecondaryCount > 0 &&
        (failoverUnit.CurrentConfiguration.ReplicaCount + readyIdleCount > static_cast<size_t>(failoverUnit.MinReplicaSetSize) ||
        hasPendingRemoveSecondary))
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.StartReconfiguration(false /*isPrimaryChange*/);

        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator && failoverUnit.CurrentConfiguration.ReplicaCount + readyIdleCount > static_cast<size_t>(failoverUnit.MinReplicaSetSize); ++it)
        {
            if (it->IsCurrentConfigurationSecondary && it->IsDropped)
            {
                failoverUnit.RemoveFromCurrentConfiguration(it);
            }
        }

        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator && failoverUnit.CurrentConfiguration.ReplicaCount + readyIdleCount > static_cast<size_t>(failoverUnit.MinReplicaSetSize); ++it)
        {
            if (it->IsCurrentConfigurationSecondary && !it->IsDropped && !it->IsUp)
            {
                failoverUnit.RemoveFromCurrentConfiguration(it);
            }
        }
    }

    if (readyIdleCount > 0 && failoverUnit.CurrentConfiguration.ReplicaCount + readyIdleCount >= static_cast<size_t>(failoverUnit.MinReplicaSetSize))
    {
        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            if (!it->IsInCurrentConfiguration && it->IsAvailable && !it->IsToBeDropped)
            {
                lockedFailoverUnit.EnableUpdate();
                if (failoverUnit.PreviousConfiguration.IsEmpty)
                {
                    failoverUnit.StartReconfiguration(false /*isPrimaryChange*/);
                }

                failoverUnit.UpdateReplicaCurrentConfigurationRole(it, ReplicaRole::Secondary);
            }
        }
    }

    size_t count = failoverUnit.CurrentConfiguration.ReplicaCount;
    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator && count > static_cast<size_t>(failoverUnit.TargetReplicaSetSize); ++it)
    {
        if (it->IsCurrentConfigurationSecondary && it->IsToBeDropped && it->IsReady)
        {
            lockedFailoverUnit.EnableUpdate();
            if (failoverUnit.PreviousConfiguration.IsEmpty)
            {
                failoverUnit.StartReconfiguration(false /*isPrimaryChange*/);
            }

            failoverUnit.RemoveFromCurrentConfiguration(it);

            count--;
        }
    }

    if (failoverUnit.PreviousConfiguration.IsEmpty)
    {
        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            if (it->IsToBeDroppedByPLB &&
                it->IsInCurrentConfiguration &&
                failoverUnit.PotentialReplicaCount == 0u &&
                failoverUnit.CurrentConfiguration.UpCount <= static_cast<size_t>(failoverUnit.TargetReplicaSetSize))
            {
                lockedFailoverUnit.EnableUpdate();
                it->IsToBeDroppedByPLB = false;
            }
        }
    }
}

void ReconfigurationTask::ReconfigSwapPrimary(LockedFailoverUnitPtr & lockedFailoverUnit)
{
    lockedFailoverUnit.EnableUpdate();

    FailoverUnit & failoverUnit = *lockedFailoverUnit;

    // Search for the Secondary that is marked as ToBePromoted.
    Replica * newPrimary = nullptr;
    for (auto replica = lockedFailoverUnit->BeginIterator; replica != lockedFailoverUnit->EndIterator; ++replica)
    {
        if (replica->IsCurrentConfigurationSecondary && replica->IsToBePromoted)
        {
            newPrimary = &(*replica);
            break;
        }
    }

    ASSERT_IF(newPrimary == nullptr, "A ToBePromoted secondary must exist: {0}", failoverUnit);
    ASSERT_IF(newPrimary->IsToBeDropped, "New Primary should be stable: {0}", failoverUnit);

    failoverUnit.StartReconfiguration(true /*isPrimaryChange*/);
    failoverUnit.IsSwappingPrimary = true;
    failoverUnit.SwapPrimary(failoverUnit.GetReplicaIterator(newPrimary->FederationNodeId));
}
