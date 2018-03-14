// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

RepartitionInfo::RepartitionInfo()
    : added_()
    , removed_()
{
}

RepartitionInfo::RepartitionInfo(
    RepartitionType::Enum repartitionType,
    map<FailoverUnitId, ConsistencyUnitDescription> && added,
    map<FailoverUnitId, ConsistencyUnitDescription> && removed)
    : repartitionType_(repartitionType)
    , added_(move(added))
    , removed_(move(removed))
{
}

RepartitionInfo::RepartitionInfo(RepartitionInfo && other)
    : repartitionType_(other.repartitionType_)
    , added_(move(other.added_))
    , removed_(move(other.removed_))
{
}

RepartitionInfo & RepartitionInfo::operator=(RepartitionInfo && other)
{
    if (this != &other)
    {
        repartitionType_ = other.repartitionType_;
        added_ = move(other.added_);
        removed_ = move(other.removed_);
    }

    return *this;
}

bool RepartitionInfo::IsRemoved(FailoverUnitId const& failoverUnitId) const
{
    return (removed_.find(failoverUnitId) != removed_.end());
}
