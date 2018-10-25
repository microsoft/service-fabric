// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DynamicNodeLoadSet.h"
#include "LoadBalancingDomainEntry.h"
#include "LoadEntry.h"
#include "NodeEntry.h"
#include "SearcherSettings.h"


using namespace std;
using namespace Reliability::LoadBalancingComponent;
using namespace Common;

const size_t DynamicNodeLoadSet::OverlappingIndex = SIZE_MAX - 1;

DynamicNodeLoadSet::DynamicNodeLoad::DynamicNodeLoad(
    LoadEntry const* load,
    LoadEntry const* capacity,
    Common::TreeNodeIndex const* fdIndex,
    Common::TreeNodeIndex const* udIndex,
    bool isUp,
    int nodeIndex,
    double& overlappingLoad,
    int64& moveCost)
    : isUp_(isUp),
    nodeIndex_(nodeIndex),
    load_(load),
    capacity_(capacity),
    fdIndex_(fdIndex),
    udIndex_(udIndex),
    overlappingLoad_(overlappingLoad),
    moveCost_(moveCost)
{
}

DynamicNodeLoadSet::DynamicNodeLoad::DynamicNodeLoad(DynamicNodeLoad const& other)
    : isUp_(other.isUp_),
    nodeIndex_(other.nodeIndex_),
    load_(other.load_),
    capacity_(other.capacity_),
    fdIndex_(other.fdIndex_),
    udIndex_(other.udIndex_),
    overlappingLoad_(other.overlappingLoad_),
    moveCost_(other.moveCost_)
{
}

DynamicNodeLoadSet::DynamicNodeLoad::DynamicNodeLoad(DynamicNodeLoad && other)
    : isUp_(other.isUp_),
    nodeIndex_(other.nodeIndex_),
    load_(other.load_),
    capacity_(other.capacity_),
    fdIndex_(other.fdIndex_),
    udIndex_(other.udIndex_),
    overlappingLoad_(other.overlappingLoad_),
    moveCost_(other.moveCost_)
{
}

double DynamicNodeLoadSet::DynamicNodeLoad::Load(size_t totalMetricIndex, size_t globalMetricIndex) const
{
    //Utilized load for move cost on node doesn't exist because there is no movecost capacity
    if (totalMetricIndex == MoveCostIndex)
    {
        return static_cast<double>(moveCost_);
    }
    //Max load for overlapping is 2*(number of scoped defrag metrics)
    if (totalMetricIndex == OverlappingIndex)
    {
        return overlappingLoad_;
    }

    auto nodeCapacity = capacity_->Values[globalMetricIndex];

    if (nodeCapacity <= 0)
    {
        return 1;
    }

    return load_->Values[totalMetricIndex] * 1.0f / nodeCapacity;
}

double DynamicNodeLoadSet::DynamicNodeLoad::Load(size_t totalMetricIndex, size_t globalMetricIndex, size_t reservationLoad) const
{
    if (totalMetricIndex == MoveCostIndex)
    {
        return static_cast<double>(moveCost_);
    }

    if (totalMetricIndex == OverlappingIndex)
    {
        return overlappingLoad_;
    }

    auto nodeCapacity = capacity_->Values[globalMetricIndex];

    // If node doesn't have capacity or there isn't enough capacity, consider it as full (non beneficial for emptying for this metric)
    if (nodeCapacity <= 0 || (size_t)nodeCapacity < reservationLoad)
    {
        return 1;
    }

    auto load = max((int64)(load_->Values[totalMetricIndex] + reservationLoad - nodeCapacity), (int64)0);

    return load * 1.0f / nodeCapacity;
}

DynamicNodeLoadSet::NodeLoadComparator::NodeLoadComparator(size_t totalMetricIndex, size_t globalMetricIndex, size_t reservationLoad)
    : totalMetricIndex_(totalMetricIndex),
    globalMetricIndex_(globalMetricIndex),
    reservationAmount_(reservationLoad)
{
}
bool DynamicNodeLoadSet::NodeLoadComparator::operator()(DynamicNodeLoad const* a, DynamicNodeLoad const* b) const
{
    if (a->IsUp && !b->IsUp)
    {
        return true;
    }

    if (b->IsUp && !a->IsUp)
    {
        return false;
    }

    if (a->Load(totalMetricIndex_, globalMetricIndex_, reservationAmount_) < b->Load(totalMetricIndex_, globalMetricIndex_, reservationAmount_))
    {
        return true;
    }

    if (a->Load(totalMetricIndex_, globalMetricIndex_, reservationAmount_) > b->Load(totalMetricIndex_, globalMetricIndex_, reservationAmount_))
    {
        return false;
    }

    return a->NodeIndex < b->NodeIndex;
}
DynamicNodeLoadSet::DynamicNodeLoadSet()
{
}

