// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct JoinThrottleHeader : public Transport::MessageHeader<Transport::MessageHeaderId::JoinThrottle>, public Serialization::FabricSerializable
    {
    public:
        JoinThrottleHeader()
            : sequence_(0), queryNeeded_(true)
        {
        }

        JoinThrottleHeader(int64 sequence, bool queryNeeded)
            : sequence_(sequence), queryNeeded_(queryNeeded)
        {
        }

        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w << sequence_;
        }

        __declspec(property(get = getSequence)) int64 Sequence;
        int64 getSequence() const { return this->sequence_; }

        __declspec(property(get = getQueryNeeded)) bool QueryNeeded;
        bool getQueryNeeded() const { return this->queryNeeded_; }

        FABRIC_FIELDS_02(sequence_, queryNeeded_);

    private:
        int64 sequence_;
        bool queryNeeded_;
    };
}
