// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class SecurityConfig : public ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(SecurityConfig, "Security")

    public:
        struct X509NameIsLess
        {
            bool operator()(std::wstring const & a, std::wstring const & b) const
            {
#ifdef PLATFORM_UNIX 
                //LINUXTODO compare string based on locale
                return StringUtility::IsLessCaseInsensitive(a, b);
#else
                return ::CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a.c_str(), -1, b.c_str(), -1) == CSTR_LESS_THAN;
#endif
            }
        };

        using X509NameMapBase = std::map<std::wstring, Common::X509IdentitySet, X509NameIsLess>;

        class X509NameMap : X509NameMapBase
        {
        public:
            static X509NameMap Parse(Common::StringMap const & entries);

            X509NameMap();

            bool Match(
                std::wstring const & name,
                X509Identity::SPtr const & issuerId,
                _Out_ X509NameMap::const_iterator & matched) const;

            bool Match(
                std::wstring const & name,
                X509Identity::SPtr const & issuerId1,
                X509Identity::SPtr const & issuerId2,
                _Out_ X509NameMapBase::const_iterator & matched) const;

            static bool MatchIssuer(
                X509Identity::SPtr const & issuerId1,
                X509NameMapBase::const_iterator toMatch);

            static bool MatchIssuer(
                X509Identity::SPtr const & issuerId1,
                X509Identity::SPtr const & issuerId2,
                X509NameMapBase::const_iterator toMatch);

            bool operator == (X509NameMap const & rhs) const;
            bool operator != (X509NameMap const & rhs) const;

            Common::ErrorCode ParseError() const;

            void Add(std::wstring const & name, X509IdentitySet const & certIssuers = X509IdentitySet(), bool overwrite = false);
            Common::ErrorCode Add(std::wstring const & name, std::wstring const & issuerCertThumbprints);
            void Add(std::wstring const & name, X509Identity::SPtr const & issuerId);
            void Add(X509NameMap const & rhs);
            Common::ErrorCode AddNames(std::wstring const & names, std::wstring const & issuerCertThumbprints);
            Common::ErrorCode AddNames(PCFABRIC_X509_NAME x509Names, ULONG count);

            Common::ErrorCode SetDefaultIssuers(X509IdentitySet const & certIssuers);
            Common::ErrorCode OverwriteIssuers(X509IdentitySet const & certIssuers);
            X509IdentitySet GetAllIssuers() const;

            std::vector<std::pair<std::wstring, std::wstring>> ToVector() const;

            auto Size() const { return size(); }
            bool IsEmpty() const { return empty(); }

            auto CBegin() const { return cbegin(); }
            auto CEnd() const { return cend(); }

            auto Find(std::wstring const & name) const { return find(name); }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        private:
            Common::ErrorCodeValue::Enum parseError_;
        };

        struct X509IssuerStoreNameIsLess
        {
            bool operator()(std::wstring const & a, std::wstring const & b) const
            {
#ifdef PLATFORM_UNIX 
                //LINUXTODO compare string based on locale
                return StringUtility::IsLessCaseSensitive(a, b);
#else
                return ::CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a.c_str(), -1, b.c_str(), -1) == CSTR_LESS_THAN;
#endif
            }
        };

        class IssuerStoreKeyValueMap : std::map<std::wstring, std::set<wstring, X509IssuerStoreNameIsLess>, X509NameIsLess>
        {
          public:
            static IssuerStoreKeyValueMap Parse(Common::StringMap const & entries);
            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            Common::ErrorCode GetPublicKeys(_Out_ X509IdentitySet & publicKeys) const;
            void Add(std::wstring const & name, std::set<std::wstring, X509IssuerStoreNameIsLess> const & issuerStores);
            void Add(std::wstring const & name, std::wstring const & issuerStore);
            void AddIssuerEntries(PCFABRIC_X509_ISSUER_NAME x509Issuers, ULONG count);
            void ToPublicApi(Common::ScopedHeap & heap, FABRIC_X509_CREDENTIALS_EX3 & x509Ex3) const;

            bool operator == (IssuerStoreKeyValueMap const & rhs) const;
            bool operator != (IssuerStoreKeyValueMap const & rhs) const;

            auto Size() const { return size(); }
            bool IsEmpty() const { return empty(); }
            auto CBegin() const { return cbegin(); }
            auto CEnd() const { return cend(); }
            auto Find(std::wstring const & name) const { return find(name); }
        };

        /* -------------- Configuration Security -------------- */

        // X509 certificate store used by fabric for configuration protection
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", SettingsX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", SettingsX509StoreName, L"MY", ConfigEntryUpgradePolicy::Dynamic);
#endif

        // Use secondary X509 certificate if its expiration is newer than primary certificate
        INTERNAL_CONFIG_ENTRY(bool, L"Security", UseSecondaryIfNewer, false, ConfigEntryUpgradePolicy::Static);

        /* -------------- Transport Security ------------------ */

        // Security negotiation timeout, deprecated by Transport/ConnectionOpenTimeout 
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Security", NegotiationTimeout, TimeSpan::FromSeconds(60), ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));

        // Expiration for a secure session, set to 0 to disable session expiration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SessionExpiration, TimeSpan::FromSeconds(60 * 60 * 24 * 7), ConfigEntryUpgradePolicy::Dynamic);
        // Establish a new session before retiring the current one
        INTERNAL_CONFIG_ENTRY(bool, L"Security", ReadyNewSessionBeforeExpiration, true, ConfigEntryUpgradePolicy::Dynamic);
        // Timeout for session refresh, should be longer than ConnectOpenTimeout
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SessionRefreshTimeout, Common::TimeSpan::FromSeconds(70), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanGreaterThan(Common::TimeSpan::FromSeconds(30)));
        // Retry delay of session refresh at session expiration (does not affect session refresh at security setting update)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SessionRefreshRetryDelay, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanGreaterThan(Common::TimeSpan::FromMinutes(1)));
        // Close delay at session expiration to avoid dropping messages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SessionExpirationCloseDelay, Common::TimeSpan::FromMinutes(3), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        // Max delay before closing existing secure session, after security settings are updated.
        // The random delay is added to avoid a burst of new session negotiation after the updates.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", MaxSessionRefreshDelay, TimeSpan::FromSeconds(300), ConfigEntryUpgradePolicy::Dynamic);
        // Close delay for authentication/authorization error feedback to be delivered to remote side.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Security", AuthErrorCloseDelay, TimeSpan::FromSeconds(3), ConfigEntryUpgradePolicy::Dynamic);
        // Use a small max message size before security negotiation is completed
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", MaxMessageSizeBeforeSessionIsSecured, 128 * 1024, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(1024, 1024 * 1024));
        // Max data chunk size for SSPI EncryptMessage/MakeSignature for security provider Negotiate
        // In general, larger value means less overhead, but a value too large runs the risk of DOS,
        // as chunk size is not protected on wire.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", WindowsSecurityMaxEncryptSize, 16 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(1024, 128 * 1024 * 1024));
        //specify threshold for what's considered as slow API call
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SlowApiThreshold, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", SlowApiHealthReportTtl, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Cluster Security -------------- */

        // Indicates the type of security credentials to use in order to secure the cluster.
        // Valid values are "None/X509/Windows"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClusterCredentialType, L"None", ConfigEntryUpgradePolicy::NotAllowed);
        // The string value used to specify whether the messages (header and body) have
        // integrity and confidentiality guarantees applied to them when sent between the
        // nodes of a cluster.
        // Valid values are "None/Sign/EncryptAndSign"
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", ClusterProtectionLevel, L"EncryptAndSign", ConfigEntryUpgradePolicy::NotAllowed);
        // Allows secondary thumbprint in whitelist
        INTERNAL_CONFIG_ENTRY(bool, L"Security", AllowDefaultClient, true, ConfigEntryUpgradePolicy::Static);

        /* -------------- Client Server Security -------------- */

        // Indicates the type of security credentials to use in order to secure the
        // communication between FabricClient and the Cluster.
        // Valid values are "None/X509/Windows"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ServerAuthCredentialType, L"None", ConfigEntryUpgradePolicy::Static);
        // The string value used to specify whether the messages (header and body) have  integrity
        // and confidentiality guarantees applied to them when sent between client and server.
        // Valid values are "None/Sign/EncryptAndSign"
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", ClientServerProtectionLevel, L"EncryptAndSign", ConfigEntryUpgradePolicy::Static);

