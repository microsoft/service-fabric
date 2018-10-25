// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"
#include "Metric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Node;
        class ServiceDomainMetric;

        template <typename Identifier,
            typename Item,
            typename ItemStatus,
            typename ItemMapType = std::map<Identifier, Item>,
            typename ItemStatusMapType = std::map<std::wstring, ItemStatus>>
        class TraceLoads
        {
            DENY_COPY(TraceLoads);

        public:
            TraceLoads(
                ItemMapType const& itemMap,
                ItemStatusMapType const& itemStatusMap,
                uint iteration,
                Identifier * itemPtr,
                uint maxEntries,
                bool statusHeader = false)
                : itemMap_(itemMap),
                itemStatusMap_(itemStatusMap),
                iteration_(iteration),
                itemPtr_(itemPtr),
                maxEntries_(maxEntries),
                statusHeader_(statusHeader)
            {
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            ItemMapType const& itemMap_;
            ItemStatusMapType const& itemStatusMap_;
            Identifier * itemPtr_;
            uint iteration_;
            uint maxEntries_;
            bool statusHeader_;
        };

        template <typename Identifier,
            typename Item,
            typename ItemStatus,
            typename ItemMapType = std::vector<Item>,
            typename ItemStatusMapType = std::map<std::wstring, ItemStatus >>
        class TraceLoadsVector
        {
            DENY_COPY(TraceLoadsVector);

        public:
            TraceLoadsVector(
                ItemMapType const& itemMap,
                ItemStatusMapType const& itemStatusMap,
                uint iteration,
                Identifier * itemPtr,
                uint maxEntries,
                bool statusHeader = false)
                : itemMap_(itemMap),
                itemStatusMap_(itemStatusMap),
                iteration_(iteration),
                itemPtr_(itemPtr),
                maxEntries_(maxEntries),
                statusHeader_(statusHeader)
            {
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            ItemMapType const& itemMap_;
            ItemStatusMapType const& itemStatusMap_;
            Identifier * itemPtr_;
            uint iteration_;
            uint maxEntries_;
            bool statusHeader_;
        };

        template <typename Identifier,
            typename Item,
            typename Description,
            typename ItemMapType = std::map<Identifier, Item>,
            typename ItemVectorType = std::vector<int>>
        class TraceDescriptions
        {
            DENY_COPY(TraceDescriptions);

        public:
            TraceDescriptions(
                ItemMapType const& itemMap,
                Identifier * itemPtr,
                uint maxEntries,
                ItemVectorType const& itemQuorumDomainLogic = std::vector<int>())
                : itemMap_(itemMap),
                itemPtr_(itemPtr),
                maxEntries_(maxEntries),
                itemQuorumDomainLogic_(itemQuorumDomainLogic)
            {
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            ItemMapType const& itemMap_;
            Identifier * itemPtr_;
            uint maxEntries_;
            ItemVectorType const& itemQuorumDomainLogic_;
        };

        template<typename Item, typename Description>
        class TraceVectorDescriptions
        {
            DENY_COPY(TraceVectorDescriptions);

        public:
            TraceVectorDescriptions(
                std::vector<Item> const& itemVector,
                uint64 * itemPtr,
                uint maxEntries)
                : itemVector_(itemVector),
                itemPtr_(itemPtr),
                maxEntries_(maxEntries)
            {
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::vector<Item> const& itemVector_;
            uint64 * itemPtr_;
            uint maxEntries_;
        };

        class TraceDefragMetricInfo
        {
            DENY_COPY(TraceDefragMetricInfo);

        public:

            TraceDefragMetricInfo() {}

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            wstring metricName;
            std::map<std::vector<wstring>, std::pair<size_t, double>> faultDomainItems_;
            std::map<wstring, std::pair<size_t, double>> upgradeDomainItems_;
            double avgStdDev_;
            double avgStdDevFaultDomain_;
            double avgStdDevUpgradeDomain_;
            double averageLoad_;
            double averageFdLoad_;
            double averageUdLoad_;
            size_t emptyNodeCnt_;
            double nonEmptyNodeLoad_;
            double totalClusterLoad_;

            wstring type_;
            int32 targetEmptyNodeCount_;

            Metric::DefragDistributionType emptyNodesDistribution_;
            size_t emptyNodeLoadThreshold_;
            int64 reservationLoad_;

            double emptyNodesWeight_;
            double nonEmptyNodesWeight_;
            uint activityThreshold_;
            double balancingThreshold_;
        };

        class TraceAllBalancingMetricInfo
        {
            DENY_COPY(TraceAllBalancingMetricInfo);

        public:

            struct BalancingMetricData
            {
                double avgStdDev_;
                double averageLoad_;
                double totalClusterLoad_;

                uint activityThreshold_;
                double balancingThreshold_;

                BalancingMetricData() {}
                BalancingMetricData(
                    double avgStdDev,
                    double averageLoad,
                    double totalClusterLoad,
                    uint activityThreshold,
                    double balancingThreshold)
                {
                    avgStdDev_ = avgStdDev;
                    averageLoad_ = averageLoad;
                    totalClusterLoad_ = totalClusterLoad;

                    activityThreshold_ = activityThreshold;
                    balancingThreshold_ = balancingThreshold;
                }
            };

            TraceAllBalancingMetricInfo() {}

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            std::map<wstring, BalancingMetricData> items_;
        };
    }
}
