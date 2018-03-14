// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ReplicaPlacement.h"
#include "PlacementReplica.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ReplicaPlacement::ReplicaPlacement()
    : replicaCount_(0), replicaPlacement_()
{
}

ReplicaPlacement::ReplicaPlacement(ReplicaPlacement && other)
    : replicaCount_(other.replicaCount_), replicaPlacement_(move(other.replicaPlacement_))
{
}

ReplicaPlacement::ReplicaPlacement(ReplicaPlacement const& other)
    : replicaCount_(other.replicaCount_), replicaPlacement_(other.replicaPlacement_)
{
}

ReplicaPlacement & ReplicaPlacement::operator = (ReplicaPlacement && other)
{
    if (this != &other)
    {
        replicaCount_ = other.replicaCount_;
        replicaPlacement_ = move(other.replicaPlacement_);
    }

    return *this;
}

void ReplicaPlacement::Add(NodeEntry const* node, PlacementReplica const* replica)
{
    if (replica != nullptr && node != nullptr)
    {
        auto nodeIt = replicaPlacement_.find(node);

        if (nodeIt == replicaPlacement_.end())
        {
            nodeIt = replicaPlacement_.insert(make_pair(node, PlacementReplicaSet())).first;
        }

        nodeIt->second.insert(replica);
        ++replicaCount_;
    }
}

void ReplicaPlacement::Delete(NodeEntry const* node, PlacementReplica const* replica)
{
    if (replica != nullptr && node != nullptr)
    {
        auto nodeIt = replicaPlacement_.find(node);
        ASSERT_IF(nodeIt == replicaPlacement_.end(), "Node should exist when deleting a replica");
        nodeIt->second.erase(replica);

        if (nodeIt->second.empty())
        {
            replicaPlacement_.erase(nodeIt);
        }

        ASSERT_IFNOT(replicaCount_>0, "Replica count {0} should be > 0", replicaCount_);
        --replicaCount_;
    }
}

void ReplicaPlacement::Clear()
{
    replicaCount_ = 0;
    replicaPlacement_.clear();
}

bool ReplicaPlacement::HasReplicaOnNode(
    NodeEntry const* node,
    bool includePrimary,
    bool includeSecondary,
    PlacementReplica const* replicaToExclude) const
{
    ASSERT_IF(node == nullptr, "Node should not be null when checking replicas on node");
    ASSERT_IFNOT(includePrimary || includeSecondary, "One of includePrimary or includeSecondary should be true");

    bool ret = false;

    auto it = replicaPlacement_.find(node);
    if (it != replicaPlacement_.end())
    {
        for (auto itReplica = it->second.begin(); itReplica != it->second.end(); ++itReplica)
        {
            if (replicaToExclude != nullptr && (*itReplica) == replicaToExclude)
            {
                continue;
            }

            if (!(*itReplica)->ShouldDisappear &&
                (includePrimary && (*itReplica)->IsPrimary ||
                 includeSecondary && (*itReplica)->IsSecondary))
            {
                ret = true;
                break;
            }
        }
    }

    return ret;
}
