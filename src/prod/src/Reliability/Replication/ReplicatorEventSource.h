// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        #define DECLARE_RE_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
        #define RE_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::Replication, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        class ReplicatorEventSource
        {
        public:
            DECLARE_RE_STRUCTURED_TRACE(ReplicationSessionTrace, uint16, ReplicationEndpointId, std::wstring, ReliableOperationSender, std::wstring, uint64, bool, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryTraceInfo, uint16, FABRIC_SEQUENCE_NUMBER, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryTraceBatchInfo, uint16, FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER, std::wstring);

            DECLARE_RE_STRUCTURED_TRACE(StateProviderUpdateEpoch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(StateProviderUpdateEpochCompleted, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(StateProviderProgress, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(StateProviderError, Common::Guid, std::wstring, std::wstring, int);

            DECLARE_RE_STRUCTURED_TRACE(ReplicatorConfigUpdate, uintptr_t, std::wstring, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactory, Common::Guid, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryShareTransport, Common::Guid, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryConfigError, Common::Guid, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryReplQueueSizeConfigError, Common::Guid, FABRIC_REPLICA_ID, int, int, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryCopyQueueSizeConfigError, Common::Guid, FABRIC_REPLICA_ID, int, int);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactorySecuritySettingsError, Common::Guid, FABRIC_REPLICA_ID, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryMaxMessageSizeConfigError, Common::Guid, FABRIC_REPLICA_ID, int, int, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorFactoryGeneralConfigError, Common::Guid, FABRIC_REPLICA_ID, std::wstring);

            // Replicator traces
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorOpen, Common::Guid, ReplicationEndpointId, bool);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorInvalidState, Common::Guid, Replicator, std::wstring, int);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorDropMsgNoFromHeader, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorDropMsgInvalid, Common::Guid, Replicator, std::wstring, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorDataLoss, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorDataLossStateChanged, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorChangeRole, Common::Guid, ReplicationEndpointId, int, int, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorChangeRoleInvalid, Common::Guid, ReplicationEndpointId, int, int);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorCtor, Common::Guid, ReplicationEndpointId, REInternalSettings);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorDtor, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorInvalidMessage, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(ReplicatorReportFault, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring, int);

            // Primary traces
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCtor, Common::Guid, ReplicationEndpointId, bool, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCatchUpAll, Common::Guid, ReplicationEndpointId, bool, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCatchUpQuorum, Common::Guid, ReplicationEndpointId, bool, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCatchUpQuorumWithMustCatchupReplicas, Common::Guid, ReplicationEndpointId, bool, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAckReceive, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring, /*Common::Guid,*/ LONGLONG, std::wstring); // We can leave out the guid since we have it already as the first param (10 arg limit in tuple)
            DECLARE_RE_STRUCTURED_TRACE(PrimarySendW, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, ReliableOperationSender, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(PrimarySendBatchW, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, ReliableOperationSender, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(PrimarySend, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, std::wstring, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(PrimarySendInfo, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, std::wstring, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(PrimarySendLast, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, std::wstring, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCCMissing, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryGetInfo, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplicateCancel, Common::Guid, ReplicationEndpointId, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplicateDone, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAbort, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAbortDone, Common::Guid, ReplicationEndpointId, int);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryPreCloseWait, Common::Guid, ReplicationEndpointId, Common::TimeSpan);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryPreCloseWaitDone, Common::Guid, ReplicationEndpointId, Common::ErrorCode);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryPendingReplicateOperationsCompleted, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryClose, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCloseDone, Common::Guid, ReplicationEndpointId, int);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryRemoveReplica, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAddIdleError, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryPromoteIdle, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryUpdateReplQueue, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryMissingReplicas, Common::Guid, ReplicationEndpointId, uint64, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplicate, Common::Guid, ReplicationEndpointId, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplicateBatch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAddSessionInitialRepl, Common::Guid, ReplicationEndpointId, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryUpdateEpoch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryUpdateCatchUpConfig, Common::Guid, ReplicationEndpointId, uint64, uint64, uint64, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryUpdateCurrentConfig, Common::Guid, ReplicationEndpointId, uint64, uint64);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplMgrClosed, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryDtor, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryStartCopy, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAllCCReceived, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReceiveCC, Common::Guid, ReplicationEndpointId, LONGLONG, std::wstring, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReceiveCCNotPersisted, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCCEnqueueWithoutDispatch, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryQueueFull, Common::Guid, ReplicationEndpointId, ReplicaManager, FABRIC_SEQUENCE_NUMBER);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryDropMsgDueToIncarnationId, Common::Guid, ReplicationEndpointId, std::wstring, Common::Guid, Common::Guid);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCopySessionEstablished, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, Common::Guid, int);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryIdleDoesNotExist, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryConfiguration, Common::Guid, ReplicationEndpointId, ReplicaManager, FABRIC_SEQUENCE_NUMBER, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryMissingOperations, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCatchUpError, Common::Guid, ReplicationEndpointId, int);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryAckProcess, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, LONGLONG, LONGLONG, bool, LONGLONG, LONGLONG, uint64, uint64, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryReplicateFailed, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCleanupQueueDueToSlowIdle, Common::Guid, ReplicationEndpointId, int64, uint, uint, bool);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryMustCatchupUpdated, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, MustCatchup::Trace, MustCatchup::Trace, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryDemoteOperationQueueCount, Common::Guid, ReplicationEndpointId, ULONGLONG, int64, FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryDemoteOperationQueueMemory, Common::Guid, ReplicationEndpointId, ULONGLONG, int64, FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryFaultedSlowSecondary, Common::Guid, ReplicationEndpointId, ReplicationSession, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(PrimaryCleanupQueueDueToSlowSecondary, Common::Guid, ReplicationEndpointId, int64, uint, uint, bool);
            
            // ReliableOperationSender traces
            DECLARE_RE_STRUCTURED_TRACE(OpSenderOpen, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderAddOps, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderAddOp, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderAddBatchOp, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderRemoveOps, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderRemoveOpsEmpty, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderRemoveOpsNothingRemoved, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderNotActive, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderSend, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderTimer, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, bool);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderAck, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(OpSenderClose, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG);
            
            // Secondary Replicator traces
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCtor, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, Common::Guid);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCtorInitial, Common::Guid, ReplicationEndpointId, std::wstring, Common::Guid);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryGetInfo, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryNotActive, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDtor, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryReceive, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryReceiveBatch, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryEnqueueWithoutDispatch, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryAllCopyReceived, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryClose, Common::Guid, ReplicationEndpointId, bool, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDrainDispatchQueue, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring, ULONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDrainDispatchQueueDone, Common::Guid, ReplicationEndpointId, std::wstring, int, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryGetLastOp, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryUpdateEpochError, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryUpdateEpoch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDispatch, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryWaitForAcks, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryWaitForAcksDone, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryAllCopyAcked, Common::Guid, ReplicationEndpointId, bool, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondarySendCC, Common::Guid, ReplicationEndpointId, LONGLONG, bool, Common::Guid, uint32);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDropMsgDueToReplicaId, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDropMsgDueToEpoch, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDropMsgPrimaryAddress, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring, ReplicationEndpointId, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDropMsgDueToNoStartCopyEpoch, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryTempQueueError, Common::Guid, ReplicationEndpointId, LONGLONG, int);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryTempQueueDiscard, Common::Guid, ReplicationEndpointId, uint64);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCopyOperationCompletedInOtherThread, Common::Guid, ReplicationEndpointId, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryDropMsgDueToIncarnationId, Common::Guid, ReplicationEndpointId, std::wstring, Common::Guid, Common::Guid);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryReplDispatchCallback, Common::Guid, ReplicationEndpointId, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCloseDispatchQueues, Common::Guid, ReplicationEndpointId, bool, bool);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryEOSOperation, Common::Guid, ReplicationEndpointId, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondarySendAcknowledgement, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, int, Common::Guid, uint32, std::vector<SecondaryReplicatorTraceInfo>, std::vector<SecondaryReplicatorTraceInfo>);
            DECLARE_RE_STRUCTURED_TRACE(SecondarySendVerboseAcknowledgement, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, int, Common::Guid, uint32, std::vector<SecondaryReplicatorTraceInfo>, std::vector<SecondaryReplicatorTraceInfo>, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryUpdatePrimaryAddress, Common::Guid, ReplicationEndpointId, std::wstring, ReplicationEndpointId, std::wstring, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryUpdateMinEpoch, Common::Guid, ReplicationEndpointId, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryEstablishMinAndStartCopyEpoch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryEstablishStateProviderEpoch, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCCAckReceived, Common::Guid, ReplicationEndpointId, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(SecondaryCCAckMissingCopySender, Common::Guid, ReplicationEndpointId);

            // Copy sender traces
            DECLARE_RE_STRUCTURED_TRACE(CopySenderReplDone, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderDone, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderStopGetNext, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderBeginCopy, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderEndCopy, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, int);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderOnLastEnumOp, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderUpdateLastReplicationLSN, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, LONGLONG);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderDisableBuildCompletion, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderDisableBuildCompletionFailed, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(CopySenderBuildSucceededAfterBeingDisabled, Common::Guid, ReplicationEndpointId, FABRIC_REPLICA_ID, std::wstring, std::wstring);
                        
            // Ack sender traces
            DECLARE_RE_STRUCTURED_TRACE(AckSenderPolicy, Common::Guid, ReplicationEndpointId, bool);
            DECLARE_RE_STRUCTURED_TRACE(AckSenderNotActive, Common::Guid, ReplicationEndpointId);
            DECLARE_RE_STRUCTURED_TRACE(AckSenderForceSend, Common::Guid, ReplicationEndpointId);

            // Transport traces
            DECLARE_RE_STRUCTURED_TRACE(TransportRegister, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(TransportUnregister, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(TransportStart, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(TransportStop, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(TransportMsgCallback, std::wstring, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(TransportMsgBatchCallback, std::wstring, LONGLONG, LONGLONG, LONGLONG, uint64);
            DECLARE_RE_STRUCTURED_TRACE(TransportSendMessage, std::wstring, std::wstring, int);
            DECLARE_RE_STRUCTURED_TRACE(TransportConfig, std::wstring, std::wstring, uint64);
            DECLARE_RE_STRUCTURED_TRACE(TransportSendCopyOperation, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ReplicationHeaderMissing, std::wstring);

            // Operation stream
            DECLARE_RE_STRUCTURED_TRACE(OperationStreamCtor, Common::Guid, ReplicationEndpointId, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(OperationStreamDtor, Common::Guid, ReplicationEndpointId, std::wstring);
            
            // Health reporting
            DECLARE_RE_STRUCTURED_TRACE(BatchedHealthReporterCtor, Common::Guid, ReplicationEndpointId, HealthReportType::Trace, Common::TimeSpan);
            DECLARE_RE_STRUCTURED_TRACE(BatchedHealthReporterStateChange, Common::Guid, ReplicationEndpointId, HealthReportType::Trace, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(BatchedHealthReporterSend, Common::Guid, ReplicationEndpointId, std::wstring, FABRIC_SEQUENCE_NUMBER, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(BatchedHealthReporterClose, Common::Guid, ReplicationEndpointId, HealthReportType::Trace);
            DECLARE_RE_STRUCTURED_TRACE(BatchedHealthReporterSendWarning, Common::Guid, ReplicationEndpointId, std::wstring, FABRIC_SEQUENCE_NUMBER, std::wstring, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ApiMonitoringSend, Common::Guid, ReplicationEndpointId, std::wstring, FABRIC_SEQUENCE_NUMBER, std::wstring);
            DECLARE_RE_STRUCTURED_TRACE(ApiMonitoringSendWarning, Common::Guid, ReplicationEndpointId, std::wstring, FABRIC_SEQUENCE_NUMBER, std::wstring);

            ReplicatorEventSource() :
                RE_STRUCTURED_TRACE(ReplicationSessionTrace, 121, Info, "\r\n{1}->{2}: {3}\t{4} dupAcks={5} IsIdleFaultedDueToSlowProgress={6} IsActiveFaultedDueToSlowProgress={7}\r\n", "contextSequenceId", "endpointUniqueId", "transportTarget", "replOpSender", "copySender", "replAcksReceivedDuringProcessing", "isIdleFaulted", "isActiveFaulted"),
                RE_STRUCTURED_TRACE(SecondaryTraceInfo, 188, Info, "\r\nLSN: {1}, ReceiveTime: {2}", "contextSequenceId", "lsn", "receiveTime"),
                RE_STRUCTURED_TRACE(SecondaryTraceBatchInfo, 189, Info, "\r\nLSN: (Low={1}, High={2}) ReceiveTime: {3}","contextSequenceId", "lowLsn", "highLsn", "receiveTime"),

                RE_STRUCTURED_TRACE(StateProviderUpdateEpoch, 5, Info, "{1}: Call StateProvider UpdateEpoch {2}.{3:x}, {4}", "id", "endpointUniqueId", "epochDataLoss", "epochConfiguration", "previousLSN"),
                RE_STRUCTURED_TRACE(StateProviderUpdateEpochCompleted, 104, Info, "{1}: StateProvider UpdateEpoch completed", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(StateProviderProgress, 6, Info, "{1}: State provider current progress: {2}", "id", "endpointUniqueId", "LSN"),
                RE_STRUCTURED_TRACE(StateProviderError, 7, Warning, "{1}: Error calling state provider {2}: {3}", "id", "endpointUniqueId", "function", "error"),
                // Factory
                RE_STRUCTURED_TRACE(ReplicatorConfigUpdate, 176, Info, "Parameter {1} Updated from {2}->{3}", "id", "parameter", "old", "new"),
                RE_STRUCTURED_TRACE(ReplicatorFactory, 4, Info, "{1}: Create config and transport at {2} for node {3}", "id", "replicaId", "endpoint", "node"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryConfigError, 9, Error, "{1}: Batch Ack period {2} greater than or equal to replication retry interval {3}", "id", "replicaId", "batchAckPeriod", "retryInterval"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryReplQueueSizeConfigError, 101, Error, "{1}: Initial {4} queue size {2} must be less than or equal to max {4} queue size {3}; Initial {4} queue size must be greater than 1;  Initial and max {4} queue size must be power of 2.", "id", "replicaId", "initialReplicationQueueSize", "maxReplicationQueueSize", "queueName"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryCopyQueueSizeConfigError, 102, Error, "{1}: Replicator Initial queue size {2} must be less than or equal to max queue size {3}; Initial queue size must be greater than 1;  Initial and max queue size must be power of 2.", "id", "replicaId", "initialCopyQueueSize", "maxCopyQueueSize"),
                RE_STRUCTURED_TRACE(ReplicatorFactorySecuritySettingsError, 140, Error, "{1}: The security settings {2} do no match the current transport security", "id", "replicaId", "security"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryMaxMessageSizeConfigError, 143, Error, "{1}: Max Replication message size {2} is greater than Max {4} Replication queue size {3}", "id", "replicaId", "maxMessageSize", "maxQueueSize", "queueName"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryGeneralConfigError, 148, Error, "{1}: {2}", "id", "replicaId", "reason"),
                // Replicator traces
                RE_STRUCTURED_TRACE(ReplicatorOpen, 8, Noise, "{1}: Open replicator, persisted state {2}", "id", "endpointUniqueId", "persistedState"),
                RE_STRUCTURED_TRACE(ReplicatorInvalidState, 10, Info, "{1}: {2}: Invalid state, desired {3}", "id", "description", "function", "state"),
                RE_STRUCTURED_TRACE(ReplicatorDropMsgNoFromHeader, 11, Info, "{1}: Drop {2}: no FROM header or empty", "id", "endpointUniqueId", "action"),
                RE_STRUCTURED_TRACE(ReplicatorDropMsgInvalid, 12, Info, "{1}: Drop {2} from {3}/{4}: invalid state or role", "id", "description", "action", "address", "actor"),
                RE_STRUCTURED_TRACE(ReplicatorDataLoss, 13, Info, "{1}: OnDataLoss: state provider didn't change state", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(ReplicatorDataLossStateChanged, 14, Info, "{1}: OnDataLoss: state provider changed state {2} to {3}", "id", "endpointUniqueId", "oldState", "newState"),
                RE_STRUCTURED_TRACE(ReplicatorChangeRole, 15, Info, "{1}: ChangeRole from {2} to {3}, epoch {4}.{5:x}", "id", "endpointUniqueId", "oldRole", "newRole", "epochDataLoss", "epochConfigNumber"),
                RE_STRUCTURED_TRACE(ReplicatorChangeRoleInvalid, 16, Info, "{1}: ChangeRole from {2} to {3} is not valid", "id", "endpointUniqueId", "oldRole", "newRole"),
                RE_STRUCTURED_TRACE(ReplicatorCtor, 151, Info, "{1}: Replicator.ctor. ReplicatorConfig: {2}", "id", "endpointUniqueId", "reConfig"),
                RE_STRUCTURED_TRACE(ReplicatorDtor, 17, Info, "{1}: Replicator.dtor", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(ReplicatorInvalidMessage, 18, Info, "{1}: Drop {2} - Invalid format from {3}/{4}", "id", "endpointUniqueId", "action", "adress", "actor"),
                RE_STRUCTURED_TRACE(ReplicatorReportFault, 103, Warning, "{1}: Replicator reported {2} fault due to {3} returned error {4}", "id", "endpointUniqueId", "faulttype", "reason", "error"),
                // Primary traces
                RE_STRUCTURED_TRACE(PrimaryCtor, 19, Info, "{1}: PrimaryReplicator.ctor: mini-reconfig enable: {2}, epoch {3}.{4:x}", "id", "endpointUniqueId", "minireconfig", "epochDataLoss", "epochConfiguration"),
                RE_STRUCTURED_TRACE(PrimaryCatchUpAll, 20, Info, "{1}: CatchUp ALL: progress achieved {2}: latest LSN {3}, current completed ({4})", "id", "endpointUniqueId", "progressAchieved", "previousLSN", "completedLSN"),
                RE_STRUCTURED_TRACE(PrimaryCatchUpQuorum, 21, Info, "{1}: CatchUp QUORUM: progress achieved {2}: previous last LSN {3}, current committed {4}", "id", "endpointUniqueId", "progressAchieved", "previousLSN", "currentLSN"),
                RE_STRUCTURED_TRACE(PrimaryCatchUpQuorumWithMustCatchupReplicas, 172, Info, "{1}: CatchUp QUORUM: progress achieved {2}: previous last LSN {3}, current committed {4}, lowest LSN amongst Must Catchup Replicas = {5}", "id", "endpointUniqueId", "progressAchieved", "previousLSN", "currentLSN", "lowestMustCatchup"),
                RE_STRUCTURED_TRACE(PrimaryAckReceive, 22, Noise, "{1}: Received ACK {2}.{3}:{4}.{5} from {6}:{0}-{7}-{8}", "id", "endpointUniqueId", "receivedReplLSN", "quorumReplLSN", "receivedCopyLSN", "quorumCopyLSN", "address", "actorReplicaId", "status"),
                RE_STRUCTURED_TRACE(PrimarySend, 24, Noise, "{1}->{2}: Send {3} {4} ({5}). Message {6}, {7}", "id", "endpointUniqueId", "replicaId", "opType", "LSN", "detail", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(PrimarySendInfo, 163, Info, "{1}->{2}: Send {3} {4} ({5}). Message {6}, {7}", "id", "endpointUniqueId", "replicaId", "opType", "LSN", "detail", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(PrimarySendW, 130, Noise, "{1}->{2}: Send {3} {4} ({5}). Message {6}, {7}", "id", "endpointUniqueId", "replicaId", "opType", "LSN", "sender", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(PrimarySendBatchW, 132, Noise, "{1}->{2}: Send {3} [{4}, {5}] ({6}). Message {7}, {8}", "id", "endpointUniqueId", "replicaId", "opType", "firstLSN", "lastLSN", "sender", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(PrimarySendLast, 25, Info, "{1}->{2}: Send {2} {3}, last ({4}). Message {5}, {6}", "id", "endpointUniqueId", "replicaId", "opType", "LSN", "detail", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(PrimaryCCMissing, 26, Info, "{1}->{2}: EndCopy: Not all CopyCONTEXT ops received", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryGetInfo, 27, Noise, "{1}: {2}: {3}", "id", "endpointUniqueId", "function", "LSN"),
                RE_STRUCTURED_TRACE(PrimaryReplicateCancel, 28, Info, "{1}: Cancel {2} pending REPL operations", "id", "endpointUniqueId", "count"),
                RE_STRUCTURED_TRACE(PrimaryReplicateDone, 29, Info, "{1}: Replicate op starting at {2} totaling {3} ops done", "id", "endpointUniqueId", "fromLSN", "total"),
                RE_STRUCTURED_TRACE(PrimaryPreCloseWait, 153, Info, "{1}: Primary pre close wait for replicate operations to complete in {2}", "id", "endpointUniqueId", "timeout"),
                RE_STRUCTURED_TRACE(PrimaryPreCloseWaitDone, 154, Info, "{1}: Finished waiting for pending operations to complete with Error {2} (Ignoring Error Code)", "id", "endpointUniqueId", "error"),
                RE_STRUCTURED_TRACE(PrimaryPendingReplicateOperationsCompleted, 155, Info, "{1}: Primary pending operations completed", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(PrimaryClose, 30, Info, "{1}: Close primary", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(PrimaryCloseDone, 137, Info, "{1}: Close primary Done with Error {2}", "id", "endpointUniqueId", "error"),
                RE_STRUCTURED_TRACE(PrimaryAbort, 138, Info, "{1}: Abort primary", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(PrimaryAbortDone, 139, Info, "{1}: Abort primary Done with Error {2}", "id", "endpointUniqueId", "error"),
                RE_STRUCTURED_TRACE(PrimaryRemoveReplica, 31, Info, "{1}: Remove {2}", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryAddIdleError, 32, Info, "{1}: Replica {2} already exists", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryPromoteIdle, 33, Info, "{1}: Promote idle {2} to active", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryUpdateReplQueue, 34, Noise, "{1}: Update replication queue with ({2}, {3})", "id", "endpointUniqueId", "committedLSN", "completedLSN"),
                RE_STRUCTURED_TRACE(PrimaryMissingReplicas, 35, Info, "{1}: UpdateConfig: {2} are not enough active replicas for active writeQuorum {3}", "id", "endpointUniqueId", "activeReplicas", "writeQuorum"),
                RE_STRUCTURED_TRACE(PrimaryReplicate, 36, Info, "{1}: Replicate {2} to {3} replicas", "id", "endpointUniqueId", "LSN", "count"),
                RE_STRUCTURED_TRACE(PrimaryReplicateBatch, 131, Info, "{1}: Replicate batch [{2}, {3}] to {4} replicas", "id", "endpointUniqueId", "firstLSN", "lastLSN", "count"),
                RE_STRUCTURED_TRACE(PrimaryAddSessionInitialRepl, 37, Noise, "{1}: AddPendingOperationsToSession: {2}: {3} pending REPL operations", "id", "endpointUniqueId", "replicaId", "count"),
                RE_STRUCTURED_TRACE(PrimaryUpdateEpoch, 38, Info, "{1}: UpdateEpoch {2}.{3:x} to {4}.{5:x}, {6}", "id", "endpointUniqueId", "epochDataLoss", "epochConfigNumber", "newEpochDataLoss", "newEpochConfigNumber", "state"),
                RE_STRUCTURED_TRACE(PrimaryUpdateCatchUpConfig, 39, Info, "{1}: UpdateCatchUpReplicaSetConfiguration with current: ({2} replicas, {3} writeQuorum), previous: ({4} replicas, {5} writeQuorum)", "id", "endpointUniqueId", "replicaCount", "writeQuorum", "previousReplicaCount", "previousWriteQuorum"),
                RE_STRUCTURED_TRACE(PrimaryUpdateCurrentConfig, 105, Info, "{1}: UpdateCurrentReplicaSetConfiguration with current: ({2} replicas, {3} writeQuorum)", "id", "endpointUniqueId", "replicaCount", "writeQuorum"),
                RE_STRUCTURED_TRACE(PrimaryReplMgrClosed, 40, Noise, "{1}: {2}: replica manager not opened", "id", "endpointUniqueId", "function"),
                RE_STRUCTURED_TRACE(PrimaryDtor, 41, Noise, "{1}: Primary.dtor", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(PrimaryStartCopy, 42, Info, "{1}->{2}: Start copy upto {3} replication LSN (exclusive). Incarnation ID is the last GUID", "id", "endpointUniqueId", "transportTarget", "LSN"),
                RE_STRUCTURED_TRACE(PrimaryAllCCReceived, 43, Info, "{1}->{2}:{3}: All CopyCONTEXT received, last LSN {4}", "id", "endpointUniqueId", "replicaId", "purpose", "LSN"),
                RE_STRUCTURED_TRACE(PrimaryReceiveCC, 44, Info, "{1}: Received CopyCONTEXT {2} from {3}/{4}: {5}", "id", "endpointUniqueId", "LSN", "address", "actor", "detail"),
                RE_STRUCTURED_TRACE(PrimaryReceiveCCNotPersisted, 45, Noise, "{1}: Drop CopyCONTEXT - service doesn't have persisted state", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(PrimaryCCEnqueueWithoutDispatch, 46, Noise, "{1}->{2}:{3}: EnqueueWithoutDispatch {4}", "id", "endpointUniqueId", "replicaId", "purpose", "LSN"),
                RE_STRUCTURED_TRACE(PrimaryQueueFull, 106, Info, "{1}: Primary replicator queue full:{2}\r\n Primary Latest LSN={3}", "id", "endpointUniqueId", "fulldesc", "latest"),
                RE_STRUCTURED_TRACE(PrimaryDropMsgDueToIncarnationId, 107, Info, "{1}: drop {2} message.  expected = {3}; received = {4}", "id", "endpointUniqueId", "message", "expected", "received"),
                RE_STRUCTURED_TRACE(PrimaryCopySessionEstablished, 108, Info, "{1}->{2}: Copy session establish operation for incarnationId = {3} complete with errorcode = {4}", "id", "endpointUniqueId", "replicaId", "incarnationId", "error"),
                RE_STRUCTURED_TRACE(PrimaryIdleDoesNotExist, 110, Info, "{1}: idle {2} does not exist", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryConfiguration, 142, Info, "{1}:{2}\r\nPrimary Latest LSN={3}\r\nREConfig=\r\n{4}", "id", "endpointUniqueId", "replicaManager", "latestLSN", "reconfig"),
                RE_STRUCTURED_TRACE(PrimaryMissingOperations, 144, Info, "{1}:Closing session for replica {2} as it cannot be caught up due to missing operations.", "id", "endpointUniqueId", "replicaId"),
                RE_STRUCTURED_TRACE(PrimaryCatchUpError, 92, Info, "{1}: CatchUp completed with {2}", "id", "endpointUniqueId", "error"),
                RE_STRUCTURED_TRACE(PrimaryAckProcess, 170, Info, "{1}: Processed ACK for Replica {2}. {3}.{4} {5}.{6}.{7}. dupAcks = {8}. batchProcessingAckBenefit = {9}; {10}", "id", "endpointUniqueId", "replicaId", "receivedReplLSN", "quorumReplLSN", "copyAcksBeingProcessed", "receivedCopyLSN", "quorumCopyLSN", "replAcksReceivedDuringProcessing", "batchackbenefit", "messageId"),
                RE_STRUCTURED_TRACE(PrimaryReplicateFailed, 152, Warning, "{1}: {2}", "id", "endpointUniqueId", "message"),
                RE_STRUCTURED_TRACE(PrimaryCleanupQueueDueToSlowIdle, 169, Warning, "{1}: Idle replica(s) is(are) causing primary queue usage to go over {2}. Clearing operations to decrease queue usage percent from {3} to {4}. Faulted any idle replicas in this iteration of clearing the primary queue = {5}", "id", "endpointUniqueId", "config", "startUsage", "endUsage", "isFaulted"),
                RE_STRUCTURED_TRACE(PrimaryMustCatchupUpdated, 171, Info, "{1}: Replica {2} updated must catchup flag from {3} to {4}. Last Ackd LSN is {5}", "id", "endpointUniqueId", "replicaId", "oldFlag", "newFlag", "lastAcked"),
                RE_STRUCTURED_TRACE(PrimaryDemoteOperationQueueCount, 173, Info, "{1}: Operation Queue Count {2} greater than max secondary queue size {3}. Completing operations from LSN {4}(INC) to {5}(INC) before changing role", "id", "endpointUniqueId", "opCount", "secondaryQueueSize", "from", "to"),
                RE_STRUCTURED_TRACE(PrimaryDemoteOperationQueueMemory, 174, Info, "{1}: Operation Queue Memory size {2} greater than max secondary queue memory size {3}. Completing operations from LSN {4}(INC) to {5}(INC) before changing role", "id", "endpointUniqueId", "opSize", "secondaryQueueMemSize", "from", "to"),
                RE_STRUCTURED_TRACE(PrimaryFaultedSlowSecondary, 175, Error, "{1}: Faulted slow secondary replica \r\n {2} \r\n Reason = {3}", "id", "endpointUniqueId", "session", "reason"),
                RE_STRUCTURED_TRACE(PrimaryCleanupQueueDueToSlowSecondary, 180, Warning, "{1}: Active secondary replica(s) is(are) causing primary queue usage to go over {2}. Clearing operations to decrease queue usage percent from {3} to {4}. Faulted active replicas in this iteration of clearing the primary queue = {5}", "id", "endpointUniqueId", "config", "startUsage", "endUsage", "isFaulted"),

                // ReliableOperationSender
                RE_STRUCTURED_TRACE(OpSenderOpen, 47, Info, "{1}->{2}:{3}: Open, ACK {4}.{5}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "lastAckQuorum"),
                RE_STRUCTURED_TRACE(OpSenderAddOps, 48, Noise, "{1}->{2}:{3}: ACKS {4}.{5}: Add operations {6} to {7}. SendWindowSize: {8}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "lastAckQuorum", "fromLSN", "endLSN", "sendWindowSize"),
                RE_STRUCTURED_TRACE(OpSenderAddOp, 49, Noise, "{1}->{2}:{3}: ACKS {4}.{5}: Add operation {6}. SendWindowSize: {7}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "lastAckQuorum", "LSN", "sendWindowSize"),
                RE_STRUCTURED_TRACE(OpSenderAddBatchOp, 100, Noise, "{1}->{2}:{3}: ACKS {4}.{5}: Add batch operation [{6}, {7}]. SendWindowSize: {8}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "lastAckQuorum", "firstLSN", "lastLSN", "sendWindowSize"),
                RE_STRUCTURED_TRACE(OpSenderRemoveOps, 50, Noise, "{1}->{2}:{3}: Remove operations {4} to {5} (up to LSN {6})", "id", "endpointUniqueId", "replicaId", "purpose", "fromLSN", "toLSN", "uptoLSN"),
                RE_STRUCTURED_TRACE(OpSenderRemoveOpsEmpty, 98, Noise, "{1}->{2}:{3}: Empty operations list (up to LSN {4})", "id", "endpointUniqueId", "replicaId", "purpose", "uptoLSN"),
                RE_STRUCTURED_TRACE(OpSenderRemoveOpsNothingRemoved, 99, Noise, "{1}->{2}:{3}: No operations have been removed {4} to {5} (up to LSN {6})", "id", "endpointUniqueId", "replicaId", "purpose", "frontLSN", "backLSN", "uptoLSN"),
                RE_STRUCTURED_TRACE(OpSenderNotActive, 51, Noise, "{1}->{2}:{3}: {4}: Op sender not active", "id", "endpointUniqueId", "replicaId", "purpose", "function"),
                RE_STRUCTURED_TRACE(OpSenderSend, 52, Noise, "{1}->{2}:{3}: Send {4} to {5}. SendWindowSize: {6}", "id", "endpointUniqueId", "replicaId", "purpose", "fromLSN", "toLSN", "sendWindowSize"),
                RE_STRUCTURED_TRACE(OpSenderTimer, 53, Noise, "{1}->{2}:{3}: No operations to send. Disable timer: {4}", "id", "endpointUniqueId", "replicaId", "purpose", "disableTimer"),
                RE_STRUCTURED_TRACE(OpSenderAck, 54, Noise, "{1}->{2}:{3}: Ack: {4} to {5}, {6} to {7}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "newAckReceived", "lastAckQuorum", "newAckQuorum"),
                RE_STRUCTURED_TRACE(OpSenderClose, 55, Info, "{1}->{2}:{3}: Close, last ACK {4}.{5}", "id", "endpointUniqueId", "replicaId", "purpose", "lastAckReceived", "lastAckQuorum"),
                // Secondary traces
                RE_STRUCTURED_TRACE(SecondaryCtor, 56, Info, "{1}: SecondaryReplicator.ctor ({2}): epoch {3}.{4:x}, incarnationId = {5}", "id", "endpointUniqueId", "state", "epochDataLoss", "epochConfiguration", "incarnationId"),
                RE_STRUCTURED_TRACE(SecondaryCtorInitial, 177, Info, "{1}: SecondaryReplicator.ctor ({2}): incarnationId = {3}", "id", "endpointUniqueId", "state", "incarnationId"),
                RE_STRUCTURED_TRACE(SecondaryGetInfo, 57, Info, "{1}: {2}: {3}, epoch {4}.{5:x}", "id", "endpointUniqueId", "infoType", "LSN", "epochDataLoss", "epochConfiguration"),
                RE_STRUCTURED_TRACE(SecondaryNotActive, 58, Info, "{1}: {2}: Secondary not active, NOP", "id", "endpointUniqueId", "function"),
                RE_STRUCTURED_TRACE(SecondaryDtor, 59, Info, "{1}: SecondaryReplicator.dtor", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(SecondaryReceive, 60, Noise, "{1}: Received {2} {3} from {4}/{5}", "id", "endpointUniqueId", "opType", "opLSN", "fromAddress", "fromDemuxerActor"),
                RE_STRUCTURED_TRACE(SecondaryReceiveBatch, 134, Noise, "{1}: Received {2} batch operation [{3}, {4}] (lastLSNInBatch={5}) from {6}/{7}", "id", "endpointUniqueId", "opType", "firstLSN", "lastLSN", "lastLSNInBatch", "fromAddress", "fromDemuxerActor"),
                RE_STRUCTURED_TRACE(SecondaryEnqueueWithoutDispatch, 61, Noise, "{1}: EnqueueWithoutDispatch {2} {3}", "id", "endpointUniqueId", "opType", "LSN"),
                RE_STRUCTURED_TRACE(SecondaryAllCopyReceived, 62, Info, "{1}: All COPY received, last non-null LSN {2}", "id", "endpointUniqueId", "LSN"),
                RE_STRUCTURED_TRACE(SecondaryClose, 63, Info, "{1}: Close secondary, promoting {2}, copy in progress {3}", "id", "endpointUniqueId", "promoting", "copyInProgress"),
                RE_STRUCTURED_TRACE(SecondaryDrainDispatchQueue, 64, Info, "{1}: Wait for {2} dispatch queue to drain when {3}. Item Count = {4}.", "id", "endpointUniqueId", "queueType", "reason", "itemCount"),
                RE_STRUCTURED_TRACE(SecondaryDrainDispatchQueueDone, 65, Info, "{1}: Wait for {2} dispatch queue to drain done with error code {3} when {4}", "id", "endpointUniqueId", "queueType", "error", "reason"),
                RE_STRUCTURED_TRACE(SecondaryGetLastOp, 66, Info, "{1}:{2} Got last null operation", "id", "endpointUniqueId", "purpose"),
                RE_STRUCTURED_TRACE(SecondaryUpdateEpochError, 67, Info, "{1}: UpdateEpoch: The proposed epoch {2}.{3:x} is less than existing one {4}.{5:x}", "id", "endpointUniqueId", "newEpochDataLoss", "newEpochConfiguration", "minAllowedEpochDataLoss", "minAllowedEpochConfiguration"),
                RE_STRUCTURED_TRACE(SecondaryUpdateEpoch, 68, Info, "{1}: UpdateEpoch: {2}.{3:x} to {4}.{5:x}. Next sequence number to complete: {6}", "id", "endpointUniqueId", "minAllowedEpochDataLoss", "minAllowedEpochConfiguration", "newEpochDataLoss", "newEpochConfiguration", "LSN"),
                RE_STRUCTURED_TRACE(SecondaryDispatch, 69, Noise, "{1}: Dispatch operations", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(SecondaryWaitForAcks, 70, Noise, "{1}: Wait for service to ACK {2} operations", "id", "endpointUniqueId", "opType"),
                RE_STRUCTURED_TRACE(SecondaryWaitForAcksDone, 71, Info, "{1}: Service ACKed all {2} operations when {3}. StreamFaulted = {4}", "id", "endpointUniqueId", "opType", "reason", "streamfaulted"),
                RE_STRUCTURED_TRACE(SecondaryAllCopyAcked, 72, Info, "{1}: All copy operations are service ACKed.  All copy op received {2}; copy queue closed {3}", "id", "endpointUniqueId", "allCopyOpReceived", "copyQueueClosed"),
                RE_STRUCTURED_TRACE(SecondarySendCC, 74, Noise, "{1}: Send CopyCONTEXT {2}, last {3}. Message {4}, {5}", "id", "endpointUniqueId", "LSN", "last", "messageGuid", "messageIndex"),
                RE_STRUCTURED_TRACE(SecondaryDropMsgDueToReplicaId, 75, Noise, "{1}: Drop {2} {3}: invalid replicaId {4}, expected {5}", "id", "endpointUniqueId", "opType", "LSN", "replicaId", "expectedReplicaId"),
                RE_STRUCTURED_TRACE(SecondaryDropMsgDueToEpoch, 76, Noise, "{1}: Drop {2} {3}: epoch {4}.{5:x} less than current one {6}.{7:x}", "id", "endpointUniqueId", "opType", "LSN", "epochDataLoss", "epochConfigNumber", "newEpochDataLoss", "newEpochConfigNumber"),
                RE_STRUCTURED_TRACE(SecondaryDropMsgPrimaryAddress, 77, Noise, "{1}: Drop {2}: primary address {3}/{4} not expected one {5}/{6}", "id", "endpointUniqueId", "opType", "address", "actor", "expectedAddress", "expectedActor"),
                RE_STRUCTURED_TRACE(SecondaryDropMsgDueToNoStartCopyEpoch, 178, Info, "{1}: Drop {2} {3}: as start copy epoch is not established yet. Primary epoch is {4}.{5:x}", "id", "endpointUniqueId", "opType", "LSN", "epochDataLoss", "epochConfigNumber"),
                RE_STRUCTURED_TRACE(SecondaryTempQueueError, 78, Noise, "{1}: Failed to move {2} from temp queue to replication queue: {3}", "id", "endpointUniqueId", "LSN", "error"),
                RE_STRUCTURED_TRACE(SecondaryTempQueueDiscard, 136, Noise, "{1}: Discarded {2} operations from the temporary replication queue", "id", "endpointUniqueId", "numberOfOps"),
                RE_STRUCTURED_TRACE(SecondaryCopyOperationCompletedInOtherThread, 97, Info, "{1}: Copy operation {2} already completed in other thread", "id", "endpointUniqueId", "LSN"),
                RE_STRUCTURED_TRACE(SecondaryDropMsgDueToIncarnationId, 109, Info, "{1}: drop {2} message.  expected = {3}; received = {4}", "id", "endpointUniqueId", "message", "expected", "received"),
                RE_STRUCTURED_TRACE(SecondaryReplDispatchCallback, 93, Info, "{1}: REPL commit callback enabled state: {2}", "id", "endpointUniqueId", "state"),
                RE_STRUCTURED_TRACE(SecondaryCloseDispatchQueues, 94, Info, "{1}: Close dispatch queues, abort copy {2}, abort repl {3}", "id", "endpointUniqueId", "abortCopy", "abortRepl"),
                RE_STRUCTURED_TRACE(SecondaryEOSOperation, 146, Info, "{1}: {2}-{3}", "id", "endpointUniqueId", "queueType", "operationType"),
                RE_STRUCTURED_TRACE(SecondaryUpdatePrimaryAddress, 160, Info, "{1}: Update primary address from {2}/{3} to {4}/{5}", "id", "endpointUniqueId", "fromaddress", "fromactor", "toAddress", "toActor"),
                RE_STRUCTURED_TRACE(SecondaryUpdateMinEpoch, 161, Info, "{1}: {2}: Update Min Epoch due to swap primary: {3}.{4:x} to {5}.{6:x}", "id", "endpointUniqueId", "operation", "minAllowedEpochDataLoss", "minAllowedEpochConfiguration", "newEpochDataLoss", "newEpochConfiguration"),
                RE_STRUCTURED_TRACE(SecondaryEstablishMinAndStartCopyEpoch, 179, Info, "{1}: Establish Min Epoch to {2}.{3:x}", "id", "endpointUniqueId", "minAllowedEpochDataLoss", "minAllowedEpochConfiguration"),
                RE_STRUCTURED_TRACE(SecondaryEstablishStateProviderEpoch, 181, Info, "{1}: Establish State Provider Epoch to {2}.{3:x}", "id", "endpointUniqueId", "spDataLoss", "spConfiguration"),
                RE_STRUCTURED_TRACE(SecondarySendAcknowledgement, 184, Info, "{1} Send ACK to {2}: {3}.{4}:{5},{6}. Error: {7}. Message {8}, {9}. \r\nReceived Replication Operation Summary:{10} \r\nReceived Copy Operation Summary:{11}", "id", "endpointUniqueId", "transportTarget", "replCommitLSN", "replCompleteLSN", "copyCommitLSN", "copyCompleteLSN", "error", "messageGuid", "messageIndex","replicationOpSummary", "copyOpSummary"),
                RE_STRUCTURED_TRACE(SecondarySendVerboseAcknowledgement, 190, Info, "{1} Send ACK to {2}: {3}.{4}:{5},{6}. Error: {7}. Message {8}, {9}. \r\nReceived Replication Operation Summary:{10} \r\nReceived Copy Operation Summary:{11} \r\nREConfig = \r\n{12}", "id", "endpointUniqueId", "transportTarget", "replCommitLSN", "replCompleteLSN", "copyCommitLSN", "copyCompleteLSN", "error", "messageGuid", "messageIndex", "replicationOpSummary", "copyOpSummary", "reconfig"),
                RE_STRUCTURED_TRACE(SecondaryCCAckReceived, 186, Info, "{1}: Received Copy Context ack from Primary. LSN: {2}, Error Code: {3}", "id", "endpointUniqueId", "LSN", "errorCodeValue"),
                RE_STRUCTURED_TRACE(SecondaryCCAckMissingCopySender, 187, Info,"{1}: cachedCopySender not found, Copy Context Ack will not be processed","id","endpointUniqueId"),

                // Copy traces
                RE_STRUCTURED_TRACE(CopySenderReplDone, 79, Noise, "{1}->{2}:{3}: All repl ACKs received with {4}", "id", "endpointUniqueId", "replicaId", "purpose", "LSN"),
                RE_STRUCTURED_TRACE(CopySenderDone, 80, Noise, "{1}->{2}:{3}: All copy and repl ops Acked", "id", "endpointUniqueId", "replicaId", "purpose"),
                RE_STRUCTURED_TRACE(CopySenderStopGetNext, 81, Info, "{1}->{2}:{3}: The copy async operation is completed, stop getting ops from async enumerator", "id", "endpointUniqueId", "replicaId", "purpose"),
                RE_STRUCTURED_TRACE(CopySenderBeginCopy, 82, Info, "{1}->{2}:{3}: start copy", "id", "endpointUniqueId", "replicaId", "purpose"),
                RE_STRUCTURED_TRACE(CopySenderEndCopy, 83, Info, "{1}->{2}:{3}: Completed with {4}", "id", "endpointUniqueId", "replicaId", "purpose","failReason"),
                RE_STRUCTURED_TRACE(CopySenderOnLastEnumOp, 23, Info, "{1}->{2}:{3}: Enumerator gave last op: COPY {4}, REPL {5}", "id", "endpointUniqueId", "replicaId", "purpose", "copyLSN", "replLSN"),
                RE_STRUCTURED_TRACE(CopySenderUpdateLastReplicationLSN, 150, Info, "{1}->{2}:{3}: Last Replication LSN Updated to {4}", "id", "endpointUniqueId", "replicaId", "purpose", "replLSN"),
                RE_STRUCTURED_TRACE(CopySenderDisableBuildCompletion, 164, Warning, "{1}->{2}: Build completion disabled. CopyState:{3}, Details:{4}", "id", "endpointUniqueId", "replicaId", "copyState", "progress"),
                RE_STRUCTURED_TRACE(CopySenderDisableBuildCompletionFailed, 165, Info, "{1}->{2}: Build completion disabling failed because the build is already completed: CopyState:{3}, Progress:{4}", "id", "endpointUniqueId", "replicaId", "copyState", "progress"),
                RE_STRUCTURED_TRACE(CopySenderBuildSucceededAfterBeingDisabled, 166, Warning, "{1}->{2}: Build completed successfully after disabling build completion: Purpose:{3}, Progress:{4}", "id", "endpointUniqueId", "replicaId", "purpose", "progress"),

                // Ack sender traces
                RE_STRUCTURED_TRACE(AckSenderPolicy, 85, Info, "{1}: Acks batched {2}", "id", "endpointUniqueId", "batchEnabled"),
                RE_STRUCTURED_TRACE(AckSenderNotActive, 86, Noise, "{1}: Not active", "id", "endpointUniqueId"),
                RE_STRUCTURED_TRACE(AckSenderForceSend, 87, Noise, "{1}: Trigger send now, disable timer", "id", "endpointUniqueId"),
                // Transport traces
                RE_STRUCTURED_TRACE(TransportRegister, 88, Info, "Register {1}", "id", "replicator"),
                RE_STRUCTURED_TRACE(TransportUnregister, 89, Info, "Unregister {1}", "id", "replicator"),
                RE_STRUCTURED_TRACE(TransportStart, 185, Info, "Starting transport with publishaddress: {1}", "id", "publishadd"),
                RE_STRUCTURED_TRACE(TransportStop, 90, Info, "Stop", "id"),
                RE_STRUCTURED_TRACE(TransportMsgCallback, 91, Noise, "{0} {1} operation sent ({2} size)", "Optype", "LSN", "size"),
                RE_STRUCTURED_TRACE(TransportMsgBatchCallback, 133, Noise, "{0} [{1}, {2}] operation (lastLSN in batch = {3}) sent ({4} size)", "Optype", "firstLSN", "lastLSN", "lastLSNInBatch", "size"),
                RE_STRUCTURED_TRACE(TransportSendMessage, 141, Noise, "SendMessage to {1} returned uneexpected error code {2}", "id", "receiver", "error"),
                RE_STRUCTURED_TRACE(TransportConfig, 145, Info, "Setting {1}={2}", "id", "configName", "value"),
                RE_STRUCTURED_TRACE(TransportSendCopyOperation, 135, Noise, "{0} being sent", "Optype"),
                RE_STRUCTURED_TRACE(ReplicationHeaderMissing, 182, Warning, "replication operation header is missing: {0}", "message"),
                // Misc
                RE_STRUCTURED_TRACE(OperationStreamCtor, 95, Info, "{1}:{2} Stream ctor", "id", "endpointUniqueId", "purpose"),
                RE_STRUCTURED_TRACE(OperationStreamDtor, 96, Info, "{1}:{2} Stream dtor", "id", "endpointUniqueId", "purpose"),

                // Health Reporting
                RE_STRUCTURED_TRACE(BatchedHealthReporterCtor, 156, Info, "{1}: Health Report type: {2}, Reporting Interval: {3}", "id", "endpointUniqueId", "reportType", "interval"),
                RE_STRUCTURED_TRACE(BatchedHealthReporterStateChange, 157, Noise, "{1}: Health report type: {2} state changed from {3} to {4}", "id", "endpointUniqueId", "reportType", "from", "to"),
                RE_STRUCTURED_TRACE(BatchedHealthReporterSend, 158, Info, "{1}: Health report state: {2}, SequenceNumber: {3}, Property: {4}, Description: {5}", "id", "endpointUniqueId", "state", "seqNumber", "property", "description"),
                RE_STRUCTURED_TRACE(BatchedHealthReporterSendWarning, 167, Warning, "{1}: Health report state: {2}, SequenceNumber: {3}, Property: {4}, Description: {5}", "id", "endpointUniqueId", "state", "seqNumber", "property", "description"),
                RE_STRUCTURED_TRACE(BatchedHealthReporterClose, 183, Info, "{1}: Health report type: {2}, closing BatchedHealthReporter, scheduling OK health report", "id", "endpointUniqueId", "reportType"),
                RE_STRUCTURED_TRACE(ApiMonitoringSend, 159, Info, "{1}: Health report state: {2}, SequenceNumber: {3}, Property: {4}", "id", "endpointUniqueId", "state", "seqNumber", "property"),
                RE_STRUCTURED_TRACE(ApiMonitoringSendWarning, 168, Warning, "{1}: Health report state: {2}, SequenceNumber: {3}, Property: {4}", "id", "endpointUniqueId", "state", "seqNumber", "property"),
                RE_STRUCTURED_TRACE(ReplicatorFactoryShareTransport, 162, Info, "{1}: Sharing transport at {2} for node {3}", "id", "replicaId", "endpoint", "node")
            {
            }

            static Common::Global<ReplicatorEventSource> Events;
        }; // end ReplicatorEventSource

    } // end namespace ReplicationComponent
} // end namespace Reliability
