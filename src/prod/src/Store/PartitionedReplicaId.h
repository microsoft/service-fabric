// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class PartitionedReplicaId
    {
       DEFAULT_COPY_ASSIGNMENT(PartitionedReplicaId)

    public:
        PartitionedReplicaId(Common::Guid const &, ::FABRIC_REPLICA_ID);
        explicit PartitionedReplicaId(PartitionedReplicaId const &);
        explicit PartitionedReplicaId(PartitionedReplicaId &&);

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;

        std::wstring const & get_TraceId() const { return traceId_; }
        Common::Guid const & get_PartitionId() const { return partitionId_; }
        ::FABRIC_REPLICA_ID const & get_ReplicaId() const { return replicaId_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

    private:
        std::wstring traceId_;
        Common::Guid partitionId_;
        ::FABRIC_REPLICA_ID replicaId_;
    };
}
