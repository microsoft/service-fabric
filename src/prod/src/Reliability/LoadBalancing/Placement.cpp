// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <numeric>
#include "PLBConfig.h"
#include "Placement.h"
#include "AccumulatorWithMinMax.h"
#include "PlacementAndLoadBalancing.h"
#include "Service.h"
#include "BalanceChecker.h"
#include "PLBSchedulerAction.h"
#include "ThrottlingConstraint.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

Placement::Placement(
    BalanceCheckerUPtr && balanceChecker,
    vector<ServiceEntry> && services,
    vector<PartitionEntry> && partitions,
    vector<ApplicationEntry> && applications,
    vector<ServicePackageEntry> && servicePackages,
    vector<std::unique_ptr<PlacementReplica>> && allReplicas,
    vector<std::unique_ptr<PlacementReplica>> && standByReplicas,
    ApplicationReservedLoad && applicationReservedLoads,
    InBuildCountPerNode && ibCountsPerNode,
    size_t partitionsInUpgradeCount,
    bool isSingletonReplicaMoveAllowedDuringUpgrade,
    int randomSeed,
    PLBSchedulerAction const& schedulerAction,
    PartitionClosureType::Enum partitionClosureType,
    set<Common::Guid> const& partialClosureFTs,
    ServicePackagePlacement && servicePackagePlacement,
    size_t quorumBasedServicesCount,
    size_t quorumBasedPartitionsCount,
    std::set<uint64> && quorumBasedServicesTempCache)
    : balanceChecker_(move(balanceChecker)),
    services_(move(services)),
    partitions_(move(partitions)),
    applications_(move(applications)),
    servicePackages_(move(servicePackages)),
    parentPartitions_(),
    allReplicas_(move(allReplicas)),
    standByReplicas_(move(standByReplicas)),
    movableReplicas_(),
    beneficialReplicas_(),
    beneficialDefragReplicas_(),
    beneficialRestrictedDefragReplicas_(),
    swappablePrimaryReplicas_(),
    beneficialPrimaryReplicas_(),
    beneficialPrimaryDefragReplicas_(),
    beneficialRestrictedDefragPrimaryReplicas_(),
    newReplicas_(),
    nodePlacements_(),
    nodeMovingInPlacements_(),
    partitionPlacements_(),
    applicationPlacements_(),
    applicationNodeCount_(),
    applicationTotalLoad_(GlobalMetricCount, TotalMetricCount),
    applicationNodeLoads_(GlobalMetricCount, TotalMetricCount - GlobalMetricCount),
    applicationReservedLoads_(move(applicationReservedLoads)),
    faultDomainStructures_(true),
    upgradeDomainStructures_(false),
    beneficialTargetNodesPerMetric_(),
    beneficialTargetNodesForPlacementPerMetric_(),
    partitionsInUpgradeCount_(partitionsInUpgradeCount),
    isSingletonReplicaMoveAllowedDuringUpgrade_(isSingletonReplicaMoveAllowedDuringUpgrade),
    eligibleNodes_(this, true),
    partitionsInUpgradePlacementCount_(0),
    random_(randomSeed),
    settings_(balanceChecker_->Settings),
    partitionClosureType_(partitionClosureType),
    servicePackagePlacements_(move(servicePackagePlacement)),
    inBuildCountPerNode_(move(ibCountsPerNode)),
    quorumBasedServicesCount_(quorumBasedServicesCount),
    quorumBasedPartitionsCount_(quorumBasedPartitionsCount),
    quorumBasedServicesTempCache_(move(quorumBasedServicesTempCache)),
    throttlingConstraintPriority_(-1)
{
    // Remove down and deactivated nodes from eligible nodes!
    eligibleNodes_.DeleteNodeVecWithIndex(BalanceCheckerObj->DownNodes);
    eligibleNodes_.DeleteNodeVecWithIndex(BalanceCheckerObj->DeactivatedNodes);
    PrepareServices();
    PreparePartitions();
    PrepareReplicas(partialClosureFTs);
    ComputeBeneficialTargetNodesPerMetric();
    if (schedulerAction.Action == PLBSchedulerActionType::NewReplicaPlacement ||
        schedulerAction.Action == PLBSchedulerActionType::NewReplicaPlacementWithMove)
    {
        ComputeBeneficialTargetNodesForPlacementPerMetric();
    }
    CreateReplicaPlacement();
    CreateNodePlacement();
    PrepareApplications();
    UpdateAction(schedulerAction.Action, true);
}

void Placement::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.WriteLine("{0}", *LoadBalancingDomainEntry::TraceDescription);
    for (LoadBalancingDomainEntry const& lbDomainEntry : balanceChecker_->LBDomains)
    {
        writer.WriteLine("{0}", lbDomainEntry);
    }

    writer.WriteLine("{0}", *NodeEntry::TraceDescription);
    for (NodeEntry const& nodeEntry : balanceChecker_->Nodes)
    {
        writer.WriteLine("{0}", nodeEntry);
    }

    writer.WriteLine("{0}", *ServiceEntry::TraceDescription);
    for (ServiceEntry const& serviceEntry : services_)
    {
        writer.WriteLine("{0}", serviceEntry);
    }

    writer.WriteLine("{0}", *PartitionEntry::TraceDescription);
    for (PartitionEntry const& partitionEntry : partitions_)
    {
        writer.WriteLine("{0}", partitionEntry);
    }

    if (!applications_.empty())
    {
        writer.WriteLine("{0}", *ApplicationEntry::TraceDescription);
        for (ApplicationEntry const& applicationEntry : applications_)
        {
            writer.WriteLine("{0}", applicationEntry);
        }
    }

    if (!servicePackages_.empty())
    {
        writer.WriteLine("{0}", *ServicePackageEntry::TraceDescription);
        for (ServicePackageEntry const& servicePackageEntry : servicePackages_)
        {
            writer.WriteLine("{0}", servicePackageEntry);
        }
    }
}

void Placement::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PlacementRecord(
        contextSequenceId, 
        *LoadBalancingDomainEntry::TraceDescription,
        balanceChecker_->LBDomains, 
        *NodeEntry::TraceDescription,
        balanceChecker_->Nodes, 
        *ServiceEntry::TraceDescription,
        services_, 
        *PartitionEntry::TraceDescription,
        partitions_,
        applications_
        );
}

