// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AccumulatorWithMinMax.h"
#include "Service.h"
#include "BalanceChecker.h"
#include "CandidateSolution.h"
#include "PLBConfig.h"
#include "PlacementAndLoadBalancing.h"
#include "DiagnosticsDataStructures.h"
#include "SearcherSettings.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

BalanceChecker::BalanceChecker(
    DomainTree && faultDomainStructure,
    DomainTree && upgradeDomainStructure,
    vector<LoadBalancingDomainEntry> && lbDomainEntries,
    vector<NodeEntry> && nodeEntries,
    std::vector<int> && deactivatedNodes,
    std::vector<int> && deactivatingNodesAllowPlacement,
    std::vector<int> && deactivatingNodesAllowServiceOnEveryNode,
    std::vector<int> && downNodes,
    bool existDefragMetric,
    bool existScopedDefragMetric,
    BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr,
    std::set<Common::TreeNodeIndex> && UDsIndex,
    std::map<uint64, std::set<Common::TreeNodeIndex>> && appUDs,
    std::map<Common::TreeNodeIndex, std::vector<wstring>> && faultDomainMappings,
    std::map<Common::TreeNodeIndex, wstring> && upgradeDomainMappings,
    SearcherSettings const & settings,
    PartitionClosureType::Enum closureType,
    std::set<std::wstring> && metricsWithNodeCapacity,
    std::map<Common::TreeNodeIndex, NodeSet> && faultDomainNodeSet,
    std::map<Common::TreeNodeIndex, NodeSet> && upgradeDomainNodeSet
)
    :
faultDomainStructure_(move(faultDomainStructure)),
upgradeDomainStructure_(move(upgradeDomainStructure)),
lbDomainEntries_(move(lbDomainEntries)),
nodeEntries_(move(nodeEntries)),
deactivatedNodes_(move(deactivatedNodes)),
deactivatingNodesAllowPlacement_(move(deactivatingNodesAllowPlacement)),
deactivatingNodesAllowServiceOnEveryNode_(move(deactivatingNodesAllowServiceOnEveryNode)),
downNodes_(downNodes),
totalMetricCount_(0),
isBalanced_(true),
globalMetricIndicesList_(),
faultDomainLoads_(),
upgradeDomainLoads_(),
dynamicNodeLoads_(nodeEntries_, lbDomainEntries_, settings.NodesWithReservedLoadOverlap),
existDefragMetric_(existDefragMetric),
existScopedDefragMetric_(existScopedDefragMetric),
balancingDiagnosticsDataSPtr_(balancingDiagnosticsDataSPtr),
upgradeCompletedUDsIndex_(move(UDsIndex)),
applicationUpgradedUDs_(move(appUDs)),
faultDomainMappings_(move(faultDomainMappings)),
upgradeDomainMappings_(move(upgradeDomainMappings)),
settings_(settings),
closureType_(closureType),
metricsWithNodeCapacity_(move(metricsWithNodeCapacity)),
faultDomainNodeSet_(move(faultDomainNodeSet)),
upgradeDomainNodeSet_(move(upgradeDomainNodeSet))
{

    //all the nodes that are filtered here should NOT be filtered in PlacementCreator::GetFilteredDomainTree
    FilterDomainTree(faultDomainStructure_, downNodes_, true);
    FilterDomainTree(upgradeDomainStructure_, downNodes_, false);
    FilterDomainTree(faultDomainStructure_, deactivatedNodes_, true);
    FilterDomainTree(upgradeDomainStructure_, deactivatedNodes_, false);
    FilterDomainTree(faultDomainStructure_, deactivatingNodesAllowServiceOnEveryNode_, true);
    FilterDomainTree(upgradeDomainStructure_, deactivatingNodesAllowServiceOnEveryNode_, false);
    CreateGlobalMetricIndicesList();
    RefreshIsBalanced();
}

