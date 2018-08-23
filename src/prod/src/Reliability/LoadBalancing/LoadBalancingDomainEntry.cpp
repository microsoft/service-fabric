// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadBalancingDomainEntry.h"
#include "PLBConfig.h"
#include "PlacementAndLoadBalancing.h"
#include "DiagnosticsDataStructures.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadBalancingDomainEntry::LoadBalancingDomainEntry(std::vector<Metric> && metrics, double metricWeightSum, size_t metricStartIndex, int serviceIndex)
    : metrics_(move(metrics)),
    metricWeightSum_(metricWeightSum),
    metricStartIndex_(metricStartIndex),
    serviceIndex_(serviceIndex),
    fdLoadStats_(MetricCount),
    udLoadStats_(MetricCount),
    isBalanced_(false)
{
    for (size_t i = 0; i < MetricCount; i++)
    {
        Metric & metric = metrics_[i];
        if (metric.BalancingByPercentage)
        {
            loadStats_.push_back(AccumulatorWithMinMax(true));
        }
        else
        {
            loadStats_.push_back(AccumulatorWithMinMax(false));
        }
    }
}

LoadBalancingDomainEntry::LoadBalancingDomainEntry(LoadBalancingDomainEntry && other)
    : metrics_(move(other.metrics_)),
    metricWeightSum_(other.metricWeightSum_),
    metricStartIndex_(other.metricStartIndex_),
    serviceIndex_(other.serviceIndex_),
    loadStats_(move(other.loadStats_)),
    fdLoadStats_(move(other.fdLoadStats_)),
    udLoadStats_(move(other.udLoadStats_)),
    isBalanced_(other.isBalanced_)
{
}

LoadBalancingDomainEntry & LoadBalancingDomainEntry::operator = (LoadBalancingDomainEntry && other)
{
    if (this != &other)
    {
        metrics_ = move(other.metrics_);
        metricWeightSum_ = other.metricWeightSum_;
        metricStartIndex_ = other.metricStartIndex_,
            serviceIndex_ = other.serviceIndex_;
        loadStats_ = move(other.loadStats_);
        fdLoadStats_ = move(other.fdLoadStats_);
        udLoadStats_ = move(other.udLoadStats_);
        isBalanced_ = other.isBalanced_;
    }

    return *this;
}

