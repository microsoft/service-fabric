// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Keeps track of the progress of a replica 
        // from the configuration.
        class ReplicationSession : public RemoteSession
        {
            DENY_COPY(ReplicationSession)

        public:
            
            ReplicationSession(
                REInternalSettingsSPtr const & config,
                ComProxyStatefulServicePartition const & partition,
                FABRIC_REPLICA_ID replicaId, 
                std::wstring const & replicatorAddress, 
                std::wstring const & transportEndpointId,
                FABRIC_SEQUENCE_NUMBER currentProgress,
                ReplicationEndpointId const & primaryEndpointUniqueId,
                Common::Guid const & partitionId,
                FABRIC_EPOCH const & epoch,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                ReplicationTransportSPtr const & transport);
           
            virtual ~ReplicationSession();

            __declspec (property(get=get_IdleReplicaProgress)) FABRIC_SEQUENCE_NUMBER IdleReplicaProgress;
            FABRIC_SEQUENCE_NUMBER get_IdleReplicaProgress();

            __declspec(property(get=get_IsIdleFaultedDueToSlowProgress)) bool IsIdleFaultedDueToSlowProgress;
            bool get_IsIdleFaultedDueToSlowProgress() const;

            __declspec(property(get=get_IsActiveFaultedDueToSlowProgress)) bool IsActiveFaultedDueToSlowProgress;
            bool get_IsActiveFaultedDueToSlowProgress() const;

            __declspec(property(get=get_MustCatchupEnum, put=put_MustCatchupEnum)) MustCatchup::Enum MustCatchup;
            MustCatchup::Enum get_MustCatchupEnum() const;
            void put_MustCatchupEnum(MustCatchup::Enum value);

            void Open() override;

            void Close() override;

            void OnPromoteToActiveSecondary();

            bool TryFaultIdleReplicaDueToSlowProgress(
                FABRIC_SEQUENCE_NUMBER firstReplicationLsn, 
                FABRIC_SEQUENCE_NUMBER lastReplicationLsn,
                FABRIC_SEQUENCE_NUMBER currentReplicaAtLsn);

            bool TryFaultActiveReplicaDueToSlowProgress(
                StandardDeviation const & avgApplyAckDuration,
                StandardDeviation const & avgReceiveAckDuration,
                FABRIC_SEQUENCE_NUMBER firstReplicationLsn,
                FABRIC_SEQUENCE_NUMBER lastReplicationLsn);

            std::wstring ToString() const;
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        protected:

            void StartFaultReplicaMessageSendTimerIfNeeded() override;

            void StartCopy(FABRIC_SEQUENCE_NUMBER replicationStartSeq, Transport::SendStatusCallback const & sendStatusCallback) override;

            bool SendStartCopyMessage(
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber, Transport::SendStatusCallback const & sendStatusCallback) override;

            bool SendCopyOperation(
                ComOperationCPtr const & operation,
                bool isLast) override;

            bool SendReplicateOperation(
                ComOperationCPtr const & operation,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber) override;

            bool SendRequestAck() override;

            bool SendCopyContextAck(bool shouldTrace) override;

        private:
            KBuffer::SPtr const replicationOperationHeadersSPtr_;
            KBuffer::SPtr const copyOperationHeadersSPtr_;
            KBuffer::SPtr const copyContextAckOperationHeadersSPtr_;
            KBuffer::SPtr const requestAckHeadersSPtr_;
            KBuffer::SPtr const induceFaultHeadersSPtr_;
            KBuffer::SPtr const startCopyHeadersSPtr_;

            void FaultReplicaMessageSender(Common::TimerSPtr const & timer);
            void StartFaultReplicaMessageSendTimerIfNeededCallerHoldsLock();
            void StartFaultReplicaMessageSendTimerCallerHoldsLock();

            Common::atomic_bool isPromotedtoActiveSecondary_;

            MUTABLE_RWLOCK(REReplicationSessionFault, faultLock_);
            // ----------------------Slow Idle Progress Mitigation Fields-------------------------------
            // bool to indicate if this replica is considered faulted due to slow progress
            // If this is 'true', the replica does not get sent any additional replication operations and will not be included in quorum/receive ack calculations
            Common::atomic_bool isIdleFaultedDueToSlowProgress_;
            std::wstring progressInformationWhenFaulted_;
            Common::TimerSPtr sendInduceFaultMessageTimer_;
            // The above fields are protected by the faultLock 
            // ----------------------Slow Idle Progress Mitigation Fields-------------------------------

            // ----------------------Slow Active Progress Mitigation Fields-------------------------------
            // bool to indicate if this replica is considered faulted due to slow progress
            // If this is 'true', the replica does not get sent any additional replication operations and will not be included in quorum/receive ack calculations
            Common::atomic_bool isActiveFaultedDueToSlowProgress_;
            // The above fields are protected by the faultLock
            // ----------------------Slow Active Progress Mitigation Fields-------------------------------

            MUTABLE_RWLOCK(REReplicationSessionMustCatchup, mustCatchupLock_);
            MustCatchup::Enum mustCatchup_;
        }; // end ReplicationSession

    } // end namespace ReplicationComponent
} // end namespace Reliability