wstring Placement::GetImbalancedServices() const
{
    ServiceEntry const* firstImbalancedService = nullptr;
    size_t imbalanceCount = 0;
    for (auto itService = Services.begin(); itService != Services.end(); ++itService)
    {
        if (!itService->IsBalanced)
        {
            ++imbalanceCount;
            if (firstImbalancedService == nullptr)
            {
                firstImbalancedService = &(*itService);
            }
        }
    }

    wstring result;
    StringWriter writer(result);
    if (imbalanceCount == 0)
    {
        writer.Write("all service balanced");
    }
    else if (imbalanceCount == 1)
    {
        writer.Write("{0} is imbalanced", firstImbalancedService->Name);
    }
    else if (imbalanceCount == Services.size())
    {
        writer.Write("all services are imbalanced");
    }
    else
    {
        writer.Write("{0} services including {1} ... are imbalanced", imbalanceCount, firstImbalancedService->Name);
    }

    return result;
}

NodeSet const* Placement::GetBeneficialTargetNodesForMetric(size_t globalMetricIndex) const
{
    auto it = beneficialTargetNodesPerMetric_.find(globalMetricIndex);
    return it == beneficialTargetNodesPerMetric_.end() ? nullptr : &it->second;
}

NodeSet const* Placement::GetBeneficialTargetNodesForPlacementForMetric(size_t globalMetricIndex) const
{
    auto it = beneficialTargetNodesForPlacementPerMetric_.find(globalMetricIndex);
    return it == beneficialTargetNodesForPlacementPerMetric_.end() ? nullptr : &it->second;
}

PlacementReplica const* Placement::SelectRandomReplica(Common::Random & random, bool useNodeLoadAsHeuristic, bool useRestrictedDefrag, bool targetEmptyNodesAchieved) const
{
    auto & defragReplicas = (useRestrictedDefrag == false) ? beneficialDefragReplicas_ : beneficialRestrictedDefragReplicas_;
    auto & beneficialReplicas = (targetEmptyNodesAchieved || defragReplicas.size() == 0) ? beneficialReplicas_ : defragReplicas;
    auto & replicas = useNodeLoadAsHeuristic ? beneficialReplicas : movableReplicas_;
    if (replicas.empty())
    {
        return nullptr;
    }
    else
    {
        int index = random.Next(static_cast<int>(replicas.size()));
        return replicas[index];
    }
}

PlacementReplica const* Placement::SelectRandomExistingReplica(Common::Random & random, bool usePartialClosure) const
{
    if (allReplicas_.empty())
    {
        return nullptr;
    }
    else
    {
        bool usePartialClosureReplicas = usePartialClosure && !partialClosureReplicas_.empty();
        size_t replicasSize = usePartialClosureReplicas ? partialClosureReplicas_.size() : allReplicas_.size();
        int index = random.Next(static_cast<int>(replicasSize));
        auto r = usePartialClosureReplicas ? partialClosureReplicas_[index] : allReplicas_[index].get();
        if (r->IsNew || r->IsNone || !r->IsMovable)
        {
            return nullptr;
        }
        else
        {
            return r;
        }
    }
}

PlacementReplica const* Placement::SelectRandomPrimary(Common::Random & random, bool useNodeLoadAsHeuristic, bool useRestrictedDefrag, bool targetEmptyNodesAchieved) const
{
    auto & defragReplicas = (useRestrictedDefrag == false) ? beneficialPrimaryDefragReplicas_ : beneficialRestrictedDefragPrimaryReplicas_;
    auto & beneficialReplicas = (targetEmptyNodesAchieved || defragReplicas.size() == 0) ? beneficialPrimaryReplicas_ : defragReplicas;
    auto & replicas = useNodeLoadAsHeuristic ? beneficialReplicas : swappablePrimaryReplicas_;
    if (replicas.empty())
    {
        return nullptr;
    }
    else
    {
        int index = random.Next(static_cast<int>(replicas.size()));
        return replicas[index];
    }
}

size_t Placement::GetThrottledMoveCount() const
{
    size_t availableSlots = 0;
    for (auto const& node : balanceChecker_->Nodes)
    {
        if (node.IsValid)
        {
            if (!node.IsThrottled)
            {
                // One non-throttled node is enough to have max slots.
                return SIZE_MAX;
            }
            else
            {
                availableSlots += node.MaxConcurrentBuilds;
            }
        }
    }
    return availableSlots;
}

// Sometimes placement can be created without an action, so we need to update it when it is known.
// Per-node throttling limits depend on the action type.
void Placement::UpdateAction(PLBSchedulerActionType::Enum action, bool constructor)
{
    bool throttlingNeeded = false;
    // Node placements are created only if node has capacity or throttling defined.
    // Since we may change throttling limit in this function (0 -> >0) then we may need to recalculate.
    bool recreateNodePlacements = false;
    // If called from constructor, always update the action and set up throttling limits.
    if (!constructor && action == action_)
    {
        return;
    }
    action_ = action;
    if (   action_ != PLBSchedulerActionType::NoActionNeeded
        && action_ != PLBSchedulerActionType::None)
    {
        for (auto const& node : balanceChecker_->Nodes)
        {
            int maxConcurrentBuilds = 0;
            int globalConcurrentBuilds = INT_MAX;
            int phaseConcurrentBuilds = INT_MAX;

            auto const & throttlingLimitIt = settings_.MaximumInBuildReplicasPerNode.find(node.NodeTypeName);
            if (throttlingLimitIt != settings_.MaximumInBuildReplicasPerNode.end())
            {
                globalConcurrentBuilds = throttlingLimitIt->second;
            }

            switch (action_)
            {
            case PLBSchedulerActionType::NewReplicaPlacement:
            case PLBSchedulerActionType::NewReplicaPlacementWithMove:
                {
                    auto const & throttlingLimitPlacement = settings_.MaximumInBuildReplicasPerNodePlacementThrottle.find(node.NodeTypeName);
                    if (throttlingLimitPlacement != settings_.MaximumInBuildReplicasPerNodePlacementThrottle.end())
                    {
                        phaseConcurrentBuilds = throttlingLimitPlacement->second;
                    }
                }
                break;
            case PLBSchedulerActionType::LoadBalancing:
            case PLBSchedulerActionType::QuickLoadBalancing:
                {
                    auto const & throttlingLimitBalancing = settings_.MaximumInBuildReplicasPerNodeBalancingThrottle.find(node.NodeTypeName);
                    if (throttlingLimitBalancing != settings_.MaximumInBuildReplicasPerNodeBalancingThrottle.end())
                    {
                        phaseConcurrentBuilds = throttlingLimitBalancing->second;
                    }
                }
                break;
            case PLBSchedulerActionType::ConstraintCheck:
                {
                    auto const & throttlingLimitConstraintCheck = settings_.MaximumInBuildReplicasPerNodeConstraintCheckThrottle.find(node.NodeTypeName);
                    if (throttlingLimitConstraintCheck != settings_.MaximumInBuildReplicasPerNodeConstraintCheckThrottle.end())
                    {
                        phaseConcurrentBuilds = throttlingLimitConstraintCheck->second;
                    }
                }
                break;
            default:
                break;
            }

            maxConcurrentBuilds = min(globalConcurrentBuilds, phaseConcurrentBuilds);
            if (maxConcurrentBuilds != INT_MAX)
            {
                // If this node already had throttling or node capacity, node placements were created.
                if (!node.HasCapacity && node.MaxConcurrentBuilds == 0)
                {
                    recreateNodePlacements = true;
                }
                balanceChecker_->UpdateNodeThrottlingLimit(node.NodeIndex, maxConcurrentBuilds);
                throttlingNeeded = true;
            }
        }
    }
    if (throttlingNeeded)
    {
        throttlingConstraintPriority_ = ThrottlingConstraint::GetPriority(action_);
    }
    if (recreateNodePlacements)
    {
        // We need to recreate node placements since throttling limits have changed.
        nodePlacements_.Clear();
        CreateNodePlacement();
    }
}

