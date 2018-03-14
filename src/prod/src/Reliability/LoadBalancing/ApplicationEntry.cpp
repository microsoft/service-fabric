// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementReplica.h"
#include "ApplicationEntry.h"
#include "TempSolution.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ApplicationEntry::ApplicationEntry(
    wstring && name,
    uint64 applicationId,
    int scaleoutCount,
    LoadEntry && appCapacities,
    LoadEntry && perNodeCapacities,
    LoadEntry && reservation,
    LoadEntry && totalLoad,
    std::map<NodeEntry const*, LoadEntry> && nodeLoads,
    std::map<NodeEntry const*, LoadEntry> && disappearingNodeLoads,
    std::map<NodeEntry const*, size_t> && nodeCounts,
    std::set<Common::TreeNodeIndex> && ugradedUDs)
    : name_(move(name)),
    applicationId_(applicationId),
    scaleoutCount_(scaleoutCount),
    appCapacities_(move(appCapacities)),
    perNodeCapacities_(move(perNodeCapacities)),
    reservation_(move(reservation)),
    hasAppCapacity_(false),
    hasPerNodeCapacity_(false),
    hasReservation_(false),
    totalLoads_(move(totalLoad)),
    nodeLoads_(move(nodeLoads)),
    disappearingNodeLoads_(move(disappearingNodeLoads)),
    nodeCounts_(move(nodeCounts)),
    upgradeCompletedUDs_(move(ugradedUDs)),
    hasPartitionsInSingletonReplicaUpgrade_(false),
    corelatedSingletonReplicasInUpgrade_()
{
    ASSERT_IFNOT(appCapacities_.Values.size() == perNodeCapacities_.Values.size(),
        "Application capacities size {0} and capacity per node size {1} should be equal",
        appCapacities_.Values.size(), perNodeCapacities_.Values.size());

    if (find_if(appCapacities_.Values.begin(), appCapacities_.Values.end(), [](int64 value) { return value >= 0; })
        != appCapacities_.Values.end())
    {
        hasAppCapacity_ = true;
    }

    if (find_if(perNodeCapacities_.Values.begin(), perNodeCapacities_.Values.end(), [](int64 value) { return value >= 0; })
        != perNodeCapacities_.Values.end())
    {
        hasPerNodeCapacity_ = true;
    }

    if (find_if(reservation_.Values.begin(), reservation_.Values.end(), [](int64 value) { return value > 0; })
        != reservation_.Values.end())
    {
        hasReservation_ = true;
    }

    hasScaleoutOrCapacity_ = hasAppCapacity_ || hasPerNodeCapacity_ || hasReservation_ || (scaleoutCount > 0);
}

ApplicationEntry::ApplicationEntry(ApplicationEntry && other)
    : name_(move(other.name_)),
    applicationId_(other.applicationId_),
    scaleoutCount_(other.scaleoutCount_),
    appCapacities_(move(other.appCapacities_)),
    perNodeCapacities_(move(other.perNodeCapacities_)),
    reservation_(move(other.reservation_)),
    hasAppCapacity_(other.hasAppCapacity_),
    hasPerNodeCapacity_(other.hasPerNodeCapacity_),
    hasReservation_(other.hasReservation_),
    totalLoads_(move(other.totalLoads_)),
    nodeLoads_(move(other.NodeLoads)),
    disappearingNodeLoads_(move(other.disappearingNodeLoads_)),
    nodeCounts_(move(other.nodeCounts_)),
    upgradeCompletedUDs_(move(other.upgradeCompletedUDs_)),
    hasScaleoutOrCapacity_(other.hasScaleoutOrCapacity_),
    hasPartitionsInSingletonReplicaUpgrade_(other.hasPartitionsInSingletonReplicaUpgrade_),
    corelatedSingletonReplicasInUpgrade_(move(other.corelatedSingletonReplicasInUpgrade_))
{
}

ApplicationEntry & ApplicationEntry::operator = (ApplicationEntry && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        applicationId_ = other.applicationId_;
        scaleoutCount_ = other.scaleoutCount_;
        appCapacities_ = move(other.appCapacities_);
        perNodeCapacities_ = move(other.perNodeCapacities_);
        reservation_ = move(other.reservation_);
        hasAppCapacity_ = other.hasAppCapacity_;
        hasPerNodeCapacity_ = other.hasPerNodeCapacity_;
        hasReservation_ = other.hasReservation_;
        totalLoads_ = move(other.totalLoads_);
        nodeLoads_ = move(other.nodeLoads_);
        disappearingNodeLoads_ = move(other.disappearingNodeLoads_);
        nodeCounts_ = move(other.nodeCounts_);
        upgradeCompletedUDs_ = move(other.upgradeCompletedUDs_);
        hasScaleoutOrCapacity_ = other.hasScaleoutOrCapacity_;
        hasPartitionsInSingletonReplicaUpgrade_ = other.hasPartitionsInSingletonReplicaUpgrade_;
        corelatedSingletonReplicasInUpgrade_ = move(other.corelatedSingletonReplicasInUpgrade_);
    }

    return *this;
}

// return the effective load reservation.
int64 ApplicationEntry::GetReservationDiff(size_t capacityIndex, int64 actualNodeLoad) const
{
    if (reservation_.Values.empty())
    {
        // No reservation capacity defined
        return 0;
    }

    int64 diff = (actualNodeLoad < reservation_.Values[capacityIndex] ? (reservation_.Values[capacityIndex] - actualNodeLoad) : 0);
    return diff;
}

