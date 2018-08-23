// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace std;
using namespace Common;
using namespace LeaseWrapper;

StringLiteral const TraceState("State");
StringLiteral const TraceTTL("TTL");
StringLiteral const TraceReport("Report");
StringLiteral const TraceArbitration("Arbitration");
StringLiteral const TraceCertVerify("RemoteCertVerify");
StringLiteral const TraceSecSettings("SecuritySettings");
StringLiteral const TraceHeartbeat("Heartbeat");

#pragma region Callbacks
void WINAPI OnLeaseEstablished( __in HANDLE leaseHandle, __in LPCWSTR remoteLeasingApplicationId, __in PVOID context )
{
    if (context != NULL)
    {
        LeaseAgent *la = (LeaseAgent *) context;
        la->OnLeaseEstablished( remoteLeasingApplicationId, leaseHandle);
    }
}

void WINAPI OnLeaseFailed( __in PVOID context )
{
    if (context != NULL)
    {
        LeaseAgent *la = (LeaseAgent *) context;
        la->OnLeaseFailed();
    }
}

void WINAPI OnRemoteLeasingApplicationFailed( __in LPCWSTR remoteLeasingApplicationId, __in PVOID context)
{
    if (context != NULL)
    {
        LeaseAgent *la = (LeaseAgent *) context;
        la->OnRemoteLeasingApplicationFailed( remoteLeasingApplicationId );
    }
}

void WINAPI Dummy_OnArbitration(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in LONG localTTL,
    __in LPCWSTR remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG remoteTTL,
    __in PVOID context)
{
    if (context != NULL)
    {
        LeaseAgent *la = (LeaseAgent *) context;
        la->Arbitrate(localApplicationHandle, localInstance, localTTL, remoteSocketAddress, remoteInstance, remoteTTL, 0, 0, 0, 0);
    }
}

void WINAPI OnArbitration(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in LONG localTTL,
    __in PTRANSPORT_LISTEN_ENDPOINT remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG remoteTTL,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID context)
{
    RemoteLeasingApplicationIdentifier; //TODO value is unused

    if (context != NULL)
    {
        wostringstream remoteAddress;
        remoteAddress << remoteSocketAddress->Address << L':' << remoteSocketAddress->Port;
        wstring remoteLeaseAddress = remoteAddress.str();

        LeaseAgent *la = (LeaseAgent *) context;
        la->Arbitrate(localApplicationHandle, localInstance, localTTL, remoteLeaseAddress, remoteInstance, remoteTTL, remoteVersion, monitorLeaseInstance, subjectLeaseInstance, remoteArbitrationDurationUpperBound);
    }
}

#ifndef PLATFORM_UNIX
void WINAPI OnRemoteCertificateVerify(
    DWORD cbCertificate,
    PBYTE pbCertificate,
    PVOID certVerifyCallbackOperation,
    PVOID context )
{

    if (context != NULL)
    {
        LeaseAgent *la = (LeaseAgent *) context;
        la->OnCompleteRemoteCertificateVerification(cbCertificate, pbCertificate, certVerifyCallbackOperation);
    }

}
#endif

#if !defined(PLATFORM_UNIX)
void WINAPI OnHealthReport(
    int reportCode,
    LPCWSTR dynamicProperty,
    LPCWSTR extraDescription,
    PVOID context)
{
    UNREFERENCED_PARAMETER(context);
    LeaseAgent::InvokeHealthReportCallback(reportCode, dynamicProperty, extraDescription);
}
#endif

#pragma endregion 

#pragma region Lease Driver Wrapper Functions


LeaseAgentConfiguration::LeaseAgentConfiguration(
    std::wstring const & leasingApplicationId,
    std::wstring const & localLeaseAddress, 
    bool enableArbitrate)
    :
    leasingApplicationId_(leasingApplicationId),
    localLeaseAddress_(localLeaseAddress),
    arbitrationEnabled_(enableArbitrate)
{
    Federation::FederationConfig & config = Federation::FederationConfig::GetConfig();

    leaseSuspendTimeout_ = config.LeaseSuspendTimeout;
    arbitrationTimeout_ = config.ArbitrationTimeout;
    leaseRetryCount_ = config.LeaseRetryCount;
    LeaseRenewBeginRatio_  = config.LeaseRenewBeginRatio;
    leasingAppExpiryTimeout_ = config.ApplicationLeaseDuration;
    MaxIndirectLeaseTimeout_ = config.MaxConsecutiveIndirectLeaseDuration;
}

LeaseAgentConfiguration::LeaseAgentConfiguration(LeaseAgentConfiguration const & rhs)
    :
    leasingApplicationId_(rhs.leasingApplicationId_),
    localLeaseAddress_(rhs.localLeaseAddress_),
    leaseSuspendTimeout_(rhs.leaseSuspendTimeout_),
    arbitrationTimeout_(rhs.arbitrationTimeout_),
    leaseRetryCount_(rhs.leaseRetryCount_),
    LeaseRenewBeginRatio_(rhs.LeaseRenewBeginRatio_),
    leasingAppExpiryTimeout_(rhs.leasingAppExpiryTimeout_),
    arbitrationEnabled_(rhs.arbitrationEnabled_),
    MaxIndirectLeaseTimeout_(rhs.MaxIndirectLeaseTimeout_)
{
}

std::function<void(int, wstring const &, wstring const &)> LeaseAgent::healthReportCallback_;
Common::Global<Common::RwLock> LeaseAgent::healthReportGlobalLock_ = Common::make_global<Common::RwLock>();

TimeSpan LeaseAgent::LeaseDuration() const
{
    return Federation::FederationConfig::GetConfig().LeaseDuration;
}

HANDLE LeaseAgent::RegisterLeasingApplication(SecuritySettings const & securitySettings)
{
    LEASE_CONFIG_DURATIONS leaseConfigDurations;
    InitializeLeaseConfigDurations(leaseConfigDurations);

    LONG leaseSuspendTimeout = (LONG) leaseAgentConfig_.leaseSuspendTimeout_.TotalMilliseconds();
    LONG arbitrationTimeout =  (LONG) leaseAgentConfig_.arbitrationTimeout_.TotalMilliseconds();

    LONG numberOfLeaseRetry = (LONG) leaseAgentConfig_.leaseRetryCount_;
    LONG startOfLeaseRetry = (LONG) leaseAgentConfig_.LeaseRenewBeginRatio_;
    LONG sessionExpiration = (LONG) Common::SecurityConfig::GetConfig().SessionExpiration.TotalMilliseconds();
    LONG leasingAppExpiryTimeout = (LONG) leaseAgentConfig_.leasingAppExpiryTimeout_.TotalMilliseconds();

    if ( dummyLeaseDriverEnabled_ )
    {
        return ::Dummy_RegisterLeasingApplication(
            leaseAgentConfig_.localLeaseAddress_.c_str(),
            leaseAgentConfig_.leasingApplicationId_.c_str(),
            &leaseConfigDurations,
            leaseSuspendTimeout,
            arbitrationTimeout,
            numberOfLeaseRetry,
            ::OnLeaseFailed,
            ::OnRemoteLeasingApplicationFailed,
            leaseAgentConfig_.arbitrationEnabled_ ? Dummy_OnArbitration : NULL,
            ::OnLeaseEstablished,
            this,
            &instanceId_);
    }

    TRANSPORT_LISTEN_ENDPOINT ListenEndPoint = {};
    if (!InitializeListenEndpoint(leaseAgentConfig_.localLeaseAddress_, ListenEndPoint))
    {
        return NULL;
    }

#ifdef PLATFORM_UNIX
    return ::RegisterLeasingApplication(
        &ListenEndPoint,
        leaseAgentConfig_.leasingApplicationId_.c_str(),
        &leaseConfigDurations,
        leaseSuspendTimeout,
        arbitrationTimeout,
        numberOfLeaseRetry,
        startOfLeaseRetry,
        &securitySettings,
        leasingAppExpiryTimeout,
        ::OnLeaseFailed,
        ::OnRemoteLeasingApplicationFailed,
        leaseAgentConfig_.arbitrationEnabled_ ? ::OnArbitration : NULL,
        ::OnLeaseEstablished,
        this,
        &instanceId_,
        &leaseAgentHandle_);
#else
    return ::RegisterLeasingApplication(
        &ListenEndPoint,
        leaseAgentConfig_.leasingApplicationId_.c_str(),
        &leaseConfigDurations,
        leaseSuspendTimeout,
        arbitrationTimeout,
        numberOfLeaseRetry,
        startOfLeaseRetry,
        securitySettings.SecurityProvider(),
        certHash_.ShaHash,
        certHash_.pwszStoreName,
        sessionExpiration,
        leasingAppExpiryTimeout,
        ::OnLeaseFailed,
        ::OnRemoteLeasingApplicationFailed,
        leaseAgentConfig_.arbitrationEnabled_ ? ::OnArbitration : NULL,
        ::OnLeaseEstablished,
        ::OnRemoteCertificateVerify,
        ::OnHealthReport,
        this,
        &instanceId_,
        &leaseAgentHandle_);
#endif
}

