// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RequestInstanceHeader 
        : public Transport::MessageHeader<Transport::MessageHeaderId::RequestInstance>
        , public Serialization::FabricSerializable
    {
    public:
        RequestInstanceHeader();

        explicit RequestInstanceHeader(__int64 instance);

        RequestInstanceHeader(RequestInstanceHeader const & other);
        
        __declspec(property(get=get_Instance)) __int64 Instance;
        inline __int64 get_Instance() const { return instance_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& f) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_01(instance_);

    private:
        __int64 instance_;
    };
}
