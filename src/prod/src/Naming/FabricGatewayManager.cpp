// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Hosting2/HostedServiceParameters.h"
#include "Hosting2/FabricActivatorClient.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Naming;
using namespace HttpGateway;
using namespace Transport;

StringLiteral const TraceType_FabricGatewayManager("FabricGatewayManager");

FabricGatewayManager::FabricGatewayManager(
    FabricNodeConfigSPtr const & nodeConfig,
    IFabricActivatorClientSPtr const & activatorClient,
    ComponentRoot const & componentRoot)
    : RootedObject(componentRoot)
    , nodeConfig_(nodeConfig)
    , activatorClient_(activatorClient)
    , settingsUpdateLock_()
    , ipcListenAddress_()
{
}

bool FabricGatewayManager::RegisterGatewaySettingsUpdateHandler()
{
    weak_ptr<FabricGatewayManager> mgrWPtr = this->shared_from_this();

    {
        AcquireExclusiveLock grab(settingsUpdateLock_);

        if (!GetServerCertThumbprint_CallerHoldingWLock(nullptr, serverCertThumbprint_).IsSuccess()) return false;

        CreateCertMonitorTimerIfNeeded();
    }

    // Httpgateway settings
    nodeConfig_->HttpGatewayListenAddressEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    nodeConfig_->HttpGatewayProtocolEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    // When the following is changed, the cert binding to the http endpoint needs
    // to be updated.
    nodeConfig_->ServerAuthX509StoreNameEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr); });
    nodeConfig_->ServerAuthX509FindTypeEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr); });
    nodeConfig_->ServerAuthX509FindValueEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr); });
    nodeConfig_->ServerAuthX509FindValueSecondaryEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr); });

    // Runas settings
    RunAsConfig::GetConfig().RunAsAccountNameEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    RunAsConfig::GetConfig().RunAsAccountTypeEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    RunAsConfig::GetConfig().RunAsAccountPasswordEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    RunAsConfig::GetConfig().RunAsPasswordEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    return true;
}

AsyncOperationSPtr FabricGatewayManager::BeginActivateGateway(
    wstring const &ipcServerAddres,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    ipcListenAddress_ = ipcServerAddres;
    HostedServiceParameters params;

    // We need to use exclusive lock because config settings are read from more than one config
    // sections, also, BeginActivateHostedService call needs to be serialized.
    AcquireExclusiveLock grab(settingsUpdateLock_);

    auto error = GetHostedServiceParameters_CallerHoldingWLock(params);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Activating FabricGateway with params : {0}",
        params);

    return activatorClient_->BeginActivateHostedService(
        params,
        timeout,
        callback,
        parent);
}

ErrorCode FabricGatewayManager::EndActivateGateway(AsyncOperationSPtr const &operation)
{
    if (operation->FailedSynchronously)
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "EndActivateGateway failed : {0}",
            operation->Error);

        return operation->Error;
    }

    return activatorClient_->EndActivateHostedService(operation);
}

AsyncOperationSPtr FabricGatewayManager::BeginDeactivateGateway(
    TimeSpan const&timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    wstring gatewayName = NamingUtility::GetFabricGatewayServiceName(nodeConfig_->InstanceName);

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Deactivating FabricGateway - Name : {0}",
        gatewayName);

    StopCertMonitorTimer();
    return activatorClient_->BeginDeactivateHostedService(
        gatewayName,
        timeout,
        callback,
        parent);
}

ErrorCode FabricGatewayManager::EndDeactivateGateway(
    AsyncOperationSPtr const &operation)
{
    return activatorClient_->EndDeactivateHostedService(operation);
}

void FabricGatewayManager::AbortGateway()
{
    wstring gatewayName = NamingUtility::GetFabricGatewayServiceName(nodeConfig_->InstanceName);

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Aborting FabricGateway - Name : {0}",
        gatewayName);

    StopCertMonitorTimer();
    activatorClient_->BeginDeactivateHostedService(
        gatewayName,
        Hosting2::HostingConfig::GetConfig().RequestTimeout,
        [this](AsyncOperationSPtr operation)
    {
        ErrorCode error = activatorClient_->EndDeactivateHostedService(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_FabricGatewayManager,
                Root.TraceId,
                "Abort: EndDeactivateHostedService failed : {0}",
                error);
        }
    },
        Root.CreateAsyncOperationRoot());
}

