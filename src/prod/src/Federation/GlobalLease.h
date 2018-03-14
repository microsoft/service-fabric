// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteTicket.h"

namespace Federation
{
    class GlobalLease : public Serialization::FabricSerializable
    {
    public:
        GlobalLease()
        {
        }

        GlobalLease(std::vector<VoteTicket> const & tickets, Common::TimeSpan delta)
            :   tickets_(tickets),
                delta_(delta),
                baseTime_(Common::Stopwatch::Now())
        {
        }

        GlobalLease(std::vector<VoteTicket> && tickets, Common::TimeSpan delta)
            :   tickets_(std::move(tickets)),
                delta_(delta),
                baseTime_(Common::Stopwatch::Now())
        {
        }

        __declspec (property(get=getTickets)) std::vector<VoteTicket> const & Tickets;
        std::vector<VoteTicket> const & getTickets() const
        {
            return tickets_;
        }

        __declspec (property(get=getBaseTime)) Common::StopwatchTime BaseTime;
        Common::StopwatchTime getBaseTime() const
        {
            return baseTime_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        {
            w.Write(tickets_);
        }

        FABRIC_FIELDS_03(tickets_, delta_, baseTime_);

        void AdjustTime()
        {
            for (auto it = tickets_.begin(); it != tickets_.end(); it++)
            {
                it->AdjustTime(delta_, baseTime_);
            }
        }

    private:
        std::vector<VoteTicket> tickets_;
        Common::TimeSpan delta_;
        Common::StopwatchTime baseTime_;
    };
}
