// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReplicaHealthId : public Serialization::FabricSerializable
        {
        public:
            ReplicaHealthId();

            ReplicaHealthId(
                PartitionHealthId partitionId,
                FABRIC_REPLICA_ID replicaId);

            ReplicaHealthId(ReplicaHealthId const & other);
            ReplicaHealthId & operator = (ReplicaHealthId const & other);

            ReplicaHealthId(ReplicaHealthId && other);
            ReplicaHealthId & operator = (ReplicaHealthId && other);
            
            ~ReplicaHealthId();
                        
            __declspec (property(get=get_PartitionId)) PartitionHealthId PartitionId;
            PartitionHealthId get_PartitionId() const { return partitionId_; }
                        
            __declspec (property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }
            
            bool operator == (ReplicaHealthId const & other) const;
            bool operator != (ReplicaHealthId const & other) const;
            bool operator < (ReplicaHealthId const & other) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_02(partitionId_, replicaId_);
            
            static bool TryParse(
                std::wstring const & entityIdStr,
                __inout PartitionHealthId & partitionId,
                __inout FABRIC_REPLICA_ID & replicaId);
            
        private:
            PartitionHealthId partitionId_;
            FABRIC_REPLICA_ID replicaId_;
        };
    }
}
