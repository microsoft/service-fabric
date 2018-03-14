// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicatorQueueStatus
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ReplicatorQueueStatus();

        ReplicatorQueueStatus(
            uint queueUtilizationPercentage,
            LONGLONG queueMemorySize,
            FABRIC_SEQUENCE_NUMBER firstSequenceNumber,
            FABRIC_SEQUENCE_NUMBER completedSequenceNumber,
            FABRIC_SEQUENCE_NUMBER committedSequenceNumber,
            FABRIC_SEQUENCE_NUMBER lastSequenceNumber);

        ReplicatorQueueStatus(ReplicatorQueueStatus && other);

        ReplicatorQueueStatus const & operator = (ReplicatorQueueStatus && other);
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICATOR_QUEUE_STATUS & publicResult) const ;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICATOR_QUEUE_STATUS & publicResult) ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_06(
            queueUtilizationPercentage_,
            queueMemorySize_,
            firstSequenceNumber_, 
            completedSequenceNumber_, 
            committedSequenceNumber_, 
            lastSequenceNumber_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::QueueUtilizationPercentage, queueUtilizationPercentage_)
            SERIALIZABLE_PROPERTY(Constants::QueueMemorySize, queueMemorySize_)
            SERIALIZABLE_PROPERTY(Constants::FirstSequenceNumber, firstSequenceNumber_)
            SERIALIZABLE_PROPERTY(Constants::CompletedSequenceNumber, completedSequenceNumber_)
            SERIALIZABLE_PROPERTY(Constants::CommittedSequenceNumber, committedSequenceNumber_)
            SERIALIZABLE_PROPERTY(Constants::LastSequenceNumber, lastSequenceNumber_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        uint queueUtilizationPercentage_;
        LONGLONG queueMemorySize_;
        FABRIC_SEQUENCE_NUMBER firstSequenceNumber_;
        FABRIC_SEQUENCE_NUMBER completedSequenceNumber_;
        FABRIC_SEQUENCE_NUMBER committedSequenceNumber_;
        FABRIC_SEQUENCE_NUMBER lastSequenceNumber_;
    };
}