DynamicNodeLoadSet::DynamicNodeLoadSet(vector<NodeEntry> const& nodeEntries, vector<LoadBalancingDomainEntry> const& lbDomainEntries, bool overlapping)
    : nodeCount_(nodeEntries.size()),
    version_(0),
    nodeLoads_(),
    nodeLoadBackup_(),
    nodeCapacities_(),
    fdIndexes_(),
    udIndexes_(),
    nodeUp_(),
    nodeLoadIndexes_(),
    overlappingLoads_(),
    moveCosts_(),
    lastBeneficialVersions_(),
    fdDistributionHelper_(),
    udDistributionHelper_(),
    reservedLoads_(),
    allDynamicNodeLoads_(),
    overlapping_(overlapping)
{
    allDynamicNodeLoads_.reserve(nodeCount_);
    nodeLoads_.reserve(nodeCount_);
    nodeLoadBackup_.reserve(nodeCount_);
    nodeCapacities_.reserve(nodeCount_);
    fdIndexes_.reserve(nodeCount_);
    udIndexes_.reserve(nodeCount_);
    overlappingLoads_.reserve(nodeCount_);
    moveCosts_.reserve(nodeCount_);
    nodeUp_.reserve(nodeCount_);

    for (int index = 0; index < nodeEntries.size(); ++index)
    {
        nodeLoads_.push_back(move(LoadEntry(nodeEntries[index].Loads)));
        nodeLoadBackup_.push_back(move(LoadEntry(nodeEntries[index].Loads)));
        nodeCapacities_.push_back(move(LoadEntry(nodeEntries[index].TotalCapacities)));
        fdIndexes_.push_back(move(TreeNodeIndex(nodeEntries[index].FaultDomainIndex)));
        udIndexes_.push_back(move(TreeNodeIndex(nodeEntries[index].UpgradeDomainIndex)));
        overlappingLoads_.push_back(0);
        moveCosts_.push_back(0);
        nodeUp_.push_back((nodeEntries[index].IsUp && !nodeEntries[index].IsDeactivated) ? true : false);

        DynamicNodeLoad dynamicLoad = DynamicNodeLoad(&(nodeLoads_[index]),
            &(nodeCapacities_[index]),
            &(fdIndexes_[index]),
            &(udIndexes_[index]),
            nodeUp_[index],
            index,
            (overlappingLoads_[index]),
            (moveCosts_[index]));

        allDynamicNodeLoads_.push_back(dynamicLoad);
    }

    auto& globalLBDomainEntry = lbDomainEntries.back();
    std::map<std::wstring, size_t> metricNameToGlobalIndex;
    for (size_t i = 0; i < globalLBDomainEntry.MetricCount; i++)
    {
        metricNameToGlobalIndex.insert(
            make_pair(globalLBDomainEntry.Metrics[i].Name,
                i + globalLBDomainEntry.MetricStartIndex));
    }

    auto prepareSetForMetric = [&](size_t totalMetricIndex, size_t globalMetricIndex, bool realMetric, size_t reservationLoad)
    {
        NodeLoadComparator cmp(totalMetricIndex, globalMetricIndex, reservationLoad);
        std::set<DynamicNodeLoad*, NodeLoadComparator> nodeLoadIndex(cmp);
        std::vector<int> lastBeneficialVersion;
        std::map<TreeNodeIndex, int> fdDistribution;
        std::map<TreeNodeIndex, int> udDistribution;
        auto threshold = make_pair(globalMetricIndex, reservationLoad);

        for (int index = 0; index < nodeEntries.size(); ++index)
        {
            if (realMetric)
            {
                CalculateOverlappingLoadOnNode(index, allDynamicNodeLoads_[index].Load(totalMetricIndex, globalMetricIndex, reservationLoad), 0);
            }
            nodeLoadIndex.insert(&(allDynamicNodeLoads_[index]));
            lastBeneficialVersion.push_back(-1);
        }

        nodeLoadIndexes_.insert(make_pair(totalMetricIndex, nodeLoadIndex));
        lastBeneficialVersions_.insert(make_pair(totalMetricIndex, lastBeneficialVersion));
        fdDistributionHelper_.insert(make_pair(totalMetricIndex, fdDistribution));
        udDistributionHelper_.insert(make_pair(totalMetricIndex, udDistribution));
        reservedLoads_.insert(make_pair(totalMetricIndex, threshold));
    };

    size_t totalMetricIndex = 0;
    size_t domainIndex = 0;
    for (auto it = lbDomainEntries.begin(); it != lbDomainEntries.end(); ++it, ++domainIndex)
    {
        for (size_t i = 0; i < it->MetricCount; ++i, ++totalMetricIndex)
        {
            auto& metric = it->Metrics[i];
            if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled)
            {
                auto mappingIt = metricNameToGlobalIndex.find(metric.Name);
                ASSERT_IF(mappingIt == metricNameToGlobalIndex.end(),
                    "Metric {0} does not exist in metricNameToGlobalIndex for lbDomain {1}",
                    metric.Name,
                    domainIndex);

                size_t globalMetricIndex = it->IsGlobal ? i : mappingIt->second - globalLBDomainEntry.MetricStartIndex;

                if (it->IsGlobal)
                {
                    //For global domain mark blocklisted nodes as invalid
                    for (auto nodeIt = nodeEntries.begin(); nodeIt != nodeEntries.end(); ++nodeIt)
                    {
                        if (!metric.IsValidNode(nodeIt->NodeIndex))
                        {
                            nodeUp_[nodeIt->NodeIndex] = false;
                        }
                    }
                }
                prepareSetForMetric(totalMetricIndex, globalMetricIndex, true, metric.ReservationLoad);
            }
        }
    }

    prepareSetForMetric(MoveCostIndex, MoveCostIndex, false, 0);
    prepareSetForMetric(OverlappingIndex, OverlappingIndex, false, 0);
}

