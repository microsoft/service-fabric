// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace ProtectionLevel
    {
        enum Enum
        {
            None = 0,
            Sign = 1,
            EncryptAndSign = 2
        };

        Common::ErrorCode Parse(std::wstring const & inputString, _Out_ Enum & result);
        Common::ErrorCode FromPublic(::FABRIC_PROTECTION_LEVEL publicEnum, _Out_ ProtectionLevel::Enum & internalEnum);
        ::FABRIC_PROTECTION_LEVEL ToPublic(ProtectionLevel::Enum internalEnum);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
