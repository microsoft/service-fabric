// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const BuildReplicaTag("BuildReplica");
StringLiteral const ReconfigurationReplyTag("ReconfigurationReply");

FailoverUnitMessageProcessor::FailoverUnitMessageProcessor(FailoverManager & fm)
    : fm_(fm)
{
}

void FailoverUnitMessageProcessor::AddInstanceReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const
{
    auto it = failoverUnit->GetReplicaIterator(body.ReplicaDescription.FederationNodeId);
    if (it == failoverUnit->EndIterator || !it->IsInBuild || !it->IsUp)
    {
        fm_.WriteInfo(
            "AddInstanceReplyProcessor",
            "Message {0} ignored for {1}",
            body, *failoverUnit);

        body.ErrorCode.ReadValue();
    }
    else
    {
        CompleteBuildReplica(failoverUnit, *it, body);

        if (!body.ErrorCode.IsSuccess())
        {
            failoverUnit.EnableUpdate();
            failoverUnit->OnReplicaDropped(*it);
        }
    }
}

void FailoverUnitMessageProcessor::AddPrimaryReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const
{
    if (failoverUnit->IsCreatingPrimary &&
        failoverUnit->Primary->IsUp &&
        failoverUnit->Primary->FederationNodeInstance == body.ReplicaDescription.FederationNodeInstance)
    {
        CompleteBuildReplica(failoverUnit, *(failoverUnit->Primary), body);

        if (!body.ErrorCode.IsSuccess())
        {
            failoverUnit.EnableUpdate();
            failoverUnit->OnReplicaDropped(*failoverUnit->Primary);
        }

        failoverUnit->CompleteReconfiguration();
    }
}

void FailoverUnitMessageProcessor::AddReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const
{
    auto it = failoverUnit->GetReplicaIterator(body.ReplicaDescription.FederationNodeId);
    if (it == failoverUnit->EndIterator || !it->IsInBuild || !it->IsUp || !failoverUnit->IsReconfigurationPrimaryAvailable())
    {
        fm_.WriteInfo(
            "AddReplicaReplyProcessor",
            "Message {0} ignored for {1}",
            body, *failoverUnit);

        body.ErrorCode.ReadValue();
    }
    else
    {
        CompleteBuildReplica(failoverUnit, *it, body);

        if (!body.ErrorCode.IsSuccess())
        {
            failoverUnit.EnableUpdate();
            it->IsToBeDroppedByFM = true;
            if (!it->IsInConfiguration && failoverUnit->IsReconfigurationPrimaryAvailable())
            {
                it->IsPendingRemove = true;
            }
        }
    }
}

void FailoverUnitMessageProcessor::CompleteBuildReplica(LockedFailoverUnitPtr & failoverUnit, Replica & replica, ReplicaReplyMessageBody const& body) const
{
    bool isNewReplica = (replica.VersionInstance == ServiceModel::ServicePackageVersionInstance::Invalid);

    if (body.ErrorCode.IsSuccess())
    {
        if (!replica.IsInBuild)
        {
            TRACE_AND_TESTASSERT(
                fm_.WriteError,
                "CompleteBuildReplica", failoverUnit->IdString,
                "Replica on {0} for {1} is not InBuild.", replica.FederationNodeId, *failoverUnit);

            return;
        }

        bool skipPersistence = !failoverUnit->IsStateful &&
            failoverUnit->InBuildReplicaCount > 1 &&
            FailoverConfig::GetConfig().LazyPersistWaitDuration > TimeSpan::Zero;

        failoverUnit.EnableUpdate(skipPersistence);

        fm_.Events.EndpointsUpdated(
            failoverUnit->Id.Guid,
            replica.FederationNodeInstance,
            replica.ReplicaId,
            replica.InstanceId,
            BuildReplicaTag,
            body.ReplicaDescription.ServiceLocation,
            body.ReplicaDescription.ReplicationEndpoint);

        replica.ServiceLocation = body.ReplicaDescription.ServiceLocation;
        replica.ReplicationEndpoint = body.ReplicaDescription.ReplicationEndpoint;
        if (isNewReplica ||
            body.ReplicaDescription.InstanceId > replica.InstanceId ||
            body.ReplicaDescription.PackageVersionInstance.InstanceId > replica.VersionInstance.InstanceId)
        {
            replica.VersionInstance = body.ReplicaDescription.PackageVersionInstance;
        }
        replica.ReplicaState = ReplicaStates::Ready;
        replica.IsPendingRemove = false;
        replica.IsPreferredReplicaLocation = false;
        replica.IsReplicaToBePlaced = false;

        if (isNewReplica)
        {
            fm_.FTEvents.BuildReplicaSuccess(
                failoverUnit->Id.Guid,
                replica.ReplicaId,
                replica.FederationNodeId);
        }
    }
    else
    {
        fm_.FTEvents.BuildReplicaFailure(
            failoverUnit->Id.Guid,
            replica.ReplicaId,
            replica.FederationNodeId,
            body.ErrorCode);

        if (body.ErrorCode.IsError(ErrorCodeValue::ApplicationInstanceDeleted))
        {
            auto applicationInfo = failoverUnit->ServiceInfoObj->ServiceType->Application;
            auto serviceInfo = failoverUnit->ServiceInfoObj;

            fm_.ServiceCacheObj.BeginDeleteApplication(
                applicationInfo->ApplicationId,
                applicationInfo->InstanceId,
                [this, serviceInfo](AsyncOperationSPtr const& operation)
                {
                    OnDeleteApplicationCompleted(operation, serviceInfo);
                },
                fm_.CreateAsyncOperationRoot());
        }
    }
}

