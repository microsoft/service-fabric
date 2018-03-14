// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ClientSecurityHelper
    {
    public:
        static Common::GlobalWString DefaultSslClientCredentials;
        static Common::GlobalWString DefaultWindowsCredentials;

    public:
        static void ClearDefaultCertIssuers();
        static void SetDefaultCertIssuers();
        
        static bool TryCreateClientSecuritySettings(
            std::wstring const & credentialsToParse,
            __out std::wstring & credentialsType,
            __out Transport::SecuritySettings & settingsResult,
            __out std::wstring & errorMessage);
    };
}
