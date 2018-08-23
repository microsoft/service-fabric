// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Score.h"
#include "Placement.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

Score::Score(
    size_t totalMetricCount,
    std::vector<LoadBalancingDomainEntry> const& lbDomainEntries,
    size_t totalReplicaCount,
    LoadBalancingDomainEntry::DomainAccMinMaxTree const& faultDomainInitialLoads,
    LoadBalancingDomainEntry::DomainAccMinMaxTree const& upgradeDomainInitialLoads,
    bool existDefragMetric,
    bool existScopedDefragMetric,
    SearcherSettings const & settings,
    DynamicNodeLoadSet* dynamicNodeLoads)
    : totalMetricCount_(totalMetricCount),
    lbDomainEntries_(lbDomainEntries),
    totalReplicaCount_(totalReplicaCount),
    dynamicNodeLoads_(dynamicNodeLoads),
    nodeMetricScores_(),
    udMetricScores_(),
    fdMetricScores_(),
    existDefragMetric_(existDefragMetric),
    existScopedDefragMetric_(existScopedDefragMetric),
    defragTargetEmptyNodesAchieved_(false),
    avgStdDev_(0.0),
    cost_(0.0),
    energy_(0.0),
    settings_(settings)
{
    if (existDefragMetric_)
    {
        InitializeDomainAccTree(faultDomainInitialLoads, faultDomainInitialLoads_);
        InitializeDomainAccTree(upgradeDomainInitialLoads, upgradeDomainInitialLoads_);

        udMetricScores_.reserve(totalMetricCount_);
        fdMetricScores_.reserve(totalMetricCount_);
    }

    nodeMetricScores_.reserve(totalMetricCount_);

    size_t lbDomainCount = lbDomainEntries_.size();

    for (size_t i = 0; i < lbDomainCount; i++)
    {
        LoadBalancingDomainEntry const& lbDomain = lbDomainEntries_[i];
        size_t metricCount = lbDomain.MetricCount;

        for (size_t j = 0; j < metricCount; j++)
        {
            AccumulatorWithMinMax const& loadStat = lbDomain.GetLoadStat(j);
            nodeMetricScores_.push_back(Accumulator(loadStat));

            if (existDefragMetric_)
            {
                AccumulatorWithMinMax const& fdLoadStat = lbDomain.GetFdLoadStat(j);
                AccumulatorWithMinMax const& udLoadStat = lbDomain.GetUdLoadStat(j);
                fdMetricScores_.push_back(Accumulator(fdLoadStat));
                udMetricScores_.push_back(Accumulator(udLoadStat));
            }
        }
    }

    Calculate(0);
}

Score::Score(Score && other)
    : totalMetricCount_(other.totalMetricCount_),
    lbDomainEntries_(other.lbDomainEntries_),
    totalReplicaCount_(other.totalReplicaCount_),
    faultDomainInitialLoads_(move(other.faultDomainInitialLoads_)),
    upgradeDomainInitialLoads_(move(other.upgradeDomainInitialLoads_)),
    dynamicNodeLoads_(other.dynamicNodeLoads_),
    nodeMetricScores_(move(other.nodeMetricScores_)),
    udMetricScores_(move(other.udMetricScores_)),
    fdMetricScores_(move(other.fdMetricScores_)),
    existDefragMetric_(other.existDefragMetric_),
    existScopedDefragMetric_(other.existScopedDefragMetric_),
    defragTargetEmptyNodesAchieved_(other.defragTargetEmptyNodesAchieved_),
    avgStdDev_(other.avgStdDev_),
    cost_(other.cost_),
    energy_(other.energy_),
    settings_(other.settings_)
{
}

