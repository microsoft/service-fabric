// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

// List of common action sets
Global<ProxyActionsList> ProxyActionsList::Empty = 
                                    make_global<ProxyActionsList>();

// Stateless services
Global<ProxyActionsList> ProxyActionsList::StatelessServiceOpen = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                                ProxyActions::OpenPartition,
                                                ProxyActions::OpenInstance});

Global<ProxyActionsList> ProxyActionsList::StatelessServiceClose = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::CloseInstance,
                                            ProxyActions::ClosePartition});

Global<ProxyActionsList> ProxyActionsList::StatelessServiceAbort = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::AbortInstance,
                                            ProxyActions::ClosePartition});
// Stateful services
Global<ProxyActionsList> ProxyActionsList::StatefulServiceOpenIdle =
                                        make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::OpenPartition,
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::OpenReplica,
                                            ProxyActions::OpenReplicator,
                                            ProxyActions::ChangeReplicatorRole,
                                            ProxyActions::ChangeReplicaRole});

// Stateful services
Global<ProxyActionsList> ProxyActionsList::StatefulServiceOpenPrimary = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::OpenPartition,
                                            ProxyActions::ReconfigurationStarting,
                                            ProxyActions::OpenReplica, 
                                            ProxyActions::OpenReplicator, 
                                            ProxyActions::ChangeReplicatorRole, 
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::ReportAnyDataLoss,
                                            ProxyActions::ReconfigurationEnding});

// Replica must be closed (guaranteed by RA) prior to reopen being sent
Global<ProxyActionsList> ProxyActionsList::StatefulServiceReopen = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::OpenPartition,
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::OpenReplica, 
                                            ProxyActions::OpenReplicator});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceAbort = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::OpenPartition,
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::OpenReplica,
                                            ProxyActions::OpenReplicator,
                                            ProxyActions::ChangeReplicatorRole,
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::AbortReplicator,
                                            ProxyActions::AbortReplica,
                                            ProxyActions::ClosePartition});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceClose = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::CloseReplicator,
                                            ProxyActions::CloseReplica,
                                            ProxyActions::ClosePartition});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceDrop = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::OpenPartition,
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::OpenReplica,
                                            ProxyActions::OpenReplicator,
                                            ProxyActions::ChangeReplicatorRole,
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::CloseReplicator,
                                            ProxyActions::CloseReplica,
                                            ProxyActions::ClosePartition});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceChangeRole = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ChangeReplicatorRole,
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::UpdateReadWriteStatus});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceFinalizeDemoteToSecondary =
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                        ProxyActions::ChangeReplicatorRole,
                                        ProxyActions::ChangeReplicaRole,
                                        ProxyActions::UpdateReadWriteStatus,
                                        ProxyActions::FinalizeDemoteToSecondary });

Global<ProxyActionsList> ProxyActionsList::StatefulServicePromoteToPrimary = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ReconfigurationStarting,
                                            ProxyActions::ChangeReplicatorRole, 
                                            
                                            /*
                                                The update epoch is important here
                                                Consider abort swap case where the catchup gets cancelled
                                                In this case, failover has generated a new primary change epoch 
                                                and the promote to primary action list runs

                                                Since the replicator never saw CR(S) it will not get a chance to see
                                                the new update epoch via the CR API

                                                Thus the primary needs an explicit update epoch call to tell it the new epoch for operations
                                            */
                                            ProxyActions::UpdateEpoch,
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::ReportAnyDataLoss,
                                            ProxyActions::ReplicatorUpdateCatchUpConfiguration,
                                            ProxyActions::CatchupReplicaSetQuorum,
                                            ProxyActions::UpdateReadWriteStatus});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceDemoteToSecondary = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ReconfigurationStarting, 
                                            ProxyActions::ReplicatorPreWriteStatusRevokeUpdateConfiguration,
                                            ProxyActions::PreWriteStatusRevokeCatchup,
                                            ProxyActions::UpdateEpoch,                   
                                            ProxyActions::ReplicatorUpdateCatchUpConfiguration, 
                                            ProxyActions::CatchupReplicaSetAll, 
                                            ProxyActions::UpdateReadWriteStatus,
                                            ProxyActions::ChangeReplicatorRole,
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::ReconfigurationEnding,
                                            ProxyActions::GetReplicatorStatus});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceFinishDemoteToSecondary = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ChangeReplicaRole,
                                            ProxyActions::ReconfigurationEnding,
                                            ProxyActions::GetReplicatorStatus});

// Replicator only
Global<ProxyActionsList> ProxyActionsList::ReplicatorBuildIdleReplica = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::BuildIdleReplica});

Global<ProxyActionsList> ProxyActionsList::ReplicatorRemoveIdleReplica = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::RemoveIdleReplica});

Global<ProxyActionsList> ProxyActionsList::ReplicatorGetStatus = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::GetReplicatorStatus});

Global<ProxyActionsList> ProxyActionsList::ReplicatorUpdateEpochAndGetStatus = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::UpdateEpoch,
                                            ProxyActions::GetReplicatorStatus});

Global<ProxyActionsList> ProxyActionsList::ReplicatorUpdateReplicas = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ReplicatorUpdateConfiguration});

Global<ProxyActionsList> ProxyActionsList::ReplicatorUpdateAndCatchupQuorum = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ReconfigurationStarting,
                                            ProxyActions::UpdateEpoch,
                                            ProxyActions::ReplicatorUpdateCatchUpConfiguration, 
                                            ProxyActions::CatchupReplicaSetQuorum});

Global<ProxyActionsList> ProxyActionsList::CancelCatchupReplicaSet = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::CancelCatchupReplicaSet});

