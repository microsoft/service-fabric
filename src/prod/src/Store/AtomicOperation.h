// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class AtomicOperation : public Serialization::FabricSerializable
    {
    public:
        AtomicOperation();

        AtomicOperation(
            Common::Guid const & activityId,
            std::vector<ReplicationOperation> && replicationOperations,
            ::FABRIC_SEQUENCE_NUMBER lastQuorumAck);

        __declspec(property(get=get_ActivityId)) Common::Guid const & ActivityId;
        __declspec(property(get=get_LastQuorumAcked)) ::FABRIC_SEQUENCE_NUMBER const LastQuorumAcked;

        Common::Guid const & get_ActivityId() const { return activityId_; }
        ::FABRIC_SEQUENCE_NUMBER const get_LastQuorumAcked() const { return lastQuorumAcked_; }

        Common::ErrorCode TakeReplicationOperations(__out std::vector<ReplicationOperation> &);

        FABRIC_FIELDS_03(activityId_, replicationOperations_, lastQuorumAcked_);

    private:
        Common::Guid activityId_;
        std::vector<ReplicationOperation> replicationOperations_;
        ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked_;
    };
}