void Placement::PrepareServices()
{
    for (auto itService = services_.begin(); itService != services_.end(); ++itService)
    {
        itService->RefreshIsBalanced();
    }
}

void Placement::PreparePartitions()
{
    sort(partitions_.begin(), partitions_.end(), [](PartitionEntry const& p1, PartitionEntry const& p2) -> bool
    {
        return p1.Order < p2.Order;
    });

    extraReplicasCount_ = 0;

    size_t newReplicaBatchCount(0);
    int configBatchReplicaCount = PLBConfig::GetConfig().PlacementReplicaCountPerBatch;

    size_t tempUpgradeIndex(0);
    for (size_t i = 0; i < partitions_.size(); i++)
    {
        PartitionEntry & partition = partitions_[i];

        if (newReplicaBatchCount > configBatchReplicaCount)
        {
            newReplicaBatchCount = 0;
            partitionBatchIndexVec_.push_back(i);
        }

        newReplicaBatchCount += partition.NewReplicaCount;

        extraReplicasCount_ += partition.NumberOfExtraReplicas;

        partition.ConstructReplicas();

        ASSERT_IFNOT(services_.data() <= partition.Service && &(services_.back()) >= partition.Service, "Invalid service pointer");
        ServiceEntry & serviceEntry = services_[partition.Service - &(services_[0])];

        serviceEntry.AddPartition(&partition);

        // Increase placement movement slots during singleton replica upgrades,
        // for partitions which do not have new replicas, but are in affinity correlation,
        // with partitions in single replica upgrade
        if (settings_.CheckAffinityForUpgradePlacement &&
            partition.IsTargetOne &&                                                             // It is single replica service
            !partition.IsInSingleReplicaUpgrade &&                                               // Partition is not in upgrade
            ((serviceEntry.DependedService != nullptr &&                                         // Service has a parent service
                serviceEntry.DependedService->HasAffinityAssociatedSingletonReplicaInUpgrade) || //    which has correlation with singleton upgrade
            (serviceEntry.DependentServices.size() > 0 &&                                        // Service is parent service
                serviceEntry.HasAffinityAssociatedSingletonReplicaInUpgrade)))                   //    which is in singleton upgrade correlation
        {
            serviceEntry.HasAffinityAssociatedSingletonReplicaInUpgrade = true;
            partitionsInUpgradePlacementCount_++;
        }

        // If partition belongs to the application which has singleton replicas in upgrade,
        // but it hasn't received request for additional replica yet (or it is stateless),
        // and relaxed scaleout during upgrade should be performed,
        // increase the required movement count for placement
        if (settings_.RelaxScaleoutConstraintDuringUpgrade &&
            partition.IsTargetOne &&                                                    // It is single replica service
            partition.Service->Application != nullptr &&                                // Partition has application
            partition.Service->Application->HasPartitionsInSingletonReplicaUpgrade &&   // Application has at least one partition in upgrade
            !partition.IsInSingleReplicaUpgrade)                                        // Partition is not in upgrade
        {
            partitionsInUpgradePlacementCount_++;
        }

        if (partition.IsInUpgrade)
        {
            partition.SetUpgradeIndex(tempUpgradeIndex);
            tempUpgradeIndex++;

            if (settings_.CheckAlignedAffinityForUpgrade)
            {
                serviceEntry.HasInUpgradePartition = true;

                if (serviceEntry.IsAlignedChild)
                {
                    ServiceEntry & parentService = services_[serviceEntry.DependedService - &(services_[0])];
                    parentService.HasInUpgradePartition = true;
                }
            }
        }

        if (!serviceEntry.DependentServices.empty())
        {
            ASSERT_IFNOT(partition.Order < 2, "Parent partition should have order 0 or 1.");
            parentPartitions_.push_back(&partition);
        }

        if (!serviceEntry.OnEveryNode)
        {
            if (!balanceChecker_->FaultDomainStructure.IsEmpty && serviceEntry.FDDistribution != Service::Type::Ignore)
            {
                PartitionDomainTree & structure = faultDomainStructures_[&partition];
                if (structure.IsEmpty)
                {
                    structure.SetRoot(make_unique<PartitionDomainTree::Node>(PartitionDomainTree::Node::Create<BalanceChecker::DomainData>(
                        balanceChecker_->FaultDomainStructure.Root, [](BalanceChecker::DomainData const&) -> PartitionDomainData
                    {
                        return PartitionDomainData();
                    })));
                }

                partition.ForEachExistingReplica([&](PlacementReplica const* r)
                {
                    NodeEntry const* n = r->Node;
                    structure.ForEachNodeInPath(n->FaultDomainIndex, [=](PartitionDomainTree::Node & n)
                    {
                        ++n.DataRef.ReplicaCount;
                        if (n.Children.empty())
                        {
                            n.DataRef.Replicas.push_back(r);
                        }
                    });
                }, false);
            }

            if (!balanceChecker_->UpgradeDomainStructure.IsEmpty)
            {
                PartitionDomainTree & structure = upgradeDomainStructures_[&partition];
                if (structure.IsEmpty)
                {
                    structure.SetRoot(make_unique<PartitionDomainTree::Node>(PartitionDomainTree::Node::Create<BalanceChecker::DomainData>(
                        balanceChecker_->UpgradeDomainStructure.Root, [](BalanceChecker::DomainData const&) -> PartitionDomainData
                    {
                        return PartitionDomainData();
                    })));
                }

                partition.ForEachExistingReplica([&](PlacementReplica const* r)
                {
                    NodeEntry const* n = r->Node;
                    structure.ForEachNodeInPath(n->UpgradeDomainIndex, [=](PartitionDomainTree::Node & n)
                    {
                        ++n.DataRef.ReplicaCount;
                        if (n.Children.empty())
                        {
                            n.DataRef.Replicas.push_back(r);
                        }
                    });
                }, false);
            }
        }
    }

    if (settings_.CheckAlignedAffinityForUpgrade)
    {
        // Update partitions in aligned affinity with upgrade index
        for (size_t i = 0; i < partitions_.size(); i++)
        {
            PartitionEntry & partition = partitions_[i];

            if (partition.IsInUpgrade)
            {
                continue;
            }

            ServiceEntry const* service = partition.Service;
            if ((service->IsAlignedChild && service->DependedService->HasInUpgradePartition)
                || (service->HasInUpgradePartition && !service->DependentServices.empty()))
            {
                partition.SetUpgradeIndex(tempUpgradeIndex);
                tempUpgradeIndex++;
            }
        }

        partitionsInUpgradeCount_ = tempUpgradeIndex;

        // Update services with in upgrade partitions
        for (auto itService = services_.begin(); itService != services_.end(); ++itService)
        {
            if (itService->DependentServices.empty() || false == itService->HasInUpgradePartition)
            {
                continue;
            }

            itService->AddAlignedAffinityPartitions();
        }
    }

}

