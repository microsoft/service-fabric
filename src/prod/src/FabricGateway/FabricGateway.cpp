// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AzureActiveDirectory/wrapper.server/ServerWrapper.h"

using namespace std;
using namespace FabricGateway;
using namespace Common;
using namespace Naming;
using namespace Reliability;
using namespace Federation;
using namespace Transport;
using namespace HttpGateway;
using namespace Client;

StringLiteral Lifecycle("Lifecycle");
static StringLiteral const TraceHealth("Health");

static Global<RwLock> configInitLock_ = make_global<RwLock>();

class Gateway::CreateAndOpenAsyncOperation
    : public Common::AsyncOperation
{
    DENY_COPY(CreateAndOpenAsyncOperation)

public:

    CreateAndOpenAsyncOperation(
        wstring const &ipcAddress,
        Common::FabricNodeConfigSPtr && config,
        FailedEvent::EventHandler const & failedEventHandler,
        Common::AsyncCallback const & callback)
        : AsyncOperation(callback, Common::AsyncOperationSPtr())
        , config_(std::move(config))
        , ipcAddress_(ipcAddress)
        , failedEventHandler_(failedEventHandler)
    {
    }

    static Common::ErrorCode End(
        __in Common::AsyncOperationSPtr const& operation,
        __out GatewaySPtr & server)
    {
        auto thisPtr = AsyncOperation::End<CreateAndOpenAsyncOperation>(operation);
        server = std::move(thisPtr->server_);
        return thisPtr->Error;
    }

protected:

    void OnStart(__in Common::AsyncOperationSPtr const & thisSPtr)
    {
        //
        // Create the gateway and start it.
        //
        auto error = Gateway::Create(config_, ipcAddress_, server_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                "GatewayCreate failed : {0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        if (failedEventHandler_)
        {
            server_->RegisterFailedEvent(failedEventHandler_);
        }

        auto op = server_->BeginOpen(
            TimeSpan::MaxValue,
            [this] (AsyncOperationSPtr const& operation)
            {
                OnGatewayOpenComplete(operation, false);
            },
            thisSPtr);

        OnGatewayOpenComplete(op, true);
    }

private:

    void OnGatewayOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto errorCode = server_->EndOpen(operation);
        TryComplete(operation->Parent, errorCode);
    }

    Common::FabricNodeConfigSPtr config_;
    GatewaySPtr server_;
    wstring ipcAddress_;
    FailedEvent::EventHandler failedEventHandler_;
};

class Gateway::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation( 
        Gateway & owner,
        TimeSpan const &timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            Lifecycle,
            "Opening timeout : {0}",
            timeoutHelper_.GetRemainingTime());

        auto inner = owner_.proxyIpcClient_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const &operation)
            {
                this->OnIpcTransportOpenComplete(operation, false);
            },
            thisSPtr);

        OnIpcTransportOpenComplete(inner, true);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();
        if (Error.IsSuccess())
        {
            owner_.RegisterForConfigUpdates();
        }
    }

private:

    void OnIpcTransportOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyIpcClient_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                "Proxy Ipc Client open failed :{0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        owner_.SetTraceId(owner_.proxyIpcClient_->Instance.ToString());
        owner_.proxyIpcClient_->UpdateTraceId(owner_.TraceId);

        SecuritySettings serverSecuritySettings;
        error = owner_.CreateSecuritySettings(serverSecuritySettings);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                owner_.TraceId,
                "SecuritySettings from config failed : {0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (serverSecuritySettings.SecurityProvider() == SecurityProvider::Ssl)
        {
            ComponentRootWPtr thisWPtr = owner_.shared_from_this();
            CrlOfflineErrCache::GetDefault().SetHealthReportCallback(
                [thisWPtr] (size_t errCount, wstring const & report)
                {
                    if (auto thisSPtr = thisWPtr.lock())
                    {
                        ((Gateway*)thisSPtr.get())->ReportCrlHealth(errCount, report);
                    }
                });
        }

        owner_.entreeServiceProxy_ = make_unique<EntreeServiceProxy>(
            owner_,
            owner_.config_->ClientConnectionAddress,
            *(owner_.proxyIpcClient_),
            serverSecuritySettings);

        error = owner_.entreeServiceProxy_->Open();
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                owner_.TraceId,
                "EntreeService proxy open failed : {0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        owner_.healthClient_ = make_shared<HealthReportingComponent>(*owner_.entreeServiceProxy_, owner_.TraceId, false);
        error = owner_.healthClient_->Open();
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                owner_.TraceId,
                "healthClient_ open failed : {0}",
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        owner_.EnableTransportHealthReporting();

        OpenHttpGateway(operation->Parent);
    }

    void OpenHttpGateway(AsyncOperationSPtr const &thisSPtr)
    {
        WriteNoise(
            Lifecycle,
            owner_.TraceId,
            "Opening httpgateway");

        ErrorCode error(ErrorCodeValue::Success);
        
        if (!HttpGatewayConfig::GetConfig().IsEnabled)
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // TODO: make httpgateway a rooted component.
        FabricNodeConfigSPtr config = owner_.config_;
        error = HttpGateway::HttpGatewayImpl::Create(config, owner_.httpGateway_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        auto inner = owner_.httpGateway_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const &operation)
            {
                this->OnHttpGatewayOpenComplete(operation, false);
            },
            thisSPtr);

        OnHttpGatewayOpenComplete(inner, true);
    }

    void OnHttpGatewayOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.httpGateway_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                owner_.TraceId,
                "Httpgateway open failed : {0}",
                error);
        }

        TryComplete(operation->Parent, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    Gateway & owner_;
};

