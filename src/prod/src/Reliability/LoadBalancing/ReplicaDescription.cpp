// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ReplicaDescription.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::LoadBalancingComponent;

ReplicaDescription::ReplicaDescription(
    Federation::NodeInstance nodeInstance,
    ReplicaRole::Enum role,
    ReplicaStates::Enum state,
    bool isUp,
    Reliability::ReplicaFlags::Enum flags)
    : nodeInstance_(nodeInstance),
    role_(role),
    state_(state),
    isUp_(isUp),
    flags_(flags),
    verifyReplicaLoad_()
{
    // PLB should not interact with inTransition replicas
    isInTransition_ = IsInBuild || IsStandBy || IsOffline || ShouldDisappear || IsToBePromoted;

    // PLB should not interact with any replica of inTransition partition and
    // it should discard it's balancing operations if partition becomes inTransition during the balancing run
    makesFuInTransition_ = IsUp && !IsStandBy && (IsInBuild || IsToBePromoted);
}

ReplicaDescription::ReplicaDescription(ReplicaDescription const & other)
    : nodeInstance_(other.nodeInstance_),
    role_(other.role_),
    state_(other.state_),
    isUp_(other.isUp_),
    isInTransition_(other.isInTransition_),
    makesFuInTransition_(other.makesFuInTransition_),
    flags_(other.flags_),
    verifyReplicaLoad_(other.verifyReplicaLoad_)
{
}

ReplicaDescription::ReplicaDescription(ReplicaDescription && other)
    : nodeInstance_(other.nodeInstance_),
    role_(other.role_),
    state_(other.state_),
    isUp_(other.isUp_),
    isInTransition_(other.isInTransition_),
    makesFuInTransition_(other.makesFuInTransition_),
    flags_(other.flags_),
    verifyReplicaLoad_(move(other.verifyReplicaLoad_))
{
}

ReplicaDescription & ReplicaDescription::operator = (ReplicaDescription && other)
{
    if (this != &other)
    {
        nodeInstance_ = other.nodeInstance_;
        role_ = other.role_;
        state_ = other.state_;
        isUp_ = other.isUp_;
        isInTransition_ = other.isInTransition_;
        makesFuInTransition_ = other.makesFuInTransition_;
        flags_ = other.flags_;
        verifyReplicaLoad_ = move(other.verifyReplicaLoad_);
    }

    return *this;
}

bool ReplicaDescription::UsePrimaryLoad() const
{
    return role_ == ReplicaRole::Primary;
}

bool ReplicaDescription::UseSecondaryLoad() const
{
    return role_ == ReplicaRole::Secondary || role_ == ReplicaRole::StandBy;
}

bool ReplicaDescription::UseNoneLoad(bool isRGLoad) const
{
    return role_ == ReplicaRole::None && isRGLoad;
}

bool ReplicaDescription::HasLoad() const
{
    return (UsePrimaryLoad() || UseSecondaryLoad());
}

void ReplicaDescription::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0} {1}", nodeInstance_, role_);

    if (!IsReady)
    {
        writer.Write(" {0}", state_);
    }
    if (HasAnyFlag)
    {
        writer.Write(" {0}", flags_);
    }
    if (!isUp_)
    {
        writer.Write(" Down");
    }
    if (isInTransition_)
    {
        writer.Write(" InTransition");

        if (!makesFuInTransition_)
        {
            writer.Write(" FuNotInTransition");
        }
    }
}

bool ReplicaDescription::CouldCarryReservationOrScaleout() const
{
    return !ShouldDisappear && role_ != ReplicaRole::Enum::Dropped && role_ != ReplicaRole::Enum::None;
}
