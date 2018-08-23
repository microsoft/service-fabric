// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace std;

static const StringLiteral TraceType("Security");
static const Common::Global<wstring> HostSpnPrefix = Common::make_global<wstring>(WINDOWS_FABRIC_SPN_PREFIX);

bool TransportSecurity::AssignAdminRoleToLocalUser = true;
ULONG TransportSecurity::credentialRefreshCount_ = 0;

TransportSecurity::TransportSecurity(bool isClientOnly)
    : initialized_(false)
    , isClientOnly_(isClientOnly)
    , securityRequirements_(0)
    , messageHeaderProtectionLevel_(ProtectionLevel::None)
    , messageBodyProtectionLevel_(ProtectionLevel::None)
    , authenticationData_(nullptr)
    , sessionExpirationEnabled_(true)
    , localTokenHandle_(INVALID_HANDLE_VALUE)
    , runningAsMachineAccount_(false)
    , shouldWaitForAuthStatus_(false)
{
}

TransportSecurity::~TransportSecurity()
{
    if (localTokenHandle_!= INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(localTokenHandle_);
    }
}

ULONG TransportSecurity::GetCredentialRefreshCount()
{
    return credentialRefreshCount_;
}

ProtectionLevel::Enum TransportSecurity::get_HeaderSecurity() const
{
    return messageHeaderProtectionLevel_;
}

ProtectionLevel::Enum TransportSecurity::get_BodySecurity() const
{
    return messageBodyProtectionLevel_;
}

ErrorCode TransportSecurity::Set(Transport::SecuritySettings const & securitySettings)
{
    ASSERT_IF(initialized_, "Already initialized");
    initialized_ = true;
    securitySettings_ = securitySettings;

    if (this->SecurityProvider == SecurityProvider::None)
    {
        return ErrorCodeValue::Success;
    }

    if (this->SecurityProvider == SecurityProvider::Ssl)
    {
        // For SSL, protection level is a result of security negotiation
        messageHeaderProtectionLevel_ = ProtectionLevel::EncryptAndSign;
        messageBodyProtectionLevel_ = ProtectionLevel::EncryptAndSign;

        AcquireWriteLock grab(credentialLock_);
        return AcquireCredentials_CallerHoldingWLock();
    }

    if (this->SecurityProvider == SecurityProvider::Claims)
    {
        // For SSL, protection level is a result of security negotiation
        messageHeaderProtectionLevel_ = ProtectionLevel::EncryptAndSign;
        messageBodyProtectionLevel_ = ProtectionLevel::EncryptAndSign;

        AcquireWriteLock grab(credentialLock_);
        return AcquireCredentials_CallerHoldingWLock();
    }

    messageHeaderProtectionLevel_ = securitySettings.ProtectionLevel();
    messageBodyProtectionLevel_ = securitySettings.ProtectionLevel();

    if (UsingWindowsCredential())
    {
#ifdef PLATFORM_UNIX
        return ErrorCodeValue::Success; //LINUXTODO ???
#else
        auto error = InitializeLocalWindowsIdentityIfNeeded();
        if (!error.IsSuccess())
        {
            return error;
        }

        if (!isClientOnly_)
        {
            // remoteClientSids_ is used by server side to authorize incoming connections, no applicable for client only transport
            error = CreateSecurityDescriptorFromAllowedList(
                localTokenHandle_,
                true,
                securitySettings.RemoteIdentities(),
                FILE_GENERIC_EXECUTE,
                connectionAuthSecurityDescriptor_);

            if (!error.IsSuccess())
            {
                WriteError(TraceType, "Failed to create security descriptor for connection authorization: {0}", error);
                return error;
            }

            WriteInfo(TraceType, "security descriptor for connection authorization: {0}", *connectionAuthSecurityDescriptor_);

            if (securitySettings.IsClientRoleInEffect())
            {
                error = CreateSecurityDescriptorFromAllowedList(
                    localTokenHandle_,
                    AssignAdminRoleToLocalUser,
                    securitySettings.AdminClientIdentities(),
                    FILE_GENERIC_EXECUTE,
                    adminRoleSecurityDescriptor_);

                if (!error.IsSuccess())
                {
                    WriteError(TraceType, "Failed to create security descriptor for assigning admin role: {0}", error);
                    return error;
                }

                WriteInfo(TraceType, "security descriptor for assigning admin role: {0}", *adminRoleSecurityDescriptor_);
            }
        }
#endif
    }

    AcquireWriteLock grab(credentialLock_);
    return AcquireCredentials_CallerHoldingWLock();
}