void LeaseAgent::UnregisterLeasingApplication(HANDLE handle, wstring const & leasingApplicationId, BOOL isDelayed)
{
    if ( !dummyLeaseDriverEnabled_ )
    {
        if (!::UnregisterLeasingApplication(handle, isDelayed))
        {
            DWORD errorCode = GetLastError();
            WriteWarning(TraceState,
                "{0}@{1} failed to unregister handle {2} with error: {3}",
                leasingApplicationId, leaseAgentConfig_.localLeaseAddress_,
                handle, errorCode);
            // Ignore Error during Unregistering
        }
    }
    else
    {
        ::Dummy_UnregisterLeasingApplication(handle);
    }

    WriteInfo(TraceState, "Lease agent {0}@{1} unregistered",
        leasingApplicationId, leaseAgentConfig_.localLeaseAddress_);
}

// VoteManager -> GetLeasingApplicationExpirationTime
// getIsLeaseExpired -> GetLeasingApplicationExpirationTime
BOOL LeaseAgent::GetLeasingApplicationExpirationTime(PLONG milliseconds, PLONGLONG kernelCurrentTime) const
{
    if (!dummyLeaseDriverEnabled_)
    {
        LONG requestTTL = calledFromIPC_ ? MAXLONG : static_cast<LONG>(Federation::FederationConfig::GetConfig().ApplicationLeaseDuration.TotalMilliseconds());
        return ::GetLeasingApplicationExpirationTime(appHandle_, requestTTL, milliseconds, kernelCurrentTime);
    }
    else
    {
        return ::Dummy_GetLeasingApplicationExpirationTime(appHandle_, milliseconds, kernelCurrentTime);
    }
}

// ApplicationHostManager -> GetLeasingApplicationExpirationTimeFromIPC, for initial app ttl
// LeaseMonitor ->(IPC) HostingSubsystem -> GetLeasingApplicationExpirationTimeFromIPC
BOOL LeaseAgent::GetLeasingApplicationExpirationTimeFromIPC(PLONG milliseconds, PLONGLONG kernelCurrentTime) const
{
    calledFromIPC_ = true;
    return GetLeasingApplicationExpirationTime(milliseconds, kernelCurrentTime);
}

// static function
// LeaseMonitor ->(function) GetLeasingApplicationExpirationTime
BOOL LeaseAgent::GetLeasingApplicationExpirationTime(HANDLE appHandle, PLONG milliseconds, PLONGLONG kernelCurrentTime)
{
    static bool configLoaded = false;
    static bool dummyLeaseDriverEnabled = false;
    static RwLock loadLock;

    if (!configLoaded)
    {
        AcquireExclusiveLock grab(loadLock);
        dummyLeaseDriverEnabled = LeaseConfig::GetConfig().DebugLeaseDriverEnabled;
        configLoaded = true;
    }

    if (!dummyLeaseDriverEnabled)
    {
        LONG requestTTL = static_cast<LONG>(Federation::FederationConfig::GetConfig().ApplicationLeaseDuration.TotalMilliseconds());
        return ::GetLeasingApplicationExpirationTime(appHandle, requestTTL, milliseconds, kernelCurrentTime);
    }
    else
    {
        return ::Dummy_GetLeasingApplicationExpirationTime(appHandle, milliseconds, kernelCurrentTime);
    }
}

BOOL LeaseAgent::SetGlobalLeaseExpirationTime(LONGLONG expireTime)
{
    if ( !dummyLeaseDriverEnabled_ )
    {
        return ::SetGlobalLeaseExpirationTime(appHandle_, expireTime);
    }
    else
    {
        return ::Dummy_SetGlobalLeaseExpirationTime(appHandle_, expireTime);
    }
}

BOOL LeaseAgent::GetRemoteLeaseExpirationTime(wstring const & remoteLeasingApplicationId, StopwatchTime & monitorExpireTime, StopwatchTime & subjectExpireTime)
{
    BOOL result;
    LONG monitorTTL = 0;
    LONG subjectTTL = 0;

    StopwatchTime t1 = Stopwatch::Now();

    if (!dummyLeaseDriverEnabled_)
    {
        result = ::GetRemoteLeaseExpirationTime(appHandle_, remoteLeasingApplicationId.c_str(), &monitorTTL, &subjectTTL);
    }
    else
    {
        result = ::Dummy_GetRemoteLeaseExpirationTime(appHandle_, remoteLeasingApplicationId.c_str(), &monitorTTL, &subjectTTL);
    }

    if (result)
    {
        // Use upper bound for monitor side
        StopwatchTime t2 = Stopwatch::Now();
        monitorExpireTime = (monitorTTL == -MAXLONG ? StopwatchTime::Zero : t2 + TimeSpan::FromMilliseconds(monitorTTL) + StopwatchTime::TimerMargin);

        // Use lower bound for subject side
        subjectExpireTime = (subjectTTL == -MAXLONG ? StopwatchTime::Zero : t1 + TimeSpan::FromMilliseconds(subjectTTL) - StopwatchTime::TimerMargin);
    }

    return result;
}

BOOL LeaseAgent::CompleteArbitrationSuccessProcessing(
    HANDLE appHandle,
    LONGLONG localInstance,
    __in LPCWSTR remoteLeaseAddress,
    LONGLONG remoteInstance,
    LONG localTTL,
    LONG remoteTTL,
    BOOL isDelayed)
{
    if (dummyLeaseDriverEnabled_)
    {
        return ::Dummy_CompleteArbitrationSuccessProcessing(appHandle, localInstance, remoteLeaseAddress, remoteInstance, localTTL, remoteTTL);
    }

    TRANSPORT_LISTEN_ENDPOINT endPoint = {};
    if (!InitializeListenEndpoint(remoteLeaseAddress, endPoint))
    {
        return FALSE;
    }

    return ::CompleteArbitrationSuccessProcessing(appHandle, localInstance, &endPoint, remoteInstance, localTTL, remoteTTL, isDelayed);
}

