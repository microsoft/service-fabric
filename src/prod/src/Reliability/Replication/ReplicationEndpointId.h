// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicationEndpointId
        {
        public:
            ReplicationEndpointId()
                : partitionId_(), 
                  replicaId_(0), 
                  incarnationId_()
            {
            }

            ReplicationEndpointId(
                Common::Guid const & partitionId, 
                ::FABRIC_REPLICA_ID replicaId)
                : partitionId_(partitionId), 
                  replicaId_(replicaId), 
                  incarnationId_(Common::Guid::NewGuid())
            {
            }

            ReplicationEndpointId(
                Common::Guid const & partitionId, 
                ::FABRIC_REPLICA_ID replicaId,
                Common::Guid const & incarnationId)
                : partitionId_(partitionId), 
                  replicaId_(replicaId), 
                  incarnationId_(incarnationId)
            {
            }

            ReplicationEndpointId(ReplicationEndpointId const & other)
                : partitionId_(other.partitionId_),
                  replicaId_(other.replicaId_),
                  incarnationId_(other.incarnationId_)
            {
            }

            __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
            Common::Guid const & get_PartitionId() const { return partitionId_; }

            __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
            ::FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec(property(get=get_IncarnationId)) Common::Guid const & IncarnationId;
            Common::Guid const & get_IncarnationId() const { return incarnationId_; }

            bool operator == (ReplicationEndpointId const & rhs) const;

            bool operator != (ReplicationEndpointId const & rhs) const;

            bool operator < (ReplicationEndpointId const & rhs) const;

            static ReplicationEndpointId FromString(
                std::wstring const & stringEndpointId);

            std::wstring ToString() const;

            bool IsEmpty() const;
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            FABRIC_PRIMITIVE_FIELDS_03(partitionId_, replicaId_, incarnationId_);

        private:
            Common::Guid partitionId_;
            ::FABRIC_REPLICA_ID replicaId_;
            Common::Guid incarnationId_;
        };
    }
}