DynamicNodeLoadSet::DynamicNodeLoadSet(DynamicNodeLoadSet && other)
{
    nodeCount_ = other.nodeCount_;
    version_ = other.version_;
    nodeLoads_ = move(other.nodeLoads_);
    nodeLoadBackup_ = move(other.nodeLoadBackup_);
    nodeCapacities_ = move(other.nodeCapacities_);
    fdIndexes_ = move(other.fdIndexes_);
    udIndexes_ = move(other.udIndexes_);
    nodeUp_ = move(other.nodeUp_);
    nodeLoadIndexes_ = move(other.nodeLoadIndexes_);
    moveCosts_ = move(other.moveCosts_);
    overlappingLoads_ = move(other.overlappingLoads_);
    lastBeneficialVersions_ = move(other.lastBeneficialVersions_);
    fdDistributionHelper_ = move(other.fdDistributionHelper_);
    udDistributionHelper_ = move(other.udDistributionHelper_);
    reservedLoads_ = move(other.reservedLoads_);
    allDynamicNodeLoads_ = move(other.allDynamicNodeLoads_);
    overlapping_ = other.overlapping_;
}

DynamicNodeLoadSet& DynamicNodeLoadSet::operator=(DynamicNodeLoadSet& other)
{
    if (this != &other)
    {
        nodeCount_ = other.nodeCount_;
        version_ = other.version_;
        nodeLoads_ = move(other.nodeLoads_);
        nodeLoadBackup_ = move(other.nodeLoadBackup_);
        nodeCapacities_ = move(other.nodeCapacities_);
        fdIndexes_ = move(other.fdIndexes_);
        udIndexes_ = move(other.udIndexes_);
        nodeUp_ = move(other.nodeUp_);
        nodeLoadIndexes_ = move(other.nodeLoadIndexes_);
        moveCosts_ = move(other.moveCosts_);
        overlappingLoads_ = move(other.overlappingLoads_);
        lastBeneficialVersions_ = move(other.lastBeneficialVersions_);
        fdDistributionHelper_ = move(other.fdDistributionHelper_);
        udDistributionHelper_ = move(other.udDistributionHelper_);
        reservedLoads_ = move(other.reservedLoads_);
        allDynamicNodeLoads_ = move(other.allDynamicNodeLoads_);
        overlapping_ = other.overlapping_;
    }

    return *this;
}

