// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "AutoScaleStatistics.h"
#include "DefragStatistics.h"
#include "RGStatistics.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackageDescription;
        class NodeDescription;
        class Snapshot;
        class ServiceDescription;
        class PLBEventSource;

        class PLBStatistics
        {
        public:
            __declspec(property(get = get_RGStatistics)) RGStatistics const & StatisticsRG;
            RGStatistics const & get_RGStatistics() const { return rgStatistics_; }

            __declspec(property(get = get_AutoScaleStatistics)) AutoScaleStatistics const & StatisticsAutoScale;
            AutoScaleStatistics const & get_AutoScaleStatistics() const { return autoScaleStatistics_; }

            __declspec(property(get = get_DefragStatistics)) DefragStatistics const & StatisticsDefrag;
            DefragStatistics const & get_DefragStatistics() const { return defragStatistics_; }

            // Tracking service package (add, delete, update) for statistics
            void AddServicePackage(ServicePackageDescription const&);
            void RemoveServicePackage(ServicePackageDescription const&);
            void UpdateServicePackage(ServicePackageDescription const&, ServicePackageDescription const&);

            // Functions that update nodes, needed to track available resources
            // In case when node capacity is not defined on at least one node, PLB will consider total capacity to be infinite.
            // We want to avoid that and to track how much resources are in the cluster, even if there are such nodes.
            void AddNode(NodeDescription const&);
            void RemoveNode(NodeDescription const&);
            void UpdateNode(NodeDescription const&, NodeDescription const&);

            // Tracking services for statistics (add, delete, update)
            void AddService(ServiceDescription const&);
            void DeleteService(ServiceDescription const&);
            void UpdateService(ServiceDescription const&, ServiceDescription const&);

            // Updates loads and configuration parameters
            void Update(Snapshot const&);

            void TraceStatistics(PLBEventSource const&);

        private:
            RGStatistics rgStatistics_;
            AutoScaleStatistics autoScaleStatistics_;
            DefragStatistics defragStatistics_;
        };
    }
}
