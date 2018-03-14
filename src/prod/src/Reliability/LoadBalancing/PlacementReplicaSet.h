// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementReplica;

        // Used for comparing pointers to PlacementReplica objects. Objects will be compared based on their index.
        // In this way we get a stable order of iteration over sets of PlacementReplica const*
        typedef struct PlacementReplicaPointerCompare
        {
            bool operator() (PlacementReplica const * lhs, PlacementReplica const * rhs) const;
        } PlacementReplicaPointerCompare;

        typedef std::set<PlacementReplica const *, PlacementReplicaPointerCompare> PlacementReplicaSet;
    }
}
