// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <numeric>
#include "PLBConfig.h"
#include "PlacementCreator.h"
#include "AccumulatorWithMinMax.h"
#include "ServiceDomain.h"
#include "PlacementAndLoadBalancing.h"
#include "Service.h"
#include "BalanceChecker.h"
#include "Placement.h"
#include "NodeSet.h"
#include "DiagnosticsDataStructures.h"
#include "BalanceCheckerCreator.h"
#include "ApplicationReservedLoads.h"
#include "ServicePackageNode.h"
#include "InBuildCountPerNode.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PlacementCreator::PlacementCreator(
    PlacementAndLoadBalancing const& plb,
    ServiceDomain const& serviceDomain,
    BalanceCheckerUPtr && balanceChecker,
    set<Guid> && throttledPartitions,
    PartitionClosureUPtr const& partitionClosure)
    : plb_(plb),
    serviceDomain_(serviceDomain),
    balanceChecker_(move(balanceChecker)),
    throttledPartitions_(move(throttledPartitions)),
    partitionClosure_(partitionClosure),
    partitionsInUpgradeCount_(0),
    reservationLoads_(balanceChecker_->LBDomains.back().MetricCount),
    quorumBasedServicesCount_(0),
    quorumBasedPartitionsCount_(0)
{
    // Prepare global metric indexes.
    auto& globalLBDomainEntry = balanceChecker_->LBDomains.back();
    for (size_t i = 0; i < globalLBDomainEntry.MetricCount; i++)
    {
        metricNameToGlobalIndex_.insert(
            make_pair(globalLBDomainEntry.Metrics[i].Name,
            i + globalLBDomainEntry.MetricStartIndex));
    }
}

void PlacementCreator::CalculateApplicationNodeLoads(
    ApplicationLoad const& appLoad,
    std::map<NodeEntry const*, LoadEntry> & loadsToCalculate,
    bool disappearingLoads)
{
    auto const& globalLBDomainEntry = balanceChecker_->LBDomains.back();
    int metricIndex = 0;
    for (auto itMetric = serviceDomain_.Metrics.begin(); itMetric != serviceDomain_.Metrics.end(); ++itMetric)
    {
        auto const& itMetricLoad = appLoad.MetricLoads.find(itMetric->first);
        if (itMetricLoad != appLoad.MetricLoads.end())
        {
            auto const& nodeLoads =
                disappearingLoads
                ? itMetricLoad->second.ShouldDisappearNodeLoads
                : itMetricLoad->second.NodeLoads;

            for (auto nodeLoad : nodeLoads)
            {
                auto nodeIndex = plb_.GetNodeIndex(nodeLoad.first);
                if (nodeIndex == UINT64_MAX)
                {
                    continue;
                }
                NodeEntry const* node = &(balanceChecker_->Nodes[nodeIndex]);
                auto it = loadsToCalculate.find(node);
                if (it == loadsToCalculate.end())
                {
                    it = loadsToCalculate.insert(make_pair(node, LoadEntry(globalLBDomainEntry.MetricCount))).first;
                }
                it->second.Set(metricIndex, nodeLoad.second);
            }
        }
        metricIndex++;
    }
}

void PlacementCreator::CreateApplicationEntries()
{
    auto& globalLBDomainEntry = balanceChecker_->LBDomains.back();

    // MSVC++ does not support reserve, so doing rehash on an empty container
    applicationEntryDict_.rehash(plb_.applicationTable_.size());
    applicationEntries_.reserve(plb_.applicationTable_.size());

    for (Application const* app : partitionClosure_->Applications)
    {
        uint64 applicationId = app->ApplicationDesc.ApplicationId;
        ApplicationDescription const& appDesc = app->ApplicationDesc;

        LoadEntry appTotalCapacity(globalLBDomainEntry.MetricCount, -1LL);
        LoadEntry perNodeCapacity(globalLBDomainEntry.MetricCount, -1LL);
        LoadEntry reservationCapacity(globalLBDomainEntry.MetricCount, -1LL);

        for (size_t j = 0; j < globalLBDomainEntry.MetricCount; j++)
        {
            auto &globalMetricName = globalLBDomainEntry.Metrics[j].Name;
            auto itCapacities = appDesc.AppCapacities.find(globalMetricName);
            if (itCapacities != appDesc.AppCapacities.end())
            {
                appTotalCapacity.Set(j, itCapacities->second.TotalCapacity);
                perNodeCapacity.Set(j, itCapacities->second.MaxInstanceCapacity);
                reservationCapacity.Set(j, itCapacities->second.ReservationCapacity);
            }
        }

        std::map<NodeEntry const*, LoadEntry> activeLoads;
        std::map<NodeEntry const*, LoadEntry> disappearingLoads;
        std::map<NodeEntry const*, size_t> nodeCounts;

        LoadEntry totalLoad(globalLBDomainEntry.MetricCount);

        if (serviceDomain_.ApplicationHasLoad(applicationId))
        {
            ApplicationLoad const& appLoad = serviceDomain_.GetApplicationLoad(applicationId);
            CalculateApplicationNodeLoads(appLoad, activeLoads, false);

            for (auto nodeLoad : activeLoads)
            {
                totalLoad += nodeLoad.second;
            }

            CalculateApplicationNodeLoads(appLoad, disappearingLoads, true);

            for (auto nodeCount : appLoad.NodeCounts)
            {
                auto nodeIndex = plb_.GetNodeIndex(nodeCount.first);
                if (nodeIndex == UINT64_MAX)
                {
                    continue;
                }

                nodeCounts.insert(make_pair(&(balanceChecker_->Nodes[nodeIndex]), nodeCount.second));
            }
        }

        //Convert the ud string to tree index
        std::set<Common::TreeNodeIndex> udIndex;
        if (!app->UpgradeCompletedUDs.empty())
        {
            auto itAppUDs = balanceChecker_->ApplicationUpgradedUDs.find(app->ApplicationDesc.ApplicationId);
            if (itAppUDs != balanceChecker_->ApplicationUpgradedUDs.end())
            {
                udIndex = move(itAppUDs->second);
            }
        }

        applicationEntries_.push_back(ApplicationEntry(
            wstring(appDesc.Name),
            applicationId,
            app->ApplicationDesc.ScaleoutCount,
            move(appTotalCapacity),
            move(perNodeCapacity),
            move(reservationCapacity),
            move(totalLoad),
            move(activeLoads),
            move(disappearingLoads),
            move(nodeCounts),
            move(udIndex)
            ));

        ApplicationEntry & applicationEntry = applicationEntries_.back();
        applicationEntryDict_.insert(make_pair(applicationId, &applicationEntry));
    }
}