#if !defined(PLATFORM_UNIX)
BOOL LeaseAgent::UpdateLeaseGlobalConfig()
{
    if (dummyLeaseDriverEnabled_)
    {
        return FALSE;
    }

    WriteInfo(TraceState, "Lease agent {0} updating global config, maintenanceInterval = {1}, processAssertExitTimeout = {2}, delayLeaseAgentCloseInterval = {3}",
        *this,
        static_cast<LONG>(Federation::FederationConfig::GetConfig().LeaseMaintenanceInterval.TotalMilliseconds()),
        static_cast<LONG>(Federation::FederationConfig::GetConfig().ProcessAssertExitTimeout.TotalMilliseconds()),
        static_cast<LONG>(Federation::FederationConfig::GetConfig().DelayLeaseAgentCloseInterval.TotalMilliseconds())
        );

    LEASE_GLOBAL_CONFIG leaseGlobalConfig = {};
    leaseGlobalConfig.MaintenanceInterval = static_cast<LONG>(Federation::FederationConfig::GetConfig().LeaseMaintenanceInterval.TotalMilliseconds());
    leaseGlobalConfig.ProcessAssertExitTimeout = static_cast<LONG>(Federation::FederationConfig::GetConfig().ProcessAssertExitTimeout.TotalMilliseconds());
    leaseGlobalConfig.DelayLeaseAgentCloseInterval = static_cast<LONG>(Federation::FederationConfig::GetConfig().DelayLeaseAgentCloseInterval.TotalMilliseconds());
    return ::UpdateLeaseGlobalConfig(&leaseGlobalConfig);
}
#endif

BOOL LeaseAgent::UpdateLeaseDuration()
{
    if (!dummyLeaseDriverEnabled_ && leaseAgentHandle_)
    {
        LEASE_CONFIG_DURATIONS leaseConfigDurations;
        InitializeLeaseConfigDurations(leaseConfigDurations);

        return ::UpdateLeaseDuration(leaseAgentHandle_, &leaseConfigDurations);
    }

    return TRUE;
}

BOOL LeaseAgent::CompleteRemoteCertificateVerification(
    PVOID RemoteCertVerifyCallbackOperation,
    NTSTATUS verifyResult
    )
{
    if (::CompleteRemoteCertificateVerification(appHandle_, RemoteCertVerifyCallbackOperation, verifyResult))
    {
        return true;
    }
    else
    {
        DWORD errorCode = GetLastError();

        WriteWarning(TraceState, "lease agent complete remote cert verify failed: operation {0}, verify result {1}, error code {2}", 
            RemoteCertVerifyCallbackOperation, verifyResult, errorCode);

        return false;
    }

}

#pragma endregion

LeaseAgent::LeaseAgent(
    Common::ComponentRoot const & root, 
    ILeasingApplication & leasingApp, 
    LeaseAgentConfiguration const & leaseAgentConfig, 
    Transport::SecuritySettings const& securitySettings)
    :
    RootedObject( root ),
    leasingApp_ ( leasingApp ),
    leaseAgentConfig_( leaseAgentConfig ),
    pRoot_ ( nullptr ),
    appHandle_ ( NULL ),
    leaseAgentHandle_(NULL),
    instanceId_ ( 0 ),
    calledFromIPC_(false),
    expiryTime_ (0),
    dummyLeaseDriverEnabled_(LeaseConfig::GetConfig().DebugLeaseDriverEnabled),
    initialSecuritySetting_(securitySettings),
    addressResolveType_(ResolveOptions::Invalid),
    leaseDurationChangeHandler_(0),
    leaseDurationAcrossFDChangeHandler_(0),
    unresponsiveDurationChangeHandler_(0),
#if !defined(PLATFORM_UNIX)
    maintenanceIntervalChangeHandler_(0),
    processAssertExitTimeoutChangeHandler_(0),
    delayLeaseAgentCloseIntervalChangeHandler_(0),
#endif
    random_(),
    heartbeatTimer_(),
    diskProbeFileHandle_(INVALID_HANDLE_VALUE),
    diskProbeBuffer_(Federation::FederationConfig::GetConfig().DiskProbeBufferSectorCount * Federation::FederationConfig::GetConfig().DiskSectorSize, (unsigned char) Federation::FederationConfig::GetConfig().DiskProbeBufferData)
{
#ifndef PLATFORM_UNIX
    if (securitySettings.SecurityProvider() == SecurityProvider::Ssl)
    {
        InitializeCertHashStore(securitySettings);
    }
    else
    {
        memset(certHash_.ShaHash, 0, SHA1_LENGTH);
        certHash_.pwszStoreName[0] = L'\0';
    }
#endif
}

#ifndef PLATFORM_UNIX
bool LeaseAgent::InitializeCertHashStore(SecuritySettings const & securitySettings)
{
    Thumbprint thumbprint;
    auto error = SecurityUtil::GetX509SvrCredThumbprint(
        securitySettings.X509StoreLocation(),
        securitySettings.X509StoreName(),
        securitySettings.X509FindValue(),
        nullptr,
        thumbprint);

    if (!error.IsSuccess())
    {
        WriteError(
            TraceSecSettings,
            "SecurityUtil::GetX509SvrCredThumbprint({0}, {1}, {2}) failed: {3}", 
            securitySettings.X509StoreLocation(),
            securitySettings.X509StoreName(),
            securitySettings.X509FindValue(),
            error);

        memset(certHash_.ShaHash, 0, SHA1_LENGTH);
        certHash_.pwszStoreName[0] = L'\0';

        return false;
    }

    errno_t err;
    err = memcpy_s(certHash_.ShaHash, SHA1_LENGTH, thumbprint.Hash(), SHA1_LENGTH);

    if (err)
    {
        WriteError(TraceSecSettings, "Memory Copy Failed: {0}", err);
        memset(certHash_.ShaHash, 0, SHA1_LENGTH);
        certHash_.pwszStoreName[0] = L'\0';
        return false;
    }

    if (wcsncpy_s(certHash_.pwszStoreName, ARRAYSIZE(certHash_.pwszStoreName), securitySettings.X509StoreName().c_str(), _TRUNCATE) == STRUNCATE)
    {
        WriteError(TraceSecSettings, "wcsncpy_s(certHash_.pwszStoreName, securitySettings.X509StoreName().c_str()) failed");
        return false;
    }

    return true;
}
#endif

#ifndef PLATFORM_UNIX
BOOL LeaseAgent::UpdateCertificate()
{
    if ( !dummyLeaseDriverEnabled_ )
    {
        return ::UpdateCertificate(
            appHandle_,
            certHash_.ShaHash,
            certHash_.pwszStoreName);
    }
    
    return true;
}
#endif

BOOL LeaseAgent::SetSecurityDescriptor()
{
    if ( !dummyLeaseDriverEnabled_ )
    {
        return ::SetListenerSecurityDescriptor(leaseAgentHandle_, leaseSecurity_->ConnectionAuthSecurityDescriptor());
    }

    return TRUE;
}

_Use_decl_annotations_
bool LeaseAgent::InitializeListenEndpoint(
    wstring const & leaseAgentAddress,
    TRANSPORT_LISTEN_ENDPOINT & listenEndPoint)
{
    return InitializeListenEndpoint(this, leaseAgentAddress, listenEndPoint);
}

