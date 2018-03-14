// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"
#include "ReplicaPlacement.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Movement;
        class PartitionEntry;

        class PartitionPlacement : public LazyMap<PartitionEntry const*, ReplicaPlacement>
        {
            DENY_COPY_ASSIGNMENT(PartitionPlacement);
        public:
            PartitionPlacement();
            PartitionPlacement(PartitionPlacement && other);
            PartitionPlacement(PartitionPlacement const& other);
            PartitionPlacement & operator = (PartitionPlacement && other);

            PartitionPlacement(PartitionPlacement const* basePlacement);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
        };
    }
}
