// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Api;
using namespace Client;
using namespace HttpGateway;
using namespace Transport;
using namespace Naming;
using namespace HttpServer;
using namespace HttpClient;

StringLiteral const TraceType("HttpGatewayImpl");
StringLiteral const TraceTypeLifeCycle("LifeCycle");

HttpGatewayImpl::HttpGatewayImpl(__in FabricNodeConfigSPtr &config)
    : config_(move(config))
#if !defined(PLATFORM_UNIX)
    , protocol_(config_->HttpGatewayProtocol)
    , protocolType_(ServiceModel::ProtocolType::GetProtocolType(protocol_))
#endif
{
}

HttpGatewayImpl::~HttpGatewayImpl()
{
}

ErrorCode HttpGatewayImpl::Create(__in FabricNodeConfigSPtr &config, __out HttpGatewayImplSPtr & server)
{
    server = HttpGatewayImplSPtr(new HttpGatewayImpl(config));
    return ErrorCode::Success();
}

ErrorCode HttpGatewayImpl::InitializeServer()
{
    listenUrl_ = GetListenUrl();

    WriteInfo(TraceTypeLifeCycle, "Listening at Root Url - {0}", listenUrl_);

    auto error = FabricClientWrapper::CreateFabricClient(config_, adminClient_, RoleMask::Admin);
    if (!error.IsSuccess()) { return error; }

    error = FabricClientWrapper::CreateFabricClient(config_, userClient_, RoleMask::User);
    if (!error.IsSuccess()) { return error; }

    error = InitializeSecurity();
    if (!error.IsSuccess()) { return error; }

    httpServer_ = make_shared<HttpServerImpl>(
        *this, 
        listenUrl_,
        HttpGatewayConfig::GetConfig().ActiveListeners,
        securitySettings_.SecurityProvider());

#if !defined(PLATFORM_UNIX)
    error = HttpClientImpl::CreateHttpClient(
        L"HttpGateway",
        *this,
        defaultHttpClientSPtr_);
    if (!error.IsSuccess()) { return error; }

    CreateServiceResolver();
#endif

    // Register the handlers after creating the HttpServer
    error = RegisterHandlers();

    return error;
}