DynamicNodeLoadSet::DynamicNodeLoadSet(DynamicNodeLoadSet const& other)
    : nodeCount_(other.nodeCount_),
    version_(other.version_),
    nodeLoads_(other.nodeLoads_),
    nodeLoadBackup_(other.nodeLoadBackup_),
    nodeCapacities_(other.nodeCapacities_),
    fdIndexes_(other.fdIndexes_),
    udIndexes_(other.udIndexes_),
    nodeUp_(other.nodeUp_),
    moveCosts_(other.moveCosts_),
    overlappingLoads_(other.overlappingLoads_),
    nodeLoadIndexes_(),
    lastBeneficialVersions_(),
    fdDistributionHelper_(),
    udDistributionHelper_(),
    reservedLoads_(),
    allDynamicNodeLoads_(),
    overlapping_(other.overlapping_)
{
    for (int index = 0; index < nodeLoads_.size(); ++index)
    {
        DynamicNodeLoad dynamicLoad = DynamicNodeLoad(&(nodeLoads_[index]),
            &(nodeCapacities_[index]),
            &(fdIndexes_[index]),
            &(udIndexes_[index]),
            nodeUp_[index],
            index,
            (overlappingLoads_[index]),
            (moveCosts_[index]));
        allDynamicNodeLoads_.push_back(dynamicLoad);
    }

    for (auto it = other.nodeLoadIndexes_.begin(); it != other.nodeLoadIndexes_.end(); ++it)
    {
        auto oldComparator = (NodeLoadComparator)it->second.value_comp();

        size_t totalMetricIndex = oldComparator.TotalMetricIndex;
        size_t globalMetricIndex = oldComparator.GlobalMetricIndex;
        size_t reservationAmount = oldComparator.ReservationAmount;

        NodeLoadComparator cmp(totalMetricIndex, globalMetricIndex, reservationAmount);
        std::set<DynamicNodeLoad*, NodeLoadComparator> nodeLoadIndex(cmp);
        std::vector<int> lastBeneficialVersion;
        std::map<TreeNodeIndex, int> fdDistribution;
        std::map<TreeNodeIndex, int> udDistribution;

        for (int index = 0; index < nodeLoads_.size(); ++index)
        {
            nodeLoadIndex.insert(&allDynamicNodeLoads_[index]);
            lastBeneficialVersion.push_back(-1);
        }
        nodeLoadIndexes_.insert(make_pair(totalMetricIndex, nodeLoadIndex));
        lastBeneficialVersions_.insert(make_pair(totalMetricIndex, lastBeneficialVersion));
        fdDistributionHelper_.insert(make_pair(totalMetricIndex, fdDistribution));
        udDistributionHelper_.insert(make_pair(totalMetricIndex, udDistribution));
        reservedLoads_.insert(make_pair(totalMetricIndex, other.reservedLoads_.find(totalMetricIndex)->second));
    }
}

DynamicNodeLoadSet::~DynamicNodeLoadSet()
{
}

void DynamicNodeLoadSet::ResetLoads()
{
    nodeLoads_.clear();

    for (auto it = nodeLoadBackup_.begin(); it != nodeLoadBackup_.end(); ++it)
    {
        nodeLoads_.push_back(move(LoadEntry(*it)));
    }

    for (auto it = nodeLoadIndexes_.begin(); it != nodeLoadIndexes_.end(); ++it)
    {
        auto& nodeLoadIndex = it->second;

        nodeLoadIndex.clear();
        for (int index = 0; index < nodeLoadBackup_.size(); ++index)
        {
            nodeLoadIndex.insert(&allDynamicNodeLoads_[index]);
        }
    }
}

