// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityUtil: public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
    public:
        static SECURITY_STATUS VerifyCertificate(
            PCCERT_CONTEXT certContext,
            Common::ThumbprintSet const & certThumbprintsToMatch,
            Common::SecurityConfig::X509NameMap const & x509NamesToMatch,
            bool traceCert,
            DWORD certChainFlags,
            bool shouldIgnoreCrlOffline);

        static Common::ErrorCode GetX509SvrCredThumbprint(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _In_opt_ Common::Thumbprint const * loadedCredThumbprint,
            _Out_ Common::Thumbprint & thumbprint);

        static Common::ErrorCode GetX509CltCredExpiration(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetX509SvrCredExpiration(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetX509CltCredAttr(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _Out_ Common::Thumbprint & thumbprint,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetX509SvrCredAttr(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _Out_ Common::Thumbprint & thumbprint,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode AllowDefaultClientsIfNeeded(
            _Inout_ SecuritySettings & serverSecuritySettings,
            Common::FabricNodeConfig const & nodeConfig,
            std::wstring const& traceId);

        static Common::ErrorCode EnableClientRoleIfNeeded(
            _Inout_ SecuritySettings & serverSecuritySettings,
            Common::FabricNodeConfig const & nodeConfig,
            std::wstring const& traceId);
    };
}
