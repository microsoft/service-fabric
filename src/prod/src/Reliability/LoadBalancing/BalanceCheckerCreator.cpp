// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "BalanceCheckerCreator.h"

#include "PLBConfig.h"
#include "PartitionClosure.h"
#include "NodeEntry.h"
#include "Node.h"
#include "Service.h"
#include "Metric.h"
#include "ServiceMetric.h"
#include "LoadBalancingDomainEntry.h"
#include "ServiceDomainMetric.h"

#include "BalanceChecker.h"
#include "DiagnosticsDataStructures.h"
#include "Application.h"
#include "SearcherSettings.h"
#include "NodeSet.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

BalanceCheckerCreator::BalanceCheckerCreator(
    PartitionClosure const& partitionClosure,
    vector<Node> const& nodes,
    std::map<std::wstring, ServiceDomainMetric> const& serviceDomainMetrics,
    BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr,
    std::set<std::wstring> const& upgradeCompletedUDs,
    SearcherSettings const & settings)
    : nodes_(nodes),
    partitionClosure_(partitionClosure),
    serviceDomainMetrics_(serviceDomainMetrics),
    faultDomainStructure_(),
    upgradeDomainStructure_(),
    existDefragMetric_(false),
    existScopedDefragMetric_(false),
    balancingDiagnosticsDataSPtr_(balancingDiagnosticsDataSPtr),
    upgradeCompletedUDs_(upgradeCompletedUDs),
    faultDomainMappings_(),
    upgradeDomainMappings_(),
    settings_(settings),
    faultDomainNodeSet_(),
    upgradeDomainNodeSet_()
{
}

void BalanceCheckerCreator::CreateNodes()
{
    // nodes and capacity and capacity ratio entries
    // the size of capacity and capacity ratios is equal to total metric count (duplication is possible now)
    capacityRatios_.reserve(nodes_.size());
    bufferedCapacities_.reserve(nodes_.size());
    totalCapacities_.reserve(nodes_.size());

    bool createFaultDomainStructure = settings_.FaultDomainEnabled;
    bool createUpgradeDomainStructure = settings_.UpgradeDomainEnabled;
    size_t fdMaxLen = 0;
    if (createFaultDomainStructure || createUpgradeDomainStructure)
    {
        set<wstring> fdHeadSet;
        set<wstring> udSet;
        size_t nodeCount = 0;

        for (auto const& node : nodes_)
        {
            NodeDescription const& nd = node.NodeDescriptionObj;
            if (!nd.IsUp)
            {
                continue;
            }

            ++nodeCount;
            if (createFaultDomainStructure)
            {
                fdHeadSet.insert(nd.FaultDomainId[0]);

                size_t len = nd.FaultDomainId.size();
                if (fdMaxLen < len)
                {
                    fdMaxLen = len;
                }
            }

            if (createUpgradeDomainStructure)
            {
                udSet.insert(nd.UpgradeDomainId);
            }
        }

        if (createFaultDomainStructure && fdHeadSet.size() < nodeCount)
        {
            faultDomainStructure_.SetRoot(make_unique<DomainTree::Node>(DomainData()));
        }

        if (createUpgradeDomainStructure)
        {
            upgradeDomainStructure_.SetRoot(make_unique<DomainTree::Node>(DomainData()));
        }
    }

    // TODO:
    // 1. remove the last x levels of domain if every domain only has 1 node
    // 2. remove the first x levels of domain if they contain all nodes

    for (auto node : nodes_)
    {
        NodeDescription const& nd = node.NodeDescriptionObj;

        capacityRatios_.push_back(map<wstring, uint>(nd.CapacityRatios));
        totalCapacities_.push_back(map<wstring, uint>(nd.Capacities));
        bufferedCapacities_.push_back(map<wstring, uint>(nd.Capacities));

        for (auto it = bufferedCapacities_.back().begin(); it != bufferedCapacities_.back().end(); ++it)
        {
            if (metricsWithNodeCapacity_.empty() || metricsWithNodeCapacity_.find(it->first) == metricsWithNodeCapacity_.end())
            {
                metricsWithNodeCapacity_.insert(it->first);
            }

            auto itNodeBufferPercentage = settings_.NodeBufferPercentage.find(it->first);
            double nodeBufferPercentage = itNodeBufferPercentage != settings_.NodeBufferPercentage.end() ?
                itNodeBufferPercentage->second : 0;

            it->second = (uint)(it->second * (1 - nodeBufferPercentage));
        }

        if (!faultDomainStructure_.IsEmpty)
        {
            NodeDescription::DomainId fdId = nd.FaultDomainId;
            while (fdId.size() < fdMaxLen)
            {
                fdId.push_back(L"");
            }
            faultDomainIndices_.push_back(AddDomainId(fdId, faultDomainStructure_));
            faultDomainMappings_[faultDomainIndices_.back()] = node.NodeDescriptionObj.FaultDomainId;
        }

        if (!upgradeDomainStructure_.IsEmpty)
        {
            NodeDescription::DomainId udId;
            udId.push_back(nd.UpgradeDomainId);
            upgradeDomainIndices_.push_back(AddDomainId(udId, upgradeDomainStructure_));
            upgradeDomainMappings_[upgradeDomainIndices_.back()] = node.NodeDescriptionObj.UpgradeDomainId;
        }
    }
}

