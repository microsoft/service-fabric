// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "NodeEntry.h"
#include "ServiceEntry.h"
#include "PlacementAndLoadBalancing.h"
#include "SearcherSettings.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PartitionEntry::PartitionEntry(
    Guid partitionId,
    int64 version,
    bool isInTransition,
    bool movable,
    ServiceEntry const* service,
    vector<PlacementReplica *> && replicas,
    LoadEntry && primary,
    LoadEntry && secondary,
    uint primaryMoveCost,
    uint secondaryMoveCost,
    vector<NodeEntry const*> && standByLocations,
    NodeEntry const* primarySwapOutLocation,
    NodeEntry const* primaryUpgradeLocation,
    vector<NodeEntry const*> && secondaryUpgradeLocation,
    bool inUpgrade,
    bool isFUInUpgrade,
    bool isInSingleReplicaUpgrade,
    SingletonReplicaUpgradeOptimizationType upgradeOptimization,
    int order,
    int extraReplicas,
    map<Federation::NodeId, std::vector<uint>> const& ftSecondaryMap,
    std::vector<PlacementReplica *> && standbyReplicas,
    SearcherSettings const & settings,
    int targetReplicaSetSize)
    : partitionId_(partitionId),
    version_(version),
    isInTransition_(isInTransition),
    movable_(movable),
    service_(service),
    primary_(move(primary)),
    secondary_(move(secondary)),
    primaryMoveCost_(primaryMoveCost),
    secondaryMoveCost_(secondaryMoveCost),
    order_(order),
    existingReplicas_(),
    primaryReplica_(nullptr),
    secondaryReplicas_(),
    newReplicas_(),
    standByLocations_(move(standByLocations)),
    primarySwapOutLocation_(primarySwapOutLocation),
    primaryUpgradeLocation_(primaryUpgradeLocation),
    secondaryUpgradeLocations_(move(secondaryUpgradeLocation)),
    inUpgrade_(inUpgrade),
    isFUInUpgrade_(isFUInUpgrade),
    isInSingleReplicaUpgrade_(isInSingleReplicaUpgrade),
    singletonReplicaUpgradeOptimization_(upgradeOptimization),
    upgradeIndex_(SIZE_MAX),
    numberOfExtraReplicas_(extraReplicas),
    secondaryAverage_(secondary_),
    standbyReplicas_(move(standbyReplicas)),
    settings_(settings),
    targetReplicaSetSize_(targetReplicaSetSize)
{
    for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
    {
        PlacementReplica * replica = *itReplica;

        if (replica->IsNew)
        {
            newReplicas_.push_back(replica);
        }
        else
        {
            existingReplicas_.push_back(replica);
        }

        if (service->IsStateful)
        {
            if (replica->Role == ReplicaRole::Primary && !replica->ShouldDisappear)
            {
                ASSERT_IFNOT(primaryReplica_ == nullptr, "Primary replica already exists");
                primaryReplica_ = replica;
            }
            else if (replica->Role == ReplicaRole::Secondary && !replica->ShouldDisappear)
            {
                secondaryReplicas_.push_back(replica);
            }
        }

        if (!replica->IsNew)
        {
            SetSecondaryLoadMap(ftSecondaryMap, replica->Node->NodeId);
        }
    }

    for (auto itSB = standByLocations_.begin(); itSB != standByLocations_.end(); ++itSB)
    {
        SetSecondaryLoadMap(ftSecondaryMap, (*itSB)->NodeId);
    }

    if (!secondaryMap_.empty())
    {
        // Calculate average load entry

        size_t numMetrics = secondary_.Length;
        vector<uint64> loadSum(numMetrics, 0);
        for (auto itMap = secondaryMap_.begin(); itMap != secondaryMap_.end(); ++itMap)
        {
            for (int i = 0; i < numMetrics; ++i)
            {
                loadSum[i] += itMap->second.Values[i];
            }
        }

        LoadEntry averageLoads(numMetrics, 0);
        for (int i = 0; i < numMetrics; ++i)
        {
            averageLoads.Set(i, static_cast<int64>(floor(static_cast<double>(loadSum[i]) / ftSecondaryMap.size() + 0.5)));
        }

        secondaryAverage_ = move(averageLoads);
    }

}