void PlacementCreator::CreateApplicationReservedLoads()
{
    for (auto const& reservedIt : serviceDomain_.reservationLoadTable_)
    {
        wstring const& metricName = reservedIt.first;
        size_t metricIndex = metricNameToGlobalIndex_[metricName] - balanceChecker_->LBDomains.back().MetricStartIndex;

        for (auto const& nodeLoadIt : reservedIt.second.NodeReservedLoad)
        {
            if (nodeLoadIt.second > 0)
            {
                uint64 nodeIndex = plb_.GetNodeIndex(nodeLoadIt.first);
                if (nodeIndex < balanceChecker_->Nodes.size())
                {
                    NodeEntry const* node = &(balanceChecker_->Nodes[nodeIndex]);
                    LoadEntry & load = reservationLoads_[node];
                    load.Set(metricIndex, nodeLoadIt.second);
                }
            }
        }
    }
}

void PlacementCreator::PrepareNodes()
{
    auto nodeIBCounts = serviceDomain_.InBuildCountPerNode;
    for (auto nodeCount : nodeIBCounts)
    {
        if (nodeCount.first >= balanceChecker_->Nodes.size())
        {
            serviceDomain_.plb_.Trace.InternalError(wformatString(
                "PlacementCreator::PrepareNodes() Node with index {0} not found.",
                nodeCount.first));
            continue;
        }
        NodeEntry const* node = &(balanceChecker_->Nodes[nodeCount.first]);
        inBuildCountPerNode_.SetCount(node, nodeCount.second);
    }
}

void PlacementCreator::CreateServicePackageEntries()
{
    servicePackageEntries_.reserve(partitionClosure_->ServicePackages.size());

    for (auto servicePackage : partitionClosure_->ServicePackages)
    {
        auto itServicePackage = plb_.servicePackageToIdMap_.find(servicePackage->Description.ServicePackageIdentifier);

        if (itServicePackage != plb_.servicePackageToIdMap_.end())
        {
            TESTASSERT_IF(servicePackage->Description.RequiredResources.size() == 0 &&
                servicePackage->Description.ContainersImages.size() == 0,
                "Service Package {0} should have some resources defined or some container images or it should not be in closure.",
                servicePackage->Description);
            LoadEntry resources(balanceChecker_->TotalMetricCount);
            for (auto requiredResource : servicePackage->Description.CorrectedRequiredResources)
            {
                auto itMetricNameToIndex = metricNameToGlobalIndex_.find(requiredResource.first);
                if (itMetricNameToIndex != metricNameToGlobalIndex_.end())
                {
                    size_t totalMetricIndex = itMetricNameToIndex->second;
                    resources.Set(totalMetricIndex, requiredResource.second);
                }
                else
                {
                    TESTASSERT_IF(itMetricNameToIndex == metricNameToGlobalIndex_.end(),
                        "Metric {0} not found in the name to index mapping", requiredResource.first);
                }
            }

            // Get all container images for this SP
            auto containerImages = servicePackage->Description.ContainersImages;

            servicePackageEntries_.push_back(ServicePackageEntry(
                servicePackage->Description.Name,
                move(resources),
                move(containerImages)));
            servicePackageDict_.insert(make_pair(itServicePackage->second, &(servicePackageEntries_.back())));

            //we setup the initial replica counts on the nodes
            for (auto itNode = balanceChecker_->Nodes.begin(); itNode != balanceChecker_->Nodes.end(); ++itNode)
            {
                auto itPlbServicePackages = serviceDomain_.servicePackageReplicaCountPerNode_.find(itNode->NodeId);
                if (itPlbServicePackages != serviceDomain_.servicePackageReplicaCountPerNode_.end())
                {
                    auto itPlbServicePackage = itPlbServicePackages->second.find(itServicePackage->second);
                    if (itPlbServicePackage != itPlbServicePackages->second.end())
                    {
                        //we just add the shared replicas here, exclusive services have their own load set already
                        int totalReplicaCount = itPlbServicePackage->second.GetSharedServiceReplicaCount();
                        servicePackagePlacement_.SetCountToNode(&(servicePackageEntries_.back()), &(*itNode), totalReplicaCount);
                    }
                }
            }
        }
    }
}