TreeNodeIndex BalanceCheckerCreator::AddDomainId(NodeDescription::DomainId const& domainId, DomainTree & domainStructure)
{
    TreeNodeIndex index;
    ASSERT_IF(domainId.empty(), "Invalid domainId {0}", domainId);
    DomainTree::Node * currentNode = &(domainStructure.RootRef);
    ++(currentNode->DataRef.NodeCount);

    for (size_t i = 0; i < domainId.size(); i++)
    {
        wstring const& segment = domainId[i];

        DomainTree::Node * subNode = nullptr;

        for (size_t j = 0; j < currentNode->Children.size(); j++)
        {
            DomainTree::Node & tempNode = currentNode->ChildrenRef[j];
            if (tempNode.Data.Segment == segment)
            {
                subNode = &tempNode;
                index.Append(j);
                break;
            }
        }

        if (subNode == nullptr)
        {
            currentNode->ChildrenRef.push_back(DomainTree::Node(DomainData(wstring(segment), 0)));
            subNode = &(currentNode->ChildrenRef.back());
            index.Append(currentNode->ChildrenRef.size() - 1);
        }

        currentNode = subNode;
        ++(currentNode->DataRef.NodeCount);
    }

    return index;
}

TreeNodeIndex BalanceCheckerCreator::GetDomainIndex(NodeDescription::DomainId const& domainId, Common::Tree<DomainData> const& domainStructure)
{
    TreeNodeIndex index;

    DomainTree::Node const* currentNode = &(domainStructure.Root);
    for (size_t i = 0; i < domainId.size(); i++)
    {
        wstring const& segment = domainId[i];
        DomainTree::Node const* subNode = nullptr;

        for (size_t j = 0; j < currentNode->Children.size(); j++)
        {
            DomainTree::Node const& tempNode = currentNode->Children[j];
            if (tempNode.Data.Segment == segment)
            {
                subNode = &tempNode;
                index.Append(j);
                break;
            }
        }

        if (subNode == nullptr)
        {
            // Did not find the segment, return empty index
            return TreeNodeIndex();
        }
        currentNode = subNode;
    }

    return index;
}

