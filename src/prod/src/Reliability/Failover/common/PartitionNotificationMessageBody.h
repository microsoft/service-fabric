// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class PartitionNotificationMessageBody : public Serialization::FabricSerializable
    {
    public:
        PartitionNotificationMessageBody()
        {
        }

        PartitionNotificationMessageBody(std::vector<PartitionId> && disabledPartitions, uint64 sequenceNumber)
            : disabledPartitions_(std::move(disabledPartitions)), sequenceNumber_(sequenceNumber)
        {
        }

        __declspec(property(get=get_DisabledPartitions)) std::vector<PartitionId> const& DisabledPartitions;
        std::vector<PartitionId> const& get_DisabledPartitions() const { return disabledPartitions_; }

        __declspec(property(get=get_SequenceNumber)) uint64 SequenceNumber;
        uint64 get_SequenceNumber() const { return sequenceNumber_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.WriteLine("SeqNum: {0}", sequenceNumber_);

            for (PartitionId const& disabledPartition : disabledPartitions_)
            {
                w.WriteLine("{0} ", disabledPartition);
            }
        }

        FABRIC_FIELDS_02(disabledPartitions_, sequenceNumber_);

    private:
        std::vector<PartitionId> disabledPartitions_;
        uint64 sequenceNumber_;
    };
}