Score::Score(Score const& other)
    : totalMetricCount_(other.totalMetricCount_),
    lbDomainEntries_(other.lbDomainEntries_),
    totalReplicaCount_(other.totalReplicaCount_),
    faultDomainInitialLoads_(other.faultDomainInitialLoads_),
    upgradeDomainInitialLoads_(other.upgradeDomainInitialLoads_),
    dynamicNodeLoads_(other.dynamicNodeLoads_),
    nodeMetricScores_(other.nodeMetricScores_),
    udMetricScores_(other.udMetricScores_),
    fdMetricScores_(other.fdMetricScores_),
    existDefragMetric_(other.existDefragMetric_),
    existScopedDefragMetric_(other.existScopedDefragMetric_),
    defragTargetEmptyNodesAchieved_(other.defragTargetEmptyNodesAchieved_),
    avgStdDev_(other.avgStdDev_),
    cost_(other.cost_),
    energy_(other.energy_),
    settings_(other.settings_)
{
}

Score & Score::operator = (Score && other)
{
    if (this != &other)
    {
        // lbDomainEntries_ shouldn't change during PLB run
        ASSERT_IFNOT(totalMetricCount_ == other.totalMetricCount_,
            "Assign from a score with a different total metric count: {0} {1}",
            totalMetricCount_,
            other.totalMetricCount_);

        totalReplicaCount_ = other.totalReplicaCount_;
        faultDomainInitialLoads_ = move(other.faultDomainInitialLoads_);
        upgradeDomainInitialLoads_ = move(other.upgradeDomainInitialLoads_);
        dynamicNodeLoads_ = other.dynamicNodeLoads_;
        nodeMetricScores_ = move(other.nodeMetricScores_);
        udMetricScores_ = move(other.udMetricScores_);
        fdMetricScores_ = move(other.fdMetricScores_);
        existDefragMetric_ = other.existDefragMetric_;
        existScopedDefragMetric_ = other.existScopedDefragMetric_;
        defragTargetEmptyNodesAchieved_ = other.defragTargetEmptyNodesAchieved_;
        avgStdDev_ = other.avgStdDev_;
        cost_ = other.cost_;
        energy_ = other.energy_;
    }

    return *this;
}

void Score::InitializeDomainAccTree(
    LoadBalancingDomainEntry::DomainAccMinMaxTree const& sourceTree,
    DomainAccTree & targetTree)
{
    if (!sourceTree.IsEmpty)
    {
        targetTree.SetRoot(make_unique<DomainAccTree::Node>(
            DomainAccTree::Node::Create<LoadBalancingDomainEntry::DomainMetricAccsWithMinMax>(
                sourceTree.Root,
                [](LoadBalancingDomainEntry::DomainMetricAccsWithMinMax const& d) -> DomainMetricAccs
                {
                    return DomainMetricAccs(d);
                })));
    }
}

void Score::Calculate(double moveCost)
{
    if (existScopedDefragMetric_)
    {
        defragTargetEmptyNodesAchieved_ = true;
    }

    CalculateAvgStdDev();
    CalculateCost(moveCost);
    CalculateEnergy();
}

void Score::ForEachValidMetric(NodeEntry const* node, std::function<void(size_t, size_t, bool, bool)> processor)
{
    if (node->IsDeactivated || !node->IsUp)
    {
        return;
    }

    size_t totalMetricIndex = 0;
    for (auto itDomain = lbDomainEntries_.begin(); itDomain != lbDomainEntries_.end(); ++itDomain)
    {
        for (auto itMetric = itDomain->Metrics.begin(); itMetric != itDomain->Metrics.end(); ++itMetric)
        {
            if (itMetric->IsValidNode(node->NodeIndex))
            {
                processor(totalMetricIndex, itMetric->IndexInGlobalDomain, itMetric->IsDefrag, itMetric->DefragmentationScopedAlgorithmEnabled);
            }
            ++totalMetricIndex;
        }
    }
}