_Use_decl_annotations_ ErrorCode TransportSecurity::AcquireCredentials_CallerHoldingWLock()
{
    if (this->SecurityProvider == SecurityProvider::None)
    {
        return ErrorCodeValue::Success;
    }

    if (this->SecurityProvider == SecurityProvider::Ssl)
    {
        ErrorCode error;
        if (securitySettings_.IsSelfGeneratedCert())
        {
            error = SecurityCredentials::AcquireSsl(
                *securitySettings_.CertContext(),
                &credentials_,
                isClientOnly_ ? nullptr : &svrCredentials_);
        }
        else
        {
            error = SecurityCredentials::AcquireSsl(
                securitySettings_.X509StoreLocation(),
                securitySettings_.X509StoreName(),
                securitySettings_.X509FindValue(),
                &credentials_,
                isClientOnly_ ? nullptr : &svrCredentials_);
        }

        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Unable to acquire ssl credentials: {0}", error);
            return error;
        }

        ReportCertHealthIfNeeded_LockHeld(true);
    }
    else if (this->SecurityProvider == SecurityProvider::Claims)
    {
        ASSERT_IFNOT(isClientOnly_, "Claim based authentication is only supported on client only transport");
        auto error = SecurityCredentials::AcquireAnonymousSslClient(credentials_);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Unable to acquire anonymous ssl client credentials: {0}", error);
            return error;
        }
    }
    else // Kerberos and Negotiate
    {
        auto error = SecurityCredentials::AcquireWin(
            localWindowsIdentity_,
            this->SecurityProvider,
            isClientOnly_,
            authenticationData_ == nullptr ? nullptr : authenticationData_->data(),
            credentials_);

        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Unable to acquire credentials for \"{0}\": {1}", localWindowsIdentity_, error);
            return error;
        }
    }

    auto credentialExpiration = credentials_.front()->Expiration();
    trace.SecurityCredentialExpiration(credentialExpiration.ToString());

    if (!SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking && 
        this->SecurityProvider != SecurityProvider::Claims &&
        (credentialExpiration <= DateTime::Now()))
    {
        WriteError(TraceType, "Credentials expired at {0}", credentialExpiration);
        return ErrorCode(ErrorCodeValue::InvalidCredentials);
    }

    return ErrorCode::Success();
}

bool TransportSecurity::IsClientOnly() const
{
    return isClientOnly_;
}

ULONG TransportSecurity::MaxIncomingFrameSize() const
{
    return maxIncomingFrameSize_;
}

ULONG TransportSecurity::MaxOutgoingFrameSize() const
{
    return maxOutgoingFrameSize_;
}

ULONG TransportSecurity::GetInternalMaxFrameSize(ULONG value)
{
    if (value == 0)
    {
        return value;
    }

    if (value <= MessageHeaders::SizeLimitStatic())
    {
        return value + sizeof(TcpFrameHeader) + MessageHeaders::SizeLimitStatic() + 8*1024 /* body overhead */;
    }

    return value + sizeof(TcpFrameHeader) + MessageHeaders::SizeLimitStatic() + (ULONG)(value * 0.05 /* body overhead */);
}

void TransportSecurity::SetMaxIncomingFrameSize(ULONG value)
{
    maxIncomingFrameSize_ = GetInternalMaxFrameSize(value);
    WriteInfo(TraceType, "SetMaxIncomingFrameSize: input = {0}, internal = {1}", value, maxIncomingFrameSize_);
}

void TransportSecurity::SetMaxOutgoingFrameSize(ULONG value)
{
    maxOutgoingFrameSize_ = GetInternalMaxFrameSize(value);
    WriteInfo(TraceType, "SetMaxOutgoingFrameSize: input = {0}, internal = {1}", value, maxOutgoingFrameSize_);
}

