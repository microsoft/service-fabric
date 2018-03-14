// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    using namespace std;
    using namespace Common;

    TimeoutHeader::TimeoutHeader() 
        : timeout_(TimeSpan::Zero)
    {
    }

    TimeoutHeader::TimeoutHeader(TimeSpan const & timeout)
        : timeout_(timeout) 
    {
    }

    TimeoutHeader::TimeoutHeader(TimeoutHeader const & other)
        : timeout_(other.Timeout)
    {
    }

    void TimeoutHeader::put_Timeout(Common::TimeSpan value)
    {
        timeout_ = value;
    }

    TimeoutHeader TimeoutHeader::FromMessage(Message & message)
    {
        TimeoutHeader timeoutHeader;
        if (message.Headers.TryReadFirst(timeoutHeader))
        {
            if (message.HasReceiveTime())
            {
                auto expiry = message.ReceiveTime() + timeoutHeader.Timeout;
                if (expiry >= message.ReceiveTime()) //check for overflow
                {
                    timeoutHeader.localExpiry_ = expiry;
                }
            }
        }
        else
        {
            Assert::TestAssert("TimeoutHeader not found {0}", message.MessageId);
        }
        return timeoutHeader;
    }

    TimeSpan TimeoutHeader::GetRemainingTime() const
    {
        if (localExpiry_ == StopwatchTime::MaxValue) return timeout_;

        auto remaining = localExpiry_ - Stopwatch::Now();
        if (remaining >= TimeSpan::Zero) return remaining;
        
        return TimeSpan::Zero;
    }

    void TimeoutHeader::WriteTo(TextWriter& w, FormatOptions const& f) const
    {
        timeout_.WriteTo(w, f);
    }

    wstring TimeoutHeader::ToString() const
    {
        wstring result;
        StringWriter writer(result);
        writer.Write("{0}", timeout_);
        return result;
    }
}