void FabricGatewayManager::SettingsUpdateHandler(weak_ptr<FabricGatewayManager> const &mgrWPtr)
{
    auto mgrSPtr = mgrWPtr.lock();
    if (mgrSPtr)
    {
        auto error = mgrSPtr->OnSettingsUpdated();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_FabricGatewayManager,
                Root.TraceId,
                "Config update handling failed {0}",
                error);
        }
    }
}

_Use_decl_annotations_ ErrorCode FabricGatewayManager::GetServerCertThumbprint_CallerHoldingWLock(
    Thumbprint const* loadedCredThumbprint,
    Thumbprint & serverCertThumbprint)
{
    serverCertThumbprint = Thumbprint();

    SecurityProvider::Enum securityProvider;
    auto error = SecurityProvider::FromCredentialType(SecurityConfig::GetConfig().ServerAuthCredentialType, securityProvider);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to parse ServerAuthCredentialType: {0}",
            error);

        return error;
    }

    if (securityProvider != SecurityProvider::Ssl) return error;

    X509FindValue::SPtr x509FindValue;
    error = X509FindValue::Create(
        nodeConfig_->ServerAuthX509FindType,
        nodeConfig_->ServerAuthX509FindValue,
        nodeConfig_->ServerAuthX509FindValueSecondary,
        x509FindValue);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to parse x509 find type and value: {0}",
            error);

        return error;
    }

    error = SecurityUtil::GetX509SvrCredThumbprint(
        X509Default::StoreLocation(),
        nodeConfig_->ServerAuthX509StoreName,
        x509FindValue,
        loadedCredThumbprint,
        serverCertThumbprint);

    if (!error.IsSuccess()) return error;

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Server certificate thumbprint: {0}",
        serverCertThumbprint);

    return error;
}

ErrorCode FabricGatewayManager::ActivateOnSettingsUpdate_CallerHoldingWLock()
{
    HostedServiceParameters params;
    auto error = GetHostedServiceParameters_CallerHoldingWLock(params);
    if (!error.IsSuccess())
    {
        return error;
    }

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Activating FabricGateway on settings updated with params : {0}",
        params);

    auto op = activatorClient_->BeginActivateHostedService(
        params,
        HostingConfig::GetConfig().ActivationTimeout,
        [this](AsyncOperationSPtr const &op)
    {
        auto error = this->activatorClient_->EndActivateHostedService(op);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_FabricGatewayManager,
                Root.TraceId,
                "Activate hosted service failed : {0}",
                error);
        }
    },
        Root.CreateAsyncOperationRoot());

    return ErrorCodeValue::Success;
}

ErrorCode FabricGatewayManager::OnSettingsUpdated()
{
    // We need to use exclusive lock because config settings are read from more than one config
    // sections, also, BeginActivateHostedService call needs to be serialized.
    AcquireExclusiveLock writeLock(settingsUpdateLock_);

    GetServerCertThumbprint_CallerHoldingWLock(nullptr, serverCertThumbprint_);
    CreateCertMonitorTimerIfNeeded();
    return ActivateOnSettingsUpdate_CallerHoldingWLock();
}

void FabricGatewayManager::CreateCertMonitorTimerIfNeeded()
{
    TimeSpan certMonitorInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;
    if (certMonitorTimer_ || (certMonitorInterval <= TimeSpan::Zero) || (serverCertThumbprint_ == Thumbprint()))
        return;

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "creating timer to check server certificate thumbprint every {0}",
        certMonitorInterval);

    static StringLiteral const timerTag("ServerCertRefresh");
    certMonitorTimer_ = Timer::Create(
        timerTag,
        [this](TimerSPtr const &) { CertMonitorCallback(); },
        false);

    certMonitorTimer_->SetCancelWait();

    Random r;
    TimeSpan dueTime = TimeSpan::FromMilliseconds(certMonitorInterval.TotalMilliseconds() * r.NextDouble());
    certMonitorTimer_->Change(dueTime, certMonitorInterval);
}

void FabricGatewayManager::StopCertMonitorTimer()
{
    AcquireExclusiveLock grab(settingsUpdateLock_);

    if (certMonitorTimer_)
        certMonitorTimer_->Cancel();
}