#ifndef PLATFORM_UNIX
        // Whether transport framing data should be protected after secure session is established
        INTERNAL_CONFIG_ENTRY(bool, L"Security", FramingProtectionEnabled, false, ConfigEntryUpgradePolicy::Dynamic);
        // Whether transport framing data should be protected after secure session is established, in peer-to-peer mode transport, e.g. federation and replication
        INTERNAL_CONFIG_ENTRY(bool, L"Security", FramingProtectionEnabledInPeerToPeerMode, true, ConfigEntryUpgradePolicy::Dynamic);
        // Whether transport framing data should be protected after secure session is established, in client mode transport
        INTERNAL_CONFIG_ENTRY(bool, L"Security", FramingProtectionEnabledInClientMode, true, ConfigEntryUpgradePolicy::Dynamic);
#endif

        // Whether to add our own message framing on top of TLS/SSL record protocol for data messages after session is secured.
        // Before session is secured, such extra framing is always added for security negotiation messages.
        DEPRECATED_CONFIG_ENTRY(bool, L"Security", X509ExtraFramingEnabled, true, ConfigEntryUpgradePolicy::Dynamic);
        // Indicates if client role is enabled, when set to true, clients are assigned roles based on their
        // identities. For V2, enabling this means clients not in AdminClientCommonNames/AdminClientIdentities
        // can only execute read-only operations. Client role should always be enabled for new deployments, this
        // config is kept only for backward compatibility with old clusters that do not have client roles.
        PUBLIC_CONFIG_ENTRY(bool, L"Security", ClientRoleEnabled, false, ConfigEntryUpgradePolicy::Static);
        //Code outside SecurityConfig should call IsClientRoleInEffect() below instead of reading ClientRoleEnabled directly

        // Whether server side should send connection authorization status to clients
        DEPRECATED_CONFIG_ENTRY(bool, L"Security", ServerShouldSendConnectionAuthStatus, true, ConfigEntryUpgradePolicy::Dynamic);
        // Timeout for waiting for connection authorization status
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Security", MaxWaitForConnectionAuthStatus, TimeSpan::FromSeconds(30), ConfigEntryUpgradePolicy::Dynamic);
        // Timeout for waiting for connection authorization status, when talking to pre-3.1 servers that does not send connection authorization status to clients.
        // This can be removed after all clusters are upgraded to 3.1 or beyond.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Security", MaxWaitForConnectionAuthStatusIfNotPromised, TimeSpan::FromSeconds(1), ConfigEntryUpgradePolicy::Dynamic);

        /* X509 specific */

        // When set true, local public key is implicitly added to allow list, remote is trusted if its public key is the same as the local one
        INTERNAL_CONFIG_ENTRY(bool, L"Security", ImplicitPeerTrustEnabled, true, ConfigEntryUpgradePolicy::Dynamic);

        // Thumbprints of certificates allowed to join the cluster, a comma separated name list.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClusterCertThumbprints, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Thumbprints of server certificates used by cluster to talk to clients, clients use this to authenticate the cluster.
        // It is a comma separated name list.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ServerCertThumbprints, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Thumbprints of certificates used by clients to talk to the cluster, cluster uses this authorize incoming connection.
        // It is a comma separated name list.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClientCertThumbprints, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Thumbprints of certificates used by clients in admin role.
        // It is a comma separated name list.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AdminClientCertThumbprints, L"", ConfigEntryUpgradePolicy::Dynamic);

        // Certificate common names of remote subjects allowed to join the cluster, a setting used by
        // cluster node to recognize other peer cluster nodes.
        // It is a comma separated name list.
        // Deprecated by ClusterX509Names
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ClusterAllowedCommonNames, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(X509NameMap, L"Security/ClusterX509Names", ClusterX509Names, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Certificate common names of expected naming gateway servers, a setting used by default fabric clients.
        // It is a comma separated name list.
        // Deprecated by ServerX509Names
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ServerAuthAllowedCommonNames, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(X509NameMap, L"Security/ServerX509Names", ServerX509Names, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Certificate common names of fabric clients allowed to talk to naming gateway, a setting used
        // by naming gateway to authorize fabric clients. It is a comma separated name list, common name
        // of default fabric client is automatically added to the list.
        // Deprecated by ClientX509Names, "ClientCertificate" will not be allowed implicitly.
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ClientAuthAllowedCommonNames, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(X509NameMap, L"Security/ClientX509Names", ClientX509Names, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Certificate common names of fabric clients in admin role, used to authorize privileged fabric operations.
        // It is a comma separated list.
        // Deprecated by AdminClientX509Names, "ClientCertificate" will not be allowed implicitly.
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", AdminClientCommonNames, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(X509NameMap, L"Security/AdminClientX509Names", AdminClientX509Names, Common::ConfigEntryUpgradePolicy::Dynamic);

        // X509 certificate issuers for cluster certificates, a comma seperated thumbprint list.
        // Issuer Checking is disabled when the list is empty, which is the default.
        // ClusterX509Names supports per-name issuer pinning
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ClusterCertIssuers, L"", ConfigEntryUpgradePolicy::Dynamic);
        // X509 certificate issuers for server certificates, a comma seperated thumbprint list.
        // Issuer Checking is disabled when the list is empty, which is the default.
        // ServerX509Names supports per-name issuer pinning
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ServerCertIssuers, L"", ConfigEntryUpgradePolicy::Dynamic);
        // X509 certificate issuers for client certificates, a comma seperated thumbprint list.
        // Issuer Checking is disabled when the list is empty, which is the default.
        // ClientX509Names and AdminClientX509Name support per-name issuer pinning
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", ClientCertIssuers, L"", ConfigEntryUpgradePolicy::Dynamic);

        // X509 issuer certificate stores for cluster certificates, Name = clusterIssuerCN, Value = comma separated list of stores
        PUBLIC_CONFIG_GROUP(IssuerStoreKeyValueMap, L"Security/ClusterCertificateIssuerStores", ClusterCertificateIssuerStores, Common::ConfigEntryUpgradePolicy::Dynamic);
        // X509 issuer certificate stores for server certificates, Name = serverIssuerCN, Value = comma separated list of stores
        PUBLIC_CONFIG_GROUP(IssuerStoreKeyValueMap, L"Security/ServerCertificateIssuerStores", ServerCertificateIssuerStores, Common::ConfigEntryUpgradePolicy::Dynamic);
        // X509 issuer certificate stores for client certificates, Name = clientIssuerCN, Value = comma separated list of stores
        PUBLIC_CONFIG_GROUP(IssuerStoreKeyValueMap, L"Security/ClientCertificateIssuerStores", ClientCertificateIssuerStores, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Whether to use cluster certificate to secure IPC Server TLS transport unit
        PUBLIC_CONFIG_ENTRY(bool, L"Security", UseClusterCertForIpcServerTlsSecurity, false, ConfigEntryUpgradePolicy::Static);
        
        // Bitmask for default certificate chain validation, may be overridden by component specific flag, e.g. Federation/X509CertChainFlags
        // Values often used:
        // 0x80000000 CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY: disable certificate revocation list (CRL) downloading when unavailable
        // 0x00000004 CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL : disable certificate trust list (CTL) downloading on Windows machines
        // lacking Internet access. Details about CTL can be found at:
        // https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/dn265983(v=ws.11)
        // Setting to 0 disables CRL checking, but it does not disable CTL downloading.
        // Full list of supported values is documented by dwFlags of CertGetCertificateChain:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa376078(v=vs.85).aspx
#ifdef PLATFORM_UNIX
        // Disable CRL checking on Linux until CRL installation and update issues are figured out
        PUBLIC_CONFIG_ENTRY(uint, L"Security", CrlCheckingFlag, 0, ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(uint, L"Security", CrlCheckingFlag, 0x40000000, ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Timeout for retrieving CRL, accumulative when there are multiple CRLs to retrieve for a given chain. For details, refer to 
        // CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT in http://msdn.microsoft.com/en-us/library/windows/desktop/aa376078(v=vs.85).aspx
        // Default is zero, which means system default timeout value will be used.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlRetrivalTimeout, Common::TimeSpan::Zero, ConfigEntryUpgradePolicy::Dynamic, Common::InRange<Common::TimeSpan>(Common::TimeSpan::Zero, Common::TimeSpan::FromMilliseconds(std::numeric_limits<DWORD>().max())));

        // Whether to ignore CRL offline error when server side verifies incoming client certifiates
        PUBLIC_CONFIG_ENTRY(bool, L"Security", IgnoreCrlOfflineError, false, ConfigEntryUpgradePolicy::Dynamic);
        // Whether to ignore CRL offline error when client side verifies incoming server certifiates, default to true.
        // Attacks with revoked server certifidates require compromising DNS, harder than with revoked client certificates.
        PUBLIC_CONFIG_ENTRY(bool, L"Security", IgnoreSvrCrlOfflineError, true, ConfigEntryUpgradePolicy::Dynamic);

        // How long CRL checking is disabled for a given certificate after encountering offline error, if CRL offline error can be ignored. 
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlDisablePeriod, Common::TimeSpan::FromMinutes(15), ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(uint, L"Security", CrlOfflineErrCacheCapacity, 10000, Common::ConfigEntryUpgradePolicy::Dynamic, Common::UIntNoLessThan(32));
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlOfflineHealthReportTtl, Common::TimeSpan::FromMinutes(1440), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanNoLessThan(Common::TimeSpan::FromMinutes(30)));
        // For a given certificate thumbprint, a new error is only reported if it is newer than the existing one by this threshold value 
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlOfflineReportThreshold, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        TEST_CONFIG_ENTRY(bool, L"Security", CrlTestEnabled, false, ConfigEntryUpgradePolicy::Dynamic);
        TEST_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlTestStartDelay, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        TEST_CONFIG_ENTRY(Common::TimeSpan, L"Security", CrlTestPeriod, Common::TimeSpan::FromMinutes(10000), Common::ConfigEntryUpgradePolicy::Dynamic);
        TEST_CONFIG_ENTRY(double, L"Security", CrlOfflineProbability, 0.1, ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));

        // Local certificate chain verification makes things easier for investigating certificate related failures,
        // but in failure cases, it prevents the execution of remote certificate checking logic, which security really
        // depends on, setting this to false enables testing of remote certificate checking.
        DEPRECATED_CONFIG_ENTRY(bool, L"Security", LocalCertCheckingEnabled, false, ConfigEntryUpgradePolicy::Static);
        // To allow testing with expired certificates, SkipX509CredentialExpirationChecking needs to be set true, besides setting LocalCertCheckingEnabled false.
        INTERNAL_CONFIG_ENTRY(bool, L"Security", SkipX509CredentialExpirationChecking, false, ConfigEntryUpgradePolicy::Static);

        // How often certificate update is checked.
        // This is used to detect certificate credential update where X509FindType & X509FindValue stay unchanged:
        // 1. certificate renewed and FindBySubjectName is used.
        // 2. find value has a primary and a secondary, one of them is being installed or uninstalled
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", CertificateMonitorInterval, Common::TimeSpan::FromMinutes(60), ConfigEntryUpgradePolicy::Static);

        // Specify interval for certificate health reporting, default to 8 hours,
        // setting to 0 disables certificate health reporting
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Security", CertificateHealthReportingInterval, Common::TimeSpan::FromSeconds(3600 * 8), ConfigEntryUpgradePolicy::Static);

        // Safety margin for certificate expiration, certificate health report status changes from OK to Warning when expiration is closer than this.
        // Default is 30 days.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Security", CertificateExpirySafetyMargin, Common::TimeSpan::FromMinutes(43200), ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        /* Claim based authentication settings */

        // Indicates if claim based authentication is enabled on clients
        PUBLIC_CONFIG_ENTRY(bool, L"Security", ClientClaimAuthEnabled, false, ConfigEntryUpgradePolicy::Static);

    public:
        // client role is in effect when either ClientRoleEnabled or ClientClaimAuthEnabled is set.
        // As mentioned above, ClientRoleEnabled was kept for backward compatibility. Client role
        // should always be enabled for new deployments. Client claims auth support was added after
        // client role, thus ClientClaimAuthEnabled implies client role is enabled. Such implicit client
        // role with client claims auth has been made pubilc since the beginning of claims auth. 
        // We cannot rename existing config setting ClientRoleEnabled, the following method has to
        // be named differently to avoid conflict, although intention of both is the same.
        bool IsClientRoleInEffect() { return ClientRoleEnabled || ClientClaimAuthEnabled; } 

        // All possible claims expected from clients for connecting to gateway.
        // This is a 'OR' list: ClaimsEntry || ClaimsEntry || ClaimsEntry ... each ClaimsEntry is
        // a "AND" list: ClaimType=ClaimValue && ClaimType=ClaimValue && ClaimType=ClaimValue ...
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClientClaims, L"", ConfigEntryUpgradePolicy::Dynamic);
        // All possible claims expected from admin clients, the same format as ClientClaims, this list
        // internally gets added to ClientClaims, so no need to also add the same entries to ClientClaims.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AdminClientClaims, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Indicates that the dummy authenticator should be used. This is used by FabricTest.
        TEST_CONFIG_ENTRY(bool, L"Security", UseTestClaimsAuthenticator, false, ConfigEntryUpgradePolicy::Static);

        /* Windows security */

        // Service principal name of the cluster, when fabric runs as a single domain user (gMSA/domain user account).
        // It is the SPN of lease listeners and listeners in fabric.exe: federation listeners, internal replication 
        // listeners, runtime service listener and naming gateway listener. This should be left empty when fabric runs
        // as machine accounts, in which case connecting side compute listener SPN from listener transport address.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClusterSpn, L"", ConfigEntryUpgradePolicy::NotAllowed);
        // Windows identities of cluster nodes, used for cluster membership authorization.
        // It is a comma separated list, each entry is a domain account name or group name
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClusterIdentities, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Windows identities of FabricClient, naming gateway uses this to authorize incoming connections.
        // It is a comma separated list, each entry is a domain account name or group name. For convenience,
        // the account that runs fabric.exe is automatically allowed, so are group ServiceFabricAllowedUsers and ServiceFabricAdministrators.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", ClientIdentities, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Windows identities of fabric clients in admin role, used to authorize privileged fabric operations.
        // It is a comma separated list, each entry is a domain account name or group name. For convenience,
        // the account that runs fabric.exe is automatically assigned admin role, so is group ServiceFabricAdministrators.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AdminClientIdentities, L"", ConfigEntryUpgradePolicy::Dynamic);
        // Use "Negotiate" as the security provider for "Windows" security. By default, "Kerberos" is used to 
        // be backward compatible with existing clusters wth "Windows" security. The default value will be changed in future milestones.
        INTERNAL_CONFIG_ENTRY(bool, L"Security", NegotiateForWindowsSecurity, true, ConfigEntryUpgradePolicy::NotAllowed);
        // Specifies whether mutual-auth is required for non-loopback traffic, for windows security
        INTERNAL_CONFIG_ENTRY(bool, L"Security", MutualAuthRequiredForWindowsSecurity, false, ConfigEntryUpgradePolicy::Dynamic);
        // Service principal name of FabricHost, when fabric runs as a single domain user (gMSA/domain user account) and FabricHost runs under machine account.
        // It is the SPN of IPC listener for FabricHost, which by default should be left empty since FabricHost runs under machine account 
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", FabricHostSpn, L"", ConfigEntryUpgradePolicy::Static);
        // Control the rate of tracing remote group SIDs on auth failure, there may be many group SIDs associcated with an incoming token
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", RemoteGroupSidTraceInterval, Common::TimeSpan::FromSeconds(300), ConfigEntryUpgradePolicy::Dynamic);

        //Indicates if firewall rule should not be enabled for public profile
        PUBLIC_CONFIG_ENTRY(bool, L"Security", DisableFirewallRuleForPublicProfile, true, ConfigEntryUpgradePolicy::Static);
        //Indicates if firewall rule should not be enabled for private profile
        PUBLIC_CONFIG_ENTRY(bool, L"Security", DisableFirewallRuleForPrivateProfile, true, ConfigEntryUpgradePolicy::Static);
        //Indicates if firewall rule should not be enabled for domain profile
        PUBLIC_CONFIG_ENTRY(bool, L"Security", DisableFirewallRuleForDomainProfile, true, ConfigEntryUpgradePolicy::Static);

        /* Azure Active Directory (Public) */

    public:
        std::map<std::wstring, std::wstring> GetDefaultAzureActiveDirectoryConfigurations();

        // Tenant ID (GUID)
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AADTenantId, L"", ConfigEntryUpgradePolicy::Static);
        // Web API application name or ID representing the cluster
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AADClusterApplication, L"", ConfigEntryUpgradePolicy::Static);
        // Native Client application name or ID representing Fabric Clients
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security", AADClientApplication, L"", ConfigEntryUpgradePolicy::Static);

        /* Azure Active Directory (Internal) */

        // Native Client redirect URI
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADClientRedirectUri, L"urn:ietf:wg:oauth:2.0:oob", ConfigEntryUpgradePolicy::Static);
        // Endpoint for AAD login (used for adal.js initialization in SFX)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADLoginEndpoint, L"https://login.microsoftonline.com", ConfigEntryUpgradePolicy::Static);
        // Endpoint for acquiring security tokens
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADTokenEndpointFormat, L"https://login.microsoftonline.com/{0}", ConfigEntryUpgradePolicy::Static);
        // Security token issuer
        // Deprecated by extracting from federation xml
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"Security", AADTokenIssuerFormat, L"https://sts.windows.net/{0}/", ConfigEntryUpgradePolicy::Static);
        // Endpoint for getting token-signing certificates
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADCertEndpointFormat, L"https://login.microsoftonline.com/{0}/federationmetadata/2007-06/federationmetadata.xml", ConfigEntryUpgradePolicy::Static);
        // Application Role claim key
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADRoleClaimKey, L"http://schemas.microsoft.com/ws/2008/06/identity/claims/role", ConfigEntryUpgradePolicy::Static);
        // Application Role claim value for Admin role
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADAdminRoleClaimValue, L"Admin", ConfigEntryUpgradePolicy::Static);
        // Application Role claim value for User role
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Security", AADUserRoleClaimValue, L"User", ConfigEntryUpgradePolicy::Static);
        // Token signing cert rollover check interval
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Security", AADSigningCertRolloverCheckInterval, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));

#if defined(PLATFORM_UNIX)
        // Folder where X509 certificates and private keys are located 
        PUBLIC_CONFIG_ENTRY(std::string, L"Security", X509Folder, "/var/lib/sfcerts", ConfigEntryUpgradePolicy::Static);
        // Folder where AppRunAsAccountGroup X509 certificates and private keys are located 
        PUBLIC_CONFIG_ENTRY(std::string, L"Security", AppRunAsAccountGroupX509Folder, "/home/sfuser/sfusercerts", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", CertFileExtension, ".crt", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", PrivateKeyFileExtension, ".prv", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", X509InstallExtension, ".pem", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", PrivateKeyInstallExtension, ".pem", ConfigEntryUpgradePolicy::Static);
        // Folder used as currentuser X509Store in CoreClr
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", CoreFxX509UserCertificatesPath, ".dotnet/corefx/cryptography/x509stores/my", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::string, L"Security", CoreFxX509CertFileExtension, ".pfx", ConfigEntryUpgradePolicy::Static);
#endif

    };
}
