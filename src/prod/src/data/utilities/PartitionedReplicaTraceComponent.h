// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // Class that must be inherited by other classes in order to be able to trace events using replica and partition identifiers
        //
        template <Common::TraceTaskCodes::Enum TraceType>
        class PartitionedReplicaTraceComponent 
            : public Common::TextTraceComponent<TraceType>
        {

        public:
            PartitionedReplicaTraceComponent(__in PartitionedReplicaTraceComponent const & other)
                : partitionedReplicaId_(other.partitionedReplicaId_)
            {
            }

            PartitionedReplicaTraceComponent(__in PartitionedReplicaTraceComponent && other)
                : partitionedReplicaId_(std::move(other.partitionedReplicaId_))
            {
            }

            PartitionedReplicaTraceComponent(__in PartitionedReplicaId const & partitionedReplicaId) 
                : partitionedReplicaId_(&partitionedReplicaId)
            {
            }

            __declspec(property(get=get_TraceId)) Common::StringLiteral TraceId;
            Common::StringLiteral get_TraceId() const 
            { 
                return partitionedReplicaId_->TraceId; 
            }

            __declspec(property(get=get_PartitionId)) KGuid const & PartitionId;
            KGuid const & get_PartitionId() const 
            { 
                return partitionedReplicaId_->PartitionId; 
            }

            __declspec(property(get=get_TracePartitionId)) Common::Guid const & TracePartitionId;
            Common::Guid const & get_TracePartitionId() const 
            { 
                return partitionedReplicaId_->TracePartitionId; 
            }

            __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
            ::FABRIC_REPLICA_ID const & get_ReplicaId() const 
            { 
                return partitionedReplicaId_->ReplicaId; 
            }

            __declspec(property(get=get_PartitionedReplicaId)) PartitionedReplicaId::CSPtr const & PartitionedReplicaIdentifier;
            PartitionedReplicaId::CSPtr const & get_PartitionedReplicaId() const 
            { 
                return partitionedReplicaId_; 
            }

        private:
            PartitionedReplicaTraceComponent()
                : partitionedReplicaId_()
            {
            }

            PartitionedReplicaId::CSPtr partitionedReplicaId_;
        };
    }
}