void Score::UpdateDomainLoads(
    DomainAccTree & domainLoads,
    Common::TreeNodeIndex const& nodeDomainIndex,
    int64 nodeLoadOld,
    int64 nodeLoadNew,
    int64 nodeCapacity,
    size_t totalMetricIndex,
    std::vector<Accumulator> & aggregatedDomainLoads,
    bool updateAggregates)
{
    if (domainLoads.IsEmpty)
    {
        return;
    }

    // applying node change to domain tree and after that, applying domain change to aggregated domain loads.
    double fdLoadOld = domainLoads.GetNodeByIndex(nodeDomainIndex).Data.AccEntries[totalMetricIndex].Sum;
    domainLoads.GetNodeByIndex(nodeDomainIndex).DataRef.AccEntries[totalMetricIndex].AdjustOneValue(nodeLoadOld, nodeLoadNew, nodeCapacity);
    if (updateAggregates)
    {
        double fdLoadNew = domainLoads.GetNodeByIndex(nodeDomainIndex).Data.AccEntries[totalMetricIndex].Sum;
        aggregatedDomainLoads[totalMetricIndex].AdjustOneValue((int64)fdLoadOld, (int64)fdLoadNew, nodeCapacity);
    }
}

void Score::ResetDynamicNodeLoads()
{
    dynamicNodeLoads_->ResetLoads();
}

void Score::UpdateDynamicNodeLoads(DynamicNodeLoadSet* nodeLoadSet, int nodeIndex, int64 newNodeLoad, size_t totalMetricIndex)
{
    nodeLoadSet->UpdateNodeLoad(nodeIndex, newNodeLoad, totalMetricIndex);
}

void Score::UpdateMetricScores(NodeMetrics const& nodeChanges)
{
    DomainAccTree faultDomainTempLoads;
    DomainAccTree upgradeDomainTempLoads;

    if (existDefragMetric_)
    {
        // udMetricScores_ and fdMetricScores_ should be updated with ud/fd total load changes that nodeChanges created (for defrag metrics).
        // we will iterate through node changes and apply them on faultDomainTempLoads and upgradeDomainTempLoads one by one.
        // after every applying in domain tree accumulators, domain change should be applied to udMetricScore_ or fdMetricScore_.
        faultDomainTempLoads = DomainAccTree(faultDomainInitialLoads_);
        upgradeDomainTempLoads = DomainAccTree(upgradeDomainInitialLoads_);
    }

    nodeChanges.ForEach([&](pair<NodeEntry const*, LoadEntry> const& p) -> bool
    {
        LoadEntry const& changes = p.second;
        NodeEntry const* node = p.first;

        ForEachValidMetric(node, [&](size_t totalMetricIndex, size_t globalMetricIndex, bool isDefragMetric, bool isScopedDefragMetric)
        {
            int64 loadLevelOld = node->GetLoadLevel(totalMetricIndex);
            int64 loadLevelNew = node->GetLoadLevel(totalMetricIndex, changes.Values[totalMetricIndex]);

            int64 nodeCapacity = node->GetNodeCapacity(globalMetricIndex);
            nodeMetricScores_[totalMetricIndex].AdjustOneValue(loadLevelOld, loadLevelNew, nodeCapacity);

            if (isDefragMetric)
            {
                UpdateDomainLoads(
                    faultDomainTempLoads,
                    node->FaultDomainIndex,
                    loadLevelOld,
                    loadLevelNew,
                    nodeCapacity,
                    totalMetricIndex,
                    fdMetricScores_);

                UpdateDomainLoads(
                    upgradeDomainTempLoads,
                    node->UpgradeDomainIndex,
                    loadLevelOld,
                    loadLevelNew,
                    nodeCapacity,
                    totalMetricIndex,
                    udMetricScores_);

                if (isScopedDefragMetric)
                {
                    UpdateDynamicNodeLoads(dynamicNodeLoads_, node->NodeIndex, loadLevelNew, totalMetricIndex);
                }
            }
        });

        return true;
    });
}

