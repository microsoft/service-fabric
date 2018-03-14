// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

LoadInfo::LoadInfo()
    : StoreData(),
      loadDescription_(),
      idString_(),
      isPending_(false),
      isUpdating_(false)
{
}

LoadInfo::LoadInfo(LoadBalancingComponent::LoadOrMoveCostDescription && loads)
    : StoreData(PersistenceState::ToBeInserted),
      loadDescription_(move(loads)),
      idString_(),
      isPending_(true),
      isUpdating_(false)
{
}

LoadInfo::LoadInfo(LoadInfo && other)
    : StoreData(other),
      loadDescription_(move(other.loadDescription_)),
      idString_(),
      isPending_(other.isPending_),
      isUpdating_(other.isUpdating_)
{
}

LoadInfo::LoadInfo(LoadInfo const& other)
    : StoreData(other),
      loadDescription_(other.loadDescription_),
      idString_(),
      isPending_(other.isPending_),
      isUpdating_(other.isUpdating_)
{
}

LoadInfo & LoadInfo::operator = (LoadInfo && other)
{
    if (this != &other) 
    {
        PostRead(other.OperationLSN);
        PersistenceState = other.PersistenceState;

        loadDescription_ = move(other.loadDescription_);
        idString_.clear();
        isPending_ = other.isPending_;
        isUpdating_ = other.isUpdating_;
    }

    return *this;
}

wstring const& LoadInfo::GetStoreType()
{
    return FailoverManagerStore::LoadInfoType;
}

wstring const& LoadInfo::GetStoreKey() const
{
    return IdString;
}

wstring const& LoadInfo::get_IdString() const
{
    if (idString_.empty())
    {
        idString_ = loadDescription_.FailoverUnitId.ToString();
    }

    return idString_;
}

void LoadInfo::AdjustTimestamps(TimeSpan diff)
{
    // this is only called after loading data from store
    loadDescription_.AdjustTimestamps(diff);
}

bool LoadInfo::MergeLoads(LoadBalancingComponent::LoadOrMoveCostDescription && loads)
{
    bool changed = loadDescription_.MergeLoads(move(loads));

    return OnUpdate(changed);
}

bool LoadInfo::MarkForDeletion()
{
    if (PersistenceState == PersistenceState::ToBeDeleted ||
        PersistenceState == PersistenceState::ToBeInserted)
    {
        return false;
    }

    PersistenceState =  PersistenceState::ToBeDeleted;
    return OnUpdate(true);
}

LoadBalancingComponent::LoadOrMoveCostDescription LoadInfo::GetPLBLoadOrMoveCostDescription()
{
    return LoadBalancingComponent::LoadOrMoveCostDescription(loadDescription_);
}

bool LoadInfo::OnUpdate(bool changed)
{
    if (changed && !isPending_)
    {
        if (PersistenceState == PersistenceState::NoChange)
        {
            PersistenceState = PersistenceState::ToBeUpdated;
        }

        isPending_ = true;
        return !isUpdating_;
    }

    return false;
}

void LoadInfo::StartPersist()
{
    ASSERT_IFNOT(isPending_ && !isUpdating_,
        "Invalid state {0} when start persistence", *this);

    isPending_ = false;
    isUpdating_ = true;
}

bool LoadInfo::OnPersistCompleted(bool isSuccess, bool isDeleted, LoadBalancingComponent::IPlacementAndLoadBalancing & plb)
{
    ASSERT_IFNOT(isUpdating_,
        "Invalid state {0} when persistence completed", *this);

    isUpdating_ = false;
    isPending_ = isPending_ || !isSuccess;

    if (isSuccess)
    {
        if (!isDeleted)
        {
            plb.UpdateLoadOrMoveCost(GetPLBLoadOrMoveCostDescription());
        }

        if (isPending_)
        {
            PersistenceState = PersistenceState::ToBeUpdated;
        }
    }

    return isPending_;
}

void LoadInfo::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.WriteLine("{0}, isPending:{1}, isUpdating:{2}, persistence state:{3}",
        loadDescription_, isPending_, isUpdating_, PersistenceState);
    writer.Write(" {0}", this->OperationLSN);
}
