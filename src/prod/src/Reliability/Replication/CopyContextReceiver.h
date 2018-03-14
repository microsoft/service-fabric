// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Keeps track of the progress of copy context operations;
        // Used on primary to take the live operations coming from secondary
        // and serving them to the state provider.
        class CopyContextReceiver : public Common::ComponentRoot
        {
            DENY_COPY(CopyContextReceiver)

        public:
            CopyContextReceiver(
                REInternalSettingsSPtr const & config,
                Common::Guid const & partitionId,
                std::wstring const & purpose,
                ReplicationEndpointId const & from, 
                FABRIC_REPLICA_ID to);

            ~CopyContextReceiver();

            __declspec (property(get=get_AllOperationsReceived)) bool AllOperationsReceived;
            bool get_AllOperationsReceived() const { return allOperationsReceived_; }
            
            void GetStateAndSetLastAck(
                __out FABRIC_SEQUENCE_NUMBER & lastCompletedSequenceNumber,
                __out int & errorCodeValue);

            void Open(SendCallback const & copySendCallback);

            void Close(Common::ErrorCode const error);

            void CreateEnumerator(
                __out Common::ComPointer<IFabricOperationDataStream> & context);

            bool ProcessCopyContext(
                ComOperationCPtr && op,
                bool isLast);

        private:
            static std::wstring GetDescription(
                std::wstring const & purpose,
                ReplicationEndpointId const & from,
                FABRIC_REPLICA_ID to);

            void DispatchOperation(ComOperationCPtr const & operation);

            REInternalSettingsSPtr const config_;
            Common::Guid const partitionId_;
            ReplicationEndpointId const endpointUniqueId_;
            FABRIC_REPLICA_ID const replicaId_;
            std::wstring const purpose_;

            // Operation queue where the copy context operations
            // are sorted and prepared to be moved to the dispatch queue.
            // Since the user only received IFabricOperationData*, 
            // there's no need to ACK the operations.
            // Therefore, the queue will act as if persisted state is false.
            OperationQueue queue_;
            FABRIC_SEQUENCE_NUMBER lastCopyContextSequenceNumber_;
            bool allOperationsReceived_;
            // Dispatch queue from where the async enumerator given to the 
            // state provider is taking the operations.
            DispatchQueueSPtr dispatchQueue_;
            // Ack sender that acknowledges the copy context operations
            // to the secondary.
            AckSender ackSender_;
            FABRIC_SEQUENCE_NUMBER lastAckSent_;
            // The copy context enumerator that is passed to the 
            // state provider
            Common::ComPointer<ComOperationDataAsyncEnumerator> copyContextEnumerator_;
            // Value of the error encountered;
            // if there are no errors, it will be 0
            int errorCodeValue_;

            bool isActive_;
            // Lock that protects the copy context data structures
            MUTABLE_RWLOCK(RECopyContextReceiver, lock_);
            
        }; // end CopyContextReceiver

    } // end namespace ReplicationComponent
} // end namespace Reliability