_Use_decl_annotations_
bool LeaseAgent::InitializeListenEndpoint(
    LeaseAgent* leaseAgent,
    wstring const & LeaseAgentAddress,
    TRANSPORT_LISTEN_ENDPOINT & listenEndPoint)
{
    WriteInfo(TraceState, "Resolve ListenEndPoint for lease agent: {0}", LeaseAgentAddress);

    // Set resolve type if necessary
    Transport::ResolveOptions::Enum resolveTypeLocal = ResolveOptions::Invalid;
    Transport::ResolveOptions::Enum & resolveType = leaseAgent? leaseAgent->addressResolveType_ : resolveTypeLocal;
    if (resolveType == ResolveOptions::Invalid)
    {
        // racing here is fine here since addressResolveType_ is an integer
        TransportConfig & transportConfig = TransportConfig::GetConfig();
        if (!ResolveOptions::TryParse(transportConfig.ResolveOption, resolveType))
        {
            WriteError(TraceState, "InitializeResolveType failed as parse the resolve options {0} failed", transportConfig.ResolveOption);
            SetLastError(ERROR_INVALID_PARAMETER);
            listenEndPoint.ResolveType = AF_UNSPEC;
            return false;
        }
    }

    listenEndPoint.ResolveType = static_cast<ADDRESS_FAMILY>(resolveType);

    Endpoint endpoint;
    ErrorCode error = TcpTransportUtility::TryParseEndpointString(LeaseAgentAddress, endpoint);
    if (error.IsSuccess())
    {
        endpoint.GetIpString(listenEndPoint.Address);
        listenEndPoint.Port = endpoint.Port;
    }
    else if (error.ToHResult() == HRESULT_FROM_WIN32(WSAEINVAL))
    {
        // LeaseAgentAddress has hostname
        wstring hostname;
        USHORT port;
        if (!TcpTransportUtility::TryParseHostNameAddress(
                LeaseAgentAddress,
                hostname,
                port))
        {
            WriteError(
                TraceState,
                "Failed to parse {0} as lease agent address string",
                LeaseAgentAddress);

            SetLastError(ERROR_INVALID_PARAMETER);
            return false;
        }

        listenEndPoint.Port = static_cast<unsigned short>(port);

        if (wcsncpy_s(listenEndPoint.Address, ARRAYSIZE(listenEndPoint.Address), hostname.c_str(), _TRUNCATE) == STRUNCATE)
        {
            WriteError(
                TraceState,
                "hostname {0} is too long",
                hostname);

            return false;
        }
    }
    else
    {
        WriteError(TraceState, "Encountered unexpected error {0} when parsing {1}", error, LeaseAgentAddress);
        return false;
    }

    if (leaseAgent)
    {
        AcquireReadLock lockInScope(leaseAgent->leaseSecurityLock_);
        if (SecurityProvider::IsWindowsProvider(leaseAgent->leaseSecurity_->SecurityProvider))
        {
            if (wcsncpy_s(
                listenEndPoint.SspiTarget,
                ARRAYSIZE(listenEndPoint.SspiTarget),
                SecurityConfig::GetConfig().ClusterSpn.c_str(),
                _TRUNCATE) == STRUNCATE)
            {
                WriteError(
                    TraceState,
                    "ClusterSpn {0} is too long",
                    SecurityConfig::GetConfig().ClusterSpn);

                return false;
            }
        }
    }

    return true;
}

void LeaseAgent::InitializeLeaseConfigDurations(LEASE_CONFIG_DURATIONS & durations)
{
    durations.LeaseDuration = static_cast<LONG>(Federation::FederationConfig::GetConfig().LeaseDuration.TotalMilliseconds());
    durations.LeaseDurationAcrossFD = static_cast<LONG>(Federation::FederationConfig::GetConfig().LeaseDurationAcrossFaultDomain.TotalMilliseconds());
    durations.UnresponsiveDuration = static_cast<LONG>(Federation::FederationConfig::GetConfig().UnresponsiveDuration.TotalMilliseconds());
    durations.MaxIndirectLeaseTimeout = static_cast<LONG>(Federation::FederationConfig::GetConfig().MaxConsecutiveIndirectLeaseDuration.TotalMilliseconds());

}

ErrorCode LeaseAgent::Restart(std::wstring const & newLeasingApplicationId)
{
    HANDLE oldHandle;
    wstring oldId;
    {
        AcquireExclusiveLock lock(leaseAgentLock_);
        oldId = leaseAgentConfig_.leasingApplicationId_;
        leaseAgentConfig_.leasingApplicationId_ = newLeasingApplicationId;

        oldHandle = InternalClose();
    }

    if (oldHandle)
    {
        UnregisterLeasingApplication(oldHandle, oldId, false);
    }

    return OnOpen();
}

ErrorCode LeaseAgent::OnOpen()
{
    AcquireExclusiveLock lock(leaseAgentLock_);

    if (!leaseSecurity_)
    {
        leaseSecurity_ = make_shared<TransportSecurity>();
        auto error = leaseSecurity_->Set(initialSecuritySetting_);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceSecSettings, "Lease agent {0} : failed to apply initial security settings {1} : {2}",
                *this, initialSecuritySetting_, error);
            return error;
        }
    }

    // listen to the lease duration config changes
    auto rootSPtr = Root.shared_from_this();
    leaseDurationChangeHandler_ = Federation::FederationConfig::GetConfig().LeaseDurationEntry.AddHandler([this, rootSPtr] (EventArgs const&)
    {
        this->UpdateLeaseDuration();
    });
    
    leaseDurationAcrossFDChangeHandler_ = Federation::FederationConfig::GetConfig().LeaseDurationAcrossFaultDomainEntry.AddHandler([this, rootSPtr] (EventArgs const&)
    {
        this->UpdateLeaseDuration();
    });

    unresponsiveDurationChangeHandler_ = Federation::FederationConfig::GetConfig().UnresponsiveDurationEntry.AddHandler([this, rootSPtr](EventArgs const&)
    {
        this->UpdateLeaseDuration();
    });

#if !defined(PLATFORM_UNIX)
    maintenanceIntervalChangeHandler_ = Federation::FederationConfig::GetConfig().LeaseMaintenanceIntervalEntry.AddHandler([this, rootSPtr](EventArgs const&)
    {
        this->UpdateLeaseGlobalConfig();
    });

    delayLeaseAgentCloseIntervalChangeHandler_ = Federation::FederationConfig::GetConfig().DelayLeaseAgentCloseIntervalEntry.AddHandler([this, rootSPtr](EventArgs const&)
    {
        this->UpdateLeaseGlobalConfig();
    });

    this->UpdateLeaseGlobalConfig();
#endif

    appHandle_ = RegisterLeasingApplication(initialSecuritySetting_);

    if (appHandle_ != NULL && SecurityProvider::IsWindowsProvider(leaseSecurity_->SecurityProvider))
    {
        if (!SetSecurityDescriptor())
        {
            DWORD winErrorCode = GetLastError();
            WriteError(TraceSecSettings, "Lease agent could not be opened because SetSecurityDescriptor failed with error code {0}", winErrorCode);
            return Common::ErrorCode::FromWin32Error(winErrorCode);
        }        
    }

    if (appHandle_ == NULL)
    {
        DWORD errorCode = GetLastError();

        if (errorCode == ERROR_ACCESS_DENIED)
        {
            WriteError(TraceState, "Lease agent {0} register with driver failed with ERROR_ACCESS_DENIED; Check system section in device manager to see if the driver is in working state and you have authorization to access driver.", *this);
        }
        else if (errorCode == ERROR_FILE_NOT_FOUND)
        {
            WriteError(TraceState, "Lease agent {0} register with driver failed with error ERROR_FILE_NOT_FOUND; Check system section in device manager to see if the driver is installed.", *this);
        }
        else if (errorCode == ERROR_INVALID_PARAMETER || errorCode == STATUS_INVALID_PARAMETER)
        {
            WriteError(TraceState, "Lease agent {0} register with driver failed with ERROR_INVALID_PARAMETER; Check configuration for any invalid inputs.", *this);
        }
        else if (errorCode == ERROR_REVISION_MISMATCH)
        {
            WriteError(TraceState, "Lease agent {0} register with driver failed with ERROR_REVISION_MISMATCH; Check if the driver is up to date.", *this);
        }

        TESTASSERT_IF(
            errorCode == ERROR_INVALID_HANDLE ||
            errorCode == ERROR_FILE_NOT_FOUND ||
            errorCode == ERROR_ACCESS_DENIED ||
            errorCode == ERROR_INVALID_PARAMETER ||
            errorCode == STATUS_INVALID_PARAMETER ||
            errorCode == STATUS_UNSUCCESSFUL ||
            errorCode == ERROR_REVISION_MISMATCH,
            "{0} got invalid return code {1} for registration",
            *this, errorCode);
                   
        WriteWarning(TraceState,
            "Lease agent {0} opened failed: {1}",
            *this, errorCode);

        if (ERROR_RETRY == errorCode)
            return ErrorCode( ErrorCode::FromWin32Error(errorCode) );
        else
            return ErrorCode( ErrorCodeValue::RegisterWithLeaseDriverFailed );
    }

    pRoot_ = Root.shared_from_this();

    WriteInfo(TraceState,
        "Lease agent {0} opened with handle {1}, instance {2}, config: {3}/{4}/{5}",
        *this, appHandle_, instanceId_,
        LeaseDuration() ,
        LeaseSuspendTimeout(),
        ArbitrationTimeout());
        
    TryStartHeartbeat();

    return ErrorCodeValue::Success;
}

