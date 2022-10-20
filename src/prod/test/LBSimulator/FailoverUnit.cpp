// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Service.h"
#include "Node.h"
#include "FailoverUnit.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

void FailoverUnit::Replica::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}:{1}/{2}", NodeIndex, NodeInstance, Role);
}

FailoverUnit::FailoverUnit(
    Common::Guid failoverUnitId,
    int serviceIndex,
    std::wstring serviceName,
    int targetReplicaCount,
    bool isStateful,
    map<wstring, uint> && primaryLoad,
    map<wstring, uint> && secondaryLoad)
    : index_(0),
    serviceIndex_(serviceIndex),
    serviceName_(serviceName),
    targetReplicaCount_(targetReplicaCount),
    isStateful_(isStateful),
    primaryLoad_(move(primaryLoad)),
    secondaryLoad_(move(secondaryLoad)),
    replicas_(),
    version_(0),
    failoverUnitId_(failoverUnitId)
{
}

FailoverUnit::FailoverUnit(FailoverUnit && other)
    : index_(other.index_),
    serviceIndex_(other.serviceIndex_),
    serviceName_(other.serviceName_),
    targetReplicaCount_(other.targetReplicaCount_),
    isStateful_(other.isStateful_),
    primaryLoad_(move(other.primaryLoad_)),
    secondaryLoad_(move(other.secondaryLoad_)),
    replicas_(move(other.replicas_)),
    version_(other.version_),
    failoverUnitId_(other.failoverUnitId_)
{
}

FailoverUnit & FailoverUnit::operator = (FailoverUnit && other)
{
    if (this != &other)
    {
        index_ = other.index_;
        serviceIndex_ = other.serviceIndex_;
        serviceName_ = other.serviceName_;
        targetReplicaCount_ = other.targetReplicaCount_;
        isStateful_ = other.isStateful_;
        primaryLoad_ = move(other.primaryLoad_);
        secondaryLoad_ = move(other.secondaryLoad_);
        replicas_ = move(other.replicas_);
        version_ = other.version_;
        failoverUnitId_ = other.failoverUnitId_;
    }

    return *this;
}

void FailoverUnit::ChangeVersion(int increment)
{
    version_ += increment;
}

void FailoverUnit::UpdateLoad(int role, wstring const& metric, uint load)
{
    if (IsStateful)
    {
        ASSERT_IFNOT(role == 0 || role == 1, "Role {0} invalid for a stateful FailoverUnit", role);
    }
    else
    {
        ASSERT_IFNOT(role == 1, "Role {0} invalid for a stateless FailoverUnit", role);
    }

    if (role == 0)
    {
        auto it = primaryLoad_.find(metric);
        if (it == primaryLoad_.end())
        {
            primaryLoad_.insert(make_pair(metric, load));
        }
        else
        {
            it->second = load;
        }
    }
    else
    {
        auto it = secondaryLoad_.find(metric);
        if (it == secondaryLoad_.end())
        {
            secondaryLoad_.insert(make_pair(metric, load));
        }
        else
        {
            it->second = load;
        }
    }
}

uint FailoverUnit::GetLoad(Replica const& replica, wstring const& metricName) const
{
    ASSERT_IFNOT(replica.Role == 0 || replica.Role == 1, "Invalid replica role {0}", replica.Role);

    uint load = 0;
    if (metricName == Service::PrimaryCountName)
    {
        if (replica.Role == 0) { load = 1; }
    }
    else if (metricName == Service::SecondaryCountName)
    {
        if (replica.Role == 1 && IsStateful) { load = 1; }
    }
    else if (metricName == Service::ReplicaCountName)
    {
        if (IsStateful) { load = 1; }
    }
    else if (metricName == Service::InstanceCountName)
    {
        if (!IsStateful) { load = 1; }
    }
    else if (metricName == Service::CountName)
    {
        load = 1;
    }
    else
    {
        map<wstring, uint> const & loadVec = (replica.Role == 0) ? primaryLoad_ : secondaryLoad_;
        auto itLoad = loadVec.find(metricName);
        ASSERT_IF(itLoad == loadVec.end(), "Load metric {0} doesn't exist", metricName);

        load = itLoad->second;
    }

    return load;
}