void BalanceCheckerCreator::CreateLoadBalancingDomainEntries()
{
    size_t totalMetricCount = 0;
    vector<Service const*> const& services = partitionClosure_.Services;

    // Maps aren't initialised, but first access to field will set it to 0
    std::map<std::wstring, uint> metricsBufferedCapacities;
    std::map<std::wstring, uint> metricsTotalCapacities;
    std::map<std::wstring, uint> metricsMaxNodeCapacities;
    std::map<std::wstring, uint> metricsNumberOfNodesWithCapacity;

    uint numberOfNodesWithAvailableCapacity = 0;

    TESTASSERT_IFNOT(nodes_.size() == bufferedCapacities_.size() && nodes_.size() == totalCapacities_.size(),
        "Nodes vector size doesn't match capacity vectors");
    for (size_t i = 0; i < nodes_.size(); i++)
    {
        // We should not count Down or DeactivationComplete nodes with intent RemoveData or RemoveNode
        // towards total cluster capacity because no replicas can be placed on them
        if (nodes_[i].NodeDescriptionObj.IsCapacityAvailable)
        {
            numberOfNodesWithAvailableCapacity++;

            for (auto j = bufferedCapacities_[i].begin(); j != bufferedCapacities_[i].end(); ++j)
            {
                metricsBufferedCapacities[j->first] += j->second;
            }

            for (auto j = totalCapacities_[i].begin(); j != totalCapacities_[i].end(); ++j)
            {
                metricsTotalCapacities[j->first] += j->second;
                metricsNumberOfNodesWithCapacity[j->first]++;

                if (metricsMaxNodeCapacities[j->first] < j->second)
                {
                    metricsMaxNodeCapacities[j->first] = j->second;
                }
            }
        }
    }

    // create local load balancing domains for services which are not singleton and positive metric weights
    for (size_t i = 0; i < services.size(); i++)
    {
        Service const& service = *services[i];
        if (service.IsSingleton)
        {
            continue;
        }

        vector<ServiceMetric> const& metrics = service.ServiceDesc.Metrics;

        vector<Metric> lbDomainMetrics;
        double metricWeightSum = 0.0;
        for (auto itServiceMetric = metrics.begin(); itServiceMetric != metrics.end(); ++itServiceMetric)
        {
            metricWeightSum += itServiceMetric->Weight;

            // activity threshold is read from config
            auto itActivityThreshold = settings_.MetricActivityThresholds.find(itServiceMetric->Name);
            uint activityThreshold = 0;
            if(itActivityThreshold != settings_.MetricActivityThresholds.end())
            {
                // If metric is CpuCores RG metric for which we allow double value and we always multiply its load with CpuCorrectionFactor
                // ActivityThreshold needs to be multliplied with CpuCorrectionFactor as well
                activityThreshold = (itServiceMetric->Name == ServiceModel::Constants::SystemMetricNameCpuCores) ?
                    itActivityThreshold->second * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor :
                    itActivityThreshold->second;
            }

            bool isMetricDefrag = IsDefragmentationMetric(itServiceMetric->Name);
            bool defragmentationScopedAlgorithmEnabled = GetDefragmentationScopedAlgorithmEnabled(itServiceMetric->Name);

            int32 numberOfEmptyNodes = -1;
            Metric::DefragDistributionType emptyNodeDistribution = Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
            size_t defragEmptyNodeLoadThreshold = 0;
            int64 reservationLoad = 0;
            if (isMetricDefrag)
            {
                numberOfEmptyNodes = GetDefragEmptyNodesCount(itServiceMetric->Name, defragmentationScopedAlgorithmEnabled);
                defragEmptyNodeLoadThreshold = GetDefragEmptyNodeLoadTreshold(itServiceMetric->Name);
                reservationLoad = GetReservationLoadAmount(itServiceMetric->Name, metricsMaxNodeCapacities[itServiceMetric->Name]);
                emptyNodeDistribution = GetDefragDistribution(itServiceMetric->Name);
            }

            double placementHeuristicIncomingLoadFactor = GetPlacementHeuristicIncomingLoadFactor(itServiceMetric->Name);
            double placementHeuristicEmptySpacePercent = GetPlacementHeuristicEmptySpacePercent(itServiceMetric->Name);
            Metric::PlacementStrategy placementStrategy = GetPlacementStrategy(itServiceMetric->Name);
            double defragmentationEmptyNodeWeight = GetDefragmentationEmptyNodeWeight(itServiceMetric->Name, placementStrategy);
            double defragmentationNonEmptyNodeWeight = GetDefragmentationNonEmptyNodeWeight(itServiceMetric->Name, placementStrategy);
            bool balancingByPercentage = false;
            if (metricsNumberOfNodesWithCapacity[itServiceMetric->Name] == numberOfNodesWithAvailableCapacity)
            {
                balancingByPercentage = IsBalancingByPercentageEnabled(itServiceMetric->Name);
            }

            // cluster buffered capacity
            int64 clusterBufferedCapacity;
            auto itBufferedCapacity = metricsBufferedCapacities.find(itServiceMetric->Name);
            if (itBufferedCapacity != metricsBufferedCapacities.end())
            {
                clusterBufferedCapacity = static_cast<int64>(itBufferedCapacity->second);
            }
            else
            {
                clusterBufferedCapacity = 0;
            }

            // cluster total capacity
            int64 clusterTotalCapacity;
            auto itTotalCapacity = metricsTotalCapacities.find(itServiceMetric->Name);
            if (itTotalCapacity != metricsTotalCapacities.end())
            {
                clusterTotalCapacity = static_cast<int64>(itTotalCapacity->second);
            }
            else
            {
                clusterTotalCapacity = 0;
            }

            // calculate the cluster load and should PLB do defragmentation or balancing for this metric
            int64 clusterLoad(0);
            auto serviceDomainMetric = serviceDomainMetrics_.find(itServiceMetric->Name);
            if (serviceDomainMetric != serviceDomainMetrics_.end())
            {
                clusterLoad = serviceDomainMetric->second.NodeLoadSum;
            }

            lbDomainMetrics.push_back(Metric(
                wstring(itServiceMetric->Name),
                itServiceMetric->Weight,
                settings_.LocalBalancingThreshold,
                DynamicBitSet(service.BlockList),
                activityThreshold,
                clusterTotalCapacity,
                clusterBufferedCapacity,
                clusterLoad,
                isMetricDefrag,
                numberOfEmptyNodes,
                defragEmptyNodeLoadThreshold,
                reservationLoad,
                emptyNodeDistribution,
                placementHeuristicIncomingLoadFactor,
                placementHeuristicEmptySpacePercent,
                defragmentationScopedAlgorithmEnabled,
                placementStrategy,
                defragmentationEmptyNodeWeight,
                defragmentationNonEmptyNodeWeight,
                balancingByPercentage));
        }

        ASSERT_IF(lbDomainMetrics.empty(), "LB domain doesn't have metrics");

        // only create local balancing domains for those with positive metric weight sum
        if (metricWeightSum > 0.0)
        {
            lbDomainEntries_.push_back(LoadBalancingDomainEntry(move(lbDomainMetrics), metricWeightSum, totalMetricCount, static_cast<int>(i)));
            totalMetricCount += lbDomainEntries_.back().MetricCount;
        }
    }

    // always create the global load balancing domain
    vector<Metric> globalMetrics;
    double globalMetricWeightSum = 0.0;
    for (auto it = serviceDomainMetrics_.begin(); it != serviceDomainMetrics_.end(); ++it)
    {
        wstring metricName(it->first);

        double balancingThreshold = 1.0;
        auto itBalancingThreshold = settings_.MetricBalancingThresholds.find(metricName);
        if (itBalancingThreshold != settings_.MetricBalancingThresholds.end())
        {
            balancingThreshold = itBalancingThreshold->second;
        }

        double weight;
        auto itWeight = settings_.GlobalMetricWeights.find(metricName);
        if (itWeight != settings_.GlobalMetricWeights.end())
        {
            weight = itWeight->second;
        }
        else
        {
            weight = it->second.AverageWeight;
        }

        globalMetricWeightSum += weight;

        auto itActivityThreshold = settings_.MetricActivityThresholds.find(metricName);
        uint activityThreshold = 0;
        if (itActivityThreshold != settings_.MetricActivityThresholds.end())
        {
            // If metric is CpuCores RG metric for which we allow double value and we always multiply its load with CpuCorrectionFactor
            // ActivityThreshold needs to be multliplied with CpuCorrectionFactor as well
            activityThreshold = (metricName == ServiceModel::Constants::SystemMetricNameCpuCores) ?
                itActivityThreshold->second * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor :
                itActivityThreshold->second;
        }

        bool isMetricDefrag = IsDefragmentationMetric(it->second.Name);
        existDefragMetric_ |= isMetricDefrag;

        if (isMetricDefrag)
        {
            existScopedDefragMetric_ |= GetDefragmentationScopedAlgorithmEnabled(it->second.Name);
        }
        bool defragmentationScopedAlgorithmEnabled = GetDefragmentationScopedAlgorithmEnabled(metricName);
        
        int32 numberOfEmptyNodes = -1;
        Metric::DefragDistributionType emptyNodeDistribution = Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
        size_t defragEmptyNodeLoadThreshold = 0;
        int64 reservationLoad = 0;
        if (isMetricDefrag)
        {
            numberOfEmptyNodes = GetDefragEmptyNodesCount(metricName, defragmentationScopedAlgorithmEnabled);
            defragEmptyNodeLoadThreshold = GetDefragEmptyNodeLoadTreshold(metricName);
            reservationLoad = GetReservationLoadAmount(metricName, metricsMaxNodeCapacities[metricName]);
            emptyNodeDistribution = GetDefragDistribution(metricName);
        }

        double placementHeuristicIncomingLoadFactor = GetPlacementHeuristicIncomingLoadFactor(metricName);
        double placementHeuristicEmptySpacePercent = GetPlacementHeuristicEmptySpacePercent(metricName);
        Metric::PlacementStrategy placementStrategy = GetPlacementStrategy(metricName);
        double defragmentationEmptyNodeWeight = GetDefragmentationEmptyNodeWeight(metricName, placementStrategy);
        double defragmentationNonEmptyNodeWeight = GetDefragmentationNonEmptyNodeWeight(metricName, placementStrategy);

        // If some nodes don't have capacity for a metric, it can't be balanced by percentage
        bool balancingByPercentage = false;
        if (metricsNumberOfNodesWithCapacity[metricName] == numberOfNodesWithAvailableCapacity)
        {
            balancingByPercentage = IsBalancingByPercentageEnabled(metricName);
        }

        int64 clusterBufferedCapacity = metricsBufferedCapacities[metricName];
        int64 clusterTotalCapacity = metricsTotalCapacities[metricName];

        globalMetrics.push_back(Metric(
            move(metricName),
            weight,
            balancingThreshold,
            it->second.GetBlockNodeList(),
            activityThreshold,
            clusterTotalCapacity,
            clusterBufferedCapacity,
            it->second.NodeLoadSum,
            isMetricDefrag,
            numberOfEmptyNodes,
            defragEmptyNodeLoadThreshold,
            reservationLoad,
            emptyNodeDistribution,
            placementHeuristicIncomingLoadFactor,
            placementHeuristicEmptySpacePercent,
            defragmentationScopedAlgorithmEnabled,
            placementStrategy,
            defragmentationEmptyNodeWeight,
            defragmentationNonEmptyNodeWeight,
            balancingByPercentage));
    }

    lbDomainEntries_.push_back(LoadBalancingDomainEntry(move(globalMetrics), globalMetricWeightSum, totalMetricCount, -1));
}

