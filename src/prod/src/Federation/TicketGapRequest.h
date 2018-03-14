// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class TicketGapRequest : public Serialization::FabricSerializable
    {
    public:
        TicketGapRequest()
        {
        }

        TicketGapRequest(NodeIdRange const & range)
            :   range_(range),
                requestTime_(Common::Stopwatch::Now())
        {
        }

        __declspec (property(get=getRange)) NodeIdRange const & Range;
        NodeIdRange const & getRange() const
        {
            return range_;
        }

        __declspec (property(get=getRequestTime)) Common::StopwatchTime RequestTime;
        Common::StopwatchTime getRequestTime() const
        {
            return requestTime_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("{0}:{1}", range_, requestTime_);
        }

        FABRIC_FIELDS_02(range_, requestTime_);

    private:
        NodeIdRange range_;
        Common::StopwatchTime requestTime_;
    };
}