ErrorCode HttpGatewayImpl::RegisterHandlers()
{   
    //
    // Create/Register various handlers.
    //

    auto error = RequestHandlerBase::Create<NodesHandler>(*this, nodesHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ApplicationsHandler>(*this, applicationsHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ApplicationTypesHandler>(*this, applicationTypesHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ClusterManagementHandler>(*this, systemHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ComposeDeploymentsHandler>(*this, composeDeploymentsHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ImageStoreHandler>(*this, imageStoreHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<ToolsHandler>(*this, toolsHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<NamesHandler>(*this, namesHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<FaultsHandler>(*this, faultsHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<NetworksHandler>(*this, networksHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

#if !defined(PLATFORM_UNIX)
    error = RequestHandlerBase::Create<BackupRestoreHandler>(*this, backupRestoreHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    appGatewayRequestHandlerSPtr_ = make_shared<HttpApplicationGateway::GatewayRequestHandler>(*this);

    error = RequestHandlerBase::Create<EventsStoreHandler>(*this, eventsStoreHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }
#endif

    error = RequestHandlerBase::Create<ApplicationsResourceHandler>(*this, applicationsResourceHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<VolumesHandler>(*this, volumesHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<SecretsResourceHandler>(*this, secretsResourceHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    error = RequestHandlerBase::Create<GatewaysResourceHandler>(*this, gatewaysResourceHandlerSPtr_);
    if (!error.IsSuccess()) { return error; }

    return error;
}

#if !defined(PLATFORM_UNIX)
ErrorCode HttpGatewayImpl::CreateHttpClientRequest(
    wstring const &clientUri,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    KAllocator &allocator,
    __in bool allowRedirects,
    __in bool enableCookies,
    __in bool enableWinauthAutoLogon,
    __out IHttpClientRequestSPtr &clientRequest)
{
    return defaultHttpClientSPtr_->CreateHttpRequest(
        clientUri,
        connectTimeout,
        sendTimeout,
        receiveTimeout,
        allocator,
        clientRequest,
        allowRedirects,
        enableCookies,
        enableWinauthAutoLogon);
}
#endif

AsyncOperationSPtr HttpGatewayImpl::BeginCreateAndOpen(__in AsyncCallback const& callback)
{
    return HttpGatewayImpl::BeginCreateAndOpen(make_shared<FabricNodeConfig>(), callback);
}

AsyncOperationSPtr HttpGatewayImpl::BeginCreateAndOpen(
    FabricNodeConfigSPtr && nodeConfig,
    __in AsyncCallback const& callback)
{
    return AsyncOperation::CreateAndStart<CreateAndOpenAsyncOperation>(move(nodeConfig), callback);
}

ErrorCode HttpGatewayImpl::EndCreateAndOpen(
    __in AsyncOperationSPtr const& operation,
    __out HttpGatewayImplSPtr &server)
{
    return CreateAndOpenAsyncOperation::End(operation, server);
}

AsyncOperationSPtr HttpGatewayImpl::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceTypeLifeCycle, "BeginOpen");
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpGatewayImpl::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    auto error = HttpGatewayImpl::OpenAsyncOperation::End(asyncOperation);
    WriteInfo(TraceTypeLifeCycle, "Open completed with {0}", error);
    return error;
}

AsyncOperationSPtr HttpGatewayImpl::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceTypeLifeCycle, "BeginClose");
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpGatewayImpl::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    auto error = CloseAsyncOperation::End(asyncOperation);
    WriteInfo(TraceTypeLifeCycle, "Close completed with {0}", error);
    return error;
}

void HttpGatewayImpl::OnAbort()
{
    // no op
    WriteInfo(TraceTypeLifeCycle, "OnAbort");
}

AsyncOperationSPtr HttpGatewayImpl::BeginCheckAccess(
    IRequestMessageContext & request,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<AccessCheckAsyncOperation>(
        *this,
        request,
        timeout,
        callback,
        parent);
}

ErrorCode HttpGatewayImpl::EndCheckAccess(
     AsyncOperationSPtr const& operation,
    __out USHORT &httpStatusOnError,
    __out wstring & authHeaderName,
    __out wstring & authHeaderValue,
    __out RoleMask::Enum & roleOnSuccess)
{
    return AccessCheckAsyncOperation::End(operation, httpStatusOnError, authHeaderName, authHeaderValue, roleOnSuccess);
}

ErrorCode HttpGatewayImpl::InitializeSecurity()
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    auto error = SecuritySettings::FromConfiguration(
        securityConfig.ServerAuthCredentialType,
        config_->ServerAuthX509StoreName,
        wformatString(X509Default::StoreLocation()),
        config_->ServerAuthX509FindType,
        config_->ServerAuthX509FindValue,
        config_->ServerAuthX509FindValueSecondary,
        securityConfig.ClientServerProtectionLevel,
        securityConfig.ClientCertThumbprints,
        securityConfig.ClientX509Names,
        securityConfig.ClientCertificateIssuerStores, // todo, add issuer cert refresh logic to timer FabricGatewayManager::certMonitorTimer_
        securityConfig.ClientAuthAllowedCommonNames,
        securityConfig.ClientCertIssuers,
        L"",
        securityConfig.ClientIdentities,
        securitySettings_);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "ServerSecurity settings fromConfiguration error={0}", error);
        return error;
    }

    SecuritySettings fabricClientSecuritySettings;
    error = SecuritySettings::FromConfiguration(
        securityConfig.ServerAuthCredentialType,
        config_->ClientAuthX509StoreName,
        wformatString(X509Default::StoreLocation()),
        config_->ClientAuthX509FindType,
        config_->ClientAuthX509FindValue,
        config_->ClientAuthX509FindValueSecondary,
        securityConfig.ClientServerProtectionLevel,
        securityConfig.ServerCertThumbprints,
        securityConfig.ServerX509Names,
        securityConfig.ServerCertificateIssuerStores,
        securityConfig.ServerAuthAllowedCommonNames,
        securityConfig.ServerCertIssuers,
        securityConfig.ClusterSpn,
        L"",
        fabricClientSecuritySettings);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "ClientSecurity settings fromConfiguration error={0}", error);
        return error;
    }

    error = SecurityUtil::AllowDefaultClientsIfNeeded(securitySettings_, *config_, *Constants::HttpGatewayTraceId);
    if (!error.IsSuccess()) { return error; }

    error = SecurityUtil::EnableClientRoleIfNeeded(securitySettings_, *config_, *Constants::HttpGatewayTraceId);
    if (!error.IsSuccess()) { return error; }

    if (AzureActiveDirectory::ServerWrapper::TryOverrideClaimsConfigForAad())
    {
        WriteInfo(
            TraceType, 
            "Claim-based configurations overidden: AAD={0} user='{1}' admin='{2}'", 
            securityConfig.AADTenantId,
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);
    }

    if (securityConfig.ClientClaimAuthEnabled)
    {
        error = securitySettings_.EnableClaimBasedAuthOnClients(
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);

        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                "ServerSecuritySettings.EnableClaimsBasedAuthOnClients error={0}",
                error);
            return error;
        }
    }

    if (securitySettings_.SecurityProvider() == SecurityProvider::None)
    {
        WriteInfo(TraceType, "HttpGateway configured with security - {0}", SecurityProvider::None);
        return ErrorCode::Success();
    }

#if !defined(PLATFORM_UNIX)


    if (SecurityProvider::IsWindowsProvider(securitySettings_.SecurityProvider()))
    {
        HttpAuthHandlerSPtr authHandler;
        error = HttpAuthHandler::Create<HttpKerberosAuthHandler>(securitySettings_, securitySettings_.SecurityProvider(), adminClient_, authHandler);
        if (!error.IsSuccess()) { return error; }

        WriteInfo(TraceType, "HttpGateway configured with security - {0}", securitySettings_.SecurityProvider());
        httpAuthHandlers_.push_back(move(authHandler));
    }
    else if (securitySettings_.SecurityProvider() == SecurityProvider::Ssl)
    {
        // Initialize client cert that gateway will present to services when used as a reverse proxy
        error = GetGatewayClientCertificateContext();
        if (!error.IsSuccess()) { return error; }

#else
    if (securitySettings_.SecurityProvider() == SecurityProvider::Ssl)
    {
#endif
        HttpAuthHandlerSPtr authHandler;
        if (securitySettings_.ClaimBasedClientAuthEnabled())
        {
            error = HttpAuthHandler::Create<HttpClaimsAuthHandler>(securitySettings_, SecurityProvider::Claims, adminClient_, authHandler);
            if (!error.IsSuccess()) { return error; }

            WriteInfo(TraceType, "HttpGateway configured with security - {0}", SecurityProvider::Claims);
            httpAuthHandlers_.push_back(move(authHandler));
        }

        error = HttpAuthHandler::Create<HttpCertificateAuthHandler>(securitySettings_, SecurityProvider::Ssl, adminClient_, authHandler);
        if (!error.IsSuccess()) { return error; }

        WriteInfo(TraceType, "HttpGateway configured with security - {0}", SecurityProvider::Ssl);
        httpAuthHandlers_.push_back(move(authHandler));
    }
    else if (AzureActiveDirectory::ServerWrapper::IsAadEnabled())
    {
        WriteError(TraceType, "Security provider type unknown - {0}", securitySettings_.SecurityProvider());
        return ErrorCodeValue::InvalidCredentialType;
    }

    AllowHttpGatewayOnOtherNodes(fabricClientSecuritySettings);

    auto securitySettingsCopy = fabricClientSecuritySettings;
    error = adminClient_->SetSecurity(move(fabricClientSecuritySettings));
    if (!error.IsSuccess()) { return error; }

    error = userClient_->SetSecurity(move(securitySettingsCopy));
    if (error.IsSuccess()) { RegisterConfigUpdateHandler(); }

    return error;
}

#if !defined(PLATFORM_UNIX)

ErrorCode HttpGatewayImpl::GetGatewayClientCertificateContext()
{
    // Initialize client cert that gateway will present to services when used as a reverse proxy

    if (securitySettings_.SecurityProvider() != SecurityProvider::Ssl)
    {
        WriteWarning(TraceType, "SecurityProvider is not SecurityProvider::Ssl, cannot set client certificate context");
        return ErrorCodeValue::NotFound;
    }

    X509FindValue::SPtr x509FindValue;
    auto error = X509FindValue::Create(
        config_->ServerAuthX509FindType,
        config_->ServerAuthX509FindValue,
        config_->ServerAuthX509FindValueSecondary,
        x509FindValue);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "Failed to create x509FindValue from ServerAuthX509FindType and ServerAuthX509FindValue : error = {0}", error);
        return error;
    }

    Common::Thumbprint gatewayCertThumbprint;

    error = SecurityUtil::GetX509SvrCredThumbprint(
        X509Default::StoreLocation(),
        config_->ServerAuthX509StoreName,
        x509FindValue,
        nullptr,
        gatewayCertThumbprint);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "GetX509SvrCredThumbprint failed, error = {0}", error);
        return error;
    }

    WriteInfo(
        TraceType,
        "Gateway certificate thumbprint: {0}",
        gatewayCertThumbprint);

    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(),
        config_->ServerAuthX509StoreName,
        L"FindByThumbprint",
        gatewayCertThumbprint.ToStrings().first,
        clientCertContext_);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "Error getting the GatewayX509 certificate for connecting as a client: selected thumbprint = {0}, error = {1}", gatewayCertThumbprint.ToStrings().first, error);
        return error;
    }

    return error;
}