ErrorCode LeaseAgent::OnClose()
{
    Cleanup(false);
    return ErrorCode( ErrorCodeValue::Success );
}

void LeaseAgent::OnAbort()
{
    Cleanup(true);
}

void LeaseAgent::Cleanup(bool isDelayed)
{
    HANDLE oldHandle = INVALID_HANDLE_VALUE;
    wstring id;
    {
        AcquireExclusiveLock lock(leaseAgentLock_);
        id = leaseAgentConfig_.leasingApplicationId_;
        oldHandle = InternalClose();
    }

    if (oldHandle)
    {
        UnregisterLeasingApplication(oldHandle, id, isDelayed);
    }

    if (heartbeatTimer_)
    {
        heartbeatTimer_->Cancel();
        if (diskProbeFileHandle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(diskProbeFileHandle_);
        }
    }

    pRoot_ = nullptr; // Release Reference
}

HANDLE LeaseAgent::InternalClose()
{
    HANDLE oldHandle = appHandle_;

    if (appHandle_ != NULL)
    {
        WriteInfo(TraceState,
            "Closing leases for {0} with handle {1}",
            *this, appHandle_);

        appHandle_ = NULL;
        leaseAgentHandle_ = NULL;

        for (LeasePartnerMap::value_type const & lease : leases_)
        {
            lease.second->Abort();
        }

        leases_.clear();
    }

    if (leaseDurationChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().LeaseDurationEntry.RemoveHandler(leaseDurationChangeHandler_);
        leaseDurationChangeHandler_ = 0;
    }

    if (leaseDurationAcrossFDChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().LeaseDurationAcrossFaultDomainEntry.RemoveHandler(leaseDurationAcrossFDChangeHandler_);
        leaseDurationAcrossFDChangeHandler_ = 0;
    }

    if (unresponsiveDurationChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().UnresponsiveDurationEntry.RemoveHandler(unresponsiveDurationChangeHandler_);
        unresponsiveDurationChangeHandler_ = 0;
    }

#if !defined(PLATFORM_UNIX)
    if (maintenanceIntervalChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().LeaseMaintenanceIntervalEntry.RemoveHandler(maintenanceIntervalChangeHandler_);
        maintenanceIntervalChangeHandler_ = 0;
    }
    if (processAssertExitTimeoutChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().ProcessAssertExitTimeoutEntry.RemoveHandler(processAssertExitTimeoutChangeHandler_);
        processAssertExitTimeoutChangeHandler_ = 0;
    }
    if (delayLeaseAgentCloseIntervalChangeHandler_)
    {
        Federation::FederationConfig::GetConfig().DelayLeaseAgentCloseIntervalEntry.RemoveHandler(delayLeaseAgentCloseIntervalChangeHandler_);
        delayLeaseAgentCloseIntervalChangeHandler_ = 0;
    }
#endif

    return oldHandle;
}

LeasePartner* LeaseAgent::GetLeasePartnerByAddress(wstring const & remoteLeaseAddress)
{
    for (auto it = leases_.begin(); it != leases_.end(); ++it)
    {
        if (it->second->GetRemoteLeaseAgentAddress() == remoteLeaseAddress)
        {
            return it->second.get();
        }
    }

    return nullptr;
}

void LeaseAgent::Establish(
    wstring const & remoteLeasingApplicationId,
    wstring const & remoteFaultDomain,
    wstring const & remoteLeaseAddress,
    int64 remoteLeaseAgentInstanceId,
    LEASE_DURATION_TYPE leaseTimeoutType)
{
    // Optimize to avoid actually creating async operation?
    AsyncOperationSPtr context = BeginEstablish(
        remoteLeasingApplicationId,
        remoteFaultDomain,
        remoteLeaseAddress,
        remoteLeaseAgentInstanceId,
        leaseTimeoutType,
        TimeSpan::MaxValue,
        [](AsyncOperationSPtr const & operation) 
    {
        ErrorCode code = LeaseAgent::EndEstablish(operation);
        code.ReadValue();
    },
        nullptr);
}

AsyncOperationSPtr LeaseAgent::BeginEstablish(
    wstring const & remoteLeasingApplicationId,
    wstring const & remoteFaultDomain,
    wstring const &remoteLeaseAgentAddress,
    int64 remoteLeaseAgentInstanceId,
    LEASE_DURATION_TYPE leaseTimeoutType,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & context)

{
    AcquireExclusiveLock lock(leaseAgentLock_);
    if (!appHandle_)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::OperationFailed, callback, context);
    }

    LeasePartnerSPtr lease;

    auto iter = leases_.find( remoteLeasingApplicationId );
    if (iter == leases_.end())
    {
        lease = make_shared<LeasePartner>(*this, appHandle_, leaseAgentConfig_.leasingApplicationId_, remoteLeasingApplicationId, remoteFaultDomain, remoteLeaseAgentAddress, remoteLeaseAgentInstanceId);
        leases_.insert( LeasePartnerMap::value_type( remoteLeasingApplicationId, lease ) );
    }
    else
    {
        lease = iter->second;
    }

    return lease->Establish(leaseTimeoutType, timeout, callback, context );
}

ErrorCode LeaseAgent::EndEstablish(__in AsyncOperationSPtr const & operation)
{    
    auto thisPtr = AsyncOperation::End<AsyncOperation>(operation);
    return thisPtr->Error;
}

void LeaseAgent::RetryEstablish( LeasePartnerSPtr const & leaseParnter, LEASE_DURATION_TYPE leaseTimeoutType)
{
    AcquireExclusiveLock lock(leaseAgentLock_);
    if (appHandle_)
    {
        leaseParnter->Retry(leaseTimeoutType);
    }
}

void LeaseAgent::Terminate( wstring const & remoteLeasingApplicationId )
{
    AcquireExclusiveLock lock(leaseAgentLock_);
    if (!appHandle_)
    {
        return;
    }

    auto iter = leases_.find( remoteLeasingApplicationId );
    if (iter != leases_.end())
    {
        iter->second->Terminate();
        leases_.erase(iter);
    }
}

bool LeaseAgent::getIsLeaseExpired()
{
    StopwatchTime now = Stopwatch::Now();
    if (now.Ticks < expiryTime_)
    {
        return false;
    }

    // We only need the read lock here, since the write operation has the interlock protected
    AcquireReadLock lock(leaseAgentLock_);
    if (!appHandle_)
    {
        return false;
    }

    LONG milliseconds = 0;
    LONGLONG kernelNow = 0;
    if (!GetLeasingApplicationExpirationTime(&milliseconds, &kernelNow))
    {
        DWORD errorCode = GetLastError();
        ASSERT_IF( errorCode != ERROR_INVALID_HANDLE &&
            errorCode != ERROR_INVALID_PARAMETER &&
            errorCode != STATUS_INVALID_PARAMETER,
            "{0} got error code {1} from GetLeasingApplicationExpirationTime",
            *this, errorCode );
        return true;
    }

    LeaseAgentEventSource::Events->TTL(
        leaseAgentConfig_.leasingApplicationId_,
        leaseAgentConfig_.localLeaseAddress_,
        static_cast<int64>(milliseconds));
    
    if (milliseconds == 0)
        return true;
    
    LONGLONG newExpiryTime = 0;
    LONGLONG oldExpiryTime = expiryTime_;

    if (MAXLONG == milliseconds)
        newExpiryTime = (now + LeaseDuration()).Ticks;
    else
        newExpiryTime = (now + TimeSpan::FromMilliseconds(static_cast<double>(milliseconds))).Ticks;
    
    LONGLONG initialValue;
    for (;;)
    {
        initialValue = InterlockedCompareExchange64(&expiryTime_, newExpiryTime, oldExpiryTime);
        if (initialValue == oldExpiryTime) break;
        
        // If the current value is greater than me, take the greater value, skip update
        if (newExpiryTime <= initialValue)
        {
            return now.Ticks >= initialValue;
        }

        WriteNoise(TraceReport, "InterlockedCompareExchange64 needs to be retried {0} = {1} -> {2}", initialValue, oldExpiryTime, newExpiryTime);

        oldExpiryTime = initialValue;
    }

    return now.Ticks >= newExpiryTime;
}

