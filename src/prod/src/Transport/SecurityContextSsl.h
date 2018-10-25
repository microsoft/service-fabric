// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityContextSsl : public SecurityContext
    {
    public:
        SecurityContextSsl(
            IConnectionSPtr const & connection,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & targetName,
            ListenInstance localListenInstance);

        static bool Supports(SecurityProvider::Enum provider);

        SECURITY_STATUS AuthorizeRemoteEnd() override;

        SECURITY_STATUS EncodeMessage(_Inout_ MessageUPtr & message) override;
        SECURITY_STATUS EncodeMessage(
            TcpFrameHeader const & frameHeader,
            Message & message,
            Common::ByteBuffer2 & encrypted) override;

        SECURITY_STATUS DecodeMessage(_Inout_ MessageUPtr & message) override; 
        SECURITY_STATUS DecodeMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & decrypted) override; 

        bool AuthenticateRemoteByClaims() const override;
        bool ShouldPerformClaimsRetrieval() const override;
        void CompleteClaimsRetrieval(Common::ErrorCode const &, std::wstring const & claimsToken) override;
        void CompleteClientAuth(Common::ErrorCode const &, SecuritySettings::RoleClaims const & clientClaims, Common::TimeSpan expiration) override;

        virtual bool AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const override;

        static void TraceCertificate(
            std::wstring const & traceId,
            PCCERT_CONTEXT certContext,
            Common::Thumbprint const & issuerCertThumbprint);

        static SECURITY_STATUS VerifyCertificate(
            std::wstring const & traceId,
            PCCERT_CONTEXT certContext,
            DWORD certChainFlags,
            bool shouldIgnoreCrlOffline,
            bool remoteAuthenticatedAsPeer,
            Common::ThumbprintSet const & certThumbprintsToMatch,
            Common::SecurityConfig::X509NameMap const & x509NamesToMatch,
            bool traceCert,
            _Out_opt_ std::wstring * commonNameMatched = nullptr);

        static SECURITY_STATUS Test_VerifyCertificate(_In_ PCCERT_CONTEXT certContext, std::wstring const & commonNameToMatch);

#ifdef PLATFORM_UNIX
        Common::ErrorCode Encrypt(void const* buffer, size_t len);
        Common::ByteBuffer2 EncryptFinal();
