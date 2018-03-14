// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static const StringLiteral TraceType("SecurityUtil");

_Use_decl_annotations_ ErrorCode SecurityUtil::GetX509SvrCredThumbprint(
    X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    Thumbprint const * loadedCredThumbprint,
    Thumbprint & thumbprint)
{
    vector<SecurityCredentialsSPtr> svrCred;
    auto error = SecurityCredentials::AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue,
        nullptr,
        &svrCred);

    if (!error.IsSuccess())
    {
        thumbprint = Thumbprint();
        return error;
    }

    thumbprint = svrCred.front()->X509CertThumbprint();
    if (loadedCredThumbprint && (*loadedCredThumbprint == thumbprint))
    {
        return ErrorCodeValue::CredentialAlreadyLoaded;
    }

    return error;
}

_Use_decl_annotations_  ErrorCode SecurityUtil::GetX509SvrCredExpiration(
    X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    DateTime & expiration)
{
    vector<SecurityCredentialsSPtr> svrCred;
    auto error = SecurityCredentials::AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue,
        nullptr,
        &svrCred);

    if (!error.IsSuccess())
    {
        expiration = DateTime::Zero;
        return error;
    }

    expiration = svrCred.front()->Expiration();
    return error;
}

_Use_decl_annotations_  ErrorCode SecurityUtil::GetX509CltCredAttr(
    X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    Thumbprint & thumbprint,
    DateTime & expiration)
{
    vector<SecurityCredentialsSPtr> cred;
    auto error = SecurityCredentials::AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue,
        &cred,
        nullptr);

    if (!error.IsSuccess())
    {
        thumbprint = Thumbprint();
        expiration = DateTime::Zero;
        return error;
    }

    thumbprint = cred.front()->X509CertThumbprint();
    expiration = cred.front()->Expiration();
    return error;
}

_Use_decl_annotations_  ErrorCode SecurityUtil::GetX509SvrCredAttr(
    Common::X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    Thumbprint & thumbprint,
    DateTime & expiration)
{
    vector<SecurityCredentialsSPtr> svrCred;
    auto error = SecurityCredentials::AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue,
        nullptr,
        &svrCred);

    if (!error.IsSuccess())
    {
        thumbprint = Thumbprint();
        expiration = DateTime::Zero;
        return error;
    }

    thumbprint = svrCred.front()->X509CertThumbprint();
    expiration = svrCred.front()->Expiration();
    return error;
}

_Use_decl_annotations_
ErrorCode SecurityUtil::AllowDefaultClientsIfNeeded(
    SecuritySettings & serverSecuritySettings,
    FabricNodeConfig const & nodeConfig,
    wstring const& traceId)
{
    ErrorCode error;

#ifdef PLATFORM_UNIX
    // Stop having implicit behavior wherever possible, as they may cause potential complexity in future.
    return error;

#else

    if (serverSecuritySettings.SecurityProvider() == SecurityProvider::Ssl)
    {
        if (!PaasConfig::GetConfig().ClusterId.empty() || !SecurityConfig::GetConfig().AllowDefaultClient)
        {
            Trace.WriteInfo(TraceType, traceId, "SFRP or SFOP cluster, skip adding default client certificate to allow list");
            return error;
        }

        if (!nodeConfig.ClientAuthX509FindValueSecondary.empty() || !nodeConfig.UserRoleClientX509FindValueSecondary.empty())
        {
            Trace.WriteError(TraceType, traceId, "Non-SFRP, Non-SFOP cluster, X509FindValueSecondary is not supported");
            return ErrorCodeValue::InvalidConfiguration;
        }

        CertContextUPtr defaultClientCert;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(),
            nodeConfig.ClientAuthX509StoreName,
            nodeConfig.ClientAuthX509FindType,
            nodeConfig.ClientAuthX509FindValue,
            defaultClientCert);

        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceType, traceId, "Failed to retrieve default client certificate: {0}", error);
            return error;
        }

        error = serverSecuritySettings.AddRemoteX509Name(defaultClientCert.get());
        Trace.WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
            TraceType,
            traceId,
            "implicitly add default client certificate to allow list: {0}",
            error);

        if (!error.IsSuccess()) return error;

        if (nodeConfig.UserRoleClientX509FindValue.empty()) return error;

        CertContextUPtr userRoleClientCert;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(),
            nodeConfig.UserRoleClientX509StoreName,
            nodeConfig.UserRoleClientX509FindType,
            nodeConfig.UserRoleClientX509FindValue,
            userRoleClientCert);

        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceType, traceId, "Failed to retrieve default user role client certificate");
            return error;
        }

        error = serverSecuritySettings.AddRemoteX509Name(userRoleClientCert.get());
        Trace.WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
            TraceType,
            "implicitly add default user role client certificate to allow list: {0}",
            error);
    }

    if (SecurityProvider::IsWindowsProvider(serverSecuritySettings.SecurityProvider()))
    {
        Trace.WriteInfo(TraceType, traceId, "Add {0} to allowed client list", FabricConstants::WindowsFabricAllowedUsersGroupName);
        serverSecuritySettings.AddRemoteIdentity(FabricConstants::WindowsFabricAllowedUsersGroupName);
        serverSecuritySettings.AddRemoteIdentity(FabricConstants::WindowsFabricAdministratorsGroupName);
        // No need to explictly add the current account, as transport always allows listener's account to connect
    }

    return error;