void FabricGatewayManager::CertMonitorCallback()
{
    Thumbprint serverCertThumbprint;

    // We need to use exclusive lock because config settings are read from more than one config
    // sections, also, BeginActivateHostedService call needs to be serialized.
    AcquireExclusiveLock grab(settingsUpdateLock_);

    auto error = GetServerCertThumbprint_CallerHoldingWLock(&serverCertThumbprint_, serverCertThumbprint);
    if (!error.IsSuccess()) return;

    WriteInfo(
        TraceType_FabricGatewayManager,
        Root.TraceId,
        "Server certificate thumbprint update: {0} -> {1}",
        serverCertThumbprint_,
        serverCertThumbprint);

    serverCertThumbprint_ = serverCertThumbprint;
    ActivateOnSettingsUpdate_CallerHoldingWLock();
}

ErrorCode FabricGatewayManager::GetHostedServiceParameters_CallerHoldingWLock(__out HostedServiceParameters &params)
{
    ErrorCode error;
    bool runAsEnabled = false;
    ServiceModel::SecurityPrincipalAccountType::Enum accountType = ServiceModel::SecurityPrincipalAccountType::NetworkService;

    error = ServiceModel::SecurityPrincipalAccountType::FromString(
        RunAsConfig::GetConfig().RunAsAccountType,
        accountType);
    if (error.IsSuccess())
    {
        runAsEnabled = true;
    }

    wstring binRoot;
    error = FabricEnvironment::GetFabricBinRoot(binRoot);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to retreive fabric bin root.  error: {0}",
            error);

        return error;
    }

    wstring dataRoot;
    error = FabricEnvironment::GetFabricDataRoot(dataRoot);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to retreive fabric data root.  error: {0}",
            error);

        return error;
    }

    wstring codePath;
    error = FabricEnvironment::GetFabricCodePath(codePath);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to retreive fabric code path. error: {0}",
            error);

        return error;
    }

    UINT port = 0;
    wstring protocol;

    //LINUXTODO
#if !defined(PLATFORM_UNIX)    
    if (HttpGatewayConfig::GetConfig().IsEnabled)
    {
        wstring portString;
        wstring hostString;
        error = TcpTransportUtility::TryParseHostPortString(
            nodeConfig_->HttpGatewayListenAddress,
            hostString,
            portString);

        if (!error.IsSuccess() || !StringUtility::TryFromWString<UINT>(portString, port))
        {
            WriteError(
                TraceType_FabricGatewayManager,
                Root.TraceId,
                "Failed to parse port from HttpGatewayListenAddress : {0}",
                nodeConfig_->HttpGatewayListenAddress);

            return ErrorCodeValue::InvalidAddress;
        }

        protocol = nodeConfig_->HttpGatewayProtocol;
    }
#endif
    wstring fabricDirectory = Environment::GetExecutablePath();
    wstring gatewayPath = Path::Combine(fabricDirectory, FabricConstants::FabricGatewayName);
    if (!File::Exists(gatewayPath))
    {
        WriteWarning(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "FabricGateway not found in {0}, using {1}", gatewayPath, codePath);

        gatewayPath = Path::Combine(codePath, FabricConstants::FabricGatewayName);
    }

    wstring environment;
    if (Environment::GetEnvironmentVariableW(FabricEnvironment::PackageConfigStoreEnvironmentVariable, environment, NOTHROW()))
    {
        environment = wformatString("{0}={1}", FabricEnvironment::PackageConfigStoreEnvironmentVariable, environment);
    }

    wstring thisHostedServiceName;
    if (!Environment::GetEnvironmentVariableW(FabricEnvironment::HostedServiceNameEnvironmentVariable, thisHostedServiceName, NOTHROW()))
    {
        WriteWarning(
            TraceType_FabricGatewayManager,
            Root.TraceId,
            "Failed to get the hosted service name from environment. FabricHost will not automatically deactivate the gateway on failure.");
    }

    params = move(HostedServiceParameters(
        NamingUtility::GetFabricGatewayServiceName(nodeConfig_->InstanceName),
        gatewayPath,
        ipcListenAddress_,
        codePath,
        environment,
        true,
        nodeConfig_->InstanceName,
        thisHostedServiceName,
        port,
        protocol,
        runAsEnabled,
        RunAsConfig::GetConfig().RunAsAccountName,
        accountType,
        RunAsConfig::GetConfig().RunAsPassword,
        RunAsConfig::GetConfig().RunAsPasswordEntry.IsEncrypted(),
        serverCertThumbprint_.ToStrings().first,
        nodeConfig_->ServerAuthX509StoreName,
        X509FindType::FindByThumbprint));

    return ErrorCodeValue::Success;
}
