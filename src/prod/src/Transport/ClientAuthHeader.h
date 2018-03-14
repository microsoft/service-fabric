// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ClientAuthHeader : public MessageHeader<MessageHeaderId::MessageSecurity>, public Serialization::FabricSerializable
    {
    public:
        ClientAuthHeader();
        ClientAuthHeader(SecurityProvider::Enum securityProvider);

        SecurityProvider::Enum ProviderType();

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(providerType_);

    private:
        SecurityProvider::Enum providerType_;
    };
}

