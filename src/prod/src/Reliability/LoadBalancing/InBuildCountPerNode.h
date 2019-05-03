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
        class NodeEntry;

        // Tracks the number of InBuild replicas per node for each node that is throttled.
        class InBuildCountPerNode : public LazyMap <NodeEntry const*, uint64_t>
        {
            DENY_COPY_ASSIGNMENT(InBuildCountPerNode);
        public:
            InBuildCountPerNode();
            InBuildCountPerNode(InBuildCountPerNode && other);
            InBuildCountPerNode(InBuildCountPerNode const& other);
            InBuildCountPerNode(InBuildCountPerNode const* baseNodeCounts);

            InBuildCountPerNode & operator = (InBuildCountPerNode && other);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
            void SetCount(NodeEntry const* node, uint64_t count);
        private:
            void IncreaseCount(NodeEntry const* node);
            void DecreaseCount(NodeEntry const* node);
        };
    }
}
