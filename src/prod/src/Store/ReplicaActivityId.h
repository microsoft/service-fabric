// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicaActivityId
    {
        DEFAULT_COPY_ASSIGNMENT(ReplicaActivityId)

    public:
        ReplicaActivityId(Store::PartitionedReplicaId const &, Common::ActivityId const &);
        explicit ReplicaActivityId(ReplicaActivityId const &);
        explicit ReplicaActivityId(ReplicaActivityId &&);

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
        __declspec(property(get=get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;

        std::wstring const & get_TraceId() const { return traceId_; }
        Common::Guid const & get_PartitionId() const { return partitionedReplicaId_.PartitionId; }
        ::FABRIC_REPLICA_ID const & get_ReplicaId() const { return partitionedReplicaId_.ReplicaId; }
        Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return partitionedReplicaId_; }
        Common::ActivityId const & get_ActivityId() const { return activityId_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

    private:
        std::wstring traceId_;
        Store::PartitionedReplicaId partitionedReplicaId_;
        Common::ActivityId activityId_;
    };
}