class Gateway::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        Gateway & owner,
        TimeSpan const &timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , innerError_(ErrorCodeValue::Success)
    {
    }

    static ErrorCode CloseAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        WriteInfo(
            Lifecycle,
            "Closing - timeout : {0}",
            timeoutHelper_.GetRemainingTime());

        owner_.failedEvent_.Close();

        if (owner_.httpGateway_)
        {
            CloseHttpGateway(thisSPtr);
            owner_.healthClient_->Close();
        }
        else
        {
            owner_.healthClient_->Close();
            CloseIpcTransport(thisSPtr);
        }
    }

    void CloseHttpGateway(AsyncOperationSPtr const &thisSPtr)
    {
        auto inner = owner_.httpGateway_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnHttpGatewayCloseComplete(operation, false);
            },
            thisSPtr);

        OnHttpGatewayCloseComplete(inner, true);
    }

    void OnHttpGatewayCloseComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.httpGateway_->EndClose(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                "Httpgateway close failed : {0}",
                error);

            innerError_ = ErrorCode::FirstError(innerError_, error);
        }

        CloseIpcTransport(operation->Parent);
    }

    void CloseIpcTransport(AsyncOperationSPtr const &thisSPtr)
    {
        if (owner_.entreeServiceProxy_)
        {
            auto error = owner_.entreeServiceProxy_->Close();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    Lifecycle,
                    "EntreeService proxy close failed : {0}",
                    error);
                innerError_ = ErrorCode::FirstError(innerError_, error);
            }
        }

        if (!owner_.proxyIpcClient_)
        {
            TryComplete(thisSPtr, innerError_);
            return;
        }

        auto inner = owner_.proxyIpcClient_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this] (AsyncOperationSPtr const &operation)
            {
                this->OnIpcTransportCloseComplete(operation, false);
            },
            thisSPtr);

        OnIpcTransportCloseComplete(inner, true);
    }

    void OnIpcTransportCloseComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyIpcClient_->EndClose(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Lifecycle,
                "ProxyIpcClient close failed : {0}",
                error);
            innerError_ = ErrorCode::FirstError(innerError_, error);
        }

        TryComplete(operation->Parent, innerError_);
    }

private:

    TimeoutHelper timeoutHelper_;
    Gateway & owner_;
    ErrorCode innerError_;
};

Gateway::Gateway(FabricNodeConfigSPtr const &config)
    : config_(config)
{
}

ErrorCode Gateway::Create(FabricNodeConfigSPtr const &config, wstring const &address, __out GatewaySPtr &gatewaySPtr)
{
    gatewaySPtr = GatewaySPtr(new Gateway(config));
    gatewaySPtr->proxyIpcClient_ = make_unique<ProxyToEntreeServiceIpcChannel>(
            *gatewaySPtr,
            Guid::NewGuid().ToString(),
            address);

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr Gateway::BeginCreateAndOpen(
    wstring const &address,
    FabricNodeConfigSPtr && nodeConfig,
    __in AsyncCallback const& callback)
{
    return AsyncOperation::CreateAndStart<CreateAndOpenAsyncOperation>(address, move(nodeConfig), nullptr, callback);
}

AsyncOperationSPtr Gateway::BeginCreateAndOpen(
    wstring const &address,
    FailedEvent::EventHandler const & failedEventHandler,
    __in AsyncCallback const& callback)
{
    return AsyncOperation::CreateAndStart<CreateAndOpenAsyncOperation>(address, make_shared<FabricNodeConfig>(), failedEventHandler, callback);
}

ErrorCode Gateway::EndCreateAndOpen(
    __in AsyncOperationSPtr const& operation,
    __out GatewaySPtr &server)
{
    return CreateAndOpenAsyncOperation::End(operation, server);
}

AsyncOperationSPtr Gateway::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode Gateway::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    auto error = Gateway::OpenAsyncOperation::End(asyncOperation);
    return error;
}

