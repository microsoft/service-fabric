// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "PlacementReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class NodeEntry;
        class PlacementReplica;

        class ReplicaPlacement
        {
            DENY_COPY_ASSIGNMENT(ReplicaPlacement);

        public:
            ReplicaPlacement();
            ReplicaPlacement(ReplicaPlacement && other);
            ReplicaPlacement(ReplicaPlacement const& other);
            ReplicaPlacement & operator = (ReplicaPlacement && other);

            __declspec (property(get = get_Data)) std::map<NodeEntry const*, PlacementReplicaSet> const& Data;
            std::map<NodeEntry const*, PlacementReplicaSet> const& get_Data() const { return replicaPlacement_; }

            __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
            size_t get_ReplicaCount() const { return replicaCount_; }

            void Add(NodeEntry const* node, PlacementReplica const* replica);
            void Delete(NodeEntry const* node, PlacementReplica const* replica);

            void Clear();

            bool HasReplicaOnNode(
                NodeEntry const* node,
                bool includePrimary,
                bool includeSecondary,
                PlacementReplica const* replicaToExclude = nullptr) const; //whether if there is primary or secondary replica on this node

        private:
            size_t replicaCount_;
            std::map<NodeEntry const*, PlacementReplicaSet> replicaPlacement_;
        };
    }
}