Global<ProxyActionsList> ProxyActionsList::StatefulServiceEndReconfiguration = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::ReplicatorUpdateCurrentConfiguration,
                                            ProxyActions::ReconfigurationEnding});

Global<ProxyActionsList> ProxyActionsList::ReplicatorGetQuery = 
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::GetReplicatorQuery});

Global<ProxyActionsList> ProxyActionsList::UpdateServiceDescription =
                                    make_global<ProxyActionsList>(ProxyActionsList{
                                            ProxyActions::UpdateServiceDescription,
                                            ProxyActions::UpdateReadWriteStatus});

Global<ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable> ProxyActionsList::CompatibilityTable = 
                                    make_global<ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable>();

bool ProxyActionsList::AreAcceptableActionsListsForParallelExecution(std::vector<ActionListInfo> const & runningActions, ProxyActionsListTypes::Enum const toRun)
{
    bool retVal = true;

    if(runningActions.size() == 0)
    {
        return true;
    }
    
    std::for_each(runningActions.cbegin(), 
                  runningActions.cend(),
                  [&retVal, toRun](ActionListInfo const& runningAction) 
                { 
                    retVal = retVal && ProxyActionsList::CompatibilityTable->CanExecuteConcurrently(runningAction.Name, toRun);
                }
    );

    return retVal;
}

bool ProxyActionsList::AreStandaloneActionsLists(ProxyActionsListTypes::Enum const actionList)
{
    return actionList == ProxyActionsListTypes::StatefulServiceAbort ||
           actionList == ProxyActionsListTypes::StatefulServiceClose ||
           actionList == ProxyActionsListTypes::StatefulServiceDrop ||
           actionList == ProxyActionsListTypes::StatefulServiceReopen ||
           actionList == ProxyActionsListTypes::StatelessServiceClose || 
           actionList == ProxyActionsListTypes::StatelessServiceAbort;
}

bool ProxyActionsList::IsAbort(ProxyActionsListTypes::Enum const actionList)
{
    return actionList == ProxyActionsListTypes::StatefulServiceAbort ||
           actionList == ProxyActionsListTypes::StatelessServiceAbort;
}

bool ProxyActionsList::IsClose(ProxyActionsListTypes::Enum const actionList)
{
    return actionList == ProxyActionsListTypes::StatefulServiceClose ||
           actionList == ProxyActionsListTypes::StatefulServiceDrop ||
           actionList == ProxyActionsListTypes::StatelessServiceClose;
}

ProxyActionsList const & ProxyActionsList::GetActionsListByType(ProxyActionsListTypes::Enum actionListName)
{
    switch(actionListName)
    {
        case ProxyActionsListTypes::Empty:
            return GetEmpty();

        case ProxyActionsListTypes::StatelessServiceOpen:
            return GetStatelessServiceOpen();
        case ProxyActionsListTypes::StatelessServiceClose:
            return GetStatelessServiceClose();
        case ProxyActionsListTypes::StatelessServiceAbort:
            return GetStatelessServiceAbort();

        case ProxyActionsListTypes::StatefulServiceReopen:
            return GetStatefulServiceReopen();
        case ProxyActionsListTypes::StatefulServiceOpenIdle:
            return GetStatefulServiceOpenIdle();
        case ProxyActionsListTypes::StatefulServiceOpenPrimary:
            return GetStatefulServiceOpenPrimary();
        case ProxyActionsListTypes::StatefulServiceClose:
            return GetStatefulServiceClose();
        case ProxyActionsListTypes::StatefulServiceDrop:
            return GetStatefulServiceDrop();
        case ProxyActionsListTypes::StatefulServiceAbort:
            return GetStatefulServiceAbort();
        case ProxyActionsListTypes::StatefulServiceChangeRole:
            return GetStatefulServiceChangeRole();
        case ProxyActionsListTypes::StatefulServicePromoteToPrimary:
            return GetStatefulServicePromoteToPrimary();
        case ProxyActionsListTypes::StatefulServiceDemoteToSecondary:
            return GetStatefulServiceDemoteToSecondary();
        case ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary:
            return GetStatefulServiceFinishDemoteToSecondary();
        case ProxyActionsListTypes::StatefulServiceEndReconfiguration:
            return GetStatefulServiceEndReconfiguration();
        case ProxyActionsListTypes::ReplicatorBuildIdleReplica:
            return GetReplicatorBuildIdleReplica();
        case ProxyActionsListTypes::ReplicatorRemoveIdleReplica:
            return GetReplicatorRemoveIdleReplica();
        case ProxyActionsListTypes::ReplicatorGetStatus:
            return GetReplicatorGetStatus();
        case ProxyActionsListTypes::ReplicatorUpdateEpochAndGetStatus:
            return GetReplicatorUpdateEpochAndGetStatus();
        case ProxyActionsListTypes::ReplicatorUpdateReplicas:
            return GetReplicatorUpdateReplicas();
        case ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum:
            return GetReplicatorUpdateAndCatchupQuorum();
        case ProxyActionsListTypes::CancelCatchupReplicaSet:
            return GetCancelCatchupReplicaSet();
        case ProxyActionsListTypes::ReplicatorGetQuery:
            return GetReplicatorGetQuery();
        case ProxyActionsListTypes::UpdateServiceDescription:
            return GetUpdateServiceDescription();
        case ProxyActionsListTypes::StatefulServiceFinalizeDemoteToSecondary:
            return GetStatefulServiceFinalizeDemoteToSecondary();
        default: 
            Common::Assert::CodingError("Unknown ProxyActionsListTypes value");
    }
}

void ProxyActionsList::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    UNREFERENCED_PARAMETER(options);

    for (size_t i = 0; i < actions_.size(); i++)
    {
        w << i << L":" << actions_[i] << L" ";
    }
}
