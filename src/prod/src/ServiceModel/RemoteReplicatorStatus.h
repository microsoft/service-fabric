// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class RemoteReplicatorStatus
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        RemoteReplicatorStatus();

        RemoteReplicatorStatus(
            FABRIC_REPLICA_ID replicaId,
            Common::DateTime lastAcknowledgementProcessedTime,
            FABRIC_SEQUENCE_NUMBER lastReceivedReplicationSeqNumber,
            FABRIC_SEQUENCE_NUMBER lastAppliedReplicationSeqNumber,
            bool isInBuild,
            FABRIC_SEQUENCE_NUMBER lastReceivedCopySeqNumber,
            FABRIC_SEQUENCE_NUMBER lastAppliedCopySeqNumber,
            RemoteReplicatorAcknowledgementStatus && remoteReplicatorAcknowledgementStatus);

        RemoteReplicatorStatus(RemoteReplicatorStatus && other);
        
        RemoteReplicatorStatus & operator=(RemoteReplicatorStatus && other);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REMOTE_REPLICATOR_STATUS & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REMOTE_REPLICATOR_STATUS & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_08(
            replicaId_,
            lastAckProcessedTime_,
            lastReceivedReplicationSeqNumber_,
            lastAppliedReplicationSeqNumber_,
            isInBuild_,
            lastReceivedCopySeqNumber_,
            lastAppliedCopySeqNumber_,
            remoteReplicatorAckStatus_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ReplicaId, replicaId_)
            SERIALIZABLE_PROPERTY(Constants::LastAcknowledgementProcessedTimeUtc, lastAckProcessedTime_)
            SERIALIZABLE_PROPERTY(Constants::LastReceivedReplicationSequenceNumber, lastReceivedReplicationSeqNumber_)
            SERIALIZABLE_PROPERTY(Constants::LastAppliedReplicationSequenceNumber, lastAppliedReplicationSeqNumber_)
            SERIALIZABLE_PROPERTY(Constants::IsInBuild, isInBuild_)
            SERIALIZABLE_PROPERTY(Constants::LastReceivedCopySequenceNumber, lastReceivedCopySeqNumber_)
            SERIALIZABLE_PROPERTY(Constants::LastAppliedCopySequenceNumber, lastAppliedCopySeqNumber_)
            SERIALIZABLE_PROPERTY(Constants::RemoteReplicatorAckStatus, remoteReplicatorAckStatus_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_REPLICA_ID replicaId_;
        Common::DateTime lastAckProcessedTime_;
        FABRIC_SEQUENCE_NUMBER lastReceivedReplicationSeqNumber_;
        FABRIC_SEQUENCE_NUMBER lastAppliedReplicationSeqNumber_;
        bool isInBuild_;
        FABRIC_SEQUENCE_NUMBER lastReceivedCopySeqNumber_;
        FABRIC_SEQUENCE_NUMBER lastAppliedCopySeqNumber_;
        RemoteReplicatorAcknowledgementStatus remoteReplicatorAckStatus_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::RemoteReplicatorStatus);
