// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayImpl
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
#if !defined(PLATFORM_UNIX)
        , public HttpApplicationGateway::IHttpApplicationGateway
#endif
    {
    public:
        ~HttpGatewayImpl();

        static Common::ErrorCode Create(
            __in Common::FabricNodeConfigSPtr &config,
            __out HttpGatewayImplSPtr & fabricHttpGatewaySPtr);

        Common::ErrorCode InitializeServer();
        Common::ErrorCode RegisterHandlers();

        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            Common::FabricNodeConfigSPtr &&,
            __in Common::AsyncCallback const & callback);

        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            __in Common::AsyncCallback const & callback);

        static Common::ErrorCode EndCreateAndOpen(
            __in Common::AsyncOperationSPtr const & operation,
            __out HttpGatewayImplSPtr & server);

        Common::AsyncOperationSPtr BeginCheckAccess(
            HttpServer::IRequestMessageContext & request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCheckAccess(
            Common::AsyncOperationSPtr const& operation,
            __out USHORT &httpStatusOnError,
            __out std::wstring & authHeaderName,
            __out std::wstring & authHeaderValue,
            __out Transport::RoleMask::Enum &roleOnSuccess);

#if !defined(PLATFORM_UNIX)
        // IHttpApplicationGateway interface methods
        //
        HttpApplicationGateway::IServiceResolver & GetServiceResolver()
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
        }
        HttpApplicationGateway::IHttpApplicationGatewayConfig const & GetGatewayConfig()
        {
            return HttpGatewayConfig::GetConfig();
        }
        HttpApplicationGateway::IHttpApplicationGatewayEventSource const & GetTraceEventSource()
        {
            return (HttpApplicationGateway::IHttpApplicationGatewayEventSource const &)*HttpGatewayEventSource::Trace;
        }
        Transport::SecurityProvider::Enum GetSecurityProvider()
        {
            return securitySettings_.SecurityProvider();
        }

        Common::ErrorCode GetGatewayClientCertificateContext();

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

        __declspec(property(get = get_AppGatewayHandler)) std::shared_ptr<HttpApplicationGateway::GatewayRequestHandler> & AppGatewayHandler;
        std::shared_ptr<HttpApplicationGateway::GatewayRequestHandler> & get_AppGatewayHandler() { return appGatewayRequestHandlerSPtr_; }
#endif

        __declspec(property(get = get_AdminClient)) FabricClientWrapperSPtr &AdminClient;
        FabricClientWrapperSPtr const& get_AdminClient() const { return adminClient_; }

        __declspec(property(get = get_UserClient)) FabricClientWrapperSPtr &UserClient;
        FabricClientWrapperSPtr const& get_UserClient() const { return userClient_; }

        __declspec(property(get = get_InnerHttpServer)) std::shared_ptr<HttpServer::HttpServerImpl> & InnerServer;
        std::shared_ptr<HttpServer::HttpServerImpl> & get_InnerHttpServer() { return httpServer_; }

        __declspec(property(get = get_NodeConfig)) Common::FabricNodeConfigSPtr & NodeConfig;
        Common::FabricNodeConfigSPtr const& get_NodeConfig() { return config_; }

    protected:

        //
        // AsyncFabricComponent methods.
        //

        Common::AsyncOperationSPtr OnBeginOpen(
            __in Common::TimeSpan timeout,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(__in Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            __in Common::TimeSpan timeout,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(__in Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:

        HttpGatewayImpl(__in Common::FabricNodeConfigSPtr &config);

        std::shared_ptr<HttpServer::HttpServerImpl>  httpServer_;

#if !defined(PLATFORM_UNIX)
        void CreateServiceResolver();
        HttpClient::IHttpClientSPtr defaultHttpClientSPtr_;
#endif

        Common::ErrorCode InitializeSecurity();

        static void SecuritySettingsUpdateHandler(std::weak_ptr<Common::ComponentRoot> const& rootWPtr);
        Common::ErrorCode OnSecuritySettingsUpdated();
        void RegisterConfigUpdateHandler();
        void AllowHttpGatewayOnOtherNodes(Transport::SecuritySettings const & clientSettings);
        std::wstring GetListenUrl();

        class CreateAndOpenAsyncOperation;
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class AccessCheckAsyncOperation;

        Common::FabricNodeConfigSPtr config_;
        std::wstring listenUrl_;

        HttpServer::IHttpRequestHandlerSPtr nodesHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr applicationsHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr applicationTypesHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr composeDeploymentsHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr systemHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr imageStoreHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr nativeImageStoreHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr toolsHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr faultsHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr namesHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr containerGroupDeploymentsHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr applicationsResourceHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr volumesHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr secretsResourceHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr networksHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr gatewaysResourceHandlerSPtr_;

#if !defined(PLATFORM_UNIX)
        HttpServer::IHttpRequestHandlerSPtr backupRestoreHandlerSPtr_;
        std::shared_ptr<HttpApplicationGateway::GatewayRequestHandler> appGatewayRequestHandlerSPtr_;
        HttpServer::IHttpRequestHandlerSPtr eventsStoreHandlerSPtr_;
#endif

        Common::RwLock securitySettingsUpdateLock_;
        Transport::SecuritySettings securitySettings_;

        //
        // Security providers
        //
        std::vector<HttpAuthHandlerSPtr> httpAuthHandlers_;

        //
        // FabricClient instance
        //
        FabricClientWrapperSPtr adminClient_;
        FabricClientWrapperSPtr userClient_;

#if !defined(PLATFORM_UNIX)
        // IHttpApplicationGateway members
        std::unique_ptr<HttpApplicationGateway::IServiceResolver> serviceResolver_;
        std::wstring protocol_;
        ServiceModel::ProtocolType::Enum protocolType_;

        // Note: We pass a raw pointer of this unique_ptr to KTL in the KHttpCliRequest.
        // Ensure this doesnt go out of scope for the lifetime of the request.
        Common::CertContextUPtr clientCertContext_;
#endif
    };
}