SecuritySettings const & TransportSecurity::Settings() const
{
    return securitySettings_;
}

PSECURITY_DESCRIPTOR TransportSecurity::ConnectionAuthSecurityDescriptor() const
{
    return connectionAuthSecurityDescriptor_->PSecurityDescriptor;
}

SecurityProvider::Enum TransportSecurity::get_SecurityProvider() const
{
    return securitySettings_.SecurityProvider();
}

void TransportSecurity::SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata && metadata)
{
    AcquireWriteLock lockInScope(claimsHandlerLock_);
    WriteInfo(TraceType, "SetClaimsRetrievalMetadata({0})", metadata);
    claimsRetrievalMetadata_ = make_shared<ClaimsRetrievalMetadata>(move(metadata));
}

void TransportSecurity::SetClaimsRetrievalHandler(ClaimsRetrievalHandler const & handler)
{
    AcquireWriteLock lockInScope(claimsHandlerLock_);
    ASSERT_IFNOT(claimsRetrievalHandler_ == nullptr, "Claim retrieval handler cannot be set more than once");
    claimsRetrievalHandler_ = handler;
}

void TransportSecurity::SetClaimsHandler(ClaimsHandler const & handler)
{
    AcquireWriteLock lockInScope(claimsHandlerLock_);
    ASSERT_IFNOT(claimsHandler_ == nullptr, "Claim handler cannot be set more than once");
    claimsHandler_ = handler;
}

void TransportSecurity::CopyNonSecuritySettings(TransportSecurity const & other)
{
    maxIncomingFrameSize_ = other.maxIncomingFrameSize_;
    maxOutgoingFrameSize_ = other.maxOutgoingFrameSize_;

    AcquireWriteLock lock(claimsHandlerLock_);

    // AAD configurations are currently static, so changing them will
    // cause a process restart and configuration reload.
    // Copy claims retrieval metadata (generated from AAD configurations)
    // when other security settings are updated dynamically.
    //
    // Longer term, we will support dynamic AAD configuration rollover
    // as well.
    //
    WriteInfo(TraceType, "CopyNonSecuritySettings: claims metadata={0}", other.claimsRetrievalMetadata_);

    claimsRetrievalMetadata_ = other.claimsRetrievalMetadata_;
    claimsRetrievalHandler_ = other.claimsRetrievalHandler_;
    claimsHandler_ = other.claimsHandler_;
}

bool TransportSecurity::SessionExpirationEnabled() const
{
    return sessionExpirationEnabled_ && 
        (TimeSpan::Zero < securitySettings_.SessionDuration()) &&
        (securitySettings_.SessionDuration() < TimeSpan::MaxValue);
}

bool TransportSecurity::UsingWindowsCredential() const
{
    return SecurityProvider::IsWindowsProvider(this->SecurityProvider);
}

wstring TransportSecurity::GetServerIdentity(wstring const & address) const
{
    if (!UsingWindowsCredential())
    {
        WriteInfo(TraceType, "server identity = '' when not using Windows security");
        return L"";
    }

    if (!securitySettings_.RemoteSpn().empty())
    {
        WriteInfo(TraceType, "server identity = {0}, remote spn", securitySettings_.RemoteSpn());
        return securitySettings_.RemoteSpn();
    }

    wstring spn = AddressToSpn(address);
    WriteInfo(TraceType, "server identity = {0}, generated from address {1}", spn, address);
    return spn;
}

wstring TransportSecurity::GetPeerIdentity(wstring const & address) const
{
    if (!UsingWindowsCredential())
    {
        WriteInfo(TraceType, "peer identity = '' when not using Windows security");
        return L"";
    }

    if (!securitySettings_.RemoteSpn().empty())
    {
        WriteInfo(TraceType, "peer identity = {0}, remote spn", securitySettings_.RemoteSpn());
        return securitySettings_.RemoteSpn();
    }

    if (runningAsMachineAccount_)
    {
        wstring spn = AddressToSpn(address);
        WriteInfo(TraceType, "peer identity = {0}, generated from address {1}", spn, address);
        return spn;
    }

    // If local process is not running as machine account, then the peer cannot be running as machine
    // account either, thus the peer is required to run as the same account as the local process, thus
    // we can use local logon name as the remote identity. Logon name is only used for running CITs, 
    // SPN is always used in production, due to CEC compliance requirement of always using SPN and 
    // ticket caching performance benefit of using SPN.
    WriteInfo(TraceType, "peer identity = {0}, same as local identity", localWindowsIdentity_);
    return localWindowsIdentity_;
}

