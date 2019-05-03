// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "BalanceChecker.h"
#include "ApplicationReservedLoads.h"
#include "ServicePackagePlacement.h"
#include "Hasher.h"
#include "InBuildCountPerNode.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementAndLoadBalancing;
        class ServiceDomain;
        class PartitionClosure;
        typedef std::unique_ptr<PartitionClosure> PartitionClosureUPtr;
        class PlacementReplica;
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;
        class Service;
        class NodeEntry;
        class ServiceEntry;
        class PartitionEntry;
        class ApplicationEntry;
        class ApplicationLoad;
        class DynamicBitSet;
        class ServicePackageEntry;
        class ServicePackagePlacement;

        class BalanceChecker;
        typedef std::unique_ptr<BalanceChecker> BalanceCheckerUPtr;

        class PlacementCreator
        {
            DENY_COPY(PlacementCreator);

        public:

            enum NodeMatchType
            {
                NotExist = 0,
                IsDown = 1,
                InstanceNotMatch = 2,
                IsPaused = 3,
                IsRestartingOrRemoving = 4,
                IsDeactivated = 5,
                Match = 6
            };

            PlacementCreator(
                PlacementAndLoadBalancing const& plb, 
                ServiceDomain const& serviceDomain,
                BalanceCheckerUPtr && balanceChecker,
                std::set<Common::Guid> && throttledPartitions,
                PartitionClosureUPtr const& partitionClosure
                );

            PlacementUPtr Create(int randomSeed);

        private:
            void CreateServicePackageEntries();
            void CreateServiceEntries();
            void CreatePartitionEntries();
            void CreateApplicationEntries();
            void CreateApplicationReservedLoads();
            void PrepareNodes();

            void CalculateApplicationNodeLoads(
                ApplicationLoad const& appLoad,
                std::map<NodeEntry const*, LoadEntry> & loadsToCalculate,
                bool disappearingLoads);

            NodeMatchType NodeExistAndMatch(Federation::NodeInstance node) const;
            BalanceChecker::DomainTree GetFilteredDomainTree(BalanceChecker::DomainTree const& baseTree, DynamicBitSet const& blockList, bool isFaultDomain);

            BalanceChecker::DomainTree FilterAndCalculateDomainStructure(size_t & domainCount, size_t & nodeCount, bool isFaultDomain, Service const* service);

            PlacementAndLoadBalancing const& plb_;
            ServiceDomain const& serviceDomain_;
            BalanceCheckerUPtr balanceChecker_;
            std::set<Common::Guid> throttledPartitions_;
            PartitionClosureUPtr const& partitionClosure_;

            std::vector<ServicePackageEntry> servicePackageEntries_;
            std::map<uint64, ServicePackageEntry const*> servicePackageDict_;

            std::vector<ServiceEntry> serviceEntries_;
            Uint64UnorderedMap<ServiceEntry *> serviceEntryDict_;

            std::vector<PartitionEntry> partitionEntries_;
            std::vector<std::unique_ptr<PlacementReplica>> allReplicas_;
            std::vector<std::unique_ptr<PlacementReplica>> standByReplicas_;

            std::vector<ApplicationEntry> applicationEntries_;
            Uint64UnorderedMap<ApplicationEntry *> applicationEntryDict_;
            ApplicationReservedLoad reservationLoads_;

            size_t partitionsInUpgradeCount_;

            std::map<std::wstring, size_t> metricNameToGlobalIndex_;
            ServicePackagePlacement servicePackagePlacement_;

            size_t quorumBasedServicesCount_;
            size_t quorumBasedPartitionsCount_;

            // Makes temporary cache of services that use quorum based logic.
            // Takes into account only services that have a partition in closure.
            std::set<uint64> quorumBasedServicesTempCache_;

            // Count of InBuild replicas per node
            InBuildCountPerNode inBuildCountPerNode_;
        };

    }
}
