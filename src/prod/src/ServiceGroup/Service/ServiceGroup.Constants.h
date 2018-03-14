// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// Constants.
//
#define MEMBER_SERVICE_NAME_DELIMITER std::wstring(L"#")
#define SERVICE_ADDRESS_DELIMITER std::wstring(L"%")
#define SERVICE_ADDRESS_DOUBLE_DELIMITER std::wstring(L"%%")
#define SERVICE_ADDRESS_ESCAPED_DELIMITER std::wstring(L"(%)")
#define COPY_STREAM_NAME L"copy stream"
#define REPLICATION_STREAM_NAME L"replication stream"
#define HR_STOP_OPERATION_PUMP S_FALSE

//
// Defines.
//
#define SERVICE_OBJECT_TYPE(x) \
    (FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL == x) ? L"Replica" : L"Instance" \
    
#define SERVICE_TYPE(x) \
    (FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL == x) ? L"Stateful" : L"Stateless" \

#define SERVICE_REPLICA_DOWN "Service replica must go down"
#define UNKNOWN_SERVICE_UNIT_REPLICA_CLOSED TraceUnknownMemberNameReplicaClosed

#define ROLE_TEXT(x) \
    (FABRIC_REPLICA_ROLE_NONE == x)             ? TraceReplicaRoleNone    : ( \
    (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == x)   ? TraceReplicaRoleIdle    : ( \
    (FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == x) ? TraceReplicaRoleActive  : ( \
    (FABRIC_REPLICA_ROLE_PRIMARY == x)          ? TraceReplicaRolePrimary : ( \
                                                  TraceReplicaRoleUnknown   ))))\