void Score::UpdateMetricScores(NodeMetrics const& newNodeChanges, NodeMetrics const& oldNodeChanges)
{
    DomainAccTree faultDomainTempLoads;
    DomainAccTree upgradeDomainTempLoads;

    if (existDefragMetric_)
    {
        // udMetricScores_ and fdMetricScores_ should be updated with ud/fd total load changes that nodeChanges created (for defrag metrics).
        // we will iterate through node changes and apply them on faultDomainTempLoads and upgradeDomainTempLoads one by one.
        // after applying in domain tree accumulators, domain change should be applied to udMetricScore_ or fdMetricScore_.
        faultDomainTempLoads = DomainAccTree(faultDomainInitialLoads_);
        upgradeDomainTempLoads = DomainAccTree(upgradeDomainInitialLoads_);
        oldNodeChanges.ForEach([&](pair<NodeEntry const*, LoadEntry> const& p) -> bool
        {
            LoadEntry const& oldChanges = p.second;
            NodeEntry const* node = p.first;

            ForEachValidMetric(node, [&](size_t totalMetricIndex, size_t globalMetricIndex, bool isDefragMetric, bool isScopedDefragMetric)
            {
                UNREFERENCED_PARAMETER(isScopedDefragMetric);

                int64 loadLevelOld = node->GetLoadLevel(totalMetricIndex);
                int64 loadLevelNew = node->GetLoadLevel(totalMetricIndex, oldChanges.Values[totalMetricIndex]);
                int64 nodeCapacity = node->GetNodeCapacity(globalMetricIndex);

                if (isDefragMetric)
                {
                    UpdateDomainLoads(
                        faultDomainTempLoads,
                        node->FaultDomainIndex,
                        loadLevelOld,
                        loadLevelNew,
                        nodeCapacity,
                        totalMetricIndex,
                        fdMetricScores_,
                        false);

                    UpdateDomainLoads(
                        upgradeDomainTempLoads,
                        node->UpgradeDomainIndex,
                        loadLevelOld,
                        loadLevelNew,
                        nodeCapacity,
                        totalMetricIndex,
                        udMetricScores_,
                        false);
                }
            });

            return true;
        });
    }

    newNodeChanges.ForEach([&](pair<NodeEntry const*, LoadEntry> const& p) -> bool
    {
        LoadEntry const& newChanges = p.second;
        NodeEntry const* node = p.first;

        LoadEntry const& oldChanges = oldNodeChanges[node];

        ForEachValidMetric(node, [&](size_t totalMetricIndex, size_t globalMetricIndex, bool isDefragMetric, bool isScopedDefragMetric)
        {
            int64 loadLevelOld = oldChanges.Values.empty() ?
                node->GetLoadLevel(totalMetricIndex) : node->GetLoadLevel(totalMetricIndex, oldChanges.Values[totalMetricIndex]);
            int64 loadLevelNew = node->GetLoadLevel(totalMetricIndex, newChanges.Values[totalMetricIndex]);
            int64 nodeCapacity = node->GetNodeCapacity(globalMetricIndex);
            nodeMetricScores_[totalMetricIndex].AdjustOneValue(loadLevelOld, loadLevelNew, nodeCapacity);

            if (isDefragMetric)
            {
                UpdateDomainLoads(
                    faultDomainTempLoads,
                    node->FaultDomainIndex,
                    loadLevelOld,
                    loadLevelNew,
                    nodeCapacity,
                    totalMetricIndex,
                    fdMetricScores_);

                UpdateDomainLoads(
                    upgradeDomainTempLoads,
                    node->UpgradeDomainIndex,
                    loadLevelOld,
                    loadLevelNew,
                    nodeCapacity,
                    totalMetricIndex,
                    udMetricScores_);

                if (isScopedDefragMetric)
                {
                    UpdateDynamicNodeLoads(dynamicNodeLoads_, node->NodeIndex, loadLevelNew, totalMetricIndex);
                }
            }
        });

        return true;
    });
}

