// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/TicketGap.h"

namespace Federation
{
    class VoteTicket : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(VoteTicket)
    
    public:
        VoteTicket()
        {
        }

        // Generate the global lease without adjustment, which can be
        // later used to create global lease with adjustment for each
        // individual receiver
        VoteTicket(NodeId voteId, Common::StopwatchTime expireTime)
            :   voteId_(voteId),
                expireTime_(expireTime)
        {
        }
        
        // Generate receiver-specific ticket
        VoteTicket(VoteTicket const & original, Common::TimeSpan delta)
            :   voteId_(original.voteId_),
                expireTime_(original.expireTime_ - delta)
        {
            for (TicketGap const & gap : original.gaps_)
            {
                gaps_.push_back(TicketGap(gap, delta));
            }
        }

        VoteTicket(VoteTicket && rhs)
            :   voteId_(rhs.voteId_),
                expireTime_(rhs.expireTime_),
                gaps_(std::move(rhs.gaps_))
        {
        }

        __declspec (property(get=getVoteId)) NodeId VoteId;
        NodeId getVoteId() const
        {
            return voteId_;
        }

        __declspec (property(get=getExpireTime)) Common::StopwatchTime ExpireTime;
        Common::StopwatchTime getExpireTime() const
        {
            return expireTime_;
        }

        std::vector<TicketGap> const & GetGaps() const
        {
            return gaps_;
        }

        void AddGap(TicketGap const & gap)
        {
            gaps_.push_back(gap);
        }

        void AdjustTime(Common::TimeSpan delta, Common::StopwatchTime baseTime)
        {
            expireTime_ = Common::StopwatchTime::GetLowerBound(expireTime_, delta);

            for (auto it = gaps_.begin(); it != gaps_.end(); it++)
            {
                it->AdjustTime(delta, baseTime);
            }
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("{0}:{1}", voteId_, expireTime_);
            if (gaps_.size() > 0)
            {
                w.Write(gaps_);
            }
        }

        FABRIC_FIELDS_03(voteId_, expireTime_, gaps_);

    private:
        NodeId voteId_;
        Common::StopwatchTime expireTime_;
        std::vector<TicketGap> gaps_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::VoteTicket);