PartitionEntry::PartitionEntry(PartitionEntry && other)
    : partitionId_(other.partitionId_),
    version_(other.version_),
    isInTransition_(other.isInTransition_),
    movable_(other.movable_),
    service_(move(other.service_)),
    primary_(move(other.primary_)),
    secondary_(move(other.secondary_)),
    primaryMoveCost_(other.primaryMoveCost_),
    secondaryMoveCost_(other.secondaryMoveCost_),
    order_(other.order_),
    existingReplicas_(move(other.existingReplicas_)),
    primaryReplica_(other.primaryReplica_),
    secondaryReplicas_(move(other.secondaryReplicas_)),
    newReplicas_(move(other.newReplicas_)),
    standByLocations_(move(other.standByLocations_)),
    primarySwapOutLocation_(other.primarySwapOutLocation_),
    primaryUpgradeLocation_(other.primaryUpgradeLocation_),
    secondaryUpgradeLocations_(move(other.secondaryUpgradeLocations_)),
    inUpgrade_(other.inUpgrade_),
    isFUInUpgrade_(other.isFUInUpgrade_),
    isInSingleReplicaUpgrade_(other.isInSingleReplicaUpgrade_),
    singletonReplicaUpgradeOptimization_(other.singletonReplicaUpgradeOptimization_),
    upgradeIndex_(other.upgradeIndex_),
    numberOfExtraReplicas_(other.numberOfExtraReplicas_),
    secondaryMap_(move(other.secondaryMap_)),
    secondaryAverage_(move(other.secondaryAverage_)),
    standbyReplicas_(move(other.standbyReplicas_)),
    settings_(other.settings_),
    targetReplicaSetSize_(other.targetReplicaSetSize_)
{
    ForEachReplica([&](PlacementReplica * replica)
    {
        replica->partitionEntry_ = this;
    }, true, true);
    ForEachStandbyReplica([=](PlacementReplica *replica)
    {
        replica->partitionEntry_ = this;
    });
}

PartitionEntry & PartitionEntry::operator = (PartitionEntry && other)
{
    if (this != &other)
    {
        partitionId_ = other.partitionId_;
        version_ = other.version_;
        isInTransition_ = other.isInTransition_;
        movable_ = other.movable_;
        service_ = move(other.service_);
        primary_ = move(other.primary_);
        secondary_ = move(other.secondary_);
        primaryMoveCost_ = other.primaryMoveCost_;
        secondaryMoveCost_ = other.secondaryMoveCost_;
        order_ = other.order_;
        existingReplicas_ = move(other.existingReplicas_);
        primaryReplica_ = other.primaryReplica_;
        secondaryReplicas_ = move(other.secondaryReplicas_);
        newReplicas_ = move(other.newReplicas_);
        standByLocations_ = move(other.standByLocations_);
        primarySwapOutLocation_ = other.primarySwapOutLocation_;
        primaryUpgradeLocation_ = other.primaryUpgradeLocation_;
        secondaryUpgradeLocations_ = move(other.secondaryUpgradeLocations_);
        inUpgrade_ = other.inUpgrade_;
        isFUInUpgrade_ = other.isFUInUpgrade_;
        isInSingleReplicaUpgrade_ = other.isInSingleReplicaUpgrade_;
        singletonReplicaUpgradeOptimization_ = other.singletonReplicaUpgradeOptimization_;
        upgradeIndex_ = other.upgradeIndex_;
        numberOfExtraReplicas_ = other.numberOfExtraReplicas_;
        secondaryMap_ = move(other.secondaryMap_);
        secondaryAverage_ = move(other.secondaryAverage_);
        standbyReplicas_ = move(other.standbyReplicas_);
        targetReplicaSetSize_ = other.targetReplicaSetSize_;

        ForEachReplica([&](PlacementReplica * replica)
        {
            replica->partitionEntry_ = this;
        }, true, true);
        ForEachStandbyReplica([=](PlacementReplica *replica)
        {
            replica->partitionEntry_ = this;
        });
    }

    return *this;
}

// True if this partition has existing replica on the node.
bool PartitionEntry::HasReplicaOnNode(NodeEntry const* node) const
{
    if (existingReplicas_.size() == 0)
    {
        return false;
    }
    for (auto replica : existingReplicas_)
    {
        if (replica->Node == node)
        {
            return true;
        }
    }

    return false;
}