void HttpGatewayImpl::CreateServiceResolver()
{
    serviceResolver_ = make_unique<ServiceResolverImpl>(config_);
}
#endif

void HttpGatewayImpl::RegisterConfigUpdateHandler()
{
    weak_ptr<ComponentRoot> rootWPtr = this->shared_from_this();

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();

    // X509: server side settings
    config_->ServerAuthX509StoreNameEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindTypeEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindValueEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindValueSecondaryEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });

    securityConfig.ClientCertThumbprintsEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.AdminClientCertThumbprintsEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ClientX509NamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ClientAuthAllowedCommonNamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.AdminClientX509NamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.AdminClientCommonNamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ClientCertIssuersEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ClientCertificateIssuerStoresEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
#if !defined(PLATFORM_UNIX)
    // Claims: server side settings
    securityConfig.ClientClaimAuthEnabledEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ClientClaimsEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.AdminClientClaimsEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
#endif
    // X509: client side settings
    config_->ClientAuthX509StoreNameEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindTypeEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindValueEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindValueSecondaryEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });

    config_->UserRoleClientX509StoreNameEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindTypeEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindValueEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindValueSecondaryEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });

    securityConfig.ServerCertThumbprintsEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ServerX509NamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ServerAuthAllowedCommonNamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ServerCertIssuersEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
    securityConfig.ServerCertificateIssuerStoresEntry.AddHandler(
        [rootWPtr](EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });

#if !defined(PLATFORM_UNIX)
    // Negotiate/Kerberos: server side settings
    securityConfig.ClientIdentitiesEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });

    securityConfig.AdminClientIdentitiesEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { SecuritySettingsUpdateHandler(rootWPtr); });
#endif
}

void HttpGatewayImpl::SecuritySettingsUpdateHandler(weak_ptr<ComponentRoot> const& rootWPtr)
{
    auto rootSPtr = rootWPtr.lock();
    if (rootSPtr)
    {
        HttpGatewayImpl *thisPtr = static_cast<HttpGatewayImpl*>(rootSPtr.get());
        if (!thisPtr->OnSecuritySettingsUpdated().IsSuccess())
        {
            thisPtr->Abort();
        }
    }
}

void HttpGatewayImpl::AllowHttpGatewayOnOtherNodes(SecuritySettings const & clientSettings)
{
    // HTTP gateway instance that acccepts docker REST API call needs to forward the call
    // to the gateway instance on the destination node, where the call is forwarded to
    // container endpoint on loopback address. So here we need to treat gateway processes on 
    // other nodes as admin clients (container API call requires admin role). We do not plan
    // to support this when client role is disabled, thus they are only added for admin role.
    if (securitySettings_.SecurityProvider() == SecurityProvider::Ssl)
    {
        securitySettings_.AddAdminClientX509Names(clientSettings.RemoteX509Names());
    }
    else if (SecurityProvider::IsWindowsProvider(securitySettings_.SecurityProvider()))
    {
        securitySettings_.AddAdminClientIdentities(clientSettings.RemoteIdentities());
    }
}

