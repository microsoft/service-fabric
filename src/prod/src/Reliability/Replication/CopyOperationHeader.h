// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class CopyOperationHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::CopyOperation>
            , public Serialization::FabricSerializable
        {
        public:
            CopyOperationHeader() {}

            CopyOperationHeader(
                FABRIC_REPLICA_ID replicaId,
                FABRIC_EPOCH const & primaryEpoch,
                FABRIC_OPERATION_METADATA operationMetadata,
                std::vector<ULONG> && segmentSizes,
                bool isLast) 
                :   replicaId_(replicaId),
                    primaryEpoch_(primaryEpoch),
                    operationMetadata_(operationMetadata),
                    segmentSizes_(std::move(segmentSizes)),
                    isLast_(isLast)
            {
            }

            __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec(property(get=get_OperationMetadata)) FABRIC_OPERATION_METADATA const & OperationMetadata;
            FABRIC_OPERATION_METADATA const & get_OperationMetadata() const { return operationMetadata_; }

            __declspec(property(get=get_PrimaryEpoch)) FABRIC_EPOCH const & PrimaryEpoch;
            FABRIC_EPOCH const & get_PrimaryEpoch() const { return primaryEpoch_; }

            __declspec(property(get=get_SegmentSizes)) std::vector<ULONG> const & SegmentSizes;
            std::vector<ULONG> const & get_SegmentSizes() const { return segmentSizes_; }
                        
            __declspec(property(get=get_IsLast)) bool IsLast;
            bool get_IsLast() const { return isLast_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write(
                    "{0},{1}: {2} {3} (Sizes:", 
                    replicaId_, 
                    primaryEpoch_,
                    operationMetadata_, 
                    isLast_ ? L" (last)" : L"");

                for (ULONG size : segmentSizes_)
                {
                    w.Write(" {0}", size);
                }

                w.Write(")");
            }

            FABRIC_FIELDS_05(replicaId_, primaryEpoch_, operationMetadata_, segmentSizes_, isLast_);

        private:
            FABRIC_REPLICA_ID replicaId_;
            FABRIC_EPOCH primaryEpoch_;
            FABRIC_OPERATION_METADATA operationMetadata_;
            std::vector<ULONG> segmentSizes_;
            bool isLast_;
        };
    }
}
