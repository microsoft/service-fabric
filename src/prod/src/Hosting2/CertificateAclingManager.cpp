// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType("CertificateAclingManager");
#define AnonymousCertCommonName  L"AzureServiceFabric-AnonymousClient"
#define AnonymousCertSubjectName  L"CN=AzureServiceFabric-AnonymousClient"
#define AnonymousCertStoreName  L"My"

#define RegisterConfigUpdateHandler(title, section, storeNameKey, findTypeKey, findValueKey) \
certificateConfigs_.push_back(CertificateConfig((wstring)title, (wstring)section, (wstring)storeNameKey, (wstring)findTypeKey, (wstring)findValueKey))

CertificateAclingManager::CertificateAclingManager(Common::ComponentRoot const & root) :
    RootedObject(root),
    traceId_(),
    lock_(),
    certificateConfigs_(),
    config_(),
    timer_()
{
    StringWriter writer(traceId_);
    writer.Write("<{0}>", reinterpret_cast<void*>(this));
}

CertificateAclingManager::~CertificateAclingManager()
{
    config_.UnregisterForUpdate(this);
}

ErrorCode CertificateAclingManager::OnOpen()
{
    WriteInfo(
        TraceType,
        traceId_,
        "Open");

    weak_ptr<const ComponentRoot> rootWPtr = this->Root.shared_from_this();

    timer_ = Common::Timer::Create("CertificateAclingManager", [this](TimerSPtr const & t) { this->TimerCallback(t); }, false);
    timer_->SetCancelWait();

    RegisterConfigUpdateHandler(L"ClusterPrimaryCertificate", L"FabricNode", L"ClusterX509StoreName", L"ClusterX509FindType", L"ClusterX509FindValue");
    RegisterConfigUpdateHandler(L"ClusterSecondaryCertificate", L"FabricNode", L"ClusterX509StoreName", L"ClusterX509FindType", L"ClusterX509FindValueSecondary");

    RegisterConfigUpdateHandler(L"ServerPrimaryCertificate", L"FabricNode", L"ServerAuthX509StoreName", L"ServerAuthX509FindType", L"ServerAuthX509FindValue");
    RegisterConfigUpdateHandler(L"ServerSecondaryCertificate", L"FabricNode", L"ServerAuthX509StoreName", L"ServerAuthX509FindType", L"ServerAuthX509FindValueSecondary");

    RegisterConfigUpdateHandler(L"ClientPrimaryCertificate", L"FabricNode", L"ClientAuthX509StoreName", L"ClientAuthX509FindType", L"ClientAuthX509FindValue");
    RegisterConfigUpdateHandler(L"ClientSecondaryCertificate", L"FabricNode", L"ClientAuthX509StoreName", L"ClientAuthX509FindType", L"ClientAuthX509FindValueSecondary");

    RegisterConfigUpdateHandler(L"UpgradeServicePrimaryCertificate", L"UpgradeService", L"X509StoreName", L"X509FindType", L"X509FindValue");
    RegisterConfigUpdateHandler(L"UpgradeServiceSecondaryCertificate", L"UpgradeService", L"X509StoreName", L"X509FindType", L"X509SecondaryFindValue");

    RegisterConfigUpdateHandler(L"FileStoreServicePrimaryThumbprintCertificate", L"FileStoreService", L"PrimaryAccountNTLMX509StoreName", L"#FindByThumbprint", L"PrimaryAccountNTLMX509Thumbprint");
    RegisterConfigUpdateHandler(L"FileStoreServiceSecondaryThumbprintCertificate", L"FileStoreService", L"SecondaryAccountNTLMX509StoreName", L"#FindByThumbprint", L"SecondaryAccountNTLMX509Thumbprint");
    
    RegisterConfigUpdateHandler(L"FileStoreServicePrimaryCommonNameCertificate", L"FileStoreService", L"CommonName1Ntlmx509StoreName", L"#FindBySubjectName", L"CommonName1Ntlmx509CommonName");
    RegisterConfigUpdateHandler(L"FileStoreServiceSecondaryCommonNameCertificate", L"FileStoreService", L"CommonName2Ntlmx509StoreName", L"#FindBySubjectName", L"CommonName2Ntlmx509CommonName");

#ifndef PLATFORM_UNIX
    RegisterConfigUpdateHandler(L"ReverseProxyPrimaryCertificate", L"ApplicationGateway/Http", L"GatewayX509CertificateStoreName", L"GatewayX509CertificateFindType", L"GatewayX509CertificateFindValue");
    RegisterConfigUpdateHandler(L"ReverseProxySecondaryCertificate", L"ApplicationGateway/Http", L"GatewayX509CertificateStoreName", L"GatewayX509CertificateFindType", L"GatewayX509CertificateFindValueSecondary");

    RegisterConfigUpdateHandler(L"BackupRestoreServiceEncryptionCertificate", L"BackupRestoreService", L"SecretEncryptionCertX509StoreName", L"#FindByThumbprint", L"SecretEncryptionCertThumbprint");
#endif

    vector<wstring> sectionsToTrack;
    for (auto certIter = certificateConfigs_.begin(); certIter != certificateConfigs_.end(); certIter++)
    {
        sectionsToTrack.push_back(certIter->GetSectionName());
    }

    sort(sectionsToTrack.begin(), sectionsToTrack.end());
    sectionsToTrack.erase(unique(sectionsToTrack.begin(), sectionsToTrack.end()), sectionsToTrack.end());

    for (auto sectionIter = sectionsToTrack.begin(); sectionIter != sectionsToTrack.end(); sectionIter++)
    {
        config_.RegisterForUpdate(*sectionIter, this);
    }

    ScheduleCertificateAcling();

    WriteInfo(
        TraceType,
        traceId_,
        "Open complete");

    return ErrorCode::Success();
}

