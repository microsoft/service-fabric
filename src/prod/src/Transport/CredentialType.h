// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace CredentialType
    {
        enum Enum
        {
            None = SecurityProvider::None,
            X509 = SecurityProvider::Ssl,
            Windows = SecurityProvider::Kerberos
        };

        bool TryParse(std::wstring const & providerString, __out Enum & result);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