void BalanceChecker::FilterDomainTree(BalanceChecker::DomainTree & tree, std::vector<int> const& nodesToRemove, bool isFaultDomain)
{
    if (tree.IsEmpty)
    {
        return;
    }

    for (auto nodeIndex : nodesToRemove)
    {
        TreeNodeIndex const& treeNodeIndex = isFaultDomain ? nodeEntries_[nodeIndex].FaultDomainIndex : nodeEntries_[nodeIndex].UpgradeDomainIndex;

        tree.ForEachNodeInPath(treeNodeIndex, [](BalanceChecker::DomainTree::Node & n)
        {
            ASSERT_IFNOT(n.Data.NodeCount > 0, "Node count should be greater than 0");
            --n.DataRef.NodeCount;
        });
    }
}

void BalanceChecker::CreateGlobalMetricIndicesList()
{
    std::map<std::wstring, size_t> metricNameToGlobalIndex;
    auto& globalLBDomainEntry = lbDomainEntries_.back();
    for (size_t i = 0; i < globalLBDomainEntry.MetricCount; i++)
    {
        metricNameToGlobalIndex.insert(
            make_pair(globalLBDomainEntry.Metrics[i].Name,
                i + globalLBDomainEntry.MetricStartIndex));
    }

    globalMetricIndicesList_.resize(lbDomainEntries_.size() - 1);
    for (size_t i = 0; i < lbDomainEntries_.size() - 1; i++)
    {
        auto &metrics = lbDomainEntries_[i].Metrics;
        size_t metricCount = lbDomainEntries_[i].MetricCount;
        totalMetricCount_ += metricCount;
        globalMetricIndicesList_[i].resize(metricCount);
        for (size_t j = 0; j < metricCount; j++)
        {
            auto it = metricNameToGlobalIndex.find(metrics[j].Name);
            ASSERT_IF(it == metricNameToGlobalIndex.end(),
                "Metric {0} does not exist in metricNameToGlobalIndex for lbDomain {1}",
                metrics[j].Name,
                i);
            globalMetricIndicesList_[i][j] = it->second;
        }
    }

    totalMetricCount_ += globalLBDomainEntry.MetricCount;
}

void BalanceChecker::InitializeEmptyDomainAccMinMaxTree(
    size_t totalMetricCount,
    DomainTree const & sourceTree,
    LoadBalancingDomainEntry::DomainAccMinMaxTree & targetTree)
{
    if (!sourceTree.IsEmpty)
    {
        targetTree.SetRoot(make_unique<LoadBalancingDomainEntry::DomainAccMinMaxTree::Node>(
            LoadBalancingDomainEntry::DomainAccMinMaxTree::Node::CreateEmpty<DomainData>(
                sourceTree.Root,
                [totalMetricCount]() -> LoadBalancingDomainEntry::DomainMetricAccsWithMinMax
        {
            return LoadBalancingDomainEntry::DomainMetricAccsWithMinMax(totalMetricCount);
        })));
    }
}

