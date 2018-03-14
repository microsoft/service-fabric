// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GatewayRetryHeader : 
        public Transport::MessageHeader<Transport::MessageHeaderId::GatewayRetry>, 
        public Serialization::FabricSerializable
    {
    public:
        GatewayRetryHeader();

        explicit GatewayRetryHeader(uint64 retryCount);

        GatewayRetryHeader(GatewayRetryHeader const & other);

        static GatewayRetryHeader FromMessage(Transport::Message &);
        
        __declspec(property(get=get_RetryCount)) uint64 RetryCount;
        inline uint64 get_RetryCount() const { return retryCount_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& f) const;

        FABRIC_FIELDS_01(retryCount_);

    private:
        uint64 retryCount_;
    };
}
