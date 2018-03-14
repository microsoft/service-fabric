// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class TicketTransferBody;

    class TicketTransfer : public Serialization::FabricSerializable
    {
    public:
        TicketTransfer()
        {
        }

        TicketTransfer(NodeId voteId,
            Common::StopwatchTime superTicket,
            Common::StopwatchTime globalTicket,
            Common::TimeSpan delta)
            :   voteId_(voteId),
                superTicket_(superTicket),
                globalTicket_(globalTicket),
                delta_(delta),
                baseTime_(Common::Stopwatch::Now())
        {
        }

        __declspec (property(get=getVoteId)) NodeId VoteId;
        NodeId getVoteId() const
        {
            return voteId_;
        }

        __declspec (property(get=getSuperTicket)) Common::StopwatchTime SuperTicket;
        Common::StopwatchTime getSuperTicket() const
        {
            return superTicket_;
        }

        __declspec (property(get=getGlobalTicket)) Common::StopwatchTime GlobalTicket;
        Common::StopwatchTime getGlobalTicket() const
        {
            return globalTicket_;
        }

        void AdjustTime()
        {
            superTicket_ = Common::StopwatchTime::GetLowerBound(superTicket_, delta_);
            globalTicket_ = Common::StopwatchTime::GetUpperBound(globalTicket_, baseTime_);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        { 
            w.Write("{0}:{1}/{2}", voteId_, superTicket_, globalTicket_);
        }

        FABRIC_FIELDS_05(voteId_, superTicket_, globalTicket_, delta_, baseTime_);

    private:
        NodeId voteId_;
        Common::StopwatchTime superTicket_;
        Common::StopwatchTime globalTicket_;
        Common::TimeSpan delta_;
        Common::StopwatchTime baseTime_;

        friend class TicketTransferBody;
    };

    class TicketTransferBody : public Serialization::FabricSerializable
    {
    public:

        TicketTransferBody()
        {
        }

        TicketTransferBody(std::vector<TicketTransfer> && transfers)
            : transfers_(std::move(transfers))
        {
        }

        static bool ReadFromMessage(Transport::Message & message, std::vector<TicketTransfer> & tickets)
        {
            TicketTransferBody body;
            if (!message.GetBody<TicketTransferBody>(body))
            {
                return false;
            }

            tickets = std::move(body.transfers_);

            for (auto it = tickets.begin(); it != tickets.end(); it++)
            {
                it->AdjustTime();
            }

            return true;
        }

        FABRIC_FIELDS_01(transfers_);

    private:
        std::vector<TicketTransfer> transfers_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::TicketTransfer);