void FailoverUnitMessageProcessor::OnDeleteApplicationCompleted(Common::AsyncOperationSPtr const& operation, ServiceInfoSPtr const& serviceInfo) const
{
    ErrorCode error = fm_.ServiceCacheObj.EndDeleteApplication(operation);
    if (!error.IsSuccess())
    {
        return;
    }

    fm_.ServiceCacheObj.BeginDeleteService(
        serviceInfo->Name,
        false, // IsForce
        serviceInfo->Instance,
        [this](AsyncOperationSPtr const& operation)
        {
            OnDeleteServiceCompleted(operation);
        },
        fm_.CreateAsyncOperationRoot());
}

void FailoverUnitMessageProcessor::OnDeleteServiceCompleted(Common::AsyncOperationSPtr const& operation) const
{
    ErrorCode error = fm_.ServiceCacheObj.EndDeleteService(operation);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FMServiceDeleteInProgress))
    {
        auto deleteServiceOperation = AsyncOperation::Get<ServiceCache::DeleteServiceAsyncOperation>(operation);

        fm_.WriteWarning(
            Constants::ServiceSource, deleteServiceOperation->ServiceName,
            "Delete of service {0} failed with {1}", deleteServiceOperation->ServiceName, error);
    }
}

void FailoverUnitMessageProcessor::RemoveReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const
{
    if (body.ErrorCode.IsSuccess())
    {
        ReplicaDescription const & replica = body.ReplicaDescription;
        auto it = failoverUnit->GetReplicaIterator(replica.FederationNodeId);
        if (it != failoverUnit->EndIterator && it->InstanceId == replica.InstanceId && it->IsPendingRemove)
        {
            failoverUnit.EnableUpdate();

            it->IsPendingRemove = false;
        }
    }
}

void FailoverUnitMessageProcessor::DropReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const
{
    if (body.ErrorCode.IsSuccess())
    {
        ProcessDownReplica(failoverUnit, body.ReplicaDescription, true, false);
    }
}

void FailoverUnitMessageProcessor::DeleteReplicaReplyProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    wstring const& action,
    unique_ptr<ReplicaReplyMessageBody> && body,
    NodeInstance const & from) const
{
    if (body->ErrorCode.IsSuccess())
    {
        if (failoverUnit)
        {
            ProcessDownReplica(failoverUnit, body->ReplicaDescription, true, true);
        }
        else
        {
            ErrorCode result = fm_.InBuildFailoverUnitCacheObj.OnReplicaDropped(body->FailoverUnitDescription.FailoverUnitId, from.Id, true);
            if (result.IsError(ErrorCodeValue::FMFailoverUnitNotFound))
            {
                if (fm_.FailoverUnitCacheObj.FailoverUnitExists(body->FailoverUnitDescription.FailoverUnitId))
                {
                    FailoverUnitMessageTask<ReplicaReplyMessageBody>::ProcessRequest(action, move(body), fm_, from);
                }
                else
                {
                    fm_.WriteInfo(
                        (action == RSMessage::GetDeleteReplicaReply().Action ? StringLiteral("DeleteReplicaProcessor") : StringLiteral("RemoveInstanceProcessor")),
                        "FailoverUnit {0} not found while processing {1} message.",
                        body->FailoverUnitDescription.FailoverUnitId, action);
                }
            }
        }
    }
}

