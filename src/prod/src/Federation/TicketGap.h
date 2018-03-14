// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/TimeRange.h"

namespace Federation
{
    struct TicketGap : public Serialization::FabricSerializable
    {
    public:
        TicketGap()
        {
        }

        TicketGap(NodeIdRange const & range, TimeRange const & interval);

        TicketGap(TicketGap const & original, Common::TimeSpan delta);

        __declspec (property(get=getRange)) NodeIdRange const & Range;
        NodeIdRange const & getRange() const
        {
            return range_; 
        }

        __declspec (property(get=getInterval)) TimeRange const & Interval;
        TimeRange const & getInterval() const
        {
            return interval_; 
        }

        bool IsEffective() const;

        bool IsExpired() const;

        bool Merge(TicketGap const & rhs);

        void AdjustTime(Common::TimeSpan delta, Common::StopwatchTime baseTime);

        void UpdateInterval();

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(range_, interval_);

    private:
        // The end points of the range belong to the recovering nodes and not
        // the part of id space that has global lease disabled.
        NodeIdRange range_;

        // The time range for which a vote is guaranteed not to have global
        // lease for the gap.  Initially the range is empty (end > begin).
        TimeRange interval_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::TicketGap);