void Placement::PrepareReplicas(set<Common::Guid>const& partialClosureFTs)
{
    vector<int64> moveCosts(BalanceCheckerObj->Nodes.size(), 0);

    for (auto itReplica = allReplicas_.begin(); itReplica != allReplicas_.end(); ++itReplica)
    {
        auto replica = itReplica->get();
        if (replica->IsNew)
        {
            replica->SetNewReplicaIndex(newReplicas_.size());
            newReplicas_.push_back(replica);
        }
        else if (replica->Role != ReplicaRole::None &&
            !replica->Partition->Service->IsBalanced && replica->IsMovable && replica->Partition->IsMovable)
        {
            movableReplicas_.push_back(replica);
            moveCosts.at(replica->Node->NodeIndex) += replica->Partition->GetMoveCost(replica->Role);
        }

        // check if this replicas partition belongs to the partial closure set (has affinity/app group relations to the partitions with new replicas) and it is movable
        bool isInPartialClosure = std::find(partialClosureFTs.begin(), partialClosureFTs.end(),
            replica->get_PartitionEntry()->PartitionId) != partialClosureFTs.end();

        if (isInPartialClosure)
        {
            partialClosureReplicas_.push_back(replica);
        }
    }

    auto& dynamicNodeLoads = BalanceCheckerObj->DynamicNodeLoads;
    dynamicNodeLoads.AdvanceVersion();
    for (int i = 0; i < BalanceCheckerObj->Nodes.size(); i++) 
    {
        if (moveCosts.at(i) > 0)
        {
            dynamicNodeLoads.UpdateMoveCost(i, moveCosts.at(i));
        }
    }

    auto defragType = Metric::DefragDistributionType::NumberOfEmptyNodes;
    int32 numberOfEmptyNodes = 0;
    size_t totalMetricIndex = 0;
    for (auto it = BalanceCheckerObj->LBDomains.begin(); it != BalanceCheckerObj->LBDomains.end(); ++it)
    {
        for (auto metric = it->Metrics.begin(); metric != it->Metrics.end(); ++metric, ++totalMetricIndex)
        {
            if (metric->IsDefrag && metric->DefragmentationScopedAlgorithmEnabled)
            {
                dynamicNodeLoads.PrepareBeneficialNodes(totalMetricIndex, metric->DefragNodeCount, metric->DefragDistribution, metric->ReservationLoad);
                
                if (metric->DefragDistribution == Metric::DefragDistributionType::SpreadAcrossFDs_UDs)
                {
                    defragType = Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
                }
                if (numberOfEmptyNodes < metric->DefragNodeCount) 
                {
                    numberOfEmptyNodes  = metric->DefragNodeCount;
                }
            }
        }
    }

    // For moveCost beneficial nodes, number of empty nodes is max for all defrag metrics,
    // DefragDistributionType is SpreadAcross FDs and UDs if any defrag metric has such type
    dynamicNodeLoads.PrepareMoveCostBeneficialNodes(numberOfEmptyNodes, defragType);

    for (PlacementReplica const* replica : movableReplicas_)
    {
        bool isBeneficialReplica = false;
        bool isBeneficialReplicaForDefrag = false;
        bool isBeneficialReplicaRestrictedDefrag = false;
        replica->ForEachBeneficialMetric([&](size_t metricIndex, bool forDefrag) -> bool
        {
            metricIndex;
            isBeneficialReplica = true;

            if (forDefrag)
            {
                isBeneficialReplicaForDefrag = true;
            }
            if (dynamicNodeLoads.IsBeneficialNodeByMoveCost(replica->Node->NodeIndex))
            {
                isBeneficialReplicaRestrictedDefrag = true;
            }
            
            return false;
        });
        
        if (isBeneficialReplica)
        {
            beneficialReplicas_.push_back(replica);
            if (isBeneficialReplicaForDefrag)
            {
                beneficialDefragReplicas_.push_back(replica);
            }
            if (isBeneficialReplicaRestrictedDefrag)
            {
                beneficialRestrictedDefragReplicas_.push_back(replica);
            }

        }

        if (replica->IsPrimary && replica->Partition->SecondaryReplicaCount > 0)
        {
            swappablePrimaryReplicas_.push_back(replica);

            if (IsSwapBeneficial(replica))
            {
                beneficialPrimaryReplicas_.push_back(replica);

                if (isBeneficialReplicaForDefrag)
                {
                    beneficialPrimaryDefragReplicas_.push_back(replica);
                }
                if (isBeneficialReplicaRestrictedDefrag)
                {
                    beneficialRestrictedDefragPrimaryReplicas_.push_back(replica);
                }
            }
        }
    }
}