void FailoverUnit::AddReplica(Replica && replica)
{
    if (replica.Role == 0)
    {
        ASSERT_IF(!IsStateful, "Only can add primary replica for stateful partition");
        size_t primaryCount = count_if(replicas_.begin(), replicas_.end(), [](Replica const& r)
        {
            return r.Role == 0;
        });
        ASSERT_IF(primaryCount > 0, "Primary already exist");
    }

    replicas_.push_back(move(replica));

    ++version_;
}

void FailoverUnit::ReplicaDown(int nodeIndex)
{
    bool primaryRemoved = false;
    for (auto it = replicas_.begin(); it != replicas_.end(); it++)
    {
        if (it->NodeIndex == nodeIndex)
        {
            if (it->Role == 0)
            {
                primaryRemoved = true;
            }
            replicas_.erase(it);
            break;
        }
    }

    if (primaryRemoved && !replicas_.empty())
    {
        ASSERT_IFNOT(IsStateful, "Error");
        // promote the first replica to primary
        Replica & r = replicas_.front();
        r.Role = 0;
    }

    ++version_;
}

void FailoverUnit::PromoteReplica(int nodeIndex)
{
    for (auto it = replicas_.begin(); it != replicas_.end(); it++)
    {
        if (it->NodeIndex == nodeIndex)
        {
            // Set to replica on target node to Primary
            it->Role = static_cast<int>(Primary);
        }
        else if (it->Role == static_cast<int>(Primary))
        {
            // Else set replica to Secondary If it is Primary
            it->Role = static_cast<int>(Secondary);
        }
    }
    ++version_;
}

void FailoverUnit::MoveReplica(int sourceNodeIndex, int targetNodeIndex, map<int, LBSimulator::Node> const& nodes)
{
    for (auto it = replicas_.begin(); it != replicas_.end(); it++)
    {
        ASSERT_IF(it->NodeIndex == targetNodeIndex, "Target node has replica of this failover unit");
    }

    auto itSourceNode = nodes.find(sourceNodeIndex);
    auto itTargetNode = nodes.find(targetNodeIndex);
    if (itSourceNode != nodes.end() && itSourceNode->second.IsUp && itTargetNode != nodes.end() && itTargetNode->second.IsUp)
    {
        auto itReplica = GetReplica(sourceNodeIndex);
        int currentRole = itReplica->Role;
        replicas_.erase(itReplica);
        replicas_.push_back(FailoverUnit::Replica(targetNodeIndex, itTargetNode->second.Instance, currentRole));
    }
    ++version_;
}

void FailoverUnit::FailoverUnitToBeDeleted()
{
    replicas_.clear();
    targetReplicaCount_ = 0;
    ++version_;
}

