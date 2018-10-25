// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Metric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

Metric::Metric(
    wstring && name,
    double weight,
    double balancingThreshold,
    DynamicBitSet && blockList,
    uint activityThreshold,
    int64 clusterTotalCapacity,
    int64 clusterBufferedCapacity,
    int64 clusterLoad,
    bool isDefrag,
    int32 defragEmptyNodeCount,
    size_t defragEmptyNodeLoadThreshold,
    int64 reservationLoad,
    DefragDistributionType defragEmptyNodesDistribution,
    double placementHeuristicIncomingLoadFactor,
    double placementHeuristicEmptySpacePercent,
    bool defragmentationScopedAlgorithmEnabled,
    PlacementStrategy placementStrategy,
    double defragmentationEmptyNodeWeight,
    double defragmentationNonEmptyNodeWeight,
    bool balancingByPercentage)
    : name_(move(name)),
    weight_(weight),
    balancingThreshold_(balancingThreshold),
    blockList_(move(blockList)),
    isBalanced_(false),
    activityThreshold_(activityThreshold),
    clusterTotalCapacity_(clusterTotalCapacity),
    clusterBufferedCapacity_(clusterBufferedCapacity),
    clusterLoad_(clusterLoad),
    isDefrag_(isDefrag),
    defragEmptyNodeCount_(defragEmptyNodeCount),
    defragEmptyNodeLoadThreshold_(defragEmptyNodeLoadThreshold),
    reservationLoad_(reservationLoad),
    defragEmptyNodesDistribution_(defragEmptyNodesDistribution),
    placementHeuristicIncomingLoadFactor_(placementHeuristicIncomingLoadFactor),
    placementHeuristicEmptySpacePercent_(placementHeuristicEmptySpacePercent),
    defragmentationScopedAlgorithmEnabled_(defragmentationScopedAlgorithmEnabled),
    placementStrategy_(placementStrategy),
    defragmentationEmptyNodeWeight_(defragmentationEmptyNodeWeight),
    defragmentationNonEmptyNodeWeight_(defragmentationNonEmptyNodeWeight),
    balancingByPercentage_(balancingByPercentage)
{
    // 1.0 means do balancing for any diff, 0.0 means infinity, e.g. never trigger balancing
    ASSERT_IFNOT(balancingThreshold >= 1.0 || balancingThreshold == 0.0,
        "balancingThreshold should >= 1 or == 0, current value is {0}", balancingThreshold);
}

Metric::Metric(Metric const & other)
    : name_(other.name_),
    weight_(other.weight_),
    balancingThreshold_(other.balancingThreshold_),
    blockList_(other.blockList_),
    isBalanced_(other.isBalanced_),
    activityThreshold_(other.activityThreshold_),
    clusterTotalCapacity_(other.clusterTotalCapacity_),
    clusterBufferedCapacity_(other.clusterBufferedCapacity_),
    clusterLoad_(other.clusterLoad_),
    isDefrag_(other.isDefrag_),
    defragEmptyNodeCount_(other.defragEmptyNodeCount_),
    defragEmptyNodeLoadThreshold_(other.defragEmptyNodeLoadThreshold_),
    reservationLoad_(other.reservationLoad_),
    defragEmptyNodesDistribution_(other.defragEmptyNodesDistribution_),
    placementHeuristicIncomingLoadFactor_(other.placementHeuristicIncomingLoadFactor_),
    placementHeuristicEmptySpacePercent_(other.placementHeuristicEmptySpacePercent_),
    defragmentationScopedAlgorithmEnabled_(other.defragmentationScopedAlgorithmEnabled_),
    placementStrategy_(other.placementStrategy_),
    defragmentationEmptyNodeWeight_(other.defragmentationEmptyNodeWeight_),
    defragmentationNonEmptyNodeWeight_(other.defragmentationNonEmptyNodeWeight_),
    balancingByPercentage_(other.balancingByPercentage_),
    indexInGlobalDomain_(other.indexInGlobalDomain_),
    indexInLocalDomain_(other.indexInLocalDomain_),
    indexInTotalDomain_(other.indexInTotalDomain_)
{
}