void BalanceCheckerCreator::CreateLoadsPerLBDomainList()
{
    // node loads based on all replicas except replicas that should Disappear (MoveInProgress or ToBeDropped)
    CreateLoadsPerLBDomainListInternal(loadsPerLBDomainList_, false);

    // node loads based on replicas that should Disappear
    CreateLoadsPerLBDomainListInternal(shouldDisappearLoadsPerLBDomainList_, true);
}

void BalanceCheckerCreator::CreateLoadsPerLBDomainListInternal(
    std::vector<std::vector<LoadEntry>> & loadsPerLBDomainList,
    bool shouldDisappear)
{
    // calculate node lbDomain entries: dimension: node count * lbDomain count
    loadsPerLBDomainList.reserve(nodes_.size());
    auto &globalDomain = lbDomainEntries_.back();

    vector<Service const*> const& services = partitionClosure_.Services;

    for (size_t i = 0; i < nodes_.size(); i++)
    {
        NodeDescription const& nodeDescription = nodes_[i].NodeDescriptionObj;
        vector<LoadEntry> loadsPerLBDomain;
        loadsPerLBDomain.reserve(lbDomainEntries_.size());

        for (size_t j = 0; j < lbDomainEntries_.size() - 1; j++)
        {
            loadsPerLBDomain.push_back(services[lbDomainEntries_[j].ServiceIndex]->GetNodeLoad(nodeDescription.NodeId, shouldDisappear));
        }

        // get global lb domain loads from serviceDomainMetrics
        loadsPerLBDomain.push_back(LoadEntry(globalDomain.MetricCount, 0));
        LoadEntry & globalLoads = loadsPerLBDomain.back();
        size_t globalMetricCount = globalLoads.Values.size();

        int metricIndex = 0;
        for (auto it = serviceDomainMetrics_.begin(); it != serviceDomainMetrics_.end(); ++it)
        {
            ASSERT_IF(static_cast<size_t>(metricIndex) >= globalMetricCount,
                "metricIndex {0} out of bound: [{1}, {2})", metricIndex, 0, globalMetricCount);
            globalLoads.Set(metricIndex++, it->second.GetLoad(nodeDescription.NodeId, shouldDisappear));
        }

        loadsPerLBDomainList.push_back(move(loadsPerLBDomain));
    }
}

