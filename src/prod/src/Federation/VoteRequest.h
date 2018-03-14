// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class VoteRequest : public Serialization::FabricSerializable
    {
    public:
        VoteRequest()
        {
        }

        VoteRequest(NodeId voteId)
            :   voteId_(voteId),
                requestTime_(Common::Stopwatch::Now())
        {
        }

        __declspec (property(get=getVoteId)) NodeId VoteId;
        NodeId getVoteId() const
        {
            return voteId_;
        }

        __declspec (property(get=getRequestTime)) Common::StopwatchTime RequestTime;
        Common::StopwatchTime getRequestTime() const
        {
            return requestTime_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        { 
            w.Write(voteId_);
        }

        FABRIC_FIELDS_02(voteId_, requestTime_);

    private:
        NodeId voteId_;
        Common::StopwatchTime requestTime_;
    };
}
