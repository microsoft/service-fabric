// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FailoverUnitDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

FailoverUnitDescription::FailoverUnitDescription(
    Common::Guid fuId,
    wstring && serviceName,
    int64 version,
    vector<ReplicaDescription> && replicas,
    int replicaDifference,
    Reliability::FailoverUnitFlags::Flags flags,
    bool isReconfigurationInProgress,
    bool isInQuorumLost,
    int targetReplicaSetSize)
    : fuId_(fuId),
    serviceName_(move(serviceName)),
    version_(version),
    primaryReplicaIndex_(-1),
    replicas_(move(replicas)),
    replicaDifference_(replicaDifference),
    serviceId_(0),
    flags_(flags),
    targetReplicaSetSize_(targetReplicaSetSize)
{
    plbFlags_.SetDeleted(false);
    plbFlags_.SetInQuorumLost(isInQuorumLost);
    plbFlags_.SetReconfigurationInProgress(isReconfigurationInProgress);

    Initialize();
}

FailoverUnitDescription::FailoverUnitDescription(Common::Guid fuId, wstring && serviceName, uint64 serviceId)
    : fuId_(fuId),
    serviceName_(move(serviceName)),
    version_(-1),
    primaryReplicaIndex_(-1),
    replicaDifference_(0),
    serviceId_(serviceId),
    flags_(Reliability::FailoverUnitFlags::Flags::None),
    plbFlags_(PLBFailoverUnitDescriptionFlags::Flags::None),
    targetReplicaSetSize_(0)
{
    plbFlags_.SetDeleted(true);
}

FailoverUnitDescription::FailoverUnitDescription(FailoverUnitDescription const & other)
    : fuId_(other.fuId_),
    serviceName_(other.serviceName_),
    version_(other.version_),
    replicas_(other.replicas_),
    primaryReplicaIndex_(other.primaryReplicaIndex_),
    replicaDifference_(other.replicaDifference_),
    serviceId_(other.serviceId_),
    flags_(other.flags_),
    plbFlags_(other.plbFlags_),
    targetReplicaSetSize_(other.targetReplicaSetSize_)
{
}

FailoverUnitDescription::FailoverUnitDescription(FailoverUnitDescription && other)
    : fuId_(other.fuId_),
    serviceName_(move(other.serviceName_)),
    version_(other.version_),
    replicas_(move(other.replicas_)),
    primaryReplicaIndex_(other.primaryReplicaIndex_),
    replicaDifference_(other.replicaDifference_),
    serviceId_(other.serviceId_),
    flags_(other.flags_),
    plbFlags_(other.plbFlags_),
    targetReplicaSetSize_(other.targetReplicaSetSize_)
{
}

FailoverUnitDescription & FailoverUnitDescription::operator = (FailoverUnitDescription && other)
{
    if (this != &other)
    {
        fuId_ = other.fuId_; 
        serviceName_ = move(other.serviceName_);
        version_ = other.version_;
        replicas_ = move(other.replicas_);
        replicaDifference_ = other.replicaDifference_;
        primaryReplicaIndex_ = other.primaryReplicaIndex_;
        serviceId_ = other.serviceId_;
        flags_ = other.flags_;
        plbFlags_ = other.plbFlags_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
    }

    return *this;
}

size_t FailoverUnitDescription::get_MovableReplicaCount() const
{
    return plbFlags_.IsInQuorumLost() ?
        0 :
        count_if(
            replicas_.begin(),
            replicas_.end(),
            [&](ReplicaDescription const& r) { return r.CurrentState != ReplicaRole::None && !r.IsInTransition; });
}

size_t FailoverUnitDescription::get_InBuildReplicaCount() const
{
    return count_if(replicas_.begin(), replicas_.end(), [&](ReplicaDescription const& r) { return r.IsInBuild; });
}

bool FailoverUnitDescription::HasAnyReplicaOnNode(Federation::NodeId nodeId) const
{
    for (auto replicaIt = replicas_.begin(); replicaIt != replicas_.end(); ++replicaIt)
    {
        if (replicaIt->NodeId == nodeId)
        {
            return true;
        }
    }

    return false;
}

bool FailoverUnitDescription::HasReplicaWithRoleOnNode(Federation::NodeId nodeId, ReplicaRole::Enum role) const
{
    for (auto replicaIt = replicas_.begin(); replicaIt != replicas_.end(); ++replicaIt)
    {
        if (replicaIt->NodeId == nodeId && replicaIt->CurrentRole == role)
        {
            return true;
        }
    }

    return false;
}

bool FailoverUnitDescription::HasReplicaWithSecondaryLoadOnNode(Federation::NodeId nodeId) const
{
    for (auto replicaIt = replicas_.begin(); replicaIt != replicas_.end(); ++replicaIt)
    {
        if (replicaIt->NodeId == nodeId && replicaIt->UseSecondaryLoad())
        {
            return true;
        }
    }

    return false;
}

const ReplicaDescription * FailoverUnitDescription::get_PrimaryReplica() const
{
    if (primaryReplicaIndex_ == -1)
    {
        return nullptr;
    }

    return &(replicas_[primaryReplicaIndex_]);
}

void FailoverUnitDescription::Initialize()
{
    primaryReplicaIndex_ = -1;

    for (int index = 0; index < replicas_.size(); ++index)
    {
        ReplicaDescription const& replica = replicas_[index];

        if (replica.CurrentRole == ReplicaRole::Primary)
        {
            primaryReplicaIndex_ = index;
        }

        if (replica.MakesFuInTransition)
        {
            plbFlags_.SetHasInTransitionReplica(true);
        }

        if (replica.IsInUpgrade)
        {
            plbFlags_.SetHasInUpgradeReplica(true);
        }

        if (replica.IsToBePromoted)
        {
            plbFlags_.SetHasToBePromotedReplica(true);
        }
    }

    plbFlags_.SetInTransition(
        flags_.IsToBeDeleted() ||
        flags_.IsUpgrading() ||
        plbFlags_.HasInTransitionReplica() ||
        plbFlags_.IsReconfigurationInProgress() ||
        plbFlags_.IsInQuorumLost());
}

void FailoverUnitDescription::ForEachReplica(std::function<void(ReplicaDescription const &)> processor) const
{
    for_each(replicas_.begin(), replicas_.end(), [&](ReplicaDescription const& replica)
    {
        processor(replica);
    });
}

void FailoverUnitDescription::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write(
        "{0} service:{1} flags:{2} plbFlags:{3} version:{4} replicaDiff:{5} replicas:{6} targetReplicaSetSize:{7}",
        fuId_,
        serviceName_,
        flags_,
        plbFlags_,
        version_,
        replicaDifference_,
        replicas_,
        targetReplicaSetSize_);
}

void FailoverUnitDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