void Placement::ComputeBeneficialTargetNodesPerMetric()
{
    std::map<size_t, std::vector<NodeEntry const*>> beneficialTargetNodesPerMetric;
    vector<LoadBalancingDomainEntry> const& lbDomainEntries = balanceChecker_->LBDomains;
    for (auto it = lbDomainEntries.begin(); it != lbDomainEntries.end(); ++it)
    {
        auto & lbDomain = *it;
        for (size_t metricIndex = 0; metricIndex < lbDomain.MetricCount; metricIndex++)
        {
            auto & metric = lbDomain.Metrics[metricIndex];

            if (!metric.IsBalanced)
            {
                vector<NodeEntry const*> beneficialTargetNodes;
                size_t globalIndex = lbDomain.MetricStartIndex + metricIndex;
                int coefficient = metric.IsDefrag ? (metric.DefragmentationScopedAlgorithmEnabled ? 1 : -1) : 1;
                for (size_t i = 0; i < balanceChecker_->Nodes.size(); i++)
                {
                    auto & node = balanceChecker_->Nodes[i];
                    auto & loadStat = lbDomain.GetLoadStat(metricIndex);

                    double loadDiff;
                    if (metric.BalancingByPercentage)
                    {
                        double average = loadStat.AbsoluteSum / loadStat.CapacitySum;
                        if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
                        {
                            average = loadStat.AbsoluteSum / (loadStat.CapacitySum - metric.DefragNodeCount * metric.ReservationLoad);
                        }

                        auto nodeCapacity = node.GetNodeCapacity(metric.IndexInGlobalDomain);
                        auto nodeLoad = node.GetLoadLevel(globalIndex);

                        loadDiff = nodeLoad - nodeCapacity * average;
                    }
                    else
                    {
                        double average = loadStat.Average;
                        if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
                        {
                            // Since we want to empty out a certain number of nodes,
                            // the amount of load we want to have on all nodes changes
                            // (we try to distribute the load on all nodes, except on the empty nodes).
                            // Here we calculate the total load from the average and use it to calculate the desired load (new average)
                            average = loadStat.Average * loadStat.Count / (loadStat.Count - metric.DefragNodeCount);
                        }
                        loadDiff = node.GetLoadLevel(globalIndex) - average;
                    }

                    if (!node.IsDeactivated && node.IsUp && metric.IsValidNode(node.NodeIndex))
                    {
                        // If scoped defrag is enabled, we select all nodes to be able to do balancing if needed
                        if (coefficient * loadDiff < coefficient * -1e-9 || metric.DefragmentationScopedAlgorithmEnabled)
                        {
                            beneficialTargetNodes.push_back(&node);
                        }
                    }
                }

                if (!beneficialTargetNodes.empty())
                {
                    beneficialTargetNodesPerMetric.insert(make_pair(globalIndex, move(beneficialTargetNodes)));
                }
            }
        }
    }

    for (auto it = beneficialTargetNodesPerMetric.begin(); it != beneficialTargetNodesPerMetric.end(); ++it)
    {
        NodeSet & nodeSet = beneficialTargetNodesPerMetric_.insert(make_pair(it->first, NodeSet(this, false))).first->second;
        auto & nodes = it->second;
        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            nodeSet.Add(*nodeIt);
        }
    }

}

