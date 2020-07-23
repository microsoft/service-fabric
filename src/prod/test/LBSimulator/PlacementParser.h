// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Reliability/LoadBalancing/BalanceChecker.h"
#include "Reliability/LoadBalancing/DynamicBitSet.h"
#include "Reliability/LoadBalancing/SearcherSettings.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;

        class NodeEntry;
        class ServiceEntry;
        class PartitionEntry;
        class PlacementReplica;
        class SearcherSettings;
    }
}

namespace LBSimulator
{
    class PlacementParser
    {
        DENY_COPY(PlacementParser);

    public:
        static std::wstring const SectionLBDomains;
        static std::wstring const SectionNodes;
        static std::wstring const SectionServices;
        static std::wstring const SectionPartitions;
        static int const MaxReplicaCount;

        PlacementParser(Reliability::LoadBalancingComponent::SearcherSettings const & settings);
        void Parse(std::wstring const& fileName);
        Reliability::LoadBalancingComponent::PlacementUPtr CreatePlacement();

    private:
        void TrimParenthese(std::wstring & text);
        void ParseOneLBDomain(std::wstring const& text, size_t & metricStartIndex);
        static void AddDomainId(Common::TreeNodeIndex const& domainIndex, Reliability::LoadBalancingComponent::BalanceChecker::DomainTree & domainStructure);
        void ParseOneNode(std::wstring const& text, int index);
        void ParseOneService(std::wstring const& text, int index);
        static Reliability::LoadBalancingComponent::ReplicaRole::Enum ReplicaRoleFromString(std::wstring const& roleStr);
        void ParseOnePartition(std::wstring const& text);

        bool Validate();

        void CreateServiceToLBDomainTable();
        void CreateNodeDict();
        void CreateServiceDict();
        static std::set<Reliability::LoadBalancingComponent::NodeEntry const*> ConstructBlockList(std::set<Federation::NodeId> const& blockList,
            std::map<Federation::NodeId, Reliability::LoadBalancingComponent::NodeEntry const*> const& nodeEntryDict);
        static Reliability::LoadBalancingComponent::BalanceChecker::DomainTree GetFilteredDomainTree(
            Reliability::LoadBalancingComponent::BalanceChecker::DomainTree const& baseTree,
            std::set<Reliability::LoadBalancingComponent::NodeEntry const*> const& blockList, bool isFaultDomain);

        size_t replicaIndex_;

        Reliability::LoadBalancingComponent::DynamicBitSet CreateBitSetFromNodes(std::set<Reliability::LoadBalancingComponent::NodeEntry const*>);

    private:
        Reliability::LoadBalancingComponent::BalanceChecker::DomainTree fdStructure_;
        Reliability::LoadBalancingComponent::BalanceChecker::DomainTree udStructure_;

        std::vector<Reliability::LoadBalancingComponent::LoadBalancingDomainEntry> lbDomainEntries_;
        std::map<int, Reliability::LoadBalancingComponent::LoadBalancingDomainEntry const* > serviceToLBDomainTable_;

        bool existDefragMetric_;

        std::vector<Reliability::LoadBalancingComponent::NodeEntry> nodeEntries_;
        std::map<Federation::NodeId, Reliability::LoadBalancingComponent::NodeEntry const* > nodeDict_;

        std::vector<Reliability::LoadBalancingComponent::ServiceEntry> serviceEntries_;
        std::map<std::wstring, Reliability::LoadBalancingComponent::ServiceEntry const* > serviceDict_;

        std::vector<Reliability::LoadBalancingComponent::PartitionEntry> partitionEntries_;
        std::vector<std::unique_ptr<Reliability::LoadBalancingComponent::PlacementReplica>> allReplicas_;

        std::vector<std::unique_ptr<Reliability::LoadBalancingComponent::PlacementReplica>> standByReplicas_;
        std::vector<Reliability::LoadBalancingComponent::PlacementReplica const*> toBeDroppedReplicas_;

        Reliability::LoadBalancingComponent::SearcherSettings const & settings_;

        size_t quorumBasedServicesCount_;
        size_t quorumBasedPartitionsCount_;

        std::set<uint64> quorumBasedServicesTempCache_;
    };
}
