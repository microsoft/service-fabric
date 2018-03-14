// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    template <Common::TraceTaskCodes::Enum TraceType>
    class PartitionedReplicaTraceComponent : public Common::TextTraceComponent<TraceType>
    {
    public:
        explicit PartitionedReplicaTraceComponent(PartitionedReplicaTraceComponent const & other)
            : partitionedReplicaId_(other.partitionedReplicaId_)
        {
        }

        explicit PartitionedReplicaTraceComponent(PartitionedReplicaTraceComponent && other)
            : partitionedReplicaId_(std::move(other.partitionedReplicaId_))
        {
        }

        explicit PartitionedReplicaTraceComponent(::Store::PartitionedReplicaId const & partitionedReplicaId) 
            : partitionedReplicaId_(partitionedReplicaId)
        {
        }

        explicit PartitionedReplicaTraceComponent(Common::Guid const & partitionId, ::FABRIC_REPLICA_ID replicaId)
            : partitionedReplicaId_(::Store::PartitionedReplicaId(partitionId, replicaId))
        {
        }

        virtual ~PartitionedReplicaTraceComponent() { }

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
        __declspec(property(get=get_PartitionedReplicaId)) ::Store::PartitionedReplicaId const & PartitionedReplicaId;

        std::wstring const & get_TraceId() const { return partitionedReplicaId_.TraceId; }
        Common::Guid const & get_PartitionId() const { return partitionedReplicaId_.PartitionId; }
        ::FABRIC_REPLICA_ID const & get_ReplicaId() const { return partitionedReplicaId_.ReplicaId; }
        ::Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return partitionedReplicaId_; }

    protected:

        void UpdateTraceId(Common::Guid const & partitionId, ::FABRIC_REPLICA_ID replicaId)
        {
            partitionedReplicaId_ = ::Store::PartitionedReplicaId(partitionId, replicaId);
        }

    private:
        ::Store::PartitionedReplicaId partitionedReplicaId_;
    };
}
