// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "LazyMap.h"
#include "LoadEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class NodeEntry;
        class ServiceEntry;
        class Movement;
        class PartitionEntry;

        class NodeMetrics : public LazyMap<NodeEntry const*, LoadEntry>
        {
            DENY_COPY_ASSIGNMENT(NodeMetrics);
        public:
            explicit NodeMetrics(size_t totalMetricCount, size_t metricStartIndex, bool useAllMetrics);
            NodeMetrics(size_t totalMetricCount,
                std::map<NodeEntry const*, LoadEntry> && data,
                bool useAllMetrics,
                size_t metricStartIndex);
            NodeMetrics(NodeMetrics && other);
            NodeMetrics(NodeMetrics const& other);
            NodeMetrics & operator = (NodeMetrics && other);

            NodeMetrics(NodeMetrics const* baseChanges);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
            void ChangeMovingIn(Movement const& oldMovement, Movement const& newMovement);

            void UpdateWithOldMovement(Movement const& oldMovement);
            void UpdateWithNewMovement(Movement const& newMovement);

            // Used to add and delete Service Package loads
            void AddLoad(NodeEntry const* node, LoadEntry const& load);
            void DeleteLoad(NodeEntry const* node, LoadEntry const& load);

        private:
            size_t totalMetricCount_;
            // If true, then each LoadEntry will contain both local and global metrics.
            // If false, then each LoadEntry will contain only global metrics.
            // This is needed because application loads (node and reserved) use only global metrics.
            bool useAllMetrics_;

            // When we use all metrics (for node loads and such) metricStartIndex_ will be zero.
            // When we use only global metrics (for application loads) metricStartIndex_ will be first index of global metrics.
            // In this second case NodeMetrics class will record only global values so subtraction is necessary not to go out of bounds.
            size_t metricStartIndex_;

            void UpdateWithOldMoveIn(Movement const& oldMovement);
            void UpdateWithNewMoveIn(Movement const& newMovement);

            void AddLoad(NodeEntry const* node, ServiceEntry const& service, LoadEntry const& loadChange);

            void ApplyPositiveDifference(
                NodeEntry const* node,
                ServiceEntry const& service,
                LoadEntry const& loadChange1,
                LoadEntry const& loadChange2,
                bool removeDifference);

            void DeleteLoad(NodeEntry const* node, ServiceEntry const& service, LoadEntry const& loadChange);
            void MoveLoad(NodeEntry const* sourceNode, NodeEntry const* targetNode, ServiceEntry const& service, LoadEntry const& loadChange);

            LoadEntry const* StandByReplicaLoadOnTargetNode(NodeEntry const* node, PartitionEntry const* partition);
            LoadEntry const* ExistingReplicaLoadOnTargetNode(NodeEntry const* node, PartitionEntry const* partition);
            void AddSbLoad(NodeEntry const* node, PartitionEntry const* partition);
            void DeleteSbLoad(NodeEntry const* node, PartitionEntry const* partition);
        };

    }
}