void DynamicNodeLoadSet::UpdateLoadOnNode(int nodeIndex, int64 newNodeLoad, size_t metricIndex)
{
    auto it = nodeLoadIndexes_.find(metricIndex);
    auto& nodeLoadIndex = it->second;

    TESTASSERT_IF(nodeIndex >= nodeCount_, "Invalid node index");

    size_t startCount = nodeLoadIndex.size();

    auto metricReservationLoad = reservedLoads_.find(metricIndex)->second;
    auto oldLoadPercent = allDynamicNodeLoads_[nodeIndex].Load(metricIndex, metricReservationLoad.first, metricReservationLoad.second);
    nodeLoadIndex.erase(&allDynamicNodeLoads_[nodeIndex]);

    size_t removedCount = nodeLoadIndex.size();
    TESTASSERT_IF(removedCount != startCount - 1, "Node not removed successfully");

    if (metricIndex == MoveCostIndex)
    {
        moveCosts_[nodeIndex] = newNodeLoad;
    }
    else
    {
        nodeLoads_[nodeIndex].Set(metricIndex, newNodeLoad);
        if (overlapping_)
        {
            //When load for metric is updated, update overlapping load also
            auto newLoadPercent = allDynamicNodeLoads_[nodeIndex].Load(metricIndex, metricReservationLoad.first, metricReservationLoad.second);
            auto& overlappingNodes = nodeLoadIndexes_.find(OverlappingIndex)->second;
            size_t startCount1 = overlappingNodes.size();
            overlappingNodes.erase(&allDynamicNodeLoads_[nodeIndex]);
            CalculateOverlappingLoadOnNode(nodeIndex, newLoadPercent, oldLoadPercent);
            size_t removedCount1 = overlappingNodes.size();
            TESTASSERT_IF(removedCount1 != startCount1 - 1, "Node not removed successfully");
            overlappingNodes.insert(&allDynamicNodeLoads_[nodeIndex]);
        }
    }

    nodeLoadIndex.insert(&allDynamicNodeLoads_[nodeIndex]);

    size_t endCount = nodeLoadIndex.size();
    TESTASSERT_IF(startCount != endCount, "Node count shouldn't change");
}

void DynamicNodeLoadSet::CalculateOverlappingLoadOnNode(int nodeIndex, double newLoadPercentage, double oldLoadPercentage)
{
    // Overlapping load is calculated as sum of squared emptiness percentage per node for all scoped defrag metrics
    oldLoadPercentage = oldLoadPercentage < 1 ? oldLoadPercentage : 1;

    overlappingLoads_[nodeIndex] += pow(1 - oldLoadPercentage, 2);

    newLoadPercentage = newLoadPercentage < 1 ? newLoadPercentage : 1;

    overlappingLoads_[nodeIndex] -= pow(1 - newLoadPercentage, 2);
}

void DynamicNodeLoadSet::UpdateNodeLoad(int nodeIndex, int64 newNodeLoad, size_t metricIndex)
{
    TESTASSERT_IF(metricIndex == MoveCostIndex || metricIndex == OverlappingIndex, "Invalid metric index, index:{0} is reserved for virtual metrics.", metricIndex);

    UpdateLoadOnNode(nodeIndex, newNodeLoad, metricIndex);
}

void DynamicNodeLoadSet::UpdateMoveCost(int nodeIndex, int64 moveCost)
{
    UpdateLoadOnNode(nodeIndex, moveCost, MoveCostIndex);
}

void DynamicNodeLoadSet::ForEachCandidateNode(
    size_t metricIndex,
    int32 emptyNodesTarget,
    Metric::DefragDistributionType const& defragDistributionType,
    size_t reservationLoad,
    std::function<void(double nodeLoadPercentage)> processor)
{
    ForEachCandidateNode(metricIndex, emptyNodesTarget, defragDistributionType, reservationLoad, processor, false);
}

