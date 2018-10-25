// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NodeSet.h"
#include "Placement.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

NodeSet::NodeSet(Placement const* pl, bool isComplete)
    : pl_(pl),
      bitmapLength_(static_cast<int>((pl->NodeCount + BitmapElementSize - 1) / BitmapElementSize)),
      bitmap_(bitmapLength_),
      bitmapPtr_(bitmap_.data()),
      reCalculateNodeCount_(false)
{
    if (isComplete)
    {
        InnerSelectAll();
    }
    else
    {
        InnerClear();
    }
}

NodeSet::NodeSet(int NodeCount)
: pl_(nullptr),
bitmapLength_(static_cast<int>((NodeCount + BitmapElementSize - 1) / BitmapElementSize)),
bitmap_(bitmapLength_),
bitmapPtr_(bitmap_.data()),
reCalculateNodeCount_(false)
{
    InnerClear();
}

NodeSet::NodeSet(NodeSet const& other)
    : pl_(other.pl_),
      nodeCount_(other.nodeCount_),
      bitmapLength_(other.bitmapLength_),
      bitmap_(other.bitmap_),
      bitmapPtr_(bitmap_.data()),
      reCalculateNodeCount_(other.reCalculateNodeCount_)
{
}

NodeSet::NodeSet(NodeSet && other)
    : pl_(other.pl_),
      nodeCount_(other.nodeCount_),
      bitmapLength_(other.bitmapLength_),
      bitmap_(move(other.bitmap_)),
      bitmapPtr_(other.bitmapPtr_),
    reCalculateNodeCount_(other.reCalculateNodeCount_)
{
}

NodeSet & NodeSet::operator = (NodeSet && other)
{
    if (this != &other)
    {
        pl_ = other.pl_;
        nodeCount_ = other.nodeCount_;
        bitmapLength_ = other.bitmapLength_;
        bitmap_ = move(other.bitmap_);
        bitmapPtr_ = other.bitmapPtr_;
        reCalculateNodeCount_ = other.reCalculateNodeCount_;
    }

    return *this;
}

void NodeSet::Copy(NodeSet const& other)
{
    pl_ = other.pl_;
    nodeCount_ = other.nodeCount_;
    bitmapLength_= other.bitmapLength_;
    bitmap_ = other.bitmap_;
    bitmapPtr_ = bitmap_.data();
    reCalculateNodeCount_ = other.reCalculateNodeCount_;
}

/// <summary>
/// Adds the specified node entry into the set at position of node index.
/// </summary>
/// <param name="node">The node.</param>
void NodeSet::Add(NodeEntry const* node)
{
    ASSERT_IF(node == nullptr, "NodeEntry to be added is empty");
    if (!Check(node->NodeIndex))
    {
        Add(node->NodeIndex);
    }
}

/// <summary>
/// Deletes the specified node node entry from the set at position of node index.
/// </summary>
/// <param name="node">The node.</param>
void NodeSet::Delete(NodeEntry const* node)
{
    ASSERT_IF(node == nullptr, "NodeEntry to be deleted is empty");
    if (Check(node->NodeIndex))
    {
        Delete(node->NodeIndex);
    }
}

/// <summary>
/// Deletes the entries from the set specified by vector parameter
/// </summary>
/// <param name="nodes">The vector of indexes to be removed from the set.</param>
void NodeSet::DeleteNodeVecWithIndex(vector<int> const& nodes)
{
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        if (Check(*it))
        {
            Delete(*it);
        }
    }
}

/// <summary>
/// Deletes the entries from the set specified by DynamicBitSet parameter
/// </summary>
/// <param name="nodesToDelete">DynamicBitSet of indexes to be removed from the set.</param>
void NodeSet::DeleteNodes(DynamicBitSet const& nodesToDelete)
{
    nodesToDelete.ForEach([&](size_t nodeIndex) -> void
    {
        if (Check(static_cast<int>(nodeIndex)))
        {
            Delete(static_cast<int>(nodeIndex));
        }
    });
}

/// <summary>
/// Checks if the set is occupied on node index position
/// </summary>
/// <param name="node">The node.</param>
/// <returns>Returns whether the node index postion in set is occupied or not</returns>
bool NodeSet::Check(NodeEntry const* node) const
{
    ASSERT_IF(node == nullptr, "NodeEntry to check is empty");
    return Check(node->NodeIndex);
}

/// <summary>
/// Selects randomly one NodeEntry from the NodeSet
/// </summary>
/// <param name="rand">The Random object.</param>
/// <returns>Return one randomly selected NodeEntry from the set </returns>
NodeEntry const* NodeSet::SelectRandom(Common::Random & rand)
{
    vector<size_t> nodeCountVec(bitmapLength_);
    UpdateNodeCountVec(nodeCountVec);

    if (nodeCount_ == 0)
    {
        return nullptr;
    }

    int randIndex = rand.Next(static_cast<int>(nodeCount_));
    int nodeIndex = 0;

    for (int i = 0; i < bitmapLength_; i++)
    {
        BitmapElementType x = bitmapPtr_[i];

        int count = static_cast<int>(nodeCountVec[i]);
        if (count <= randIndex)
        {
            randIndex -= count;
            nodeIndex += BitmapElementSize;
        }
        else
        {
            // the target must be in this block
            for (int j = 0; j < BitmapElementSize; j++)
            {
                if ((x & 1 << j) != 0 && randIndex-- == 0)
                {
                     return &pl_->SelectNode(nodeIndex);
                }

                ++nodeIndex;
            }

            Assert::CodingError("The target should have been found in the current block");
        }
    }

    Assert::CodingError("We should have found the target in the NodeSet");
}

