// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"
#include "ReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Movement;
        class ServicePackageEntry;
        class NodeEntry;
        class NodeMetrics;
        class PlacementReplica;

        class ServicePackagePlacement : public LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>
        {
            DENY_COPY_ASSIGNMENT(ServicePackagePlacement);
        public:
            ServicePackagePlacement();
            ServicePackagePlacement(ServicePackagePlacement && other);
            ServicePackagePlacement(ServicePackagePlacement const& other);
            ServicePackagePlacement & operator = (ServicePackagePlacement && other);

            ServicePackagePlacement(ServicePackagePlacement const* basePlacement);

            // Hiding the original operator so that we can initialize LazyMap properly
            virtual LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> const& operator[](NodeEntry const* key) const;
            virtual LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & operator[](NodeEntry const* key);

            void ChangeMovement(Movement const& oldMovement,
                Movement const& newMovement,
                NodeMetrics & nodeChanges,
                NodeMetrics & nodeMovingInChanges);

            //this does not change the node replica count because we actually set the initial counts using set count to node method
            //this is needed because we might not have all the replicas of the SP in the closure
            void AddReplicaToNode(ServicePackageEntry const*, NodeEntry const*, PlacementReplica const*);
            void SetCountToNode(ServicePackageEntry const*, NodeEntry const*, int count);

            bool HasServicePackageOnNode(ServicePackageEntry const*, NodeEntry const*, bool inHighestBase = false) const;
        private:
            void AddReplica(LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> &,
                NodeEntry const*,
                ServicePackageEntry const*,
                PlacementReplica const*,
                NodeMetrics & nodeChanges,
                NodeMetrics & nodeMovingInChanges);
            void DeleteReplica(LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> &,
                NodeEntry const*,
                ServicePackageEntry const*,
                PlacementReplica const*,
                NodeMetrics & nodeChanges,
                NodeMetrics & nodeMovingInChanges);
        };
    }
}