void PlacementCreator::CreateServiceEntries()
{
    vector<LoadBalancingDomainEntry> const& lbDomains = balanceChecker_->LBDomains;

    auto& globalLBDomainEntry = lbDomains.back();

    // Create service entries and set the metrics, LB domains and block lists
    vector<Service const*> const& services = partitionClosure_->Services;

    // MSVC++ does not support reserve, hence using rehash explicitly on empty container.
    serviceEntryDict_.rehash(services.size());
    serviceEntries_.reserve(services.size());

    LoadBalancingDomainEntry const* nextLBDomain = lbDomains.data();
    for (size_t i = 0; i < services.size(); i++)
    {
        Service const* service = services[i];

        ServiceDescription const& serviceDesc = service->ServiceDesc;

        vector<ServiceMetric> const& metrics = serviceDesc.Metrics;
        vector<size_t> globalMetricIndices(metrics.size());
        bool metricHasCapacity = false;

        for (size_t m = 0; m < metrics.size(); m++)
        {
            auto it = metricNameToGlobalIndex_.find(metrics[m].Name);
            ASSERT_IF(it == metricNameToGlobalIndex_.end(),
                "Metric {0} does not exist in metricNameToGlobalIndex for service {1}",
                metrics[m].Name,
                serviceDesc.Name);

            if (!metricHasCapacity &&
                balanceChecker_->MetricsWithNodeCapacity.find(metrics[m].Name) != balanceChecker_->MetricsWithNodeCapacity.end())
            {
                metricHasCapacity = true;
            }

            globalMetricIndices[m] = it->second;
        }

        size_t fdDomainCount = 0;
        size_t fdNodeCount = 0;
        BalanceChecker::DomainTree faultDomainStructure = FilterAndCalculateDomainStructure(fdDomainCount, fdNodeCount, true, service);

        size_t udDomainCount = 0;
        size_t udNodeCount = 0;
        BalanceChecker::DomainTree upgradeDomainStructure = FilterAndCalculateDomainStructure(udDomainCount, udNodeCount, false, service);

        //TESTASSERT_IF(fdNodeCount != udNodeCount,
        //    "Number of FD nodes {0} and number of UD nodes {1} for the service {2} should match",
        //    fdNodeCount, udNodeCount, serviceDesc.Name);

        // If auto-scaling is turned on, then service will always respect quorum based FD/UD distribution.
        // Otherwise, calculating if switch from '+1' to quorum based logic should be done (cases when '+1' logic is too strict and new nodes can't be utilized)
        // Calculation is determined by formula: 
        //     target replica set size is divisible by both number of FDs/UDs and number of FD/UD nodes is less than a product of number of FDs/UDs
        bool autoSwitchToQuorumBasedLogic = 
            serviceDesc.IsAutoScalingDefined ||
                (plb_.Settings.QuorumBasedLogicAutoSwitch &&
                fdDomainCount != 0 && udDomainCount != 0 &&
                fdNodeCount != 0 && udNodeCount != 0 &&
                serviceDesc.TargetReplicaSetSize != 0 &&
                serviceDesc.TargetReplicaSetSize % fdDomainCount == 0 &&
                serviceDesc.TargetReplicaSetSize % udDomainCount == 0 &&
                fdNodeCount <= fdDomainCount * udDomainCount &&
                udNodeCount <= fdDomainCount * udDomainCount);

        if (autoSwitchToQuorumBasedLogic ||
            plb_.Settings.QuorumBasedReplicaDistributionPerFaultDomains ||
            plb_.Settings.QuorumBasedReplicaDistributionPerUpgradeDomains)
        {
            quorumBasedServicesCount_++;
            quorumBasedServicesTempCache_.insert(serviceDesc.ServiceId);
        }

        ApplicationEntry const* applicationEntry = nullptr;
        if (!applicationEntryDict_.empty() && serviceDesc.ApplicationName != L"")
        {
            auto itApp = applicationEntryDict_.find(serviceDesc.ApplicationId);

            if (itApp != applicationEntryDict_.end())
            {
                applicationEntry = itApp->second;
            }
            else
            {
                auto const& appTableIt = plb_.applicationTable_.find(serviceDesc.ApplicationId);
                if (appTableIt == plb_.applicationTable_.end())
                {
                    plb_.Trace.ApplicationNotFound(serviceDesc.ApplicationName, serviceDesc.ApplicationId, serviceDesc.Name);
                    Common::Assert::TestAssert("Application {0} (id: {1}) not found for service {2}",
                        serviceDesc.ApplicationName,
                        serviceDesc.ApplicationId,
                        serviceDesc.Name);
                }
            }
        }

        ServicePackageEntry const* servicePackage = nullptr;
        //only for shared host services we set the service package
        //for exclusive ones we will set the load in the service metrics
        if (service->ServiceDesc.ServicePackageId != 0 && service->ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::SharedProcess)
        {
            auto spIterator = servicePackageDict_.find(service->ServiceDesc.ServicePackageId);
            if (spIterator != servicePackageDict_.end())
            {
                servicePackage = spIterator->second;
            }
        }

        if (!metricHasCapacity && applicationEntry != nullptr && applicationEntry->HasScaleoutOrCapacity)
        {
            metricHasCapacity = true;
        }

        serviceEntries_.push_back(ServiceEntry(
            wstring(serviceDesc.Name),
            servicePackage,
            serviceDesc.IsStateful,
            serviceDesc.HasPersistedState,
            serviceDesc.TargetReplicaSetSize,
            DynamicBitSet(service->BlockList),
            DynamicBitSet(service->PrimaryReplicaBlockList),
            service->ServiceBlockList.Count,
            move(globalMetricIndices),
            serviceDesc.OnEveryNode,
            service->FDDistribution,
            move(faultDomainStructure),
            move(upgradeDomainStructure),
            move(ServiceEntryDiagnosticsData(wstring(serviceDesc.PlacementConstraints))),
            service->PartiallyPlace,
            applicationEntry,
            metricHasCapacity,
            autoSwitchToQuorumBasedLogic));

        ServiceEntry & serviceEntry = serviceEntries_.back();

        if (static_cast<size_t>(nextLBDomain->ServiceIndex) == i)
        {
            serviceEntry.SetLBDomain(nextLBDomain++, &globalLBDomainEntry);
        }
        else
        {
            serviceEntry.SetLBDomain(nullptr, &globalLBDomainEntry);
        }

        // TODO: is the following code used?
        double weightSum = accumulate(serviceDesc.Metrics.begin(), serviceDesc.Metrics.end(), 0.0,
            [](double sum, ServiceMetric const& metric) { return sum + metric.Weight; });
        ASSERT_IF(serviceEntry.LBDomain == nullptr && !partitionClosure_->Services[i]->IsSingleton && weightSum > 0.0,
            "Nonsingleton service {0} with a positive weight sum {1} does not have a load balancing domain.",
            serviceDesc.Name,
            weightSum);

        serviceEntryDict_.insert(make_pair(serviceDesc.ServiceId, &serviceEntry));
    }

    // set the dependency relationships
    for (size_t i = 0; i < services.size(); i++)
    {
        ServiceDescription const& serviceDesc = services[i]->ServiceDesc;
        wstring const& dependedService = serviceDesc.AffinitizedService;
        if (!dependedService.empty() && !serviceDesc.OnEveryNode)
        {
            ServiceEntry & serviceEntry = serviceEntries_[i];
            auto const& itId = plb_.ServiceIdMap.find(dependedService);
            if (itId != plb_.ServiceIdMap.end())
            {
                auto it = serviceEntryDict_.find(itId->second);
                if (it != serviceEntryDict_.end())
                {
                    serviceEntry.SetDependedService(it->second, serviceDesc.AlignedAffinity);
                    it->second->AddDependentService(&serviceEntry);
                }
                else
                {
                    serviceEntry.set_HasAffinity(true);
                }
            }
            else
            {
                serviceEntry.set_HasAffinity(true);
            }
        }
    }
}

