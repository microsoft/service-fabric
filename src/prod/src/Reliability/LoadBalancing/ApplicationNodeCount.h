// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"
#include "Movement.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementReplica;
        class ApplicationEntry;
        class NodeEntry;

        class ApplicationNodeCount : public LazyMap < ApplicationEntry const*, std::map<NodeEntry const*, size_t>>
        {
            DENY_COPY_ASSIGNMENT(ApplicationNodeCount);
        public:
            ApplicationNodeCount();
            ApplicationNodeCount(ApplicationNodeCount && other);
            ApplicationNodeCount(ApplicationNodeCount const& other);
            ApplicationNodeCount(ApplicationNodeCount const* baseNodeCounts);

            ApplicationNodeCount & operator = (ApplicationNodeCount && other);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
            void IncreaseCount(ApplicationEntry const* app, NodeEntry const* node);
            void SetCount(ApplicationEntry const* app, NodeEntry const* node, size_t count);
        private:
            void AddReplica(std::map<NodeEntry const*, size_t> & nodeCounts, NodeEntry const* node, PlacementReplica const* replica);
            void RemoveReplica(std::map<NodeEntry const*, size_t> & nodeCounts, NodeEntry const* node, PlacementReplica const* replica);
        };
    }
}