void CertificateAclingManager::ScheduleCertificateAcling()
{
    WriteInfo(
        TraceType,
        traceId_,
        "ScheduleCertificateAcling");

    AcquireReadLock lock(lock_);
    if (this->timer_ == nullptr)
    {
        return;
    }

    this->timer_->Change(Common::TimeSpan::Zero, HostingConfig::GetConfig().ClusterCertificateAclingInterval);
}

ErrorCode CertificateAclingManager::OnClose()
{
    WriteNoise(
        TraceType,
        traceId_,
        "Close");

    StopTimer();
    config_.UnregisterForUpdate(this);

    WriteNoise(
        TraceType,
        traceId_,
        "Close complete");

    return ErrorCode::Success();
}

void CertificateAclingManager::OnAbort()
{
    WriteNoise(
        TraceType,
        traceId_,
        "Abort");

    StopTimer();
    config_.UnregisterForUpdate(this);

    WriteNoise(
        TraceType,
        traceId_,
        "Abort complete");
}

void CertificateAclingManager::StopTimer()
{
    WriteNoise(
        TraceType,
        traceId_,
        "StopTimer");

    AcquireWriteLock lock(lock_);

    if (timer_ != nullptr)
    {
        timer_->Cancel();
        timer_ = nullptr;
    }

    WriteNoise(
        TraceType,
        traceId_,
        "StopTimer complete");
}

void CertificateAclingManager::TimerCallback(TimerSPtr const & timer)
{
    _Unreferenced_parameter_(timer);
    if (!IsOpened())
    {
        return;
    }

    auto error = AclCertificates();
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            traceId_,
            "Acl certificates failed. ErrorCode {0}", error);
    }
}

Common::ErrorCode CertificateAclingManager::AclCertificates()
{
    WriteInfo(
        TraceType,
        traceId_,
        "Start@AclCertificates");

    SidSPtr fabAdminSid;
    auto error = BufferedSid::CreateSPtr(FabricConstants::WindowsFabricAdministratorsGroupName, fabAdminSid);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            traceId_,
            "Can't find SID for {0} , ErrorCode {1}", FabricConstants::WindowsFabricAdministratorsGroupName, error);

        return error;
    }

    wstring sidString;
    error = fabAdminSid->ToString(sidString);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't convert SID to string , ErrorCode {0}", error);
    }

    vector<SidSPtr> targetSids;
    targetSids.push_back(fabAdminSid);

    error = AclAnonymousCertificate(targetSids);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Error at AclAnonymousCertificate, ErrorCode {0}", error);
    }

    error = AclConfiguredCertificates(targetSids);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Error at AclConfiguredCertificates, ErrorCode {0}", error);
    }

    return ErrorCode::Success();
}

