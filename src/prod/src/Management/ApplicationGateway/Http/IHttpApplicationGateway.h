// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class IHttpApplicationGateway
    {
    public:
        virtual ~IHttpApplicationGateway() = default;

        virtual IServiceResolver & GetServiceResolver() = 0;
        virtual ServiceModel::ProtocolType::Enum GetProtocolType() = 0;
        virtual PCCERT_CONTEXT GetCertificateContext() = 0;
        virtual std::wstring const & GetProtocol() = 0;
        virtual Common::ComponentRoot const & GetComponentRoot() = 0;
        virtual IHttpApplicationGatewayConfig const & GetGatewayConfig() = 0;
        virtual IHttpApplicationGatewayEventSource const & GetTraceEventSource() = 0;
        virtual Transport::SecurityProvider::Enum  GetSecurityProvider() = 0;

        virtual Common::ErrorCode CreateHttpClientRequest(
            std::wstring const &clientUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            __in KAllocator &ktlAllocator,
            __in bool allowRedirects,
            __in bool enableCookies,
            __in bool enableWinauthAutoLogon,
            __out HttpClient::IHttpClientRequestSPtr &clientRequest) = 0;
    };
}
