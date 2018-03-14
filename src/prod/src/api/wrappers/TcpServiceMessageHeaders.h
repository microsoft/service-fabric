// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class TcpServiceMessageHeader : public Transport::MessageHeader<Transport::MessageHeaderId::TcpServiceMessageHeader>
        , public Serialization::FabricSerializable
    {
    public:
        TcpServiceMessageHeader() {}

        TcpServiceMessageHeader(
            BYTE * bytes, ULONG size)
           :headers_()
        {
            headers_.reserve(size);
            for (ULONG i = 0; i < size; ++i)
            {
                headers_.push_back(bytes[i]);
            }
        }

        __declspec(property(get = get_Headers))std::vector<byte>  Headers;
        std::vector<byte> const &  get_Headers() const
        {
            return headers_;
        }
  
        FABRIC_FIELDS_01(headers_);

    private:
        std::vector<byte>  headers_;
    };
}
