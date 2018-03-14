// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class NodeIdRangeTable
    {
    public:
        NodeIdRangeTable();

        ~NodeIdRangeTable();

        void AddRange(NodeIdRange const & range);

        __declspec (property(get=get_AreAllRangesReceived)) bool AreAllRangesReceived;

        bool get_AreAllRangesReceived() const;

        std::vector<NodeIdRange>::const_iterator begin() const
        {
            return this->rangesNotReceived_.begin();
        }

        std::vector<NodeIdRange>::const_iterator end() const
        {
            return this->rangesNotReceived_.end();
        }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        std::vector<NodeIdRange> rangesNotReceived_;

        void InternalAddRange(NodeId const & rangeStart, NodeId const & rangeEnd);
    };
}