void BalanceCheckerCreator::CreateNodeEntries()
{
    auto& globalLBDomainEntry = lbDomainEntries_.back();
    nodeEntries_.reserve(nodes_.size());
    deactivatedNodes_.reserve(nodes_.size());
    deactivatingNodesAllowPlacement_.reserve(nodes_.size());
    deactivatingNodesAllowServiceOnEveryNode_.reserve(nodes_.size());

    int nodeCount = static_cast<int>(nodes_.size());
    for (size_t i = 0; i < nodeCount; i++)
    {
        bool isNodeValid = false;
        LoadEntry capacityRatio(globalLBDomainEntry.MetricCount, 1);
        LoadEntry bufferedCapacity(globalLBDomainEntry.MetricCount, -1LL);
        LoadEntry totalCapacity(globalLBDomainEntry.MetricCount, -1LL);

        for (size_t j = 0; j < globalLBDomainEntry.MetricCount; j++)
        {
            auto &globalMetricName = globalLBDomainEntry.Metrics[j].Name;
            auto itCapacityRatio = capacityRatios_[i].find(globalMetricName);
            if (itCapacityRatio != capacityRatios_[i].end())
            {
                capacityRatio.Set(j, itCapacityRatio->second);
            }

            auto itBufferedCapacity = bufferedCapacities_[i].find(globalMetricName);
            if (itBufferedCapacity != bufferedCapacities_[i].end())
            {
                bufferedCapacity.Set(j, itBufferedCapacity->second);
            }

            auto itTotalCapacity = totalCapacities_[i].find(globalMetricName);
            if (itTotalCapacity != totalCapacities_[i].end())
            {
                totalCapacity.Set(j, itTotalCapacity->second);
            }
            if (globalLBDomainEntry.Metrics[j].IsValidNode(i))
            {
                // Node is valid if it is not blocklisted for any of the metrics.
                isNodeValid = true;
            }
        }

        NodeDescription const& nodeDescription = nodes_[i].NodeDescriptionObj;
        auto nodeImages = nodes_[i].NodeImages;

        if (nodeDescription.IsUp)
        {
            if (!faultDomainIndices_.empty())
            {
                TreeNodeIndex const& fdIndex = faultDomainIndices_[i];

                auto fdIt = faultDomainNodeSet_.find(fdIndex);
                if (fdIt == faultDomainNodeSet_.end())
                {
                    fdIt = faultDomainNodeSet_.insert(make_pair(fdIndex, NodeSet(nodeCount))).first;
                }

                fdIt->second.Add(static_cast<int>(i));
            }

            if (!upgradeDomainIndices_.empty())
            {
                TreeNodeIndex const& udIndex = upgradeDomainIndices_[i];

                auto udIt = upgradeDomainNodeSet_.find(udIndex);
                if (udIt == upgradeDomainNodeSet_.end())
                {
                    udIt = upgradeDomainNodeSet_.insert(make_pair(udIndex, NodeSet(nodeCount))).first;
                }

                udIt->second.Add(static_cast<int>(i));
            }
        }
        else
        {
            isNodeValid = false;
        }

        wstring nodeTypeName = L"";
        auto const& typeNamePropertyIt = nodeDescription.NodeProperties.find(*Constants::ImplicitNodeType);
        if (typeNamePropertyIt != nodeDescription.NodeProperties.end())
        {
            nodeTypeName = typeNamePropertyIt->second;
        }

        nodeEntries_.push_back(NodeEntry(
            static_cast<int>(i),
            nodeDescription.NodeId,
            NodeEntry::ConvertToFlatEntries(loadsPerLBDomainList_[i]),
            NodeEntry::ConvertToFlatEntries(shouldDisappearLoadsPerLBDomainList_[i]),
            move(capacityRatio),
            move(bufferedCapacity),
            move(totalCapacity),
            faultDomainIndices_.empty() ? TreeNodeIndex() : move(faultDomainIndices_[i]),
            upgradeDomainIndices_.empty() ? TreeNodeIndex() : move(upgradeDomainIndices_[i]),
            nodeDescription.IsDeactivated,
            nodeDescription.IsUp,
            move(nodeImages),
            isNodeValid,
            move(nodeTypeName)));

        if (!nodeDescription.IsUp)
        {
            downNodes_.push_back(static_cast<int>(i));
        }

        else if (nodeDescription.IsDeactivated)
        {
            if (nodeDescription.IsRestartOrPauseCheckInProgress)
            {
                deactivatingNodesAllowPlacement_.push_back(static_cast<int>(i));
            }
            else if (nodeDescription.IsRestartOrPauseCheckCompleted)
            {
                deactivatingNodesAllowServiceOnEveryNode_.push_back(static_cast<int>(i));
            }
            else
            {
                deactivatedNodes_.push_back(static_cast<int>(i));
            }
        }
    }
}

