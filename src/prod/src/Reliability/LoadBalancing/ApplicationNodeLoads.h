// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "LoadEntry.h"
#include "NodeMetrics.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationEntry;

        /*-----------------------------------------------------------------------------------------
            Mapping from application to load for each node.
            This load is indexed by capacityIndex (0 to global metric count).
        -----------------------------------------------------------------------------------------*/
        class ApplicationNodeLoads : public LazyMap<ApplicationEntry const*, NodeMetrics>
        {
            DENY_COPY_ASSIGNMENT(ApplicationNodeLoads);
            DEFAULT_COPY_CONSTRUCTOR(ApplicationNodeLoads);
        public:
            ApplicationNodeLoads(size_t globalMetricCount, size_t globalMetricStartIndex);
            ApplicationNodeLoads(ApplicationNodeLoads && other);
            ApplicationNodeLoads(ApplicationNodeLoads const* base);
            ApplicationNodeLoads & operator = (ApplicationNodeLoads && other);

            // Hiding the original operator so that we can initialize NodeMetrics properly
            virtual NodeMetrics const& operator[](ApplicationEntry const* key) const;
            virtual NodeMetrics & operator[](ApplicationEntry const* key);

            void SetNodeLoadsForApplication(ApplicationEntry const* app, std::map<NodeEntry const*, LoadEntry> && data);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
        private:
            size_t globalMetricCount_;
            size_t globalMetricStartIndex_;
        };
    }
}
