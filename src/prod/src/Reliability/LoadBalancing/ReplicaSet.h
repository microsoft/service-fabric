// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"
#include "PlacementReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementReplica;
        class ServiceEntry;
        class LoadEntry;

        class ReplicaSet
        {
            DENY_COPY_ASSIGNMENT(ReplicaSet);

        public:
            ReplicaSet();
            ReplicaSet(ReplicaSet && other);
            ReplicaSet(ReplicaSet const& other);
            ReplicaSet & operator = (ReplicaSet && other);

            __declspec (property(get = get_Data)) PlacementReplicaSet const& Data;
            PlacementReplicaSet const& get_Data() const { return replicas_; }

            void Add(PlacementReplica const* replica);
            void Delete(PlacementReplica const* replica);

            void Clear();

        private:
            PlacementReplicaSet replicas_;
        };

        class NodeEntry;
        class Movement;

        class NodePlacement : public LazyMap<NodeEntry const*, ReplicaSet>
        {
            DENY_COPY_ASSIGNMENT(NodePlacement);
        public:
            NodePlacement();
            NodePlacement(NodePlacement && other);
            NodePlacement(NodePlacement const& other);
            NodePlacement & operator = (NodePlacement && other);

            NodePlacement(NodePlacement const* basePlacement);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);

            // This method is not removing replicas from the source node when move is initated.
            // It tracks only replicas that are increasing target node load.
            void ChangeMovingIn(Movement const& oldMovement, Movement const& newMovement);

            bool IsMakingPositiveDifference(ServiceEntry const& service, LoadEntry const& loadChange1, LoadEntry const& loadChange2);
        };

    }
}