ErrorCode HttpGatewayImpl::OnSecuritySettingsUpdated()
{
    AcquireWriteLock writeLock(securitySettingsUpdateLock_);

    WriteInfo(TraceType, "OnSecuritySettingsUpdated");

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    auto error = SecuritySettings::FromConfiguration(
        securityConfig.ServerAuthCredentialType,
        config_->ServerAuthX509StoreName,
        wformatString(X509Default::StoreLocation()),
        config_->ServerAuthX509FindType,
        config_->ServerAuthX509FindValue,
        config_->ServerAuthX509FindValueSecondary,
        securityConfig.ClientServerProtectionLevel,
        securityConfig.ClientCertThumbprints,
        securityConfig.ClientX509Names,
        securityConfig.ClientCertificateIssuerStores,
        securityConfig.ClientAuthAllowedCommonNames,
        securityConfig.ClientCertIssuers,
        L"",
        securityConfig.ClientIdentities,
        securitySettings_);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "ServerSecurity settings fromConfiguration error={0}", error);
        return error;
    }

    SecuritySettings fabricClientSecuritySettings;
    error = SecuritySettings::FromConfiguration(
        securityConfig.ServerAuthCredentialType,
        config_->ClientAuthX509StoreName,
        wformatString(X509Default::StoreLocation()),
        config_->ClientAuthX509FindType,
        config_->ClientAuthX509FindValue,
        config_->ClientAuthX509FindValueSecondary,
        securityConfig.ClientServerProtectionLevel,
        securityConfig.ServerCertThumbprints,
        securityConfig.ServerX509Names,
        securityConfig.ServerCertificateIssuerStores,
        securityConfig.ServerAuthAllowedCommonNames,
        securityConfig.ServerCertIssuers,
        securityConfig.ClusterSpn,
        L"",
        fabricClientSecuritySettings);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, "ClientSecurity settings fromConfiguration error={0}", error);
        return error;
    }

    error = SecurityUtil::AllowDefaultClientsIfNeeded(securitySettings_, *config_, *Constants::HttpGatewayTraceId);
    if (!error.IsSuccess()) { return error; }

    error = SecurityUtil::EnableClientRoleIfNeeded(securitySettings_, *config_, *Constants::HttpGatewayTraceId);
    if (!error.IsSuccess()) { return error; }
#if !defined(PLATFORM_UNIX)
    if (AzureActiveDirectory::ServerWrapper::TryOverrideClaimsConfigForAad())
    {
        WriteInfo(
            TraceType, 
            "Claim-based configurations overidden: AAD={0} user='{1}' admin='{2}'", 
            securityConfig.AADTenantId,
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);
    }

    if (securityConfig.ClientClaimAuthEnabled)
    {
        error = securitySettings_.EnableClaimBasedAuthOnClients(
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);

        if (!error.IsSuccess())
        {
            WriteError(TraceType, "serverSecuritySettings.EnableClaimBasedAuthOnClients error={0}", error);
            return error;
        }
    }

    // Update the client cert that gateway will present to services when used as a reverse proxy
    error = GetGatewayClientCertificateContext();
    if (!error.IsSuccess())
    {
        return error;
    }

#endif
    // Update the auth handlers and fabricclientf
    for (auto handlerItr = httpAuthHandlers_.cbegin(); handlerItr != httpAuthHandlers_.cend(); ++handlerItr)
    {
        auto &handlerSPtr = *handlerItr;
        if (!handlerSPtr->SetSecurity(securitySettings_).IsSuccess())
        {
            WriteError(TraceType, "Failed to update security settings");
            return error;
        }
    }

    AllowHttpGatewayOnOtherNodes(fabricClientSecuritySettings);

    auto settingsCopy = fabricClientSecuritySettings;
    error = adminClient_->SetSecurity(move(fabricClientSecuritySettings));
    if (!error.IsSuccess()) { return error; }

    error = userClient_->SetSecurity(move(settingsCopy));

    return error;
}

wstring HttpGatewayImpl::GetListenUrl()
{
    //
    // This routine constructs the url that the http server should listen on.
    // This will be used till we add changes to the fabricdeployer to get the
    // rooturl.
    //

    wstring portString;
    wstring hostString;
    auto error = TcpTransportUtility::TryParseHostPortString(
                config_->HttpGatewayListenAddress,
                hostString,
                portString);

   
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            "Failed to parse port from HttpGatewayListenAddress : {0} ErrorCode : {1}",
            config_->HttpGatewayListenAddress,
            error);

        //
        // Crashing here, would give feedback to the user that there is an invalid address format.
        //

        Assert::CodingError(
            "Failed to parse port from HttpGatewayListenAddress : {0} ErrorCode : {1}",
            config_->HttpGatewayListenAddress,
            error);
    }

#if defined(PLATFORM_UNIX)
    wstring rootUrl = wformatString("{0}://0.0.0.0:{1}/", config_->HttpGatewayProtocol, portString);
#else
    wstring rootUrl = wformatString("{0}://+:{1}/", config_->HttpGatewayProtocol, portString);
#endif

    return move(rootUrl);
}
