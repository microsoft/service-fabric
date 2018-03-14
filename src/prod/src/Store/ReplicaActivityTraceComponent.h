// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    template <Common::TraceTaskCodes::Enum TraceType>
    class ReplicaActivityTraceComponent : public Common::TextTraceComponent<TraceType>
    {
        DEFAULT_COPY_ASSIGNMENT(ReplicaActivityTraceComponent)

    public:
        explicit ReplicaActivityTraceComponent(ReplicaActivityTraceComponent const & other)
            : replicaActivityId_(other.replicaActivityId_)
        {
        }

        explicit ReplicaActivityTraceComponent(ReplicaActivityTraceComponent && other)
            : replicaActivityId_(std::move(other.replicaActivityId_))
        {
        }

        explicit ReplicaActivityTraceComponent(Store::ReplicaActivityId const & replicaActivityId) 
            : replicaActivityId_(replicaActivityId)
        {
        }

        explicit ReplicaActivityTraceComponent(Store::PartitionedReplicaId const & partitionedReplicaId, Common::ActivityId const & activityId) 
            : replicaActivityId_(Store::ReplicaActivityId(partitionedReplicaId, activityId))
        {
        }
        
        ReplicaActivityTraceComponent & operator = (ReplicaActivityTraceComponent && other)
        {
            if (&other != this)
            {
                replicaActivityId_ = std::move(other.replicaActivityId_);
            }
            return *this;
        }

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
        __declspec(property(get=get_PartitionedReplicaId)) Store::ReplicaActivityId const & ReplicaActivityId;

        std::wstring const & get_TraceId() const { return replicaActivityId_.TraceId; }
        Common::Guid const & get_PartitionId() const { return replicaActivityId_.PartitionId; }
        ::FABRIC_REPLICA_ID const & get_ReplicaId() const { return replicaActivityId_.ReplicaId; }
        Common::ActivityId const & get_ActivityId() const { return replicaActivityId_.ActivityId; }
        Store::ReplicaActivityId const & get_PartitionedReplicaId() const { return replicaActivityId_; }

        void ReInitializeTracing(Store::ReplicaActivityId const & replicaActivityId)
        {
            replicaActivityId_ = replicaActivityId;
        }

    private:
        Store::ReplicaActivityId replicaActivityId_;
    };
}