Common::ErrorCode CertificateAclingManager::AclConfiguredCertificates(vector<SidSPtr> const & targetSids)
{
    bool errorsFound = false;
    vector<wstring> acledCertificates;

    for (auto certIter = certificateConfigs_.begin(); certIter != certificateConfigs_.end(); certIter++)
    {
        auto sectionName = certIter->GetSectionName();
        wstring storeName, findType, findValue;
        auto certificateName = sectionName + L"/" + certIter->GetFindValueKeyName();
        if (!TryReadConfigKey(sectionName, certIter->GetStoreNameKeyName(), storeName))
        {
            continue;
        }

        if (!TryReadConfigKey(sectionName, certIter->GetFindTypeKeyName(), findType))
        {
            continue;
        }

        if (!TryReadConfigKey(sectionName, certIter->GetFindValueKeyName(), findValue))
        {
            continue;
        }

        if (StringUtility::Compare(findValue, L"") == 0)
        {
            WriteInfo(
                TraceType,
                traceId_,
                "Find value is empty for {0} -> Skip", certIter->GetTitle());
            continue;
        }

        wstring certificateKey = storeName + L";" + findType + L";" + findValue;
        StringUtility::ToUpper(certificateKey);
        if (std::find(acledCertificates.begin(), acledCertificates.end(), certificateKey) != acledCertificates.end())
        {
            WriteInfo(
                TraceType,
                traceId_,
                "Certificate is acled before '{0}' storeName: '{1}', findType: '{2}', findValue: '{3}' -> Skip",
                certIter->GetTitle(),
                storeName,
                findType,
                findValue);
            continue;
        }

        if (TrySetCertificateAcls(
            certificateName,
            findValue,
            findType,
            storeName,
            targetSids,
            errorsFound))
        {
            acledCertificates.push_back(certificateKey);
            WriteInfo(
                TraceType,
                traceId_,
                "Certificate acled successfully '{0}' storeName: '{1}', findType: '{2}', findValue: '{3}'",
                certIter->GetTitle(),
                storeName,
                findType,
                findValue);
        }
    }

    return errorsFound ? ErrorCode::FromHResult(E_FAIL) : ErrorCode::Success();
}

bool CertificateAclingManager::TryReadConfigKey(wstring const & section, wstring const & key, wstring & value)
{
    if (key[0] == L'#')
    {
        value = key;
        StringUtility::TrimLeading(value, (wstring)L"#");
        return true;
    }

    if (!config_.ReadUnencryptedConfig<wstring>(section, key, value, L""))
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't read key [{0}/{1}]", section, key);
        return false;
    }

    return true;
}

bool CertificateAclingManager::TrySetCertificateAcls(
    wstring const & certificateName,
    wstring const & findValue,
    wstring const & findType,
    wstring const & storeName,
    vector<SidSPtr> const & targetSids,
    bool& errorsFound)
{
    auto error = SetCertificateAcls(findValue, findType, storeName, targetSids);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't ACL {0}, ErrorCode {1}", certificateName, error);
        errorsFound = true;
        return false;
    }

    return true;
}

Common::ErrorCode CertificateAclingManager::AclAnonymousCertificate(vector<SidSPtr> const & targetSids)
{
    wstring privateKeyFileName;
    auto error = SecurityUtility::GetCertificatePrivateKeyFile(
        AnonymousCertCommonName,
        AnonymousCertStoreName,
        X509FindType::FindBySubjectName,
        privateKeyFileName);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't find anonymous certificate. ErrorCode: {0}", error);

        if (!error.IsError(ErrorCodeValue::CertificateNotFound))
        {
            return error;
        }
    }

    if (!error.IsSuccess() || StringUtility::Compare(privateKeyFileName, L"") == 0)
    {
        WriteInfo(
            TraceType,
            traceId_,
            "Can't find anonymous certificate, try to create one");

        CertContextUPtr cert;
        error = CryptoUtility::CreateSelfSignedCertificate(AnonymousCertSubjectName, cert);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                traceId_,
                "Create anonynous certificate failed. ErrorCode: {0}", error);

            return error;
        }

        error = CryptoUtility::InstallCertificate(cert, X509Default::StoreLocation(), AnonymousCertStoreName);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                traceId_,
                "Install anonynous certificate failed. ErrorCode: {0}", error);

            return error;
        }

        // Try to get certificate thumbprint for tracing it only.
        wstring thumbprintStr;
        Thumbprint thumbprint;
        error = thumbprint.Initialize(cert.get());
        if (!error.IsSuccess())
        {
            // Will not return on this error as thumprint is not required after that.
            WriteWarning(
                TraceType,
                traceId_,
                "Can't read certificate thumbprint. ErrorCode: {0}", error);
        }
        else
        {
            thumbprintStr = thumbprint.PrimaryToString();
        }

        WriteInfo(
            TraceType,
            traceId_,
            "Anonymous certificate created and installed. Certificate thumbprint: {0}", thumbprintStr);
    }

    error = SetCertificateAcls(AnonymousCertCommonName, X509FindType::FindBySubjectName, AnonymousCertStoreName, targetSids);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't set certificate acls for anonymous certificate. ErrorCode: {0}", error);

        return error;
    }

    return ErrorCode::Success();
}