wstring TransportSecurity::AddressToSpn(std::wstring const & address)
{
    if (TcpTransportUtility::IsLoopbackAddress(address))
    {
        wstring host;
        auto error = TcpTransportUtility::GetLocalFqdn(host);
        ASSERT_IFNOT(error.IsSuccess(), "GetLocalFqdn failed: {0}", error);
        return *HostSpnPrefix + host;
    }

    size_t hostLength = address.rfind(L":");
    ASSERT_IF(hostLength == wstring::npos, "Invalid address: {0}", address);
    return *HostSpnPrefix + address.substr(0, hostLength);
}

void TransportSecurity::RemoveClaimsRetrievalHandler()
{
    AcquireWriteLock lockInScope(claimsHandlerLock_);
    claimsRetrievalHandler_ = nullptr;
}

void TransportSecurity::RemoveClaimsHandler()
{
    AcquireWriteLock lockInScope(claimsHandlerLock_);
    claimsHandler_ = nullptr;
}

ClaimsRetrievalMetadataSPtr const & TransportSecurity::GetClaimsRetrievalMetadata() const
{
    AcquireReadLock lockInScope(claimsHandlerLock_);
    return claimsRetrievalMetadata_;
}

TransportSecurity::ClaimsRetrievalHandler TransportSecurity::ClaimsRetrievalHandlerFunc() const
{
    AcquireReadLock lockInScope(claimsHandlerLock_);
    return claimsRetrievalHandler_;
}

TransportSecurity::ClaimsHandler TransportSecurity::ClaimsHandlerFunc() const
{
    AcquireReadLock lockInScope(claimsHandlerLock_);
    return claimsHandler_;
}

ULONG TransportSecurity::get_SecurityRequirements() const
{
    return securityRequirements_;
}

ErrorCode TransportSecurity::CanUpgradeTo(TransportSecurity const & rhs)
{
    if (!initialized_)
    {
        return ErrorCodeValue::Success;
    }

    if (this->SecurityProvider != rhs.SecurityProvider)
    {
        WriteError(
            TraceType, "SecurityProvider does not match: this = {0}, rhs = {1}", 
            this->SecurityProvider, rhs.SecurityProvider);
        return ErrorCodeValue::InvalidCredentialType;
    }

    if (this->messageHeaderProtectionLevel_ != rhs.messageHeaderProtectionLevel_)
    {
        WriteError(
            TraceType, "Message header ProtectionLevel does not match: this = {0}, rhs = {1}",
            this->messageHeaderProtectionLevel_, rhs.messageHeaderProtectionLevel_);
        return ErrorCodeValue::InvalidProtectionLevel;
    }

    if (this->messageBodyProtectionLevel_ != rhs.messageBodyProtectionLevel_)
    {
        WriteError(
            TraceType, "Message body ProtectionLevel does not match: this = {0}, rhs = {1}",
            this->messageBodyProtectionLevel_, rhs.messageBodyProtectionLevel_);
        return ErrorCodeValue::InvalidProtectionLevel;
    }

    return ErrorCodeValue::Success;
}

SecurityCredentialsSPtr TransportSecurity::Credentails() const
{
    AcquireReadLock grab(credentialLock_);
    return credentials_.front();
}

_Use_decl_annotations_ void TransportSecurity::GetCredentials(
    vector<SecurityCredentialsSPtr> & credentials,
    vector<SecurityCredentialsSPtr> & svrCredentials)
{
    AcquireReadLock grab(credentialLock_);
    credentials = credentials_;
    svrCredentials = svrCredentials_;
}

