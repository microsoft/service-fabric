// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport 
{
    class ClientRoleHeader 
        : public MessageHeader<MessageHeaderId::Enum::ClientRole>, public Serialization::FabricSerializable
    {
    public:
        ClientRoleHeader();
        ClientRoleHeader(RoleMask::Enum role);

        RoleMask::Enum Role();

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(role_);

    private:
        RoleMask::Enum role_;
    };
}