void FailoverUnitMessageProcessor::RemoveInstanceReplyProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    wstring const& action,
    unique_ptr<ReplicaReplyMessageBody> && body,
    NodeInstance const & from) const
{
    DeleteReplicaReplyProcessor(failoverUnit, action, move(body), from);
}

void FailoverUnitMessageProcessor::ReplicaDroppedProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    wstring const& action,
    unique_ptr<ReplicaMessageBody> && body,
    NodeInstance const & from,
    vector<StateMachineActionUPtr> & actions) const
{
    ErrorCode result;

    if (failoverUnit)
    {
        ProcessDownReplica(failoverUnit, body->ReplicaDescription, true, false);
    }
    else
    {
        result = fm_.InBuildFailoverUnitCacheObj.OnReplicaDropped(body->FailoverUnitDescription.FailoverUnitId, from.Id, false);

        if (result.IsError(ErrorCodeValue::FMFailoverUnitNotFound))
        {
            if (fm_.FailoverUnitCacheObj.FailoverUnitExists(body->FailoverUnitDescription.FailoverUnitId))
            {
                FailoverUnitMessageTask<ReplicaMessageBody>::ProcessRequest(action, move(body), fm_, from);
                return;
            }
            else
            {
                fm_.WriteInfo(
                    "ReplicaDroppedMessageHandler",
                    "FailoverUnit {0} not found while processing ReplicaDropped message.",
                    body->FailoverUnitDescription.FailoverUnitId);
            }
        }
    }

    ReplicaReplyMessageBody replyBody(
        body->FailoverUnitDescription,
        body->ReplicaDescription,
        result);

    actions.push_back(make_unique<ReplicaDroppedReplyAction>(move(replyBody), from));
}

void FailoverUnitMessageProcessor::ReplicaEndpointUpdatedProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    unique_ptr<ReplicaMessageBody> && body,
    NodeInstance const & from,
    vector<StateMachineActionUPtr> & actions) const
{
    ErrorCode error(ErrorCodeValue::Success);

    if (failoverUnit)
    {
        if (failoverUnit->IsChangingConfiguration)
        {
            auto it = failoverUnit->GetReplicaIterator(body->ReplicaDescription.FederationNodeId);

            if (it != failoverUnit->EndIterator && it->InstanceId == body->ReplicaDescription.InstanceId)
            {
                failoverUnit.EnableUpdate();

                it->ServiceLocation = body->ReplicaDescription.ServiceLocation;
                it->ReplicationEndpoint = body->ReplicaDescription.ReplicationEndpoint;

                it->IsEndpointAvailable = true;

                fm_.Events.EndpointsUpdated(
                    failoverUnit->Id.Guid,
                    it->FederationNodeInstance,
                    it->ReplicaId,
                    it->InstanceId,
                    BuildReplicaTag,
                    it->ServiceLocation,
                    it->ReplicationEndpoint);
            }
            else
            {
                fm_.WriteInfo(
                    "ReplicaEndpointUpdatedMessageHandler",
                    "Replica not found while processing ReplicaEndpointUpdated message: {0}",
                    body->ReplicaDescription);
            }
        }
    }
    else
    {
        fm_.WriteInfo(
            "ReplicaEndpointUpdatedMessageHandler",
            "FailoverUnit {0} not found while processing ReplicaEndpointUpdated message.",
            body->FailoverUnitDescription.FailoverUnitId);
    }

    auto replyBody = ReplicaReplyMessageBody(
        body->FailoverUnitDescription,
        body->ReplicaDescription,
        error);

    actions.push_back(make_unique<ReplicaEndpointUpdatedReplyAction>(move(replyBody), from));
}

