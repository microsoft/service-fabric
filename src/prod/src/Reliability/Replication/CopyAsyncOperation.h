// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Manages copy process - takes operations from the 
        // async enumerator, queues them and sends them to the other side.
        // Used on the secondary by the persisted services 
        // for copy context operation.
        // Used on the primary to get copy operations from the state provider
        // and send them to the idle secondary to bring it up to date
        // (partial copy for persisted services or full copy for non-persisted ones).
        // The context is completed when all copy operations
        // are ACKed by the receiver.
        class CopyAsyncOperation : public Common::AsyncOperation
        {
        public:
           CopyAsyncOperation(
                REInternalSettingsSPtr const & config, 
                Common::Guid const & partitionId,
                std::wstring const & purpose,
                std::wstring const & description,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_REPLICA_ID const & replicaId,
                ComProxyAsyncEnumOperationData && asyncEnumOperationData,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                Common::ApiMonitoring::ApiName::Enum apiName, 
                SendOperationCallback const & copySendCallback,
                EnumeratorLastOpCallback const & enumeratorLastOpCallback,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            virtual ~CopyAsyncOperation();

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
            void OnCancel();

            void UpdateProgress(
                FABRIC_SEQUENCE_NUMBER quorumSequenceNumber, 
                FABRIC_SEQUENCE_NUMBER receivedSequenceNumber, 
                Common::AsyncOperationSPtr const & thisSPtr,
                bool isLast);

            std::wstring GetOperationSenderProgress() const;

            void GetCopyProgress(
                __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
                __out FABRIC_SEQUENCE_NUMBER & copyAppliedLSN,
                __out Common::TimeSpan & avgCopyReceiveAckDuration,
                __out Common::TimeSpan & avgCopyApplyAckDuration,
                __out FABRIC_SEQUENCE_NUMBER & notReceivedCopyCount,
                __out FABRIC_SEQUENCE_NUMBER & receivedAndNotAppliedCopyCount,
                __out Common::DateTime & lastAckProcessedTime);
            
            bool TryCompleteWithSuccessIfCopyDone(Common::AsyncOperationSPtr const & thisSPtr);
            
            void OpenReliableOperationSender();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                Common::ErrorCode & faultErrorCode,
                std::wstring & faultDescription);

        private:

            bool TryEnqueue(
                Common::AsyncOperationSPtr const & asyncOperation, 
                Common::ComPointer<IFabricOperationData> && opPointer, 
                bool isLast);
            void GetNext(Common::AsyncOperationSPtr const & thisSPtr);
            void GetNextCallback(Common::AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc);
            bool FinishGetNext(Common::AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc);
            
            REInternalSettingsSPtr const config_;
            Common::Guid const partitionId_;
 
            // Lock that protects the queue (and its related state variables) used to
            // keep the operations given by the enumerator until they are completed
            MUTABLE_RWLOCK(RECopyAsyncOperation, lock_);
            OperationQueue queue_;
            bool queuePreviouslyFull_;
            ComOperationCPtr pendingOperation_;
            bool pendingOperationIsLast_;
            bool getNextCallsDone_;
            
            ComProxyAsyncEnumOperationData asyncEnumOperationData_;
            SendOperationCallback const copySendCallback_;
            ReliableOperationSender copyOperations_;
            EnumeratorLastOpCallback enumeratorLastOpCallback_;
            bool readyToComplete_;
            
            Common::ErrorCode faultErrorCode_;
            std::wstring faultDescription_;

            ApiMonitoringWrapperSPtr apiMonitor_;
            Common::ApiMonitoring::ApiNameDescription apiNameDesc_;
        }; 
        
    } // end namespace ReplicationComponent
} // end namespace Reliability
