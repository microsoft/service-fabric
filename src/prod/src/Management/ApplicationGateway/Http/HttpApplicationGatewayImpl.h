// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class HttpApplicationGatewayImpl
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public IHttpApplicationGateway
    {
        DENY_COPY(HttpApplicationGatewayImpl);

    public:
        ~HttpApplicationGatewayImpl() = default;
        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            __in Common::FabricNodeConfigSPtr &config,
            Common::TimeSpan &timeout,
            Common::AsyncCallback const &callback);

        static Common::ErrorCode EndCreateAndOpen(
            Common::AsyncOperationSPtr const & operation,
            __out HttpApplicationGatewaySPtr & server);

        Common::ErrorCode Initialize();

        // IHttpApplicationGateway interface methods
        //
        IServiceResolver & GetServiceResolver()
        { 
            return *serviceResolver_;
        }
        ServiceModel::ProtocolType::Enum GetProtocolType()
        {
            return protocolType_;
        }
        PCCERT_CONTEXT GetCertificateContext()
        {
            return clientCertContext_.get();
        }
        std::wstring const & GetProtocol()
        {
            return protocol_;
        }
        Common::ComponentRoot const & GetComponentRoot()
        {
            return *this;
        };
        HttpApplicationGateway::IHttpApplicationGatewayConfig const & GetGatewayConfig() 
        {
            return HttpApplicationGatewayConfig::GetConfig();
        }
        HttpApplicationGateway::IHttpApplicationGatewayEventSource const & GetTraceEventSource()
        {
            return (HttpApplicationGateway::IHttpApplicationGatewayEventSource const &)*HttpApplicationGatewayEventSource::Trace;
        }
        Transport::SecurityProvider::Enum GetSecurityProvider()
        {
            // Reverse proxy supports SSL - X509 only.
            if (protocolType_ == ServiceModel::ProtocolType::Https)
            {
                return Transport::SecurityProvider::Ssl;
            }
            return Transport::SecurityProvider::None;
        }

        // This should include any other client specific settings.
        //
        Common::ErrorCode CreateHttpClientRequest(
            std::wstring const &clientUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            __in KAllocator &ktlAllocator,
            __in bool allowRedirects,
            __in bool enableCookies,
            __in bool enableWinauthAutoLogon,
            __out HttpClient::IHttpClientRequestSPtr &clientRequest);

    protected:

        //
        // AsyncFabricComponent methods.
        //

        Common::AsyncOperationSPtr OnBeginOpen(
            __in Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            __in Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:

        HttpApplicationGatewayImpl(__in Common::FabricNodeConfigSPtr &config)
            : fabricNodeConfigSPtr_(config),
             gatewayConfigSPtr_(std::make_shared<HttpApplicationGatewayConfig>()),
             protocol_(std::move(GetProtocol(config))),
             protocolType_(ServiceModel::ProtocolType::GetProtocolType(protocol_))
        {
        }

        static std::wstring GetProtocol(__in Common::FabricNodeConfigSPtr &config)
        {
            if (HttpApplicationGatewayConfig::GetConfig().StandAloneMode)
            {
                return HttpApplicationGatewayConfig::GetConfig().Protocol;
            }
            return config->HttpApplicationGatewayProtocol;
        }
           
        Common::ErrorCode RegisterMessageHandlers();
        std::wstring GetListenAddress();
        std::wstring ParseListenAddress(std::wstring const &protocol, std::wstring const &listenAddress);
        void CreateServiceResolver();

        Common::ErrorCode GetClientCertificateContext();

        class CreateAndOpenAsyncOperation;
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        HttpServer::IHttpServerSPtr httpServerSPtr_;
        HttpClient::IHttpClientSPtr defaultHttpClientSPtr_;
        HttpServer::IHttpRequestHandlerSPtr gatewayRequestHandlerSPtr_;

        // Note: Do not change the order below. The HttpApplicationGatewayImpl constructor initializer list depends on this initialization order.
        Common::FabricNodeConfigSPtr fabricNodeConfigSPtr_;
        std::shared_ptr<IHttpApplicationGatewayConfig> gatewayConfigSPtr_;
        IServiceResolverUPtr serviceResolver_;
        std::wstring protocol_;
        ServiceModel::ProtocolType::Enum protocolType_;

        // Note: We pass a raw pointer of this unique_ptr to KTL in the KHttpCliRequest.
        // Ensure this doesnt go out of scope for the lifetime of the request.
        Common::CertContextUPtr clientCertContext_;
    };
}
