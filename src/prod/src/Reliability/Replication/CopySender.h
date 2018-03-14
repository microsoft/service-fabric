// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Keeps track of the progress of copy operation
        // from starting it until completing it.
        // Creates a copy async operation to get operations from state provider
        // and maintains copy state to indicate whether copy 
        // was not started, is in progress or it finished
        class CopySender
        {
            DENY_COPY(CopySender)

        public:
            CopySender(
                REInternalSettingsSPtr const & config,
                ComProxyStatefulServicePartition const & partition,
                Common::Guid const & partitionId,
                std::wstring const & purpose,
                ReplicationEndpointId const & from,
                FABRIC_REPLICA_ID to,
                bool checkReplication,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                Common::ApiMonitoring::ApiName::Enum apiName);

            ~CopySender();

            __declspec (property(get=get_FinishCopySequenceNumber)) FABRIC_SEQUENCE_NUMBER FinishCopySequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_FinishCopySequenceNumber() const
            {
                Common::AcquireReadLock lock(copyLock_);
                return copyState_.LastReplicationSequenceNumber;
            }

            void GetCopyProgress(
                __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
                __out FABRIC_SEQUENCE_NUMBER & copyAppliedLSN,
                __out Common::TimeSpan & avgCopyReceiveAckDuration,
                __out Common::TimeSpan & avgCopyApplyAckDuration,
                __out FABRIC_SEQUENCE_NUMBER & notReceivedCopyCount,
                __out FABRIC_SEQUENCE_NUMBER & receivedAndNotAppliedCopyCount,
                __out Common::DateTime & lastCopyAckProcessedTime)  const;

            Common::AsyncOperationSPtr BeginCopy(
                ComProxyAsyncEnumOperationData && asyncEnumOperationData,
                SendOperationCallback const & copySendCallback,
                EnumeratorLastOpCallback const & enumeratorLastOpCallback,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndCopy(
                Common::AsyncOperationSPtr const & asyncOperation,
                Common::ErrorCode & faultErrorCode,
                std::wstring & faultDescription);

            void FinishOperation(Common::ErrorCode const & error);

            // Sets the last copy sequence number 
            // when the state provider supplies a null operation
            void OnLastEnumeratorCopyOp(
                FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber,
                FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber);

            bool UpdateLastReplicationOperationDuringCopy(FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber);
            
            void ProcessReplicationAck(
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
                FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber);

            bool ProcessCopyAck(
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN);

            bool IsOperationLast(ComOperationCPtr const & operation);

            bool TryDisableBuildCompletion(std::wstring const & progress);

            std::wstring ToString() const;
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            
        private:
            // Struct that follows the state of the copy operation:
            // not in progress, started but last copy LSN not received,
            // last copy LSN received.
            struct CopyState
            {
                DENY_COPY(CopyState);

            public:
                explicit CopyState(bool waitForReplicationAcks);
                
                __declspec (property(get=get_LastCopySequenceNumber)) FABRIC_SEQUENCE_NUMBER LastCopySequenceNumber;
                FABRIC_SEQUENCE_NUMBER get_LastCopySequenceNumber() const { return lastCopySequenceNumber_; }
                
                __declspec (property(get=get_LastReplicationSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastReplicationSequenceNumber;
                FABRIC_SEQUENCE_NUMBER get_LastReplicationSequenceNumber() const { return lastReplicationSequenceNumber_; }

                bool IsInProgress() const;
                bool IsLastCopyLSN(FABRIC_SEQUENCE_NUMBER copyLSN) const;
                bool IsReplicationCompleted() const { return state_ == ReplCompleted; }
                bool IsCompleted() const { return state_ == Completed; }
                bool IsStarted() const { return state_ == Started; }
                
                void Start();
                void SetLSNs(FABRIC_SEQUENCE_NUMBER copyLSN, FABRIC_SEQUENCE_NUMBER replicationLSN);
                bool UpdateReplicationLSN(FABRIC_SEQUENCE_NUMBER replicationLSN);
                bool TryCompleteReplication(FABRIC_SEQUENCE_NUMBER sequenceNumber);
                void Finish(bool succeeded);

                std::wstring ToString() const;
            private:

                enum LSNState 
                { 
                    // Copy is not started, the copy and repl LSNs are not initialized
                    NotStarted = 0, 
                    // Started, but copy and repl LSN are still not set
                    Started, 
                    // The repl and copy LSNs are set                    
                    LSNSet, 
                    // Replication condition is achieved
                    ReplCompleted,
                    // Copy is fully done
                    Completed 
                };

                // The last copy sequence number that must be ACKed before
                // copy can complete
                FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber_;
                
                // The last replication operation that must
                // be ACKed before copy can complete
                FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber_;
                
                LSNState state_;
            
                // If set to true, copy can't be done
                // until all replication operations up to the desired LSN 
                // are also ACKed.
                bool waitForReplicationAcks_;
            };

            REInternalSettingsSPtr const config_;
            ComProxyStatefulServicePartition partition_;
            ReplicationEndpointId const endpointUniqueueId_;
            FABRIC_REPLICA_ID const replicaId_;
            Common::Guid const partitionId_;
            std::wstring const & purpose_;
            ApiMonitoringWrapperSPtr apiMonitor_;
            Common::ApiMonitoring::ApiName::Enum apiName_;
            
            // Async operation that takes the copy operations from 
            // the state provider and sends them to the secondary
            Common::AsyncOperationSPtr copyAsyncOperation_;

            int errorCodeValue_;

            bool disableBuildCompletion_;

            // Copy state that checks whether copy has completed or not,
            // and keeps track of the last copy and replication LSNs
            mutable CopyState copyState_;
            MUTABLE_RWLOCK(RECopySender, copyLock_);
            
        }; // end CopySender

    } // end namespace ReplicationComponent
} // end namespace Reliability