Metric::Metric(Metric && other)
    : name_(move(other.name_)),
    weight_(other.weight_),
    balancingThreshold_(other.balancingThreshold_),
    blockList_(move(other.blockList_)),
    isBalanced_(other.isBalanced_),
    activityThreshold_(other.activityThreshold_),
    clusterTotalCapacity_(other.clusterTotalCapacity_),
    clusterBufferedCapacity_(other.clusterBufferedCapacity_),
    clusterLoad_(other.clusterLoad_),
    isDefrag_(other.isDefrag_),
    defragEmptyNodeCount_(other.defragEmptyNodeCount_),
    defragEmptyNodeLoadThreshold_(other.defragEmptyNodeLoadThreshold_),
    reservationLoad_(other.reservationLoad_),
    defragEmptyNodesDistribution_(other.defragEmptyNodesDistribution_),
    placementHeuristicIncomingLoadFactor_(other.placementHeuristicIncomingLoadFactor_),
    placementHeuristicEmptySpacePercent_(other.placementHeuristicEmptySpacePercent_),
    defragmentationScopedAlgorithmEnabled_(other.defragmentationScopedAlgorithmEnabled_),
    placementStrategy_(other.placementStrategy_),
    defragmentationEmptyNodeWeight_(other.defragmentationEmptyNodeWeight_),
    defragmentationNonEmptyNodeWeight_(other.defragmentationNonEmptyNodeWeight_),
    balancingByPercentage_(other.balancingByPercentage_),
    indexInGlobalDomain_(other.indexInGlobalDomain_),
    indexInLocalDomain_(other.indexInLocalDomain_),
    indexInTotalDomain_(other.indexInTotalDomain_)
{
}

Metric & Metric::operator = (Metric && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        weight_ = other.weight_;
        balancingThreshold_ = other.balancingThreshold_;
        blockList_ = move(other.blockList_);
        isBalanced_ = other.isBalanced_;
        activityThreshold_ = other.activityThreshold_;
        clusterTotalCapacity_ = other.clusterTotalCapacity_;
        clusterBufferedCapacity_ = other.clusterBufferedCapacity_;
        clusterLoad_ = other.clusterLoad_;
        isDefrag_ = other.isDefrag_;
        defragEmptyNodeCount_ = other.defragEmptyNodeCount_;
        defragEmptyNodeLoadThreshold_ = other.defragEmptyNodeLoadThreshold_;
        reservationLoad_ = other.reservationLoad_;
        defragEmptyNodesDistribution_ = other.defragEmptyNodesDistribution_;
        placementHeuristicIncomingLoadFactor_ = other.placementHeuristicIncomingLoadFactor_;
        placementHeuristicEmptySpacePercent_ = other.placementHeuristicEmptySpacePercent_;
        defragmentationScopedAlgorithmEnabled_ = other.defragmentationScopedAlgorithmEnabled_;
        placementStrategy_ = other.placementStrategy_;
        defragmentationEmptyNodeWeight_ = other.defragmentationEmptyNodeWeight_;
        defragmentationNonEmptyNodeWeight_ = other.defragmentationNonEmptyNodeWeight_;
        balancingByPercentage_ = other.balancingByPercentage_;
        indexInGlobalDomain_ = other.indexInGlobalDomain_;
        indexInLocalDomain_ = other.indexInLocalDomain_;
        indexInTotalDomain_ = other.indexInTotalDomain_;
    }

    return *this;
}

bool Metric::get_ShouldCalculateBeneficialNodesForPlacement() const
{
    return isDefrag_ &&
        (
            placementHeuristicIncomingLoadFactor_ != 0 ||
            placementHeuristicEmptySpacePercent_ != 0 ||
            defragmentationScopedAlgorithmEnabled_
            );
}

void Metric::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}",
        name_, weight_, balancingThreshold_, isBalanced_, blockList_,
        activityThreshold_, clusterTotalCapacity_, clusterBufferedCapacity_,
        clusterLoad_, balancingByPercentage_);

    if (isDefrag_)
    {
        writer.Write("/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}",
            isDefrag_, defragEmptyNodeLoadThreshold_, reservationLoad_, defragEmptyNodeCount_,
            defragEmptyNodesDistribution_, placementHeuristicIncomingLoadFactor_,
            placementHeuristicEmptySpacePercent_, defragmentationScopedAlgorithmEnabled_,
            defragmentationEmptyNodeWeight_, placementStrategy_, defragmentationNonEmptyNodeWeight_);
    }
}

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & writer, Metric::DefragDistributionType const & val)
{
    switch (val)
    {
    case Metric::DefragDistributionType::SpreadAcrossFDs_UDs:
        writer.Write("SpreadAcrossFDsAndUDs");
        break;
    case Metric::DefragDistributionType::NumberOfEmptyNodes:
        writer.Write("NoDistribution");
        break;
    }
}

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & writer, Metric::PlacementStrategy const & val)
{
    switch (val)
    {
    case Metric::PlacementStrategy::Balancing:
        writer.Write("Balancing");
        break;
    case Metric::PlacementStrategy::ReservationAndBalance:
        writer.Write("ReservationAndBalance");
        break;
    case Metric::PlacementStrategy::Reservation:
        writer.Write("Reservation");
        break;
    case Metric::PlacementStrategy::ReservationAndPack:
        writer.Write("ReservationAndPack");
        break;
    case Metric::PlacementStrategy::Defragmentation:
        writer.Write("Defragmentation");
        break;
    }
}
