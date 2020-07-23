// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayConfig
        : Common::ComponentConfig
#if !defined (PLATFORM_UNIX)
        , public HttpApplicationGateway::IHttpApplicationGatewayConfig
#endif
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(HttpGatewayConfig, "HttpGateway")

        //
        // Enables/Disables the HttpGateway. HttpGateway is disabled by default.
        //
        PUBLIC_CONFIG_ENTRY(bool, L"HttpGateway", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //
        // Number of reads to post to the http server queue. This controls the number
        // of concurrent requests that can be satisfied by the HttpGateway.
        //
        PUBLIC_CONFIG_ENTRY(uint, L"HttpGateway", ActiveListeners, 500, Common::ConfigEntryUpgradePolicy::Static);
        //
        // Controls if API version checking is enforced or not.
        //
        INTERNAL_CONFIG_ENTRY(bool, L"HttpGateway", VersionCheck, true, Common::ConfigEntryUpgradePolicy::Static);
        //
        // Gives the maximum size of the body that can be expected from a http request. Default value is 4MB.
        // HttpGateway will fail a request if it has a body of size > this value.
        // Minimum read chunk size is 4096 bytes. So this has to be >= 4096.
        //
        PUBLIC_CONFIG_ENTRY(uint, L"HttpGateway", MaxEntityBodySize, 4194304, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // The interval at which the HttpGateway sends accumulated health reports to the Health Manager. Default: 30 seconds.
        // Report batching helps optimize the messages sent to health store.
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"HttpGateway", HttpGatewayHealthReportSendInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);

        //
        // Specify the HTTP Strict Transport Security header value to be included in every response sent by the HttpGateway.
        // When set to empty string, this header will not be included in the gateway response.
        //
        PUBLIC_CONFIG_ENTRY(std::wstring, L"HttpGateway", HttpStrictTransportSecurityHeader, L"", Common::ConfigEntryUpgradePolicy::Dynamic);

        // Whether or not the volume GET query results are returned in preview format
        TEST_CONFIG_ENTRY(bool, L"HttpGateway", UsePreviewFormatForVolumeQueryResults, true, Common::ConfigEntryUpgradePolicy::Dynamic);

#if !defined (PLATFORM_UNIX)
        // HttpApplicationGateway settings
        //
        // Gives the default request timeout for the http requests being processed in the http app gateway. 
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"HttpGateway", DefaultHttpRequestTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the connect timeout for the http requests being sent from the http app gateway. 
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"HttpGateway", HttpRequestConnectTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the default back-off interval before retrying a failed resolve service operation.
        //
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"HttpGateway", ResolveServiceBackoffInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the size of for the chunk in bytes used to read the body.
        //
        PUBLIC_CONFIG_ENTRY(uint, L"HttpGateway", BodyChunkSize, 16384, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Semi colon/ comma separated list of response headers that will be removed from the service response, before forwarding it to the client.
        // If this is set to empty string, pass all the headers returned by the service as-is. i.e do not overwrite the Date and Server headers.
        // 
        PUBLIC_CONFIG_ENTRY(std::wstring, L"HttpGateway", RemoveServiceResponseHeaders, L"Date;Server", Common::ConfigEntryUpgradePolicy::Static);
        //
        // Gives the default max timeout used for resolve calls.
        //
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"HttpGateway", ResolveTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // This controls the allocation block size in KB. For better perf, this should be tuned based on tests or looking at the perf counters so that all allocations for
        // a single request fit in this block size.
        //

        /* -------------- Websocket related configs -------------- */

        // The max timeout value for the service to complete a websocket handshake initiated by the gateway. 20 Seconds
        //
        INTERNAL_CONFIG_ENTRY(uint, L"HttpGateway", WebSocketOpenHandshakeTimeout, 20000, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the buffer size used for receive buffer
        //
        INTERNAL_CONFIG_ENTRY(uint, L"HttpGateway", WebSocketReceiveBufferSize, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // Gives the buffer size used for send buffer.
        //
        INTERNAL_CONFIG_ENTRY(uint, L"HttpGateway", WebSocketSendBufferSize, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        //
        // The timeout value for the service to complete a websocket handshake initiated by the gateway. 10 Seconds
        //
        INTERNAL_CONFIG_ENTRY(uint, L"HttpGateway", WebSocketCloseHandshakeTimeout, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Security -------------- */
        //
        // ApplicationCertificateValidationPolicy:
        // None: Do not validate server certificate, succeed the request.
        // ServiceCertificateThumbprints: Refer to config ServiceCertificateThumbprints for the comma separated list of thumbprints of the remote certs that the reverse proxy can trust.
        // ServiceCommonNameAndIssuer:  Refer to config ServiceCommonNameAndIssuer for the subject name and issuer thumbprint of the remote certs that the reverse proxy can trust.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"HttpGateway", ApplicationCertificateValidationPolicy, L"None", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"HttpGateway", ServiceCertificateThumbprints, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_GROUP(Common::SecurityConfig::X509NameMap, L"HttpGateway/ServiceCommonNameAndIssuer", ServiceCommonNameAndIssuer, Common::ConfigEntryUpgradePolicy::Dynamic);

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
        PUBLIC_CONFIG_ENTRY(uint, L"HttpGateway", CrlCheckingFlag, 0x40000000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Whether to ignore CRL offline error for application/service certificate verification. 
        PUBLIC_CONFIG_ENTRY(bool, L"HttpGateway", IgnoreCrlOfflineError, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // SecureOnlyMode:
        // true: Reverse Proxy will only forward to services that publish secure endpoints.
        // false: Reverse Proxy can forward requests to secure/non-secure endpoints. 
        //
        PUBLIC_CONFIG_ENTRY(bool, L"HttpGateway", SecureOnlyMode, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        PUBLIC_CONFIG_ENTRY(bool, L"HttpGateway", ForwardClientCertificate, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        std::unordered_map<std::wstring, bool, HttpApplicationGateway::CaseInsensitiveHasher, HttpApplicationGateway::CaseInsensitiveEqualFn> const & ResponseHeadersToRemove() const
        {
            return ResponseHeadersToRemoveMap;
        }

        HttpApplicationGateway::CertValidationPolicy::Enum GetCertValidationPolicy() const
        {
            return CertValidationPolicyVal;
        }

        std::unordered_map<std::wstring, bool, HttpApplicationGateway::CaseInsensitiveHasher, HttpApplicationGateway::CaseInsensitiveEqualFn> ResponseHeadersToRemoveMap;
        HttpApplicationGateway::CertValidationPolicy::Enum CertValidationPolicyVal;
        void Initialize() override;
#endif
    };
}
