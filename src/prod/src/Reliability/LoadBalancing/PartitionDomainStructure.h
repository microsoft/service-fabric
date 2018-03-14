// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        struct PartitionDomainData
        {
            PartitionDomainData()
                : ReplicaCount(0), Replicas()
            {                    
            }

            size_t ReplicaCount;
            std::vector<PlacementReplica const*> Replicas;
        };
            
        typedef Common::Tree<PartitionDomainData> PartitionDomainTree;

        class PartitionDomainStructure : public LazyMap<PartitionEntry const*, PartitionDomainTree>
        {
            DENY_COPY_ASSIGNMENT(PartitionDomainStructure);
        public:
            explicit PartitionDomainStructure(bool isFaultDomain);
            PartitionDomainStructure(PartitionDomainStructure && other);
            PartitionDomainStructure(PartitionDomainStructure const& other);
            PartitionDomainStructure & operator = (PartitionDomainStructure && other);

            PartitionDomainStructure(PartitionDomainStructure const* baseDomainStructure, bool isFaultDomain);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);

        private:
            static void AddReplica(PartitionDomainTree & tree, Common::TreeNodeIndex const& toDomain, PlacementReplica const* replica);
            static void DeleteReplica(PartitionDomainTree & tree, Common::TreeNodeIndex const& fromDomain, PlacementReplica const* replica);
            static void SwapReplica(PartitionDomainTree & tree, Common::TreeNodeIndex const& index1, PlacementReplica const* replica1, Common::TreeNodeIndex const& index2, PlacementReplica const* replica2);
            bool isFaultDomain_;
        };
    }
}
