// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FailoverUnitMovement.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

FailoverUnitMovement::FailoverUnitMovement(
    Guid failoverUnitId,
    wstring && serviceName,
    bool isStateful,
    int64 version,
    bool isFuInTransition,
    vector<PLBAction> && actions)
    : failoverUnitId_(failoverUnitId),
    serviceName_(move(serviceName)),
    isStateful_(isStateful),
    version_(version),
    isFuInTransition_(isFuInTransition),
    actions_(move(actions))
{
}

FailoverUnitMovement::FailoverUnitMovement(FailoverUnitMovement && other)
    : failoverUnitId_(move(other.failoverUnitId_)),
    serviceName_(move(other.serviceName_)),
    isStateful_(other.isStateful_),
    version_(other.version_),
    isFuInTransition_(other.isFuInTransition_),
    actions_(move(other.actions_))
{
}

FailoverUnitMovement::FailoverUnitMovement(FailoverUnitMovement const& other)
    : failoverUnitId_(other.failoverUnitId_),
    serviceName_(other.serviceName_),
    isStateful_(other.isStateful_),
    version_(other.version_),
    isFuInTransition_(other.isFuInTransition_),
    actions_(other.actions_)
{
}

FailoverUnitMovement & FailoverUnitMovement::operator = (FailoverUnitMovement && other)
{
    if (this != &other)
    {
        failoverUnitId_ = move(other.failoverUnitId_);
        serviceName_ = move(other.serviceName_);
        isStateful_ = other.isStateful_;
        version_ = other.version_;
        isFuInTransition_ = other.isFuInTransition_;
        actions_ = move(other.actions_);
    }

    return *this;
}

bool FailoverUnitMovement::operator == (FailoverUnitMovement const& other) const
{
    if (failoverUnitId_ != other.failoverUnitId_ ||
        serviceName_ != other.serviceName_ ||
        isStateful_ != other.isStateful_ ||
        version_ != other.version_ ||
        isFuInTransition_ != other.isFuInTransition_ ||
        actions_.size() != other.actions_.size())
    {
        return false;
    }

    for (size_t i = 0; i < actions_.size(); ++i)
    {
        // order sensitive for now
        if (actions_[i] != other.actions_[i])
        {
            return false;
        }
    }

    return true;
}

bool FailoverUnitMovement::operator != (FailoverUnitMovement const& other) const
{
    return !(*this == other);
}

void FailoverUnitMovement::UpdateActions(vector<PLBAction> && actions)
{
    actions_ = move(actions);
}

bool FailoverUnitMovement::ContainCreation()
{
    bool ret = false;
    for (size_t i = 0; i < actions_.size(); ++i)
    {
        if (actions_[i].IsAdd)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

void FailoverUnitMovement::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}:{1}:{2} {3} {4} actions {5}",
        failoverUnitId_, serviceName_, version_, isFuInTransition_, actions_.size(), actions_);
}