Common::ErrorCode CertificateAclingManager::SetCertificateAcls(
    wstring const & findValue,
    wstring const & findTypeString,
    wstring const & storeName,
    vector<SidSPtr> const & targetSids)
{
    X509FindType::Enum findType;
    auto error = X509FindType::Parse(findTypeString, findType);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "Can't parse FindType {0}, ErrorCode: {1}", findTypeString, error);

        return error;
    }

    return SetCertificateAcls(findValue, findType, storeName, targetSids);
}

Common::ErrorCode CertificateAclingManager::SetCertificateAcls(
    wstring const & findValue,
    X509FindType::Enum findType,
    wstring const & storeName,
    vector<SidSPtr> const & targetSids)
{
    auto error = ErrorCode::Success();

    if (StringUtility::Compare(findValue, L"") == 0)
    {
        return error;
    }

    WriteInfo(
        TraceType,
        traceId_,
        "SetCertificateAcls {0}", findValue);

#ifdef PLATFORM_UNIX
    WriteInfo(
        TraceType,
        traceId_,
        "SecurityUtility::InstallCoreFxCertificate {0}", findValue);

    error = SecurityUtility::InstallCoreFxCertificate(Constants::AppRunAsAccountGroup, findValue, storeName, findType);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "SecurityUtility::InstallCoreFxCertificate failed. ErrorCode: {0}", error);
    }
#endif

    WriteInfo(
        TraceType,
        traceId_,
        "SecurityUtility::SetCertificateAcls {0}", findValue);


#ifdef PLATFORM_UNIX
    vector<string> userNames;
    userNames.push_back(StringUtility::Utf16ToUtf8(Constants::AppRunAsAccountGroup));
    error = SecurityUtility::SetCertificateAcls(
        findValue,
        storeName,
        findType,
        userNames,
        targetSids,
        FILE_ALL_ACCESS,
        Common::TimeSpan::FromMinutes(1));
#else
    error = SecurityUtility::SetCertificateAcls(
        findValue,
        storeName,
        findType,
        targetSids,
        FILE_ALL_ACCESS,
        Common::TimeSpan::FromMinutes(1));

#endif

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            traceId_,
            "SetCertificateAcls failed. ErrorCode: {0}", error);
    }

    return error;
}

bool CertificateAclingManager::OnUpdate(std::wstring const & section, std::wstring const & key)
{
    if (!IsOpened())
    {
        return true;
    }

    for (auto certIter = certificateConfigs_.begin(); certIter != certificateConfigs_.end(); certIter++)
    {
        if (StringUtility::AreEqualCaseInsensitive(section, certIter->GetSectionName())
            && (StringUtility::AreEqualCaseInsensitive(key, certIter->GetFindTypeKeyName())
                || StringUtility::AreEqualCaseInsensitive(key, certIter->GetFindValueKeyName())
                || StringUtility::AreEqualCaseInsensitive(key, certIter->GetStoreNameKeyName())))
        {
            WriteInfo(
                TraceType,
                traceId_,
                "Certificate acl triggering config updated. {0}/{1}", section, key);

            ScheduleCertificateAcling();
        }
    }

    return true;
}

bool CertificateAclingManager::CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted)
{
    _Unreferenced_parameter_(section);
    _Unreferenced_parameter_(key);
    _Unreferenced_parameter_(value);
    _Unreferenced_parameter_(isEncrypted);

    return true;
}

const std::wstring &CertificateAclingManager::GetTraceId() const
{
    return traceId_;
}

CertificateAclingManager::CertificateConfig::CertificateConfig(
    wstring const & title,
    wstring const & sectionName,
    wstring const & storeNameKeyName,
    wstring const & findTypeKeyName,
    wstring const & findValueKeyName)
    :title_(title),
    sectionName_(sectionName),
    storeNameKeyName_(storeNameKeyName),
    findTypeKeyName_(findTypeKeyName),
    findValueKeyName_(findValueKeyName)
{

}

std::wstring CertificateAclingManager::CertificateConfig::GetTitle()
{
    return this->title_;
}

std::wstring CertificateAclingManager::CertificateConfig::GetSectionName()
{
    return this->sectionName_;
}

std::wstring CertificateAclingManager::CertificateConfig::GetStoreNameKeyName()
{
    return this->storeNameKeyName_;
}

std::wstring CertificateAclingManager::CertificateConfig::GetFindTypeKeyName()
{
    return this->findTypeKeyName_;
}

std::wstring CertificateAclingManager::CertificateConfig::GetFindValueKeyName()
{
    return this->findValueKeyName_;
}

