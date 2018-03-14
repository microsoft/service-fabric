// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace SecurityProvider
    {
        #include "SecurityProviderEnum.h"

        byte EnumToMask(Enum e, byte inputMask = 0);
        byte EnumsToMask(std::vector<Enum> es);
        std::vector<Enum> MaskToEnums(byte mask);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        void WriteMaskToTextWriter(Common::TextWriter & w, byte e);

        wchar_t * GetSspiProviderName(Enum e);
        Common::ErrorCode FromCredentialType(std::wstring const & credentialTypeString, Enum & result);
        Common::ErrorCode FromPublic(::FABRIC_SECURITY_CREDENTIAL_KIND publicEnum, Enum & internalEnum);
        ::FABRIC_SECURITY_CREDENTIAL_KIND ToPublic(Enum internalEnum, ::FABRIC_SECURITY_CREDENTIAL_KIND convertedFrom);

        DECLARE_ENUM_STRUCTURED_TRACE(SecurityProvider);
    }
}
