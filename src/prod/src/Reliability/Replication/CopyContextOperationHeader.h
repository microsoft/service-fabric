// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class CopyContextOperationHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::CopyContextOperation>
            , public Serialization::FabricSerializable
        {
        public:
            CopyContextOperationHeader() {}

            CopyContextOperationHeader(
                FABRIC_OPERATION_METADATA const & operationMetadata,
                std::vector<ULONG> && segmentSizes,
                bool isLast)
                :   operationMetadata_(operationMetadata),
                    segmentSizes_(std::move(segmentSizes)),
                    isLast_(isLast)
            {
            }

            __declspec(property(get=get_OperationMetadata)) FABRIC_OPERATION_METADATA const & OperationMetadata;
            FABRIC_OPERATION_METADATA const & get_OperationMetadata() const { return operationMetadata_; }

            __declspec(property(get=get_SegmentSizes)) std::vector<ULONG> const & SegmentSizes;
            std::vector<ULONG> const & get_SegmentSizes() const { return segmentSizes_; }

            __declspec(property(get=get_IsLast)) bool IsLast;
            bool get_IsLast() const { return isLast_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write(operationMetadata_);
                
                if (isLast_)
                {
                    w.Write(" (last)");
                }

                w.Write(" (Sizes:");

                for (ULONG size : segmentSizes_)
                {
                    w.Write(" {0}", size);
                }

                w.Write(")");
            }

            FABRIC_FIELDS_03(operationMetadata_, segmentSizes_, isLast_);

        private:
            FABRIC_OPERATION_METADATA operationMetadata_;
            std::vector<ULONG> segmentSizes_;
            bool isLast_;
        };
    }
}