void Placement::ComputeBeneficialTargetNodesForPlacementPerMetric()
{
    std::map<size_t, std::vector<NodeEntry const*>> beneficialTargetNodesForPlacementPerMetric;
    vector<LoadBalancingDomainEntry> const& lbDomainEntries = balanceChecker_->LBDomains;

    auto& globalLBDomainEntry = balanceChecker_->LBDomains.back();
    std::map<std::wstring, size_t> metricNameToGlobalIndex;
    bool shouldCaluclateBeneficialNodes = false;
    for (size_t i = 0; i < globalLBDomainEntry.MetricCount; i++)
    {
        metricNameToGlobalIndex.insert(
            make_pair(globalLBDomainEntry.Metrics[i].Name,
                i));

        if (globalLBDomainEntry.Metrics[i].ShouldCalculateBeneficialNodesForPlacement)
        {
            shouldCaluclateBeneficialNodes = true;
        }
    }

    // Check if we have defrag metric before iterating over all domains 
    if (!shouldCaluclateBeneficialNodes)
    {
        return;
    }

    for (auto it = lbDomainEntries.begin(); it != lbDomainEntries.end(); ++it)
    {
        auto & lbDomain = *it;
        for (size_t metricIndex = 0; metricIndex < lbDomain.MetricCount; metricIndex++)
        {
            auto & metric = lbDomain.Metrics[metricIndex];

            // Run calculation only for defrag metrics with defined config parameters
            if (metric.ShouldCalculateBeneficialNodesForPlacement)
            {
                size_t globalMetricStartIndex = TotalMetricCount - GlobalMetricCount;
                size_t totalIndex = lbDomain.MetricStartIndex + metricIndex;
                auto itCapacityIndex = metricNameToGlobalIndex.find(metric.Name);
                ASSERT_IF(itCapacityIndex == metricNameToGlobalIndex.end(), "Global metric index should exist");
                size_t capacityIndex = itCapacityIndex->second;

                vector<NodeEntry> const& nodes = balanceChecker_->Nodes;
                vector<NodeEntry const*> beneficialTargetNodesForPlacement;

                vector<bool> isConsideredBeneficialForScopedDefrag;
                isConsideredBeneficialForScopedDefrag.reserve(nodes.size());

                auto & dynamicNodeLoads = balanceChecker_->DynamicNodeLoads;
                if (metric.DefragmentationScopedAlgorithmEnabled)
                {
                    // Discarding the old beneficial nodes
                    dynamicNodeLoads.AdvanceVersion();

                    // Preparing the nodes which are now beneficial (for this metric)
                    dynamicNodeLoads.PrepareBeneficialNodes(totalIndex,
                        metric.DefragNodeCount,
                        metric.DefragDistribution,
                        metric.ReservationLoad);
                }

                vector<NodeEntry const*> sortedNodes;
                int64 totalEmptyLoad = 0;

                // Filter avaliable nodes for placement and calculate empty space
                sortedNodes.reserve(nodes.size());
                for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
                {
                    // although we allow placement of some services on deactivated nodes while safety checks are in progress
                    // we eliminate deactivated nodes as most of the services should not be placed on them 
                    // and in case of unsuccessful placement on beneficial nodes we fallback to all nodes.
                    if (nodes[nodeIndex].IsDeactivated || !nodes[nodeIndex].IsUp || !metric.IsValidNode(nodes[nodeIndex].NodeIndex))
                    {
                        isConsideredBeneficialForScopedDefrag.push_back(false);
                        continue;
                    }

                    if (nodes[nodeIndex].TotalCapacities.Values[capacityIndex] == -1)
                    {
                        // Always add a node which doesn't have capacity as it is a good node for packing
                        beneficialTargetNodesForPlacement.push_back(&nodes[nodeIndex]);

                        // Exclude node to not be added again later on
                        if (metric.DefragmentationScopedAlgorithmEnabled)
                        {
                            isConsideredBeneficialForScopedDefrag.push_back(true);
                        }

                        continue;
                    }
                    else if (nodes[nodeIndex].TotalCapacities.Values[capacityIndex] != 0)
                    {
                        int64 emptyLoad = nodes[nodeIndex].TotalCapacities.Values[capacityIndex] - nodes[nodeIndex].GetLoadLevel(totalIndex);
                        if (emptyLoad > 0)
                        {
                            // In case of scoped defrag, only include non-empty nodes
                            // Otherwise, include all empty space
                            if (!metric.DefragmentationScopedAlgorithmEnabled || dynamicNodeLoads.IsBeneficialNode(nodeIndex, totalIndex))
                            {
                                totalEmptyLoad += emptyLoad;
                            }

                            sortedNodes.push_back(&nodes[nodeIndex]);
                        }
                    }

                    if (metric.DefragmentationScopedAlgorithmEnabled && dynamicNodeLoads.IsBeneficialNode(nodeIndex, totalIndex))
                    {
                        isConsideredBeneficialForScopedDefrag.push_back(true);
                    }
                    else
                    {
                        isConsideredBeneficialForScopedDefrag.push_back(false);
                    }
                }

                // Calculate incoming new replicas load
                int64 incomingReplicaLoad = 0;
                int64 minimumReplicaLoad = -1;
                int64 maximumServiceReplicaCount = 0;

                for (auto replicaIt = newReplicas_.begin(); replicaIt != newReplicas_.end(); ++replicaIt)
                {
                    auto replica = *replicaIt;
                    if (&globalLBDomainEntry != &lbDomain && replica->Partition->Service->LBDomain != &lbDomain)
                    {
                        continue;
                    }

                    auto replicaEntry = replica->Partition->GetReplicaEntry(replica->Role);
                    int64 currentReplicaLoad = PlacementReplica::GetReplicaLoadValue(replica->Partition, replicaEntry, capacityIndex, globalMetricStartIndex);
                    incomingReplicaLoad += currentReplicaLoad;
                    
                    if (minimumReplicaLoad == -1 || currentReplicaLoad < minimumReplicaLoad)
                    {
                        minimumReplicaLoad = currentReplicaLoad;
                    }

                    // Find the service with maximum number of target replicas. When calculating beneficial nodes for defragmentation there will be at least
                    // maximumServiceReplicaCount beneficial nodes (so all replicas can be placed - since replicas of the same service cannot go on the same node).
                    // Ignore services which have TargetReplicaSetSize = -1 (which means to be placed on all nodes)
                    int64 currentServiceReplicaCount = replica->Partition->NewReplicaCount;
                    if (maximumServiceReplicaCount < currentServiceReplicaCount)
                    {
                        maximumServiceReplicaCount = currentServiceReplicaCount;
                    }
                }

                int64 replicasIncomingBufferedLoad = static_cast<int64>(static_cast<double>(incomingReplicaLoad) * metric.PlacementHeuristicIncomingLoadFactor);
                int64 emptySpaceLoad = static_cast<int64>(static_cast<double>(totalEmptyLoad) * metric.PlacementHeuristicEmptySpacePercent);
                int64 goalEmptyLoad = max(replicasIncomingBufferedLoad, emptySpaceLoad);

                // Choose beneficial nodes for scoped defrag with packing placement strategy or old defrag
                if (!metric.DefragmentationScopedAlgorithmEnabled ||
                    (metric.DefragmentationScopedAlgorithmEnabled && metric.placementStrategy == Metric::PlacementStrategy::ReservationAndPack))
                {
                    // shuffle the nodes so we can randomize node choosing in order to increase the chances to take nodes from different domains
                    auto myFunc = [&](size_t n) -> int
                    {
                        return random_.Next(static_cast<int>(n));
                    };
                    random_shuffle(sortedNodes.begin(), sortedNodes.end(), myFunc);

                    // sort the nodes
                    stable_sort(sortedNodes.begin(), sortedNodes.end(), [&](NodeEntry const* n1, NodeEntry const* n2) -> bool
                    {
                        return n1->GetLoadLevel(totalIndex) > n2->GetLoadLevel(totalIndex);
                    });

                    // Select most loaded nodes with empty space greater than goalEmptyLoad
                    // And make sure that there are at least maximumServiceReplicaCount number of nodes (so all replicas can be placed)
                    int64 currentEmptyLoad = 0;
                    for (size_t i = 0; i < sortedNodes.size() && (currentEmptyLoad < goalEmptyLoad ||
                        static_cast<int64>(beneficialTargetNodesForPlacement.size()) < maximumServiceReplicaCount); i++)
                    {
                        auto node = sortedNodes[i];

                        // If empty load on this node is not enough for minimum replica load to be placed, then this node is not beneficial
                        if ((node->TotalCapacities.Values[capacityIndex] - node->GetLoadLevel(totalIndex) < minimumReplicaLoad) ||
                             isConsideredBeneficialForScopedDefrag[node->NodeIndex]) 
                        {
                            continue;
                        }

                        beneficialTargetNodesForPlacement.push_back(node);

                        currentEmptyLoad += node->TotalCapacities.Values[capacityIndex] - node->GetLoadLevel(totalIndex);
                    }
                }
                else if (metric.placementStrategy == Metric::PlacementStrategy::ReservationAndBalance || metric.placementStrategy == Metric::PlacementStrategy::Reservation)
                {
                    bool considerHeuristicsFactor = (goalEmptyLoad > 0);
                    int64 prevNodeLoad = -1;
                    int64 currentEmptySpace = 0;
                    int selectedNodeCount = 0;

                    dynamicNodeLoads.ForEachNodeOrdered(totalIndex,
                        DynamicNodeLoadSet::Order::Ascending,
                        [&](int nodeIndex, const LoadEntry* loadEntry)
                    {
                        bool continueSearchingForNodes = true;

                        if (!isConsideredBeneficialForScopedDefrag[nodeIndex])
                        {
                            int64 currentNodeLoad = loadEntry->Values[totalIndex];

                            // Choose beneficial nodes for balancing placement strategy
                            // Accumulate the space which can be filled while still respecting balancing
                            // Choose nodes until there is enough space to place all replicas and
                            // until there are at least maximumServiceReplicaCount number of nodes (so all replicas can be placed)
                            // With the addition of each node we also get the space above all previously selected nodes
                            // since the beneficial space for balancing includes space of the selected nodes up to
                            // the load of the next node, by load
                            if (metric.placementStrategy == Metric::PlacementStrategy::ReservationAndBalance)
                            {
                                if (prevNodeLoad != -1)
                                {
                                    currentEmptySpace += (currentNodeLoad - prevNodeLoad) * selectedNodeCount;
                                }

                                if (!considerHeuristicsFactor || currentEmptySpace < goalEmptyLoad ||
                                    static_cast<int64>(beneficialTargetNodesForPlacement.size()) < maximumServiceReplicaCount)
                                {
                                    beneficialTargetNodesForPlacement.push_back(&nodes[nodeIndex]);
                                    ++selectedNodeCount;
                                }
                                else
                                {
                                    continueSearchingForNodes = false;
                                }
                            }
                            // Choose beneficial nodes for no preference placement strategy
                            else if (metric.placementStrategy == Metric::PlacementStrategy::Reservation)
                            {
                                beneficialTargetNodesForPlacement.push_back(&nodes[nodeIndex]);
                            }

                            prevNodeLoad = currentNodeLoad;
                        }

                        return continueSearchingForNodes;
                    });
                }

                if (!beneficialTargetNodesForPlacement.empty())
                {
                    beneficialTargetNodesForPlacementPerMetric.insert(make_pair(totalIndex, move(beneficialTargetNodesForPlacement)));
                }
            }
        }
    }

    for (auto it = beneficialTargetNodesForPlacementPerMetric.begin(); it != beneficialTargetNodesForPlacementPerMetric.end(); ++it)
    {
        NodeSet & nodeSet = beneficialTargetNodesForPlacementPerMetric_.insert(make_pair(it->first, NodeSet(this, false))).first->second;
        auto & nodes = it->second;
        for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
        {
            nodeSet.Add(*nodeIt);
        }
    }
}

