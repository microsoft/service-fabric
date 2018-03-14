// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationEntry;
        class ApplicationPlacement : public LazyMap < ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet> >
        {
            DENY_COPY_ASSIGNMENT(ApplicationPlacement);
        public:
            ApplicationPlacement();
            ApplicationPlacement(ApplicationPlacement && other);
            ApplicationPlacement(ApplicationPlacement const& other);
            ApplicationPlacement & operator = (ApplicationPlacement && other);

            ApplicationPlacement(ApplicationPlacement const* basePlacement);

            // Hiding the original operator so that we can initialize LazyMap properly
            virtual LazyMap<NodeEntry const*, ReplicaSet> const& operator[](ApplicationEntry const* key) const;
            virtual LazyMap<NodeEntry const*, ReplicaSet> & operator[](ApplicationEntry const* key);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
        private:
            void AddReplica(LazyMap<NodeEntry const*, ReplicaSet> & placement, NodeEntry const* node, PlacementReplica const* replica);
            void DeleteReplica(LazyMap<NodeEntry const*, ReplicaSet> & placement, NodeEntry const* node, PlacementReplica const* replica);
        };
    }
}