void LoadBalancingDomainEntry::RefreshIsBalanced(
    vector<size_t> const* globalMetricIndices,
    LoadBalancingDomainEntry const& globalLBDomain,
    DomainAccMinMaxTree const& fdsTotalLoads,
    DomainAccMinMaxTree const& udsTotalLoads,
    DynamicNodeLoadSet& dynamicLoads,
    BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr /* = nullptr */)
{
    isBalanced_ = true;
    size_t metricCount = MetricCount;

    ASSERT_IFNOT(metricCount == loadStats_.size(),
        "Metric count {0} should be equal to score count {1}", metricCount, loadStats_.size());

    for (size_t i = 0; i < metricCount; i++)
    {
        Metric & metric = metrics_[i];
        metric.IsBalanced = true;

        // always check against the max load of the corresponding global metric
        size_t globalMetricIndex = globalMetricIndices == nullptr ? i : (*globalMetricIndices)[i] - globalLBDomain.MetricStartIndex;
        TESTASSERT_IFNOT(globalMetricIndex < globalLBDomain.MetricCount,
            "Global metric index {0} should be less than global metric count {1}", globalMetricIndex, globalLBDomain.MetricCount);

        size_t totalMetricIndex = metricStartIndex_ + i;

        metric.IndexInLocalDomain = i;
        metric.IndexInGlobalDomain = globalMetricIndex;
        metric.IndexInTotalDomain = totalMetricIndex;

        if (metric.Weight <= 0.0 || metric.BalancingThreshold <= 0.0)
        {
            continue;
        }

        MetricBalancingDiagnosticInfo metricDiagnostics;

        if (metric.IsDefrag)
        {

            //Diagnostics
            if (balancingDiagnosticsDataSPtr != nullptr)
            {
                metricDiagnostics.isDefrag_ = true;
            }

            AccumulatorWithMinMax const& fdLoadStat = fdLoadStats_[i];
            AccumulatorWithMinMax const& udLoadStat = udLoadStats_[i];

            if (fdLoadStat.IsEmpty && udLoadStat.IsEmpty)
            {
                continue;
            }

            
            // defragmentation should work if maxNodeLoad/minNodeLoad in at least one FD or UD is smaller than MetricBalancingThresholds
            bool shouldDefrag = false;

            if (metric.DefragNodeCount == -1)
            {
                // Every FDs/UDs need to have satisfied defragmentation rule which is maxNodeLoad/minNodeLoad > MetricBalancingThreshold
                shouldDefrag = ShouldDefragRunByDomain(fdsTotalLoads, udsTotalLoads, totalMetricIndex, metric);
            }
            else if (metric.DefragNodeCount > 0 && (metric.DefragmentationEmptyNodeWeight > 0 || !metric.DefragmentationScopedAlgorithmEnabled))
            {
                switch (metric.DefragDistribution)
                {
                case Metric::DefragDistributionType::SpreadAcrossFDs_UDs:
                    // Spread empty nodes accross fds/uds
                    shouldDefrag = ShouldDefragRunByNodeSpreadDomain(fdsTotalLoads, udsTotalLoads, totalMetricIndex, metric);
                    break;

                case Metric::DefragDistributionType::NumberOfEmptyNodes:
                    // Don't care about distribution, just check do we have defined number of empty nodes
                    shouldDefrag = ShouldDefragRunByNumberOfEmptyNodes(fdsTotalLoads, totalMetricIndex, metric);
                    break;
                }

                // Even if there are enough empty nodes for the current defrag metric, check do they overlap with other defrag metrics
                if (dynamicLoads.Overlapping && !shouldDefrag && globalMetricIndices == nullptr)
                {
                    if (!dynamicLoads.IsEnoughLoadReserved(totalMetricIndex, metric.DefragNodeCount, metric.DefragDistribution, metric.ReservationLoad))
                    {
                        shouldDefrag = true;
                    }
                }
            }

            if (!shouldDefrag)
            {
                // If DefragmentationNonEmptyNodeWeight is set to 0, then balancing/defragmentation will never be triggered
                // since no movements are allowed if there are required number of empty nodes 
                if (!metric.DefragmentationScopedAlgorithmEnabled || metric.DefragmentationNonEmptyNodeWeight <= 0)
                {
                    continue;
                }

                AccumulatorWithMinMax const& loadStat = loadStats_[i];

                if (loadStat.IsEmpty)
                {
                    continue;
                }

                // (we assume all capacity ratios for each metric are the same)
                if (loadStat.NonBeneficialMax <= loadStat.NonBeneficialMin + 1)
                {
                    continue;
                }

                if (metric.ActivityThreshold > 0)
                {
                    AccumulatorWithMinMax const& globalMetricLoadStat = globalLBDomain.GetLoadStat(globalMetricIndex);
                    if (globalMetricLoadStat.NonBeneficialMax <= metric.ActivityThreshold)
                    {
                        continue;
                    }
                }

                // For strategies Reservation with packing and Reservation balancing phase will never be triggered
                // Only if one of the beneficial nodes is occupied then the moves will be made
                if (metric.placementStrategy == Metric::PlacementStrategy::Reservation || metric.placementStrategy == Metric::PlacementStrategy::ReservationAndPack)
                {
                    continue;
                }
                else if (metric.placementStrategy == Metric::PlacementStrategy::ReservationAndBalance && metric.BalancingThreshold > 1.0 && loadStat.NonBeneficialMin > 0 &&
                    static_cast<double>(loadStat.NonBeneficialMax) / loadStat.NonBeneficialMin <= metric.BalancingThreshold)
                {
                    continue;
                }
            }

            //Diagnostics
            if (balancingDiagnosticsDataSPtr != nullptr)
            {
                metricDiagnostics.needsDefrag_ = true;
            }

        }
        else
        {
            //Diagnostics
            if (balancingDiagnosticsDataSPtr != nullptr)
            {
                metricDiagnostics.needsDefrag_ = false;
                metricDiagnostics.metricActivityThreshold_ = 0;
            }

            AccumulatorWithMinMax const& loadStat = loadStats_[i];

            if (loadStat.IsEmpty)
            {
                continue;
            }

            // (we assume all capacity ratios for each metric are the same)
            if (loadStat.Max <= loadStat.Min + 1)
            {
                continue;
            }

            if (metric.ActivityThreshold > 0)
            {
                AccumulatorWithMinMax const& globalMetricLoadStat = globalLBDomain.GetLoadStat(globalMetricIndex);
                if (globalMetricLoadStat.Max <= metric.ActivityThreshold)
                {
                    continue;
                }

                //Diagnostics
                if (balancingDiagnosticsDataSPtr != nullptr)
                {
                    metricDiagnostics.metricActivityThreshold_ = metric.ActivityThreshold;
                }
            }

            if (metric.BalancingThreshold > 1.0 && loadStat.Min > 0 && static_cast<double>(loadStat.Max) / loadStat.Min <= metric.BalancingThreshold)
            {
                continue;
            }

            //Diagnostics
            if (balancingDiagnosticsDataSPtr != nullptr)
            {
                metricDiagnostics.minLoad_ = loadStat.Min;
                metricDiagnostics.maxLoad_ = loadStat.Max;
                metricDiagnostics.minNode_ = loadStat.MinNode;
                metricDiagnostics.maxNode_ = loadStat.MaxNode;
                if (balancingDiagnosticsDataSPtr->plbNodesPtr_ != nullptr)
                {
                    // To get the node name from the array of nodes we need to map from node ID to node index, and to check that tables are not nullptr.
                    uint64 minNodeIndex = (balancingDiagnosticsDataSPtr->plbNodeToIndexMap_ == nullptr
                        || balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->find(loadStat.MinNode) == balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->end())
                        ? UINT64_MAX
                        : balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->find(loadStat.MinNode)->second;
                    uint64 maxNodeIndex = (balancingDiagnosticsDataSPtr->plbNodeToIndexMap_ == nullptr
                        || balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->find(loadStat.MaxNode) == balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->end())
                        ? UINT64_MAX
                        : balancingDiagnosticsDataSPtr->plbNodeToIndexMap_->find(loadStat.MaxNode)->second;
                    metricDiagnostics.minNodeName_ = (minNodeIndex != UINT64_MAX) ? (*(balancingDiagnosticsDataSPtr->plbNodesPtr_)).at(minNodeIndex).NodeDescriptionObj.NodeName : L"";
                    metricDiagnostics.maxNodeName_ = (maxNodeIndex != UINT64_MAX) ? (*(balancingDiagnosticsDataSPtr->plbNodesPtr_)).at(maxNodeIndex).NodeDescriptionObj.NodeName : L"";
                }
                else
                {
                    metricDiagnostics.minNodeName_ = L"";
                    metricDiagnostics.maxNodeName_ = L"";
                }
            }
        }

        metric.IsBalanced = false;
        isBalanced_ = false;

        //Diagnostics
        if (balancingDiagnosticsDataSPtr != nullptr)
        {
            metricDiagnostics.isBalanced_ = metric.IsBalanced;
            metricDiagnostics.metricBalancingThreshold_ = metric.BalancingThreshold;
            metricDiagnostics.metricName_ = metric.Name;
            metricDiagnostics.globalOrServiceIndex_ = ServiceIndex;

            if (ServiceIndex != -1)
            {
                metricDiagnostics.serviceName_ = (*(balancingDiagnosticsDataSPtr->servicesListPtr_))[ServiceIndex]->ServiceDesc.Name;
            }

            balancingDiagnosticsDataSPtr->changed_ = true;
            balancingDiagnosticsDataSPtr->metricDiagnostics.push_back(move(metricDiagnostics));

        }
    }
}