void Score::UndoMetricScoresForDynamicLoads(NodeMetrics const& newNodeChanges, NodeMetrics const& oldNodeChanges)
{
    if (!existScopedDefragMetric_ || !existDefragMetric_)
    {
        return;
    }

    newNodeChanges.ForEach([&](pair<NodeEntry const*, LoadEntry> const& p) -> bool
    {
        NodeEntry const* node = p.first;
        LoadEntry const& oldChanges = oldNodeChanges[node];

        ForEachValidMetric(node, [&](size_t totalMetricIndex, size_t globalMetricIndex, bool isDefragMetric, bool isScopedDefragMetric)
        {
            UNREFERENCED_PARAMETER(globalMetricIndex);
            if (isDefragMetric && isScopedDefragMetric)
            {
                // Restore old dynamic load state
                int64 loadLevelOld = node->GetLoadLevel(totalMetricIndex, oldChanges.Values[totalMetricIndex]);
                UpdateDynamicNodeLoads(dynamicNodeLoads_, node->NodeIndex, loadLevelOld, totalMetricIndex);
            }
        });

        return true;
    });
}

double Score::StdDevCaculationHelper(size_t metricScoreIndex,
    bool isDefrag,
    bool isScopedDefrag,
    Metric::PlacementStrategy const& placementStrategy,
    int32 emptyNodesTarget,
    double defragEmptyNodeWeight,
    double defragNonEmptyNodeWeight,
    Metric::DefragDistributionType const& defragDistributionType,
    size_t reservationLoad,
    double weight)
{
    if (isDefrag)
    {
        if (!isScopedDefrag)
        {
            return weight * (
                settings_.DefragmentationNodesStdDevFactor -
                settings_.DefragmentationNodesStdDevFactor * nodeMetricScores_[metricScoreIndex].NormStdDev +
                settings_.DefragmentationFdsStdDevFactor * fdMetricScores_[metricScoreIndex].NormStdDev +
                settings_.DefragmentationUdsStdDevFactor * udMetricScores_[metricScoreIndex].NormStdDev);
        }
        else
        {
            // When using the new defrag algorithm, we reward the emptiness of the nodes as well as the balance of the cluster
            // The weight of these in the calculation is controlled with defragEmptyNodeWeight(emptiness) and nonEmptyNodeWeight(balance)
            if (placementStrategy == Metric::PlacementStrategy::ReservationAndBalance)
            {
                return weight * (
                    defragNonEmptyNodeWeight *
                    (
                        settings_.DefragmentationNodesStdDevFactor * nodeMetricScores_[metricScoreIndex].NormStdDev
                        )
                    +
                    defragEmptyNodeWeight *
                    CalculateFreeNodeScoreHelper(metricScoreIndex, emptyNodesTarget, defragDistributionType, reservationLoad)
                    );
            }
            else if (placementStrategy == Metric::PlacementStrategy::ReservationAndPack)
            {
                return weight * (
                    defragNonEmptyNodeWeight *
                    (
                        settings_.DefragmentationNodesStdDevFactor -
                        settings_.DefragmentationNodesStdDevFactor * nodeMetricScores_[metricScoreIndex].NormStdDev +
                        settings_.DefragmentationFdsStdDevFactor * fdMetricScores_[metricScoreIndex].NormStdDev +
                        settings_.DefragmentationUdsStdDevFactor * udMetricScores_[metricScoreIndex].NormStdDev
                        )
                    +
                    defragEmptyNodeWeight *
                    CalculateFreeNodeScoreHelper(metricScoreIndex, emptyNodesTarget, defragDistributionType, reservationLoad)
                    );
            }
            // Placement strategy is NoPreference
            else
            {
                return weight * defragEmptyNodeWeight *
                    CalculateFreeNodeScoreHelper(metricScoreIndex, emptyNodesTarget, defragDistributionType, reservationLoad);
            }
        }
    }
    else
    {
        return weight * nodeMetricScores_[metricScoreIndex].NormStdDev;
    }
}