#endif
}

ErrorCode SecurityUtil::EnableClientRoleIfNeeded(
    SecuritySettings & serverSecuritySettings,
    FabricNodeConfig const & nodeConfig,
    wstring const& traceId)
{
    ErrorCode error;
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    if ((serverSecuritySettings.SecurityProvider() == SecurityProvider::None) || !securityConfig.IsClientRoleInEffect())
    {
        return error;
    }

    if (serverSecuritySettings.SecurityProvider() == SecurityProvider::Ssl)
    {
        error = serverSecuritySettings.EnableAdminRole(
            securityConfig.AdminClientCertThumbprints,
            securityConfig.AdminClientX509Names,
            securityConfig.AdminClientCommonNames);

        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceType,
                traceId,
                "EnableAdminRole failed on {0};{1};{2}",
                securityConfig.AdminClientCertThumbprints,
                securityConfig.AdminClientX509Names,
                securityConfig.AdminClientCommonNames);

            return error;
        }

#ifdef PLATFORM_UNIX
        // Stop having implicit behavior wherever possible, as they may cause potential complexity in upgrade.
        return error;
#else

        if (!PaasConfig::GetConfig().ClusterId.empty() || !SecurityConfig::GetConfig().AllowDefaultClient)
        {
            Trace.WriteInfo(TraceType, traceId, "SFRP or SFOP cluster, skip adding default admin client certificate to admin role list");
            return error;
        }

        if (!nodeConfig.ClientAuthX509FindValueSecondary.empty())
        {
            Trace.WriteError(TraceType, traceId, "Non-SFRP and Non-SFOP cluster, X509FindValueSecondary is not supported");
            return ErrorCodeValue::InvalidConfiguration;
        }

        CertContextUPtr defaultClientCert;
        error = CryptoUtility::GetCertificate(
            X509Default::StoreLocation(),
            nodeConfig.ClientAuthX509StoreName,
            nodeConfig.ClientAuthX509FindType,
            nodeConfig.ClientAuthX509FindValue,
            defaultClientCert);

        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceType, traceId, "Failed to retrieve default client certificate: {0}", error);
            return error;
        }

        error = serverSecuritySettings.AddAdminClientX509Name(defaultClientCert.get());
        Trace.WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
            TraceType,
            "implicitly add default client certificate to admin role: {0}",
            error);

        return error;
#endif
    }

    Invariant(SecurityProvider::IsWindowsProvider(serverSecuritySettings.SecurityProvider()));

    serverSecuritySettings.EnableAdminRole(securityConfig.AdminClientIdentities);
    serverSecuritySettings.AddClientToAdminRole(FabricConstants::WindowsFabricAdministratorsGroupName);
    // No need to explictly add the current account to admin role list, it is automatically added in transport

    return error;
}

#ifdef PLATFORM_UNIX

_Use_decl_annotations_  ErrorCode SecurityUtil::GetX509CltCredExpiration(
    Common::X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    DateTime & expiration)
{
    Assert::CodingError("Not implemented");
    return ErrorCode();
}

#else

SECURITY_STATUS SecurityUtil::VerifyCertificate(
    PCCERT_CONTEXT certContext,
    ThumbprintSet const & certThumbprintsToMatch,
    SecurityConfig::X509NameMap const & x509NamesToMatch,
    bool traceCert,
    DWORD certChainFlags,
    bool shouldIgnoreCrlOffline)
{
    return SecurityContextSsl::VerifyCertificate(
        L"",
        certContext,
        certChainFlags,
        shouldIgnoreCrlOffline,
        false,
        certThumbprintsToMatch,
        x509NamesToMatch,
        traceCert);
}

_Use_decl_annotations_  ErrorCode SecurityUtil::GetX509CltCredExpiration(
    Common::X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<Common::X509FindValue> const & findValue,
    DateTime & expiration)
{
    vector<SecurityCredentialsSPtr> cred;
    auto error = SecurityCredentials::AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue,
        &cred,
        nullptr);

    if (!error.IsSuccess())
    {
        expiration = DateTime::Zero;
        return error;
    }

    expiration = cred.front()->Expiration();
    return error;
}

#endif