int64 ApplicationEntry::GetMetricNodeLoad(NodeEntry const* node, size_t metricIndex) const
{
    auto itOrigLoad = NodeLoads.find(node);
    if (itOrigLoad != NodeLoads.end())
    {
        return itOrigLoad->second.Values[metricIndex];
    }

    return 0;
}

int64 ApplicationEntry::GetDisappearingMetricNodeLoad(NodeEntry const* node, size_t metricIndex) const
{
    auto itDisappearingLoad = DisappearingNodeLoads.find(node);
    if (itDisappearingLoad != DisappearingNodeLoads.end())
    {
        return itDisappearingLoad->second.Values[metricIndex];
    }

    return 0;
}

void ApplicationEntry::SetRelaxedScaleoutReplicaSet(Placement const* placement)
{
    int appNodeCount = static_cast<int>(NodeCounts.size());
    bool hasNonMovableReplicas = false;

    // If application is in a scaleout violation with more that 2 nodes
    // then do not proceed with the upgrade
    if (appNodeCount > 2)
    {
        corelatedSingletonReplicasInUpgrade_.clear();
        return;
    }

    // Function that adds movable replicas to the vector
    auto AddMovableReplicasFunc = [&](const Reliability::LoadBalancingComponent::PlacementReplica *const curReplica)
    {
        // If the replica is in StandBy state,
        // (return persisted service after upgrade)
        // skip the search, as movable replica is already included
        if (curReplica->Role != ReplicaRole::Enum::StandBy)
        {
            // If partition is in upgrade, return new one.
            // Find movable replicas, for partitions which are not in upgrade,
            // but belongs to the application in singleton replica upgrade
            // (returns nullptr if there is none)
            PlacementReplica const* replica = curReplica->Partition->FindMovableReplicaForSingletonReplicaUpgrade();
            if (replica)
            {
                corelatedSingletonReplicasInUpgrade_.push_back(replica);
            }
            else
            {
                hasNonMovableReplicas = true;
            }
        }
    };

    // Add replicas from all relevant application partitions (including also the ones that are not in upgrade yet or from stateless services)
    // Application node count can have following values:
    //    - One node:
    //      * In this case we either have all partitions in upgrade or some still didn't receive +1 (or stateless),
    //        but there were no movements generated
    //          Node:           N0          N1
    //          Replicas(+1):   P/LI
    //          Replicas(-):    P
    //          Replicas(-):    S
    //    - Two nodes:
    //      * Either it is not an initial upgrade run (some replicas have successfully moved and build to destination node)
    //        and all other replicas from the source node are now in upgrade mode (initial move was dropped by FM)
    //          Node:           N0          N1
    //          Replicas(-):    S           P
    //          Replicas(+1):   P/LI
    //        Or application was in scaleout violation before upgrade, in which case optimization will wait for violation fix.

    if (appNodeCount == 1)
    {
        NodeEntry const* node = NodeCounts.begin()->first;
        ReplicaSet const& replicas = placement->ApplicationPlacements[this].operator[](node);
        std::for_each(replicas.Data.begin(), replicas.Data.end(), AddMovableReplicasFunc);
    }
    else
    {
        // Include all relevant replicas from both nodes
        placement->ApplicationPlacements[this].ForEach([&](std::pair<NodeEntry const*, ReplicaSet> const& itAppNode) -> bool
        {
            ReplicaSet const& curNodereplicas = itAppNode.second;
            std::for_each(curNodereplicas.Data.begin(), curNodereplicas.Data.end(), AddMovableReplicasFunc);
            return true;
        });
    }

    // If there is a non-movable replica in the application
    // which is correlated to the in-upgrade replicas,
    // clear the list and wait for future runs to move all replicas
    if (hasNonMovableReplicas)
    {
        corelatedSingletonReplicasInUpgrade_.clear();
    }
}

bool ApplicationEntry::IsInSingletonReplicaUpgrade(PlacementAndLoadBalancing const& plb, vector<PlacementReplica *>& replicas) const
{
    bool isAppInSingletonUpgrade = false;

    // Check if it is application upgrade
    Application const* app = plb.GetApplicationPtrCallerHoldsLock(ApplicationId);
    if (app != nullptr && app->ApplicationDesc.UpgradeInProgess)
    {
        isAppInSingletonUpgrade = true;
    }

    // Check if it is cluster upgrade
    if (const_cast<PlacementAndLoadBalancing&>(plb).IsClusterUpgradeInProgress())
    {
        isAppInSingletonUpgrade = true;
    }

    // Check if it is node deactivation
    for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
    {
        PlacementReplica * replica = *itReplica;
        if (replica->Role == ReplicaRole::Primary &&
            replica->IsOnDeactivatedNode)
        {
            isAppInSingletonUpgrade = true;
        }
    }

    return isAppInSingletonUpgrade;
}

GlobalWString const ApplicationEntry::TraceDescription = make_global<wstring>(
    L"[Applications]\r\n#applicationName scaleoutCount appCapacities perNodeCapacities");

void ApplicationEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7}", name_, scaleoutCount_, appCapacities_, perNodeCapacities_, reservation_, 
        hasAppCapacity_, hasPerNodeCapacity_, totalLoads_);
}

void ApplicationEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