void BalanceChecker::RefreshIsBalanced()
{
    if (ExistDefragMetric)
    {
        InitializeEmptyDomainAccMinMaxTree(totalMetricCount_, FaultDomainStructure, faultDomainLoads_);
        InitializeEmptyDomainAccMinMaxTree(totalMetricCount_, UpgradeDomainStructure, upgradeDomainLoads_);

        if (ExistScopedDefragMetric)
        {
            dynamicNodeLoads_.AdvanceVersion();

            size_t totalMetricIndex = 0;
            for (auto itDomain = lbDomainEntries_.begin(); itDomain != lbDomainEntries_.end(); ++itDomain)
            {
                for (auto itMetric = itDomain->Metrics.begin(); itMetric != itDomain->Metrics.end(); ++itMetric, ++totalMetricIndex)
                {
                    if (!itMetric->DefragmentationScopedAlgorithmEnabled)
                    {
                        continue;
                    }

                    dynamicNodeLoads_.PrepareBeneficialNodes(totalMetricIndex,
                        itMetric->DefragNodeCount,
                        itMetric->DefragDistribution,
                        itMetric->ReservationLoad);
                }
            }
        }

        if (!faultDomainLoads_.IsEmpty || !upgradeDomainLoads_.IsEmpty)
        {
            // accumulate node loads per FD/UD
            for (size_t k = 0; k < nodeEntries_.size(); k++)
            {
                if (nodeEntries_[k].IsDeactivated || !nodeEntries_[k].IsUp)
                {
                    continue;
                }

                size_t totalMetricIndex = 0;
                size_t domainId = 0;
                for (auto itDomain = lbDomainEntries_.begin(); itDomain != lbDomainEntries_.end(); ++itDomain, ++domainId)
                {
                    size_t metricId = 0;
                    for (auto itMetric = itDomain->Metrics.begin(); itMetric != itDomain->Metrics.end(); ++itMetric, ++metricId)
                    {
                        if (itMetric->IsValidNode(nodeEntries_[k].NodeIndex))
                        {
                            auto globalMetricIndexStart = lbDomainEntries_.back().MetricStartIndex;
                            size_t globalMetricIndex = 0;
                            if (itDomain->IsGlobal)
                            {
                                globalMetricIndex = metricId;
                            }
                            else
                            {
                                // globalMetricIndices are saved for each local domain
                                globalMetricIndex = globalMetricIndicesList_[domainId][metricId] - globalMetricIndexStart;
                            }
                            size_t loadLevel = nodeEntries_[k].GetLoadLevel(totalMetricIndex);
                            int64 nodeCapacity = nodeEntries_[k].GetNodeCapacity(globalMetricIndex);

                            // Max load to be able to store reservation load
                            size_t loadThreshold = 0;
                            bool nodeWithEnoughCapacity = true;
                            if (itMetric->DefragmentationScopedAlgorithmEnabled)
                            {
                                if (nodeCapacity < itMetric->ReservationLoad)
                                {
                                    nodeWithEnoughCapacity = false;
                                }
                                loadThreshold = nodeCapacity - itMetric->ReservationLoad;
                            }
                            else
                            {
                                loadThreshold = itMetric->DefragEmptyNodeLoadThreshold;
                            }

                            if (!faultDomainLoads_.IsEmpty)
                            {
                                AccumulatorWithMinMax&  faultDomainLoadsAcc = faultDomainLoads_.GetNodeByIndex(
                                    nodeEntries_[k].FaultDomainIndex).DataRef.AccMinMaxEntries[totalMetricIndex];
                                faultDomainLoadsAcc.AddOneValue(loadLevel, nodeCapacity, nodeEntries_[k].NodeId);

                                if (nodeWithEnoughCapacity && loadLevel <= loadThreshold)
                                {
                                    faultDomainLoadsAcc.AddEmptyNodes(1);
                                }
                            }

                            if (!upgradeDomainLoads_.IsEmpty)
                            {
                                AccumulatorWithMinMax&  upgradeDomainLoadsAcc = upgradeDomainLoads_.GetNodeByIndex(
                                    nodeEntries_[k].UpgradeDomainIndex).DataRef.AccMinMaxEntries[totalMetricIndex];
                                upgradeDomainLoadsAcc.AddOneValue(loadLevel, nodeCapacity, nodeEntries_[k].NodeId);
                                
                                if (nodeWithEnoughCapacity && loadLevel <= loadThreshold)
                                {
                                    upgradeDomainLoadsAcc.AddEmptyNodes(1);
                                }
                            }
                        }

                        totalMetricIndex++;
                    }
                }
            }

            // accumulate FD/UD loads so score can know stdDev of total FD/UDs load
            for (int i = static_cast<int>(lbDomainEntries_.size()) - 1; i >= 0; i--)
            {
                LoadBalancingDomainEntry & lbDomain = lbDomainEntries_[i];
                size_t metricCount = lbDomain.MetricCount;
                for (size_t j = 0; j < metricCount; j++)
                {
                    if (!faultDomainLoads_.IsEmpty)
                    {
                        AccumulatorWithMinMax & fdLoadStatAccumulator = lbDomain.GetFdLoadStat(j);
                        faultDomainLoads_.ForEachNodeInPreOrder([&](LoadBalancingDomainEntry::DomainAccMinMaxTree::Node const & node)
                        {
                            if (node.Children.size() == 0)
                            {
                                fdLoadStatAccumulator.AddOneValue((int64)node.Data.AccMinMaxEntries[lbDomain.MetricStartIndex + j].Sum, 1, Federation::NodeId::MinNodeId);
                            }
                        });
                    }

                    if (!upgradeDomainLoads_.IsEmpty)
                    {
                        AccumulatorWithMinMax & udLoadStatAccumulator = lbDomain.GetUdLoadStat(j);
                        upgradeDomainLoads_.ForEachNodeInPreOrder([&](LoadBalancingDomainEntry::DomainAccMinMaxTree::Node const & node)
                        {
                            if (node.Children.size() == 0)
                            {
                                udLoadStatAccumulator.AddOneValue((int64)node.Data.AccMinMaxEntries[lbDomain.MetricStartIndex + j].Sum, 1, Federation::NodeId::MinNodeId);
                            }
                        });
                    }
                }
            }
        }
    }

    auto& globalLBDomainEntry = lbDomainEntries_.back();
    for (int i = static_cast<int>(lbDomainEntries_.size()) - 1; i >= 0; i--)
    {
        LoadBalancingDomainEntry & lbDomain = lbDomainEntries_[i];
        size_t metricCount = lbDomain.MetricCount;
        for (size_t j = 0; j < metricCount; j++)
        {
            Metric const& metric = lbDomain.Metrics[j];
            AccumulatorWithMinMax & loadStat = lbDomain.GetLoadStat(j);
            for (size_t k = 0; k < nodeEntries_.size(); k++)
            {
                if (!nodeEntries_[k].IsDeactivated && nodeEntries_[k].IsUp && metric.IsValidNode(nodeEntries_[k].NodeIndex))
                {
                    size_t totalMetricIndex = lbDomain.MetricStartIndex + j;
                    int64 load = nodeEntries_[k].GetLoadLevel(totalMetricIndex);
                    int64 capacity = nodeEntries_[k].GetNodeCapacity(j);
                    loadStat.AddOneValue(load, capacity, nodeEntries_[k].NodeId);

                    if (ExistScopedDefragMetric &&
                        metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled)
                    {
                        if (!dynamicNodeLoads_.IsBeneficialNode(nodeEntries_[k].NodeIndex, totalMetricIndex))
                        {
                            loadStat.AddNonBeneficialLoad(load);
                        }
                    }
                }
            }
        }

        if (lbDomain.IsGlobal)
        {
            lbDomain.RefreshIsBalanced(
                nullptr,
                globalLBDomainEntry,
                faultDomainLoads_,
                upgradeDomainLoads_,
                dynamicNodeLoads_,
                balancingDiagnosticsDataSPtr_);
        }
        else
        {
            lbDomain.RefreshIsBalanced(
                &globalMetricIndicesList_[i],
                globalLBDomainEntry,
                faultDomainLoads_,
                upgradeDomainLoads_,
                dynamicNodeLoads_,
                balancingDiagnosticsDataSPtr_);
        }

        balancingDiagnosticsDataSPtr_->changed_ = true;

        if (!lbDomain.IsBalanced)
        {
            isBalanced_ = false;
        }
    }
}

