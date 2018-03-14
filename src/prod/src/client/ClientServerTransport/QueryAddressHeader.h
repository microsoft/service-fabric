// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class QueryAddressHeader
    : public Transport::MessageHeader<Transport::MessageHeaderId::QueryAddress>
    , public Serialization::FabricSerializable
    {
    public:
        QueryAddressHeader()
            : address_()
        {
        }
            
        QueryAddressHeader(std::wstring const & address)
            : address_(address)
        {
        }

        __declspec(property(get=get_Address)) std::wstring const & Address;
        std::wstring const & get_Address() const { return address_; }

        FABRIC_FIELDS_01(address_);

    private:
        std::wstring address_;
   };
}