double Score::CalculateAvgStdDevForMetric(wstring const & metricName)
{
    LoadBalancingDomainEntry const& lbDomain = lbDomainEntries_.back();
    size_t globalMetricCount = lbDomain.MetricCount;
    size_t globalMetricStartIndex = totalMetricCount_ - globalMetricCount;
    double metricAvgStdDev = 0.0;

    for (size_t j = 0; j < globalMetricCount; j++)
    {
        if (lbDomain.Metrics[j].Name == metricName)
        {
            auto& metric = lbDomain.Metrics[j];

            metricAvgStdDev = StdDevCaculationHelper(globalMetricStartIndex + j,
                metric.IsDefrag,
                metric.DefragmentationScopedAlgorithmEnabled,
                metric.placementStrategy,
                metric.DefragNodeCount,
                metric.DefragmentationEmptyNodeWeight,
                metric.DefragmentationNonEmptyNodeWeight,
                metric.DefragDistribution,
                metric.ReservationLoad,
                metric.Weight);

            break;
        }
    }

    return metricAvgStdDev;
}

void Score::CalculateAvgStdDev()
{
    size_t lbDomainCount = lbDomainEntries_.size();

    double avgStdDev = 0.0;
    size_t currentIndex = 0;

    //the last LB Domain is a global domain...
    size_t actualLocalDomainCount = lbDomainCount - 1;
    for (size_t i = 0; i < lbDomainCount; i++)
    {
        LoadBalancingDomainEntry const& lbDomain = lbDomainEntries_[i];
        double weightSum = lbDomain.MetricWeightSum;
        size_t metricCount = lbDomain.MetricCount;

        double lbDomainScore = 0.0;
        for (size_t j = 0; j < metricCount; j++)
        {
            auto& metric = lbDomain.Metrics[j];

            lbDomainScore += StdDevCaculationHelper(currentIndex,
                metric.IsDefrag,
                metric.DefragmentationScopedAlgorithmEnabled,
                metric.placementStrategy,
                metric.DefragNodeCount,
                metric.DefragmentationEmptyNodeWeight,
                metric.DefragmentationNonEmptyNodeWeight,
                metric.DefragDistribution,
                metric.ReservationLoad,
                metric.Weight);

            currentIndex++;
        }

        if (weightSum > 1e-7)
        {
            lbDomainScore /= weightSum;
        }
        else
        {
            lbDomainScore = 0;
        }

        if (i < lbDomainCount - 1)
        {
            avgStdDev += lbDomainScore;
        }
        else
        {
            // This is the last LB domain, and it is global
            // We'll divide the sum of local standard deviations with their count, and will add global score
            avgStdDev = avgStdDev / max(size_t(1), actualLocalDomainCount);
            double localDomainWeight = settings_.LocalDomainWeight;
            avgStdDev = localDomainWeight * avgStdDev + (1 - localDomainWeight) * lbDomainScore;
        }
    }

    avgStdDev_ = avgStdDev;
}

void Score::CalculateCost(double moveCost)
{
    // TempSolution and candidate solution will keep track of the cost.
    cost_ = moveCost;
}

void Score::CalculateEnergy()
{
    if (totalReplicaCount_ == 0 || settings_.IgnoreCostInScoring)
    {
        energy_ = avgStdDev_;
    }
    else
    {
        int signOfStdDev = (avgStdDev_ + 0.001) >= 0 ? 1 : -1;        
        double costFactor = (signOfStdDev * cost_ + totalReplicaCount_ + settings_.MoveCostOffset);
        double stdDev = (avgStdDev_ + 0.001);
        energy_ =  stdDev * costFactor;
    }
}

double Score::CalculateFreeNodeScoreHelper(size_t metricScoreIndex,
    int32 emptyNodesTarget,
    Metric::DefragDistributionType const& defragDistributionType,
    size_t reservationLoad)
{
    // We reward emptyNodesTarget for value up to 1
    // We additionaly reward for 1 when a node is completely empty to enforce empty nodes
    // Here we initialize with the worst case and improve from there
    double score = emptyNodesTarget * 2;

    dynamicNodeLoads_->ForEachCandidateNode(metricScoreIndex, emptyNodesTarget, defragDistributionType, reservationLoad, [&](double percentage)
    {
        score -= pow(1 - percentage, 2);

        if (percentage < 1e-7)
        {
            score -= 1;
        }
    });

    if (score > 1e-7)
    {
        defragTargetEmptyNodesAchieved_ = false;
    }

    return score;
}