void BalanceCheckerCreator::GetUpgradedUDsIndex(std::set<std::wstring> const& UDs, std::set<Common::TreeNodeIndex> & UDsIndex) const
{
    if (upgradeDomainStructure_.IsEmpty || UDs.empty())
    {
        return;
    }

    for (auto itUDs = UDs.begin(); itUDs != UDs.end(); ++itUDs)
    {
        NodeDescription::DomainId domainId;
        domainId.push_back(*itUDs);
        UDsIndex.insert(GetDomainIndex(domainId, upgradeDomainStructure_));
    }
}

void BalanceCheckerCreator::CreateApplicationUpgradedUDs()
{
    if (upgradeDomainStructure_.IsEmpty)
    {
        return;
    }

    vector<Application const*> const& application = partitionClosure_.Applications;
    for (auto itApp = application.begin(); itApp != application.end(); ++itApp)
    {
        Application const* app = *itApp;
        if (app->UpgradeCompletedUDs.empty())
        {
            continue;
        }

        //Convert the ud string to tree index
        std::set<Common::TreeNodeIndex> UDsIndex;
        GetUpgradedUDsIndex(app->UpgradeCompletedUDs, UDsIndex);

        applicationUpgradedUDs_.insert(make_pair(app->ApplicationDesc.ApplicationId, move(UDsIndex)));
    }

}

BalanceCheckerUPtr BalanceCheckerCreator::Create()
{
    CreateNodes();
    CreateLoadBalancingDomainEntries();
    CreateLoadsPerLBDomainList();
    CreateNodeEntries();

    //Convert the fabric upgraded UDs string to tree index
    std::set<Common::TreeNodeIndex> UDsIndex;
    GetUpgradedUDsIndex(upgradeCompletedUDs_, UDsIndex);

    CreateApplicationUpgradedUDs();

    BalanceChecker::DomainTree fdStructure;

    if (!faultDomainStructure_.IsEmpty)
    {
        fdStructure.SetRoot(make_unique<BalanceChecker::DomainTree::Node>(
            BalanceChecker::DomainTree::Node::Create<BalanceCheckerCreator::DomainData>(
                faultDomainStructure_.Root,
                [](BalanceCheckerCreator::DomainData const& d) -> BalanceChecker::DomainData
        {
            return BalanceChecker::DomainData(d.NodeCount);
        })));
    }

    BalanceChecker::DomainTree udStructure;
    if (!upgradeDomainStructure_.IsEmpty)
    {
        udStructure.SetRoot(make_unique<BalanceChecker::DomainTree::Node>(
            BalanceChecker::DomainTree::Node::Create<BalanceCheckerCreator::DomainData>(
                upgradeDomainStructure_.Root,
                [](BalanceCheckerCreator::DomainData const& d) -> BalanceChecker::DomainData
        {
            return BalanceChecker::DomainData(d.NodeCount);
        })));
    }


    if (balancingDiagnosticsDataSPtr_ != nullptr)
    {
        balancingDiagnosticsDataSPtr_->servicesListPtr_ = &(partitionClosure_.Services);
    }

    return make_unique<BalanceChecker>(
        move(fdStructure),
        move(udStructure),
        move(lbDomainEntries_),
        move(nodeEntries_),
        move(deactivatedNodes_),
        move(deactivatingNodesAllowPlacement_),
        move(deactivatingNodesAllowServiceOnEveryNode_),
        move(downNodes_),
        existDefragMetric_,
        existScopedDefragMetric_,
        balancingDiagnosticsDataSPtr_,
        move(UDsIndex),
        move(applicationUpgradedUDs_),
        move(faultDomainMappings_),
        move(upgradeDomainMappings_),
        settings_,
        partitionClosure_.Type,
        move(metricsWithNodeCapacity_),
        move(faultDomainNodeSet_),
        move(upgradeDomainNodeSet_)
        );

}

bool BalanceCheckerCreator::IsDefragmentationMetric(std::wstring const& metricName)
{
    bool isMetricDefrag = false;

    auto defragmentationMetric = settings_.DefragmentationMetrics.find(metricName);
    if (defragmentationMetric != settings_.DefragmentationMetrics.end())
    {
        isMetricDefrag = defragmentationMetric->second;
    }
    auto placementStrategy = settings_.PlacementStrategy.find(metricName);
    if (placementStrategy != settings_.PlacementStrategy.end())
    {
        if (placementStrategy->second == Metric::PlacementStrategy::Balancing)
        {
            isMetricDefrag = false;
        }
        else
        {
            isMetricDefrag = true;
        }
    }

    return isMetricDefrag;
}