void PlacementCreator::CreatePartitionEntries()
{
    size_t allReplicaCount = 0;
    size_t replicaIndex = 0;
    for (auto it = partitionClosure_->Partitions.begin(); it != partitionClosure_->Partitions.end(); ++it)
    {
        FailoverUnit const& failoverUnit = *(*it);
        int replicaDiff = 0;
        if (failoverUnit.ActualReplicaDifference > 0)
        {
            replicaDiff = min(failoverUnit.ActualReplicaDifference, static_cast<int>(plb_.upNodeCount_));
        }
        allReplicaCount += failoverUnit.FuDescription.Replicas.size() + replicaDiff;
    }

    allReplicas_.reserve(allReplicaCount);
    standByReplicas_.reserve(allReplicaCount);

    bool isCurrentActionConstraintCheck = partitionClosure_->IsForConstraintCheck();

    bool allowConstraintCheckFixesDuringAppUpgrade =
        isCurrentActionConstraintCheck
        && PLBConfig::GetConfig().AllowConstraintCheckFixesDuringApplicationUpgrade;

    for (auto it = partitionClosure_->Partitions.begin(); it != partitionClosure_->Partitions.end(); ++it)
    {
        FailoverUnit const& failoverUnit = *(*it);
        FailoverUnitDescription const& fuDesc = failoverUnit.FuDescription;

        Service const& service = serviceDomain_.GetService(fuDesc.ServiceId);

        ServiceType const& serviceType = plb_.GetServiceType(service.ServiceDesc);

        vector<PlacementReplica *> replicas;
        replicas.reserve(fuDesc.Replicas.size());
        size_t upReplicaCount = 0;
        vector<PlacementReplica *> partitionStandBy;
        vector<NodeEntry const*> standByLocations;
        NodeEntry const* primarySwapOutLocation = nullptr;
        NodeEntry const* primaryUpgradeLocation = nullptr;
        vector<NodeEntry const*> secondaryUpgradeLocations;
        PlacementReplica* primaryReplica = nullptr;

        bool isPartitionInUpgrade = false;
        bool isSwapPrimary = false;
        // Children in affinity relationship cannot be throttled.
        // Only stateful services can be throttled.
        bool canBeThrottled = service.ServiceDesc.AffinitizedService == L"" && service.ServiceDesc.IsStateful;

        for (ReplicaDescription const& replica : fuDesc.Replicas)
        {
            NodeMatchType nodeResult = NodeExistAndMatch(replica.NodeInstance);
            if (nodeResult >= InstanceNotMatch)
            {

                uint64 nodeIndex = plb_.GetNodeIndex(replica.NodeId);
                if (nodeIndex != UINT64_MAX)
                {
                    bool isReplicaInUpgrade = false;
                    bool isReplicaToBeDropped = replica.IsToBeDropped;

                    if (replica.IsUp)
                    {
                        ++upReplicaCount;
                    }

                    // The current replica node may already be de-activated, the nodeResult would be 1
                    if (replica.IsPrimaryToBeSwappedOut && replica.CurrentRole == ReplicaRole::Enum::Primary)
                    {
                        // Ignore the I flag if it is set on secondary
                        primarySwapOutLocation = &(balanceChecker_->Nodes[nodeIndex]);
                        isPartitionInUpgrade = true;
                        isReplicaInUpgrade = true;
                        isSwapPrimary = true;

                        if (isReplicaToBeDropped && failoverUnit.ActualReplicaDifference > 0 && fuDesc.Replicas.size() == 1)
                        {
                            // Special case for single replica to be upgraded; new replica should be placed in different UDs
                            isReplicaToBeDropped = false;
                        }
                    }

                    ReplicaRole::Enum currentRole = replica.CurrentRole;

                    if (replica.IsPrimaryToBePlaced && !replica.ShouldDisappear)
                    {
                        // Ignore the J flag if it is set on MoveInProgress or ToBeDropped replica
                        primaryUpgradeLocation = &(balanceChecker_->Nodes[nodeIndex]);
                        isPartitionInUpgrade = true;
                        isReplicaInUpgrade = true;
                    }
                    else if (replica.IsReplicaToBePlaced)
                    {
                        secondaryUpgradeLocations.push_back(&(balanceChecker_->Nodes[nodeIndex]));
                        isPartitionInUpgrade = true;
                        isReplicaInUpgrade = true;
                    }

                    if (currentRole == ReplicaRole::Dropped)
                    {
                        continue;
                    }
                    else if (currentRole == ReplicaRole::StandBy)
                    {
                        standByLocations.push_back(&(balanceChecker_->Nodes[nodeIndex]));
                        standByReplicas_.push_back(make_unique<PlacementReplica>(
                            replicaIndex++,
                            currentRole,
                            false,
                            false,
                            true,
                            &(balanceChecker_->Nodes[nodeIndex]),
                            replica.IsMoveInProgress,
                            isReplicaToBeDropped,
                            isReplicaInUpgrade,
                            false,
                            canBeThrottled));
                        partitionStandBy.push_back(standByReplicas_.back().get());
                    }
                    else
                    {
                        // Allow constraint check fixes for replicas on a Restart/RemoveData/RemoveNode nodes
                        // while safety checks are in progress.
                        bool isMovableDuringDeactivation = isCurrentActionConstraintCheck && nodeResult == IsRestartingOrRemoving;

                        bool isInTransition =
                            replica.IsInTransition ||
                            // instance is not match or node is paused or node is deactivated
                            nodeResult != Match && !isMovableDuringDeactivation ||
                            serviceType.ServiceTypeDesc.IsInBlockList(replica.NodeId);

                        // If partition is in upgrade, all replicas are not movable
                        // if PLB config prevent constraint check during application upgrade.
                        // If constraint check is allowed during application upgrade
                        // all replicas are movable except IsPrimaryToBeSwappedOut replica (marked with I) or
                        // primary replica together with IsPrimaryToBePlaced replica (marked with J).
                        bool isMovable = currentRole != ReplicaRole::None && !isInTransition && !fuDesc.IsInQuorumLost;

                        // Allow constraint check fixes for all other replicas except replicas that are currently in app upgrade
                        bool movableDueToAppUpgrade =
                            !fuDesc.IsInUpgrade ||
                            allowConstraintCheckFixesDuringAppUpgrade && !isReplicaInUpgrade;

                        isMovable = isMovable && movableDueToAppUpgrade;

                        // Don't drop extra replicas only from paused node.
                        bool isDroppable = nodeResult != IsPaused;

                        // Check if singleton replica is movable during upgrade
                        bool isSingletonReplicaMovableDuringUpgrade = service.ServiceDesc.TargetReplicaSetSize == 1 ?
                            PlacementReplica::CheckIfSingletonReplicaIsMovableDuringUpgrade(replica) :
                            false;

                        allReplicas_.push_back(make_unique<PlacementReplica>(
                            replicaIndex++,
                            currentRole,
                            isMovable,
                            isDroppable,
                            isInTransition,
                            &(balanceChecker_->Nodes[nodeIndex]),
                            replica.IsMoveInProgress,
                            isReplicaToBeDropped,
                            isReplicaInUpgrade,
                            isSingletonReplicaMovableDuringUpgrade,
                            canBeThrottled));

                        if (currentRole == ReplicaRole::Primary)
                        {
                            primaryReplica = allReplicas_.back().get();
                        }

                        replicas.push_back(allReplicas_.back().get());
                    }
                }
            }
        }

        if (primaryReplica != nullptr && primaryUpgradeLocation != nullptr)
        {
            primaryReplica->IsMovable = false;
        }

        // get service entry
        auto itService = serviceEntryDict_.find(fuDesc.ServiceId);
        if (itService == serviceEntryDict_.end())
        {
            TESTASSERT_IF(itService == serviceEntryDict_.end(), "No such service entry {0} for a partition entry {1}",
                fuDesc.ServiceName,
                fuDesc.FUId);
            serviceDomain_.plb_.Trace.PlacementCreatorServiceNotFound(fuDesc.FUId, fuDesc.ServiceName);
            // Ignore this partition while searching.
            continue;
        }
        ServiceEntry* serviceEntry = itService->second;

        if (serviceEntry->AutoSwitchToQuorumBasedLogic || 
            plb_.Settings.QuorumBasedReplicaDistributionPerFaultDomains ||
            plb_.Settings.QuorumBasedReplicaDistributionPerUpgradeDomains)
        {
            quorumBasedPartitionsCount_++;
        }

        // Is partition in singleton replica upgrade
        bool isSingletonReplicaUpgrade = isPartitionInUpgrade &&
            serviceEntry->TargetReplicaSetSize == 1 &&
            failoverUnit.ActualReplicaDifference == 1 &&
            plb_.isSingletonReplicaMoveAllowedDuringUpgradeEntry_.GetValue();
        bool isSingletonReplicaInUpgradeTransition = serviceEntry->TargetReplicaSetSize == 1 && replicas.size() > 1;


        PartitionEntry::SingletonReplicaUpgradeOptimizationType upgradeOptimization = PartitionEntry::SingletonReplicaUpgradeOptimizationType::None;

        // Set appropriate singleton upgrade optimization
        if ((isSingletonReplicaUpgrade ||
            isSingletonReplicaInUpgradeTransition) &&
            balanceChecker_->Settings.CheckAffinityForUpgradePlacement &&
            ((serviceEntry->DependedService != nullptr &&             // child: has parent service with TRC=1
                serviceEntry->DependedService->TargetReplicaSetSize == 1) ||
                serviceEntry->DependentServices.size() > 0))          // parent: has child services
        {
            // If it is initial upgrade FT update, set the appropriate optimization
            if (isSingletonReplicaUpgrade)
            {
                upgradeOptimization = PartitionEntry::SingletonReplicaUpgradeOptimizationType::CheckAffinityDuringUpgrade;
            }
            serviceEntry->HasAffinityAssociatedSingletonReplicaInUpgrade = true;
        }
        else if ((isSingletonReplicaUpgrade ||
            isSingletonReplicaInUpgradeTransition) &&
            balanceChecker_->Settings.RelaxScaleoutConstraintDuringUpgrade &&
            serviceEntry->Application != nullptr &&
            serviceEntry->Application->ScaleoutCount == 1 &&
            serviceEntry->Application->IsInSingletonReplicaUpgrade(plb_, replicas))
        {
                // If it is initial upgrade FT update, set the appropriate optimization
                if (isSingletonReplicaUpgrade)
                {
                    upgradeOptimization = PartitionEntry::SingletonReplicaUpgradeOptimizationType::RelaxScaleoutDuringUpgrade;
                }
                ApplicationEntry* application = const_cast<ApplicationEntry*>(serviceEntry->Application);
                application->HasPartitionsInSingletonReplicaUpgrade = true;
        }

        int replicaDiff = failoverUnit.ActualReplicaDifference;
        
        bool isScaledToEveryNode = false;
        int minInstanceCount = 0;

        if (service.ServiceDesc.IsAutoScalingDefined 
            && service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled()
            && service.ServiceDesc.AutoScalingPolicy.Mechanism->Kind == ScalingMechanismKind::PartitionInstanceCount)
        {
            InstanceCountScalingMechanismSPtr mechanism = static_pointer_cast<InstanceCountScalingMechanism>(service.ServiceDesc.AutoScalingPolicy.Mechanism);
            if (mechanism->MaximumInstanceCount == -1)
            {
                isScaledToEveryNode = true;
            }
            minInstanceCount = mechanism->MinimumInstanceCount;
        }

        if (INT_MAX == replicaDiff || isScaledToEveryNode && replicaDiff == 0)
        {
            int numReplicasOnBlocklistedNodes = 0;
            NodeSet nodes(static_cast<int>(plb_.nodes_.size()));

            for (auto dIt = balanceChecker_->DeactivatedNodes.begin(); dIt != balanceChecker_->DeactivatedNodes.end(); ++dIt)
            {
                nodes.Add(*dIt);
            }

            for (auto downIt = balanceChecker_->DownNodes.begin(); downIt != balanceChecker_->DownNodes.end(); ++downIt)
            {
                nodes.Add(*downIt);
            }

            // replicaDiff should be recalculated for FTs with target -1, and for FT's that have maxInstanceCount -1 and target is above nonBlocklisted node count
            // If target is below nonBlocklisted node count, replicas on blocklisted nodes should be moved in Constraint check phase.
            if (INT_MAX == replicaDiff || failoverUnit.FuDescription.TargetReplicaSetSize > static_cast<int>(plb_.nodes_.size()) - nodes.GetTotalOneCount())
            {
                serviceEntry->BlockList.ForEach([&](size_t nodeIndex) -> void
                {
                    nodes.Add(static_cast<int>(nodeIndex));
                });

                for (auto rIt = replicas.begin(); rIt != replicas.end(); ++rIt)
                {
                    if (serviceEntry->IsNodeInBlockList((*rIt)->Node))
                    {
                        numReplicasOnBlocklistedNodes++;
                    }

                    nodes.Add((*rIt)->Node);
                }

                replicaDiff = static_cast<int>(plb_.nodes_.size()) - nodes.GetTotalOneCount();

                // Do not place more replicas if we are already over scaleout count
                if (serviceEntry->Application != nullptr
                    && serviceEntry->Application->ScaleoutCount > 0
                    && serviceEntry->Application->ScaleoutCount <= replicas.size())
                {
                    replicaDiff = 0;
                }

                if (replicaDiff < 0)
                {
                    TESTASSERT_IF(replicaDiff < 0, "The calculated replicaDiff {0} shouldn't be less than 0", replicaDiff);
                    serviceDomain_.plb_.Trace.PlacementReplicaDiffError(fuDesc.FUId, replicaDiff);
                    replicaDiff = 0;
                }

                if (0 == replicaDiff)
                {
                    // Drop replicas that are on blocklisted nodes
                    replicaDiff = -numReplicasOnBlocklistedNodes;

                    if (isScaledToEveryNode)
                    {
                        replicaDiff = max(replicaDiff, minInstanceCount - (int)replicas.size());
                    }
                }
            }
        }

        replicaDiff = min(replicaDiff, static_cast<int>(plb_.upNodeCount_));
        int extraReplicaCount = 0;

        if (replicaDiff > 0)
        {
            if (service.ServiceDesc.IsStateful)
            {
                bool havingPrimary = any_of(failoverUnit.FuDescription.Replicas.begin(), failoverUnit.FuDescription.Replicas.end(), [](ReplicaDescription const& r)
                {
                    if (r.CurrentRole == ReplicaRole::Primary && !r.IsToBeDropped)
                    {
                        return true;
                    }
                    else if (r.CurrentRole == ReplicaRole::Primary && r.IsToBeDropped && r.IsPrimaryToBeSwappedOut)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                });

                if (!havingPrimary)
                {
                    // Primary build cannot be throttled if there are no replicas in the configuration.
                    bool canPrimaryBeThrottled = canBeThrottled &&
                        any_of(failoverUnit.FuDescription.Replicas.begin(),
                            failoverUnit.FuDescription.Replicas.end(),
                            [](ReplicaDescription const& r) {
                                if (r.CurrentRole == ReplicaRole::Primary || r.CurrentRole == ReplicaRole::Secondary)
                                {
                                    return true;
                                }
                                return false;
                            });
                    allReplicas_.push_back(make_unique<PlacementReplica>(replicaIndex++, ReplicaRole::Primary, canPrimaryBeThrottled));
                    replicas.push_back(allReplicas_.back().get());
                    replicaDiff--;
                }
            }

            while (replicaDiff > 0)
            {
                allReplicas_.push_back(make_unique<PlacementReplica>(replicaIndex++, ReplicaRole::Secondary, service.ServiceDesc.IsStateful));
                replicas.push_back(allReplicas_.back().get());
                replicaDiff--;
            }
        }
        else
        {
            extraReplicaCount = -replicaDiff;
        }

        if (replicas.empty() && partitionStandBy.empty())
        {
            // don't include empty partitions
            continue;
        }

        // set the affinity order by ServiceEntry
        int order = 0;
        ServiceEntry const* dependedServiceEntry = serviceEntry->DependedService;
        while (dependedServiceEntry != nullptr)
        {
            // This is a child service, needs to be sorted after parent
            order += 2;
            // Set parent flag for upgrade as well, hence proper initialization is done,
            // in scenarios when only child partition is in upgrade
            if ((isSingletonReplicaUpgrade ||
                isSingletonReplicaInUpgradeTransition) &&
                balanceChecker_->Settings.CheckAffinityForUpgradePlacement)
            {
                const_cast<ServiceEntry*>(dependedServiceEntry)->HasAffinityAssociatedSingletonReplicaInUpgrade = true;
            }
            dependedServiceEntry = dependedServiceEntry->DependedService;
        }

        if (upReplicaCount >= failoverUnit.FuDescription.TargetReplicaSetSize)
        {
            // If up replica count is not at target, "prioritize" this replica.
            // Final order number will be:
            //  0 - parent not on target, 1 - parent on target, 2 - child not on target, 3 - child on target
            // "Prioritization" is done in searcher, by placing lower order first.
            // Optimization for later: do better ordering (i.e. parents with children on target to the back).
            ++order;
        }

        vector<int64> primaryEntry(failoverUnit.PrimaryEntries.begin(), failoverUnit.PrimaryEntries.end());
        vector<int64> secondaryEntry(failoverUnit.SecondaryEntries.begin(), failoverUnit.SecondaryEntries.end());

        if (isPartitionInUpgrade)
        {
            partitionsInUpgradeCount_++;
        }

        bool aggregatedIsPartitionInUpgrade = isPartitionInUpgrade || fuDesc.IsInUpgrade;

        // If partition is in upgrade, all replicas are not movable.
        // Except if current PLB phase is ConstraintCheck and fixes are allowed during application upgrade.
        bool isPartitionMovable = throttledPartitions_.end() == throttledPartitions_.find(fuDesc.FUId);

        if (!(aggregatedIsPartitionInUpgrade && allowConstraintCheckFixesDuringAppUpgrade))
        {
            isPartitionMovable = isPartitionMovable && !fuDesc.IsInTransition && !aggregatedIsPartitionInUpgrade;
        }

        partitionEntries_.push_back(PartitionEntry(
            fuDesc.FUId,
            fuDesc.Version,
            fuDesc.IsInTransition,
            isPartitionMovable,
            serviceEntry,
            move(replicas),
            LoadEntry(move(primaryEntry)),
            LoadEntry(move(secondaryEntry)),
            failoverUnit.GetMoveCostValue(ReplicaRole::Primary, balanceChecker_->Settings),
            failoverUnit.GetMoveCostValue(ReplicaRole::Secondary, balanceChecker_->Settings),
            move(standByLocations),
            primarySwapOutLocation,
            primaryUpgradeLocation,
            move(secondaryUpgradeLocations),
            isPartitionInUpgrade,
            fuDesc.IsInUpgrade,
            isSingletonReplicaUpgrade,
            upgradeOptimization,
            order,
            extraReplicaCount,
            failoverUnit.SecondaryEntriesMap,
            move(partitionStandBy),
            balanceChecker_->Settings,
            failoverUnit.TargetReplicaSetSize));
    }
}