void DynamicNodeLoadSet::ForEachCandidateNode(
    size_t metricIndex,
    int32 emptyNodesTarget,
    Metric::DefragDistributionType const& defragDistributionType,
    size_t reservationLoad,
    std::function<void(double nodeLoadPercentage)> processor,
    bool markingBeneficialNodes = false)
{
    auto it = nodeLoadIndexes_.find(metricIndex);

    if (it == nodeLoadIndexes_.end())
    {
        return;
    }
    //Comparator has globalMetricIndex
    size_t globalMetricIndex = ((NodeLoadComparator)it->second.value_comp()).GlobalMetricIndex;

    if (overlapping_)
    {
        //In case of overlapping nodes, we should iterate through candidate nodes for overlapping (best nodes for all metrics)
        //instead of candidate nodes just for specific metric
        if (metricIndex != MoveCostIndex)
        {
            it = nodeLoadIndexes_.find(OverlappingIndex);

            if (it == nodeLoadIndexes_.end())
            {
                return;
            }
        }
    }
    auto fdDistributionIt = fdDistributionHelper_.find(metricIndex);
    TESTASSERT_IF(fdDistributionIt == fdDistributionHelper_.end(), "FD distribution for metric missing");
    auto& fdDistribution = fdDistributionIt->second;

    auto udDistributionIt = udDistributionHelper_.find(metricIndex);
    TESTASSERT_IF(udDistributionIt == udDistributionHelper_.end(), "UD distribution for metric missing");
    auto& udDistribution = udDistributionIt->second;

    auto& nodeLoadIndex = it->second;
    auto& lastBeneficialVersion = lastBeneficialVersions_.find(metricIndex)->second;

    if (!markingBeneficialNodes)
    {
        AdvanceVersion();
    }

    int count = 0;

    auto chooseNodes = [&](std::function<bool(bool fdUsed, bool udUsed)> predicate)
    {
        for (auto i = nodeLoadIndex.begin(); i != nodeLoadIndex.end() && count < emptyNodesTarget; ++i)
        {
            auto it = *i;
            if (!nodeUp_[it->NodeIndex] || lastBeneficialVersion[it->NodeIndex] == version_)
            {
                continue;
            }

            bool fdUsed = false;
            auto fdVersion = fdDistribution.find(fdIndexes_[it->NodeIndex]);
            if (fdVersion != fdDistribution.end())
            {
                if (fdVersion->second == version_)
                {
                    fdUsed = true;
                }
            }

            bool udUsed = false;
            auto udVersion = udDistribution.find(udIndexes_[it->NodeIndex]);
            if (udVersion != udDistribution.end())
            {
                if (udVersion->second == version_)
                {
                    udUsed = true;
                }
            }

            if (predicate(fdUsed, udUsed))
            {
                processor(it->Load(metricIndex, globalMetricIndex, reservationLoad));

                fdDistribution.erase(fdIndexes_[it->NodeIndex]);
                fdDistribution.insert(make_pair(fdIndexes_[it->NodeIndex], version_));

                udDistribution.erase(udIndexes_[it->NodeIndex]);
                udDistribution.insert(make_pair(udIndexes_[it->NodeIndex], version_));

                lastBeneficialVersion[it->NodeIndex] = version_;
                ++count;
            }
        }
    };

    if (defragDistributionType == Metric::DefragDistributionType::SpreadAcrossFDs_UDs)
    {
        chooseNodes([&](bool fdUsed, bool udUsed)
        {
            return !fdUsed && !udUsed;
        });

        if (count < emptyNodesTarget)
        {
            chooseNodes([&](bool fdUsed, bool udUsed)
            {
                return !fdUsed || !udUsed;
            });
        }
    }

    if (count < emptyNodesTarget)
    {
        chooseNodes([&](bool fdUsed, bool udUsed)
        {
            UNREFERENCED_PARAMETER(fdUsed);
            UNREFERENCED_PARAMETER(udUsed);

            return true;
        });
    }
}

void DynamicNodeLoadSet::PrepareBeneficialNodes(
    size_t metricIndex,
    int32 emptyNodesTarget,
    Metric::DefragDistributionType const& defragDistributionType,
    size_t reservationLoad)
{
    TESTASSERT_IF(metricIndex == MoveCostIndex, "Invalid metric index, this index is reserved for movecost.");
    ForEachCandidateNode(metricIndex, emptyNodesTarget, defragDistributionType, reservationLoad, [&](double percentage)
    {
        UNREFERENCED_PARAMETER(percentage);
    }, true);
}

void DynamicNodeLoadSet::PrepareMoveCostBeneficialNodes(
    int32 emptyNodesTarget,
    Metric::DefragDistributionType const& defragDistributionType)
{
    ForEachCandidateNode(MoveCostIndex, emptyNodesTarget, defragDistributionType, 0, [&](double percentage)
    {
        UNREFERENCED_PARAMETER(percentage);
    }, true);
}

