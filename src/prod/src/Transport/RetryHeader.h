// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RetryHeader : public MessageHeader<MessageHeaderId::Retry>, public Serialization::FabricSerializable
    {
    public:
        RetryHeader() : retryCount_(0) { }
        RetryHeader(LONG retryCount) : retryCount_(retryCount) { }

        __declspec(property(get=get_RetryCount)) LONG const & RetryCount;

        LONG const & get_RetryCount() const { return retryCount_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << retryCount_;
        }

        FABRIC_FIELDS_01(retryCount_);
    private:
        LONG retryCount_;
    };
}
