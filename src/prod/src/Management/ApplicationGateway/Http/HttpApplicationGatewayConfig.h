// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IHttpApplicationGatewayConfig.h"
namespace HttpApplicationGateway
{
    class HttpApplicationGatewayConfig : 
        Common::ComponentConfig
        ,public IHttpApplicationGatewayConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(HttpApplicationGatewayConfig, "ApplicationGateway/Http")

        //
        // Enables/Disables the HttpApplicationGateway. HttpApplicationGateway is disabled by default and this config needs to be set to
        // enable it.
        //
        PUBLIC_CONFIG_ENTRY(bool, L"ApplicationGateway/Http", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //
        // Number of reads to post to the http server queue. This controls the number
        // of concurrent requests that can be satisfied by the HttpGateway.
        //
        PUBLIC_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", NumberOfParallelOperations, 5000, Common::ConfigEntryUpgradePolicy::Static);
        //
        // Gives the default request timeout for the http requests being processed in the http app gateway. 
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ApplicationGateway/Http", DefaultHttpRequestTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the connect timeout for the http requests being sent from the http app gateway. 
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ApplicationGateway/Http", HttpRequestConnectTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the default back-off interval before retrying a failed resolve service operation.
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ApplicationGateway/Http", ResolveServiceBackoffInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the size of for the chunk in bytes used to read the body.
        //
        PUBLIC_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", BodyChunkSize, 16384, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Semi colon/ comma separated list of response headers that will be removed from the service response, before forwarding it to the client.
        // If this is set to empty string, pass all the headers returned by the service as-is. i.e do not overwrite the Date and Server headers.
        // 
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", RemoveServiceResponseHeaders, L"Date;Server", Common::ConfigEntryUpgradePolicy::Static);
        //
        // Gives the default max timeout used for resolve calls.
        //
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ApplicationGateway/Http", ResolveTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Lookaside allocator related configs ------------*/
        /* -------------- These should be tuned by looking at the lookside list perf counters --------------*/

        // The number of blocks to initially allocate in the lookaside list. This is the minimum number of allocations the lookaside allocator will keep in its list.
        // This should be atleast equal to the number of parallel operations, assuming each block can cover all the allocs needed for a single operation.
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", InitialNumberOfLookasideAllocations, 20000, Common::ConfigEntryUpgradePolicy::Static);
        // This controls the max number of blocks to keep around over the initial number that is allocated. When there is a sudden spike in the amount
        // of memory needed per request, we might allocate more blocks. But we do not want to keep them around once the spike subsides.
        // This config controls the max number of blocks to keep around over the initial number of allocs. This is given as a % of the initial number of allocations.
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", PercentofExtraAllocsToMaintain, 10, Common::ConfigEntryUpgradePolicy::Static);
        // This controls the allocation block size in KB. For better perf, this should be tuned based on tests or looking at the perf counters so that all allocations for
        // a single request fit in this block size.
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", LookasideAllocationBlockSizeInKb, 140, Common::ConfigEntryUpgradePolicy::Static);

        /* -------------- Websocket related configs -------------- */

        // The max timeout value for the service to complete a websocket handshake initiated by the gateway. 20 Seconds
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", WebSocketOpenHandshakeTimeout, 20000, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the buffer size used for receive buffer
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", WebSocketReceiveBufferSize, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the buffer size used for send buffer.
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", WebSocketSendBufferSize, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // The timeout value for the service to complete a websocket handshake initiated by the gateway. 10 Seconds
        //
        INTERNAL_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", WebSocketCloseHandshakeTimeout, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Security -------------- */

        //
        // Indicates the type of security credentials to use at the http app gateway endpoint
        // Valid values are None/X509
        //
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayAuthCredentialType, L"None", Common::ConfigEntryUpgradePolicy::Static);
        //
        // Name of X.509 certificate store that contains certificate for http app gateway.
        //
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayX509CertificateStoreName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayX509CertificateStoreName, L"My", Common::ConfigEntryUpgradePolicy::Dynamic);
#endif
        //
        // Indicates how to search for certificate in the store specified by GatewayX509CertificateStoreName
        // Supported value: FindByThumbprint, FindBySubjectName
        //
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayX509CertificateFindType, L"FindByThumbprint", Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Search filter value used to locate the http app gateway certificate. This certificate is configured on the https endpoint and can also be used to
        // verify the identity of the app if needed by the services. FindValue is looked up first, and if that doesnt exist, FindValueSecondary is looked up.
        //
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayX509CertificateFindValue, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", GatewayX509CertificateFindValueSecondary, L"", Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // ApplicationCertificateValidationPolicy:
        // None: Do not validate server certificate, succeed the request.
        // ServiceCertificateThumbprints: Refer to config ServiceCertificateThumbprints for the comma separated list of thumbprints of the remote certs that the reverse proxy can trust.
        // ServiceCommonNameAndIssuer:  Refer to config ServiceCommonNameAndIssuer for the subject name and issuer thumbprint of the remote certs that the reverse proxy can trust.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", ApplicationCertificateValidationPolicy, L"None", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", ServiceCertificateThumbprints, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(Common::SecurityConfig::X509NameMap, L"ApplicationGateway/Http/ServiceCommonNameAndIssuer", ServiceCommonNameAndIssuer, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // Flags for application/service certificate chain validation, e.g. CRL checking
        // 0x10000000 CERT_CHAIN_REVOCATION_CHECK_END_CERT
        // 0x20000000 CERT_CHAIN_REVOCATION_CHECK_CHAIN
        // 0x40000000 CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT
        // 0x80000000 CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY
        // Setting to 0 disables CRL checking
        // Full list of supported values is documented by dwFlags of CertGetCertificateChain:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa376078(v=vs.85).aspx
        //
        PUBLIC_CONFIG_ENTRY(uint, L"ApplicationGateway/Http", CrlCheckingFlag, 0x40000000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Whether to ignore CRL offline error for application/service certificate verification. 
        PUBLIC_CONFIG_ENTRY(bool, L"ApplicationGateway/Http", IgnoreCrlOfflineError, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // SecureOnlyMode:
        // true: Reverse Proxy will only forward to services that publish secure endpoints.
        // false: Reverse Proxy can forward requests to secure/non-secure endpoints. 
        //
        PUBLIC_CONFIG_ENTRY(bool, L"ApplicationGateway/Http", SecureOnlyMode, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        PUBLIC_CONFIG_ENTRY(bool, L"ApplicationGateway/Http", ForwardClientCertificate, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Test configs -------------- */

        //
        // Used for testing the application gateway in a standalone manner.
        //
        TEST_CONFIG_ENTRY(bool, L"ApplicationGateway/Http", StandAloneMode, false, Common::ConfigEntryUpgradePolicy::NotAllowed);
        //
        // Gives the protocol the gateway should use when running in standalone mode.
        //
        TEST_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", Protocol, L"http", Common::ConfigEntryUpgradePolicy::NotAllowed);
        //
        // Gives the listen address the gateway should use when running in standalone mode.
        //
        TEST_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", ListenAddress, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);
        //
        // Gives the static address that the dummy service resolver returns when running with StandAloneMode set to true
        //
        TEST_CONFIG_ENTRY(std::wstring, L"ApplicationGateway/Http", TargetResolutionAddress, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);

        public:

        std::unordered_map<std::wstring, bool, CaseInsensitiveHasher, CaseInsensitiveEqualFn> const & ResponseHeadersToRemove() const
        {
            return ResponseHeadersToRemoveMap;
        }

        CertValidationPolicy::Enum GetCertValidationPolicy() const
        {
            return CertValidationPolicyVal;
        }

        std::unordered_map<std::wstring, bool, CaseInsensitiveHasher, CaseInsensitiveEqualFn> ResponseHeadersToRemoveMap;
        CertValidationPolicy::Enum CertValidationPolicyVal;
        void Initialize() override;
    };
}