void FailoverUnit::ProcessMovement(Reliability::LoadBalancingComponent::FailoverUnitMovement const& moveAction,
    map<int, LBSimulator::Node> const& nodes, set<int> ignoredActions)
{
    // store new replicas temporarily so that we remove all old replicas first
    vector<FailoverUnit::Replica> newReplicas;

    int nodeIndex, nodeIndex2;
    map<int, LBSimulator::Node>::const_iterator itNode, itNode2;
    for each (Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction action in moveAction.Actions)
    {
        if (ignoredActions.count(static_cast<int>(action.TargetNode.IdValue.Low)) != 0)
        {
            continue;
        }
        switch (action.Action)
        {
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddPrimary:
            nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);
            if (itNode != nodes.end() && itNode->second.IsUp)
            {
                ASSERT_IFNOT(GetPrimary() == replicas_.end(), "Error");
                newReplicas.push_back(FailoverUnit::Replica(nodeIndex, itNode->second.Instance, 0));
            }
            break;

        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddSecondary:
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddInstance:
            nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);
            if (itNode != nodes.end() && itNode->second.IsUp)
            {
                newReplicas.push_back(FailoverUnit::Replica(nodeIndex, itNode->second.Instance, 1));
            }
            break;

        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary:
            nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);

            if (itNode != nodes.end() && itNode->second.IsUp)
            {
                auto itPrimary = GetPrimary();
                if (itPrimary != replicas_.end())
                {
                    itPrimary->Role = 1;
                }

                auto itReplica = GetReplica(nodeIndex);
                ASSERT_IF(itReplica == replicas_.end(), "Error");
                itReplica->Role = 0;
            }
            break;

        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::MovePrimary:
            nodeIndex = static_cast<int>(action.SourceNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);
            nodeIndex2 = static_cast<int>(action.TargetNode.IdValue.Low);
            itNode2 = nodes.find(nodeIndex2);

            if (itNode != nodes.end() && itNode->second.IsUp && itNode2 != nodes.end() && itNode2->second.IsUp)
            {
                auto itReplica = GetReplica(nodeIndex);
                ASSERT_IFNOT(itReplica != replicas_.end() && itReplica->Role == LBSimulator::FailoverUnit::ReplicaRole::Primary, "Error");
                replicas_.erase(itReplica);
                newReplicas.push_back(FailoverUnit::Replica(nodeIndex2, itNode2->second.Instance, 0));
            }
            break;

        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::MoveSecondary:
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::MoveInstance:
            nodeIndex = static_cast<int>(action.SourceNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);
            nodeIndex2 = static_cast<int>(action.TargetNode.IdValue.Low);
            itNode2 = nodes.find(nodeIndex2);

            if (itNode != nodes.end() && itNode->second.IsUp && itNode2 != nodes.end() && itNode2->second.IsUp)
            {
                auto itReplica = GetReplica(nodeIndex);
                ASSERT_IFNOT(itReplica != replicas_.end() && itReplica->Role == LBSimulator::FailoverUnit::ReplicaRole::Secondary, "Error");
                replicas_.erase(itReplica);
                newReplicas.push_back(FailoverUnit::Replica(nodeIndex2, itNode2->second.Instance, 1));
            }
            break;
        //the replica is removed unconditionally here
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::DropPrimary:
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::DropSecondary:
        case Reliability::LoadBalancingComponent::FailoverUnitMovementType::DropInstance:
            nodeIndex = static_cast<int>(action.SourceNode.IdValue.Low);
            itNode = nodes.find(nodeIndex);
            if (itNode != nodes.end() && itNode->second.IsUp)
            {
                auto itReplica = GetReplica(nodeIndex);
                replicas_.erase(itReplica);
            }
            break;

        default:
            Assert::CodingError("Invalid action type");
            break;
        }
    }

    for (auto itReplica = newReplicas.begin(); itReplica != newReplicas.end(); ++itReplica)
    {
        if (!ContainsReplica(itReplica->NodeIndex))
        {
            replicas_.push_back(*itReplica);
        }
    }

    ++version_;
}

