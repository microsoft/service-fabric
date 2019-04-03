// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StateUpdateTask::StateUpdateTask(FailoverManager const& fm, INodeCache const& nodeCache)
    : StateMachineTask(), fm_(fm), nodeCache_(nodeCache)
{
}

void StateUpdateTask::CheckFailoverUnit(
    LockedFailoverUnitPtr & lockedFailoverUnit,
    vector<StateMachineActionUPtr> &)
{
    StopwatchTime now = Stopwatch::Now();
    FailoverConfig const& config = FailoverConfig::GetConfig();
    FailoverUnit & failoverUnit = *lockedFailoverUnit;
    ApplicationInfoSPtr applicationInfo = failoverUnit.ServiceInfoObj->ServiceType->Application;

    if (lockedFailoverUnit->IsPersistencePending &&
        now.ToDateTime() - lockedFailoverUnit->LastUpdated > config.LazyPersistWaitDuration)
    {
        lockedFailoverUnit.EnableUpdate();
        lockedFailoverUnit->PersistenceState = PersistenceState::ToBeUpdated;
    }

    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
    {
        Replica & replica = *it;

        if (replica.FederationNodeInstance != replica.NodeInfoObj->NodeInstance)
        {
            if (replica.FederationNodeInstance < replica.NodeInfoObj->NodeInstance)
            {
                if (replica.NodeInfoObj->IsReplicaUploaded && !replica.IsDeleted)
                {
                    lockedFailoverUnit.EnableUpdate();
                    failoverUnit.OnReplicaDropped(replica);
                    replica.IsDeleted = true;
                }
                else if (replica.IsUp)
                {
                    lockedFailoverUnit.EnableUpdate();
                    failoverUnit.OnReplicaDown(replica, !failoverUnit.HasPersistedState);
                }
            }
        }
        else if (!replica.IsNodeUp && replica.IsUp)
        {
            lockedFailoverUnit.EnableUpdate();
            failoverUnit.OnReplicaDown(replica, !failoverUnit.HasPersistedState);
        }
        else if (replica.IsUp && replica.IsStandBy &&
            !replica.IsInConfiguration &&
            !replica.IsToBeDropped && !failoverUnit.IsToBeDeleted &&
            now.ToDateTime() - replica.LastUpTime > replica.FailoverUnitObj.ServiceInfoObj->ServiceDescription.StandByReplicaKeepDuration)
        {
            lockedFailoverUnit.EnableUpdate();
            replica.IsToBeDroppedByFM = true;
        }
        else if (replica.IsInBuild &&
            !replica.IsInConfiguration &&
            failoverUnit.ServiceInfoObj->ServiceType->IsServiceTypeDisabled(replica.FederationNodeId))
        {
            lockedFailoverUnit.EnableUpdate();
            replica.IsToBeDroppedByFM = true;
        }
        else if (replica.IsMoveInProgress &&
            failoverUnit.InBuildReplicaCount == 0u &&
            failoverUnit.AvailableReplicaCount <= failoverUnit.TargetReplicaSetSize)
        {
            lockedFailoverUnit.EnableUpdate();
            replica.IsMoveInProgress = false;
        }
        else if (replica.IsOffline && !replica.IsNodeUp && !replica.IsInConfiguration &&
            !replica.IsPendingRemove && !failoverUnit.IsToBeDeleted &&
            replica.GetUpdateTime() + config.OfflineReplicaKeepDuration < now)
        {
            lockedFailoverUnit.EnableUpdate();
            replica.PersistenceState = PersistenceState::ToBeDeleted;
        }

        if (replica.IsDropped &&
            !replica.IsInConfiguration &&
            !replica.IsPendingRemove &&
            !replica.IsPreferredPrimaryLocation &&
            !replica.IsPreferredReplicaLocation &&
            !replica.NodeInfoObj->IsPendingUpgradeOrDeactivateNode())
        {
            TimeSpan keepDuration = replica.IsDeleted ? config.DeletedReplicaKeepDuration : config.DroppedReplicaKeepDuration;

            if (replica.GetUpdateTime() + keepDuration < now)
            {
                lockedFailoverUnit.EnableUpdate();
                replica.PersistenceState = PersistenceState::ToBeDeleted;
            }
        }

        if (replica.IsToBeDropped &&
            replica.IsCurrentConfigurationPrimary &&
            !failoverUnit.IsToBeDeleted &&
            !failoverUnit.ToBePromotedReplicaExists)
        {
             // If the Primary is marked as ToBeDroppedByPLB and there is no replica that is marked as ToBePromoted,
             // we clear the ToBeDropped flags on the primary. This can happen if the ToBePromoted secondary
             // failed during a SwapPrimary or MovePrimary movement.
             lockedFailoverUnit.EnableUpdate();
             replica.IsToBeDroppedByFM = false;
             replica.IsToBeDroppedByPLB = false;
        }

        ServiceModel::ServicePackageVersionInstance versionInstance = failoverUnit.ServiceInfoObj->ServiceDescription.PackageVersionInstance;
        if (applicationInfo->GetUpgradeVersionForServiceType(failoverUnit.ServiceInfoObj->ServiceType->Type, versionInstance))
        {
            if (applicationInfo->Upgrade->IsUpgradeCompletedOnNode(*replica.NodeInfoObj))
            {
                if (replica.VersionInstance < versionInstance && replica.IsUp && !replica.IsCreating)
                {
                    lockedFailoverUnit.EnableUpdate();
                    replica.VersionInstance = versionInstance;
                }
            }
        }

        TryClearUpgradeFlags(applicationInfo, lockedFailoverUnit, replica);

        if (replica.IsUp &&
            replica.IsCurrentConfigurationPrimary &&
            !replica.IsPrimaryToBeSwappedOut &&
            !failoverUnit.ToBePromotedReplicaExists &&
            (replica.IsPreferredPrimaryLocation || !failoverUnit.PreferredPrimaryLocationExists) &&
            IsSwapPrimaryNeeded(applicationInfo, *lockedFailoverUnit, replica))
        {
            if (failoverUnit.TargetReplicaSetSize == 1)
            {
                if (FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgrade)
                {
                    lockedFailoverUnit.EnableUpdate();
                    replica.IsPrimaryToBeSwappedOut = true;
                }
            }
            else
            {
                lockedFailoverUnit.EnableUpdate();
                replica.IsPrimaryToBeSwappedOut = true;
            }

            if (FailoverConfig::GetConfig().RestoreReplicaLocationAfterUpgrade &&
                !replica.NodeInfoObj->DeactivationInfo.IsDeactivated)
            {
                lockedFailoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = true;
            }
        }

        if (replica.IsNodeUp &&
            !replica.IsOffline &&
            (replica.IsPreferredPrimaryLocation && !replica.IsPrimaryToBePlaced) ||
            (replica.IsPreferredReplicaLocation && !replica.IsReplicaToBePlaced))
        {
            bool isOkToPlacePrimary = true;

            if (fm_.FabricUpgradeManager.Upgrade &&
                fm_.FabricUpgradeManager.Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId) &&
                replica.NodeInfoObj->VersionInstance.Version != fm_.FabricUpgradeManager.Upgrade->Description.Version)
            {
                // Fabric upgrade is going on and the node has not been upgraded
                isOkToPlacePrimary = false;
            }
            
            if (applicationInfo->Upgrade &&
                applicationInfo->Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId) &&
                (!applicationInfo->Upgrade->IsUpgradeCompletedOnNode(*replica.NodeInfoObj) ||
                (!replica.IsDropped && replica.VersionInstance.Version != versionInstance.Version)))
            {
                // Application upgrade is doing on and the replica has not been upgraded
                isOkToPlacePrimary = false;
            }

            if (isOkToPlacePrimary)
            {
                TryPlacePrimary(lockedFailoverUnit, replica);
            }
        }

        if (replica.NodeInfoObj->DeactivationInfo.IsRemove)
        {
            if (replica.IsUp)
            {
                if (replica.IsCurrentConfigurationPrimary &&
                    failoverUnit.InBuildReplicaCount == 0 &&
                    failoverUnit.TargetReplicaSetSize > 1 &&
                    !failoverUnit.IsToBeDeleted)
                {
                    if (!failoverUnit.ToBePromotedReplicaExists)
                    {
                        lockedFailoverUnit.EnableUpdate();
                        replica.IsPrimaryToBeSwappedOut = true;
                    }
                }
                else
                {
                    // if the RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckComplete is set we dont want to mark these replicas as to be dropped
                    // in order to avoid dropping them while safety checks are ongoing
                    // RA will close them once the deactivation message is sent by FM
                    if (failoverUnit.IsStateful || !FailoverConfig::GetConfig().RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckComplete)
                    {
                        lockedFailoverUnit.EnableUpdate();
                        replica.IsToBeDroppedForNodeDeactivation = true;
                    }
                }
            }
        }
        else if (replica.IsToBeDroppedForNodeDeactivation)
        {
            lockedFailoverUnit.EnableUpdate();
            replica.IsToBeDroppedForNodeDeactivation = false;
        }

        if (failoverUnit.ServiceInfoObj->IsServiceUpdateNeeded &&
            failoverUnit.ServiceInfoObj->UpdatedNodes.find(replica.FederationNodeId) != failoverUnit.ServiceInfoObj->UpdatedNodes.end())
        {
            lockedFailoverUnit.EnableUpdate();
            replica.ServiceUpdateVersion = failoverUnit.ServiceInfoObj->ServiceDescription.UpdateVersion;
        }
    }

    if (failoverUnit.IsSwappingPrimary && !failoverUnit.PreviousConfiguration.Primary->IsAvailable)
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.IsSwappingPrimary = false;
    }

    if (failoverUnit.ServiceInfoObj->IsToBeDeleted &&
        !failoverUnit.IsToBeDeleted)
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.SetToBeDeleted();
    }
    else if (failoverUnit.ServiceInfoObj->RepartitionInfo &&
        failoverUnit.ServiceInfoObj->RepartitionInfo->RepartitionType == RepartitionType::Remove &&
        failoverUnit.ServiceInfoObj->RepartitionInfo->IsRemoved(failoverUnit.Id) &&
        !failoverUnit.IsToBeDeleted)
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.SetToBeDeleted();
    }

    bool isUpgrading = (applicationInfo->Upgrade != nullptr);
    if (failoverUnit.IsUpgrading != isUpgrading)
    {
        lockedFailoverUnit.EnableUpdate();
        failoverUnit.IsUpgrading = isUpgrading;
    }
}

