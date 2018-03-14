// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;

NodeIdRangeTable::NodeIdRangeTable()
{
    this->rangesNotReceived_.push_back(NodeIdRange::Full);
}

NodeIdRangeTable::~NodeIdRangeTable()
{
}

void NodeIdRangeTable::AddRange(NodeIdRange const & range)
{
    // Full and Empty are "special" values and must be dealt with
    if (range == NodeIdRange::Full)
    {
        this->rangesNotReceived_.clear();
        return;
    }

    if (range == NodeIdRange::Empty)
    {
        return;
    }

    if (range.Begin.IdValue > range.End.IdValue)
    {
        this->InternalAddRange(range.Begin, NodeId::MaxNodeId);
        this->InternalAddRange(NodeId::MinNodeId, range.End);
    }
    else
    {
        this->InternalAddRange(range.Begin, range.End);
    }
}

bool NodeIdRangeTable::get_AreAllRangesReceived() const
{
    return this->rangesNotReceived_.size() == 0;
}

void NodeIdRangeTable::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(L"[ ");

    for (auto range : this->rangesNotReceived_)
    {
        w.Write(range);
        w.Write(L" ");
    }

    w.Write(L"]");
}

void NodeIdRangeTable::InternalAddRange(NodeId const & rangeStart, NodeId const & rangeEnd)
{
    vector<NodeIdRange> temp;

    for (auto range : this->rangesNotReceived_)
    {
        if (rangeEnd.IdValue < range.Begin.IdValue || rangeStart.IdValue > range.End.IdValue)
        {
            temp.push_back(range);
        }
        else
        {
            if (rangeStart.IdValue > range.Begin.IdValue)
            {
                temp.push_back(NodeIdRange(range.Begin, rangeStart - LargeInteger::One));
            }

            if (rangeEnd.IdValue < range.End.IdValue)
            {
                temp.push_back(NodeIdRange(rangeEnd + LargeInteger::One, range.End));
            }
        }
    }

    this->rangesNotReceived_.clear();
    this->rangesNotReceived_.assign(temp.begin(), temp.end());
}