AsyncOperationSPtr Gateway::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode Gateway::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    auto error = CloseAsyncOperation::End(asyncOperation);
    return error;
}

void Gateway::OnAbort()
{
    WriteWarning(Lifecycle, "Abort");
    failedEvent_.Fire(GetError());
    failedEvent_.Close();

    if (httpGateway_)
    {
        httpGateway_->Abort();
    }
    
    if (entreeServiceProxy_)
    {
        entreeServiceProxy_->Abort();
    }

    if (proxyIpcClient_)
    {
        proxyIpcClient_->Abort();
    }
}

_Use_decl_annotations_
ErrorCode Gateway::CreateSecuritySettings(SecuritySettings &serverSecuritySettings)
{
    auto const & securityConfig = SecurityConfig::GetConfig();

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
        serverSecuritySettings);

    serverSecuritySettings.SetSessionDurationCallback([] { return ServiceModel::ServiceModelConfig::GetConfig().SessionExpiration; });

    //todo, return a gateway setting from callback, when auto refresh is implemented for high priority connection
    serverSecuritySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });

    if (!error.IsSuccess())
    {
        WriteError(
            Lifecycle,
            TraceId,
            "serverSecuritySettings.FromConfiguration error={0}",
            error);
        return error;
    }

    if (serverSecuritySettings.SecurityProvider() == SecurityProvider::Ssl)
    {
        serverSecuritySettings.SetX509CertType(L"server");
    }

    if (AzureActiveDirectory::ServerWrapper::TryOverrideClaimsConfigForAad())
    {
        WriteInfo(
            Lifecycle, 
            TraceId, 
            "Claim-based configurations overidden: AAD={0} user='{1}' admin='{2}'", 
            securityConfig.AADTenantId,
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);
    }

    if (securityConfig.ClientClaimAuthEnabled)
    {
        error = serverSecuritySettings.EnableClaimBasedAuthOnClients(
            securityConfig.ClientClaims,
            securityConfig.AdminClientClaims);

        if (!error.IsSuccess())
        {
            WriteError(Lifecycle, TraceId, "serverSecuritySettings.EnableClaimBasedAuthOnClients error={0}", error);
            return error;
        }

        if (serverSecuritySettings.ClientClaims().Value().empty())
        {
            WriteError(Lifecycle, TraceId, "ClientClaims cannot be empty");
            error = ErrorCodeValue::InvalidArgument;
            return error;
        }

        if (serverSecuritySettings.AdminClientClaims().Value().empty())
        {
            WriteWarning(Lifecycle, TraceId, "AdminClientClaims is empty");
        }
    }

    error = SecurityUtil::AllowDefaultClientsIfNeeded(serverSecuritySettings, *config_, TraceId);
    if (!error.IsSuccess())
    {
        return error;
    }
    return SecurityUtil::EnableClientRoleIfNeeded(serverSecuritySettings, *config_, TraceId);
}

void Gateway::EnableTransportHealthReporting()
{
    weak_ptr<ComponentRoot> rootWPtr = shared_from_this();
    IDatagramTransport::SetHealthReportingCallback([rootWPtr](SystemHealthReportCode::Enum reportCode, wstring const & dynamicProperty, wstring const & description, TimeSpan ttl)
    {
        if (auto thisSPtr = rootWPtr.lock())
        {
            ((Gateway&)(*thisSPtr)).ReportTransportHealth(reportCode, dynamicProperty, description, ttl);
        }
    });
}

void Gateway::RegisterForConfigUpdates()
{
    WriteInfo(Lifecycle, TraceId, "Registering for config updates");
    AcquireWriteLock grab(*configInitLock_);

    weak_ptr<ComponentRoot> rootWPtr = this->shared_from_this();
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();

    // X509: server side settings
    config_->ServerAuthX509StoreNameEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindTypeEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindValueEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ServerAuthX509FindValueSecondaryEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClientCertThumbprintsEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.AdminClientCertThumbprintsEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClientX509NamesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClientAuthAllowedCommonNamesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.AdminClientX509NamesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.AdminClientCommonNamesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClientCertIssuersEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClientCertificateIssuerStoresEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    // Claim based auth: server side settings
    securityConfig.ClientClaimAuthEnabledEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.ClientClaimsEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    securityConfig.AdminClientClaimsEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    // X509: client side settings, needed to find out properties of default client certificate
    config_->ClientAuthX509StoreNameEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindTypeEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindValueEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->ClientAuthX509FindValueSecondaryEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    config_->UserRoleClientX509StoreNameEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindTypeEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindValueEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
    config_->UserRoleClientX509FindValueSecondaryEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    // Negotiate/Kerberos related
    securityConfig.ClientIdentitiesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.AdminClientIdentitiesEntry.AddHandler(
        [this, rootWPtr](EventArgs const&) { SecuritySettingUpdateHandler(rootWPtr); });
}

