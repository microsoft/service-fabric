// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace ReconfigurationAgentComponent::Communication;

namespace
{
    static bool GetCreateReplicaFlagForAction(std::wstring const & action)
    {
        return  action == RSMessage::GetAddInstance().Action ||
            action == RSMessage::GetAddPrimary().Action ||
            action == RSMessage::GetCreateReplica().Action;
    }

    static bool GetProcessDuringCloseForAction(std::wstring const & action)
    {
        return action == RSMessage::GetRemoveInstance().Action ||
            action == RSMessage::GetDeleteReplica().Action ||
            action == RSMessage::GetReplicaUpReply().Action;
    }

    static bool GetProcessDuringOpenForAction(std::wstring const & action)
    {
        return action == RSMessage::GetRemoveInstance().Action ||
            action == RSMessage::GetDeleteReplica().Action ||
            action == RSMessage::GetReplicaUpReply().Action;
    }
}

EntityMap<FailoverUnit> & EntityTraits<FailoverUnit>::GetEntityMap(ReconfigurationAgent & ra)
{
    return ra.LocalFailoverUnitMapObj;
}

void EntityTraits<FailoverUnit>::AddEntityIdToMessage(
    Transport::Message & message,
    Reliability::FailoverUnitId const & ftId)
{
    Transport::UnreliableTransport::AddPartitionIdToMessageProperty(message, ftId.Guid);
}

typename EntityTraits<FailoverUnit>::HandlerParametersType EntityTraits<FailoverUnit>::CreateHandlerParameters(
    std::string const & traceId,
    LockedEntityPtr<FailoverUnit> & ft,
    ReconfigurationAgent & ra,
    StateMachineActionQueue & actionQueue,
    MultipleEntityWork const * work,
    std::wstring const & activityId)
{
    return HandlerParameters(traceId, ft, ra, actionQueue, work, activityId);
}

typename EntityTraits<FailoverUnit>::EntityExecutionContextType EntityTraits<FailoverUnit>::CreateEntityExecutionContext(
    ReconfigurationAgent & ra,
    StateMachineActionQueue & queue,
    UpdateContext & updateContext,
    IEntityStateBase const * state)
{
    return FailoverUnitEntityExecutionContext(
        ra.Config,
        ra.Clock,
        ra.HostingAdapterObj,
        queue,
        ra.NodeInstance,
        updateContext,
        state);
}

bool EntityTraits<FailoverUnit>::PerformChecks(JobItemCheck::Enum check, FailoverUnit const * ft)
{
    if (check & JobItemCheck::FTIsNotNull)
    {
        if (!ft)
        {
            return false;
        }
    }

    if (check & JobItemCheck::FTIsOpen)
    {
        if (ft->IsClosed)
        {
            return false;
        }
    }

    return true;
}

void EntityTraits<FailoverUnit>::Trace(
    IdType const & id,
    Federation::NodeInstance const & nodeInstance,
    DataType const & data,
    std::vector<EntityJobItemTraceInformation> const & traceInfo,
    Common::ErrorCode const & commitResult,
    Diagnostics::ScheduleEntityPerformanceData const & schedulePerfData,
    Diagnostics::ExecuteEntityJobItemListPerformanceData const & executePerfData,
    Diagnostics::CommitEntityPerformanceData const & commitPerfData)
{
    RAEventSource::Events->FTUpdate(id.Guid, nodeInstance, data, traceInfo, commitResult, schedulePerfData, executePerfData, commitPerfData);
}

void EntityTraits<FailoverUnit>::AssertInvariants(
    FailoverUnit const & ft,
    Infrastructure::EntityExecutionContext & context)
{
    UNREFERENCED_PARAMETER(context);
    ft.AssertInvariants();
}

EntityEntryBaseSPtr EntityTraits<FailoverUnit>::Create(
    IdType const & id,
    ReconfigurationAgent & ra)
{
    return make_shared<EntityEntry<FailoverUnit>>(id, ra.LfumStore);
}

EntityEntryBaseSPtr EntityTraits<FailoverUnit>::Create(
    IdType const & id,
    std::shared_ptr<DataType> && data,
    ReconfigurationAgent & ra)
{
    return make_shared<EntityEntry<FailoverUnit>>(id, ra.LfumStore, move(data));
}

bool EntityTraits<FailoverUnit>::CheckStaleness(
    StalenessCheckType::Enum type,
    std::wstring const & action,
    Reliability::FailoverUnitDescription const * incomingFTDesc,
    Reliability::ReplicaDescription const * incomingLocalReplica,
    FailoverUnit const * ft)
{
    switch (type)
    {
    case StalenessCheckType::FTFailover:
        return StalenessChecker::CanProcessTcpMessage(action, incomingFTDesc, incomingLocalReplica, ft);
    case StalenessCheckType::FTProxy:
        return StalenessChecker::CanProcessIpcMessage(action, incomingFTDesc, incomingLocalReplica, ft);
    default:
        Assert::CodingError("unknown staleness check type {0} for {1}", static_cast<int>(type), action);
    }
}