void FailoverUnitMessageProcessor::ReplicaDownProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    ReplicaDescription const & replica) const
{
    ProcessDownReplica(failoverUnit, replica, !failoverUnit->HasPersistedState, false);
}

void FailoverUnitMessageProcessor::ProcessDownReplica(
    LockedFailoverUnitPtr & failoverUnit,
    ReplicaDescription const & replica,
    bool isDropped,
    bool isDeleted) const
{
    auto it = failoverUnit->GetReplicaIterator(replica.FederationNodeId);
    if (it != failoverUnit->EndIterator && it->InstanceId <= replica.InstanceId)
    {
        failoverUnit.EnableUpdate();

        it->InstanceId = replica.InstanceId;
        
        if (it->ReplicaId > replica.ReplicaId)
        {
            TRACE_AND_TESTASSERT(
                fm_.WriteError,
                "ProcessDownReplica", failoverUnit->IdString,
                "Incoming replica id must be greater when the incoming instance is greater. Incoming {0}. Local {1}", replica, *failoverUnit);

            return;
        }

        it->ReplicaId = replica.ReplicaId;
        if (it->FederationNodeInstance.InstanceId < replica.FederationNodeInstance.InstanceId)
        {
            it->FederationNodeInstance = replica.FederationNodeInstance;
        }

        if (it->IsUp || (it->IsDropped != isDropped))
        {
            failoverUnit->OnReplicaDown(*it, isDropped);
        }

        if (isDeleted)
        {
            it->IsDeleted = true;
        }
    }
}

void FailoverUnitMessageProcessor::ReconfigurationReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ConfigurationReplyMessageBody const& body) const
{
    if (body.ErrorCode.IsSuccess())
    {
        if (failoverUnit->IsInReconfiguration)
        {
            failoverUnit.EnableUpdate();

            for (auto it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; ++it)
            {
                ReplicaDescription const* replica = body.GetReplica(it->FederationNodeId);

                if (replica == nullptr)
                {
                    if (it->CurrentConfigurationRole == ReplicaRole::Secondary)
                    {
                        failoverUnit->UpdateReplicaCurrentConfigurationRole(it, ReplicaRole::None);
                    }
                    else if (it->PreviousConfigurationRole == ReplicaRole::Secondary &&
                        it->CurrentConfigurationRole == ReplicaRole::None)
                    {
                        if (failoverUnit->HasPersistedState)
                        {
                            failoverUnit->UpdateReplicaCurrentConfigurationRole(it, ReplicaRole::Idle);
                            it->IsToBeDroppedByFM = true;
                        }
                        else
                        {
                            failoverUnit->UpdateReplicaCurrentConfigurationRole(it, ReplicaRole::None);
                        }
                    }

                    if (it->IsInConfiguration)
                    {
                        it->IsPendingRemove = false;
                    }
                }
                else
                {
                    if (replica->CurrentConfigurationRole != it->CurrentConfigurationRole)
                    {
                        failoverUnit->UpdateReplicaCurrentConfigurationRole(it, replica->CurrentConfigurationRole);
                    }

                    bool wasInBuild = it->IsInBuild;

                    if (replica->InstanceId == it->InstanceId && !it->IsDropped && replica->State != it->ReplicaState)
                    {
                        if (it->IsCreating)
                        {
                            it->VersionInstance = replica->PackageVersionInstance;
                        }

                        it->ReplicaState = replica->State;
                    }

                    if (it->IsPendingRemove &&
                        replica->InstanceId == it->InstanceId && replica->IsUp == it->IsUp && replica->State == it->ReplicaState)
                    {
                        it->IsPendingRemove = false;
                    }

                    bool isRoleChanged =
                        (it->CurrentConfigurationRole != it->PreviousConfigurationRole) ||
                        (it->IsCurrentConfigurationPrimary &&
                        (failoverUnit->PreviousConfigurationEpoch.ToPrimaryEpoch().ConfigurationVersion !=
                        failoverUnit->CurrentConfigurationEpoch.ToPrimaryEpoch().ConfigurationVersion));

                    if (replica->IsAvailable &&
                        (wasInBuild ||
                        isRoleChanged ||
                        replica->ServiceLocation != it->ServiceLocation ||
                        replica->ReplicationEndpoint != it->ReplicationEndpoint))
                    {
                        fm_.Events.EndpointsUpdated(
                            failoverUnit->Id.Guid,
                            replica->FederationNodeInstance,
                            replica->ReplicaId,
                            replica->InstanceId,
                            ReconfigurationReplyTag,
                            replica->ServiceLocation,
                            replica->ReplicationEndpoint);

                        it->ServiceLocation = move(replica->ServiceLocation);
                        it->ReplicationEndpoint = move(replica->ReplicationEndpoint);
                    }

                    if (it->IsCurrentConfigurationPrimary)
                    {
                        if (replica->CurrentConfigurationRole != ReplicaRole::Primary || !replica->IsAvailable)
                        {
                            TRACE_AND_TESTASSERT(
                                fm_.WriteError,
                                "ReconfigurationReplyProcessor", failoverUnit->IdString,
                                "Received inconsistent primary: {0} with: {1}",
                                body, *failoverUnit);

                            return;
                        }
                    }
                }
            }

            // Clear PC and also drop the replicas with None/Idle role in CC.
            failoverUnit->CompleteReconfiguration();
        }
        else
        {
            fm_.WriteInfo(
                "ReconfigurationReplyProcessor",
                "DoReconfigurationReply message received when the FailoverUnit was NOT going through a reconfiguration. MessageBody = '{0}'",
                body);
        }
    }
}

