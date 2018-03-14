// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicationOperationHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::ReplicationOperation>
            , public Serialization::FabricSerializable
        {
        public:
            ReplicationOperationHeader() :
                firstSequenceNumber_(Constants::InvalidLSN),
                lastSequenceNumber_(Constants::InvalidLSN),
                lastSequenceNumberInBatch_(Constants::InvalidLSN),
                completedSequenceNumber_(Constants::InvalidLSN)
            {}

            ReplicationOperationHeader(
                FABRIC_OPERATION_METADATA operationMetadata,
                FABRIC_EPOCH const & primaryEpoch,
                FABRIC_EPOCH const & operationEpoch,
                std::vector<ULONG> && segmentSizes,
                FABRIC_SEQUENCE_NUMBER firstSequenceNumber,
                FABRIC_SEQUENCE_NUMBER lastSequenceNumber,
                FABRIC_SEQUENCE_NUMBER lastSequenceNumberInBatch,
                std::vector<ULONG> && bufferCounts,
                FABRIC_SEQUENCE_NUMBER completedSequenceNumber)
                : operationMetadata_(operationMetadata)
                , primaryEpoch_(primaryEpoch)
                , operationEpoch_(operationEpoch)
                , segmentSizes_(std::move(segmentSizes))
                , firstSequenceNumber_(firstSequenceNumber)
                , lastSequenceNumber_(lastSequenceNumber)
                , lastSequenceNumberInBatch_(lastSequenceNumberInBatch)
                , bufferCounts_(std::move(bufferCounts))
                , completedSequenceNumber_(completedSequenceNumber)
            {
            }

            __declspec(property(get=get_OperationMetadata)) FABRIC_OPERATION_METADATA OperationMetadata;
            FABRIC_OPERATION_METADATA get_OperationMetadata() const { return operationMetadata_; }

            __declspec(property(get=get_PrimaryEpoch)) FABRIC_EPOCH const & PrimaryEpoch;
            FABRIC_EPOCH const & get_PrimaryEpoch() const { return primaryEpoch_; }

            __declspec(property(get=get_OperationEpoch)) FABRIC_EPOCH const & OperationEpoch;
            FABRIC_EPOCH const & get_OperationEpoch() const { return operationEpoch_; }

            __declspec(property(get=get_SegmentSizes)) std::vector<ULONG> const & SegmentSizes;
            std::vector<ULONG> const & get_SegmentSizes() const { return segmentSizes_; }

            __declspec(property(get=get_BufferCounts)) std::vector<ULONG> const & BufferCounts;
            std::vector<ULONG> const & get_BufferCounts() const { return bufferCounts_; }

            __declspec(property(get=get_FirstSequenceNumber)) FABRIC_SEQUENCE_NUMBER FirstSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_FirstSequenceNumber() const { return firstSequenceNumber_; }

            __declspec(property(get=get_LastSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastSequenceNumber() const { return lastSequenceNumber_; }

            __declspec(property(get=get_LastSequenceNumberInBatch)) FABRIC_SEQUENCE_NUMBER LastSequenceNumberInBatch;
            FABRIC_SEQUENCE_NUMBER get_LastSequenceNumberInBatch() const { return lastSequenceNumberInBatch_; }

            __declspec(property(get=get_CompletedSequenceNumber)) FABRIC_SEQUENCE_NUMBER CompletedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_CompletedSequenceNumber() const { return completedSequenceNumber_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write("{0} (PrimaryEpoch = {1}, OperationEpoch = {2}, Sizes:", operationMetadata_, primaryEpoch_, operationEpoch_);

                for (ULONG size : segmentSizes_)
                {
                    w.Write(" {0}", size);
                }

                w.Write(") firstSequenceNumber = {0}, lastSequenceNumber = {1}, lastSequenceNumberInBatch = {2}, BufferCounts: ", firstSequenceNumber_, lastSequenceNumber_, lastSequenceNumberInBatch_);

                for (ULONG count : bufferCounts_)
                {
                    w.Write(" {0}", count);
                }
            }

            FABRIC_FIELDS_09(
                operationMetadata_, 
                primaryEpoch_, 
                operationEpoch_, 
                segmentSizes_, 
                firstSequenceNumber_, 
                lastSequenceNumber_, 
                lastSequenceNumberInBatch_, 
                bufferCounts_,
                completedSequenceNumber_);

        private:
            FABRIC_OPERATION_METADATA operationMetadata_;
            FABRIC_EPOCH primaryEpoch_;
            FABRIC_EPOCH operationEpoch_;
            std::vector<ULONG> segmentSizes_;
            FABRIC_SEQUENCE_NUMBER firstSequenceNumber_;
            FABRIC_SEQUENCE_NUMBER lastSequenceNumber_;
            FABRIC_SEQUENCE_NUMBER lastSequenceNumberInBatch_;
            std::vector<ULONG> bufferCounts_;
            FABRIC_SEQUENCE_NUMBER completedSequenceNumber_;
        };
    }
}