bool Placement::IsSwapBeneficial(PlacementReplica const* replica)
{
    // Check if replica swap with some of the secondaries or primary replica (in case replica is secondary) will help the balancing.
    PartitionEntry const* partition = replica->Partition;

    LoadEntry const& primaryEntry = partition->GetReplicaEntry(ReplicaRole::Primary);
    LoadEntry const& secondaryEntry = replica->IsPrimary ?
        partition->GetReplicaEntry(ReplicaRole::Secondary) :
        partition->GetReplicaEntry(ReplicaRole::Secondary, true, replica->Node->NodeId);

    size_t metricCount = partition->Service->MetricCount;
    ASSERT_IFNOT(metricCount == primaryEntry.Values.size() && metricCount == secondaryEntry.Values.size(), "Metric count not same");

    bool isSwapBeneficial = false;
    for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
    {
        int64 primaryLoad = primaryEntry.Values[metricIndex];
        int64 secondaryLoad = secondaryEntry.Values[metricIndex];

        if (primaryLoad == secondaryLoad)
        {
            continue;
        }
        else
        {
            int coefficient = primaryLoad > secondaryLoad ? 1 : -1;
            coefficient *= replica->IsPrimary ? 1 : -1;

            LoadBalancingDomainEntry const* lbDomain = partition->Service->LBDomain;
            if (lbDomain != nullptr)
            {
                size_t totalMetricIndex = lbDomain->MetricStartIndex + metricIndex;
                auto metric = lbDomain->Metrics[metricIndex];
                auto loadStat = lbDomain->GetLoadStat(metricIndex);
                int64 defragCoefficient = metric.IsDefrag ? -1 : 1;
                int64 tempCoefficient = coefficient * defragCoefficient;
                double loadDiff;
                if (metric.BalancingByPercentage)
                {
                    double average = loadStat.AbsoluteSum / loadStat.CapacitySum;
                    auto nodeCapacity = replica->Node->GetNodeCapacity(metric.IndexInGlobalDomain);
                    auto nodeLoad = replica->Node->GetLoadLevel(totalMetricIndex);

                    loadDiff = nodeLoad - nodeCapacity * average;
                }
                else
                {
                    loadDiff = replica->Node->GetLoadLevel(totalMetricIndex) -
                        loadStat.Average;
                }

                if (!lbDomain->Metrics[metricIndex].IsBalanced && loadDiff * tempCoefficient > defragCoefficient * 1e-9)
                {
                    isSwapBeneficial = true;
                    break;
                }
            }

            size_t totalMetricIndexInGlobalDomain = partition->Service->GlobalMetricIndices[metricIndex];
            LoadBalancingDomainEntry const* globalDomain = partition->Service->GlobalLBDomain;
            size_t metricIndexInGlobalDomain = totalMetricIndexInGlobalDomain - globalDomain->MetricStartIndex;
            auto metric = globalDomain->Metrics[metricIndexInGlobalDomain];
            auto loadStat = globalDomain->GetLoadStat(metricIndexInGlobalDomain);
            int64 defragCoefficient = metric.IsDefrag ? -1 : 1;
            int64 tempCoefficient = coefficient * defragCoefficient;

            double loadDiff;
            if (metric.BalancingByPercentage)
            {
                double average = loadStat.AbsoluteSum / loadStat.CapacitySum;
                auto nodeCapacity = replica->Node->GetNodeCapacity(metric.IndexInGlobalDomain);
                auto nodeLoad = replica->Node->GetLoadLevel(totalMetricIndexInGlobalDomain);

                loadDiff = nodeLoad - nodeCapacity * average;
            }
            else
            {
                loadDiff = replica->Node->GetLoadLevel(totalMetricIndexInGlobalDomain) - loadStat.Average;
            }

            if (!globalDomain->Metrics[metricIndexInGlobalDomain].IsBalanced && loadDiff * tempCoefficient > defragCoefficient * 1e-9)
            {
                isSwapBeneficial = true;
                break;
            }
        }
    }

    return isSwapBeneficial;
}