void DynamicNodeLoadSet::ForEachNodeOrdered(
    size_t metricIndex,
    DynamicNodeLoadSet::Order order,
    std::function<bool(int nodeIndex, const LoadEntry* loadEntry)> processor)
{
    auto it = nodeLoadIndexes_.find(metricIndex);

    if (it == nodeLoadIndexes_.end())
    {
        return;
    }

    auto& nodeLoadIndex = it->second;

    auto processNode = [&](DynamicNodeLoad const& node)
    {
        if (nodeUp_[node.NodeIndex])
        {
            return processor(node.NodeIndex, node.Loads);
        }

        return true;
    };

    if (order == Order::Ascending)
    {
        for (auto nodeIt = nodeLoadIndex.begin(); nodeIt != nodeLoadIndex.end(); ++nodeIt)
        {
            if (!processNode(**nodeIt))
            {
                break;
            }
        }
    }
    else
    {
        for (auto nodeIt = nodeLoadIndex.rbegin(); nodeIt != nodeLoadIndex.rend(); ++nodeIt)
        {
            if (!processNode(**nodeIt))
            {
                break;
            }
        }
    }
}

bool DynamicNodeLoadSet::IsNodeBeneficial(size_t nodeIndex, size_t metricIndex) const
{
    if (!nodeUp_[nodeIndex]) return false;

    auto it = lastBeneficialVersions_.find(metricIndex);

    if (it == lastBeneficialVersions_.end()) return false;

    return it->second[nodeIndex] == version_;
}

bool DynamicNodeLoadSet::IsBeneficialNode(size_t nodeIndex, size_t metricIndex) const
{
    TESTASSERT_IF(metricIndex == MoveCostIndex, "Invalid metric index, this index is reserved for movecost.");

    return IsNodeBeneficial(nodeIndex, metricIndex);
}

bool DynamicNodeLoadSet::IsBeneficialNodeByMoveCost(size_t nodeIndex) const
{
    return IsNodeBeneficial(nodeIndex, MoveCostIndex);
}

bool DynamicNodeLoadSet::IsEnoughLoadReserved(
    size_t totalMetricIndex, 
    int32 numberOfNodesWithReservation,
    Metric::DefragDistributionType const & defragDistributionType, 
    size_t reservedLoad)
{
    //Check if node is empty is calculated this way to keep consistency with calculation in Score
    //For empty nodes, percentage is 0, so we decrease emptyNodesTarget. If emptyNodesTarget=0, then least loaded nodes for all defrag metrics
    //(candidate nodes for overlapping) are empty for given metric also, so enough load is reserved.
    double emptyNodesTarget = numberOfNodesWithReservation;
    ForEachCandidateNode(totalMetricIndex, numberOfNodesWithReservation, defragDistributionType, reservedLoad, [&](double percentage)
    {
        emptyNodesTarget -= pow(1 - percentage, 2);
    }, false);

    if (emptyNodesTarget > numeric_limits<float>::epsilon())
    {
        return false;
    }
    return true;
}

// lastBeneficialVersions_ contains a vectors of flags
// These flags determine whether a node is considered beneficial according to PrepareBeneficialNodes
// In order to not clear all flags every time we do PrepareBeneficialNodes, we use integer markers
// whose target value we change each run (the value of version_)
void DynamicNodeLoadSet::AdvanceVersion()
{
    static const int limit = INT_MAX - 1;

    ++version_;

    if (version_ >= limit)
    {
        for (auto it = lastBeneficialVersions_.begin(); it != lastBeneficialVersions_.end(); ++it)
        {
            auto curr = it->second;

            for (size_t i = 0; i < nodeCount_; ++i)
            {
                curr[i] = -1;
            }
        }

        for (auto it = fdDistributionHelper_.begin(); it != fdDistributionHelper_.end(); ++it)
        {
            it->second.clear();
        }

        for (auto it = fdDistributionHelper_.begin(); it != fdDistributionHelper_.end(); ++it)
        {
            it->second.clear();
        }

        version_ = 0;
    }
}
