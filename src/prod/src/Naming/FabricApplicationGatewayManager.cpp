// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Transport;

StringLiteral const TraceType_GatewayManager("FabricApplicationGatewayManager");

bool FabricApplicationGatewayManager::RegisterGatewaySettingsUpdateHandler()
{
    if (!GetGatewayCertThumbprints(nullptr, certThumbprint_).IsSuccess()) return false;

    CreateCertMonitorTimerIfNeeded();

    weak_ptr<FabricApplicationGatewayManager> mgrWPtr = this->shared_from_this();

    // Certificate update scenarios
    HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateStoreNameEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindTypeEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValueEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });

    HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValueSecondaryEntry.AddHandler(
        [this, mgrWPtr](EventArgs const &) { this->SettingsUpdateHandler(mgrWPtr);  });
    
    return true;
}

AsyncOperationSPtr FabricApplicationGatewayManager::BeginActivateGateway(
    wstring const &ipcServerAddres,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    UNREFERENCED_PARAMETER(ipcServerAddres);

    HostedServiceParameters params;
    auto error = GetHostedServiceParameters(params);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }

    WriteInfo(
        TraceType_GatewayManager,
        Root.TraceId,
        "Activating FabriApplicationcGateway with params : {0}",
        params);

    return activatorClient_->BeginActivateHostedService(
        params,
        timeout,
        callback,
        parent);
}

ErrorCode FabricApplicationGatewayManager::EndActivateGateway(AsyncOperationSPtr const &operation)
{
    if (operation->FailedSynchronously)
    {
        WriteError(
            TraceType_GatewayManager,
            Root.TraceId,
            "EndActivateGateway failed : {0}",
            operation->Error);

        return operation->Error;
    }

    return activatorClient_->EndActivateHostedService(operation);
}

AsyncOperationSPtr FabricApplicationGatewayManager::BeginDeactivateGateway(
    TimeSpan const&timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    wstring gatewayName = NamingUtility::GetFabricApplicationGatewayServiceName(nodeConfig_->InstanceName);

    WriteInfo(
        TraceType_GatewayManager,
        Root.TraceId,
        "Deactivating FabricApplicationGateway - Name : {0}",
        gatewayName);

    StopCertMonitorTimer();
    return activatorClient_->BeginDeactivateHostedService(
        gatewayName,
        timeout,
        callback,
        parent);
}

ErrorCode FabricApplicationGatewayManager::EndDeactivateGateway(
    AsyncOperationSPtr const &operation)
{
    return activatorClient_->EndDeactivateHostedService(operation);
}

void FabricApplicationGatewayManager::AbortGateway()
{
    wstring gatewayName = NamingUtility::GetFabricApplicationGatewayServiceName(nodeConfig_->InstanceName);

    WriteInfo(
        TraceType_GatewayManager,
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
                    TraceType_GatewayManager,
                    Root.TraceId,
                    "Abort: EndDeactivateHostedService failed : {0}",
                    error);
            }
        },
        Root.CreateAsyncOperationRoot());
}

ErrorCode FabricApplicationGatewayManager::GetHostedServiceParameters(__out HostedServiceParameters &params)
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
            TraceType_GatewayManager,
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
            TraceType_GatewayManager,
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
            TraceType_GatewayManager,
            Root.TraceId,
            "Failed to retreive fabric code path. error: {0}",
            error);

        return error;
    }

    UINT port = 0;
    wstring protocol;

    if (HttpApplicationGatewayConfig::GetConfig().IsEnabled)
    {
        wstring portString;
        wstring hostString;
        error = TcpTransportUtility::TryParseHostPortString(
            nodeConfig_->HttpApplicationGatewayListenAddress,
            hostString,
            portString);

        if (!error.IsSuccess() || !StringUtility::TryFromWString<UINT>(portString, port))
        {
            WriteError(
                TraceType_GatewayManager,
                Root.TraceId,
                "Failed to parse port from HttpApplicationGatewayListenAddress : {0}",
                nodeConfig_->HttpGatewayListenAddress);

            return ErrorCodeValue::InvalidAddress;
        }

        protocol = nodeConfig_->HttpApplicationGatewayProtocol;
    }

    wstring fabricDirectory = Environment::GetExecutablePath();
    wstring gatewayPath = Path::Combine(fabricDirectory, FabricConstants::FabricApplicationGatewayName);
    if (!File::Exists(gatewayPath))
    {
        WriteWarning(
            TraceType_GatewayManager,
            Root.TraceId,
            "FabricApplicationGateway not found in {0}, using {1}", gatewayPath, codePath);

        gatewayPath = Path::Combine(codePath, FabricConstants::FabricApplicationGatewayName);
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
            TraceType_GatewayManager,
            Root.TraceId,
            "Failed to get the hosted service name from environment. FabricHost will not automatically deactivate the gateway on failure.");
    }

    params = move(HostedServiceParameters(
        NamingUtility::GetFabricApplicationGatewayServiceName(nodeConfig_->InstanceName),
        gatewayPath,
        L"",
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
        certThumbprint_.ToStrings().first,
        nodeConfig_->ServerAuthX509StoreName,
        X509FindType::FindByThumbprint));

    return ErrorCodeValue::Success;
}