void BalanceChecker::CalculateMetricStatisticsForTracing(bool isBegin, const Score& score, const NodeMetrics& nodeChanges)
{
    LoadBalancingDomainEntry::DomainAccMinMaxTree faultDomainLoads;
    LoadBalancingDomainEntry::DomainAccMinMaxTree upgradeDomainLoads;

    InitializeEmptyDomainAccMinMaxTree(totalMetricCount_, FaultDomainStructure, faultDomainLoads);
    InitializeEmptyDomainAccMinMaxTree(totalMetricCount_, UpgradeDomainStructure, upgradeDomainLoads);

    LoadBalancingDomainEntry & globalDomain = lbDomainEntries_.back();
    size_t totalMetricIndex = totalMetricCount_ - globalDomain.MetricCount;
    TraceAllBalancingMetricInfo balancingMetricInfo;

    for (auto itMetric = globalDomain.Metrics.begin(); itMetric != globalDomain.Metrics.end(); ++itMetric)
    {
        if (itMetric->IsDefrag)
        {
            size_t totalEmptyNodeCnt = 0;
            size_t totalNonEmptyNodeCnt = 0;
            int64 totalNonEmptyNodeLoad = 0;

            for (size_t k = 0; k < nodeEntries_.size(); k++)
            {
                if (nodeEntries_[k].IsDeactivated || !nodeEntries_[k].IsUp)
                {
                    continue;
                }

                if (itMetric->IsValidNode(nodeEntries_[k].NodeIndex))
                {
                    size_t loadLevel;
                    if (isBegin)
                    {
                        loadLevel = nodeEntries_[k].GetLoadLevel(totalMetricIndex);
                    }
                    else
                    {
                        loadLevel = nodeEntries_[k].GetLoadLevel(totalMetricIndex) + nodeChanges[&nodeEntries_[k]].Values[totalMetricIndex];
                    }
                    int64 nodeCapacity = nodeEntries_[k].GetNodeCapacity(totalMetricIndex - globalDomain.MetricStartIndex);
                    
                    // Max load to be able to store reservation load
                    size_t loadThreshold = 0;
                    bool nodeWithEnoughCapacity = true;
                    if (itMetric->DefragmentationScopedAlgorithmEnabled)
                    {
                        if (nodeCapacity < itMetric->ReservationLoad)
                        {
                            nodeWithEnoughCapacity = false;
                        }
                        loadThreshold = nodeCapacity - itMetric->ReservationLoad;
                    }
                    else
                    {
                        loadThreshold = itMetric->DefragEmptyNodeLoadThreshold;
                    }

                    if (nodeWithEnoughCapacity && loadLevel <= loadThreshold)
                    {
                        ++totalEmptyNodeCnt;
                    }
                    else
                    {
                        ++totalNonEmptyNodeCnt;
                        totalNonEmptyNodeLoad += loadLevel;
                    }

                    if (!faultDomainLoads_.IsEmpty)
                    {
                        AccumulatorWithMinMax&  faultDomainLoadsAcc = faultDomainLoads.GetNodeByIndex(nodeEntries_[k].FaultDomainIndex).DataRef.AccMinMaxEntries[totalMetricIndex];

                        if (nodeWithEnoughCapacity && loadLevel <= loadThreshold)
                        {
                            faultDomainLoadsAcc.AddEmptyNodes(1);
                        }
                        else
                        {
                            faultDomainLoadsAcc.AddNonEmptyLoad(loadLevel);
                        }
                    }

                    if (!upgradeDomainLoads_.IsEmpty)
                    {
                        AccumulatorWithMinMax&  upgradeDomainLoadsAcc = upgradeDomainLoads.GetNodeByIndex(nodeEntries_[k].UpgradeDomainIndex).DataRef.AccMinMaxEntries[totalMetricIndex];

                        if (nodeWithEnoughCapacity && loadLevel <= loadThreshold)
                        {
                            upgradeDomainLoadsAcc.AddEmptyNodes(1);
                        }
                        else
                        {
                            upgradeDomainLoadsAcc.AddNonEmptyLoad(loadLevel);
                        }
                    }
                }
            }

            TraceDefragMetricInfo defragMetricInfo;
            defragMetricInfo.metricName = itMetric->Name;

            for (auto itFaultDomain = faultDomainMappings_.begin(); itFaultDomain != faultDomainMappings_.end(); ++itFaultDomain)
            {
                AccumulatorWithMinMax&  faultDomainLoadsAcc = faultDomainLoads.GetNodeByIndex(itFaultDomain->first).DataRef.AccMinMaxEntries[totalMetricIndex];
                defragMetricInfo.faultDomainItems_[itFaultDomain->second] = std::make_pair<size_t, double>(faultDomainLoadsAcc.EmptyNodeCount, faultDomainLoadsAcc.NonEmptyAverageLoad);
            }
            for (auto itUpgradeDomain = upgradeDomainMappings_.begin(); itUpgradeDomain != upgradeDomainMappings_.end(); ++itUpgradeDomain)
            {
                AccumulatorWithMinMax&  upgradeDomainLoadsAcc = upgradeDomainLoads.GetNodeByIndex(itUpgradeDomain->first).DataRef.AccMinMaxEntries[totalMetricIndex];
                defragMetricInfo.upgradeDomainItems_[itUpgradeDomain->second] = std::make_pair<size_t, double>(upgradeDomainLoadsAcc.EmptyNodeCount, upgradeDomainLoadsAcc.NonEmptyAverageLoad);
            }
            if (score.FaultDomainScores[totalMetricIndex].Count > 0)
            {
                defragMetricInfo.avgStdDevFaultDomain_ = score.FaultDomainScores[totalMetricIndex].NormStdDev;
                defragMetricInfo.averageFdLoad_ = score.FaultDomainScores[totalMetricIndex].Average;
            }
            else
            {
                defragMetricInfo.avgStdDevFaultDomain_ = 0.0;
                defragMetricInfo.averageFdLoad_ = 0.0;

            }
            if (score.UpgradeDomainScores[totalMetricIndex].Count > 0)
            {
                defragMetricInfo.avgStdDevUpgradeDomain_ = score.UpgradeDomainScores[totalMetricIndex].NormStdDev;
                defragMetricInfo.averageUdLoad_ = score.UpgradeDomainScores[totalMetricIndex].Average;
            }
            else
            {
                defragMetricInfo.avgStdDevUpgradeDomain_ = 0.0;
                defragMetricInfo.averageUdLoad_ = 0.0;
            }
            defragMetricInfo.avgStdDev_ = score.MetricScores[totalMetricIndex].NormStdDev;
            defragMetricInfo.totalClusterLoad_ = score.MetricScores[totalMetricIndex].Sum;
            defragMetricInfo.averageLoad_ = score.MetricScores[totalMetricIndex].Average;
            if (totalNonEmptyNodeCnt > 0)
            {
                defragMetricInfo.nonEmptyNodeLoad_ = (double)totalNonEmptyNodeLoad / totalNonEmptyNodeCnt;
            }
            else
            {
                defragMetricInfo.nonEmptyNodeLoad_ = 0.0;
            }
            defragMetricInfo.emptyNodeCnt_ = totalEmptyNodeCnt;

            if (itMetric->DefragmentationScopedAlgorithmEnabled)
            {
                defragMetricInfo.type_ = L"Scoped";
            }
            else
            {
                defragMetricInfo.type_ = L"Full";
            }

            defragMetricInfo.targetEmptyNodeCount_ = itMetric->DefragNodeCount;
            defragMetricInfo.emptyNodesDistribution_ = itMetric->DefragDistribution;
            defragMetricInfo.emptyNodeLoadThreshold_ = itMetric->DefragEmptyNodeLoadThreshold;
            defragMetricInfo.reservationLoad_ = itMetric->ReservationLoad;

            defragMetricInfo.emptyNodesWeight_ = itMetric->DefragmentationEmptyNodeWeight;
            defragMetricInfo.nonEmptyNodesWeight_ = itMetric->DefragmentationNonEmptyNodeWeight;
            defragMetricInfo.activityThreshold_ = itMetric->ActivityThreshold;
            defragMetricInfo.balancingThreshold_ = itMetric->BalancingThreshold;

            if (isBegin)
            {
                PlacementAndLoadBalancing::PLBTrace->DefragMetricInfoStart(defragMetricInfo);
            }
            else
            {
                PlacementAndLoadBalancing::PLBTrace->DefragMetricInfoEnd(defragMetricInfo);
            }
        }
        else
        {
            balancingMetricInfo.items_[itMetric->Name] = TraceAllBalancingMetricInfo::BalancingMetricData(
                score.MetricScores[totalMetricIndex].NormStdDev,
                score.MetricScores[totalMetricIndex].Average,
                score.MetricScores[totalMetricIndex].Sum,
                itMetric->ActivityThreshold,
                itMetric->BalancingThreshold);
        }
        totalMetricIndex++;
    }

    if (balancingMetricInfo.items_.size() > 0)
    {
        if (isBegin)
        {
            PlacementAndLoadBalancing::PLBTrace->BalancingMetricInfoStart(balancingMetricInfo);
        }
        else
        {
            PlacementAndLoadBalancing::PLBTrace->BalancingMetricInfoEnd(balancingMetricInfo);
        }
    }
}

void BalanceChecker::UpdateNodeThrottlingLimit(int nodeIndex, int throttlingLimit)
{
    if (nodeIndex >= nodeEntries_.size())
    {
        return;
    }
    nodeEntries_[nodeIndex].MaxConcurrentBuilds = throttlingLimit;
}
