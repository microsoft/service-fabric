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
        class ApplicationEntry;
        class Movement;

        class ApplicationTotalLoad : public LazyMap <ApplicationEntry const*, LoadEntry>
        {
            DENY_COPY_ASSIGNMENT(ApplicationTotalLoad);
        public:
            explicit ApplicationTotalLoad(size_t globalMetricCount, size_t totalMetricCount);
            ApplicationTotalLoad(ApplicationTotalLoad && other);
            ApplicationTotalLoad(ApplicationTotalLoad const& other);
            ApplicationTotalLoad & operator = (ApplicationTotalLoad && other);

            ApplicationTotalLoad(ApplicationTotalLoad const* baseAppLoad);

            void ChangeMovement(Movement const& oldMovement, Movement const& newMovement);
            void SetLoad(ApplicationEntry const* application);

        private:
            size_t totalMetricCount_;

            void AddSbLoad(NodeEntry const* node, PartitionEntry const* partition);
            void DeleteSbLoad(NodeEntry const* node, PartitionEntry const* partition);

            void AddLoad(ApplicationEntry const* application, PartitionEntry const* partition, LoadEntry const* load);
            void DeleteLoad(ApplicationEntry const* application, PartitionEntry const* partition, LoadEntry const* load);

        };
    }
}