/// <summary>
/// Selects the NodeEntry with highest node index from the NodeSet
/// </summary>
/// <returns>Returns the NodeEntry with the highest node index that occupies the NodeSet</returns>
NodeEntry const* NodeSet::SelectHighestNodeID()
{
    if (reCalculateNodeCount_)
    {
        UpdateNodeCount();
    }

    if (nodeCount_ == 0)
    {
        return nullptr;
    }
    Common::LargeInteger highestID = LargeInteger::Zero;
    int highestIDIndex = -1;

    int nodeIndex = bitmapLength_ * BitmapElementSize-1;

    for (int i = bitmapLength_-1; i >= 0; i--)
    {
        BitmapElementType x = bitmapPtr_[i];

        for (int j = BitmapElementSize-1; j >= 0 ; j--)
        {
            if ((x & 1 << j) != 0 )
            {
                if ((&pl_->SelectNode(nodeIndex))->NodeId.IdValue >= highestID)
                {
                    highestIDIndex=nodeIndex;
                    highestID = (&pl_->SelectNode(nodeIndex))->NodeId.IdValue;
                }
            }
            --nodeIndex;
        }

    }
    if (highestIDIndex > -1)
    {
        return (&pl_->SelectNode(highestIDIndex));
    }
    else
    {
        Assert::CodingError("We should have found the highest NodeID target in the NodeSet");
    }
}

/// <summary>
/// Filter the contents of the current set and remove those elements for which predicate doesn't return true.
/// </summary>
/// <param name="predicate">The predicate function.</param>
void NodeSet::Filter(function<bool(NodeEntry const *)> predicate)
{
    int totalNodeCount = static_cast<int>(pl_->NodeCount);
    for (int i = 0; i < totalNodeCount; i++)
    {
        if (Check(i))
        {
            if (!predicate(&pl_->SelectNode(i)))
            {
                Delete(i);
            }
        }
    }
}

/// <summary>
/// Intersect the current set with another set.
/// </summary>
/// <param name="enumerateOtherSet">The other set.</param>
/// <param name="otherSetSize">The size of the other set.</param>
void NodeSet::Intersect(function<void (function<void (NodeEntry const *)>)> enumerateOtherSet, size_t otherSetSize)
{
    vector<NodeEntry const *> result;
    result.reserve(otherSetSize);
    enumerateOtherSet([&](NodeEntry const *node)
    {
        if (Check(node->NodeIndex))
        {
            result.push_back(node);
        }
    });

    Clear();
    for (auto it = result.begin(); it != result.end(); it++)
    {
        if (!Check((*it)->NodeIndex))
        {
            Add((*it)->NodeIndex);
        }
    }
}

/// <summary>
/// Intersect the current set with another set.
/// </summary>
/// <param name="other">The other set.</param>
void NodeSet::Intersect(NodeSet const& other)
{
    ASSERT_IF(bitmapLength_ != other.bitmapLength_, "The two NodeSets to be intersected must have the same size.");

    nodeCount_ = 0;
    for (int i = 0; i < bitmapLength_; i++)
    {
        bitmapPtr_[i] &= other.bitmapPtr_[i];
        nodeCount_ += GetOneCount(bitmapPtr_[i]);
    }
}

/// <summary>
/// Intersect the current set with another set without updating node count.
/// </summary>
/// <param name="other">The other set.</param>
void NodeSet::SimpleIntersect(NodeSet const& other)
{
    ASSERT_IF(bitmapLength_ != other.bitmapLength_, "The two NodeSets to be intersected must have the same size.");

    for (int i = 0; i < bitmapLength_; i++)
    {
        bitmapPtr_[i] &= other.bitmapPtr_[i];
    }

    reCalculateNodeCount_ = true;
}

/// <summary>
/// Set the content of the node set to be a single node.
/// </summary>
/// <param name="targetNode">The target node.</param>
void NodeSet::Set(NodeEntry const *targetNode)
{
    Clear();
    Add(targetNode->NodeIndex);
}

/// <summary>
/// Set the content of the node set to be a single node if the current set contains that node
/// </summary>
/// <param name="targetNode">The target node.</param>
void NodeSet::CheckAndSet(NodeEntry const *targetNode)
{
    int index = targetNode->NodeIndex;
    bool contain = Check(index);

    Clear();

    if (contain)
    {
        Add(index);
    }
}

/// <summary>
/// Clear all nodes from the node set.
/// </summary>
void NodeSet::Clear()
{
    // If reCalc is needed or nodeCount is not 0, just do a memset
    if (reCalculateNodeCount_ || (!reCalculateNodeCount_ && nodeCount_ != 0))
    {
        InnerClear();
    }
}

