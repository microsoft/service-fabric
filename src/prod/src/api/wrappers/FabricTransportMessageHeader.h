// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class FabricTransportMessageHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FabricTransportMessageHeader>
        , public Serialization::FabricSerializable
    {
    public:
        FabricTransportMessageHeader() 
        {
            this->headerSize_ = 0;
        }

        FabricTransportMessageHeader(
            ULONG headerSize)
        {
            this->headerSize_ = headerSize;
        }

        __declspec(property(get = get_HeaderSize)) ULONG HeaderSize;
        ULONG  get_HeaderSize() 
        {
            return headerSize_;
        }
  
        FABRIC_FIELDS_01(headerSize_);

    private:
        ULONG headerSize_;
    };
}