ErrorCode FabricApplicationGatewayManager::GetGatewayCertThumbprints(
    Thumbprint const* loadedCredThumbprint,
    Thumbprint & serverCertThumbprint)
{
    serverCertThumbprint = Thumbprint();

    SecurityProvider::Enum securityProvider;
    auto error = SecurityProvider::FromCredentialType(
        HttpApplicationGatewayConfig::GetConfig().GatewayAuthCredentialType, securityProvider);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_GatewayManager,
            Root.TraceId,
            "Failed to parse GatewayAuthCredentialType: {0}",
            error);

        return error;
    }

    if (securityProvider != SecurityProvider::Ssl) return error;

    X509FindValue::SPtr x509FindValue;
    error = X509FindValue::Create(
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindType,
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValue,
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValueSecondary,
        x509FindValue);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_GatewayManager,
            Root.TraceId,
            "Failed to parse x509 find type and value: {0}",
            error);

        return error;
    }

    error = SecurityUtil::GetX509SvrCredThumbprint(
        X509Default::StoreLocation(),
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateStoreName,
        x509FindValue,
        loadedCredThumbprint,
        serverCertThumbprint);

    if (!error.IsSuccess()) return error;

    WriteInfo(
        TraceType_GatewayManager,
        Root.TraceId,
        "Certificate thumbprint: {0}",
        serverCertThumbprint);

    return error;
}

void FabricApplicationGatewayManager::SettingsUpdateHandler(weak_ptr<FabricApplicationGatewayManager> const &mgrWPtr)
{
    auto mgrSPtr = mgrWPtr.lock();
    if (mgrSPtr)
    {
        auto error = mgrSPtr->OnSettingsUpdated();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_GatewayManager,
                Root.TraceId,
                "Config update handling failed {0}",
                error);
        }
    }
}

ErrorCode FabricApplicationGatewayManager::OnSettingsUpdated()
{
    auto error = GetGatewayCertThumbprints(nullptr, certThumbprint_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_GatewayManager,
            Root.TraceId,
            "GetGatewayCertThumbprints failed {0}",
            error);

        return error;
    }
    CreateCertMonitorTimerIfNeeded();
    return ActivateOnSettingsUpdate();
}

ErrorCode FabricApplicationGatewayManager::ActivateOnSettingsUpdate()
{
    HostedServiceParameters params;
    auto error = GetHostedServiceParameters(params);
    if (!error.IsSuccess())
    {
        return error;
    }

    activatorClient_->BeginActivateHostedService(
        params,
        HostingConfig::GetConfig().ActivationTimeout,
        [this](AsyncOperationSPtr const &op)
        {
            auto error = this->activatorClient_->EndActivateHostedService(op);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceType_GatewayManager,
                    Root.TraceId,
                    "Activate hosted service failed : {0}",
                    error);
            }
        },
        Root.CreateAsyncOperationRoot());

    return ErrorCodeValue::Success;
}

void FabricApplicationGatewayManager::CreateCertMonitorTimerIfNeeded()
{
    TimeSpan certMonitorInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;
    if (certMonitorTimer_ || (certMonitorInterval <= TimeSpan::Zero) || (certThumbprint_ == Thumbprint()))
        return;

    WriteInfo(
        TraceType_GatewayManager,
        Root.TraceId,
        "creating timer to check server certificate thumbprint every {0}",
        certMonitorInterval);

    AcquireExclusiveLock grab(certMonitorTimerLock_);

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

void FabricApplicationGatewayManager::StopCertMonitorTimer()
{
    AcquireExclusiveLock grab(certMonitorTimerLock_);

    if (certMonitorTimer_)
        certMonitorTimer_->Cancel();
}

void FabricApplicationGatewayManager::CertMonitorCallback()
{
    Thumbprint serverCertThumbprint;

    auto error = GetGatewayCertThumbprints(&certThumbprint_, serverCertThumbprint);
    if (!error.IsSuccess()) return;

    WriteInfo(
        TraceType_GatewayManager,
        Root.TraceId,
        "Server certificate thumbprint update: {0} -> {1}",
        certThumbprint_,
        serverCertThumbprint);

    certThumbprint_ = serverCertThumbprint;
    ActivateOnSettingsUpdate();
}