//
// Tracing.
//
Common::StringLiteral const TraceSubComponentCompositeServiceEndpoint("CompositeServiceEndpoint");
Common::StringLiteral const TraceSubComponentEmptyOperationDataAsyncEnumerator("EmptyOperationDataAsyncEnumerator");
Common::StringLiteral const TraceSubComponentOperationDataAsyncEnumeratorWrapper("OperationDataAsyncEnumeratorWrapper");
Common::StringLiteral const TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator("AtomicGroupStateOperationDataAsyncEnumerator");
Common::StringLiteral const TraceSubComponentCopyOperationDataAsyncEnumerator("CopyOperationDataAsyncEnumerator");
Common::StringLiteral const TraceSubComponentCopyOperationCallback("CopyOperationCallback");
Common::StringLiteral const TraceSubComponentReplicationAtomicGroupOperationCallback("ReplicationAtomicGroupOperationCallback");
Common::StringLiteral const TraceSubComponentCopyContextOperationData("CopyContextOperationData");
Common::StringLiteral const TraceSubComponentAtomicGroupOperationData("AtomicGroupOperationData");
Common::StringLiteral const TraceSubComponentCopyContextStreamDispatcher("CopyContextStreamDispatcher");
Common::StringLiteral const TraceSubComponentAtomicGroupStateOperationData("AtomicGroupStateOperationData");
Common::StringLiteral const TraceSubComponentOperationContext("AsyncOperations.OperationContext");
    Common::StringLiteral const TraceSubComponentGetOperationAsyncContext("AsyncOperations.GetOperationAsyncContext");
    Common::StringLiteral const TraceSubComponentGetOperationDataAsyncContext("AsyncOperations.GetOperationDataAsyncContext");
    Common::StringLiteral const TraceSubComponentCompositeAsyncOperation("AsyncOperations.CompositeAsyncOperation");
        Common::StringLiteral const TraceSubComponentCompositeAtomicGroupCommitRollbackOperation("AsyncOperations.CompositeAtomicGroupCommitRollbackOperation");
    Common::StringLiteral const TraceSubComponentCompositeAsyncOperationAsync("AsyncOperations.CompositeAsyncOperationAsync");
        Common::StringLiteral const TraceSubComponentStatelessCompositeOpenOperationAsync("AsyncOperations.StatelessCompositeOpenOperationAsync");
        Common::StringLiteral const TraceSubComponentStatefulCompositeChangeRoleOperationAsync("AsyncOperations.StatefulCompositeChangeRoleOperationAsync");
        Common::StringLiteral const TraceSubComponentStatefulCompositeOnDataLossOperationAsync("AsyncOperations.StatefulCompositeOnDataLossOperationAsync");
        Common::StringLiteral const TraceSubComponentStatefulCompositeOpenUndoCloseOperationAsync("AsyncOperations.StatefulCompositeOpenUndoCloseOperationAsync");
        Common::StringLiteral const TraceSubComponentStatefulCompositeDataLossUndoOperationAsync("AsyncOperations.StatefulCompositeDataLossUndoOperationAsync");
        Common::StringLiteral const TraceSubComponentStatelessCompositeOpenCloseOperationAsync("AsyncOperations.StatelessCompositeOpenCloseOperationAsync");
        Common::StringLiteral const TraceSubComponentCompositeAtomicGroupCommitRollbackOperationAsync("AsyncOperations.CompositeAtomicGroupCommitRollbackOperationAsync");
        Common::StringLiteral const TraceSubComponentStatefulCompositeUndoProgressOperation("AsyncOperations.StatefulCompositeUndoProgressOperation");
        Common::StringLiteral const TraceSubComponentStatefulCompositeRollbackUpdateEpochOperation("UpdateEpochOperation");
        Common::StringLiteral const TraceSubComponentStatefulCompositeRollbackCloseOperation("CloseOperation");
        Common::StringLiteral const TraceSubComponentStatefulCompositeRollbackChangeRoleOperation("ChangeRoleOperation");
        Common::StringLiteral const TraceSubComponentStatefulCompositeRollbackWithPostProcessingOperation("AsyncOperations.StatefulRollbackWithPostProcessingOperation");
    Common::StringLiteral const TraceSubComponentSingleStatefulOperationAsync("AsyncOperations.StatefulOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatefulOpenOperationAsync("AsyncOperations.StatefulOpenOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatefulCloseOperationAsync("AsyncOperations.StatefulCloseOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatefulChangeRoleOperationAsync("AsyncOperations.StatefulChangeRoleOperationAsync");
    Common::StringLiteral const TraceSubComponentSingleStatelessOperationAsync("AsyncOperations.StatelessOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatelessOpenOperationAsync("AsyncOperations.StatelessOpenOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatelessCloseOperationAsync("AsyncOperations.StatelessCloseOperationAsync");
    Common::StringLiteral const TraceSubComponentSingleStateProviderOperationAsync("AsyncOperations.StateProviderOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleStatefulOnDataLossOperationAsync("AsyncOperations.StatefulOnDataLossOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleAtomicGroupUndoProgressOperationAsync("AsyncOperations.AtomicGroupUndoProgressOperation");
        Common::StringLiteral const TraceSubComponentSingleAtomicGroupCommitRollbackOperationAsync("AsyncOperations.AtomicGroupCommitRollbackOperationAsync");
        Common::StringLiteral const TraceSubComponentSingleUpdateEpochOperationAsync("AsyncOperations.UpdateEpochOperationAsync");
    Common::StringLiteral const TraceSubComponentSingleAtomicGroupCommitRollbackOperation("AsyncOperations.AtomicGroupCommitRollbackOperation");
    Common::StringLiteral const TraceSubComponentAtomicGroupAsyncOperationContext("AsyncOperations.AtomicGroupAsyncOperationContext");
Common::StringLiteral const TraceSubComponentStatefulServiceGroupFactory("StatefulServiceGroupFactory");
Common::StringLiteral const TraceSubComponentStatefulServiceGroup("StatefulServiceGroup");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupOpen("StatefulServiceGroupOpen");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupClose("StatefulServiceGroupClose");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupAbort("StatefulServiceGroupAbort");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupChangeRole("StatefulServiceGroupChangeRole");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupOnDataLoss("StatefulServiceGroupOnDataLoss");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupCopyContext("StatefulServiceGroupCopyContext");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupCopyState("StatefulServiceGroupCopyState");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupReplicationProcessing("StatefulServiceGroupReplicationProcessing");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupAtomicGroupReplicationProcessing("StatefulServiceGroupAtomicGroupReplicationProcessing");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupCopyProcessing("StatefulServiceGroupCopyProcessing");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupOperationStreams("StatefulServiceGroupOperationStreams");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupUpdateEpoch("StatefulServiceGroupUpdateEpoch");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupPackage("StatefulServiceGroupPackage");
    Common::StringLiteral const TraceSubComponentStatefulServiceGroupActivation("StatefulServiceGroupActivation");
Common::StringLiteral const TraceSubComponentStatelessServiceGroupFactory("StatelessServiceGroupFactory");
Common::StringLiteral const TraceSubComponentStatelessServiceGroup("StatelessServiceGroup");
    Common::StringLiteral const TraceSubComponentStatelessServiceGroupOpen("StatelessServiceGroupOpen");
    Common::StringLiteral const TraceSubComponentStatelessServiceGroupClose("StatelessServiceGroupClose");
    Common::StringLiteral const TraceSubComponentStatelessServiceGroupAbort("StatelessServiceGroupAbort");
Common::StringLiteral const TraceSubComponentStatefulServiceGroupMember("StatefulServiceGroupMember");
Common::StringLiteral const TraceSubComponentStatelessServiceGroupMember("StatelessServiceGroupMember");
Common::StringLiteral const TraceSubComponentAtomicGroupCommitRollbackOperation("AtomicGroupCommitRollbackOperation");
Common::StringLiteral const TraceSubComponentUpdateEpochOperation("UpdateEpochOperation");
Common::StringLiteral const TraceSubComponentOperationDataFactory("OperationDataFactory");
Common::StringLiteral const TraceSubComponentStatefulServiceMemberReplicationProcessing("StatefulServiceMemberReplicationProcessing");
Common::StringLiteral const TraceSubComponentStatefulServiceMemberCopyProcessing("StatefulServiceMemberCopyProcessing");
Common::StringLiteral const TraceSubComponentStatefulServiceMemberCopyContextProcessing("StatefulServiceMemberCopyContextProcessing");
Common::StringLiteral const TraceSubComponentServiceGroupFactoryBuilder("ServiceGroupFactoryBuilder");
Common::StringLiteral const TraceSubComponentOutgoingOperationData("OutgoingOperationData");
Common::StringLiteral const TraceSubComponentIncomingOperation("IncomingOperation");
Common::StringLiteral const TraceSubComponentServiceGroupCommon("Common");
Common::StringLiteral const TraceSubComponentStatefulReportFault("StatefulReportFault");
Common::StringLiteral const TraceSubComponentStatelessReportFault("StatelessReportFault");
Common::StringLiteral const TraceSubComponentStatefulReportMoveCost("StatefulReportMoveCost");
Common::StringLiteral const TraceSubComponentStatelessReportMoveCost("StatelessReportMoveCost");
Common::StringLiteral const TraceSubComponentStatelessReportPartitionHealth("StatelessReportPartitionHealth");
Common::StringLiteral const TraceSubComponentStatelessReportInstanceHealth("StatelessReportInstanceHealth");
Common::StringLiteral const TraceSubComponentStatefulReportPartitionHealth("StatefulReportPartitionHealth");
Common::StringLiteral const TraceSubComponentStatefulReportReplicaHealth("StatefulReportReplicaHealth");

//
// Task names
//
std::wstring const TraceCopyProcessingTask(L"CopyProcessing");
std::wstring const TraceReplicationProcessingTask(L"ReplicationProcessing");

// Role names
std::wstring const TraceReplicaRoleUnknown(L"U");
std::wstring const TraceReplicaRoleNone(L"N");
std::wstring const TraceReplicaRoleIdle(L"I");
std::wstring const TraceReplicaRoleActive(L"A");
std::wstring const TraceReplicaRolePrimary(L"P");

//
// AtomicGroupStatus names
//
std::wstring const TraceAtomicGroupStatusInFlight(L"IF");
std::wstring const TraceAtomicGroupStatusRollingBack(L"RB");
std::wstring const TraceAtomicGroupStatusCommitting(L"C");
std::wstring const TraceAtomicGroupStatusInvalid(L"Invalid");

//
// Operation Names
//
std::wstring const TraceOperationNameUpdateEpoch(L"update epoch");
std::wstring const TraceOperationNameClose(L"close");
std::wstring const TraceOperationNameChangeRole(L"change role");

//
// Member names
//
std::wstring const TraceUnknownMemberNameReplicaClosed(L"unknown (replica closed)");