bool StateUpdateTask::IsSwapPrimaryNeeded(ApplicationInfoSPtr const& applicationInfo, FailoverUnit const& failoverUnit, Replica const& replica) const
{
    if (replica.NodeInfoObj->DeactivationInfo.IsDeactivated)
    {
        return failoverUnit.IsReplicaMoveNeededDuringDeactivateNode(replica);
    }
    else if (fm_.FabricUpgradeManager.Upgrade &&
        fm_.FabricUpgradeManager.Upgrade->Description.UpgradeType != ServiceModel::UpgradeType::Rolling_NotificationOnly &&
        fm_.FabricUpgradeManager.Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId) &&
        replica.NodeInfoObj->VersionInstance.Version != fm_.FabricUpgradeManager.Upgrade->Description.Version)
    {
        return failoverUnit.IsReplicaMoveNeededDuringUpgrade(replica);
    }
    else
    {
        ServiceModel::ServicePackageVersionInstance versionInstance;
        if (applicationInfo->GetUpgradeVersionForServiceType(failoverUnit.ServiceInfoObj->ServiceType->Type, versionInstance))
        {
            if (applicationInfo->Upgrade &&
                applicationInfo->Upgrade->Description.UpgradeType != ServiceModel::UpgradeType::Rolling_NotificationOnly &&
                applicationInfo->Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId) &&
                replica.VersionInstance.Version != versionInstance.Version)
            {
                if (applicationInfo->Rollback)
                {
                    ServiceModel::ServicePackageVersionInstance rollbackVersionInstance;
                    applicationInfo->GetRollbackVersionForServiceType(failoverUnit.ServiceInfoObj->ServiceType->Type, rollbackVersionInstance);

                    if (replica.VersionInstance != rollbackVersionInstance)
                    {
                        return false;
                    }
                }

                bool isCodePackageBeingUpgraded = true;

                wstring codePackageName;
                if (applicationInfo->Upgrade->Description.UpgradeType != ServiceModel::UpgradeType::Rolling_ForceRestart &&
                    replica.FailoverUnitObj.ServiceInfoObj->ServiceType->TryGetCodePackageName(codePackageName))
                {
                    isCodePackageBeingUpgraded = applicationInfo->Upgrade->Description.Specification.IsCodePackageBeingUpgraded(
                        replica.FailoverUnitObj.ServiceInfoObj->ServiceType->Type,
                        codePackageName);
                }

                if (isCodePackageBeingUpgraded)
                {
                    return failoverUnit.IsReplicaMoveNeededDuringUpgrade(replica);
                }

                return false;
            }
        }
    }

    return false;
}