void TransportSecurity::RefreshX509CredentialsIfNeeded()
{
    if (securitySettings_.SecurityProvider() != SecurityProvider::Ssl) return;

    ErrorCode error;
    vector<SecurityCredentialsSPtr> credentials, svrCredentials;
    if (securitySettings_.IsSelfGeneratedCert())
    {
        error = SecurityCredentials::AcquireSsl(
            *securitySettings_.CertContext(),
            &credentials,
            isClientOnly_ ? nullptr : &svrCredentials);
    }
    else
    {
        error = SecurityCredentials::AcquireSsl(
            securitySettings_.X509StoreLocation(),
            securitySettings_.X509StoreName(),
            securitySettings_.X509FindValue(),
            &credentials,
            isClientOnly_ ? nullptr : &svrCredentials);
    }

    if (!error.IsSuccess())
    {
        // todo, leikong, consider reporting health on this
        WriteError(TraceType, "RefreshX509CredentialsIfNeeded: AcquireSsl failed: {0}", error);
        return;
    }

    AcquireWriteLock grab(credentialLock_);

    bool noUpdates = SecurityCredentials::SortedVectorsEqual(credentials, credentials_);
    if (noUpdates)
    {
        ReportCertHealthIfNeeded_LockHeld(false);
        return;
    }

    ++credentialRefreshCount_;
    WriteInfo(
        TraceType,
        "certificate credential refresh with self generatd cert: refresh count = {0}",
        credentialRefreshCount_);

    credentials_ = move(credentials);
    if (!isClientOnly_)
    {
        svrCredentials_ = move(svrCredentials);
    }

    ReportCertHealthIfNeeded_LockHeld(true);
}

void TransportSecurity::ReportCertHealthIfNeeded_LockHeld(bool newCredentialLoaded)
{
    if (securitySettings_.X509CertType().empty())
    {
        WriteInfo(TraceType, "skip health report as X509CertType is empty");
        return;
    }

    auto certHealthReportingInterval = SecurityConfig::GetConfig().CertificateHealthReportingInterval;
    if (certHealthReportingInterval <= TimeSpan::Zero)
    {
        WriteWarning(TraceType, "certificate health monitoring is disabled");
        return;
    }

    auto expiration = credentials_.front()->Expiration();
    auto reportCode = SystemHealthReportCode::FabricNode_CertificateOk;
    auto logLevel = LogLevel::Info;
    auto safetyMargin = SecurityConfig::GetConfig().CertificateExpirySafetyMargin;
    auto now = DateTime::Now();
    if (expiration <= (now + safetyMargin))
    {
        reportCode = SystemHealthReportCode::FabricNode_CertificateCloseToExpiration;
        logLevel = LogLevel::Warning;
    }

    if (!newCredentialLoaded && (logLevel == lastCertHealthReportLevel_))
    {
        auto reportDue = lastCertHealthReportTime_ + certHealthReportingInterval;
        auto stopwatchNow = Stopwatch::Now();
        if (stopwatchNow < reportDue)
        {
            return; // wait until report due time if logLevel is the same
        }
    }

    auto description = wformatString(
        " thumbprint = {0}, expiration = {1}, remaining lifetime is {2}, please refresh ahead of time to avoid catastrophic failure. Warning threshold Security/CertificateExpirySafetyMargin is configured at {3}, if needed, you can adjust it to fit your refresh process.",
        credentials_.front()->X509CertThumbprint().PrimaryToString(), expiration, expiration - now, safetyMargin);

    IDatagramTransport::RerportHealth(reportCode, securitySettings_.X509CertType(), description);
    lastCertHealthReportTime_ = Stopwatch::Now();
    lastCertHealthReportLevel_ = logLevel;
}

#ifdef PLATFORM_UNIX

wstring const & TransportSecurity::LocalWindowsIdentity()
{
    ASSERT_IFNOT(this->SecurityProvider == SecurityProvider::None, "Not implemented");
    return L"";
}

bool TransportSecurity::RunningAsMachineAccount()
{
    // LINUXTODO
    return true;
}

wstring TransportSecurity::ToString() const
{
    wstring result;
    StringWriter w(result);
    w.Write(securitySettings_);

    //LINUXTODO connectionAuthSecurityDescriptor_
    return result;
}