void LeaseAgent::OnLeaseFailed()
{
    ComponentRootSPtr const & root = pRoot_;

    // Notify Leasing Application
    Threadpool::Post(
        [this, root] () mutable { this->leasingApp_.OnLeaseFailed(); });
}

void LeaseAgent::OnRemoteLeasingApplicationFailed( wstring const & remoteLeasingApplicationId )
{
    {
        // Find and Erase Lease Partner
        AcquireExclusiveLock lock(leaseAgentLock_);

        WriteInfo(TraceReport,
            "{0} get failure notification for {1}",
            *this, remoteLeasingApplicationId );

        auto iter = leases_.find( remoteLeasingApplicationId );
        if (iter != leases_.end())
        {
            iter->second->Abort();
            leases_.erase(iter);
        }
    }

    // Notify Leasing Application
    ComponentRootSPtr const & root = pRoot_;
    Threadpool::Post(
        [this, root, remoteLeasingApplicationId] { leasingApp_.OnRemoteLeasingApplicationFailed(remoteLeasingApplicationId); });
}

void LeaseAgent::OnLeaseEstablished( std::wstring remoteLeasingApplicationId, HANDLE leaseHandle )
{
    AcquireExclusiveLock lock(leaseAgentLock_);
    auto iter = leases_.find( remoteLeasingApplicationId );
    if (iter != leases_.end())
    {
        iter->second->OnEstablished( leaseHandle );
    }
}

void LeaseAgent::Arbitrate(
    HANDLE appHandle,
    int64 localInstance,
    long localTTL,
    wstring const & remoteLeaseAddress,
    int64 remoteInstance,
    long remoteTTL,
    USHORT remoteVersion,
    LONGLONG monitorLeaseInstance,
    LONGLONG subjectLeaseInstance,
    LONG remoteArbitrationDurationUpperBound)
{
    // TODO: Arbitration Support Methods not implemented.
    if (leaseAgentConfig_.arbitrationEnabled_)
    {
        LeaseAgentInstance local(leaseAgentConfig_.localLeaseAddress_, localInstance);
        LeaseAgentInstance remote(remoteLeaseAddress, remoteInstance);
        {
            AcquireExclusiveLock lock(leaseAgentLock_);
            if (appHandle != appHandle_)
            {
                WriteError(TraceArbitration, "{0} arbitration failed as lease agent has already cleaned up", *this);
                CompleteArbitrationSuccessProcessing(appHandle, localInstance, remoteLeaseAddress.c_str(), remoteInstance, 0, MAXLONG, FALSE);
                return;
            }

            LeasePartner* remotePartner = GetLeasePartnerByAddress(remoteLeaseAddress);
            if (remotePartner)
            {
                remotePartner->OnArbitration();
            }
        }

        ComponentRootSPtr const & root = pRoot_;
        Threadpool::Post(
            [root, this, local, localTTL, remote, remoteTTL, remoteVersion, monitorLeaseInstance, subjectLeaseInstance, remoteArbitrationDurationUpperBound]
            {
                this->leasingApp_.Arbitrate(local, TimeSpan::FromMilliseconds(localTTL), remote, TimeSpan::FromMilliseconds(remoteTTL) + Federation::FederationConfig::GetConfig().LeaseClockUncertaintyInterval, remoteVersion, monitorLeaseInstance, subjectLeaseInstance, remoteArbitrationDurationUpperBound);
            });
    }
    else
    {
        WriteError(TraceArbitration, "{0} arbitration failed as lease agent is not arbitration enabled", *this);
        CompleteArbitrationSuccessProcessing(appHandle, localInstance, remoteLeaseAddress.c_str(), remoteInstance, 0, MAXLONG, FALSE);
    }
}

long ConvertTimeSpanToMilliseconds(TimeSpan ttl)
{
    return (ttl == TimeSpan::MaxValue ? MAXLONG : static_cast<long>(ttl.TotalMilliseconds()));
}

void LeaseAgent::CompleteArbitrationSuccessProcessing(LONGLONG localInstance, __in LPCWSTR remoteLeaseAddress, LONGLONG remoteInstance, TimeSpan localTTL, TimeSpan remoteTTL, BOOL isDelayed)
{
    AcquireExclusiveLock lock(leaseAgentLock_);
    if (!appHandle_)
    {
        return;
    }
    
    if (!CompleteArbitrationSuccessProcessing(appHandle_, localInstance, remoteLeaseAddress, remoteInstance, ConvertTimeSpanToMilliseconds(localTTL), ConvertTimeSpanToMilliseconds(remoteTTL), isDelayed))
    {
        DWORD errorCode = GetLastError();
        ASSERT_IF(errorCode != ERROR_INVALID_HANDLE && errorCode != ERROR_INVALID_PARAMETER,
            "{0} got invalid error code {1} from CompleteArbitrationSuccessProcessing",
            *this, errorCode);

        WriteWarning(TraceArbitration,
            "{0} got error code {1} from CompleteArbitrationSuccessProcessing {2}:{3}-{4}:{5}={6}/{7}",
            *this, errorCode, leaseAgentConfig_.localLeaseAddress_, localInstance, remoteLeaseAddress, remoteInstance, localTTL, remoteTTL);
    }

    if (localTTL == TimeSpan::MaxValue && remoteTTL == TimeSpan::MaxValue)
    {
        LeasePartner* remotePartner = GetLeasePartnerByAddress(remoteLeaseAddress);
        if (remotePartner)
        {
            remotePartner->EstablishAfterArbitration(remoteInstance);
        }
    }
}

