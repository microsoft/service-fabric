// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class RecoverPartitionRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        RecoverPartitionRequestMessageBody()
        {
        }

        RecoverPartitionRequestMessageBody(Reliability::PartitionId const& partitionId)
            : partitionId_(partitionId)
        {
        }

        __declspec (property(get=get_PartitionId)) Reliability::PartitionId const& PartitionId;
        Reliability::PartitionId const& get_PartitionId() const { return partitionId_; }

        FABRIC_FIELDS_01(partitionId_);

    private:

        Reliability::PartitionId partitionId_;
    };
}
