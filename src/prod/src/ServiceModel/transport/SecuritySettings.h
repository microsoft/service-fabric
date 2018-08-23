// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecuritySettings : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
    public:
        class IdentitySet : public std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>>
        {
        public:
            bool operator == (IdentitySet const & rhs) const;
            bool operator != (IdentitySet const & rhs) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;
        };

        class RoleClaimsOrList;

        // Claims required for a role, string representation: ClaimType1=Value1 && ClaimType2=Value2 && ClaimTyp3=Value3
        class RoleClaims
        {
        public:
            bool AddClaim(std::wstring const & claimType, std::wstring const & claimValue);
            bool AddClaim(std::wstring const & claim);
            bool operator < (RoleClaims const & rhs) const;
            bool operator == (RoleClaims const & rhs) const;
            bool operator != (RoleClaims const & rhs) const;
            bool Contains(std::wstring const & claimType, std::wstring const & claimValue) const;
            bool Contains(RoleClaims const & other) const;
            bool Satisfy(RoleClaims const & other) const;
            bool IsInRole(RoleClaimsOrList const & roleClaimsOrList) const;

            std::multimap<std::wstring, std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> const & Value() const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

        private:
            std::multimap<std::wstring/*ClaimType*/, std::wstring/*ClaimValue*/, Common::IsLessCaseInsensitiveComparer<std::wstring>> value_;
        };

        // All possible RoleClaims for a role, string representation: RoleClaims1 || RoleClaims2 || RoleClaims3
        class RoleClaimsOrList
        {
        public:
            void AddRoleClaims(RoleClaims const & roleClaims);
            bool operator == (RoleClaimsOrList const & rhs) const;
            bool operator != (RoleClaimsOrList const & rhs) const;

            bool Contains(RoleClaims const & roleClaimsEntry) const;
            bool Contains(RoleClaimsOrList const & other) const;

            std::set<RoleClaims> const & Value() const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

        private:
            std::set<RoleClaims> value_;
        };

        static Common::ErrorCode StringToRoleClaims(std::wstring const & inputString, RoleClaims & roleClaims);
        static Common::ErrorCode StringToRoleClaimsOrList(std::wstring const & inputString, RoleClaimsOrList & roleClaimsOrList);
        static bool ClaimListToRoleClaim(std::vector<ServiceModel::Claim> const& claimList, _Out_ RoleClaims &roleClaims);

        static Common::ErrorCode FromConfiguration(
            std::wstring const & credentialType,
            std::wstring const & x509StoreName,
            std::wstring const & x509StoreLocation,
            std::wstring const & x509FindType,
            std::wstring const & x509FindValue,
            std::wstring const & x509FindValueSecondary,
            std::wstring const & protectionLevel,
            std::wstring const & remoteCertThumbprints,
            Common::SecurityConfig::X509NameMap const & remoteX509Names,
            Common::SecurityConfig::IssuerStoreKeyValueMap const & remoteCertIssuers,
            std::wstring const & remoteCertCommonNames,
            std::wstring const & defaultRemoteCertIssuers,
            std::wstring const & remoteSpn,
            std::wstring const & clientIdentities,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode FromPublicApi(
            FABRIC_SECURITY_CREDENTIALS const& securityCredentials,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateNegotiateServer(
            std::wstring const & clientIdentity,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateNegotiateClient(
            std::wstring const & serverIdentity,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateNegotiate(
            std::wstring const & serverIdentity,
            std::wstring const & clientIdentities,
            std::wstring const & protectionLevel,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateSelfGeneratedCertSslServer(
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateSslClient(
            Common::CertContextUPtr & certContext,
            std::wstring const & serverThumbprint,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateSslClient(
            std::wstring const & serverThumbprint,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateKerberos(
            std::wstring const & remoteSpn,
            std::wstring const & clientIdentities,
            std::wstring const & protectionLevel,
            _Out_ SecuritySettings & object);

        static Common::ErrorCode CreateClaimTokenClient(
            std::wstring const & localClaimToken,
            std::wstring const & serverCertThumbpints,
            std::wstring const & serverCertCommonNames,
            std::wstring const & serverCertIssuers,
            std::wstring const & protectionLevel,
            _Out_ SecuritySettings & object);

        SecuritySettings();

        Common::ErrorCode GenerateClientCert(
            _Out_ Common::SecureString & certKey,
            _Out_ Common::CertContextUPtr & cert) const;

        void ToPublicApi(
            _In_ Common::ScopedHeap & heap,
            _Out_ FABRIC_SECURITY_CREDENTIALS & securityCredentials) const;

        bool operator == (SecuritySettings const & rhs) const;
        bool operator != (SecuritySettings const & rhs) const;

        SecurityProvider::Enum SecurityProvider() const;
        ProtectionLevel::Enum ProtectionLevel() const;

        void SetX509CertType(std::wstring certType) { x509CertType_ = move(certType); }
        std::wstring const & X509CertType() const { return x509CertType_; }

        Common::X509StoreLocation::Enum X509StoreLocation() const;
        std::wstring const& X509StoreName() const;
        Common::X509FindType::Enum X509FindType() const;
        Common::X509FindValue::SPtr const& X509FindValue() const;
        std::wstring const& X509FindValueString() const;

        Common::CertContextUPtrSPtr const & CertContext() const { return certContext_; }

        bool IsSelfGeneratedCert() const { return isSelfGeneratedCert_; }

        Common::ThumbprintSet const & RemoteCertThumbprints() const;
        Common::ErrorCode SetRemoteCertThumbprints(std::wstring const & thumbprints);
        Common::ThumbprintSet const & AdminClientCertThumbprints() const;

        Common::SecurityConfig::X509NameMap const & RemoteX509Names() const;
        Common::SecurityConfig::IssuerStoreKeyValueMap const & RemoteCertIssuers() const;
        Common::ErrorCode SetRemoteCertIssuerPublicKeys(Common::X509IdentitySet issuerPublicKeys);

        Common::SecurityConfig::X509NameMap const & AdminClientX509Names() const;
        Common::ErrorCode SetAdminClientCertIssuerPublicKeys(Common::X509IdentitySet issuerPublicKeys);

        DWORD X509CertChainFlags() const;

        using X509CertChainFlagCallback = std::function<DWORD(void)>;
        void SetX509CertChainFlagCallback(X509CertChainFlagCallback const& callback);

        bool ShouldIgnoreCrlOfflineError(bool inbound) const;
        using CrlOfflineIgnoreCallback = std::function<bool(bool inbound)>; 
        void SetCrlOfflineIgnoreCallback(CrlOfflineIgnoreCallback const & callback);

        bool FramingProtectionEnabled() const;
        using BooleanFlagCallback = std::function<bool(void)>;
        void SetFramingProtectionEnabledCallback(BooleanFlagCallback const & callback);

        bool ReadyNewSessionBeforeExpiration() const;
        void SetReadyNewSessionBeforeExpirationCallback(BooleanFlagCallback const & callback);

        std::wstring const& RemoteSpn() const;
        void SetRemoteSpn(std::wstring const & remoteSpn);

        IdentitySet const & RemoteIdentities() const;
        void AddRemoteIdentity(std::wstring const & value);
        Common::ErrorCode AddRemoteX509Name(PCCERT_CONTEXT certContext);

        bool IsClientRoleInEffect() const;
        RoleMask::Enum DefaultRemoteRole(bool inbound) const;
        void SetDefaultRemoteRole(RoleMask::Enum roleMask) { defaultRemoteRole_ = roleMask; }

        IdentitySet const & AdminClientIdentities() const;
        void EnableAdminRole(std::wstring const & adminClientIdentities);
        Common::ErrorCode EnableAdminRole(
            std::wstring const & adminClientCertThumbprints,
            Common::SecurityConfig::X509NameMap const & adminClientX509Names,
            std::wstring const & adminClientCommonNames);

        void AddClientToAdminRole(std::wstring const & clientIdentity);
        void AddAdminClientIdentities(IdentitySet const & identities);
        void AddAdminClientX509Names(Common::SecurityConfig::X509NameMap const & names);
        Common::ErrorCode AddAdminClientX509Name(PCCERT_CONTEXT certContext);

        // Enable claim based auth on clients automatically enables role based access control on clients
        Common::ErrorCode EnableClaimBasedAuthOnClients(
            std::wstring const & clientClaimList, // claims for talking to server, string representation of RoleClaimsOrList
            std::wstring const & adminClientClaimList // claims for admin client role, string representation of RoleClaimsOrList
            /* this list internally gets added to 'clientClaimList', so no need to add the same entry to both lists */);

        std::wstring const & LocalClaimToken() const;

        bool ClaimBasedClientAuthEnabled() const;
        RoleClaimsOrList const & ClientClaims() const;
        RoleClaimsOrList const & AdminClientClaims() const;

        bool IsRemotePeer() const { return isRemotePeer_; }
        void EnablePeerToPeerMode() { isRemotePeer_ = true; }

        Common::TimeSpan SessionDuration() const;
        using SessionDurationCallback = std::function<Common::TimeSpan(void)>;
        void SetSessionDurationCallback(SessionDurationCallback callback) { sessionDurationCallback_ = std::move(callback); }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        // For testing only
        void Test_SetRawValues(
            SecurityProvider::Enum provider,
            ProtectionLevel::Enum protectionLevel);

    private:
        static Common::ErrorCode UpdateRemoteIssuersIfNeeded(SecuritySettings &object);

        SecurityProvider::Enum securityProvider_ = SecurityProvider::None;
        ::FABRIC_SECURITY_CREDENTIAL_KIND credTypeFromPublicApi_ = FABRIC_SECURITY_CREDENTIAL_KIND_INVALID;
        ProtectionLevel::Enum protectionLevel_ = ProtectionLevel::EncryptAndSign;

        std::wstring x509CertType_;
        Common::X509StoreLocation::Enum x509StoreLocation_;
        std::wstring x509StoreName_;
        std::shared_ptr<Common::X509FindValue> x509FindValue_;

        Common::CertContextUPtrSPtr certContext_;

        // Expected thumbprints of remote certificates
        mutable Common::ThumbprintSet remoteCertThumbprints_;

        // For SSL/TLS, this is a list of expected common names in remote certificates
        Common::SecurityConfig::X509NameMap remoteX509Names_;

        // Default list of issuer certificate thumbprints for remote certificates
        Common::X509IdentitySet defaultX509IssuerThumbprints_;

        // List of issuers <CN, List of stores> for remoteX509Names_ 
        Common::SecurityConfig::IssuerStoreKeyValueMap remoteCertIssuers_;

        X509CertChainFlagCallback x509CertChainFlagCallback_;
        CrlOfflineIgnoreCallback crlOfflineIgnoreCallback_;
        BooleanFlagCallback framingProtectionEnabledCallback_;
        BooleanFlagCallback readyNewSessionBeforeExpirationCallback_;

        // For Kerberos/Negotiate, on server side, this is a list of authorized client account name or group name
        IdentitySet remoteIdentities_;

        // Service principal name of remote listeners, used by Kerberos
        std::wstring remoteSpn_;

        // Identities of clients in admin role
        Common::SecurityConfig::X509NameMap adminClientX509Names_;
        IdentitySet adminClientIdentities_;
        Common::ThumbprintSet adminClientCertThumbprints_;

        // local token string for claim based security, client side setting
        std::wstring localClaimToken_;

        // expected remote claims
        RoleClaimsOrList clientClaims_; // claims expected for clients
        RoleClaimsOrList adminClientClaims_; // claims expected for admin clients, always contained by clientClaims_

        SessionDurationCallback sessionDurationCallback_;

        RoleMask::Enum defaultRemoteRole_ = RoleMask::All;

        // flags
        bool isRemotePeer_ = false;
        bool isClientRoleInEffect_ = false;
        bool claimBasedClientAuthEnabled_ = false; // server side settings for claim based authentication
        bool isSelfGeneratedCert_ = false;
    };

    typedef std::shared_ptr<SecuritySettings> SecuritySettingsSPtr;
}