#else

bool TransportSecurity::RunningAsMachineAccount()
{
    auto error = InitializeLocalWindowsIdentityIfNeeded();
    ASSERT_IFNOT(error.IsSuccess(), "InitializeLocalWindowsIdentity failed: {0}", error);
    return runningAsMachineAccount_;
}

wstring TransportSecurity::ToString() const
{
    wstring result;
    StringWriter w(result);
    w.Write(securitySettings_);

    if (UsingWindowsCredential())
    {
        w.Write(" ConnectionAuthSD={0}", connectionAuthSecurityDescriptor_);
        if (securitySettings_.IsClientRoleInEffect())
        {
            w.Write(" AdminRoleSD={0}", adminRoleSecurityDescriptor_);
        }
    }

    return result;
}

_Use_decl_annotations_
ErrorCode TransportSecurity::CreateSecurityDescriptorFromAllowedList(
    HANDLE ownerToken,
    bool allowOwner,
    SecuritySettings::IdentitySet const & allowedList,
    DWORD allowedAccess,
    SecurityDescriptorSPtr & securityDescriptor)
{
    securityDescriptor = nullptr;

    vector<SidSPtr> allowedSidList;
    allowedSidList.reserve(allowedList.size());

    for (auto iter = allowedList.begin(); iter != allowedList.end(); ++ iter)
    {
        if (!iter->empty())
        {
            Common::SidSPtr remoteSid;
            auto error = BufferedSid::CreateSPtr(*iter, remoteSid);
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                WriteWarning(TraceType, "CreateSecurityDescriptor: failed to convert {0} to SID: {1}, will retry as machine account by appending $", *iter, error);
                wstring machineAccountName = *iter + L"$";
                error = BufferedSid::CreateSPtr(machineAccountName, remoteSid);
            }

            if (!error.IsSuccess())
            {
                WriteError(TraceType, "CreateSecurityDescriptor: failed to convert {0} to SID: {1}", *iter, error);
                return error;
            }

            wstring sidString;
            error = remoteSid->ToString(sidString);
            ASSERT_IFNOT(error.IsSuccess(), "CreateSecurityDescriptor: failed to convert sid to string: {0}", error);
            WriteInfo(TraceType, "CreateSecurityDescriptor: {0} converted to sid: {1}", *iter, sidString);
            allowedSidList.push_back(remoteSid);
        }
    }

    auto error = BufferedSecurityDescriptor::CreateAllowed(ownerToken, allowOwner, allowedSidList, allowedAccess, securityDescriptor);
    if (!error.IsSuccess())
    {
        return error;
    }

    return error;
}

PSECURITY_DESCRIPTOR TransportSecurity::AdminRoleSecurityDescriptor() const
{
    return adminRoleSecurityDescriptor_->PSecurityDescriptor;
}