PlacementCreator::NodeMatchType PlacementCreator::NodeExistAndMatch(Federation::NodeInstance node) const
{
    uint64 nodeIndex = plb_.GetNodeIndex(node.Id);

    if (nodeIndex == UINT64_MAX)
    {
        return NotExist;
    }

    auto const& plbNode = plb_.nodes_[nodeIndex];

    if (!plbNode.NodeDescriptionObj.IsUp)
    {
        return IsDown;
    }
    else if (plbNode.NodeDescriptionObj.NodeInstance != node)
    {
        return InstanceNotMatch;
    }
    else if (plbNode.NodeDescriptionObj.IsPaused)
    {
        // Node is UP but paused
        return IsPaused;
    }
    else if ((plbNode.NodeDescriptionObj.IsRestart || plbNode.NodeDescriptionObj.IsRemove) &&
        plbNode.NodeDescriptionObj.AreSafetyChecksInProgress)
    {
        // Node is UP, deactivation intent is Restart/RemoveData/RemoveNode and deactivation status is DeactivationSafetyCheckInProgress
        return IsRestartingOrRemoving;
    }
    else if (plbNode.NodeDescriptionObj.IsDeactivated)
    {
        // Node is UP but deactivated (Restart or RemoveData or RemoveNode or Pause)
        return IsDeactivated;
    }
    else
    {
        return Match;
    }
}