#if !defined(PLATFORM_UNIX)
DWORD LeaseAgent::LoadRemoteCertificate(
    DWORD cbCertificate,
    PBYTE pbCertificate,
    PCCERT_CONTEXT *ppCertContext)
{
    HCERTSTORE      hCertStore   = NULL;  
    PCCERT_CONTEXT  pCertContext = NULL;  
    PBYTE           pbCurrentRaw;  
    DWORD           cbCurrentRaw;  
    BOOL            fLeafCert;  
    DWORD           dwError;  

    if (ppCertContext == NULL)  
    {  
        return static_cast<DWORD>(SEC_E_INTERNAL_ERROR);
    }  
  
    if (*ppCertContext != NULL)  
    {  
        CertFreeCertificateContext(*ppCertContext);  
    }  
  
    *ppCertContext = NULL;  

    //
    // Create an in-memory certificate store.
    //

    hCertStore = CertOpenStore(
                    CERT_STORE_PROV_MEMORY,
                    0,
                    0,
                    CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                    0);

    if (hCertStore == NULL)  
    {
        dwError = GetLastError();
        WriteWarning(TraceState, "Failed to open in memory cert store : {0}", dwError);
        return dwError;
    }

    fLeafCert    = TRUE;  
    pbCurrentRaw = pbCertificate;
    cbCurrentRaw = cbCertificate;

    do   
    {  
        //
        // Skip to beginning of certificate.
        //
        if (cbCurrentRaw < sizeof(DWORD))  
        {
            dwError = GetLastError();
            WriteWarning(TraceState, "cbCurrentRaw is less than DWORD");
            goto Cleanup;  
        }  
  
        cbCertificate = *(UNALIGNED DWORD*)pbCurrentRaw;  
  
        pbCurrentRaw += sizeof(DWORD);
        cbCurrentRaw -= sizeof(DWORD);
  
        if (cbCertificate > cbCurrentRaw)  
        {
            dwError = GetLastError();
            WriteWarning(TraceState, "cbCertificate is greater than cbCurrentRaw");
            goto Cleanup;  
        }  
  
        //
        // Add this certificate to store.
        //
        if (!CertAddEncodedCertificateToStore(  
                hCertStore,   
                X509_ASN_ENCODING,  
                pbCurrentRaw,  
                cbCertificate,  
                CERT_STORE_ADD_USE_EXISTING,  
                &pCertContext))  
        {
            dwError = GetLastError();
            WriteWarning(TraceState, "Failed to add certificate to store");
            goto Cleanup;  
        }  
  
        pbCurrentRaw += cbCertificate;  
        cbCurrentRaw -= cbCertificate;  
  
        if (fLeafCert)  
        {  
            fLeafCert = FALSE;  
            *ppCertContext = pCertContext;  
        }  
        else  
        {  
            CertFreeCertificateContext(pCertContext);  
        }  
        pCertContext = NULL;  
  
    } while(cbCurrentRaw);  
  
    dwError = ERROR_SUCCESS;  
  
Cleanup:  
  
    CertCloseStore(hCertStore, 0);  
  
    if (dwError != ERROR_SUCCESS)  
    {  
        if(pCertContext)  
        {  
            CertFreeCertificateContext(pCertContext);  
        }  
  
        if(*ppCertContext)  
        {  
            CertFreeCertificateContext(*ppCertContext);  
            *ppCertContext = NULL;  
        }  
    }

    return dwError;
}
#endif

TransportSecuritySPtr LeaseAgent::LeaseSecurity()
{
    AcquireReadLock lockInScope(leaseAgentLock_);
    return leaseSecurity_;
}

#if !defined(PLATFORM_UNIX)
void LeaseAgent::OnCompleteRemoteCertificateVerification(
    DWORD cbCertificate,
    PBYTE pbCertificate,
    PVOID RemoteCertVerifyCallbackOperation)
{
    NTSTATUS verifyResult = STATUS_SUCCESS;

    PCCERT_CONTEXT pCertContext = NULL;
    if (LoadRemoteCertificate(cbCertificate, pbCertificate, &pCertContext))
    {
        WriteWarning(TraceState, "failed to LoadRemoteCertificate, error {0}", GetLastError());
        return;
    }

    KFinally([=] { ::CertFreeCertificateContext(pCertContext); });

    TransportSecuritySPtr leaseSecuritySPtr = LeaseSecurity();
    verifyResult = SecurityUtil::VerifyCertificate(
        pCertContext,
        leaseSecuritySPtr->Settings().RemoteCertThumbprints(),
        leaseSecuritySPtr->Settings().RemoteX509Names(),
        true,
        leaseSecurity_->Settings().X509CertChainFlags(),
        leaseSecurity_->Settings().ShouldIgnoreCrlOfflineError(false /* this param is ignored for cluster security */));


    if (!SUCCEEDED(verifyResult))
    {
        WriteWarning(TraceState, "Failed SecurityUtil::VerifyCertificate, error {0}", verifyResult);
    }

    ComponentRootSPtr const & root = pRoot_;
    //
    // send the verfiy result back to kernel
    //
    Threadpool::Post(
        [this, root, RemoteCertVerifyCallbackOperation, verifyResult] () { this->CompleteRemoteCertificateVerification(RemoteCertVerifyCallbackOperation, verifyResult); });
}
#endif        

Common::ErrorCode LeaseAgent::SetSecurity(SecuritySettings const & securitySettings)
{
    auto leaseTransportSecurityNew = std::make_shared<TransportSecurity>();
    auto errorCode = leaseTransportSecurityNew->Set(securitySettings);
    if (!errorCode.IsSuccess())
    {
        WriteError(TraceSecSettings, "Invalid security settings: {0}", errorCode);
        return errorCode;
    }

    AcquireWriteLock grab(leaseAgentLock_);
    AcquireWriteLock lockInScope(leaseSecurityLock_);

    errorCode = leaseSecurity_->CanUpgradeTo(*leaseTransportSecurityNew);
    if (!errorCode.IsSuccess())
    {
        WriteError(TraceSecSettings, "CanUpgradeTo failed with error code {0}", errorCode.ErrorCodeValueToString());
        return errorCode;
    }

    leaseSecurity_ = move(leaseTransportSecurityNew);
    DWORD winErrorCode;

#ifdef PLATFORM_UNIX
    Invariant(leaseSecurity_->SecurityProvider == SecurityProvider::Ssl);
    if ( !dummyLeaseDriverEnabled_ )
    {
        if(::UpdateSecuritySettings(appHandle_, &(leaseSecurity_->Settings())) == FALSE)
        {
            winErrorCode = GetLastError();
            WriteError(TraceSecSettings, "Update Certificate failed with error code {0}", winErrorCode);
            return Common::ErrorCode::FromWin32Error(winErrorCode);
        }
    }
#else
    if (leaseSecurity_->SecurityProvider == SecurityProvider::Ssl)
    {
        LeaseCertHashStore origCertHash(certHash_);

        if (!InitializeCertHashStore(securitySettings))
        {
            winErrorCode = GetLastError();
            WriteError(TraceSecSettings, "InitializeCertHashStore failed with error code {0}", winErrorCode);
            return Common::ErrorCode::FromWin32Error(winErrorCode);
        }


        if (memcmp(certHash_.ShaHash, origCertHash.ShaHash, SHA1_LENGTH) != 0 ||
            memcmp(certHash_.pwszStoreName, origCertHash.pwszStoreName, SCH_CRED_MAX_STORE_NAME_SIZE) != 0)
        {
            if (!UpdateCertificate())
            {
                winErrorCode = GetLastError();
                WriteError(TraceSecSettings, "Update Certificate failed with error code {0}", winErrorCode);
                return Common::ErrorCode::FromWin32Error(winErrorCode);
            }
        }
    }
    else if (SecurityProvider::IsWindowsProvider(leaseSecurity_->SecurityProvider))
    {
        if (!SetSecurityDescriptor())
        {
            winErrorCode = GetLastError();
            WriteError(TraceSecSettings, "SetSecurityDescriptor failed with error code {0}", winErrorCode);
            return Common::ErrorCode::FromWin32Error(winErrorCode);
        }        
    }
#endif

    return errorCode;
}

void LeaseAgent::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0}@{1}", leaseAgentConfig_.leasingApplicationId_, leaseAgentConfig_.localLeaseAddress_);
}