Reliability::LoadBalancingComponent::FailoverUnitDescription FailoverUnit::GetPLBFailoverUnitDescription() const
{
    vector<Reliability::LoadBalancingComponent::ReplicaDescription> replicas;
    int replicaRole = 0;
    int activeReplicaCount = 0;
    for each (Replica replicaDes in replicas_)
    {
        switch (replicaDes.Role)
        {
        case LBSimulator::FailoverUnit::ReplicaRole::Primary:
            replicaRole = Reliability::LoadBalancingComponent::ReplicaRole::Primary;
            activeReplicaCount++;
            break;
        case LBSimulator::FailoverUnit::ReplicaRole::Secondary:
            replicaRole = Reliability::LoadBalancingComponent::ReplicaRole::Secondary;
            activeReplicaCount++;
            break;
        case LBSimulator::FailoverUnit::ReplicaRole::Standby:
            replicaRole = Reliability::LoadBalancingComponent::ReplicaRole::StandBy;
            break;
        case LBSimulator::FailoverUnit::ReplicaRole::Dropped:
            replicaRole = Reliability::LoadBalancingComponent::ReplicaRole::Dropped;
            break;
        default:
            ASSERT(FALSE);
            break;
        }
        Federation::NodeInstance nodeInstance(Federation::NodeId(Common::LargeInteger(0, replicaDes.NodeIndex)), 0);
        replicas.push_back(Reliability::LoadBalancingComponent::ReplicaDescription(
            Node::CreateNodeInstance(replicaDes.NodeIndex, replicaDes.NodeInstance),
            Reliability::LoadBalancingComponent::ReplicaRole::Enum(replicaRole),
            Reliability::ReplicaStates::Ready,
            true,   // isUp
            Reliability::ReplicaFlags::None));
    }

    int replicaDiff = (targetReplicaCount_ == 0) ? INT_MAX : (targetReplicaCount_ - activeReplicaCount) ;

    Reliability::FailoverUnitFlags::Flags fmFlags;

    return Reliability::LoadBalancingComponent::FailoverUnitDescription(
        FailoverUnitId,
        wstring(serviceName_),
        version_,
        move(replicas),
        replicaDiff,
        fmFlags,
        false,  // isReconfigurationInProgress
        false,  // isInQuorumLost
        targetReplicaCount_);
}

Reliability::LoadBalancingComponent::LoadOrMoveCostDescription FailoverUnit::GetPLBLoadOrMoveCostDescription() const
{
    vector<Reliability::LoadBalancingComponent::LoadMetric> primaryEntries, secondaryEntries;

    Reliability::LoadBalancingComponent::LoadOrMoveCostDescription desc(FailoverUnitId, std::wstring(ServiceName), IsStateful);

    StopwatchTime now = Stopwatch::Now();

    if (IsStateful)
    {
        for (auto itLoad = PrimaryLoad.begin(); itLoad != PrimaryLoad.end(); ++itLoad)
        {
            primaryEntries.push_back(Reliability::LoadBalancingComponent::LoadMetric(wstring(itLoad->first), itLoad->second));
        }
        desc.MergeLoads(Reliability::LoadBalancingComponent::ReplicaRole::Primary, move(primaryEntries), now);
    }

    auto &secondaryLoad = IsStateful ? SecondaryLoad : InstanceLoad;

    for (auto itLoad = secondaryLoad.begin(); itLoad != secondaryLoad.end(); ++itLoad)
    {
        secondaryEntries.push_back(Reliability::LoadBalancingComponent::LoadMetric(wstring(itLoad->first), itLoad->second));
    }

    desc.MergeLoads(Reliability::LoadBalancingComponent::ReplicaRole::Secondary, move(secondaryEntries), now);

    return desc;
}

void FailoverUnit::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6}",
        index_,
        serviceIndex_,
        targetReplicaCount_,
        isStateful_,
        primaryLoad_,
        secondaryLoad_,
        replicas_);
}

// private members

vector<FailoverUnit::Replica>::iterator FailoverUnit::GetReplica(int nodeIndex)
{
    vector<FailoverUnit::Replica>::iterator itRet = replicas_.end();

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->NodeIndex == nodeIndex)
        {
            itRet = it;
            break;
        }
    }

    return itRet;
}

bool FailoverUnit::ContainsReplica(int nodeIndex) const
{
    bool ret = false;

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->NodeIndex == nodeIndex)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

vector<FailoverUnit::Replica>::iterator FailoverUnit::GetPrimary()
{
    vector<FailoverUnit::Replica>::iterator itRet = replicas_.end();

    ASSERT_IFNOT(IsStateful, "Error");

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->Role == 0)
        {
            itRet = it;
            break;
        }
    }

    return itRet;
}