PlacementReplica const* PartitionEntry::GetReplica(NodeEntry const* node) const
{
    ASSERT_IF(node == nullptr, "Invalid node");

    for (auto it = existingReplicas_.begin(); it != existingReplicas_.end(); ++it)
    {
        if ((*it)->Node == node && !(*it)->ShouldDisappear)
        {
            return *it;
        }
    }

    return nullptr;
}

PlacementReplica const* PartitionEntry::SelectSecondary(Random & rand) const
{
    ASSERT_IF(secondaryReplicas_.empty(), "No secondary replicas to select");
    size_t replicaSize = secondaryReplicas_.size();
    size_t index = replicaSize > 1 ? rand.Next(static_cast<int>(replicaSize)) : 0;
    return secondaryReplicas_[index];
}

PlacementReplica const* PartitionEntry::SelectNewReplica(size_t id) const
{
    if (newReplicas_.empty() || id >= newReplicas_.size())
    {
        return nullptr;
    }

    return newReplicas_[id];
}

PlacementReplica * PartitionEntry::NewReplica(size_t index) const
{
    if(newReplicas_.empty() || index > newReplicas_.size() - 1)
    {
        return nullptr;
    }

    return newReplicas_[index];
}

PlacementReplica const* PartitionEntry::SelectExistingReplica(size_t id) const
{
    if (existingReplicas_.empty() || id >= existingReplicas_.size())
    {
        return nullptr;
    }

    return existingReplicas_[id];
}

void PartitionEntry::ConstructReplicas()
{
    ForEachReplica([=](PlacementReplica *replica)
    {
        replica->SetPartitionEntry(this);
    }, true, true);
    ForEachStandbyReplica([=](PlacementReplica *replica)
    {
        replica->SetPartitionEntry(this);
    });
}

void PartitionEntry::ForEachReplica(
    std::function<void(PlacementReplica *)> processor,
    bool includeNone,
    bool includeDisappearingReplicas)
{
    ForEachExistingReplica(processor, includeNone, includeDisappearingReplicas);
    ForEachNewReplica(processor);
}

void PartitionEntry::ForEachReplica(
    std::function<void(PlacementReplica const*)> processor,
    bool includeNone,
    bool includeDisappearingReplicas) const
{
    ForEachExistingReplica(processor, includeNone, includeDisappearingReplicas);
    ForEachNewReplica(processor);
}

void PartitionEntry::ForEachExistingReplica(
    std::function<void(PlacementReplica *)> processor,
    bool includeNone,
    bool includeDisappearingReplicas)
{
    for_each(existingReplicas_.begin(), existingReplicas_.end(), [&](PlacementReplica * r)
    {
        if (!includeNone && r->Role == ReplicaRole::None)
        {
            return;
        }

        if (!includeDisappearingReplicas && r->ShouldDisappear)
        {
            return;
        }

        processor(r);
    });
}

void PartitionEntry::ForEachExistingReplica(
    std::function<void(PlacementReplica const*)> processor,
    bool includeNone,
    bool includeDisappearingReplicas) const
{
    for_each(existingReplicas_.begin(), existingReplicas_.end(), [&](PlacementReplica const* r)
    {
        if (!includeNone && r->Role == ReplicaRole::None)
        {
            return;
        }

        if (!includeDisappearingReplicas && r->ShouldDisappear)
        {
            return;
        }

        processor(r);
    });
}

void PartitionEntry::ForEachNewReplica(std::function<void(PlacementReplica *)> processor)
{
    for_each(newReplicas_.begin(), newReplicas_.end(), processor);
}

void PartitionEntry::ForEachNewReplica(std::function<void(PlacementReplica const*)> processor) const
{
    for_each(newReplicas_.begin(), newReplicas_.end(), processor);
}

void PartitionEntry::ForEachStandbyReplica(std::function<void(PlacementReplica *)> processor)
{
    for_each(standbyReplicas_.begin(), standbyReplicas_.end(), processor);
}

void PartitionEntry::ForEachStandbyReplica(std::function<void(PlacementReplica const*)> processor) const
{
    for_each(standbyReplicas_.begin(), standbyReplicas_.end(), processor);
}

