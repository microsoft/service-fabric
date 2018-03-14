// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    GatewayRetryHeader::GatewayRetryHeader() 
        : retryCount_(0)
    {
    }

    GatewayRetryHeader::GatewayRetryHeader(uint64 retryCount)
      : retryCount_(retryCount)
    {
    }

    GatewayRetryHeader::GatewayRetryHeader(GatewayRetryHeader const & other)
    {
        retryCount_ = other.retryCount_;
    }       

    GatewayRetryHeader GatewayRetryHeader::FromMessage(Transport::Message & message)
    {
        GatewayRetryHeader retryHeader;
        if (!message.Headers.TryReadFirst(retryHeader))
        {
            Assert::TestAssert("GatewayRetryHeader not found {0}", message.MessageId);
        }
        return retryHeader;
    }

    void GatewayRetryHeader::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w << "RetryCount = " << retryCount_;
    }
}
