// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityCredentials : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(SecurityCredentials);

    public:
        ~SecurityCredentials();

        static Common::ErrorCode AcquireWin(
            std::wstring const & securityPrincipal,
            SecurityProvider::Enum securityProvider,
            bool outboundOnly,
            _In_ PVOID authenticationData,
            _Out_ std::vector<SecurityCredentialsSPtr> & credentials);

        static Common::ErrorCode AcquireSsl(
            Common::CertContextUPtr const & certContext,
            _Out_opt_ std::vector<SecurityCredentialsSPtr> * credentials,
            _Out_opt_ std::vector<SecurityCredentialsSPtr> * svrCredentials);

        static Common::ErrorCode AcquireSsl(
            Common::X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<Common::X509FindValue> const & findValue,
            _Out_opt_ std::vector<SecurityCredentialsSPtr> * credentials,
            _Out_opt_ std::vector<SecurityCredentialsSPtr> * svrCredentials);

        static Common::ErrorCode AcquireSsl(
            Common::CertContextUPtr const & certContext,
            ULONG credentialUse,
            _Out_ SecurityCredentialsSPtr & credentials);

        static Common::ErrorCode AcquireAnonymousSslClient(_Out_ std::vector<SecurityCredentialsSPtr> & credentials);

        static Common::ErrorCode LocalTimeStampToDateTime(TimeStamp localTimestamp, _Out_ Common::DateTime & dateTime);

        PCredHandle Credentials();

        Common::DateTime Expiration() const;

        Common::Thumbprint const & X509CertThumbprint() const;

        Common::X509PubKey const * X509PublicKey() { return x509PubKey_.get(); }

        static bool SortedVectorsEqual(std::vector<SecurityCredentialsSPtr> const& a, std::vector<SecurityCredentialsSPtr> const& b);
        static std::wstring VectorsToString(std::vector<SecurityCredentialsSPtr> const& v);

#ifdef PLATFORM_UNIX
        SSL_CTX* SslCtx() const; 
#else
        bool IsValid() const;
#endif

        static uint64 ObjCount();

    private:
        SecurityCredentials();

        CredHandle credentials_;
        Common::DateTime expiration_;
        Common::Thumbprint x509CertThumbprint_;
        Common::X509PubKey::UPtr x509PubKey_;

#ifdef PLATFORM_UNIX
        SSL_CTX* sslCtx_;
#endif
    };
}