void NodeSet::SelectEligible(PlacementReplica const * replica, bool isSwapPreferred)
{
    auto partition = replica->Partition;
    //for singletons or stateless replicas we cannot really do swaps so allow all kinds of moves
    if (isSwapPreferred && partition->Service->IsStateful && !partition->IsTargetOne)
    {
        Clear();
        partition->ForEachExistingReplica([&](PlacementReplica const* r)
        {
            if (r->Role != replica->Role)
            {
                this->Add(r->Node);
            }
        }, false, false);
        //SB should be fine as well
        partition->ForEachStandbyReplica([&](PlacementReplica const* r)
        {
            this->Add(r->Node);
        });
    }
    else
    {
        this->SelectAll();
    }
}

/// <summary>
/// Set the node set to contain all possible nodes.
/// </summary>
void NodeSet::SelectAll()
{
    nodeCount_ = pl_->EligibleNodes.nodeCount_;
    bitmapLength_ = pl_->EligibleNodes.bitmapLength_;
    bitmap_ = pl_->EligibleNodes.bitmap_;
    bitmapPtr_ = bitmap_.data();

    reCalculateNodeCount_ = false;
}

/// <summary>
/// Gets the count of ones in BitmapElementType argument
/// </summary>
/// <param name="x">The bitmap element.</param>
/// <returns>Returns the number of ones</returns>
inline int NodeSet::GetOneCount(BitmapElementType x) const
{
    if (x == -1)
    {
        return BitmapElementSize;
    }
    else
    {
        int count = 0;
        while (x > 0)
        {
            ++count;
            x &= x - 1;
        }

        return count;
    }
}

/// <summary>
/// Gets the total number of ones in a set.
/// </summary>
/// <returns>Returns total number of ones in set</returns>
int NodeSet::GetTotalOneCount() const
{
    int nodeCount = 0;
    for (int i = 0; i < bitmapLength_; i++)
    {
        nodeCount += GetOneCount(bitmapPtr_[i]);
    }

    return nodeCount;
}

/// <summary>
/// Uninon with another node set.
/// </summary>
/// <param name="other">The other NodeSet.</param>
void NodeSet::Union(NodeSet const& other)
{
    ASSERT_IF(bitmapLength_ != other.bitmapLength_, "The two NodeSets to be unioned must have the same size.");

    nodeCount_ = 0;
    for (int i = 0; i < bitmapLength_; i++)
    {
        bitmapPtr_[i] |= other.bitmapPtr_[i];
        nodeCount_ += GetOneCount(bitmapPtr_[i]);
    }
}

/// <summary>
/// Uninon with another node set without updating the node count.
/// </summary>
void NodeSet::SimpleUnion(NodeSet const& other)
{
    ASSERT_IF(bitmapLength_ != other.bitmapLength_, "The two NodeSets to be unioned must have the same size.");

    for (int i = 0; i < bitmapLength_; i++)
    {
        bitmapPtr_[i] |= other.bitmapPtr_[i];
    }

    reCalculateNodeCount_ = true;

}

void NodeSet::UpdateNodeCount()
{
    nodeCount_ = GetTotalOneCount();

    reCalculateNodeCount_ = false;
}

void NodeSet::UpdateNodeCountVec(vector<size_t> & nodeCountVec)
{
    size_t nodeCount = 0;
    for (int i = 0; i < bitmapLength_; i++)
    {
        nodeCountVec[i] = GetOneCount(bitmapPtr_[i]);
        nodeCount += nodeCountVec[i];
    }

    nodeCount_ = nodeCount;
    reCalculateNodeCount_ = false;
}


/// <summary>
/// Do something for each node
/// </summary>
/// <param name="processor">The processor function.</param>
void NodeSet::ForEach(std::function<bool(NodeEntry const *)> processor) const
{
    int totalNodeCount = static_cast<int>(pl_->NodeCount);
    for (int i = 0; i < totalNodeCount; i++)
    {
        if (Check(i))
        {
            if (!processor(&pl_->SelectNode(i)))
            {
                break;
            }
        }
    }
}

/// <summary>
/// Clears the set
/// </summary>
void NodeSet::InnerClear()
{
    nodeCount_ = 0;
    memset(bitmapPtr_, 0, bitmapLength_ * (BitmapElementSize / 8));

    reCalculateNodeCount_ = false;
}

/// <summary>
/// Set the NodeSet to be full occupied
/// </summary>
void NodeSet::InnerSelectAll()
{
    nodeCount_ = pl_->NodeCount;
    memset(bitmapPtr_, -1, bitmapLength_ * (BitmapElementSize / 8));

    // clear the trailing bits that don't belong to the NodeSet
    int lastElementBits = static_cast<int>(pl_->NodeCount % BitmapElementSize);
    if (lastElementBits > 0)
    {
        bitmapPtr_[bitmapLength_ - 1] &= (1u << lastElementBits) - 1;
    }

    reCalculateNodeCount_ = false;
}
