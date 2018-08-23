// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TcpFrameHeader;

    class SecurityContext : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>, public std::enable_shared_from_this<SecurityContext>
    {
        DENY_COPY(SecurityContext);

    public:
        SecurityContext(
            IConnectionSPtr const & connection,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & targetName,
            ListenInstance localListenInstance);

        virtual ~SecurityContext() = 0;

        static SecurityContextSPtr Create(
            IConnectionSPtr const & connection,
            TransportSecuritySPtr const & transportSecurity,
            std::wstring const & targetName,
            ListenInstance localListenInstance);

        SECURITY_STATUS NegotiateSecurityContext(MessageUPtr const & input, _Out_ MessageUPtr & output);
        bool NegotiationSucceeded() const { return negotiationState_ == SEC_E_OK; }
        bool ShouldCheckVerificationHeaders() const { return NegotiationSucceeded() && shouldCheckVerificationHeaders_; }

        bool FramingProtectionEnabled() const { return framingProtectionEnabled_; }
        void EnableFramingProtection();
        void DisableFramingProtection();

        bool ShouldWaitForConnectionAuthStatus() const;
        Common::ErrorCode FaultToReport() const;
        SECURITY_STATUS ConnectionAuthorizationState() const { return connectionAuthorizationState_; }
        bool ConnectionAuthorizationPending() const { return connectionAuthorizationState_ == STATUS_PENDING; }
        bool ConnectionAuthorizationSucceeded() const { return connectionAuthorizationState_ == SEC_E_OK; }
        bool ConnectionAuthorizationFailed() const { return FAILED(connectionAuthorizationState_); }

        virtual SECURITY_STATUS EncodeMessage(_Inout_ MessageUPtr & message) = 0;
        virtual SECURITY_STATUS EncodeMessage(
            TcpFrameHeader const & frameHeader,
            Message & message,
            Common::ByteBuffer2 & encrypted) = 0;

        virtual SECURITY_STATUS DecodeMessage(_Inout_ MessageUPtr & message) = 0;
        virtual SECURITY_STATUS DecodeMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & decrypted) = 0; 
        virtual SECURITY_STATUS ProcessClaimsMessage(MessageUPtr & message) = 0;

        virtual SECURITY_STATUS AuthorizeRemoteEnd() = 0;

        virtual bool AuthenticateRemoteByClaims() const;
        virtual bool ShouldPerformClaimsRetrieval() const;
        virtual void CompleteClaimsRetrieval(Common::ErrorCode const &, std::wstring const & localClaims) = 0;
        virtual void CompleteClientAuth(Common::ErrorCode const & error, SecuritySettings::RoleClaims const & clientClaims, Common::TimeSpan expiration) = 0;

        virtual bool AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const = 0;

        // Input "allowedRoles" is a bitmask, each bit represents a role,
        // return value is true iff one of the roles is enabled for this SecurityContext
        bool IsInRole(RoleMask::Enum allowedRoles, RoleMask::Enum effectiveRole = RoleMask::Enum::None);

        void ScheduleSessionExpiration(Common::TimeSpan expiration);

        TransportSecurity const & TransportSecurity() const;

        MessageUPtr CreateConnectionAuthMessage(Common::ErrorCode const & authStatus);

        void AddSecurityNegotiationHeader(Message & message);
        void AddVerificationHeaders(Message & message);

        void TryGetIncomingSecurityNegotiationHeader(Message & message);
        bool CheckVerificationHeadersIfNeeded(Message & message);

    protected:
        bool NegotiationStarted() const;
        SECURITY_STATUS QueryAttributes(ULONG ulAttribute, _Out_ void* pBuffer);
        wchar_t* TargetName() const;

        MessageUPtr CreateSecurityNegoMessage(_In_ PSecBufferDesc pBuffers);
        static void FreeSecurityBuffers(std::vector<Common::const_buffer> const & buffers, void *);
        static void FreeEncryptedBuffer(std::vector<Common::const_buffer> const &, void * state);
        static void FreeByteBuffer(std::vector<Common::const_buffer> const &, void * state);
        static std::vector<byte> GetMergedMessageBody(MessageUPtr const & inputMessage);

        PCtxtHandle IscInputContext();
        PCtxtHandle AscInputContext();

        static void ReportSecurityApiSlow(wchar_t const* api, Common::TimeSpan duration, Common::TimeSpan threshold);

    private:
        void Initialize();
        virtual void OnInitialize() = 0;
        void SetNegotiationStartIfNeeded();

        virtual SECURITY_STATUS OnNegotiateSecurityContext(
            _In_ MessageUPtr const & inputMessage,
            _Inout_opt_ PSecBufferDesc pOutput,
            _Out_ MessageUPtr & outputMessage) = 0;

        virtual SECURITY_STATUS OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const & negotiationInfo) = 0;

        void CreateSessionExpirationTimer();
        void ScheduleSessionExpirationIfNeeded();
        void AddServerAuthHeaderIfNeeded(Message & message);
        void SearchForServerAuthHeaderOnFirstIncomingMessage(MessageUPtr const & incomingNegoMessage);

        bool IsConnectionAuthorizationFailureRetryable() const { return connectionAuthorizationState_ == CRYPT_E_REVOCATION_OFFLINE; }

        bool IsSessionExpirationEnabled() const;

        void OnRemoteFrameSizeLimit();

    protected:
        const IConnectionWPtr connection_;
        std::wstring const id_;
        TransportSecuritySPtr const transportSecurity_;

        const bool inbound_;
        bool shouldSendServerAuthHeader_;
        bool shouldCheckForIncomingServerAuthHeader_;
        bool remotePromisedToSendConnectionAuthStatus_;
        bool shouldCheckVerificationHeaders_ = true;

#ifdef PLATFORM_UNIX
        bool framingProtectionEnabled_ = true;
#else
        bool framingProtectionEnabled_ = Common::SecurityConfig::GetConfig().FramingProtectionEnabled;
#endif

        std::wstring const targetName_;
        ULONG securityRequirements_;

        CtxtHandle hSecurityContext_;
        std::vector<SecurityCredentialsSPtr> credentials_;
        SECURITY_STATUS negotiationState_;
        SECURITY_STATUS connectionAuthorizationState_;
        ULONG fContextAttrs_;

        RoleMask::Enum roleMask_;

        Common::DateTime contextExpiration_;
        Common::StopwatchTime sessionExpiration_;
        Common::TimerSPtr sessionExpirationTimer_;

        SecurityNegotiationHeader outgoingSecNegoHeader_;
        SecurityNegotiationHeader incomingSecNegoHeader_;
    };

    class ConnectionAuthMessage : public Serialization::FabricSerializable
    {
    public:
        ConnectionAuthMessage();
        ConnectionAuthMessage(std::wstring const & errorMessage, RoleMask::Enum roleGranted = RoleMask::None);

        std::wstring const & ErrorMessage() const;
        std::wstring && TakeMessage();
        RoleMask::Enum RoleGranted() const { return roleGranted_; }
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(errorMessage_, roleGranted_);

    private:
        std::wstring errorMessage_;
        RoleMask::Enum roleGranted_ = RoleMask::None;
    };
}
