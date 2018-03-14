// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ReservationLoad.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

wstring const ReservationLoad::FormatHeader = L"{ReservationLoad:Load}";


ReservationLoad::ReservationLoad()
    : nodeReservedLoad_(),
    totalReservedLoadUsed_()
{
}

ReservationLoad::ReservationLoad(ReservationLoad && other)
    : nodeReservedLoad_(move(other.nodeReservedLoad_)),
    totalReservedLoadUsed_(move(other.totalReservedLoadUsed_))
{
}

void ReservationLoad::AddNodeReservedLoadForAllApps(Federation::NodeId nodeId, int64 loadChange)
{
    int64 checkLoad = loadChange;
    auto it = nodeReservedLoad_.find(nodeId);
    if (it != nodeReservedLoad_.end())
    {
        it->second += loadChange;
        checkLoad = it->second;
    }
    else
    {
        nodeReservedLoad_.insert(make_pair(nodeId, loadChange));
    }
    ASSERT_IF(checkLoad < 0, "Existing reserved load {0} is less than the updated load {1} when updating reserved load", it->second, loadChange);
}

void ReservationLoad::DeleteNodeReservedLoadForAllApps(Federation::NodeId nodeId, int64 loadChange)
{
    AddNodeReservedLoadForAllApps(nodeId, -loadChange);
}


int64 ReservationLoad::GetNodeReservedLoadUsed(Federation::NodeId nodeId) const
{
    auto it = nodeReservedLoad_.find(nodeId);
    if (it != nodeReservedLoad_.end())
    {
        return it->second;
    }
    return 0;
}


void ReservationLoad::UpdateTotalReservedLoadUsed(int64 loadChange)
{
    totalReservedLoadUsed_ += loadChange;
}


void ReservationLoad::MergeReservedLoad(ReservationLoad& other)
{
    for (auto itNodeOther = other.NodeReservedLoad.begin(); itNodeOther != other.NodeReservedLoad.end(); ++itNodeOther)
    {
        auto itNode = nodeReservedLoad_.find(itNodeOther->first);
        if (itNode != nodeReservedLoad_.end())
        {
            itNode->second += itNodeOther->second;
        }
        else
        {
            nodeReservedLoad_.insert(make_pair(move(itNodeOther->first), move(itNodeOther->second)));
        }
    }

    // merge total reserved load
    totalReservedLoadUsed_ += other.TotalReservedLoadUsed;
}
