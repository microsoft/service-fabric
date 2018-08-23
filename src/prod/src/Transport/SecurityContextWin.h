// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    // For Kerberos and Negotiate
    class SecurityContextWin : public SecurityContext
    {
    public:
        SecurityContextWin(
            IConnectionSPtr const & connection,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & targetName,
            ListenInstance localListenInstance);

        ~SecurityContextWin();

        static bool Supports(SecurityProvider::Enum provider);

        SECURITY_STATUS EncodeMessage(_Inout_ MessageUPtr & message) override;
        SECURITY_STATUS EncodeMessage(
            TcpFrameHeader const & frameHeader,
            Message & message,
            Common::ByteBuffer2 & encrypted) override;

        SECURITY_STATUS DecodeMessage(_Inout_ MessageUPtr & message) override; 
        SECURITY_STATUS DecodeMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & decrypted) override; 

        SECURITY_STATUS ProcessClaimsMessage(MessageUPtr & message) override;

        SECURITY_STATUS AuthorizeRemoteEnd() override;

        void CompleteClaimsRetrieval(Common::ErrorCode const &, std::wstring const &) override { };
        void CompleteClientAuth(Common::ErrorCode const &, SecuritySettings::RoleClaims const & clientClaims, Common::TimeSpan expiration) override;

        virtual bool AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const override;

    private:
        void OnInitialize() override;

        SECURITY_STATUS OnNegotiateSecurityContext(
            _In_ MessageUPtr const & inputMessage,
            _Inout_opt_ PSecBufferDesc pOutput,
            _Out_ MessageUPtr & outputMessage) override;

        SECURITY_STATUS OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const & negotiationInfo) override;

        bool AccessCheck(
            _In_ PSECURITY_DESCRIPTOR securityDescriptor,
            DWORD desiredAccess,
            _In_ PGENERIC_MAPPING genericMapping) const;

        SECURITY_STATUS EncryptMessagePart(
            _Inout_ std::vector<byte> & token,
            _Inout_ std::vector<byte> & data,
            _Inout_ std::vector<byte> & padding);

        SECURITY_STATUS DecryptMessageHeaders(
            _Inout_ std::vector<byte> & token,
            _Inout_ std::vector<byte> & data,
            _Inout_ std::vector<byte> & padding);

        SECURITY_STATUS DecryptMessageBody(
            _Inout_ MessageUPtr & message,
            _Inout_ std::vector<byte> & token,
            _Inout_ std::vector<byte> & padding);

        SECURITY_STATUS CalculateSignature(MessageUPtr const & message, bool headers, std::vector<byte> & signature);
        SECURITY_STATUS VerifySignature(MessageUPtr const & message, bool headers, std::vector<byte> & signature);

        SECURITY_STATUS EncryptMessage(TcpFrameHeader const & frameHeader, Message & message, Common::ByteBuffer2 & encrypted);
        SECURITY_STATUS DecryptMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & decrypted);

        SECURITY_STATUS SignMessage(TcpFrameHeader const & frameHeader, Message & message, Common::ByteBuffer2 & signedMessage);
        SECURITY_STATUS VerifySignedMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & output);

        void SignatureBuffers(
            SecurityBuffers & buffers,
            MessageUPtr const & message,
            bool headers,
            std::vector<byte> & signature);

        SECURITY_STATUS AuthorizeWindowsClient();

        void TraceRemoteSid() const;
        void TryTraceRemoteGroupSidAndLocalSecuritySettings() const;

    private:
        SecPkgContext_Sizes sizes_;
        HANDLE remoteToken_ = INVALID_HANDLE_VALUE;
        uint maxEncryptSize_ = Common::SecurityConfig::GetConfig().WindowsSecurityMaxEncryptSize;
    };
}