void Placement::CreateReplicaPlacement()
{
    for (auto it = allReplicas_.begin(); it != allReplicas_.end(); ++it)
    {
        auto r = it->get();
        if (r->IsNew)
        {
            continue;
        }

        NodeEntry const* n = r->Node;

        partitionPlacements_[r->Partition].Add(n, r);

        ApplicationEntry const* app = r->Partition->Service->Application;
        if (app && app->HasScaleoutOrCapacity)
        {
            if (!r->ShouldDisappear && r->Role != ReplicaRole::Enum::Dropped && r->Role != ReplicaRole::Enum::None)
            {
                applicationPlacements_[app][n].Add(r);
            }
        }

        ServicePackageEntry const* servicePackage = r->Partition->Service->ServicePackage;
        if (nullptr != servicePackage)
        {
            servicePackagePlacements_.AddReplicaToNode(servicePackage, r->Node, r);
        }
    }

    for (auto it = standByReplicas_.begin(); it != standByReplicas_.end(); ++it)
    {
        auto r = it->get();
        NodeEntry const* n = r->Node;

        ApplicationEntry const* app = r->Partition->Service->Application;
        if (app && app->HasScaleoutOrCapacity)
        {
            if (!r->ShouldDisappear)
            {
                applicationPlacements_[app][n].Add(r);
            }
        }

        ServicePackageEntry const* servicePackage = r->Partition->Service->ServicePackage;
        if (nullptr != servicePackage)
        {
            servicePackagePlacements_.AddReplicaToNode(servicePackage, r->Node, r);
        }
    }
}

void Placement::CreateNodePlacement()
{
    for (auto it = allReplicas_.begin(); it != allReplicas_.end(); ++it)
    {
        auto r = it->get();
        if (r->IsNew)
        {
            continue;
        }

        NodeEntry const* n = r->Node;
        if (n->HasCapacity || n->IsThrottled)
        {
            nodePlacements_[n].Add(r);
        }
    }

    for (auto it = standByReplicas_.begin(); it != standByReplicas_.end(); ++it)
    {
        auto r = it->get();
        NodeEntry const* n = r->Node;

        if (n->HasCapacity || n->IsThrottled)
        {
            nodePlacements_[n].Add(r);
        }
    }
}

void Placement::PrepareApplications()
{
    ASSERT_IFNOT(TotalMetricCount >= GlobalMetricCount, "Invalid metric count");

    size_t globalMetricCount = GlobalMetricCount;
    ASSERT_IFNOT(TotalMetricCount >= globalMetricCount, "Invalid metric count");

    for (ApplicationEntry & application : applications_)
    {
        auto const& applicationNodeLoads = application.NodeLoads;

        std::map<NodeEntry const*, LoadEntry> appNodeLoads;

        for (auto nodeLoad : applicationNodeLoads)
        {
            NodeEntry const* node = nodeLoad.first;

            LoadEntry load(GlobalMetricCount);

            for (int i = 0; i < nodeLoad.second.Length; ++i)
            {
                load.Set(i, nodeLoad.second.Values[i]);
            }

            appNodeLoads.insert(make_pair(node, move(load)));
        }

        // Add application entry total load
        applicationTotalLoad_.SetLoad(&application);

        auto const& applicationNodeCounts = application.NodeCounts;
        for (auto nodeCount : applicationNodeCounts)
        {
            NodeEntry const* node = nodeCount.first;
            applicationNodeCount_.SetCount(&application, node, nodeCount.second);
        }

        applicationNodeLoads_.SetNodeLoadsForApplication(&application, move(appNodeLoads));

        // If application has singleton replicas in upgrade,
        // and appropriate configuration is set,
        // then relax the scaleout to 2, and double the application capacity
        if (application.HasPartitionsInSingletonReplicaUpgrade &&
            settings_.RelaxScaleoutConstraintDuringUpgrade)
        {
            application.ScaleoutCount = 2;
            for (int i = 0; i < application.AppCapacities.Length; i++)
            {
                const_cast<LoadEntry&>(application.AppCapacities).Set(i, application.AppCapacities.Values[i] * 2);
            }
            application.SetRelaxedScaleoutReplicaSet(this);
        }
    }

    if (settings_.IsTestMode && partitionClosureType_ == PartitionClosureType::Enum::Full)

    {
        VerifyApplicationEntries();
    }
}

bool Placement::CanNewReplicasBePlaced() const
{
    for (size_t i = 0; i < NewReplicaCount; ++i)
    {
        if (!newReplicas_[i]->Partition->Service->OnEveryNode)
        {
            return true;
        }
    }

    return false;
}

void Placement::VerifyApplicationEntries()
{
    // In test mode we will calculate reserved load and compare it to the one we got from ServiceDomain.

    std::map<NodeEntry const*, LoadEntry> appsReservedLoad;
    ApplicationReservedLoad controlApplicationReservedLoads(GlobalMetricCount);

    for (ApplicationEntry & application : applications_)
    {
        NodeMetrics const& appNodeMetrics = applicationNodeLoads_[&application];

        auto const& applicationNodeCounts = application.NodeCounts;
        for (auto nodeCount : applicationNodeCounts)
        {
            NodeEntry const* node = nodeCount.first;
            if (nodeCount.second > 0 && application.HasReservation)
            {
                // Calculate reservation here.
                LoadEntry const& appNodeLoad = appNodeMetrics[node];

                LoadEntry appMinNodeLoadDiff(GlobalMetricCount);
                bool hasReservedLoad = false;

                for (int i = 0; i < appNodeLoad.Length; ++i)
                {
                    // calculate the effective load reservation
                    int64 minLoadDiff = application.GetReservationDiff(i, appNodeLoad.Values[i]);
                    if (minLoadDiff > 0)
                    {
                        appMinNodeLoadDiff.AddLoad(i, minLoadDiff);
                        hasReservedLoad = true;
                    }
                }

                if (hasReservedLoad)
                {
                    auto nodeLoadIt = appsReservedLoad.find(node);
                    if (nodeLoadIt == appsReservedLoad.end())
                    {
                        appsReservedLoad.insert(make_pair(node, move(appMinNodeLoadDiff)));
                    }
                    else
                    {
                        nodeLoadIt->second += move(appMinNodeLoadDiff);
                    }
                }
            }
        }
    }

    for (auto it : appsReservedLoad)
    {
        controlApplicationReservedLoads.Set(it.first, move(it.second));
    }

    ASSERT_IF(applicationReservedLoads_.DataSize() != controlApplicationReservedLoads.DataSize(), "Sizes do not match");

    applicationReservedLoads_.ForEach([&](std::pair<NodeEntry const*, LoadEntry> const& pair)
    {
        ASSERT_IFNOT(controlApplicationReservedLoads.HasKey(pair.first), "Key does not exist.");

        LoadEntry const& controlLoadEntry = controlApplicationReservedLoads[pair.first];

        ASSERT_IFNOT(controlLoadEntry == pair.second, "LoadEntry different.");

        return true;
    });
}

