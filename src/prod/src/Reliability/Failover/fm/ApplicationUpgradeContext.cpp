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

ApplicationUpgradeContext::ApplicationUpgradeContext(
    ApplicationInfoSPtr const & application,
    wstring const & currentDomain)
    : UpgradeContext(wformatString("UpgradeContext.{0}.{1}", application->ApplicationId, application->Upgrade->Description.InstanceId), currentDomain),
      application_(application),
      servicesToDelete_(),
      isUpgradeNeededOnCurrentUD_(false)
{
}

BackgroundThreadContextUPtr ApplicationUpgradeContext::CreateNewContext() const
{
    return make_unique<ApplicationUpgradeContext>(application_, currentDomain_);
}

void ApplicationUpgradeContext::Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
{
    ServicePackageVersionInstance versionInstance;
    if (!failoverUnit.IsOrphaned && !application_->GetUpgradeVersionForServiceType(failoverUnit.ServiceInfoObj->ServiceType->Type, versionInstance))
    {
        if (application_->Upgrade->Description.Specification.IsServiceTypeBeingDeleted(failoverUnit.ServiceInfoObj->ServiceType->Type))
        {
            ProcessDeletedService(failoverUnit);
        }

        return;
    }

    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; it++)
    {
        if (it->NodeInfoObj->ActualUpgradeDomainId == currentDomain_)
        {
            if (it->IsNodeUp)
            {
                if (!it->IsUp &&
                    it->LastDownTime > application_->Upgrade->CurrentDomainStartedTime &&
                    DateTime::Now() - it->LastDownTime < FailoverConfig::GetConfig().ExpectedReplicaUpgradeDuration)
                {
                    notReadyNodes_.insert(it->FederationNodeId);
                }
            }

            if (!it->IsDropped)
            {
                isUpgradeNeededOnCurrentUD_ = true;
            }

            bool isPendingApplicationUpgrade = it->NodeInfoObj->IsPendingApplicationUpgrade(application_->ApplicationId);

            if (it->IsNodeUp &&
                (isPendingApplicationUpgrade ||
                 !application_->Upgrade->IsUpgradeCompletedOnNode(*(it->NodeInfoObj)) ||
                 (it->IsUp && !it->IsCreating && it->VersionInstance != versionInstance && !failoverUnit.IsOrphaned)))
            {
                //we are waiting for PLB to finish the check for resources available
                if (!application_->Upgrade->IsPLBSafetyCheckDone)
                {
                    auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, UpgradeSafetyCheckKind::WaitForResourceAvailability, failoverUnit.Id.Guid);
                    AddPendingNode(it->FederationNodeId, upgradeProgress);
                }
                else
                {
                    if (isPendingApplicationUpgrade || it->VersionInstance.Version == versionInstance.Version)
                    {
                        auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                        AddReadyNode(it->FederationNodeId, upgradeProgress, it->NodeInfoObj);
                    }
                    else if (application_->Rollback)
                    {
                        ServicePackageVersionInstance rollbackVersionInstance;
                        application_->GetRollbackVersionForServiceType(failoverUnit.ServiceInfoObj->ServiceType->Type, rollbackVersionInstance);

                        if (it->VersionInstance != rollbackVersionInstance)
                        {
                            auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                            AddReadyNode(it->FederationNodeId, upgradeProgress, it->NodeInfoObj);
                        }
                        else
                        {
                            ProcessReplica(failoverUnit, *it);
                        }
                    }
                    else
                    {
                        ProcessReplica(failoverUnit, *it);
                    }
                }
            }
            else if (IsReplicaWaitNeeded(*it))
            {
                auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::PostUpgradeSafetyCheck, UpgradeSafetyCheckKind::WaitForPrimaryPlacement, failoverUnit.Id.Guid);
                if (AddWaitingNode(it->FederationNodeId, upgradeProgress))
                {
                    failoverUnit.TraceState();
                }
            }
        }
        else if (it->NodeInfoObj->IsPendingApplicationUpgrade(application_->ApplicationId) &&
                 !application_->Upgrade->CheckUpgradeDomain(it->NodeInfoObj->ActualUpgradeDomainId))
        {
            AddCancelNode(it->FederationNodeId);
        }
    }

    ProcessCandidates(fm, *application_, failoverUnit);
}

