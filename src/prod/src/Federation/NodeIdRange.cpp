// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;

    NodeIdRange const NodeIdRange::Empty(false);
    NodeIdRange const NodeIdRange::Full(true);

    NodeIdRange::NodeIdRange(bool isFull)
        : begin_(isFull ? NodeId::StaticInit_MinNodeId() : NodeId::StaticInit_MinNodeId() - LargeInteger::StaticInit_MaxValue()),
            end_(isFull ? NodeId::StaticInit_MaxNodeId() : NodeId::StaticInit_MinNodeId())
    {
    }

    NodeIdRange::NodeIdRange()
        : begin_(Empty.begin_), end_(Empty.end_)
    {
    }

    NodeIdRange::NodeIdRange(NodeId const & begin, NodeId const & end)
        : begin_(begin), end_(end)
    {
        // Take any explicitly-constructured range literally and convert
        // to full range if needed.  Ideally user does not construct
        // such range but this is going to be risky.
        if (end.SuccDist(begin) == LargeInteger::One)
        {
            begin_ = Full.begin_;
            end_ = Full.end_;
        }
    }

    bool NodeIdRange::IsNormal() const
    {
        return (end_.SuccDist(begin_) != LargeInteger::One);
    }

    bool NodeIdRange::Contains(NodeIdRange const & range) const
    {
        ASSERT_IF(range.IsEmpty, "range is empty");

        if (IsFull)
        {
            return true;
        }

        if (IsEmpty || range.IsFull)
        {
            return false;
        }

        LargeInteger size = begin_.SuccDist(end_);
        LargeInteger dist1 = begin_.SuccDist(range.Begin);
        if (dist1 > size)
        {
            return false;
        }

        LargeInteger dist2 = begin_.SuccDist(range.End);

        return (dist2 <= size && dist2 >= dist1);
    }

    bool NodeIdRange::Contains(NodeId id) const
    {
        if (IsFull)
        {
            return true;
        }

        if (IsEmpty)
        {
            return false;
        }

        LargeInteger size = begin_.SuccDist(end_);
        LargeInteger dist = begin_.SuccDist(id);

        return (dist <= size);
    }

    bool NodeIdRange::ProperContains(NodeId id) const
    {
        return Contains(id) && (id != begin_) && (id != end_);
    }

    bool NodeIdRange::Disjoint(NodeIdRange const & range) const
    {
        ASSERT_IF(IsEmpty || range.IsEmpty, "empty range for Disjoint");

        if (IsFull || range.IsFull)
        {
            return false;
        }

        LargeInteger size = begin_.SuccDist(end_);
        LargeInteger dist1 = begin_.SuccDist(range.Begin);
        if (dist1 <= size)
        {
            return (false);
        }

        LargeInteger dist2 = begin_.SuccDist(range.End);

        return (dist2 > size && dist2 >= dist1);
    }

    bool NodeIdRange::ProperDisjoint(NodeIdRange const & range) const
    {
        return !ProperContains(range.Begin) && !ProperContains(range.End);
    }

    void NodeIdRange::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
    {
        if (IsFull)
        {
            w.Write("full");
        }
        else if (IsEmpty)
        {
            w.Write("empty");
        }
        else
        {
            w.Write("{0}-{1}", begin_, end_);
        }
    }

    bool NodeIdRange::IsSuccAdjacent(NodeIdRange const& range) const
    {
        return (IsNormal() && range.IsNormal() && end_.SuccDist(range.begin_) == LargeInteger::One);
    }

    bool NodeIdRange::IsPredAdjacent(NodeIdRange const& range) const
    {
        return (IsNormal() && range.IsNormal() && range.end_.SuccDist(begin_) == LargeInteger::One);
    }

    NodeIdRange NodeIdRange::Merge(NodeIdRange const & range1, NodeIdRange const & range2)
    {
        if (range1.IsEmpty)
        {
            return range2;
        }

        if (range2.IsEmpty)
        {
            return range1;
        }

        ASSERT_IF(range1.Disjoint(range2), "Range {0} and {1} can't be merged", range1, range2);
        if (range1.Contains(range2))
        {
            return range1;
        }

        if (range2.Contains(range1))
        {
            return range2;
        }

        if (range1.Contains(range2.Begin))
        {
            return (range1.Contains(range2.End) ? NodeIdRange::Full : NodeIdRange(range1.Begin, range2.End));
        }
        else
        {
            return NodeIdRange(range2.Begin, range1.End);
        }
    }

    void NodeIdRange::Subtract(NodeIdRange const & exclude, NodeIdRange & first, NodeIdRange & second) const
    {
        second = Empty;

        if (IsEmpty || exclude.Contains(*this))
        {
            first = Empty;
            return;
        }

        if (exclude.IsEmpty || Disjoint(exclude))
        {
            first = *this;
            return;
        }

        if (exclude.Contains(begin_))
        {
            NodeId end = (exclude.Contains(end_) ? exclude.Begin - LargeInteger::One : end_);
            first = NodeIdRange(exclude.End + LargeInteger::One, end);
        }
        else if (IsFull)
        {
            first = NodeIdRange(exclude.End + LargeInteger::One, exclude.Begin - LargeInteger::One);
        }
        else
        {
            first = NodeIdRange(begin_, exclude.Begin - LargeInteger::One);
            if (!exclude.Contains(end_))
            {
                second = NodeIdRange(exclude.End + LargeInteger::One, end_);
            }
        }
    }

    void NodeIdRange::Subtract(vector<NodeIdRange> const & excludes, vector<NodeIdRange> & result) const
    {
        result.push_back(*this);

        for (NodeIdRange const & exclude : excludes)
        {
            for (size_t i = result.size(); i > 0; i--)
            {
                NodeIdRange first, second;
                result[i - 1].Subtract(exclude, first, second);
                if (first.IsEmpty)
                {
                    result.erase(result.begin() + (i - 1));
                }
                else
                {
                    result[i - 1] = first;
                    if (!second.IsEmpty)
                    {
                        result.push_back(second);
                    }
                }
            }
        }
    }
}
