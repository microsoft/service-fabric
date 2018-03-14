// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class RemoteSession : public Common::ComponentRoot
        {
            DENY_COPY(RemoteSession)

        public:
            
            static ReplicationEndpointId GetEndpointUniqueId(std::wstring const & replicatorAddress);
            
            RemoteSession(
                REInternalSettingsSPtr const & config,
                ComProxyStatefulServicePartition const & partition,
                FABRIC_REPLICA_ID replicaId,
                std::wstring const & replicatorAddress,
                std::wstring const & transportEndpointId,
                ReplicationEndpointId const & primaryEndpointUniqueId,
                Common::Guid const & partitionId,
                FABRIC_EPOCH const & epoch,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                ReplicationTransportSPtr const & transport,
                ReliableOperationSender && replicationOperationSender);
           
            virtual ~RemoteSession();

            virtual void Open() = 0;

            virtual void Close() = 0;

            virtual void StartFaultReplicaMessageSendTimerIfNeeded() = 0;

            __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec(property(get = get_TransportTarget)) std::wstring const & TransportTarget;
            std::wstring const & get_TransportTarget() const { return transportTarget_; }

            __declspec(property(get=get_ReplicationEndpoint)) std::wstring const & ReplicationEndpoint;
            std::wstring const & get_ReplicationEndpoint() const { return endpoint_; }

            __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId const & EndpointUniqueId;
            ReplicationEndpointId const & get_EndpointUniqueId() const { return replicaEndpointUniqueId_; }

            __declspec(property(get=get_IncarnationId)) Common::Guid const & IncarnationId;
            Common::Guid const & get_IncarnationId() const { return replicaEndpointUniqueId_.IncarnationId; }

            __declspec (property(get=get_AvgReceiveAckDuration)) Common::TimeSpan AvgReceiveAckDuration;
            Common::TimeSpan get_AvgReceiveAckDuration() const { return replicationOperations_.AverageReceiveAckDuration; }

            __declspec (property(get=get_AvgApplyAckDuration)) Common::TimeSpan AvgApplyAckDuration;
            Common::TimeSpan get_AvgApplyAckDuration() const { return replicationOperations_.AverageApplyAckDuration; }

            Common::AsyncOperationSPtr BeginEstablishCopy(
                FABRIC_SEQUENCE_NUMBER replicationStartSeq,
                bool hasPersistedState,
                Transport::SendStatusCallback const & sendStatusCallback,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            Common::ErrorCode EndEstablishCopy(
                Common::AsyncOperationSPtr const & asyncOperation,
                Common::ComPointer<IFabricOperationDataStream> & context);

            Common::AsyncOperationSPtr BeginCopy(
                ComProxyAsyncEnumOperationData && asyncEnumOperationData,
                EnumeratorLastOpCallback const & enumeratorLastOpCallback,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            Common::ErrorCode EndCopy(
                Common::AsyncOperationSPtr const & asyncOperation,
                Common::ErrorCode & faultErrorCode,
                std::wstring & faultDescription);

            void CompleteEstablishCopyAsyncOperation(
                Common::Guid const & incarnationId,
                Common::ErrorCode const & errorCode);

            void CloseStartCopyReliableOperationSender();

            void CancelCopy();

            // Create the copy context on top of the queue
            // that keeps the copy context operations received from secondary.
            // Should only be called when the service has persisted state.
            void CreateCopyContext(
                __out Common::ComPointer<IFabricOperationDataStream> & context);

            void CleanupCopyContextReceiver(Common::ErrorCode error);

            void OnCopyFailure(int errorCodeValue);

            bool ProcessCopyContext(
                ComOperationCPtr && op,
                bool isLast);

            bool HasEndpoint(std::wstring const & address, ReplicationEndpointId const & endpointUniqueId) const;
            
            bool IsSameReplica(FABRIC_REPLICA_ID replica) const;
            
            void AddReplicateOperation(
                ComOperationRawPtr const operationPtr,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber);

            void AddReplicateOperations(
                ComOperationRawPtrVector const & operations,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber);

            void OnUpdateEpoch(FABRIC_EPOCH const & epoch);

            FABRIC_SEQUENCE_NUMBER GetLastApplyLsn(FABRIC_SEQUENCE_NUMBER completedLsn = Constants::InvalidLSN);
            FABRIC_SEQUENCE_NUMBER GetLastReceiveLsn(FABRIC_SEQUENCE_NUMBER completedLsn = Constants::InvalidLSN);

            void UpdateAckProgress(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
                std::wstring messageId, 
                ReplicationAckProgressCallback callback);

            void UpdateAckProgress(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                std::wstring messageId, 
                ReplicationAckProgressCallback callback);

            // Sets the last copy sequence number 
            // when the state provider supplies a null operation
            void OnLastEnumeratorCopyOp(
                FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber,
                FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber,
                Common::ErrorCode error);

            bool UpdateLastReplicationSequenceNumberForIdle(FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber);

            ServiceModel::RemoteReplicatorStatus GetDetailsForQuery(bool isIdle);

            bool Test_IsReplicationAckProcessingInProgress() const;

        protected:

            __declspec(property(get=get_IsSessionActive)) bool IsSessionActive;
            bool get_IsSessionActive() const {  return isSessionActive_.load(); }

            __declspec(property(get=get_LastCopyReceiveAck)) FABRIC_SEQUENCE_NUMBER LastCopyReceiveAck;
            FABRIC_SEQUENCE_NUMBER get_LastCopyReceiveAck() const { Common::AcquireReadLock lock(replicationAckProcessingLock_); return latestCopyReceiveAck_; }

            __declspec(property(get=get_LastCopyQuorumAck)) FABRIC_SEQUENCE_NUMBER LastCopyQuorumAck;
            FABRIC_SEQUENCE_NUMBER get_LastCopyQuorumAck() const { Common::AcquireReadLock lock(replicationAckProcessingLock_); return latestCopyQuorumAck_; }

            // 
            // Derived class must provide the callback for sending replication operations
            // 
            void OnOpen(__in SendOperationCallback const & replicationOperationSendCallback);
            
            void OnClose();

            void OnStartCopy(
                __in ComOperationCPtr const & startCopyOp_,
                __in ReliableOperationSenderSPtr & startCopySender,
                __in Transport::SendStatusCallback const & sendStatusCallback);

            virtual void StartCopy(FABRIC_SEQUENCE_NUMBER replicationStartSeq, Transport::SendStatusCallback const & sendStatusCallback) = 0;

            virtual bool SendStartCopyMessage(
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber, Transport::SendStatusCallback const & sendStatusCallback) = 0;

            virtual bool SendCopyOperation(
                ComOperationCPtr const & operation,
                bool isLast) = 0;

            virtual bool SendReplicateOperation(
                ComOperationCPtr const & operation,
                FABRIC_SEQUENCE_NUMBER completedSeqNumber) = 0;

            virtual bool SendRequestAck() = 0;

            virtual bool SendCopyContextAck(bool shouldTrace) = 0;

            bool SendTransportMessage(
                KBuffer::SPtr const & sharedHeaders,
                Transport::MessageUPtr && message,
                bool setExpiration);

            FABRIC_EPOCH ReadEpoch() const;
            void SetEpoch(FABRIC_EPOCH const & epoch);

            CopyContextReceiverSPtr GetCopyContextReceiver();
            
            REInternalSettingsSPtr const config_;
            FABRIC_REPLICA_ID const replicaId_;
            std::wstring const endpoint_;
            ReplicationEndpointId const replicaEndpointUniqueId_;
            ReplicationEndpointId const primaryEndpointUniqueId_;
            Common::Guid const partitionId_;
            ComProxyStatefulServicePartition const & partition_;
            ReplicationTransportSPtr const transport_;
            Transport::ISendTarget::SPtr const sendTarget_;
            std::wstring const transportTarget_;

            CopySender copySender_;
            ReliableOperationSender replicationOperations_;

        private:
            class EstablishCopyAsyncOperation;

            //
            // Returns true if true progress. False means duplicate ACK
            //
            bool UpdateAckProgress(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
                bool processCopyAck,
                std::wstring messageId, 
                ReplicationAckProgressCallback callback);

            void AckProcessor(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN,
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                bool processCopyAck,
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
                ReplicationAckProgressCallback callback);

            Common::atomic_bool isSessionActive_;
            ComOperationCPtr startCopyOp_;

            MUTABLE_RWLOCK(RERemoteSession, lock_);
            CopyContextReceiverSPtr copyContextReceiver_;
            ReliableOperationSenderSPtr startCopyOpsSender_;

            RWLOCK(RERemoteSessionEstablishCopy, establishCopyLock_);
            Common::AsyncOperationSPtr establishCopyAsyncOperation_;

            MUTABLE_RWLOCK(RERemoteSessionAckProcessing, replicationAckProcessingLock_);
            FABRIC_SEQUENCE_NUMBER latestReplicationReceiveAck_;
            FABRIC_SEQUENCE_NUMBER latestReplicationQuorumAck_;
            FABRIC_SEQUENCE_NUMBER latestCopyReceiveAck_;
            FABRIC_SEQUENCE_NUMBER latestCopyQuorumAck_;
            bool replicationAckProcessingInProgress_;
            bool processCopyAcks_;
            std::wstring latestTraceMessageId_;
            Common::Stopwatch lastAckTraceWatch_;

            // Setting and reading epoch is protected by a lock because it could be updated while we are trying to re-send some operation
            MUTABLE_RWLOCK(RERemoteSessionEpoch, epochLock_);
            FABRIC_EPOCH epoch_;
        }; 

    } // end namespace ReplicationComponent
} // end namespace Reliability
