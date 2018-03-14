// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents a range of node ids.
    struct NodeIdRange
    {
    public:
        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Constructs an empty range.
        //
        // Arguments:
        //
        NodeIdRange();

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Takes a pair of node ids and represents a range between them.
        //
        // Arguments:
        //      begin - the begining of the range.
        //      end - the ending of the range.
        //
        NodeIdRange(NodeId const& begin, NodeId const& end);

        //---------------------------------------------------------------------
        // Description:
        //      Copy constructor.
        //
        // Arguments:
        //      range
        //
        NodeIdRange(NodeIdRange const & range)
            : begin_(range.begin_), end_(range.end_)
        {
        }

        //---------------------------------------------------------------------
        // Description:
        //      Checks whether the range defined by the argument is a subset
        //      of the range defined by this object. The semantics does not require
        //      argument to be a proper subset.
        //
        // Arguments:
        //      range - the range to check for being a sub range.
        //
        // Returns:
        //      true - if the range provided is a subset of this object.
        //      false - otherwise.
        //
        // Exceptions: (optional)
        //       none (this must be guaranteed)
        //
        bool Contains(NodeIdRange const & range) const;

        //---------------------------------------------------------------------
        // Description:
        //      Returns whether the current range contains the node id passed.
        // Arguments:
        //      id - the nodeid which is to be tested.
        //
        // Returns:
        //      true - if this range contains the node id.
        //      false - otherwise.
        bool Contains(NodeId id) const;

        bool ProperContains(NodeId id) const;

        //---------------------------------------------------------------------
        // Description:
        //      Checks whether this range overlaps with the argument range.
        //
        // Arguments:
        //      range - the range to check for being disjoint.
        //
        // Returns:
        //      true - if this range and the argument range are disjoint.
        //      false - otherwise.
        //
        // Exceptions: (optional)
        //      none (this must be guaranteed)
        //
        bool Disjoint(NodeIdRange const & range) const;

        bool ProperDisjoint(NodeIdRange const & range) const;

        //---------------------------------------------------------------------
        // Description:
        //      Returns whether the token is adjacent to another token on the successor side.
        // Arguments:
        //      range - the other range.
        //
        // Returns:
        //      true - if this range is adjacent with the specified token on the successor side.
        //      false - otherwise.
        //
        // Exceptions: (optional)
        //      none (this must be guaranteed)
        //
        bool IsSuccAdjacent(NodeIdRange const& range) const;

        //---------------------------------------------------------------------
        // Description:
        //      Returns whether the token is adjacent to another token on the predecssor side.
        // Arguments:
        //      range - the other range.
        //
        // Returns:
        //      true - if this range is adjacent with the specified token on the predecssor side.
        //      false - otherwise.
        //
        // Exceptions: (optional)
        //      none (this must be guaranteed)
        //
        bool IsPredAdjacent(NodeIdRange const& range) const;

        static NodeIdRange Merge(NodeIdRange const & range1, NodeIdRange const & range2);

        void Subtract(NodeIdRange const & exclude, NodeIdRange & first, NodeIdRange & second) const;

        void Subtract(std::vector<NodeIdRange> const & excludes, std::vector<NodeIdRange> & result) const;

        static NodeIdRange const Empty;
        static NodeIdRange const Full;

        // Properties .
        __declspec (property(get=getBegin)) NodeId Begin;
        __declspec (property(get=getEnd)) NodeId End;
        __declspec (property(get=getIsFull)) bool IsFull;
        __declspec (property(get=getIsEmpty)) bool IsEmpty;

        // Getter functions for properties.
        NodeId getBegin() const { return begin_; }
        NodeId getEnd() const { return end_; }
        bool getIsFull() const { return (*this == Full); }
        bool getIsEmpty() const { return (*this == Empty); }

        FABRIC_PRIMITIVE_FIELDS_02(begin_, end_);

        // Equality operator.
        bool operator==(NodeIdRange const& value) const
        {
            return begin_ == value.begin_ && end_ == value.end_;
        }

        // Equality operator.
        bool operator!=(NodeIdRange const& value) const
        {
            return begin_ != value.begin_ || end_ != value.end_;
        }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:

        NodeIdRange(bool isFull);

        bool IsNormal() const;

        // The node id representing the begining of the range.
        NodeId begin_;

        // The node id representing the ending of the range.
        NodeId end_;
    };
}