void Gateway::SecuritySettingUpdateHandler(weak_ptr<ComponentRoot> const & rootWPtr)
{
    auto rootSPtr = rootWPtr.lock();
    if (rootSPtr)
    {
        Gateway* thisPtr = static_cast<Gateway*>(rootSPtr.get());
        if (!thisPtr->OnSecuritySettingsUpdated().IsSuccess())
        {
            thisPtr->Abort();
        }
    }
}

ErrorCode Gateway::OnSecuritySettingsUpdated()
{
    AcquireWriteLock writeLock(securitySettingsUpdateLock_);

    WriteInfo(Lifecycle, TraceId, "OnSecuritySettingsUpdated");

    SecuritySettings serverSecuritySettings;
    auto error = CreateSecuritySettings(serverSecuritySettings);
    if (!error.IsSuccess())
    {
        WriteWarning(
            Lifecycle,
            TraceId,
            "CreateSecuritySettings failed {0}",
            error);

        return error;
    }

    //TODO: SetSecurity for entreeservice proxy and httpgateway
    error = entreeServiceProxy_->SetSecurity(serverSecuritySettings);
    if (!error.IsSuccess())
    {
        RecordError(error);
        WriteWarning(
            Lifecycle,
            TraceId,
            "Entreeservice proxy SetSecurity failed {0}",
            error);
    }

    return error;
}

void Gateway::ReportCrlHealth(size_t errCount, wstring const & description)
{
    auto nodeInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(
        proxyIpcClient_->Instance.Id.IdValue,
        config_->InstanceName,
        proxyIpcClient_->Instance.InstanceId);
           
    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, config_->InstanceName);

    const wstring dynamicProperty(L"FabricGateway");
    WriteInfo("Security", TraceId, "ReportCrlHealth: errCount = {0}, '{1}'", errCount, description);

    ErrorCode error;
    SystemHealthReportCode::Enum reportCode;

    if (errCount > 0)
    {
        reportCode = SystemHealthReportCode::FabricNode_CertificateRevocationListOffline;
        error = healthClient_->AddHealthReport(
            ServiceModel::HealthReport::CreateSystemHealthReport(
                reportCode,
                move(nodeInfo),
                dynamicProperty,
                description,
                SecurityConfig::GetConfig().CrlOfflineHealthReportTtl,
                move(attributes)));
    }
    else
    {
        reportCode = SystemHealthReportCode::FabricNode_CertificateRevocationListOk;
        error = healthClient_->AddHealthReport(
            ServiceModel::HealthReport::CreateSystemRemoveHealthReport(
                reportCode,
                move(nodeInfo),
                dynamicProperty));
    }

    if (!error.IsSuccess())
    {
        WriteInfo("Security", TraceId, "ReportCrlHealth: AddHealthReport({0}) returned {0} for '{1}'", reportCode, error, description);
    }
}

void Gateway::ReportTransportHealth(SystemHealthReportCode::Enum reportCode, wstring const & dynamicProperty, wstring const & description, TimeSpan ttl)
{
    WriteInfo(
        TraceHealth, TraceId,
        "Reporting transport health: reportCode = {0}, dynamicProperty = {1}, description = {2}",
        reportCode, dynamicProperty, description);

    auto nodeInfo = ServiceModel::EntityHealthInformation::CreateNodeEntityHealthInformation(
        proxyIpcClient_->Instance.Id.IdValue,
        config_->InstanceName,
        proxyIpcClient_->Instance.InstanceId);

    ServiceModel::AttributeList attributes;
    attributes.AddAttribute(*ServiceModel::HealthAttributeNames::NodeName, config_->InstanceName);

    healthClient_->AddHealthReport(
        ServiceModel::HealthReport::CreateSystemHealthReport(
            reportCode,
            move(nodeInfo),
            dynamicProperty,
            description,
            ttl,
            move(attributes)));
}

Gateway::FailedEvent::HHandler Gateway::RegisterFailedEvent(Gateway::FailedEvent::EventHandler const & handler)
{
    return failedEvent_.Add(handler);
}

bool Gateway::UnRegisterFailedEvent(Gateway::FailedEvent::HHandler hHandler)
{
    return failedEvent_.Remove(hHandler);
}

void Gateway::RecordError(Common::ErrorCode const & error)
{
    AcquireWriteLock grab(errorLock_);
    if (error_.IsSuccess())
    {
        error_ = error;
    }
}

Common::ErrorCode Gateway::GetError()
{
    AcquireReadLock grab(errorLock_);
    return error_;
}