int32 BalanceCheckerCreator::GetDefragEmptyNodesCount(std::wstring const& metricName, bool scopedDefragEnabled)
{
    int32 targetNumberOfEmptyNodes = -1;
    auto percentOrNumberOfFreeNodesIt = settings_.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.find(metricName);
    if (percentOrNumberOfFreeNodesIt != settings_.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.end())
    {
        // calculate number of nodes that are eligible for placement of at least one service that reports defrag metric
        // (if a node is black-listed for all services which report defrag metric, then that node won't be included while calculating percentage of free nodes for defrag)
        size_t numberOfNodesWithDefragMetric = 0;
        numberOfNodesWithDefragMetric = nodes_.size();

        if (this->serviceDomainMetrics_.find(metricName) != this->serviceDomainMetrics_.end())
        {
            numberOfNodesWithDefragMetric -= this->serviceDomainMetrics_.find(metricName)->second.GetBlockNodeList().Count;
        }

        if (percentOrNumberOfFreeNodesIt->second >= 0 && percentOrNumberOfFreeNodesIt->second < 1)
        {
            // get the percent of empty nodes with defrag metric in cluster
            targetNumberOfEmptyNodes = static_cast<int32>(percentOrNumberOfFreeNodesIt->second * numberOfNodesWithDefragMetric);
        }
        else if (percentOrNumberOfFreeNodesIt->second >= 1.0)
        {
            // get the number of empty nodes with defrag metric in cluster
            targetNumberOfEmptyNodes = static_cast<int32>(percentOrNumberOfFreeNodesIt->second);
            if (targetNumberOfEmptyNodes > numberOfNodesWithDefragMetric)
            {
                targetNumberOfEmptyNodes = static_cast<int32>(numberOfNodesWithDefragMetric);
            }
        }
    }

    //If number of empty nodes is not specified, for defragmentation we empty 1 node in each domain
    //for Reservation, if number of nodes with reservation is not specified we don't reserve any capacity
    if (targetNumberOfEmptyNodes < 0 && scopedDefragEnabled)
    {
        targetNumberOfEmptyNodes = 0;
    }

    return targetNumberOfEmptyNodes;
}

int64 BalanceCheckerCreator::GetDefragEmptyNodeLoadTreshold(std::wstring const& metricName)
{
    int64 emptyNodeThreshold = 0;

    if (!GetDefragmentationScopedAlgorithmEnabled(metricName))
    {
        auto emptyNodeTresholdIt = settings_.MetricActivityThresholds.find(metricName);
        if (emptyNodeTresholdIt != settings_.MetricActivityThresholds.end())
        {
            emptyNodeThreshold = (metricName == ServiceModel::Constants::SystemMetricNameCpuCores) ?
                emptyNodeTresholdIt->second * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor :
                emptyNodeTresholdIt->second;
        }
    }

    return emptyNodeThreshold;
}

int64 BalanceCheckerCreator::GetReservationLoadAmount(std::wstring const& metricName, int64 nodeCapacity)
{
    int64 reservedLoad = 0;

    if (GetDefragmentationScopedAlgorithmEnabled(metricName))
    {
        auto reservedLoadIt = settings_.ReservedLoadPerNode.find(metricName);
        if (reservedLoadIt != settings_.ReservedLoadPerNode.end())
        {
            reservedLoad = reservedLoadIt->second;
            if (reservedLoad != 0 && metricName == ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                reservedLoad *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
            }
        }
        else
        {
            auto emptyNodeTresholdIt = settings_.MetricEmptyNodeThresholds.find(metricName);
            if (emptyNodeTresholdIt != settings_.MetricEmptyNodeThresholds.end())
            {
                auto emptyNodeThreshold = emptyNodeTresholdIt->second;
                if (metricName == ServiceModel::Constants::SystemMetricNameCpuCores)
                {
                    emptyNodeThreshold *= ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                }

                reservedLoad = nodeCapacity - emptyNodeThreshold;
            }
            else
            {
                // if it's not specified otherwise, reserve whole node capacity
                reservedLoad = nodeCapacity;
            }
        }
    }

    if (reservedLoad < 0)
    {
        reservedLoad = 0;
    }

    return reservedLoad;
}

Metric::DefragDistributionType BalanceCheckerCreator::GetDefragDistribution(std::wstring const& metricName)
{
    int emptyNodeDistribution = 0;
    auto emptyNodeDistributionIt = settings_.DefragmentationEmptyNodeDistributionPolicy.find(metricName);
    if (emptyNodeDistributionIt != settings_.DefragmentationEmptyNodeDistributionPolicy.end())
    {
        // get the distribution of empty nodes => 0 - spread across domains, 1 - don't care how nodes are spread
        emptyNodeDistribution = emptyNodeDistributionIt->second;
        switch (emptyNodeDistribution)
        {
        case 0: return Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
        case 1: return Metric::DefragDistributionType::NumberOfEmptyNodes;
        default: return Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
        }
    }

    return Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
}

double BalanceCheckerCreator::GetPlacementHeuristicIncomingLoadFactor(std::wstring const& metricName)
{
    double placementHeuristicIncomingLoadFactor = 0.0;
    auto itPlacementHeuristicIncomingLoadFactor = settings_.PlacementHeuristicIncomingLoadFactor.find(metricName);
    if (itPlacementHeuristicIncomingLoadFactor != settings_.PlacementHeuristicIncomingLoadFactor.end())
    {
        placementHeuristicIncomingLoadFactor = itPlacementHeuristicIncomingLoadFactor->second;
    }

    return placementHeuristicIncomingLoadFactor;
}

double BalanceCheckerCreator::GetPlacementHeuristicEmptySpacePercent(std::wstring const& metricName)
{
    double placementHeuristicEmptySpacePercent = 0.0;
    auto itPlacementHeuristicEmptySpacePercent = settings_.PlacementHeuristicEmptySpacePercent.find(metricName);
    if (itPlacementHeuristicEmptySpacePercent != settings_.PlacementHeuristicEmptySpacePercent.end())
    {
        placementHeuristicEmptySpacePercent = itPlacementHeuristicEmptySpacePercent->second;
    }

    return placementHeuristicEmptySpacePercent;
}