GlobalWString const LoadBalancingDomainEntry::TraceDescription = make_global<wstring>(L"[LBDomains]\r\n#serviceIndex metrics scores isBalanced");

void LoadBalancingDomainEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    ASSERT_IFNOT(metrics_.size() == loadStats_.size(), "Metric size {0} doesn't equal to loadLevel size {1}", metrics_.size(), loadStats_.size());
    writer.Write("{0} {1} (", serviceIndex_, metrics_);
    for (auto it = loadStats_.begin(); it != loadStats_.end(); ++it)
    {
        if (it != loadStats_.begin())
        {
            writer.Write(" ");
        }

        if (it->IsEmpty)
        {
            writer.Write("N/A");
        }
        else
        {
            writer.Write("{0}/{1}/{2}", it->Min, it->Max, it->NormStdDev);
        }
    }

    writer.Write(") {0}", isBalanced_);
}

void LoadBalancingDomainEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

bool LoadBalancingDomainEntry::ShouldDefragRunByDomain(
    DomainAccMinMaxTree const & fdAccumulator,
    DomainAccMinMaxTree const & udAccumulator,
    size_t totalMetricIndex,
    Metric const & metric) const
{
    bool shouldDefrag = false;
    if (!fdAccumulator.IsEmpty)
    {
        shouldDefrag = ShouldDefragRunByDomain(fdAccumulator, totalMetricIndex, metric);
    }

    if (!shouldDefrag && !udAccumulator.IsEmpty)
    {
        shouldDefrag = ShouldDefragRunByDomain(udAccumulator, totalMetricIndex, metric);
    }

    return shouldDefrag;
}

