// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementAndLoadBalancing.h"
#include "SearchInsight.h"
#include "PartitionEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

SearchInsight::SearchInsight()
    : hasInsight_(false),
    singleReplicaUpgradeInfo_(),
    singleReplicaTargetNode_(),
    singleReplicaNonCreatedMoves_()
{
}

SearchInsight::SearchInsight(SearchInsight && other)
    : hasInsight_(other.hasInsight_),
    singleReplicaUpgradeInfo_(move(other.singleReplicaUpgradeInfo_)),
    singleReplicaTargetNode_(move(other.singleReplicaTargetNode_)),
    singleReplicaNonCreatedMoves_(move(other.singleReplicaNonCreatedMoves_))
{
}

SearchInsight::SearchInsight(SearchInsight const& other)
    : hasInsight_(other.hasInsight_),
    singleReplicaUpgradeInfo_(other.singleReplicaUpgradeInfo_),
    singleReplicaTargetNode_(other.singleReplicaTargetNode_),
    singleReplicaNonCreatedMoves_(other.singleReplicaNonCreatedMoves_)
{
}

SearchInsight::SearchInsight(SearchInsight const* other)
    : hasInsight_(other->hasInsight_),
    singleReplicaUpgradeInfo_(other->singleReplicaUpgradeInfo_),
    singleReplicaTargetNode_(other->singleReplicaTargetNode_),
    singleReplicaNonCreatedMoves_(other->singleReplicaNonCreatedMoves_)
{
}

SearchInsight& SearchInsight::operator= (SearchInsight && other)
{
    if (this != &other)
    {
        hasInsight_ = other.hasInsight_;
        singleReplicaUpgradeInfo_ = move(other.singleReplicaUpgradeInfo_);
        singleReplicaTargetNode_ = move(other.singleReplicaTargetNode_),
        singleReplicaNonCreatedMoves_ = move(other.singleReplicaNonCreatedMoves_);
    }

    return *this;
}

void SearchInsight::Clear()
{
    hasInsight_ = false;
    singleReplicaUpgradeInfo_.clear();
    singleReplicaTargetNode_.clear();
    singleReplicaNonCreatedMoves_.clear();
}

void SearchInsight::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    const int maxNumOfItems = 20;
    int curItem = 0;
    if (singleReplicaUpgradeInfo_.size() > 0)
    {
        writer.Write("Single replica upgrade info:\r\n");
        for (auto itUpgradeInfo : singleReplicaUpgradeInfo_)
        {
            writer.Write("Partition {0} is in single replica upgrade with {1} optimization \r\nRelated replicas are:\r\n",
                itUpgradeInfo.first->PartitionId,
                itUpgradeInfo.first->SingletonReplicaUpgradeOptimization == PartitionEntry::SingletonReplicaUpgradeOptimizationType::CheckAffinityDuringUpgrade ?
                "CheckAffinityDuringUpgrade" :
                "RelaxScaleoutDuringUpgrade");
            for (auto itReplica : itUpgradeInfo.second)
            {
                writer.Write("[{0}]\r\n", *itReplica);
                curItem++;
                if (curItem >= maxNumOfItems)
                {
                    break;
                }
            }

            auto itTargetNode = singleReplicaTargetNode_.find(itUpgradeInfo.first);
            if (itTargetNode == singleReplicaTargetNode_.end())
            {
                writer.Write("[Target node]: There is no a single node which satisfies all constraints\r\n");
            }
            else
            {
                writer.Write("[Target node]: {0}\r\n", itTargetNode->second);
            }

            auto itNonCreatedMoves = singleReplicaNonCreatedMoves_.find(itUpgradeInfo.first);
            if (itNonCreatedMoves != singleReplicaNonCreatedMoves_.end())
            {
                curItem = 0;
                writer.Write("Partial movement is generated, and the issue partitions are:\r\n");
                for (auto itPartitionId : itNonCreatedMoves->second)
                {
                    writer.Write("{0}\r\n", itPartitionId);
                    curItem++;
                    if (curItem >= maxNumOfItems)
                    {
                        break;
                    }
                }
            }
            writer.Write("\r\n\r\n");
        }
    }
}

void SearchInsight::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