#endif

        SECURITY_STATUS ProcessClaimsMessage(MessageUPtr & message) override;

    private:
        void OnInitialize() override;

        SECURITY_STATUS OnNegotiateSecurityContext(
            _In_ MessageUPtr const & inputMessage,
            _Inout_opt_ PSecBufferDesc pOutput,
            _Out_ MessageUPtr & outputMessage) override;

        SECURITY_STATUS OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const & negotiationInfo) override;

        SECURITY_STATUS CheckClientAuthHeader(_In_ MessageUPtr const & inputMessage);

        SECURITY_STATUS SspiDecrypt(_Inout_ byte* input, ULONG inputLength, _Inout_ SecBuffer buffers[4]);
        SECURITY_STATUS DecryptMessageHeaders(_Inout_ std::vector<byte> & input, _Out_ ByteBique & output);
        SECURITY_STATUS DecryptMessageBody(_Inout_ MessageUPtr & message);

        HRESULT GetSslOutputLimit(ULONG inputBytes, _Out_ ULONG & outputLimit);

        SECURITY_STATUS WriteSslRecord(
            ULONG dataLength,
            _Inout_ byte * sslRecord,
            _Out_ ULONG & sslRecordSize);

        static SECURITY_STATUS TryMatchCertThumbprint(
            std::wstring const & traceId,
            PCCERT_CONTEXT certContext,
            Common::Thumbprint const & incoming,
            SECURITY_STATUS certChainErrorStatus,
            uint certChainFlags,
            bool onlyCrlOfflineEncountered,
            Common::ThumbprintSet const & thumbprintSet);

        static SECURITY_STATUS GetCertChainContext(
            std::wstring const & traceId,
            _In_ PCCERT_CONTEXT certContext,
            _In_ LPSTR usage,
            DWORD certChainFlags,
            _Out_ Common::CertChainContextUPtr & certChainContext,
            _Out_opt_ Common::X509Identity::SPtr* issuerPubKey,
            _Out_opt_ Common::X509Identity::SPtr* issuerCertThumbprint);

        static SECURITY_STATUS VerifySslCertChain(
            std::wstring const & traceId,
            PCCERT_CHAIN_CONTEXT certChainContext,
            DWORD authType,
            DWORD certChainFlags,
            bool shouldIgnoreCrlOffline,
            bool & onlyCrlOfflineEncountered);

        static bool IsClientAuthenticationCertificate(std::wstring const & traceId, _In_ PCCERT_CONTEXT certContext);
        static bool IsServerAuthenticationCertificate(std::wstring const & traceId, _In_ PCCERT_CONTEXT certContext);
        static bool CertCheckEnhancedKeyUsage(
            std::wstring const & traceId,
            _In_ PCCERT_CONTEXT certContext,
            LPCSTR usageIdentifier);

        static SECURITY_STATUS VerifyAsServerAuthCert(
            std::wstring const & traceId,
            PCCERT_CONTEXT certContext,
            PCCERT_CHAIN_CONTEXT certChainContext,
            Common::Thumbprint const & certThumbprint,
            Common::X509Identity::SPtr const & issuerPubKey,
            Common::X509Identity::SPtr const & issuerCertThumbprint,
            Common::SecurityConfig::X509NameMap const & x509NamesToMatch,
            DWORD certChainFlags,
            bool shouldIgnoreCrlOffline,
            bool onlyCrlOfflineEncountered,
            _Out_opt_ std::wstring * nameMatched = nullptr);

        static MessageUPtr CreateClaimsTokenErrorMessage(Common::ErrorCode const &);

        void TryAuthenticateRemoteAsPeer();

    private:

#ifdef PLATFORM_UNIX
        static void SslInfoCallbackStatic(const SSL* ssl, int where, int ret);
        void SslInfoCallback(int where, int ret);

        static int CertVerifyCallbackStatic(int, X509_STORE_CTX*);
        int CertVerifyCallback(int, X509_STORE_CTX*);

        void AddDataToDecrypt(void const* buffer, size_t len);
        SECURITY_STATUS Decrypt(void* buffer, _Inout_ int64& len);
        static constexpr size_t DecryptedPendingMax();

        Common::LinuxCryptUtil cryptUtil_;
        BIO* inBio_ = nullptr;
        BIO* outBio_ = nullptr;
        SslUPtr ssl_;
        Common::LinuxCryptUtil::CertChainErrors certChainErrors_;
#else
        ///
        /// Verifies whether the chain's trust status contains only allowed/non-fatal errors.
        ///
        static inline bool IsExpectedCertChainTrustError(
            PCCERT_CHAIN_CONTEXT certChainContext,
            SECURITY_STATUS certChainValidationStatus,
            HRESULT expectedChainValidationStatus,
            DWORD nonFatalCertChainErrorMask)
        {
            return (certChainValidationStatus == expectedChainValidationStatus)                                                 // act on specific validation errors
                && nonFatalCertChainErrorMask                                                                                   // must be a trust error
                && ((certChainContext->TrustStatus.dwErrorStatus & nonFatalCertChainErrorMask) == nonFatalCertChainErrorMask);  // must be an expected trust error
        }

        std::vector<SecurityCredentialsSPtr> svrCredentials_;
        SecPkgContext_StreamSizes streamSizes_;

        class TemplateHelper;

        // SSPI SSL decryption dictates a single input buffer. When and only when input span across multiple 
        // buffers, e.g. bique chunk boundary, such buffers are merged into a continuous one before decryption.
        Common::ByteBuffer2 decryptionInputMergeBuffer_;
#endif

        Common::CertContextUPtr remoteCertContext_;
        std::wstring remoteCommonName_;
        bool remoteAuthenticatedAsPeer_ = false;
        bool claimsMessageReceived_ = false;
        SecuritySettings::RoleClaims clientClaims_;

    };
}
