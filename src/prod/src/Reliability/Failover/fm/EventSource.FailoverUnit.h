// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FTEventSource
        {
        public:

            Common::TraceEventWriter<Common::Guid, uint16> FTProcessing;
            Common::TraceEventWriter<Common::Guid> FTLockFailure;
            
            Common::TraceEventWriter<Common::Guid, int64, Federation::NodeId> BuildReplicaSuccess;
            Common::TraceEventWriter<Common::Guid, int64, Federation::NodeId, Common::ErrorCode> BuildReplicaFailure;
            
            Common::TraceEventWriter<Common::Guid, FailoverUnit, std::vector<StateMachineActionUPtr>, int32, int64, int64 > FTUpdateBackground;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, std::vector<StateMachineActionUPtr>, int32, int64, int64 > FTUpdateBackgroundNoise;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, FailoverUnit, std::vector<StateMachineActionUPtr>, int32, int64, int64> FTUpdateBackgroundWithOld;
            Common::TraceEventWriter<Common::Guid> FTUpdateInBuildQuorumLossRebuild;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateDataLossRebuild;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateNodeStateRemoved;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, FailoverUnit> FTUpdateNodeStateRemovedWithOld;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateRecover;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, FailoverUnit> FTUpdateRecoverWithOld;
            Common::TraceEventWriter<Common::Guid, FailoverUnitInfo, bool> FTReplicaUpdateReceive;
            Common::TraceEventWriter<Common::Guid, Federation::NodeInstance> FTReplicaUpdateProcess;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateGFUMTransfer;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateReplicaDown;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, FailoverUnit> FTUpdateReplicaDownWithOld;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTUpdateRebuildComplete;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTState;
            Common::TraceEventWriter<std::wstring, std::wstring> FTStateAdmin;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, std::vector<StateMachineActionUPtr>, int32> FTAction;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, std::vector<StateMachineActionUPtr>, int32> FTActionDca;
            Common::TraceEventWriter<Common::Guid, std::wstring, Common::TraceCorrelatedEventBase> FTMessageProcessing;
            Common::TraceEventWriter<std::wstring, std::wstring, Federation::NodeId, int64, Common::TraceCorrelatedEventBase> FTMessageSend;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTQuorumLoss;
            Common::TraceEventWriter<std::wstring, std::wstring> FTQuorumLossAdmin;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTQuorumLossDuringDelete;
            Common::TraceEventWriter<std::wstring, std::wstring> FTQuorumLossDuringDeleteAdmin;
            Common::TraceEventWriter<Common::Guid, FailoverUnit> FTQuorumRestored;
            Common::TraceEventWriter<std::wstring> FTInBuildQuorumLoss;
            Common::TraceEventWriter<Common::Guid, Epoch> FTDataLoss;
            Common::TraceEventWriter<Common::Guid, std::wstring, Federation::NodeId, int64, Common::TraceCorrelatedEventBase> FTMessageIgnore;

            Common::TraceEventWriter<std::wstring, uint64, std::wstring, std::wstring> ReconfigurationStarted;
            Common::TraceEventWriter<std::wstring, uint64, std::wstring, std::wstring> ReconfigurationCompleted;
            Common::TraceEventWriter<std::wstring, uint64, std::wstring> ServiceLocationUpdated;
            Common::TraceEventWriter<std::wstring, uint64> PartitionDeleted;
            Common::TraceEventWriter<Common::Guid, FailoverUnit, Common::ErrorCode> FTUpdateFailureError;
            Common::TraceEventWriter<Common::Guid> FTUpdateFailureBecauseAlreadyDeleted;

            FTEventSource(Common::TraceTaskCodes::Enum id) :
                
                FTProcessing(id, 110, "FT", Common::LogLevel::Noise, "Processing {0} with {1} dynamic tasks", "id", "taskCount"),
                FTLockFailure(id, 111, "FTLockFailure", Common::LogLevel::Noise, "Failed to acquire lock for {0}", "id"),

                BuildReplicaSuccess(id, 112, "BuildReplicaSuccess", Common::LogLevel::Info, "Replica {1} on node {2} for FT {0} built successfully", "id", "replicaId", "nodeId"),
                BuildReplicaFailure(id, 113, "BuildReplicaFailure", Common::LogLevel::Warning, "Replica {1} on node {2} for FT {0} failed with error {3}", "id", "replicaId", "nodeId", "error"),

                FTUpdateBackground(id, 114, "Background_FTUpdate", Common::LogLevel::Info, "Updated: {1}\r\nActions: {2}\r\nReplicaDifference: {3}, Commit Duration: {4} ms, PLB Duration: {5} ms", "id", "failoverUnit", "actions", "replicaDifference", "commitTime", "plbDuration"),
                FTUpdateBackgroundNoise(id, 115, "Noise_Background_FTUpdate", Common::LogLevel::Noise, "Updated: {1}\r\nActions: {2}\r\nReplicaDifference: {3}, Commit Duration: {4} ms, PLB Duration: {5} ms", "id", "failoverUnit", "actions", "replicaDifference", "commitTime", "plbDuration"),
                FTUpdateInBuildQuorumLossRebuild(id, 116, "InBuildQuorumLossRebuild", Common::LogLevel::Warning, "InBuildFailoverUnit {0} in quorum loss updated to trigger full rebuild.", "id"),
                FTUpdateBackgroundWithOld(id, 117, "Background_Old_FTUpdate", Common::LogLevel::Info, "New: {1}Old: {2}\r\nActions: {3}\r\nReplicaDifference: {4}, Commit Duration: {4} ms, PLB Duration: {5} ms", "id", "failoverUnit", "failoverUnitOld", "actions", "replicaDifference", "commitTime", "plbDuration"),
                FTUpdateDataLossRebuild(id, 118, "DataLossRebuild_FTUpdate", Common::LogLevel::Warning, "Data loss detected. Triggering full rebuild: {1}", "id", "failoverUnit"),
                FTUpdateNodeStateRemoved(id, 119, "NodeStateRemoved_FTUpdate", Common::LogLevel::Info, "NodeStateRemoved: {1}", "id", "failoverUnit"),
                FTUpdateNodeStateRemovedWithOld(id, 120, "NodeStateRemoved_Old_FTUpdate", Common::LogLevel::Info, "NodeStateRemoved New: {1}Old: {2}", "id", "failoverUnit", "failoverUnitOld"),
                FTUpdateRecoverWithOld(id, 121, "Recover_Old_FTUpdate", Common::LogLevel::Info, "RecoverPartitions New: {1}Old: {2}", "id", "failoverUnit", "failoverUnitOld"),
                FTUpdateRecover(id, 122, "Recover_FTUpdate", Common::LogLevel::Info, "RecoverPartition: {1}", "id", "failoverUnit"),
                FTReplicaUpdateReceive(id, 123, "Receive_ReplicaUpdate", Common::LogLevel::Info, "Receiving {1}, isDropped {2}", "id", "fuInfo", "dropped"),
                FTReplicaUpdateProcess(id, 124, "Process_ReplicaUpdate", Common::LogLevel::Noise, "Processing replica from {1}", "id", "from"),
                FTUpdateGFUMTransfer(id, 125, "GFUMTransfer_FTUpdate", Common::LogLevel::Info, "GFUMTransfer: {1}", "id", "failoverUnit"),
                FTUpdateReplicaDown(id, 126, "ReplicaDown_FTUpdate", Common::LogLevel::Info, "ReplicaDown: {1}", "id", "failoverUnit"),
                FTUpdateReplicaDownWithOld(id, 127, "ReplicaDown_Old_FTUpdate", Common::LogLevel::Info, "ReplicaDown New: {1}Old: {2}", "id", "failoverUnit", "failoverUnitOld"),
                FTUpdateRebuildComplete(id, 128, "RebuildComplete_FTUpdate", Common::LogLevel::Info, "RebuildComplete: {1}", "id", "failoverUnit"),
                FTState(id, 129, "FTState", Common::LogLevel::Info, "{1}", "id", "failoverUnit"),
                FTStateAdmin(id, 130, "Admin_FTState", Common::LogLevel::Info, Common::TraceChannelType::Admin, "{1}", "id", "failoverUnit"),
                FTAction(id, 131, "FTAction", Common::LogLevel::Info, "FT: {1}\r\nActions: {2}\r\nDiff: {3}", "id", "failoverUnit", "actions", "replicaDifference"),
                FTActionDca(id, 132, "FTActionRetry", Common::LogLevel::Info, "FT: {1}\r\nActions: {2}\r\nDiff: {3}", "id", "failoverUnit", "actions", "replicaDifference"),
                FTMessageProcessing(id, 133, "FTMessage", Common::LogLevel::Info, "FM processing message {1}: {2}", "id", "action", "body"),
                FTMessageSend(id, 134, "FTSend", Common::LogLevel::Info, "Sending message {1} to {2}:{3}: {4}", "id", "action", "nodeId", "instanceId", "body"),
                FTQuorumLoss(id, 135, "FTQuorumLoss", Common::LogLevel::Warning, Common::TraceChannelType::Debug, "FT is in quorum loss state: {1}", "id", "failoverUnit"),
                FTQuorumLossAdmin(id, 136, "Admin_FTQuorumLoss", Common::LogLevel::Warning, Common::TraceChannelType::Admin, "FT is in quorum loss state: {1}", "id", "failoverUnit"),
                FTQuorumLossDuringDelete(id, 137, "DuringDelete_FTQuorumLoss", Common::LogLevel::Warning, Common::TraceChannelType::Debug, "FT is in quorum loss state during delete: {1}", "id", "failoverUnit"),
                FTQuorumLossDuringDeleteAdmin(id, 138, "AdminDuringDelete_FTQuorumLoss", Common::LogLevel::Warning, Common::TraceChannelType::Admin, "FT is in quorum loss state during delete: {1}", "id", "failoverUnit"),
                FTQuorumRestored(id, 139, "FTQuorumRestored", Common::LogLevel::Warning, "Quorum restored: {1}", "id", "failoverUnit"),
                FTInBuildQuorumLoss(id, 140, "FTInBuildQuorumLoss", Common::LogLevel::Warning, "InBuildFT {0} is in quorum loss state", "id"),
                FTDataLoss(id, 141, "FTDataLoss", Common::LogLevel::Warning, "Data loss reported for FT {0}: DatalossEpoch={1}", "id", "epoch"),
                FTMessageIgnore(id, 142, "FTMessageIgnore", Common::LogLevel::Info, "Ignoring message {1} from {2}:{3}: {4}", "id", "action", "nodeId", "nodeInstance", "body"),

                ReconfigurationStarted(id, 143, "_Partitions_ReconfigurationStarted", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Reconfiguration started for {0}/{1}. Primary: {2}, Secondaries: {3}", "id", "dca_instance", "primary", "secondaries"),
                ReconfigurationCompleted(id, 144, "_Partitions_ReconfigurationCompleted", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Reconfiguration completed for {0}/{1}. Primary: {2}, Secondaries: {3}", "id", "dca_instance", "primary", "secondaries"),
                ServiceLocationUpdated(id, 145, "_Partitions_ServiceLocationUpdated", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Service location updated for {0}: {2}", "id", "dca_instance", "instances"),
                PartitionDeleted(id, 146, "_Partitions_DEL_PartitionDeleted", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Partition {0} is deleted", "id", "dca_instance"),
                FTUpdateFailureError(id, 147, "Error_FTUpdateFailure", Common::LogLevel::Warning, "Update of FailoverUnit failed with error {2}: {1}", "id", "failoverUnit", "error"),
                FTUpdateFailureBecauseAlreadyDeleted(id, 148, "AlreadyDeleted_FTUpdateFailure", Common::LogLevel::Warning, "FailoverUnit {0} has already been deleted", "id")
                
                // Event id limit: 159
            {
            }

            void TraceFTUpdateBackground(
                LockedFailoverUnitPtr & failoverUnit,
                std::vector<StateMachineActionUPtr> const & actions,
                int replicaDifference,
                int64 commitDuration,
                int64 plbDuration) const;

            void TraceFTUpdateNodeStateRemoved(LockedFailoverUnitPtr & failoverUnit) const;

            void TraceFTUpdateRecover(LockedFailoverUnitPtr & failoverUnit) const;

            void TraceFTUpdateReplicaDown(LockedFailoverUnitPtr & failoverUnit) const;

            void TraceFTAction(FailoverUnit const& failoverUnit, std::vector<StateMachineActionUPtr> const& actions, int32 replicaDifference) const;

        };
    }
}