void LeaseAgent::TryStartHeartbeat()
{
    #ifndef PLATFORM_UNIX
     
    if (Federation::FederationConfig::GetConfig().HeartbeatInterval != TimeSpan::Zero &&!heartbeatTimer_)
    {
        wstring fabricDataRoot;
        ErrorCode errorCode = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        wstring filename;
        wstring reopenFilename;
        if (errorCode.IsSuccess())
        {
            filename = Path::Combine(fabricDataRoot, DISK_PROBE_FILE_NAME);
            reopenFilename = Path::Combine(fabricDataRoot, DISK_PROBE_REOPEN_FILE_NAME);
        }
        else
        {
            filename = DISK_PROBE_FILE_NAME;
            reopenFilename = DISK_PROBE_REOPEN_FILE_NAME;
        }

        WriteInfo(TraceHeartbeat, "Disk probe {0}, filename = {1}, reopen filename = {2}, buffer data = {3}, size = {4}, sector count = {5}, disk sector size = {6}, reopen file = {7}",
            *this,
            filename,
            reopenFilename,
            Federation::FederationConfig::GetConfig().DiskProbeBufferData,
            Federation::FederationConfig::GetConfig().DiskProbeBufferSectorCount * Federation::FederationConfig::GetConfig().DiskSectorSize,
            Federation::FederationConfig::GetConfig().DiskProbeBufferSectorCount,
            Federation::FederationConfig::GetConfig().DiskSectorSize,
            Federation::FederationConfig::GetConfig().DiskProbeReopenFile
            );

        ComponentRootSPtr pRoot = Root.shared_from_this();
        heartbeatTimer_ = Timer::Create("Lease.Heartbeat", [this, pRoot, fabricDataRoot, filename, reopenFilename](TimerSPtr const & timer)
        {
            int diskProbeBufferSectorCount = Federation::FederationConfig::GetConfig().DiskProbeBufferSectorCount;
            int diskProbeBufferSize = Federation::FederationConfig::GetConfig().DiskProbeBufferSectorCount * Federation::FederationConfig::GetConfig().DiskSectorSize;
            int diskProbeBufferData = Federation::FederationConfig::GetConfig().DiskProbeBufferData;
            
            if (diskProbeBufferSectorCount < 0 ||/* random size */
                (diskProbeBufferSize >= 0 && diskProbeBufferSize != diskProbeBuffer_.size()) ||/* fixed size */
                (diskProbeBufferSize != 0 && diskProbeBufferData < 0) ||/* random data*/
                (diskProbeBuffer_.size() > 0 && diskProbeBuffer_[0] != diskProbeBufferData))/* fixed data*/
            {
                if (diskProbeBufferSectorCount < 0)// random size
                {
                    diskProbeBufferSize = (random_.Next(diskProbeBufferSectorCount * -1) + 1) * Federation::FederationConfig::GetConfig().DiskSectorSize;
                }

                WriteInfo(TraceHeartbeat, "Disk probe {0}, update buffer data = {1} -> {2}, size = {3} -> {4}, sector count = {5}, disk sector size = {6}",
                    *this,
                    diskProbeBuffer_.size() > 0 ? diskProbeBuffer_[0] : 0,
                    (int) diskProbeBufferData,
                    diskProbeBuffer_.size(),
                    diskProbeBufferSize,
                    diskProbeBufferSectorCount,
                    Federation::FederationConfig::GetConfig().DiskSectorSize);

                if (diskProbeBufferData >= 0)// fixed data
                {
                    diskProbeBuffer_.assign(diskProbeBufferSize, (char)diskProbeBufferData);
                }
                else// random data
                {
                    diskProbeBuffer_.resize(diskProbeBufferSize, (char)diskProbeBufferData);
                    random_.NextBytes(diskProbeBuffer_);
                }
            }

            HANDLE diskProbeReopenFileHandle = INVALID_HANDLE_VALUE;
            // We cache this value in case the value changed during the disk probe process
            bool diskProbeReopenFile = Federation::FederationConfig::GetConfig().DiskProbeReopenFile;
            HANDLE currentDiskProbeFileHandle = INVALID_HANDLE_VALUE;

            if (!diskProbeReopenFile)
            {
                // Do not reopen file, use single file
                // if size is 0, disable disk probe
                if (diskProbeBufferSize > 0 && diskProbeFileHandle_ == INVALID_HANDLE_VALUE)
                {
                    diskProbeFileHandle_ = ::CreateFile(
                        filename.c_str(),
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                        NULL);
                    currentDiskProbeFileHandle = diskProbeFileHandle_;
                }
            }
            else
            {
                // Reopen file
                if (diskProbeBufferSize > 0)
                {
                    diskProbeReopenFileHandle = ::CreateFile(
                        reopenFilename.c_str(),
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                        NULL);
                    currentDiskProbeFileHandle = diskProbeReopenFileHandle;
                }
            }

            if (diskProbeBufferSize == 0)// disable disk probe, but still do heartbeat
            {
                WriteInfo(TraceHeartbeat, "Disk probe {0}, disabled, but continue heartbeat.", *this);

                ::UpdateHeartbeatResult(ERROR_SUCCESS);

                // restart timer
                timer->Change(Federation::FederationConfig::GetConfig().HeartbeatInterval);
            }
            else if (currentDiskProbeFileHandle != INVALID_HANDLE_VALUE)// enable disk probe
            {
                // Create a stopwatchtime to time the write operation
                Common::StopwatchTime writeStartTime = Common::Stopwatch::Now();

                Common::AsyncFile::BeginWriteFile(currentDiskProbeFileHandle, diskProbeBuffer_, TimeSpan::MaxValue, [this, pRoot, timer, currentDiskProbeFileHandle, diskProbeReopenFile, writeStartTime](AsyncOperationSPtr const& writeFileOperation)
                {
                    DWORD winError;
                    ErrorCode result = Common::AsyncFile::EndWriteFile(writeFileOperation, winError);
                    WriteInfo(TraceHeartbeat, "Disk probe error code = {0}", winError);

                    // Write operation time = stop - start
                    Common::StopwatchTime writeStopTime = Common::Stopwatch::Now();
                    WriteInfo(TraceHeartbeat, "Disk probe time = {0}", writeStopTime - writeStartTime);

                    // check to see if we want to reopen the file
                    if (diskProbeReopenFile)
                    {
                        WriteInfo(TraceHeartbeat, "Disk probe reopen file.");

                        BOOL closeRes = CloseHandle(currentDiskProbeFileHandle);
                        if (closeRes == 0)
                        {
                            WriteWarning(TraceHeartbeat, "Cannot close disk probe file, error {0}", GetLastError());
                        }
                    }

                    // white list some error code
                    if (result.IsErrno(ERROR_DISK_FULL) || result.IsErrno(ERROR_HANDLE_DISK_FULL))
                    {
                        ::UpdateHeartbeatResult(ERROR_SUCCESS);
                    }
                    else
                    {
                        ::UpdateHeartbeatResult(winError);
                    }

                    // restart timer
                    timer->Change(Federation::FederationConfig::GetConfig().HeartbeatInterval);
                }, nullptr);
            }
            else// file open error
            {
                WriteWarning(TraceHeartbeat, "Disk probe {0}, cannot open disk probing file, error {1}", *this, GetLastError());
                // restart timer to retry
                timer->Change(Federation::FederationConfig::GetConfig().HeartbeatInterval);
            }
        },
        false/* concurrent */);

        heartbeatTimer_->SetCancelWait();

        //start probing immediately
        heartbeatTimer_->Change(TimeSpan::Zero);
    }
    else
    {
        WriteWarning(TraceHeartbeat, "Heartbeat {0}, is disabled, heartbeat interval = 0", *this);
    }
    #endif
}

void LeaseAgent::SetHealthReportCallback(std::function<void(int, wstring const &, wstring const &)> & callback)
{
    AcquireExclusiveLock lock(*healthReportGlobalLock_);
    healthReportCallback_ = callback;
}

#if !defined(PLATFORM_UNIX)
void LeaseAgent::InvokeHealthReportCallback(int reportCode, LPCWSTR dynamicProperty, LPCWSTR extraDescription)
{
    AcquireReadLock lock(*healthReportGlobalLock_);
    if (healthReportCallback_)
    {
        WriteInfo(TraceState, "Report health reportCode = {0}, dynamicProperty = {1}, extraDescription = {2}", reportCode, dynamicProperty, extraDescription);
        healthReportCallback_(reportCode, dynamicProperty, extraDescription);
    }
}
#endif

