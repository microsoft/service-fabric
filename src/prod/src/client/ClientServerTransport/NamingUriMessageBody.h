// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class NamingUriMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        NamingUriMessageBody() {}
        NamingUriMessageBody(Common::NamingUri uri) : uri_(uri) {}

        __declspec(property(get=get_Address)) Common::NamingUri const & Uri;
        Common::NamingUri const & get_Address() const { return uri_; }

        virtual Serialization::FabricSerializable& GetTcpMessageBody()
        {
            return uri_;
        }

    private:
        Common::NamingUri uri_;
    };
}
