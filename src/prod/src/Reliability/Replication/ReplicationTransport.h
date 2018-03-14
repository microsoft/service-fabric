// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Manages the communication between the local replication engines
        // and other replication engines.
        // It's shared by all replication engines on the current node.
        class ReplicationTransport : public Common::ComponentRoot
        {
            DENY_COPY(ReplicationTransport)
        public:
            ~ReplicationTransport();

            __declspec(property(get = get_SecuritySetting)) std::shared_ptr<Transport::SecuritySettings> SecuritySetting;
            std::shared_ptr<Transport::SecuritySettings> get_SecuritySetting() const 
            {
                Common::AcquireReadLock lock(securitySettingsLock_);
                return std::make_shared<Transport::SecuritySettings>(securitySettings_); 
            }

            __declspec(property(get=get_Endpoint)) std::wstring const & Endpoint;
            std::wstring const & get_Endpoint() const { return endpoint_; }

            __declspec(property(get=get_Id)) std::wstring const & Id;
            std::wstring const & get_Id() { return unicastTransport_->IdString; }
            
            __declspec(property(put=set_MaxMessageSize)) int MaxMessageSize;
            void set_MaxMessageSize(int value)
            {
                if (value > maxMessageSize_)
                {
                    // We only set the maximum message size if it is greater than what was already initialized
                    // This is because multiple replicators use a common transport here, and we must guard against the largest value
                    // Individual limits are enforced in the replicator API's like BeginReplicate
                    maxMessageSize_ = value;
                    unicastTransport_->SetMaxOutgoingFrameSize(value); //This also ensures send queue limit is enough for maxMessageSize_
                    ReplicatorEventSource::Events->TransportConfig(
                        endpoint_,
                        L"MaxMessageSize",
                        maxMessageSize_);
                }
            }

            __declspec(property(put=set_MaxPendingUnsentBytes)) ULONG MaxPendingUnsentBytes;
            void set_MaxPendingUnsentBytes(ULONG value)
            {
                // We want to maintain increasing value setting, but also ensure once its set to 
                // max, we dont change it again

                // maxPendingUnsentBytes_ == -1 implies the variable is not changed after constructor initialization
                // maxPendingUnsentBytes_ == 0 implies the max value is set in transport

                if (maxPendingUnsentBytes_ == 0 ||
                    (value != 0 && value < maxPendingUnsentBytes_))
                {
                    // If variable is already initialized to 0, ignore any more changes as the queue is at its max size
                    // Also ignore if new value is lesser than already existing value
                    ReplicatorEventSource::Events->TransportConfig(
                        endpoint_,
                        L"MaxPendingUnsentBytes_Ignored",
                        maxPendingUnsentBytes_);
                    return;
                }
                    
                auto error = unicastTransport_->SetPerTargetSendQueueLimit(value);
                maxPendingUnsentBytes_ = value;
                ASSERT_IFNOT(error.IsSuccess(), "SetPerTargetSendQueueLimit failed: {0}", error);
                ReplicatorEventSource::Events->TransportConfig(
                    endpoint_,
                    L"MaxPendingUnsentBytes",
                    maxPendingUnsentBytes_);
            }

            static std::shared_ptr<ReplicationTransport> Create(std::wstring const & address, std::wstring const& id = L"");

            static Common::GlobalWString ReplicationAckAction;
            static Common::GlobalWString ReplicationOperationAction;
            static Common::GlobalWString ReplicationOperationBodyAction;
            static Common::GlobalWString CopyOperationAction;
            static Common::GlobalWString CopyContextOperationAction;
            static Common::GlobalWString StartCopyAction;
            static Common::GlobalWString CopyContextAckAction;
            static Common::GlobalWString RequestAckAction;
            static Common::GlobalWString InduceFaultAction;

            Common::ErrorCode SetSecurity(Transport::SecuritySettings const& securitySettings);

            template <class T>
            void RegisterMessageProcessor(__in T & messageProcessor);

            template <class T>
            void UnregisterMessageProcessor(__in T & messageProcessor);

            Common::ErrorCode Start(
                __in std::wstring const & publishAddress,
                __in_opt Transport::SecuritySettings const & securitySettings = Transport::SecuritySettings());

            void Stop();

            bool SecuritySettingsMatch(Transport::SecuritySettings const & securitySettings) const;
            
            Common::ErrorCode SendMessage(
                KBuffer const & buffer,
                Common::Guid const & partitionId,
                Transport::ISendTarget::SPtr const & receiverTarget, 
                __in Transport::MessageUPtr && message, Common::TimeSpan const & messageExpiration) const;

            KBuffer::SPtr CreateSharedHeaders(
                ReplicationEndpointId const & sendActor,
                ReplicationEndpointId const & receiverActor,
                std::wstring const & action) const;

            Common::ErrorCode ReturnSendMessage(Common::ErrorCode const & errorCode, Transport::ISendTarget::SPtr const & receiverTarget) const;

            Transport::ISendTarget::SPtr ResolveTarget(std::wstring const & endpoint, std::wstring const& id = L"");

            static Transport::MessageUPtr CreateReplicationOperationMessage(
                ComOperationCPtr const & operation, 
                FABRIC_SEQUENCE_NUMBER lastSequenceNumberInBatch,
                FABRIC_EPOCH const & epoch,
                bool enableReplicationOperationHeaderInBody,
                FABRIC_SEQUENCE_NUMBER completedSequenceNumber = Constants::InvalidLSN);

            static bool GetReplicationBatchOperationFromMessage(
                __in Transport::Message & message, 
                OperationAckCallback const & ackCallback,
                __out std::vector<ComOperationCPtr> & batchOperation, 
                __out FABRIC_EPOCH & epoch,
                __out FABRIC_SEQUENCE_NUMBER & completedSequenceNumber);

            template <class T>
            static bool ReadInBodyOperationHeader(
                __in Transport::Message & message,
                __inout vector<Common::const_buffer> & msgBuffers,
                __out T & header);

            static bool ReadOperationHeaderInBodyBytes(
                __in Transport::Message & message,
                __inout vector<Common::const_buffer> & msgBuffers,
                __out vector<byte> & bodyHeaderBytes);

            static Transport::MessageUPtr CreateCopyOperationMessage(
                ComOperationCPtr const & operation,
                FABRIC_REPLICA_ID replicaId, 
                FABRIC_EPOCH const & epoch,
                bool isLast,
                bool enableReplicationOperationHeaderInBody);

            static bool GetCopyOperationFromMessage(
                __in Transport::Message & message, 
                OperationAckCallback const & ackCallback,
                __out ComOperationCPtr & operation, 
                __out FABRIC_REPLICA_ID & replicaId, 
                __out FABRIC_EPOCH & epoch,
                __out bool & isLast);

            static Transport::MessageUPtr CreateCopyContextOperationMessage(
                ComOperationCPtr const & operation,
                bool isLast);

            static bool GetCopyContextOperationFromMessage(
                __in Transport::Message & message, 
                __out ComOperationCPtr & operation, 
                __out bool & isLast);

            static Transport::MessageUPtr CreateStartCopyMessage(
                FABRIC_EPOCH const & epoch,
                FABRIC_REPLICA_ID replicaId, 
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber);
            
            static void GetStartCopyFromMessage(
                __in Transport::Message & message, 
                __out FABRIC_EPOCH & epoch,
                __out FABRIC_REPLICA_ID & replicaId, 
                __out FABRIC_SEQUENCE_NUMBER & replicationStartSequenceNumber);
            
            static Transport::MessageUPtr CreateInduceFaultMessage(
                FABRIC_REPLICA_ID targetReplicaId, 
                Common::Guid const & targetReplicaIncarnationId, 
                std::wstring const & reason);

            static void GetInduceFaultFromMessage(
                __in Transport::Message & message, 
                __out FABRIC_REPLICA_ID & targetReplicaId, 
                __out Common::Guid & targetReplicaIncarnationId, 
                __out std::wstring & reason);
           
            static Transport::MessageUPtr CreateAckMessage(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN = Constants::NonInitializedLSN,
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN = Constants::NonInitializedLSN,
                int errorCodeValue = 0);

            static void GetAckFromMessage(
                __in Transport::Message & message, 
                __out FABRIC_SEQUENCE_NUMBER & replicationReceivedLSN, 
                __out FABRIC_SEQUENCE_NUMBER & replicationQuorumLSN, 
                __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
                __out FABRIC_SEQUENCE_NUMBER & copyQuorumLSN,
                __out int & errorCodeValue);

            static Transport::MessageUPtr CreateCopyContextAckMessage(
                FABRIC_SEQUENCE_NUMBER sequenceNumber,
                int errorCodeValue);

            static void GetCopyContextAckFromMessage(
                __in Transport::Message & message, 
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
                __out int & errorCodeValue);

            static Transport::MessageUPtr CreateRequestAckMessage();

            void GeneratePublishEndpoint(
                ReplicationEndpointId const & uniqueId,
                __out std::wstring & endpoint) const;

        private:
            ReplicationTransport(
                std::wstring const & endpoint, 
                std::wstring const& id);

            static Transport::MessageUPtr CreateMessageFromCopyOperationWithHeaderInBody(
                ComOperationCPtr const & operation,
                FABRIC_REPLICA_ID replicaId,
                FABRIC_EPOCH const & epoch,
                bool isLast);

            static Transport::MessageUPtr CreateMessageFromCopyOperation(
                ComOperationCPtr const & operation,
                std::vector<ULONG> & segmentSizes);

            Transport::IDatagramTransportSPtr unicastTransport_;
            std::wstring endpoint_;
            std::wstring publishEndpoint_; // Initialized during Start()
            ReplicationDemuxerUPtr demuxer_;
            Transport::SecuritySettings securitySettings_;
            MUTABLE_RWLOCK(REReplicationTransportSecuritySettings, securitySettingsLock_);
            // Maximum size of a single replication message
            int maxMessageSize_;
            // Maximum size of pending unsent bytes
            LONGLONG maxPendingUnsentBytes_;
        }; // end class ReplicationTransport
                
        template <class T>
        void ReplicationTransport::RegisterMessageProcessor(__in T & messageProcessor)
        {
            ReplicatorEventSource::Events->TransportRegister(
                endpoint_,
                messageProcessor.get_ReplicationEndpointId().ToString());

            Common::ComponentRootSPtr root = messageProcessor.CreateComponentRoot();

            demuxer_->RegisterMessageHandler(
                messageProcessor.get_ReplicationEndpointId(),
                [&messageProcessor, root](Transport::MessageUPtr & message, Transport::ReceiverContextUPtr & context) mutable
                {
                    messageProcessor.ProcessMessage(*message, context);
                },
                true/*dispatchOnTransportThread*/);
        }

        template <class T>
        bool ReplicationTransport::ReadInBodyOperationHeader(
            __in Transport::Message & message,
            __inout vector<Common::const_buffer> & msgBuffers,
            __out T & header)
        {
            bool bodyHeaderDetected = false;
            std::vector<byte> bodyHeaderBytes;

            bodyHeaderDetected = ReadOperationHeaderInBodyBytes(message, msgBuffers, bodyHeaderBytes);

            if (bodyHeaderDetected)
            {
                ErrorCode err = FabricSerializer::Deserialize(header, bodyHeaderBytes);
                TESTASSERT_IFNOT(err.IsSuccess(), "failed to deserialize the operation header from body");

                if (!err.IsSuccess())
                {
                    ReplicatorEventSource::Events->ReplicationHeaderMissing(L"deserialization failed.");
                    bodyHeaderDetected = false;
                }
                else
                {
                    bodyHeaderDetected = true;
                }
            }

            return bodyHeaderDetected;
        }

        template <class T>
        void ReplicationTransport::UnregisterMessageProcessor(
            __in T & messageProcessor)
        {
            ReplicatorEventSource::Events->TransportUnregister(
                endpoint_,
                messageProcessor.get_ReplicationEndpointId().ToString());

            demuxer_->UnregisterMessageHandler(messageProcessor.get_ReplicationEndpointId());
        }
            
    } // end namespace ReplicationComponent
} // end namespace Reliability