ErrorCode TransportSecurity::InitializeLocalWindowsIdentityIfNeeded()
{
    ErrorCode error;
    if (localTokenHandle_ != INVALID_HANDLE_VALUE)
    {
        return error; // already initialized
    }

    BOOL retval = ::OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &localTokenHandle_);
    if (retval == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        if (!error.IsWin32Error(ERROR_NO_TOKEN))
        {
            WriteError(TraceType, "InitializeLocalWindowsIdentity: OpenThreadToken failed with unexpected error: {0}", error);
            return error;
        }

        // Current thread is not an impersonation thread, will get process token
        retval = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &localTokenHandle_);
        if (retval == FALSE)
        {
            error = ErrorCode::FromWin32Error();
            WriteError(TraceType, "InitializeLocalWindowsIdentity: OpenProcessToken failed: {0}", error);
            return error;
        }

        WriteInfo(TraceType, "InitializeLocalWindowsIdentity: process token retrieved");
    }
    else
    {
        WriteInfo(TraceType, "InitializeLocalWindowsIdentity: thread token retrieved");
    }

    DWORD length = 0;
    GetTokenInformation(localTokenHandle_, TokenUser, nullptr, 0, &length);
    localUserToken_.resize(length, 0);

    retval = GetTokenInformation(localTokenHandle_, TokenUser, &localUserToken_.front(), length, &length);
    if (retval == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        localUserToken_.clear();
        WriteError(TraceType, "InitializeLocalWindowsIdentity: GetTokenInformation failed: {0}", error);
        return error;
    }

    TOKEN_USER & tokenUser = reinterpret_cast<TOKEN_USER&>(localUserToken_.front());
    wstring localSidString;
    error = Sid::ToString(tokenUser.User.Sid, localSidString);
    ASSERT_IFNOT(error.IsSuccess(), "Failed to convert local sid to string: {0}", error);
    WriteInfo(TraceType, "local sid = {0}", localSidString);

    if (::IsWellKnownSid(tokenUser.User.Sid, WinLocalSystemSid) ||
        ::IsWellKnownSid(tokenUser.User.Sid, WinLocalServiceSid) ||
        ::IsWellKnownSid(tokenUser.User.Sid, WinNetworkServiceSid))
    {
        runningAsMachineAccount_ = true;
        error = GetMachineAccount(localWindowsIdentity_);
        if (error.IsSuccess())
        {
            WriteInfo(TraceType, "local Windows identity is machine account {0}", localWindowsIdentity_);
            return error;
        }

        Invariant(error.IsError(ErrorCodeValue::NotFound));
        WriteInfo(TraceType, "Unable to get machine account: {0}", error);
    }

    DWORD nameLength = 0;
    DWORD domainLength = 0;
    SID_NAME_USE sidNameUse;
    LookupAccountSid(nullptr, tokenUser.User.Sid, nullptr, &nameLength, nullptr, &domainLength, &sidNameUse);

    std::vector<wchar_t> name(nameLength);
    std::vector<wchar_t> domain(domainLength);
    retval = LookupAccountSid(nullptr, tokenUser.User.Sid, &name.front(), &nameLength, &domain.front(), &domainLength, &sidNameUse);
    if (retval == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        WriteError(TraceType, "InitializeLocalWindowsIdentity: LookupAccountSid failed: {0}", error);
        return error;
    }

    std::wostringstream stringStream;
    stringStream << static_cast<wchar_t const *>(&domain.front()) << L"\\" << static_cast<wchar_t const *>(&name.front());
    localWindowsIdentity_ = stringStream.str();
    WriteInfo(TraceType, "local Windows identity is {0}", localWindowsIdentity_);
    return ErrorCodeValue::Success;
}

void TransportSecurity::Test_SetHeaderProtectionLevel(ProtectionLevel::Enum value)
{
    this->messageHeaderProtectionLevel_ = value;
}

void TransportSecurity::Test_SetBodyProtectionLevel(ProtectionLevel::Enum value)
{
    this->messageBodyProtectionLevel_ = value;
}

wstring const & TransportSecurity::LocalWindowsIdentity()
{
    auto error = InitializeLocalWindowsIdentityIfNeeded();
    ASSERT_IFNOT(error.IsSuccess(), "InitializeLocalWindowsIdentity failed: {0}", error);
    return localWindowsIdentity_;
}

_Use_decl_annotations_
ErrorCode TransportSecurity::GetDomainNameDns(std::wstring & domain)
{
    domain.clear();

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC domainInfo;
    DWORD retval = ::DsRoleGetPrimaryDomainInformation(nullptr, DsRolePrimaryDomainInfoBasic, (PBYTE*)&domainInfo);
    if (retval !=  ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(retval);
    }

    KFinally([domainInfo] { ::DsRoleFreeMemory(domainInfo); });

    if (domainInfo->DomainNameDns == nullptr)
    {
        return ErrorCodeValue::NotFound;
    }

    domain = wstring(domainInfo->DomainNameDns);
    Invariant(!domain.empty());
    return ErrorCodeValue::Success;
}

