// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        // The class of a configuration, it contains only iterators to 
        // the replicas within FailoverUnit.
        // Note that a configuration should have a primary if it is not empty
        struct FailoverUnitConfiguration
        {
        public:

            friend class FailoverUnit;

            // properties
            __declspec (property(get=get_Primary)) ReplicaConstIterator Primary;
            ReplicaConstIterator get_Primary() const;

            __declspec (property(get=get_Replicas)) std::vector<ReplicaConstIterator> const& Replicas;
            std::vector<ReplicaConstIterator> const& get_Replicas() const { return replicas_; }

            __declspec (property(get=get_IsPrimaryAvailable)) bool IsPrimaryAvailable;
            bool get_IsPrimaryAvailable() const { return !IsEmpty && primary_->IsAvailable; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return (replicas_.size() == 0); }

            __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
            size_t get_ReplicaCount() const { return replicas_.size(); }

            __declspec (property(get=get_SecondaryCount)) size_t SecondaryCount;
            size_t get_SecondaryCount() const { return IsEmpty ? 0 : ReplicaCount - 1; }

            __declspec (property(get=get_UpCount)) size_t UpCount;
            size_t get_UpCount() const;

            __declspec (property(get=get_OfflineCount)) size_t OfflineCount;
            size_t get_OfflineCount() const;

            __declspec (property(get=get_AvailableCount)) size_t AvailableCount;
            size_t get_AvailableCount() const;

            __declspec (property(get=get_DownSecondaryCount)) size_t DownSecondaryCount;
            size_t get_DownSecondaryCount() const;

            __declspec (property(get=get_StableReplicaCount)) size_t StableReplicaCount;
            size_t get_StableReplicaCount() const;

			__declspec (property(get = get_StandByReplicaCount)) size_t StandByReplicaCount;
			size_t get_StandByReplicaCount() const;

			__declspec (property(get=get_DroppedCount)) size_t DroppedCount;
            size_t get_DroppedCount() const;

            __declspec (property(get=get_DeletedCount)) size_t DeletedCount;
            size_t get_DeletedCount() const;

            __declspec (property(get=get_WriteQuorumSize)) size_t WriteQuorumSize;
            size_t get_WriteQuorumSize() const { return (ReplicaCount / 2 + 1);}

            __declspec (property(get=get_ReadQuorumSize)) size_t ReadQuorumSize;
            size_t get_ReadQuorumSize() const { return (ReplicaCount + 1) / 2;}

            __declspec (property(get=get_IsReadQuorumLost)) bool IsReadQuorumLost;
            bool get_IsReadQuorumLost() const { return DroppedCount >= WriteQuorumSize; }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            bool isCurrentConfiguration_;
            std::vector<ReplicaConstIterator> replicas_;
            ReplicaConstIterator primary_;
            ReplicaConstIterator end_;

            // Initialize an empty configuration
            FailoverUnitConfiguration(bool isCurrentConfiguration, ReplicaConstIterator endIt);

            ReplicaRole::Enum GetRole(Replica const& replica);
            bool InConfiguration(Replica const& replica);
            void AddReplica(ReplicaConstIterator replicaIt);
            void RemoveReplica(ReplicaConstIterator replicaIt);
            void Clear();
        };
    }
}
