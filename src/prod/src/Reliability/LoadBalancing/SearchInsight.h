// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PartitionEntry;
        class PlacementReplica;
        class NodeEntry;

        // This class is used to capture search engine internal
        // states and decisions, and then trace it at the end of the run
        class SearchInsight
        {
            DENY_COPY_ASSIGNMENT(SearchInsight);
        public:
            SearchInsight();
            SearchInsight(SearchInsight && other);
            SearchInsight(SearchInsight const& other);
            SearchInsight(SearchInsight const* insight);
            SearchInsight& operator= (SearchInsight && other);

            __declspec (property(get = get_HasInsight, put = set_HasInsight)) bool HasInsight;
            bool get_HasInsight() const { return hasInsight_; }
            void set_HasInsight(bool value) { hasInsight_ = value; }

            __declspec (property(get = get_SingleReplicaUpgradeInfo, put = set_SingleReplicaUpgradeInfo)) std::map<PartitionEntry const*, std::vector<PlacementReplica const*>>& SingleReplicaUpgradeInfo;
            std::map<PartitionEntry const*, std::vector<PlacementReplica const*>>& get_SingleReplicaUpgradeInfo() { return singleReplicaUpgradeInfo_; }

            __declspec (property(get = get_SingleReplicaTargetNode, put = set_SingleReplicaTargetNode)) std::map<PartitionEntry const*, Federation::NodeId>& SingleReplicaTargetNode;
            std::map<PartitionEntry const*, Federation::NodeId>& get_SingleReplicaTargetNode() { return singleReplicaTargetNode_; }

            __declspec (property(get = get_SingleReplicaNonCreatedMoves, put = set_SingleReplicaNonCreatedMoves)) std::map<PartitionEntry const*, std::vector<Common::Guid>>& SingleReplicaNonCreatedMoves;
            std::map<PartitionEntry const*, std::vector<Common::Guid>>& get_SingleReplicaNonCreatedMoves() { return singleReplicaNonCreatedMoves_; }

            void Clear();

            // Trace write support
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            bool hasInsight_;

            // Set of partitions in single replica upgrade optimization
            std::map<PartitionEntry const*, std::vector<PlacementReplica const*>> singleReplicaUpgradeInfo_;
            // Target node for single replica upgrade optimization
            std::map<PartitionEntry const*, Federation::NodeId> singleReplicaTargetNode_;
            // Set of partitions for which movement is not accepted
            std::map<PartitionEntry const*, std::vector<Common::Guid>> singleReplicaNonCreatedMoves_;
            

        };
    }
}