bool EntityTraits<FailoverUnit>::StalenessChecker::CanProcessTcpMessage(
    std::wstring const & action,
    Reliability::FailoverUnitDescription const * incomingFTDesc,
    Reliability::ReplicaDescription const * incomingLocalReplica,
    FailoverUnit const * failoverUnit)
{
    if (!failoverUnit)
    {
        // This is a FT that did not exist and the LFUM entry was created to process this message
        // The FT will be created by the message handler because that has access to all the required information
        return true;
    }

    bool createReplicaIntentFlag = GetCreateReplicaFlagForAction(action);
    bool processDuringClose = GetProcessDuringCloseForAction(action);
    bool processDuringOpen = GetProcessDuringOpenForAction(action);

    if (failoverUnit->LocalReplicaOpenPending.IsSet && !processDuringOpen)
    {
        return false;
    }

    if (failoverUnit->LocalReplicaClosePending.IsSet && !processDuringClose)
    {
        return false;
    }

    if (failoverUnit->IsClosed && failoverUnit->LocalReplicaDeleted && createReplicaIntentFlag)
    {
        // Short circuit processing here because the FT has been deleted on the node
        // This is equivalent to there being no FT present (iff the cleanup timer had already run)
        return true;
    }

    bool comparePrimaryEpoch = false;
    bool compareEpochEqualityOnly = false;
    bool compareReplicaInstance = true;
    bool compareCurrentEpoch = true;

    if (action == RSMessage::GetAddReplica().Action ||
        action == RSMessage::GetRemoveReplica().Action)
    {
        // Add/Remove replica can only be called for the same configuration
        compareEpochEqualityOnly = true;
    }

    if (action == RSMessage::GetCreateReplicaReply().Action)
    {
        comparePrimaryEpoch = true;
    }

    if(action == RSMessage::GetRemoveInstance().Action ||
       action == RSMessage::GetDeleteReplica().Action)
    {
         compareReplicaInstance = false;
         compareCurrentEpoch = false;
    }

    return !IsTcpMessageStale(
        action,
        incomingFTDesc, 
        incomingLocalReplica,
        failoverUnit,
        compareEpochEqualityOnly, 
        comparePrimaryEpoch,
        compareReplicaInstance,
        compareCurrentEpoch);
}

bool EntityTraits<FailoverUnit>::StalenessChecker::IsTcpMessageStale(
    std::wstring const & action,
    Reliability::FailoverUnitDescription const * incomingFTDesc,
    Reliability::ReplicaDescription const * incomingLocalReplica,
    FailoverUnit const * failoverUnit,
    bool compareEpochEqualityOnly,
    bool comparePrimaryEpoch,
    bool compareReplicaInstance,
    bool compareCurrentEpoch)
{
    Epoch incomingEpoch = incomingFTDesc->CurrentConfigurationEpoch;
    Epoch currentEpoch = failoverUnit->CurrentConfigurationEpoch;

    // For some messages, the versions should match
    if (compareEpochEqualityOnly)
    {
        if (currentEpoch != incomingEpoch)
        {
            return true;
        }

        return false;
    }

    if (comparePrimaryEpoch)
    {
        if (currentEpoch.ToPrimaryEpoch() > incomingEpoch.ToPrimaryEpoch())
        {
            return true;
        }
    }
    else if (compareCurrentEpoch && currentEpoch > incomingEpoch)
    {
        // Current version is greater, stale message
        return true;
    }

    if (incomingLocalReplica != nullptr && failoverUnit->IsOpen)
    {
        // Check staleness based on replica instance
        ReplicaDescription const & incomingReplica = *incomingLocalReplica;

        Replica const & replica = failoverUnit->GetReplica(incomingReplica.FederationNodeId);

        if (replica.IsInvalid || (compareReplicaInstance && (replica.InstanceId > incomingReplica.InstanceId)))
        {
            return true;
        }

        ASSERT_IF(
            (replica.ReplicaId == incomingReplica.ReplicaId && replica.InstanceId < incomingReplica.InstanceId) &&
            (action != RSMessage::GetCreateReplica().Action && action != RSMessage::GetAddPrimary().Action && action != RSMessage::GetGetLSNReply().Action),
            "RA received message:{0}\r\n\r\nwith incoming replica instance newer than LFUM:\r\n{1}",
            action, *failoverUnit);
    }

    return false;
}

