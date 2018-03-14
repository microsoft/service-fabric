// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBFailoverUnitDescriptionFlags.h"

Reliability::LoadBalancingComponent::PLBFailoverUnitDescriptionFlags::Flags::Flags(
    bool deleted,
    bool inTransition,
    bool quorumLost,
    bool reconfigurationInProgress,
    bool inTransitionReplica,
    bool inUpgradeReplica,
    bool toBePromotedReplica)
    : value_(0)
{
    if (deleted)
    {
        value_ |= Deleted;
    }

    if (inTransition)
    {
        value_ |= InTransition;
    }

    if (quorumLost)
    {
        value_ |= QuorumLost;
    }

    if (reconfigurationInProgress)
    {
        value_ |= ReconfigurationInProgress;
    }

    if (inTransitionReplica)
    {
        value_ |= InTransitionReplica;
    }

    if (inUpgradeReplica)
    {
        value_ |= InUpgradeReplica;
    }

    if (toBePromotedReplica)
    {
        value_ |= ToBePromotedReplica;
    }
}

void Reliability::LoadBalancingComponent::PLBFailoverUnitDescriptionFlags::Flags::WriteTo(
    Common::TextWriter& writer,
    Common::FormatOptions const &) const
{
    if (value_ == None)
    {
        writer.Write("-");
        return;
    }

    if (IsDeleted())
    {
        writer.Write("D");
    }

    if (IsInTransition())
    {
        writer.Write("T");
    }

    if (IsInQuorumLost())
    {
        writer.Write("Q");
    }

    if (IsReconfigurationInProgress())
    {
        writer.Write("C");
    }

    if (HasInTransitionReplica())
    {
        writer.Write("R");
    }

    if (HasInUpgradeReplica())
    {
        writer.Write("U");
    }

    if (HasToBePromotedReplica())
    {
        writer.Write("P");
    }
}
