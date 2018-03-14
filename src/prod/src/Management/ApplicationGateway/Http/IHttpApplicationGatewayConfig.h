// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    struct CaseInsensitiveHasher
    {
        size_t operator() (const std::wstring& key) const
        {
            std::wstring keyCopy(key);
            Common::StringUtility::ToLower(keyCopy);
            return Common::StringUtility::GetHash(keyCopy);
        };
    };

    struct CaseInsensitiveEqualFn
    {
        bool operator()(const std::wstring& a, const std::wstring& b) const
        {
            return Common::StringUtility::AreEqualCaseInsensitive(a, b);
        }
    };

    namespace CertValidationPolicy
    {
        enum Enum : unsigned int
        {
            None = 0,
            ServiceCertificateThumbprints = 1,
            ServiceCommonNameAndIssuer = 2
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        Common::ErrorCode TryParse(std::wstring const & stringInput, _Out_ Enum & certValidationPolicy);
    }

    class IHttpApplicationGatewayConfig
    {
    public:
        virtual ~IHttpApplicationGatewayConfig() = default;

        __declspec (property(get = get_DefaultHttpRequestTimeout)) Common::TimeSpan const & DefaultHttpRequestTimeout;
        virtual Common::TimeSpan get_DefaultHttpRequestTimeout() const = 0;

        __declspec (property(get = get_HttpRequestConnectTimeout)) Common::TimeSpan const & HttpRequestConnectTimeout;
        virtual Common::TimeSpan get_HttpRequestConnectTimeout() const = 0;

        __declspec (property(get = get_ResolveServiceBackoffInterval)) Common::TimeSpan const & ResolveServiceBackoffInterval;
        virtual Common::TimeSpan get_ResolveServiceBackoffInterval() const = 0;

        __declspec (property(get = get_BodyChunkSize)) uint const & BodyChunkSize;
        virtual uint get_BodyChunkSize() const = 0;

        __declspec (property(get = get_RemoveServiceResponseHeaders)) std::wstring const & RemoveServiceResponseHeaders;
        virtual std::wstring get_RemoveServiceResponseHeaders() const = 0;

        __declspec (property(get = get_ResolveTimeout)) Common::TimeSpan const & ResolveTimeout;
        virtual Common::TimeSpan get_ResolveTimeout() const = 0;

        __declspec (property(get = get_WebSocketOpenHandshakeTimeout)) uint const & WebSocketOpenHandshakeTimeout;
        virtual uint get_WebSocketOpenHandshakeTimeout() const = 0;

        __declspec (property(get = get_WebSocketReceiveBufferSize)) uint const & WebSocketReceiveBufferSize;
        virtual uint get_WebSocketReceiveBufferSize() const = 0;

        __declspec (property(get = get_WebSocketSendBufferSize)) uint const & WebSocketSendBufferSize;
        virtual uint get_WebSocketSendBufferSize() const = 0;

        __declspec (property(get = get_WebSocketCloseHandshakeTimeout)) uint const & WebSocketCloseHandshakeTimeout;
        virtual uint get_WebSocketCloseHandshakeTimeout() const = 0;

        __declspec (property(get = get_ApplicationCertificateValidationPolicy)) std::wstring const & ApplicationCertificateValidationPolicy;
        virtual std::wstring get_ApplicationCertificateValidationPolicy() const = 0;

        __declspec (property(get = get_ServiceCertificateThumbprints)) std::wstring const & ServiceCertificateThumbprints;
        virtual std::wstring get_ServiceCertificateThumbprints() const = 0;

        __declspec (property(get = get_ServiceCommonNameAndIssuer)) Common::SecurityConfig::X509NameMap const & ServiceCommonNameAndIssuer;
        virtual Common::SecurityConfig::X509NameMap const& get_ServiceCommonNameAndIssuer() const = 0;

        __declspec (property(get = get_CrlCheckingFlag)) uint const & CrlCheckingFlag;
        virtual uint get_CrlCheckingFlag() const = 0;

        __declspec (property(get = get_IgnoreCrlOfflineError)) bool const & IgnoreCrlOfflineError;
        virtual bool get_IgnoreCrlOfflineError() const = 0;

        __declspec (property(get = get_SecureOnlyMode)) bool const & SecureOnlyMode;
        virtual bool get_SecureOnlyMode() const = 0;

        __declspec (property(get = get_ForwardClientCertificate)) bool const & ForwardClientCertificate;
        virtual bool get_ForwardClientCertificate() const = 0;

        virtual std::unordered_map<std::wstring, bool, CaseInsensitiveHasher, CaseInsensitiveEqualFn> const & ResponseHeadersToRemove() const = 0;

        virtual CertValidationPolicy::Enum GetCertValidationPolicy() const = 0;
    };
}
