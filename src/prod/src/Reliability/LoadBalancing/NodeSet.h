// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        class NodeEntry;
        class DynamicBitSet;
        class PlacementReplica;

        // A bit set of nodes within the placement object
        // The index of the node is same to the index in the placement object
        // also same to the node->NodeIndex field
        class NodeSet
        {
            DENY_COPY_ASSIGNMENT(NodeSet);

        public:
            // initialize an empty or complete node set
            NodeSet(Placement const* pl, bool isComplete);
            NodeSet(int NodeCount);

            NodeSet(NodeSet const& other);

            NodeSet(NodeSet && other);

            NodeSet & operator = (NodeSet && other);

            __declspec (property(get=get_Count)) size_t Count;
            size_t get_Count() const { return nodeCount_; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return Count == 0; }

            __declspec (property(get = get_OriginalPlacement)) Placement const* OriginalPlacement;
            Placement const* get_OriginalPlacement() const { return pl_; }

            void Add(NodeEntry const* node);

            void Delete(NodeEntry const* node);
            void DeleteNodeVecWithIndex(std::vector<int> const& nodes);
            void DeleteNodes(DynamicBitSet const& nodesToDelete);

            bool Check(NodeEntry const* node) const;
            void Copy(NodeSet const& Other);

            NodeEntry const* SelectRandom(Common::Random & rand);
            NodeEntry const* SelectHighestNodeID();

            // Filter the contents of the current set and remove those elements for which predicate doesn't return true.
            void Filter(std::function<bool (NodeEntry const *)> predicate);

            // Intersect the current set with another set.
            void Intersect(std::function<void (std::function<void (NodeEntry const *)>)> enumerateOtherSet, size_t otherSetSize = 0);

            // Intersect the current set with another set.
            void Intersect(NodeSet const& other);

            // Intersect without updating node count.
            void NodeSet::SimpleIntersect(NodeSet const& other);

            // Set the content of the node set to be a single node.
            void Set(NodeEntry const *node);

            // Set the content of the node set to be a single node if the current set contains that node
            void CheckAndSet(NodeEntry const *node);

            // Clear all nodes from the node set.
            void Clear();

            // Set the node set to contain all eligible nodes for this replica
            // In case we prefer swaps we should pick existing or SB replicas
            // Else we can pick all possible nodes
            void SelectEligible(PlacementReplica const * replica, bool isSwapPreferred);

            // Set the node set to contain all possible nodes.
            void SelectAll();

            // Uninon with another node set.
            void Union(NodeSet const& other);

            // Union without updating node count
            void SimpleUnion(NodeSet const& other);

            void UpdateNodeCount();
            void UpdateNodeCountVec(std::vector<size_t> & nodeCountVec);

            // do something for each node
            void ForEach(std::function<bool(NodeEntry const *)> processor) const;

            void Add(int nodeIndex)
            {
                ++nodeCount_;
                bitmapPtr_[nodeIndex / BitmapElementSize] |= 1u << (nodeIndex % BitmapElementSize);
            }

            int GetTotalOneCount() const;

        private:
            typedef uint32 BitmapElementType;
            const static int BitmapElementSize = sizeof(BitmapElementType) * 8;

            void InnerClear();

            void InnerSelectAll();

            void Delete(int nodeIndex)
            {
                --nodeCount_;
                bitmapPtr_[nodeIndex / BitmapElementSize] &= ~(1u << (nodeIndex % BitmapElementSize));
            }

            bool Check(int nodeIndex) const
            {
                return (bitmapPtr_[nodeIndex / BitmapElementSize] & 1u << (nodeIndex % BitmapElementSize)) != 0;
            }

            int GetOneCount(BitmapElementType x) const;

            // Reference to placement obect
            Placement const* pl_;
            // Current number of elements in set
            size_t nodeCount_;

            // Size of the set
            int bitmapLength_;
            // List of all the bitmap elements
            std::vector<BitmapElementType> bitmap_;
            // Pointer to the first element of bitmap_
            BitmapElementType * bitmapPtr_;

            bool reCalculateNodeCount_;
        };
    }
}