LoadEntry const& PartitionEntry::GetReplicaEntry(ReplicaRole::Enum role, bool useNodeId, Federation::NodeId const & nodeId) const
{
    if (role == ReplicaRole::Primary)
    {
        return primary_;
    }
    else if (role == ReplicaRole::Secondary || role == ReplicaRole::StandBy)
    {
        bool useDefaultLoad = this->service_->OnEveryNode && settings_.UseDefaultLoadForServiceOnEveryNode;
        if (settings_.UseSeparateSecondaryLoad && !secondaryMap_.empty())
        {
            if (useNodeId)
            {
                auto it = secondaryMap_.find(nodeId);
                if (it != secondaryMap_.end())
                {
                    return it->second;
                }
            }

            // if nodeId not in map or no nodeId while the map has entries, use average load
            //special case for services on every node
            if (!useDefaultLoad)
            {
                return secondaryAverage_;
            }
        }
        //for services on every node in case the config is set we will return this value
        return secondary_;
    }
    else if (role == ReplicaRole::None)
    {
        return service_->ZeroLoadEntry;
    }
    else
    {
        Assert::CodingError("Unknown replica role");
    }
}

uint PartitionEntry::GetMoveCost(ReplicaRole::Enum role) const
{
    if (role == ReplicaRole::Primary)
    {
        return primaryMoveCost_;
    }
    else if (role == ReplicaRole::Secondary)
    {
        return secondaryMoveCost_;
    }
    else if (role == ReplicaRole::None)
    {
        return 0;
    }
    else
    {
        Assert::CodingError("Unknown replica role");
    }
}

void PartitionEntry::SetUpgradeIndex(size_t upgradeIndex)
{
    upgradeIndex_ = upgradeIndex;
}

PlacementReplica const* PartitionEntry::FindMovableReplicaForSingletonReplicaUpgrade() const
{
    if (IsTargetOne)
    {
        // If partition is in singleton replica upgrade, then new secondary is movable one
        // Node:       N0          N1
        // Replicas:   P/LI/+1
        if (IsInSingleReplicaUpgrade && ExistingReplicaCount == 1 && NewReplicaCount == 1)
        {
            return SelectNewReplica(0);
        }
        // If partition is not in upgrade move exiting replica, if it is movable
        // Node:       N0          N1
        // Parent:     P/LI/+1
        // Child1:     P
        // Child2:     S
        else if (!IsInSingleReplicaUpgrade &&
            newReplicas_.size() == 0 &&
            existingReplicas_.size() == 1 &&
            existingReplicas_[0]->IsSingletonReplicaMovableDuringUpgrade
            )
        {
            return SelectExistingReplica(0);
        }
        // Stateful replica movement drop:
        // If initial movement has started, but FM drops/rejects movement for one partition,
        // and then sends addition FT update with +1, hence accepted movements are in progress
        // (for stateful replicas)
        else if (Service->IsStateful &&
            newReplicas_.size() == 0 &&
            existingReplicas_.size() == 2 &&
            existingReplicas_[0]->IsSingletonReplicaMovableDuringUpgrade &&   // Wait until both replicas are movable
            existingReplicas_[1]->IsSingletonReplicaMovableDuringUpgrade)
        {
            // Before primary swap out phase (secondary is movable)
            // Node:       N0          N1
            // Parent:     P/LI        S  
            // Child1:     P/LI/+1
            // After primary swap out phase (primary is movable)
            // Node:       N0          N1
            // Parent:     S/L         P  
            // Child1:     P/LI/+1
            if ((existingReplicas_[0]->IsPrimary && existingReplicas_[0]->IsInUpgrade) ||
                (existingReplicas_[1]->IsPrimary && !existingReplicas_[1]->IsInUpgrade))
            {
                return SelectExistingReplica(1);
            }
            else if ((existingReplicas_[1]->IsPrimary && existingReplicas_[1]->IsInUpgrade) ||
                    (existingReplicas_[0]->IsPrimary && !existingReplicas_[0]->IsInUpgrade))
            {
                return SelectExistingReplica(0);
            }
        }
        // Stateless replica movement drop:
        // If initial movement has started, but FM drops/rejects movement for one partition,
        // and then sends addition FT update with +1, hence accepted movements are in progress
        // (for stateless replicas)
        else if (!Service->IsStateful &&
            newReplicas_.size() == 0 &&
            existingReplicas_.size() == 2)
        {
            // Stateless parent replica is stuck in reconfiguration,
            // as neither of parent replicas can be dropped, and -1 has been received from FM.
            // If this case, new secondary replicas which becomes ready and movable is candidate,
            // and initial move in progress replica will be dropped when all child replicas are moved
            // from node 0
            // Node:       N0          N1
            // Parent:     S/V         S  
            // Child1:     P/LI/+1
            // Child2:                 S
            if (Service->DependentServices.size() > 0) // it is parent service
            {
                if (existingReplicas_[0]->IsSingletonReplicaMovableDuringUpgrade && existingReplicas_[1]->IsMoveInProgress)
                {
                    return SelectExistingReplica(0);
                }
                else if (existingReplicas_[1]->IsSingletonReplicaMovableDuringUpgrade && existingReplicas_[0]->IsMoveInProgress)
                {
                    return SelectExistingReplica(1);
                }
            }
        }
    }
    return nullptr;
}