bool BalanceCheckerCreator::GetDefragmentationScopedAlgorithmEnabled(std::wstring const& metricName)
{
    bool scopedDefragmentationAlgorithmEnabled = false;
    auto itScopedDefragmentationAlgorithmenabled = settings_.DefragmentationScopedAlgorithmEnabled.find(metricName);
    auto itPlacementStrategy = settings_.PlacementStrategy.find(metricName);
    if (itScopedDefragmentationAlgorithmenabled != settings_.DefragmentationScopedAlgorithmEnabled.end())
    {
        scopedDefragmentationAlgorithmEnabled = itScopedDefragmentationAlgorithmenabled->second;
    }
    if (itPlacementStrategy != settings_.PlacementStrategy.end())
    {
        if (itPlacementStrategy->second == Metric::PlacementStrategy::Reservation ||
            itPlacementStrategy->second == Metric::PlacementStrategy::ReservationAndBalance ||
            itPlacementStrategy->second == Metric::PlacementStrategy::ReservationAndPack)
        {
            scopedDefragmentationAlgorithmEnabled = true;
        }
        else
        {
            scopedDefragmentationAlgorithmEnabled = false;
        }
    }

    return scopedDefragmentationAlgorithmEnabled;
}

Metric::PlacementStrategy BalanceCheckerCreator::GetPlacementStrategy(std::wstring const& metricName)
{
    // Default behavior is: no preference - this means that replicas will be placed on a randomly chosen node
    Metric::PlacementStrategy placementStrategy = Metric::PlacementStrategy::Balancing;
    int placementStrategyInt;
    auto itPlacementStrategy = settings_.PlacementStrategy.find(metricName);
    if (itPlacementStrategy != settings_.PlacementStrategy.end())
    {
        placementStrategyInt = itPlacementStrategy->second;
        switch (placementStrategyInt)
        {
        case 0: return Metric::PlacementStrategy::Balancing;
        case 1: return Metric::PlacementStrategy::ReservationAndBalance;
        case 2: return Metric::PlacementStrategy::Reservation;
        case 3: return Metric::PlacementStrategy::ReservationAndPack;
        case 4: return Metric::PlacementStrategy::Defragmentation;
        default: Assert::TestAssert("Placement strategy is not set properly");
        }
    }
    auto defragmentationMetric = settings_.DefragmentationMetrics.find(metricName);
    if (defragmentationMetric != settings_.DefragmentationMetrics.end())
    {
        if (defragmentationMetric->second)
        {
            if (GetDefragmentationScopedAlgorithmEnabled(metricName))
            {
                placementStrategy = Metric::PlacementStrategy::ReservationAndBalance;
            }
            else
            {
                placementStrategy = Metric::PlacementStrategy::Defragmentation;
            }
        }
    }
    return placementStrategy;
}

double BalanceCheckerCreator::GetDefragmentationEmptyNodeWeight(std::wstring const& metricName, Metric::PlacementStrategy placementStrategy)
{
    double defragmentationEmptyNodeWeight = 0;
    auto itDefragmentationEmptyNodeWeight = settings_.DefragmentationEmptyNodeWeight.find(metricName);
    if (itDefragmentationEmptyNodeWeight != settings_.DefragmentationEmptyNodeWeight.end())
    {
        defragmentationEmptyNodeWeight = itDefragmentationEmptyNodeWeight->second;

        if (defragmentationEmptyNodeWeight > 1)
        {
            defragmentationEmptyNodeWeight = 1;
        }
    }
    else
    {
        if (placementStrategy == Metric::PlacementStrategy::ReservationAndBalance || 
            placementStrategy == Metric::PlacementStrategy::ReservationAndPack || 
            placementStrategy == Metric::PlacementStrategy::Defragmentation)
        {
            defragmentationEmptyNodeWeight = 0.999;
        }
        else if (placementStrategy == Metric::PlacementStrategy::Reservation)
        {
            defragmentationEmptyNodeWeight = 1;
        }
    }
    return defragmentationEmptyNodeWeight;
}

double BalanceCheckerCreator::GetDefragmentationNonEmptyNodeWeight(std::wstring const& metricName, Metric::PlacementStrategy placementStrategy)
{
    double defragmentationNonEmptyNodeWeight = 1 - GetDefragmentationEmptyNodeWeight(metricName, placementStrategy);

    auto itDefragmentationNonEmptyNodeWeight = settings_.DefragmentationNonEmptyNodeWeight.find(metricName);
    if (itDefragmentationNonEmptyNodeWeight != settings_.DefragmentationNonEmptyNodeWeight.end())
    {
        defragmentationNonEmptyNodeWeight = itDefragmentationNonEmptyNodeWeight->second;
    }

    return defragmentationNonEmptyNodeWeight;
}

bool BalanceCheckerCreator::IsBalancingByPercentageEnabled(std::wstring const & metricName)
{
    bool balancingByPercentage = false;
    auto itBalancingByPercentage = settings_.BalancingByPercent.find(metricName);
    if (itBalancingByPercentage != settings_.BalancingByPercent.end())
    {
        balancingByPercentage = itBalancingByPercentage->second;
    }

    return balancingByPercentage;
}
