// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PartitionTargetHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::PartitionTarget>
        , public Serialization::FabricSerializable
    {
    public:
        PartitionTargetHeader() : partitionId_()
        {
        }

        explicit PartitionTargetHeader(Common::Guid const & partitionId) : partitionId_(partitionId)
        { 
        }

        __declspec (property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() { return partitionId_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << "PartitionTargetHeader( " << partitionId_ << " )";
        }

        FABRIC_FIELDS_01(partitionId_);

    private:
        Common::Guid partitionId_;
    };
}
