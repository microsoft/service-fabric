// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TimeoutHeader : public Transport::MessageHeader<Transport::MessageHeaderId::Timeout>, public Serialization::FabricSerializable
    {
    public:
        TimeoutHeader();
        explicit TimeoutHeader(Common::TimeSpan const & timeout);
        TimeoutHeader(TimeoutHeader const & other);

        static TimeoutHeader FromMessage(Message &);
        
        __declspec(property(get=get_Timeout, put=put_Timeout)) Common::TimeSpan Timeout;
        inline Common::TimeSpan get_Timeout() const { return timeout_; }
        inline void put_Timeout(Common::TimeSpan value);

        //Get remaining timeout, adjusted with receive time when available
        Common::TimeSpan GetRemainingTime() const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& f) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_01(timeout_);

    private:
        Common::TimeSpan timeout_;
        Common::StopwatchTime localExpiry_ = Common::StopwatchTime::MaxValue;
    };
}