BalanceChecker::DomainTree PlacementCreator::GetFilteredDomainTree(BalanceChecker::DomainTree const& baseTree, DynamicBitSet const& blockList, bool isFaultDomain)
{
    ASSERT_IF(baseTree.IsEmpty, "Base tree is empty");

    BalanceChecker::DomainTree tree(baseTree);

    blockList.ForEach([&](size_t nodeIndex) -> void
    {
        auto const& plbNode = plb_.nodes_[nodeIndex];

        //we already removed all deactivated nodes except those where safety check is in progress
        //down nodes are already removed from blocklist
        if (plbNode.NodeDescriptionObj.IsDeactivated && !plbNode.NodeDescriptionObj.IsRestartOrPauseCheckInProgress)
        {
            return;
        }

        NodeEntry const* node = &(balanceChecker_->Nodes[nodeIndex]);

        TreeNodeIndex const& treeNodeIndex = isFaultDomain ? node->FaultDomainIndex : node->UpgradeDomainIndex;

        tree.ForEachNodeInPath(treeNodeIndex, [](BalanceChecker::DomainTree::Node & n)
        {
            ASSERT_IFNOT(n.Data.NodeCount > 0, "Node count should be greater than 0");
            --n.DataRef.NodeCount;
        });
    });

    return tree;
}

