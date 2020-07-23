// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LoadEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadBalancingDomainEntry;
        class PlacementReplica;
        class NodeEntry
        {
            DENY_COPY(NodeEntry);

        public:
            static LoadEntry ConvertToFlatEntries(std::vector<LoadEntry> const& entries);

            static Common::GlobalWString const TraceDescription;

            NodeEntry(
                int nodeIndex,
                Federation::NodeId nodeId,
                LoadEntry && loads,
                LoadEntry && shouldDisappearLoads,
                LoadEntry && capacityRatios,
                LoadEntry && bufferedCapacities,
                LoadEntry && totalCapacities,
                Common::TreeNodeIndex && faultDomainIndex,
                Common::TreeNodeIndex && upgradeDomainIndex,
                bool isDeactivated,
                bool isUp,
                std::vector<std::wstring> && nodeImages = std::vector<std::wstring>(),
                bool isValid = true,
                std::wstring && nodeTypeName = L"");

            NodeEntry(NodeEntry && other);

            NodeEntry & operator = (NodeEntry && other);

            __declspec (property(get=get_NodeId)) Federation::NodeId const & NodeId;
            Federation::NodeId const & get_NodeId() const { return nodeId_; }

            __declspec (property(get=get_NodeIndex)) int NodeIndex;
            int get_NodeIndex() const { return nodeIndex_; }

            __declspec (property(get=get_Loads)) LoadEntry const& Loads;
            LoadEntry const& get_Loads() const { return loads_; }

            __declspec (property(get = get_ShouldDisappearLoads)) LoadEntry const& ShouldDisappearLoads;
            LoadEntry const& get_ShouldDisappearLoads() const { return shouldDisappearLoads_; }

            __declspec (property(get=get_Capacities)) LoadEntry const& BufferedCapacities;
            LoadEntry const& get_Capacities() const { return bufferedCapacities_; }

            __declspec (property(get=get_TotalCapacities)) LoadEntry const& TotalCapacities;
            LoadEntry const& get_TotalCapacities() const { return totalCapacities_; }

            __declspec (property(get=get_HasCapacity)) bool HasCapacity;
            bool get_HasCapacity() const { return hasCapacity_; }

            __declspec (property(get=get_GlobalMetricCount)) size_t GlobalMetricCount;
            size_t get_GlobalMetricCount() const { return bufferedCapacities_.Values.size(); }

            __declspec (property(get=get_FaultDomainIndex)) Common::TreeNodeIndex const& FaultDomainIndex;
            Common::TreeNodeIndex const& get_FaultDomainIndex() const { return faultDomainIndex_; }

            __declspec (property(get=get_UpgradeDomainIndex)) Common::TreeNodeIndex const& UpgradeDomainIndex;
            Common::TreeNodeIndex const& get_UpgradeDomainIndex() const { return upgradeDomainIndex_; }

            __declspec (property(get=get_GlobalMetricStartIndex)) size_t GlobalMetricStartIndex;
            size_t get_GlobalMetricStartIndex() const { return loads_.Values.size() - GlobalMetricCount; }

            __declspec (property(get = get_IsDeactivated)) bool IsDeactivated;
            bool get_IsDeactivated() const { return isDeactivated_; }

            __declspec (property(get = get_IsUp)) bool IsUp;
            bool get_IsUp() const { return isUp_; }

            __declspec (property(get = get_NodeImages)) std::vector<std::wstring> const& NodeImages;
            std::vector<std::wstring> const& get_NodeImages() const { return nodeImages_; }

            __declspec (property(get = get_MaxConcurrentBuilds, put = put_MaxConcurrentBuilds)) int MaxConcurrentBuilds;
            int get_MaxConcurrentBuilds() const { return maxConcurrentBuilds_; }
            void put_MaxConcurrentBuilds(int value) { maxConcurrentBuilds_ = value; }

            __declspec (property(get = get_IsThrottled)) bool IsThrottled;
            bool get_IsThrottled() const { return maxConcurrentBuilds_ > 0; }

            __declspec (property(get = get_IsValid, put = put_IsValid)) bool IsValid;
            bool get_IsValid() const { return isValid_; }

            __declspec (property(get = get_NodeTypeName)) std::wstring const& NodeTypeName;
            std::wstring const& get_NodeTypeName() const { return nodeTypeName_; }

            int64 GetLoadLevel(size_t metricIndex) const;
            int64 GetLoadLevel(size_t metricIndex, int64 diff) const;
            int64 GetNodeCapacity(size_t metricIndex) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            // Index among all nodes, used for fast node related lookups.
            int nodeIndex_;

            Federation::NodeId nodeId_;

            // loads for all replicas except moveInProgress and toBeDropped, for all lb domains, Dimension: total metric #
            LoadEntry loads_;

            // loads for only moveInProgress and toBeDropped replicas, for all lb domains, Dimension: total metric #
            LoadEntry shouldDisappearLoads_;

            // Dimension: global metric #
            LoadEntry capacityRatios_;

            // Dimension: global metric #
            // Available capacity - total capacity without NodeBufferPercentage
            LoadEntry bufferedCapacities_;

            // Dimension: global metric #
            LoadEntry totalCapacities_;

            // whether any one of the metric has a finite capacity (capacity>=0)
            bool hasCapacity_;

            // fault domain index, empty if no need to create fault domain structure
            Common::TreeNodeIndex faultDomainIndex_;

            // upgrade domain index, empty if no need to create upgrade domain structure
            Common::TreeNodeIndex upgradeDomainIndex_;

            // whether node started deactivation with any deactivation intent
            bool isDeactivated_;

            // indicates if a node is up or not
            bool isUp_;

            // all container images for that node
            std::vector<std::wstring> nodeImages_;

            // Maximum number of concurrent builds for this node. Used for throttling.
            int maxConcurrentBuilds_;

            // Is this node valid for this placement (not blocklisted by all services)
            bool isValid_;

            // Node type for this node
            std::wstring nodeTypeName_;
        };
    }
}