void ApplicationUpgradeContext::ProcessDeletedService(FailoverUnit const & failoverUnit)
{
    servicesToDelete_[failoverUnit.ServiceName] = failoverUnit.ServiceInfoObj->Instance;
            
    for (auto replica = failoverUnit.BeginIterator; replica != failoverUnit.EndIterator; replica++)
    {
        if (replica->NodeInfoObj->ActualUpgradeDomainId == currentDomain_)
        {
            isUpgradeNeededOnCurrentUD_ = true;

            if (replica->IsNodeUp)
            {
                // Add the node with deleted service to the ready list.
                // This will ensure that the node will get upgraded, and
                // any replica created on this node will be in the new version.
                // Note: We can also add the node to the pending list. The
                // drawback is that the upgrade will not proceeed if the delete
                // of the service gets stuck.
                auto upgradeProgress = NodeUpgradeProgress(replica->FederationNodeId, replica->NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                AddReadyNode(replica->FederationNodeId, upgradeProgress, replica->NodeInfoObj);
            }
        }
    }
}

void ApplicationUpgradeContext::Merge(BackgroundThreadContext const& context)
{
    UpgradeContext::Merge(context);

    ApplicationUpgradeContext const& other = dynamic_cast<ApplicationUpgradeContext const&>(context);

    for (auto const& service : other.servicesToDelete_)
    {
        servicesToDelete_[service.first] = service.second;
    }

    isUpgradeNeededOnCurrentUD_ |= other.isUpgradeNeededOnCurrentUD_;
}

void ApplicationUpgradeContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    fm.Events.AppUpgradeInProgress(
        wformatString(application_->ApplicationId),
        currentDomain_,
        isContextCompleted,
        UnprocessedFailoverUnitCount,
        readyNodes_.size(),
        pendingNodes_.size(),
        waitingNodes_.size(),
        cancelNodes_.size(),
        notReadyNodes_.size(),
        application_->Upgrade->IsPLBSafetyCheckDone);

    PruneReadyNodes();

    if (!isContextCompleted && UnprocessedFailoverUnitCount == 0u)
    {
        TraceNodes(fm, FailoverManager::TraceApplicationUpgrade, wformatString(application_->ApplicationId));
    }
    
    if (!isEnumerationAborted && UnprocessedFailoverUnitCount == 0u)
    {
        fm.ServiceCacheObj.UpdateUpgradeProgressAsync(
            *application_,
            move(pendingNodes_),
            move(readyNodes_),
            move(waitingNodes_),
            isUpgradeNeededOnCurrentUD_,
            isContextCompleted);
    }

    for (auto const& service : servicesToDelete_)
    {
        fm.ServiceCacheObj.MarkServiceAsToBeDeletedAsync(service.first, service.second);
    }

    // Send CancelApplicationUpgradeRequest messages(s) to cancel any previous upgrade
    for (NodeId nodeId : cancelNodes_)
    {
        NodeInfoSPtr nodeInfo = fm.NodeCacheObj.GetNode(nodeId);

        if (nodeInfo && nodeInfo->IsUp && application_->Upgrade)
        {
            Transport::MessageUPtr message = RSMessage::GetCancelApplicationUpgradeRequest().CreateMessage(application_->Upgrade->Description);

            GenerationHeader header(fm.Generation, false);
            message->Headers.Add(header);

            fm.SendToNodeAsync(move(message), nodeInfo->NodeInstance);
        }
    }
}

bool ApplicationUpgradeContext::IsReplicaSetCheckNeeded() const
{
    return application_->Upgrade->IsReplicaSetCheckNeeded();
}

bool ApplicationUpgradeContext::IsReplicaWaitNeeded(Replica const& replica) const
{
    if (replica.FailoverUnitObj.IsToBeDeleted ||
        !IsReplicaSetCheckNeeded())
    {
        return false;
    }

    if (DateTime::Now() - application_->Upgrade->CurrentDomainStartedTime < FailoverConfig::GetConfig().ExpectedReplicaUpgradeDuration)
    {
        return replica.IsPreferredPrimaryLocation;
    }

    return false;
}

bool ApplicationUpgradeContext::IsReplicaMoveNeeded(Replica const& replica) const
{
    bool isCodePackageBeingUpgraded = true;

    wstring codePackageName;
    if (replica.FailoverUnitObj.ServiceInfoObj->ServiceType->TryGetCodePackageName(codePackageName))
    {
        isCodePackageBeingUpgraded = application_->Upgrade->Description.Specification.IsCodePackageBeingUpgraded(
            replica.FailoverUnitObj.ServiceInfoObj->ServiceType->Type,
            codePackageName);
    }

    if (isCodePackageBeingUpgraded &&
        DateTime::Now() - application_->Upgrade->CurrentDomainStartedTime < FailoverConfig::GetConfig().SwapPrimaryRequestTimeout)
    {
        return replica.FailoverUnitObj.IsReplicaMoveNeededDuringUpgrade(replica);
    }

    return false;
}
