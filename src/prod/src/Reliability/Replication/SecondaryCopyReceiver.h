// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class that deals with copy operations received by secondary.
        // It dispatches the copy operations in order so they can be taken by the service
        // and keeps track of the progress so ACKs can be send to the primary.
        // The class is not thread safe, and must be protected by the secondary lock.
        class SecondaryCopyReceiver 
        {
            DENY_COPY(SecondaryCopyReceiver);

        public:
            SecondaryCopyReceiver(
                REInternalSettingsSPtr const & config,
                bool requireServiceAck,
                ReplicationEndpointId const & endpointUniqueId,
                Common::Guid const & partitionId,
                bool enableEOSOpAndStreamFaults,
                bool copyDone);
              
            ~SecondaryCopyReceiver();

            __declspec (property(get=get_AllOperationsReceived)) bool AllOperationsReceived;
            bool get_AllOperationsReceived() const { return allCopyOperationsReceived_; }

            __declspec (property(get=get_DispatchQueue)) DispatchQueueSPtr const & DispatchQueue;
            DispatchQueueSPtr const & get_DispatchQueue() const { return dispatchQueue_; }

            __declspec (property(get=get_EnqueuedEndOfStreamOperation)) bool EnqueuedEndOfStreamOperation;
            bool get_EnqueuedEndOfStreamOperation() const { return enqueuedEndOfStreamOperation_; }

            __declspec (property(get=get_AllOperationsAcked)) bool AllOperationsAcked;
            bool get_AllOperationsAcked()
            {
                if (enqueuedEndOfStreamOperation_)
                {
                    // If end of stream operation was enqueued, we must check that it has been ACK'd
                    return ackedEndOfStreamOperation_ && !HasCommittedOperations();
                }
                else
                {
                    return !HasCommittedOperations();
                }
            }

            bool IsCopyInProgress() const { return copyQueue_ != nullptr; }

            bool IsStreamInitialized()
            {
                auto stream = stream_.lock();
                return (stream != nullptr);
            }

            OperationStreamSPtr GetStream(
                SecondaryReplicatorSPtr const & secondary);

            void CleanQueueOnClose(
                Common::ComponentRoot const & root,
                bool ignoreEOSOperationAck);

            void CleanQueueOnUpdateEpochFailure(
                Common::ComponentRoot const & root);
            
            void CloseCopyOperationQueue() { copyQueueClosed_ = true; }

            bool ProcessCopyOperation(
                ComOperationCPtr && operation, 
                bool isLast,
                __out bool & progressDone,
                __out bool & shouldCloseDispatchQueue,
                __out bool & checkForceSendAck,
                __out bool & checkCopySender);
            
            void GetAck(
                __out FABRIC_SEQUENCE_NUMBER & copyCommittedLSN,
                __out FABRIC_SEQUENCE_NUMBER & copyCompletedLSN);

            FABRIC_SEQUENCE_NUMBER GetAckProgress();

            bool OnAckCopyOperation(
                ComOperation const & operation,
                __out bool & checkCopySender);

            void OnAckEndOfStreamOperation(ComOperation const & operation);

            void EnqueueEndOfStreamOperationAndKeepRootAlive(OperationAckCallback const & callback, Common::ComponentRoot const & root);
                                                
            void OnStreamReportFault(Common::ComponentRoot const & root);

            ServiceModel::ReplicatorQueueStatusSPtr GetQueueStatusForQuery() const;
        private:
            void CreateCopyQueue();

            bool HasCommittedOperations() const { return copyQueue_ && copyQueue_->HasCommittedOperations(); }
            void DispatchCopyOperation(ComOperationCPtr const & operation);
            bool TryDispatchCopy(__out bool & checkForceSendAck);

            void MarkEOSOperationACK(bool ackIgnored=false);
            void DiscardOutOfOrderOperations();
            void CleanQueue(
                Common::ComponentRoot const & root,
                bool ignoreEOSOperationAck);

            REInternalSettingsSPtr const config_;
            bool const requireServiceAck_;
            ReplicationEndpointId const endpointUniqueId_;
            Common::Guid const partitionId_;
             // Indicates if end of stream operation and faulted stream configs are enabled (both are enabled/disabled together)
            bool const enableEOSOpAndStreamFaults_;  
           
            DispatchQueueSPtr dispatchQueue_;
            
            // The stream given to the state provider to pump
            // copy operations.
            OperationStreamWPtr stream_;
            Common::atomic_bool streamCreated_;

            OperationQueueUPtr copyQueue_;
            FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber_;
            
            bool allCopyOperationsReceived_;
            bool copyQueueClosed_;
            
            FABRIC_SEQUENCE_NUMBER lastCopyCompletedAck_;

            bool enqueuedEndOfStreamOperation_;
            bool ackedEndOfStreamOperation_;
            
            bool replicationOrCopyStreamFaulted_;
        }; // end class SecondaryCopyReceiver

    } // end namespace ReplicationComponent
} // end namespace Reliability