bool LoadBalancingDomainEntry::ShouldDefragRunByDomain(
    DomainAccMinMaxTree const & accumulator,
    size_t totalMetricIndex,
    Metric const & metric) const
{
    bool shouldDefrag = false;
    if (metric.BalancingThreshold == 1)
    {
        shouldDefrag = true;
    }
    else
    {
        if (!accumulator.IsEmpty)
        {
            accumulator.ForEachNodeInPreOrder([&](DomainAccMinMaxTree::Node const & node)
            {
                if (node.Children.size() == 0)
                {
                    AccumulatorWithMinMax const& loadStat = node.Data.AccMinMaxEntries[totalMetricIndex];

                    if (loadStat.IsValid && loadStat.Min != 0 && loadStat.Count > 1)
                    {
                        double ratio = static_cast<double>(loadStat.Max) / loadStat.Min;
                        shouldDefrag |= ratio <= metric.BalancingThreshold;
                    }
                }
            });
        }
    }

    return shouldDefrag;
}

bool LoadBalancingDomainEntry::ShouldDefragRunByNodeSpreadDomain(
    DomainAccMinMaxTree const & fdAccumulator,
    DomainAccMinMaxTree const & udAccumulator,
    size_t totalMetricIndex,
    Metric const & metric) const
{
    bool shouldDefrag = false;
    if (!fdAccumulator.IsEmpty)
    {
        shouldDefrag = ShouldDefragRunByNodeSpreadDomain(fdAccumulator, totalMetricIndex, metric);
    }

    if (!shouldDefrag && !udAccumulator.IsEmpty)
    {
        shouldDefrag = ShouldDefragRunByNodeSpreadDomain(udAccumulator, totalMetricIndex, metric);
    }

    return shouldDefrag;
}

bool LoadBalancingDomainEntry::ShouldDefragRunByNodeSpreadDomain(
    DomainAccMinMaxTree const & accumulator,
    size_t totalMetricIndex,
    Metric const & metric) const
{
    bool shouldDefrag = false;
    if (!accumulator.IsEmpty)
    {
        shouldDefrag = true;
        size_t numberOfDomains = accumulator.LeavesCount;

        size_t nodesPerDomain = (size_t)metric.DefragNodeCount / numberOfDomains;

        size_t domainsWithNodesPlusOneCount = (size_t)metric.DefragNodeCount % numberOfDomains;
        size_t domainsWithNodesCount = numberOfDomains - domainsWithNodesPlusOneCount;

        accumulator.ForEachNodeInPreOrder([&](DomainAccMinMaxTree::Node const & node)
        {
            if (node.Children.size() == 0)
            {
                size_t emptyNodes = node.Data.AccMinMaxEntries[totalMetricIndex].EmptyNodeCount;
                if (domainsWithNodesPlusOneCount > 0 && emptyNodes >= nodesPerDomain + 1)
                {
                    domainsWithNodesPlusOneCount--;
                }
                else if (domainsWithNodesCount > 0 && emptyNodes >= nodesPerDomain)
                {
                    domainsWithNodesCount--;
                }
            }
        });

        if (domainsWithNodesPlusOneCount == 0 && domainsWithNodesCount == 0)
        {
            shouldDefrag = false;
        }
    }

    return shouldDefrag;
}

bool LoadBalancingDomainEntry::ShouldDefragRunByNumberOfEmptyNodes(
    DomainAccMinMaxTree const & accumulator,
    size_t totalMetricIndex,
    Metric const & metric) const
{
    bool shouldDefrag = false;

    if (!accumulator.IsEmpty)
    {
        shouldDefrag = true;
        size_t emptyNodeCount = 0;

        accumulator.ForEachNodeInPreOrder([&](DomainAccMinMaxTree::Node const & node)
        {
            if (node.Children.size() == 0)
            {
                AccumulatorWithMinMax const& acc = node.Data.AccMinMaxEntries[totalMetricIndex];

                emptyNodeCount += acc.EmptyNodeCount;
            }
        });

        if (emptyNodeCount >= (size_t)metric.DefragNodeCount)
        {
            shouldDefrag = false;
        }
    }

    return shouldDefrag;
}
