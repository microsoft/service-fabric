// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ReplicaSet.h"
#include "LoadEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationEntry;
        class ApplicationNodeLoads;
        class ApplicationNodeCount;

        /*-----------------------------------------------------------------------------------------
            Mapping from node to reserved load for all applications that are present on that node.
            Reserved load is the load that is not actually used by application:
                - Application has reservation capacity of 100.
                - Application load is 70 on the node.
                - Reserved load is 30 in that case.
            We need to keep track of this load in order not to give it to other applications.
            Reserved load is indexed by capacityIndex (0 to global metric count).
         ----------------------------------------------------------------------------------------*/
        class ApplicationReservedLoad : public LazyMap<NodeEntry const*, LoadEntry>
        {
            DENY_COPY_ASSIGNMENT(ApplicationReservedLoad);
            DEFAULT_COPY_CONSTRUCTOR(ApplicationReservedLoad);
            
        public:
            ApplicationReservedLoad(size_t globalMetricCount);
            ApplicationReservedLoad(ApplicationReservedLoad && other);
            ApplicationReservedLoad(ApplicationReservedLoad const* baseLoads);
            ApplicationReservedLoad & operator = (ApplicationReservedLoad && other);

            void ChangeMovement(Movement const& oldMovement,
                Movement const& newMovement,
                ApplicationNodeLoads & appLoads,
                ApplicationNodeCount & appNodeCounts);

            void Set(NodeEntry const* node, LoadEntry && reservedLoad);
        private:
            void AddReplica(NodeEntry const* node, PlacementReplica const* replica, LoadEntry const& applicationLoadOnNode);
            void DeleteReplica(NodeEntry const* node, PlacementReplica const* replica, LoadEntry const& applicationLoadOnNode);

            void ChangeMovementInternal(ApplicationEntry const* application,
                Movement const& oldMovement,
                Movement const& newMovement,
                ApplicationNodeLoads & appLoads,
                ApplicationNodeCount & appNodeCounts);

            void UpdateReservedLoad(std::set<NodeEntry const*> nodes,
                ApplicationEntry const* application,
                ApplicationNodeLoads const& appLoads,
                ApplicationNodeCount const& appNodeCounts,
                bool isAdd);

            size_t globalMetricCount_;
        };
    }
}