bool PartitionEntry::IsInSingletonReplicaUpgradeOptimization() const
{
    if (Service->HasAffinityAssociatedSingletonReplicaInUpgrade ||
        (Service->Application != nullptr &&
            Service->Application->HasPartitionsInSingletonReplicaUpgrade))
    {
        return true;
    }
    else
    {
        return false;
    }
}

size_t PartitionEntry::GetExistingSecondaryCount() const
{
    size_t count(0);
    for (size_t i = 0; i < secondaryReplicas_.size(); ++i)
    {
        if (secondaryReplicas_[i] && !secondaryReplicas_[i]->IsNew)
        {
            count++;
        }
    }
    return count;
}

void PartitionEntry::SetSecondaryLoadMap(map<Federation::NodeId, std::vector<uint>> const& inputMap, Federation::NodeId const& node)
{
    auto secondary = inputMap.find(node);
    if (secondary != inputMap.end())
    {
        vector<int64> secondaryEntry(secondary->second.begin(), secondary->second.end());
        secondaryMap_.insert(make_pair(node, LoadEntry(move(secondaryEntry))));
    }
}

GlobalWString const PartitionEntry::TraceDescription = make_global<wstring>(L"[Partitions]\r\n#partitionId serviceName version isInTransition movable primaryLoad secondaryLoad targetReplicaSetSize existingReplicas newReplicas standByLocations primarySwapOutLocation primaryUpgradeLocation secondaryUpgradeLocation inUpgrade upgradeIndex");

void PartitionEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7} (",
        partitionId_,
        service_->Name,
        version_,
        isInTransition_,
        movable_,
        primary_,
        secondary_,
        targetReplicaSetSize_);

    for (auto it = existingReplicas_.begin(); it != existingReplicas_.end(); ++it)
    {
        if (it != existingReplicas_.begin())
        {
            writer.Write(" ");
        }

        writer.Write("{0}/{1}/{2}", (*it)->Node->NodeId, (*it)->Role, (*it)->IsMovable);
    }
    writer.Write(") (");

    for (auto it = newReplicas_.begin(); it != newReplicas_.end(); ++it)
    {
        if (it != newReplicas_.begin())
        {
            writer.Write(" ");
        }

        writer.Write("{0}", (*it)->Role);
    }
    writer.Write(") (");

    for (auto it = standByLocations_.begin(); it != standByLocations_.end(); ++it)
    {
        if (it != standByLocations_.begin())
        {
            writer.Write(" ");
        }

        writer.Write("{0}", (*it)->NodeId);
    }

    writer.Write(") (");
    if (primarySwapOutLocation_)
    {
        writer.Write("{0}", primarySwapOutLocation_->NodeId);
    }

    writer.Write(") (");
    if (primaryUpgradeLocation_)
    {
        writer.Write("{0}", primaryUpgradeLocation_->NodeId);
    }

    writer.Write(") (");
    for (auto it = secondaryUpgradeLocations_.begin(); it != secondaryUpgradeLocations_.end(); ++it)
    {
        if (it != secondaryUpgradeLocations_.begin())
        {
            writer.Write(" ");
        }

        writer.Write("{0}", (*it)->NodeId);
    }

    writer.Write(") (");
    writer.Write("{0}", inUpgrade_);

    if (inUpgrade_)
    {
        writer.Write(") (");
        writer.Write("{0}", upgradeIndex_);
    }
}

void PartitionEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
