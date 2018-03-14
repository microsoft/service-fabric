// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace RoleMask
    {
        enum Enum: unsigned int
        {
            None  = FABRIC_CLIENT_ROLE_UNKNOWN,
            User = FABRIC_CLIENT_ROLE_USER,
            Admin = FABRIC_CLIENT_ROLE_ADMIN,
            All   = 0xffffffff
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        Common::ErrorCode TryParse(std::wstring const & stringInput, _Out_ Enum & roleMask);
        Common::ErrorCode TryParse(FABRIC_CLIENT_ROLE clientRole, _Out_ Enum & roleMask);
        std::wstring EnumToString(Transport::RoleMask::Enum value);
    };
}
