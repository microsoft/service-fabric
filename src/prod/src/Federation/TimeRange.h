// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct TimeRange : public Serialization::FabricSerializable
    {
    public:
        TimeRange()
        {
        }

        TimeRange(Common::StopwatchTime begin, Common::StopwatchTime end)
            :   begin_(begin),
                end_(end)
        {
        }

        TimeRange(TimeRange const & rhs)
            :   begin_(rhs.begin_),
                end_(rhs.end_)
        {
        }

        __declspec (property(get=getBegin)) Common::StopwatchTime Begin;
        Common::StopwatchTime getBegin() const
        {
            return begin_; 
        }

        __declspec (property(get=getEnd)) Common::StopwatchTime End;
        Common::StopwatchTime getEnd() const
        {
            return end_; 
        }

        void Merge(TimeRange const & rhs);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write("{0}-{1}", begin_, end_);
        }

        FABRIC_FIELDS_02(begin_, end_);

    private:
        Common::StopwatchTime begin_;
        Common::StopwatchTime end_;
    };
}
