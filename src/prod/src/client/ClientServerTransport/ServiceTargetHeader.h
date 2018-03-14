// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceTargetHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::ServiceTarget>
        , public Serialization::FabricSerializable
    {
    public:
        ServiceTargetHeader() : serviceName_()
        {
        }

        explicit ServiceTargetHeader(Common::NamingUri const & serviceName) : serviceName_(serviceName)
        {
        }

        __declspec (property(get=get_ServiceName)) Common::NamingUri const & ServiceName;
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << "ServiceTargetHeader( " << serviceName_ << " )";
        }

        FABRIC_FIELDS_01(serviceName_);

    private:
        Common::NamingUri serviceName_;
    };
}