BalanceChecker::DomainTree PlacementCreator::FilterAndCalculateDomainStructure(size_t & domainCount, size_t & nodeCount, bool isFaultDomain, Service const* service)
{
    domainCount = 0;
    nodeCount = 0;
    BalanceChecker::DomainTree domainStructure = BalanceChecker::DomainTree();
    BalanceChecker::DomainTree const& domainTree =
        isFaultDomain ? balanceChecker_->FaultDomainStructure : balanceChecker_->UpgradeDomainStructure;
    BalanceChecker::DomainTree::Node const* currentNode = nullptr;

    if (!domainTree.IsEmpty)
    {
        currentNode = &(domainTree.Root);
    }

    // Create specific domain structure if necessary
    if (!(domainTree.IsEmpty || service->BlockList.Count == 0 || service->ServiceDesc.OnEveryNode
        || (isFaultDomain && service->FDDistribution == Service::Type::Ignore)))
    {
        domainStructure = GetFilteredDomainTree(domainTree, service->BlockList, isFaultDomain);
        currentNode = &(domainStructure.Root);
    }

    if (nullptr != currentNode)
    {
        nodeCount = currentNode->Data.NodeCount;
    }

    // If auto switching  between '+1' and quorum based logic is enabled,
    // calculate necessary information (number of domains and number of nodes).
    // Otherwise, skip that.
    if (!plb_.Settings.QuorumBasedLogicAutoSwitch)
    {
        return domainStructure;
    }

    // Search for the first level in the domain structure with more than one domain (with non zero number of nodes)
    do
    {
        if (nullptr == currentNode)
        {
            break;
        }

        size_t lastChildWithNodes = 0;
        domainCount = 0;
        for (size_t childId = 0; childId < currentNode->Children.size(); childId++)
        {
            if (currentNode->Children[childId].Data.NodeCount != 0)
            {
                domainCount++;
                lastChildWithNodes = childId;
            }
        }

        if (domainCount != 0)
        {
            currentNode = &(currentNode->Children[lastChildWithNodes]);
        }
    } while (domainCount == 1);

    return domainStructure;
}

PlacementUPtr PlacementCreator::Create(int randomSeed)
{
    CreateApplicationEntries();
    CreateServicePackageEntries();
    CreateServiceEntries();
    CreatePartitionEntries();
    CreateApplicationReservedLoads();
    PrepareNodes();

    return make_unique<Placement>(
        move(balanceChecker_),
        move(serviceEntries_),
        move(partitionEntries_),
        move(applicationEntries_),
        move(servicePackageEntries_),
        move(allReplicas_),
        move(standByReplicas_),
        move(reservationLoads_),
        move(inBuildCountPerNode_),
        partitionsInUpgradeCount_,
        plb_.isSingletonReplicaMoveAllowedDuringUpgradeEntry_.GetValue(),
        randomSeed,
        serviceDomain_.Scheduler.CurrentAction,
        partitionClosure_->Type,
        partitionClosure_->PartialClosureFailoverUnits,
        move(servicePackagePlacement_),
        quorumBasedServicesCount_,
        quorumBasedPartitionsCount_,
        move(quorumBasedServicesTempCache_));
}