void StateUpdateTask::TryPlacePrimary(LockedFailoverUnitPtr & failoverUnit, Replica & replica) const
{
    if (replica.IsPreferredPrimaryLocation)
    {
        if (replica.IsCurrentConfigurationPrimary)
        {
            if (!replica.IsPrimaryToBeSwappedOut && !failoverUnit->ToBePromotedReplicaExists)
            {
                failoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = false;
            }
        }
        else
        {
            failoverUnit.EnableUpdate();
            replica.IsPrimaryToBePlaced = true;
        }
    }
    else if (replica.IsDropped)
    {
        failoverUnit.EnableUpdate();
        replica.IsReplicaToBePlaced = true;
    }
}

bool StateUpdateTask::IsUpgradingOrDeactivating(ApplicationInfoSPtr const& applicationInfo, Replica const& replica) const
{
    if (replica.NodeInfoObj->DeactivationInfo.IsDeactivated)
    {
        return true;
    }

    if (fm_.FabricUpgradeManager.Upgrade &&
        fm_.FabricUpgradeManager.Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
    {
        return true;
    }

    if (applicationInfo->Upgrade &&
        applicationInfo->Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
    {
        return true;
    }

    return false;
}

void StateUpdateTask::TryClearUpgradeFlags(ApplicationInfoSPtr const& applicationInfo, LockedFailoverUnitPtr & failoverUnit, Replica & replica) const
{
    if (replica.IsPreferredPrimaryLocation ||
        replica.IsPrimaryToBeSwappedOut ||
        replica.IsPrimaryToBePlaced ||
        replica.IsPreferredReplicaLocation ||
        replica.IsReplicaToBePlaced)
    {
        if (!replica.IsInConfiguration && failoverUnit->IsStable && failoverUnit->TargetReplicaSetSize > 1)
        {
            // Partition has TargetReplicaSetSize of available replicas. This can happen if additional replicas got
            // created while waiting for replicas to be placed at their preferred location.

            failoverUnit.EnableUpdate();
            replica.IsPreferredPrimaryLocation = false;
            replica.IsPrimaryToBeSwappedOut = false;
            replica.IsPrimaryToBePlaced = false;
            replica.IsPreferredReplicaLocation = false;
            replica.IsReplicaToBePlaced = false;
        }
        else if (IsUpgradingOrDeactivating(applicationInfo, replica))
        {
            // Do not clear any flags. Wait for the upgrade or node deactivation to complete for that replica.
        }
        else if (fm_.FabricUpgradeManager.Upgrade)
        {
            //
            // Fabric upgrade is going on
            //

            if (fm_.FabricUpgradeManager.Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
            {
                // Replica belongs to the UD that is currently undergoing upgrade
                // No-op
            }
            else if (fm_.FabricUpgradeManager.Upgrade->CheckUpgradeDomain(replica.NodeInfoObj->ActualUpgradeDomainId))
            {
                // Replica belongs to a UD which has completed upgrade

                failoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = false;
                replica.IsPrimaryToBeSwappedOut = false;
                replica.IsPrimaryToBePlaced = false;
                if (DateTime::Now() - replica.NodeInfoObj->LastUpdated > FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDuration)
                {
                    replica.IsPreferredReplicaLocation = false;
                    replica.IsReplicaToBePlaced = false;
                }
            }
            else
            {
                // Replica belongs to a UD for which upgrade has not started yet

                failoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = false;
                replica.IsPrimaryToBeSwappedOut = false;
                replica.IsPrimaryToBePlaced = false;
                replica.IsPreferredReplicaLocation = false;
                replica.IsReplicaToBePlaced = false;
            }
        }
        else if (applicationInfo->Upgrade)
        {
            //
            // Application upgrade is going on
            //

            if (applicationInfo->Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
            {
                // Replica belongs to the UD that is currently undergoing upgrade
                // No-op
            }
            else if (applicationInfo->Upgrade->CheckUpgradeDomain(replica.NodeInfoObj->ActualUpgradeDomainId))
            {
                // Replica belongs to a UD which has completed upgrade

                failoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = false;
                replica.IsPrimaryToBeSwappedOut = false;
                replica.IsPrimaryToBePlaced = false;
                if (!replica.IsNodeUp || // A node is not expected to go down during application upgrade
                    DateTime::Now() - replica.NodeInfoObj->LastUpdated > FailoverConfig::GetConfig().ExpectedReplicaUpgradeDuration)
                {
                    replica.IsPreferredReplicaLocation = false;
                    replica.IsReplicaToBePlaced = false;
                }
            }
            else
            {
                // Replica belongs to a UD for which upgrade has not started yet

                failoverUnit.EnableUpdate();
                replica.IsPreferredPrimaryLocation = false;
                replica.IsPrimaryToBeSwappedOut = false;
                replica.IsPrimaryToBePlaced = false;
                replica.IsPreferredReplicaLocation = false;
                replica.IsReplicaToBePlaced = false;
            }
        }
        else
        {
            //
            // No upgrade is going on
            //

            failoverUnit.EnableUpdate();
            replica.IsPreferredPrimaryLocation = false;
            replica.IsPrimaryToBeSwappedOut = false;
            replica.IsPrimaryToBePlaced = false;
            if (DateTime::Now() - replica.NodeInfoObj->LastUpdated > FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDuration)
            {
                replica.IsPreferredReplicaLocation = false;
                replica.IsReplicaToBePlaced = false;
            }
        }
    }
}