_Use_decl_annotations_
ErrorCode TransportSecurity::GetMachineAccount(wstring & machineAccount)
{
    machineAccount.clear();

    wstring domain;
    ErrorCode error = GetDomainNameDns(domain);
    if (!error.IsSuccess())
    {
        return error;
    }

    DWORD nameLength = 0;
    if (!GetComputerNameEx(ComputerNameNetBIOS, nullptr, &nameLength))
    {
        DWORD lastError = ::GetLastError();
        if (lastError != ERROR_MORE_DATA)
        {
            return ErrorCode::FromWin32Error(lastError);
        }
    }

    vector<WCHAR> nameBuffer(nameLength, 0);
    if (!GetComputerNameEx(ComputerNameNetBIOS, nameBuffer.data(), &nameLength))
    {
        return ErrorCode::FromWin32Error();
    }

    machineAccount.reserve(domain.length() + nameLength + 3);
    machineAccount.append(domain);
    machineAccount.push_back(L'\\');
    machineAccount.append((PCWCHAR)nameBuffer.data());
    machineAccount.push_back(L'$');

    return ErrorCodeValue::Success;
}

ErrorCode TransportSecurity::UpdateWindowsFabricSpn(DS_SPN_WRITE_OP operation)
{
    wstring domain;
    ErrorCode error = GetDomainNameDns(domain);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(TraceType, "Domain information not found, skip spn update operation");
            return ErrorCodeValue::Success;
        }
    }

    HANDLE dsHandle;
    DWORD retval = DsBind(nullptr, domain.c_str(), &dsHandle);
    if (retval != ERROR_SUCCESS)
    {
        WriteError(TraceType, "Failed to bind to a domain controller of {0}: {1}", domain, retval);
        return ErrorCode::FromWin32Error(retval);
    }

    KFinally([&dsHandle] { DsUnBind(&dsHandle); });

    ULONG length = 0;
    GetComputerObjectName(NameFullyQualifiedDN, nullptr, &length);
    vector<WCHAR> machineNameBuffer(length);
    LPWSTR machineNameDn = machineNameBuffer.data();
    retval = GetComputerObjectName(NameFullyQualifiedDN, machineNameDn, &length);
    if (retval == 0)
    {
        error = ErrorCode::FromWin32Error();
        WriteError(TraceType, "GetComputerObjectName failed: {0}", error);
        return error;
    }

    wstring localFqdn;
    error = TcpTransportUtility::GetLocalFqdn(localFqdn);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring spnToRegister = WINDOWS_FABRIC_SPN_PREFIX + localFqdn;
    LPCWSTR spn = spnToRegister.c_str();
    retval = DsWriteAccountSpn(dsHandle, operation, machineNameDn, 1, &spn);
    if (retval != ERROR_SUCCESS)
    {
        WriteWarning(TraceType, "DsWriteAccountSpn failed on {0}: {1}", spn, retval);
        return ErrorCode::FromWin32Error(retval);
    }

    WriteInfo(TraceType, "spn({0}) udpated on {1}", spn, machineNameDn);
    return error;
}

ErrorCode TransportSecurity::RegisterWindowsFabricSpn()
{
    SidUPtr currentUserSid;
    ErrorCode error = BufferedSid::GetCurrentUserSid(currentUserSid);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!currentUserSid->IsLocalSystem() && !currentUserSid->IsNetworkService())
    {
        WriteInfo(TraceType, "not running as LocalSystem/NetworkService, skipping spn registration");
        return error;
    }

    error = UpdateWindowsFabricSpn(DS_SPN_ADD_SPN_OP);
    if (error.IsSuccess())
    {
        WriteInfo(TraceType, "spn registration completed");
    }
    else
    {
        WriteError(TraceType, "spn registration failed: {0}", error);
    }

    return error;
}

ErrorCode TransportSecurity::UnregisterWindowsFabricSpn()
{
    SidUPtr currentUserSid;
    ErrorCode error = BufferedSid::GetCurrentUserSid(currentUserSid);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!currentUserSid->IsLocalSystem() && !currentUserSid->IsNetworkService())
    {
        WriteInfo(TraceType, "not running as LocalSystem/NetworkService, skipping spn unregistration");
        return error;
    }

    error = UpdateWindowsFabricSpn(DS_SPN_DELETE_SPN_OP);
    if (error.IsSuccess())
    {
        WriteInfo(TraceType, "spn unregistration completed");
    }
    else
    {
        WriteError(TraceType, "spn unregistration failed: {0}", error);
    }

    return error;
}

#endif

