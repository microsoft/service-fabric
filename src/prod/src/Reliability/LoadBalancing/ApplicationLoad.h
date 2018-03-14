// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"
#include "ServiceDomainMetric.h"
#include "Application.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationLoad
        {
            DENY_COPY(ApplicationLoad);

        public:
            explicit ApplicationLoad(uint64 applicationId);

            ApplicationLoad(ApplicationLoad && other);

            __declspec(property(get = get_MetricLoads)) std::map<std::wstring, ServiceDomainMetric> const& MetricLoads;
            std::map<std::wstring, ServiceDomainMetric> const& get_MetricLoads() const { return metricLoads_; }

            __declspec(property(get = get_NodeCounts)) std::map<Federation::NodeId, size_t> const& NodeCounts;
            std::map<Federation::NodeId, size_t> const& get_NodeCounts() const { return nodeCounts_; }

            static Common::GlobalWString const FormatHeader;

            void AddLoad(std::wstring const& metricName, int64 load);
            void DeleteLoad(std::wstring const& metricName, int64 load);

            void AddNodeLoad(std::wstring const& metricName, Federation::NodeId node, uint load, bool shouldDisappear);
            void DeleteNodeLoad(std::wstring const& metricName, Federation::NodeId node, uint load, bool shouldDisappear);

            void AddReplicaToNode(Federation::NodeId node);
            void DeleteReplicaFromNode(Federation::NodeId node);

            // Get reserved load that is used (totalReservedCapacity if more is used)
            int64 GetReservedLoadUsed(std::wstring const& metricName, int64 totalReservedCapacity) const;

            // merge application loads
            void Merge(ApplicationLoad& other);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            bool WriteFor(Application const& app, Common::StringWriterA& w) const;

        private:

            uint64 applicationId_;

            // MetricName to total load table
            std::map<std::wstring, int64> loadTable_;

            // MetricName to ServiceDomainMetric table
            std::map<std::wstring, ServiceDomainMetric> metricLoads_;

            // NodeId to number of replicas mapping
            std::map<Federation::NodeId, size_t> nodeCounts_;
        };
    }
}