void FailoverUnitMessageProcessor::ChangeConfigurationProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    ConfigurationMessageBody const& body,
    NodeInstance const & from,
    vector<StateMachineActionUPtr> & actions) const
{
    if (!failoverUnit->IsInReconfiguration)
    {
        fm_.WriteInfo(
            "ChangeConfigurationProcessor",
            "ChangeConfiguration message received when the FailoverUnit was NOT going throug a reconfiguration. MessageBody = '{0}'.",
            body);

        return;
    }

    failoverUnit.EnableUpdate();

    ReplicaIterator newPrimary = failoverUnit->EndIterator;
    ReplicaIterator downHighestReplica = failoverUnit->EndIterator;
    ReplicaDescription const * newPrimaryDesc = nullptr;

    vector<ReplicaIterator> newPrimaryList;

    for (ReplicaIterator it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; it++)
    {
        if (it->IsInConfiguration)
        {
            // If replica not found in the message, it must have been dropped.
            ReplicaDescription const * desc = body.GetReplica(it->FederationNodeId);
            if (desc)
            {
                if (it->InstanceId >= desc->InstanceId)
                {
                    it->ReplicaDescription.LastAcknowledgedLSN = desc->LastAcknowledgedLSN;
                    it->ReplicaDescription.FirstAcknowledgedLSN = (it->IsUp ? desc->FirstAcknowledgedLSN : 0);
                }

                if (desc->IsUp)
                {
                    if ((desc->LastAcknowledgedLSN >= 0))
                    {
                        if (newPrimaryDesc == nullptr)
                        {
                            newPrimary = it;
                            newPrimaryDesc = desc;
                            newPrimaryList.push_back(it);
                        }
                        else
                        {
                            int compareResult = CompareForPrimary(*failoverUnit, *desc, *newPrimaryDesc);
                            if (compareResult < 0)
                            {
                                newPrimary = it;
                                newPrimaryDesc = desc;
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
                else
                {
                    if (desc->InstanceId == it->InstanceId)
                    {
                        failoverUnit->OnReplicaDown(*it, desc->IsDropped);
                    }

                    if ((it->ReplicaDescription.LastAcknowledgedLSN >= 0) &&
                        ((downHighestReplica == failoverUnit->EndIterator) ||
                        (it->ReplicaDescription.LastAcknowledgedLSN > downHighestReplica->ReplicaDescription.LastAcknowledgedLSN)))
                    {
                        downHighestReplica = it;
                    }
                }
            }
            else
            {
                failoverUnit->OnReplicaDown(*it, true);
            }
        }
    }

    int newPrimarySize = static_cast<int>(newPrimaryList.size());
    if (newPrimarySize > 1 && !FailoverConfig::GetConfig().IsTestMode)
    {
        int index = failoverUnit->GetRandomInteger(newPrimarySize);
        newPrimary = newPrimaryList[index];
    }

    if (newPrimary == failoverUnit->EndIterator && downHighestReplica == failoverUnit->EndIterator)
    {
        TRACE_AND_TESTASSERT(
            fm_.WriteError,
            "ChangeConfigurationProcessor", failoverUnit->IdString,
            "Unexpected ChangeConfiguration message {0} for FailoverUnit {1}",
            body, *failoverUnit);

        return;
    }

    if (downHighestReplica != failoverUnit->EndIterator &&
        (newPrimary == failoverUnit->EndIterator ||
        downHighestReplica->ReplicaDescription.LastAcknowledgedLSN > newPrimary->ReplicaDescription.LastAcknowledgedLSN))
    {
        failoverUnit->SwapPrimary(downHighestReplica);
        failoverUnit->UpdateEpochForConfigurationChange(true /*isPrimaryChange*/);
    }
    else
    {
        if (newPrimary->IsStandBy)
        {
            newPrimary->ReplicaState = ReplicaStates::InBuild;
        }

        failoverUnit->SwapPrimary(newPrimary);
        failoverUnit->UpdateEpochForConfigurationChange(true /*isPrimaryChange*/, false /* Keep LSN */);

        actions.push_back(make_unique<DoReconfigurationAction>(*failoverUnit));
    }

    if (!failoverUnit->PreviousConfiguration.Primary->IsCurrentConfigurationSecondary)
    {
        failoverUnit->IsSwappingPrimary = false;
    }

    actions.push_back(make_unique<ChangeConfigurationReplyAction>(*failoverUnit, from));
}

void FailoverUnitMessageProcessor::DatalossReportProcessor(
    LockedFailoverUnitPtr & failoverUnit,
    ConfigurationMessageBody const& body,
    vector<StateMachineActionUPtr> & actions) const
{
    if (!failoverUnit->IsInReconfiguration)
    {
        fm_.WriteInfo(
            "DatalossReportProcessor",
            "DatalossReport message received when the FailoverUnit was NOT going throug a reconfiguration. MessageBody = '{0}'.",
            body);

        return;
    }

    failoverUnit.EnableUpdate();

    actions.push_back(make_unique<TraceDataLossAction>(failoverUnit->Id, failoverUnit->CurrentConfigurationEpoch));

    failoverUnit->UpdateEpochForDataLoss();

    if (failoverUnit->Id.Guid == Constants::FMServiceGuid)
    {
        failoverUnit->RemoveAllReplicas();

        fm_.FTEvents.FTUpdateDataLossRebuild(failoverUnit->Id.Guid, *failoverUnit);
    }
}

int FailoverUnitMessageProcessor::CompareForPrimary(FailoverUnit const& failoverUnit, ReplicaDescription const &r1, ReplicaDescription const& r2) const
{
    if (r1.LastAcknowledgedLSN != r2.LastAcknowledgedLSN)
    {
        return r1.LastAcknowledgedLSN > r2.LastAcknowledgedLSN ? -1 : 1;
    }

    bool isInCC1 = (r1.CurrentConfigurationRole == ReplicaRole::Primary || r1.CurrentConfigurationRole == ReplicaRole::Secondary);
    bool isInCC2 = (r2.CurrentConfigurationRole == ReplicaRole::Primary || r2.CurrentConfigurationRole == ReplicaRole::Secondary);

    if (isInCC1 && !isInCC2)
    {
        return -1;
    }
    else if (!isInCC1 && isInCC2)
    {
        return 1;
    }
    else if (r1.FirstAcknowledgedLSN != r2.FirstAcknowledgedLSN)
    {
        if (r1.FirstAcknowledgedLSN == 0 || r2.FirstAcknowledgedLSN == 0)
        {
            if (r1.FirstAcknowledgedLSN != r2.FirstAcknowledgedLSN)
            {
                return r1.FirstAcknowledgedLSN > r2.FirstAcknowledgedLSN ? -1 : 1;
            }
        }
        else
        {
            if (r1.FirstAcknowledgedLSN != r2.FirstAcknowledgedLSN)
            {
                return r1.FirstAcknowledgedLSN < r2.FirstAcknowledgedLSN ? -1 : 1;
            }
        }
    }

    return fm_.PLB.CompareNodeForPromotion(failoverUnit.ServiceName, failoverUnit.Id.Guid, r1.FederationNodeId, r2.FederationNodeId);
}