bool EntityTraits<FailoverUnit>::StalenessChecker::CanProcessIpcMessage(
    std::wstring const & action,
    Reliability::FailoverUnitDescription const * incomingFTDesc,
    Reliability::ReplicaDescription const * incomingLocalReplica,
    FailoverUnit const * failoverUnit)
{
    if (!failoverUnit->IsExisting)
    {
        return false;
    }

    if (failoverUnit->IsClosed || !failoverUnit->LocalReplica.IsUp)
    {
        return false;
    }

    if (failoverUnit->LocalReplicaOpenPending.IsSet &&
        action != RAMessage::GetReplicaOpenReply().Action &&
        action != RAMessage::GetStatefulServiceReopenReply().Action &&
        action != RAMessage::GetRAPQueryReply().Action &&
        action != RAMessage::GetReportFault().Action)
    {
        return false;
    }

    if (failoverUnit->LocalReplicaClosePending.IsSet &&
        action != RAMessage::GetReplicaCloseReply().Action &&
        action != RAMessage::GetRAPQueryReply().Action &&
        action != RAMessage::GetReadWriteStatusRevokedNotification().Action)
    {
        return false;
    }

    /*
        ReportFault and ReportLoad are special. They should be processed regardless of epoch
        This is because RAP could have put reportfault on the wire while epoch changed on RA
        As long as instance id matches RF should be processed

        ReadWriteStatusRevokedNotification also has the same logic - instance is sufficient for staleness
    */
    bool skipEpochComparison = false;
    if (action == RAMessage::GetReportFault().Action ||
        action == RAMessage::GetReportLoad().Action ||
        action == RAMessage::GetReadWriteStatusRevokedNotification().Action)
    {
        skipEpochComparison = true;
    }

    /*
        ReplicatorBuildIdleReplicaReply is special
        RAP uses the same epoch in which the build was started as the reply
        The failover portion of this epoch may change while build is in progress

        Consider:
        1. RA has 0/e1 [N/P] [N/I IB r2] [N/I IB r3]
        2. Build completes 0/e1 [N/I RD r2] and fm performs e1/e2 [P/P] [I/S r2]
        3. Reconfiguration completes
        4. Build completes 0/e1 [N/I RD r3]
        5. RA has cc epoch e2 and it should consider the reply from RAP in e1 as long as other checks pass
    */
    bool comparePrimaryEpochOnly = false;
    if (action == RAMessage::GetReplicatorBuildIdleReplicaReply().Action)
    {
        comparePrimaryEpochOnly = true;
    }

    return !IsIpcMessageStale(
        action,
        incomingFTDesc,
        incomingLocalReplica,
        failoverUnit,
        skipEpochComparison,
        comparePrimaryEpochOnly);
}

bool EntityTraits<FailoverUnit>::StalenessChecker::IsIpcMessageStale(
    std::wstring const & action,
    Reliability::FailoverUnitDescription const * incomingFTDesc,
    Reliability::ReplicaDescription const * incomingLocalReplica,
    FailoverUnit const * failoverUnit,
    bool skipEpochComparison,
    bool comparePrimaryEpochOnly)
{
    if (!skipEpochComparison &&
        IsIpcMessageStaleBasedOnEpoch(
            failoverUnit->CurrentConfigurationEpoch,
            incomingFTDesc->CurrentConfigurationEpoch,
            comparePrimaryEpochOnly))
    {
        return true;
    }

    if (incomingLocalReplica != nullptr)
    {
        if (failoverUnit->State == FailoverUnitStates::Open)
        {
            // Check staleness based on replica instance
            ReplicaDescription const & incomingReplica = *incomingLocalReplica;

            Replica const & replica = failoverUnit->LocalReplica;

            if (replica.IsInvalid || replica.InstanceId > incomingReplica.InstanceId)
            {
                return true;
            }

            ASSERT_IF(
                replica.ReplicaId == incomingReplica.ReplicaId && replica.InstanceId < incomingReplica.InstanceId,
                "RA received message from RAP:{0}\r\nwith incoming replica instance newer:\r\n{1}",
                action, *failoverUnit);
        }
    }

    return false;
}

bool EntityTraits<FailoverUnit>::StalenessChecker::IsIpcMessageStaleBasedOnEpoch(
    Reliability::Epoch const & ftEpoch,
    Reliability::Epoch const & incomingEpoch,
    bool comparePrimaryEpochOnly)
{
    if (comparePrimaryEpochOnly)
    {
        return incomingEpoch.ToPrimaryEpoch() < ftEpoch.ToPrimaryEpoch();
    }
    else
    {
        return incomingEpoch < ftEpoch;
    }
}
