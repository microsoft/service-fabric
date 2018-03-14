// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TransportSecurity : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        friend class TcpDatagramTransport;
        friend class SecureTransportTests;
        friend class IpcTestBase;
        DENY_COPY(TransportSecurity);

    public:
        TransportSecurity(bool isClientOnly = false);
        ~TransportSecurity();

        Common::ErrorCode Set(SecuritySettings const & securitySettings);
        SecuritySettings const & Settings() const;
        std::wstring ToString() const;

        Common::ErrorCode CanUpgradeTo(TransportSecurity const & rhs);

        __declspec (property(get=get_SecurityProvider)) SecurityProvider::Enum SecurityProvider;
        SecurityProvider::Enum get_SecurityProvider() const;

        __declspec (property(get=get_SecurityRequirements)) ULONG SecurityRequirements;
        ULONG get_SecurityRequirements() const;

        __declspec (property(get=get_HeaderSecurity)) ProtectionLevel::Enum MessageHeaderProtectionLevel;
        ProtectionLevel::Enum get_HeaderSecurity() const;

        __declspec (property(get=get_BodySecurity)) ProtectionLevel::Enum MessageBodyProtectionLevel;
        ProtectionLevel::Enum get_BodySecurity() const;

        SecurityCredentialsSPtr Credentails() const;
        void GetCredentials(
            _Out_ std::vector<SecurityCredentialsSPtr> & credentials,
            _Out_ std::vector<SecurityCredentialsSPtr> & svrCredentials);

        void RefreshX509CredentialsIfNeeded();

        bool UsingWindowsCredential() const;

        std::wstring const & LocalWindowsIdentity();
        bool RunningAsMachineAccount();

        PSECURITY_DESCRIPTOR ConnectionAuthSecurityDescriptor() const;
        PSECURITY_DESCRIPTOR AdminRoleSecurityDescriptor() const;

        bool SessionExpirationEnabled() const;
        void DisableSecureSessionExpiration() { sessionExpirationEnabled_ = false; }

        ULONG MaxIncomingFrameSize() const;
        ULONG MaxOutgoingFrameSize() const;
        void SetMaxIncomingFrameSize(ULONG value);
        void SetMaxOutgoingFrameSize(ULONG value);

        // Create self-relative security descriptor from owner token and allowed Windows identities.
        // The DACL will contain allowed ACE for the owner SID and each identity in allowedList.
        static Common::ErrorCode CreateSecurityDescriptorFromAllowedList(
            HANDLE ownerToken,
            bool allowOwner,
            SecuritySettings::IdentitySet const & allowedList,
            DWORD allowedAccess,
            _Out_ Common::SecurityDescriptorSPtr & securityDescriptor);

        static Common::ErrorCode GetMachineAccount(_Out_ std::wstring & machineAccount);

        static Common::ErrorCode RegisterWindowsFabricSpn();
        static Common::ErrorCode UnregisterWindowsFabricSpn();

        typedef std::function<void(std::wstring const & claims, SecurityContextSPtr const & context)> ClaimsHandler;
        typedef std::function<void(ClaimsRetrievalMetadataSPtr const &, SecurityContextSPtr const & context)> ClaimsRetrievalHandler;

        void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata &&);
        void SetClaimsRetrievalHandler(ClaimsRetrievalHandler const & handler);
        void SetClaimsHandler(ClaimsHandler const & handler);
        void RemoveClaimsRetrievalHandler();
        void RemoveClaimsHandler();

        ClaimsRetrievalMetadataSPtr const & GetClaimsRetrievalMetadata() const;
        ClaimsRetrievalHandler ClaimsRetrievalHandlerFunc() const;
        ClaimsHandler ClaimsHandlerFunc() const;

        bool IsClientOnly() const;

        static ULONG GetCredentialRefreshCount();

        static ULONG Test_GetInternalMaxFrameSize(ULONG value) { return GetInternalMaxFrameSize(value); }

    private:
        Common::ErrorCode InitializeLocalWindowsIdentityIfNeeded();

        Common::ErrorCode AcquireCredentials_CallerHoldingWLock();

        static std::wstring AddressToSpn(std::wstring const & address);
        std::wstring GetServerIdentity(std::wstring const & address) const;
        std::wstring GetPeerIdentity(std::wstring const & address) const;

        static ULONG GetInternalMaxFrameSize(ULONG value);

#ifndef PLATFORM_UNIX
        static Common::ErrorCode GetDomainNameDns(_Out_ std::wstring & domain);
        static Common::ErrorCode UpdateWindowsFabricSpn(DS_SPN_WRITE_OP operation);
#endif

        // Copy extra settings not covered by SecuritySettings
        void CopyNonSecuritySettings(TransportSecurity const & other);

        void ReportCertHealthIfNeeded_LockHeld(bool newCredentialLoaded);

        void Test_SetHeaderProtectionLevel(ProtectionLevel::Enum value);
        void Test_SetBodyProtectionLevel(ProtectionLevel::Enum value);

    private:
        bool initialized_;
        bool isClientOnly_;
        Transport::SecuritySettings securitySettings_;

        ULONG securityRequirements_;
        ProtectionLevel::Enum messageHeaderProtectionLevel_;
        ProtectionLevel::Enum messageBodyProtectionLevel_;
        std::unique_ptr<std::vector<byte>> authenticationData_;

        Common::SecurityDescriptorSPtr adminRoleSecurityDescriptor_; // For assigning admin role to clients

        // Windows credential specific settings
        HANDLE localTokenHandle_;
        std::vector<byte> localUserToken_;
        std::wstring localWindowsIdentity_;
        bool runningAsMachineAccount_;
        Common::SecurityDescriptorSPtr connectionAuthSecurityDescriptor_; // For incoming connection authorization

        MUTABLE_RWLOCK(TransportSecurity, credentialLock_);
        std::vector<SecurityCredentialsSPtr> credentials_; // SSL/TLS client credentials or Windows dual-purpose credentials
        std::vector<SecurityCredentialsSPtr> svrCredentials_; // SSL/TLS server credentials
        static ULONG credentialRefreshCount_;
        Common::StopwatchTime lastCertHealthReportTime_ = Common::StopwatchTime::Zero;
        Common::LogLevel::Enum lastCertHealthReportLevel_ = Common::LogLevel::Info;

        mutable Common::RwLock claimsHandlerLock_;
        ClaimsRetrievalMetadataSPtr claimsRetrievalMetadata_;
        ClaimsRetrievalHandler claimsRetrievalHandler_;
        ClaimsHandler claimsHandler_;

        bool sessionExpirationEnabled_;

        // Whether to wait for an authorization ack/nack from server side before start sending.
        // will only be set true for client-only transport in secure mode
        bool shouldWaitForAuthStatus_;

        ULONG maxIncomingFrameSize_ = 0;
        ULONG maxOutgoingFrameSize_ = 0;

        static bool AssignAdminRoleToLocalUser;
    };
}
