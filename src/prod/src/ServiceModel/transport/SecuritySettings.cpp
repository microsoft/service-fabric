// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceType = "SecuritySettings";

//Specifies settings with no security
SecuritySettings::SecuritySettings()
    : x509StoreLocation_(X509Default::StoreLocation())
{
}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateNegotiateServer(wstring const & clientIdentity, SecuritySettings & object)
{
    object = SecuritySettings();

    object.securityProvider_ = SecurityProvider::Negotiate;
    if (!clientIdentity.empty())
    {
        object.remoteIdentities_.insert(clientIdentity);
    }

    // Need to turn on protection to mitigate "forwarding attacks", details described in bug 931057
    object.protectionLevel_ = ProtectionLevel::Sign;

    return ErrorCode::Success();
}
_Use_decl_annotations_
ErrorCode SecuritySettings::CreateNegotiateClient(wstring const & serverIdentity, SecuritySettings & object)
{
    object = SecuritySettings();

    object.securityProvider_ = SecurityProvider::Negotiate;
    object.remoteSpn_ = serverIdentity;

    // Need to turn on protection to mitigate "forwarding attacks", details described in bug 931057
    object.protectionLevel_ = ProtectionLevel::Sign;

    return ErrorCode::Success();
}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateNegotiate(
wstring const & serverIdentity,
wstring const & clientIdentities,
wstring const & protectionLevel,
SecuritySettings & object)
{
    object = SecuritySettings();

    object.securityProvider_ = SecurityProvider::Negotiate;
    object.remoteSpn_ = serverIdentity;

    vector<wstring> remoteIdentities;
    StringUtility::Split<wstring>(clientIdentities, remoteIdentities, L",", true);
    object.remoteIdentities_.insert(remoteIdentities.cbegin(), remoteIdentities.cend());

    return ProtectionLevel::Parse(protectionLevel, object.protectionLevel_);
}

_Use_decl_annotations_
ErrorCode SecuritySettings::GenerateClientCert(SecureString & certKey, CertContextUPtr & cert) const
{
    wstring commonName = Guid::NewGuid().ToString();
    ErrorCode error;
#ifndef PLATFORM_UNIX
    error = CryptoUtility::GenerateExportableKey(
        L"CN=" + commonName,
        false /* fMachineKeyset=false */,
        certKey);

    if (!error.IsSuccess()) { return error; }
#endif
    error = CryptoUtility::CreateSelfSignedCertificate(
        L"CN=" + commonName,
        L"CN=" + commonName,
        false /* fMachineKeyset=false */,
        cert);

    if (!error.IsSuccess()) { return error; }

    Thumbprint thumbprint;
    error = thumbprint.Initialize(cert.get());

    if (!error.IsSuccess()) { return error; }

    remoteCertThumbprints_.Add(thumbprint);

    return ErrorCode::Success();
}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateSelfGeneratedCertSslServer(SecuritySettings & object)
{
    object = SecuritySettings();

    wstring commonName = Guid::NewGuid().ToString();

    CertContextUPtr cert;
    auto error = CryptoUtility::CreateSelfSignedCertificate(
        L"CN=" + commonName,
        L"CN=" + commonName,
        false /* fMachineKeyset=false */,
        cert);
    object.certContext_ = make_shared<CertContextUPtr>(move(cert));

    if (!error.IsSuccess()) { return error; }

    object.isSelfGeneratedCert_ = true;
    object.securityProvider_ = SecurityProvider::Ssl;

    Thumbprint thumbprint;
    error = thumbprint.Initialize(object.certContext_->get());

    if (!error.IsSuccess()) { return error; }

    // Need to turn on protection to mitigate "forwarding attacks", details described in bug 931057
    object.protectionLevel_ = ProtectionLevel::EncryptAndSign;

    return ErrorCode::Success();

}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateSslClient(
    Common::CertContextUPtr & certContext,
    wstring const & serverThumbprint,
    SecuritySettings & object)
{
    object = SecuritySettings();

    object.isSelfGeneratedCert_ = true;
    object.securityProvider_ = SecurityProvider::Ssl;
    object.certContext_ = make_shared<CertContextUPtr>(move(certContext));

    object.SetRemoteCertThumbprints(serverThumbprint);

    // Need to turn on protection to mitigate "forwarding attacks", details described in bug 931057
    object.protectionLevel_ = ProtectionLevel::EncryptAndSign;

    return ErrorCode::Success();
}

ErrorCode SecuritySettings::CreateSslClient(
    wstring const & serverThumbprint,
    _Out_ SecuritySettings & object)
{
    object = SecuritySettings();
    object.securityProvider_ = SecurityProvider::Ssl;
    object.isSelfGeneratedCert_ = false;
    object.protectionLevel_ = ProtectionLevel::EncryptAndSign;

    object.x509StoreLocation_ = X509StoreLocation::CurrentUser;
    object.x509StoreName_ = L"MY";
    X509FindValue::Create(X509FindType::FindByThumbprint, serverThumbprint, object.x509FindValue_);

    return object.remoteCertThumbprints_.Add(serverThumbprint);
}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateKerberos(
wstring const & remoteSpn,
wstring const & clientIdentities,
wstring const & protectionLevel,
SecuritySettings & object)
{
    object = SecuritySettings();

    object.securityProvider_ = SecurityProvider::Kerberos;
    object.remoteSpn_ = remoteSpn;

    vector<wstring> remoteIdentities;
    StringUtility::Split<wstring>(clientIdentities, remoteIdentities, L",", true);
    object.remoteIdentities_.insert(remoteIdentities.cbegin(), remoteIdentities.cend());

    return ProtectionLevel::Parse(protectionLevel, object.protectionLevel_);
}

_Use_decl_annotations_ ErrorCode SecuritySettings::FromConfiguration(
    wstring const & credentialType,
    wstring const & x509StoreName,
    wstring const & x509StoreLocation,
    wstring const & x509FindType,
    wstring const & x509FindValue,
    wstring const & x509FindValueSecondary,
    wstring const & protectionLevel,
    wstring const & remoteCertThumbprints,
    SecurityConfig::X509NameMap const & remoteX509Names,
    SecurityConfig::IssuerStoreKeyValueMap const & remoteCertIssuers,
    wstring const & remoteCertCommonNames,
    wstring const & defaultRemoteCertIssuers,
    wstring const & remoteSpn,
    wstring const & clientIdentities,
    SecuritySettings & object)
{
    object = SecuritySettings();

    auto error = SecurityProvider::FromCredentialType(credentialType, object.securityProvider_);
    if (!error.IsSuccess()) { return error; }

    if (object.securityProvider_ == SecurityProvider::None)
    {
        return ErrorCode::Success();
    }

    error = ProtectionLevel::Parse(protectionLevel, object.protectionLevel_);
    if (!error.IsSuccess()) { return error; }    

    if (object.securityProvider_ == SecurityProvider::Negotiate)
    {
        return CreateNegotiate(remoteSpn, clientIdentities, protectionLevel, object);
    }

    if (object.securityProvider_ == SecurityProvider::Kerberos)
    {
        return CreateKerberos(remoteSpn, clientIdentities, protectionLevel, object);
    }

    if (object.securityProvider_ == SecurityProvider::Claims)
    {
        // Claims type is only allowed on public API
        return ErrorCodeValue::InvalidCredentialType;
    }

    ASSERT_IFNOT(
        object.securityProvider_ == SecurityProvider::Ssl,
        "Unexpected security provider type at FromConfiguration: {0}",
        object.securityProvider_);

    error = object.remoteCertThumbprints_.Set(remoteCertThumbprints);
    if (!error.IsSuccess()) return error;

    if (!remoteX509Names.ParseError().IsSuccess())
    {
        WriteError(TraceType, "remoteX509Names.Error: {0}", remoteX509Names.ParseError());
        return remoteX509Names.ParseError();
    }

    object.remoteX509Names_ = (remoteX509Names);

    error = object.defaultX509IssuerThumbprints_.SetToThumbprints(defaultRemoteCertIssuers);
    if (!error.IsSuccess()) return error;

    if (!object.defaultX509IssuerThumbprints_.IsEmpty() && !object.remoteX509Names_.IsEmpty())
    {
        WriteError(TraceType, "Default issuer list '{0}' cannot be used together with X509Names '{1}'", object.defaultX509IssuerThumbprints_, object.remoteX509Names_);
        return ErrorCodeValue::InvalidX509NameList;
    }

    vector<wstring> remoteNamesIssuersUnspecified;
    StringUtility::Split<wstring>(remoteCertCommonNames, remoteNamesIssuersUnspecified, L",", true);
    for (auto const & name : remoteNamesIssuersUnspecified)
    {
        object.remoteX509Names_.Add(name, object.defaultX509IssuerThumbprints_);
    }

    object.remoteCertIssuers_ = remoteCertIssuers;
    if (!object.defaultX509IssuerThumbprints_.IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
    {
        WriteError(TraceType, "Default issuer list '{0}' cannot be used together with RemoteCertIssuers '{1}'", object.defaultX509IssuerThumbprints_, object.remoteCertIssuers_);
        return ErrorCodeValue::InvalidX509NameList;
    }

    if (!object.remoteX509Names_.GetAllIssuers().IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
    {
        WriteError(TraceType, "X509Names '{0}' with issuer pinning cannot be used together with RemoteCertIssuers '{1}'", object.remoteX509Names_, object.remoteCertIssuers_);
        return ErrorCodeValue::InvalidX509NameList;
    }

    error = UpdateRemoteIssuersIfNeeded(object);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, "FromConfiguration: UpdateRemoteIssuersIfNeeded failed: {0}", error);
        return error;
    }

    object.x509StoreName_ = x509StoreName;
#ifndef PLATFORM_UNIX
    if (object.x509StoreName_.empty()) { return ErrorCode(ErrorCodeValue::InvalidX509StoreName); }
#endif

    if (x509StoreLocation.empty()) { return Common::ErrorCodeValue::InvalidX509StoreLocation; }
    error = X509StoreLocation::Parse(x509StoreLocation, object.x509StoreLocation_);
    if (!error.IsSuccess()) { return error; }

    return X509FindValue::Create(x509FindType, x509FindValue, x509FindValueSecondary, object.x509FindValue_);
}

_Use_decl_annotations_
ErrorCode SecuritySettings::CreateClaimTokenClient(
wstring const & localClaimToken,
wstring const & serverCertThumbpints,
wstring const & serverCertCommonNames,
wstring const & serverCertIssuers,
wstring const & protectionLevel,
SecuritySettings & object)
{
    object = SecuritySettings();
    object.securityProvider_ = SecurityProvider::Claims;
    object.localClaimToken_ = localClaimToken;
    StringUtility::TrimWhitespaces(object.localClaimToken_);
    if (object.localClaimToken_.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto error = object.remoteCertThumbprints_.Set(serverCertThumbpints);
    if (!error.IsSuccess()) return error;

    error = object.defaultX509IssuerThumbprints_.SetToThumbprints(serverCertIssuers);
    if (!error.IsSuccess()) return error;

    vector<wstring> svrNames;
    StringUtility::Split<wstring>(serverCertCommonNames, svrNames, L",", true);
    for (auto const & name : svrNames)
    {
        object.remoteX509Names_.Add(name, object.defaultX509IssuerThumbprints_);
    }

    return ProtectionLevel::Parse(protectionLevel, object.protectionLevel_);
}

bool SecuritySettings::operator == (SecuritySettings const & rhs) const
{
    if (securityProvider_ != rhs.securityProvider_)
    {
        WriteInfo(TraceType, "securityProvider_: '{0}' != '{1}'", securityProvider_, rhs.securityProvider_);
        return false;
    }

    if (this->securityProvider_ == SecurityProvider::None)
    {
        // No other validation is needed
        return true;
    }

    if (protectionLevel_ != rhs.protectionLevel_)
    {
        WriteInfo(TraceType, "protectionLevel_: '{0}' != '{1}'", protectionLevel_, rhs.protectionLevel_);
        return false;
    }

    if (remoteIdentities_ != rhs.remoteIdentities_)
    {
        WriteInfo(TraceType, "remoteIdentities_: '{0}' != '{1}'", remoteIdentities_, rhs.remoteIdentities_);
        return false;
    }

    if (adminClientIdentities_ != rhs.adminClientIdentities_)
    {
        WriteInfo(TraceType, "adminClientIdentities_: '{0}' != '{1}'", adminClientIdentities_, rhs.adminClientIdentities_);
        return false;
    }

    if (adminClientX509Names_ != rhs.adminClientX509Names_)
    {
        WriteInfo(TraceType, "adminClientX509Names_: '{0}' != '{1}'", adminClientX509Names_, rhs.adminClientX509Names_);
        return false;
    }

    if (isClientRoleInEffect_ != rhs.isClientRoleInEffect_)
    {
        WriteInfo(TraceType, "isClientRoleInEffect_: '{0}' != '{1}'", isClientRoleInEffect_, rhs.isClientRoleInEffect_);
        return false;
    }

    if (remoteCertThumbprints_ != rhs.remoteCertThumbprints_)
    {
        WriteInfo(TraceType, "remoteCertThumbprints_: '{0}' != '{1}'", remoteCertThumbprints_, rhs.remoteCertThumbprints_);
        return false;
    }

    if (remoteX509Names_ != rhs.remoteX509Names_)
    {
        WriteInfo(TraceType, "remoteX509Names_: '{0}' != '{1}'", remoteX509Names_, rhs.remoteX509Names_);
        return false;
    }

    if (remoteCertIssuers_ != rhs.remoteCertIssuers_)
    {
        WriteInfo(TraceType, "remoteCertIssuers_: '{0}' != '{1}'", remoteCertIssuers_, rhs.remoteCertIssuers_);
        return false;
    }

    if (securityProvider_ == SecurityProvider::Ssl)
    {
        if (x509StoreLocation_ != rhs.x509StoreLocation_)
        {
            WriteInfo(TraceType, "x509StoreLocation_: '{0}' != '{1}'", x509StoreLocation_, rhs.x509StoreLocation_);
            return false;
        }

        if (x509StoreName_ != rhs.x509StoreName_)
        {
            WriteInfo(TraceType, "x509StoreName_: '{0}' != '{1}'", x509StoreName_, rhs.x509StoreName_);
            return false;
        }

        if ((x509FindValue_ && !rhs.x509FindValue_) || (!x509FindValue_ && rhs.x509FindValue_))
        {
            WriteInfo(TraceType, "x509FindValue_: {0} != {1}", TextTracePtr(x509FindValue_.get()), TextTracePtr(rhs.x509FindValue_.get()));
            return false;
        }

        if ((x509FindValue_ != rhs.x509FindValue_) && (*x509FindValue_ != *(rhs.x509FindValue_)))
        {
            WriteInfo(TraceType, "x509FindValue_: {0} != {1}", x509FindValue_, rhs.x509FindValue_);
            return false;
        }

        if (claimBasedClientAuthEnabled_ != rhs.claimBasedClientAuthEnabled_)
        {
            WriteInfo(TraceType, "claimBasedClientAuthEnabled_: '{0}' != '{1}'", claimBasedClientAuthEnabled_, rhs.claimBasedClientAuthEnabled_);
            return false;
        }

        if (clientClaims_ != rhs.clientClaims_)
        {
            WriteInfo(TraceType, "clientClaims_: '{0}' != '{1}'", clientClaims_, rhs.clientClaims_);
            return false;
        }

        if (adminClientClaims_ != rhs.adminClientClaims_)
        {
            WriteInfo(TraceType, "adminClientClaims_: '{0}' != '{1}'", adminClientClaims_, rhs.adminClientClaims_);
            return false;
        }
    }
    else if (SecurityProvider::IsWindowsProvider(securityProvider_))
    {
        if (!StringUtility::AreEqualCaseInsensitive(remoteSpn_, rhs.remoteSpn_))
        {
            WriteInfo(TraceType, "remoteSpn_: '{0}' != '{1}'", remoteSpn_, rhs.remoteSpn_);
            return false;
        }
    }
    else if (securityProvider_ == SecurityProvider::Claims)
    {
        if (localClaimToken_ != rhs.localClaimToken_) // not doing case-insensive comparison, because token string could be encrypted
        {
            WriteInfo(TraceType, "localClaimToken_: '{0}' != '{1}'", localClaimToken_, rhs.localClaimToken_);
            return false;
        }
    }

    return true;
}

bool SecuritySettings::operator != (SecuritySettings const & rhs) const
{
    return !(*this == rhs);
}

_Use_decl_annotations_
ErrorCode SecuritySettings::FromPublicApi(
FABRIC_SECURITY_CREDENTIALS const& securityCredentials,
SecuritySettings & object)
{
    object = SecuritySettings();

    object.credTypeFromPublicApi_ = securityCredentials.Kind;
    auto error = SecurityProvider::FromPublic(securityCredentials.Kind, object.securityProvider_);
    if (!error.IsSuccess()) { return error; }

    if (object.securityProvider_ == SecurityProvider::None)
    {
        return ErrorCodeValue::Success;
    }

    if (SecurityProvider::IsWindowsProvider(object.securityProvider_))
    {
        auto windowsCredentials = reinterpret_cast<FABRIC_WINDOWS_CREDENTIALS*>(securityCredentials.Value);
        object.remoteSpn_ = windowsCredentials->RemoteSpn;
        for (ULONG i = 0; i < windowsCredentials->RemoteIdentityCount; ++i)
        {
            object.remoteIdentities_.insert(windowsCredentials->RemoteIdentities[i]);
        }

        return ProtectionLevel::FromPublic(windowsCredentials->ProtectionLevel, object.protectionLevel_);
    }

    if (object.securityProvider_ == SecurityProvider::Claims)
    {
        auto claimCredentials = reinterpret_cast<FABRIC_CLAIMS_CREDENTIALS*>(securityCredentials.Value);

        for (ULONG i = 0; i < claimCredentials->IssuerThumbprintCount; ++i)
        {
            LPCWSTR const& issuerThumbprint = claimCredentials->IssuerThumbprints[i];
            error = object.defaultX509IssuerThumbprints_.AddThumbprint(issuerThumbprint);
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        // Public API only support client side settings, thus we do not have to worry about RBAC
        for (ULONG i = 0; i < claimCredentials->ServerCommonNameCount; i++)
        {
            if (claimCredentials->ServerCommonNames[i] == nullptr)
            {
                return ErrorCodeValue::InvalidAllowedCommonNameList;
            }

            object.remoteX509Names_.Add(claimCredentials->ServerCommonNames[i], object.defaultX509IssuerThumbprints_);
        }

        object.localClaimToken_ = claimCredentials->LocalClaims;

        if (claimCredentials->Reserved != nullptr)
        {
            auto claimCredentialsEx1 = reinterpret_cast<FABRIC_CLAIMS_CREDENTIALS_EX1*>(claimCredentials->Reserved);

            for (ULONG i = 0; i < claimCredentialsEx1->ServerThumbprintCount; i++)
            {
                if (claimCredentialsEx1->ServerThumbprints[i] == nullptr)
                {
                    return ErrorCodeValue::InvalidAllowedCommonNameList;
                }

                error = object.remoteCertThumbprints_.Add(claimCredentialsEx1->ServerThumbprints[i]);
                if (!error.IsSuccess())
                {
                    WriteError(TraceType, "FromPublicApi: remoteCertThumbprints_.Add failed: {0}", error);
                    return error;
                }
            }
        }

        //
        // Claims token is allowed to be empty. For AAD, it can be retrieved at runtime
        // when SecurityContextSsl fires ClaimsHandlerFunc()
        //

        return ProtectionLevel::FromPublic(claimCredentials->ProtectionLevel, object.protectionLevel_);
    }

    ASSERT_IFNOT(
        object.securityProvider_ == SecurityProvider::Ssl,
        "Unexpected security provider type at FromPublicApi: {0}",
        object.securityProvider_);

    if (object.credTypeFromPublicApi_ == FABRIC_SECURITY_CREDENTIAL_KIND_X509_2)
    {
#ifndef PLATFORM_UNIX
        WriteError(TraceType, "FromPublicApi: FABRIC_SECURITY_CREDENTIAL_KIND_X509_2 not supported");
        return ErrorCodeValue::InvalidCredentials; 
#else
        auto x509SCredentials = reinterpret_cast<FABRIC_X509_CREDENTIALS2*>(securityCredentials.Value);
        if (x509SCredentials == NULL) { return ErrorCode(ErrorCodeValue::InvalidCredentials); }

        error = ProtectionLevel::FromPublic(x509SCredentials->ProtectionLevel, object.protectionLevel_);
        if (!error.IsSuccess()) { return error; }

        object.x509StoreLocation_ = X509Default::StoreLocation();
        object.x509StoreName_ = wstring(x509SCredentials->CertLoadPath);
        // x509FindValue_ is left empty

        for (ULONG i = 0; i < x509SCredentials->RemoteCertThumbprintCount; ++i)
        {
            LPCWSTR remoteCertThumbprint = x509SCredentials->RemoteCertThumbprints[i];
            error = object.remoteCertThumbprints_.Add(remoteCertThumbprint);
            if (!error.IsSuccess())
            {
                WriteError(TraceType, "FromPublicApi: remoteCertThumbprints_.Add failed: {0}", error);
                return error;
            }
        }

        error = object.remoteX509Names_.AddNames(x509SCredentials->RemoteX509Names, x509SCredentials->RemoteX509NameCount);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "FromPublicApi: remoteX509Names_.AddNames failed: {0}", error);
            return error;
        }
        
        if (x509SCredentials->Reserved == nullptr)
        {
           return error;
        }

        FABRIC_X509_CREDENTIALS_EX3 const * x509Ex3 = (FABRIC_X509_CREDENTIALS_EX3 const *)(x509SCredentials->Reserved);
        object.remoteCertIssuers_.AddIssuerEntries(x509Ex3->RemoteCertIssuers, x509Ex3->RemoteCertIssuerCount);

        if (!object.defaultX509IssuerThumbprints_.IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
        {
            WriteError(TraceType, "FromPublicApi: Default issuer list '{0}' cannot be used together with RemoteCertIssuers '{1}'", object.defaultX509IssuerThumbprints_, object.remoteCertIssuers_);
            return ErrorCodeValue::InvalidX509NameList;
        }

        if (!object.remoteX509Names_.GetAllIssuers().IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
        {
            WriteError(TraceType, "FromPublicApi: X509Names '{0}' with issuer pinning cannot be used together with RemoteCertIssuers '{1}'", object.remoteX509Names_, object.remoteCertIssuers_);
            return ErrorCodeValue::InvalidX509NameList;
        }

        error = UpdateRemoteIssuersIfNeeded(object);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "FromPublicApi: UpdateRemoteIssuersIfNeeded failed: {0}", error);
            return error;
        }

        return error;
#endif
    }

    auto x509SCredentials = reinterpret_cast<FABRIC_X509_CREDENTIALS*>(securityCredentials.Value);
    if (x509SCredentials == NULL) { return ErrorCode(ErrorCodeValue::InvalidCredentials); }

    error = ProtectionLevel::FromPublic(x509SCredentials->ProtectionLevel, object.protectionLevel_);
    if (!error.IsSuccess()) { return error; }

    error = X509StoreLocation::FromPublic(x509SCredentials->StoreLocation, object.x509StoreLocation_);
    if (!error.IsSuccess()) { return error; }

    object.x509StoreName_ = wstring(x509SCredentials->StoreName);
#ifndef PLATFORM_UNIX
    if (object.x509StoreName_.empty()) { return ErrorCode(ErrorCodeValue::InvalidX509StoreName); }
#endif

    for (ULONG i = 0; i < x509SCredentials->AllowedCommonNameCount; i++)
    {
        if (x509SCredentials->AllowedCommonNames[i] == nullptr)
        {
            return ErrorCodeValue::InvalidAllowedCommonNameList;
        }

        object.remoteX509Names_.Add(x509SCredentials->AllowedCommonNames[i]);
    }

    if (x509SCredentials->Reserved == nullptr)
    {
        error = X509FindValue::Create(x509SCredentials->FindType, (LPCWSTR)x509SCredentials->FindValue, nullptr, object.x509FindValue_);
        if (!error.IsSuccess()) WriteError(TraceType, "FromPublicApi: X509FindValue::Create(primary) failed: {0}", error);
        return error;
    }

    FABRIC_X509_CREDENTIALS_EX1 const * x509Ex1 = (FABRIC_X509_CREDENTIALS_EX1 const *)(x509SCredentials->Reserved);
    for (ULONG i = 0; i < x509Ex1->IssuerThumbprintCount; ++i)
    {
        LPCWSTR issuerThumbprint = x509Ex1->IssuerThumbprints[i];
        error = object.defaultX509IssuerThumbprints_.AddThumbprint(issuerThumbprint);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "FromPublicApi: defaultX509IssuerThumbprints_.Add failed: {0}", error);
            return error;
        }
    }

    if (!object.defaultX509IssuerThumbprints_.IsEmpty())
    {
        error = object.remoteX509Names_.SetDefaultIssuers(object.defaultX509IssuerThumbprints_);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "FromPublicApi: remoteX509Names_.SetDefaultIssuers failed: {0}", error);
            return error;
        }
    }

    if (x509Ex1->Reserved == nullptr)
    {
        error = X509FindValue::Create(x509SCredentials->FindType, (LPCWSTR)x509SCredentials->FindValue, nullptr, object.x509FindValue_);
        if (!error.IsSuccess()) WriteError(TraceType, "FromPublicApi: X509FindValue::Create(primary) failed: {0}", error);
        return error;
    }

    FABRIC_X509_CREDENTIALS_EX2 const * x509Ex2 = (FABRIC_X509_CREDENTIALS_EX2 const *)(x509Ex1->Reserved);
    for (ULONG i = 0; i < x509Ex2->RemoteCertThumbprintCount; ++i)
    {
        LPCWSTR remoteCertThumbprint = x509Ex2->RemoteCertThumbprints[i];
        error = object.remoteCertThumbprints_.Add(remoteCertThumbprint);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "FromPublicApi: remoteCertThumbprints_.Add failed: {0}", error);
            return error;
        }
    }

    if (!object.defaultX509IssuerThumbprints_.IsEmpty() && (x509Ex2->RemoteX509NameCount > 0))
    {
        WriteError(TraceType, "FromPublicApi: Default issuer list '{0}' cannot be used together with X509Names '{1}'", object.defaultX509IssuerThumbprints_, x509Ex2->RemoteX509NameCount);
        return ErrorCodeValue::InvalidX509NameList;
    }

    error = object.remoteX509Names_.AddNames(x509Ex2->RemoteX509Names, x509Ex2->RemoteX509NameCount);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, "FromPublicApi: remoteX509Names_.AddNames failed: {0}", error);
        return error;
    }

    error = X509FindValue::Create(x509SCredentials->FindType, x509SCredentials->FindValue, x509Ex2->FindValueSecondary, object.x509FindValue_);
    if (!error.IsSuccess()) 
    {
        WriteError(TraceType, "FromPublicApi: X509FindValue::Create(primary, secondary) failed: {0}", error);
        return error;
    }

    if (x509Ex2->Reserved == nullptr)
    {
         return error;
    }

    FABRIC_X509_CREDENTIALS_EX3 const * x509Ex3 = (FABRIC_X509_CREDENTIALS_EX3 const *)(x509Ex2->Reserved);
    object.remoteCertIssuers_.AddIssuerEntries(x509Ex3->RemoteCertIssuers, x509Ex3->RemoteCertIssuerCount);

    if (!object.defaultX509IssuerThumbprints_.IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
    {
        WriteError(TraceType, "FromPublicApi: Default issuer list '{0}' cannot be used together with RemoteCertIssuers '{1}'", object.defaultX509IssuerThumbprints_, object.remoteCertIssuers_);
        return ErrorCodeValue::InvalidX509NameList;
    }

    if (!object.remoteX509Names_.GetAllIssuers().IsEmpty() && !object.remoteCertIssuers_.IsEmpty())
    {
        WriteError(TraceType, "FromPublicApi: X509Names '{0}' with issuer pinning cannot be used together with RemoteCertIssuers '{1}'", object.remoteX509Names_, object.remoteCertIssuers_);
        return ErrorCodeValue::InvalidX509NameList;
    }

    error = UpdateRemoteIssuersIfNeeded(object);
    if (!error.IsSuccess())
    {
       WriteError(TraceType, "FromPublicApi: UpdateRemoteIssuersIfNeeded failed: {0}", error);
       return error;
    }
   
    return error;
}

_Use_decl_annotations_
void SecuritySettings::ToPublicApi(Common::ScopedHeap & heap, FABRIC_SECURITY_CREDENTIALS & securityCredentials) const
{
    if (securityProvider_ == SecurityProvider::Ssl)
    {
        securityCredentials.Kind = SecurityProvider::ToPublic(securityProvider_, credTypeFromPublicApi_);
        if (securityCredentials.Kind ==  FABRIC_SECURITY_CREDENTIAL_KIND_X509_2)
        {
            auto x509SCredentials = heap.AddItem<FABRIC_X509_CREDENTIALS2>();
            securityCredentials.Value = x509SCredentials.GetRawPointer();

            x509SCredentials->ProtectionLevel = ProtectionLevel::ToPublic(protectionLevel_);
            x509SCredentials->CertLoadPath = heap.AddString(x509StoreName_);

            x509SCredentials->RemoteCertThumbprintCount = (ULONG)remoteCertThumbprints_.Value().size();
            auto remoteCertThumbprints = heap.AddArray<LPCWSTR>(remoteCertThumbprints_.Value().size());
            ULONG index = 0;
            for (auto const & thumbprint : remoteCertThumbprints_.Value())
            {
                remoteCertThumbprints[index++] = heap.AddString(thumbprint.PrimaryToString());
            }
            x509SCredentials->RemoteCertThumbprints = remoteCertThumbprints.GetRawArray();

            auto nameList = remoteX509Names_.ToVector();
            x509SCredentials->RemoteX509NameCount = (ULONG)nameList.size();
            auto remoteX509NameArray = heap.AddArray<FABRIC_X509_NAME>(nameList.size());
            for (ULONG i = 0; i < x509SCredentials->RemoteX509NameCount; ++i)
            {
                remoteX509NameArray[i].Name = heap.AddString(nameList[i].first, true);
                remoteX509NameArray[i].IssuerCertThumbprint = heap.AddString(nameList[i].second, true);
            }
            x509SCredentials->RemoteX509Names = remoteX509NameArray.GetRawArray();

            ReferencePointer<FABRIC_X509_CREDENTIALS_EX3> x509Ex3RPtr = heap.AddItem<FABRIC_X509_CREDENTIALS_EX3>(); //allocate and zero memory
            x509SCredentials->Reserved = x509Ex3RPtr.GetRawPointer();
 
            remoteCertIssuers_.ToPublicApi(heap, *x509Ex3RPtr);

            return;
        }

        auto x509SCredentials = heap.AddItem<FABRIC_X509_CREDENTIALS>();
        securityCredentials.Value = x509SCredentials.GetRawPointer();

        x509SCredentials->FindType = X509FindType::ToPublic(x509FindValue_->Type());
        x509SCredentials->FindValue = x509FindValue_->PrimaryValueToPublicPtr(heap);

        x509SCredentials->ProtectionLevel = ProtectionLevel::ToPublic(protectionLevel_);
        x509SCredentials->StoreLocation = X509StoreLocation::ToPublic(x509StoreLocation_);
        x509SCredentials->StoreName = heap.AddString(x509StoreName_);

        // defaultX509IssuerThumbprints_ is skipped as it is covered by remoteX509Names_
        if (remoteCertThumbprints_.Value().empty() &&
            !x509FindValue_->Secondary() &&
            remoteX509Names_.IsEmpty())
            return;

        ReferencePointer<FABRIC_X509_CREDENTIALS_EX1> x509Ex1RPtr = heap.AddItem<FABRIC_X509_CREDENTIALS_EX1>(); //allocate and zero memory
        x509SCredentials->Reserved = x509Ex1RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX1 & x509Ex1 = *x509Ex1RPtr;

        ReferencePointer<FABRIC_X509_CREDENTIALS_EX2> x509Ex2RPtr = heap.AddItem<FABRIC_X509_CREDENTIALS_EX2>(); //allocate and zero memory
        x509Ex1.Reserved = x509Ex2RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX2 & x509Ex2 = *x509Ex2RPtr;

        ReferencePointer<FABRIC_X509_CREDENTIALS_EX3> x509Ex3RPtr = heap.AddItem<FABRIC_X509_CREDENTIALS_EX3>(); //allocate and zero memory
        x509Ex2.Reserved = x509Ex3RPtr.GetRawPointer();

        x509Ex2.RemoteCertThumbprintCount = (ULONG)remoteCertThumbprints_.Value().size();
        auto remoteCertThumbprints = heap.AddArray<LPCWSTR>(remoteCertThumbprints_.Value().size());
        ULONG index = 0;
        for (auto const & thumbprint : remoteCertThumbprints_.Value())
        {
            remoteCertThumbprints[index++] = heap.AddString(thumbprint.PrimaryToString());
        }
        x509Ex2.RemoteCertThumbprints = remoteCertThumbprints.GetRawArray();

        if (x509FindValue_->Secondary())
        {
            x509Ex2.FindValueSecondary = x509FindValue_->Secondary()->PrimaryValueToPublicPtr(heap);
        }

        auto nameList = remoteX509Names_.ToVector();
        x509Ex2.RemoteX509NameCount = (ULONG)nameList.size();
        auto remoteX509NameArray = heap.AddArray<FABRIC_X509_NAME>(nameList.size());
        for (ULONG i = 0; i < x509Ex2.RemoteX509NameCount; ++i)
        {
            remoteX509NameArray[i].Name = heap.AddString(nameList[i].first, true);
            remoteX509NameArray[i].IssuerCertThumbprint = heap.AddString(nameList[i].second, true);
        }
        x509Ex2.RemoteX509Names = remoteX509NameArray.GetRawArray();

        remoteCertIssuers_.ToPublicApi(heap, *x509Ex3RPtr);

        return;
    }

    if (SecurityProvider::IsWindowsProvider(securityProvider_))
    {
        securityCredentials.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_WINDOWS;
        auto windowsCredentials = heap.AddItem<FABRIC_WINDOWS_CREDENTIALS>();
        windowsCredentials->Reserved = nullptr;
        securityCredentials.Value = windowsCredentials.GetRawPointer();

        windowsCredentials->RemoteSpn = heap.AddString(remoteSpn_);

        windowsCredentials->RemoteIdentityCount = (ULONG)(remoteIdentities_.size());
        auto remoteIdentities = heap.AddArray<LPCWSTR>(remoteIdentities_.size());
        ULONG i = 0;
        for (auto iter = remoteIdentities_.cbegin(); iter != remoteIdentities_.cend(); ++iter, ++i)
        {
            remoteIdentities[i] = heap.AddString(*iter);
        }

        windowsCredentials->RemoteIdentities = remoteIdentities.GetRawArray();
        windowsCredentials->ProtectionLevel = ProtectionLevel::ToPublic(protectionLevel_);

        return;
    }

    ASSERT_IFNOT(securityProvider_ == SecurityProvider::None, "Unexpected security CredentialType at ToPublicApi: {0}", securityProvider_);
    // Set credentials to null
    securityCredentials.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_NONE;
    securityCredentials.Value = NULL;
}

void SecuritySettings::WriteTo(TextWriter & w, FormatOptions const &) const
{
    if (securityProvider_ == SecurityProvider::None)
    {
        w.WriteChar('{');
        w.Write("{0}", securityProvider_);
        w.WriteChar('}');
        return;
    }

    w.WriteChar('{');
    w.Write(" provider={0} protection={1} ", securityProvider_, protectionLevel_);

    if (SecurityProvider::IsWindowsProvider(securityProvider_))
    {
        w.Write("remoteSpn='{0}' ", remoteSpn_);
        if (!remoteIdentities_.empty())
        {
            w.Write("remote='{0}' ", remoteIdentities_);
        }
    }

    if (securityProvider_ == SecurityProvider::Ssl)
    {
        w.Write("certType = '{0}' store='{1}/{2}' findValue='{3}' ", x509CertType_, x509StoreLocation_, x509StoreName_, x509FindValue_);

        if (!remoteCertThumbprints_.Value().empty())
        {
            w.Write("remoteCertThumbprints='{0}' ", remoteCertThumbprints_);
        }

        if (!remoteX509Names_.IsEmpty())
        {
            w.Write("remoteX509Names={0} ", remoteX509Names_);
        }

        if (!remoteCertIssuers_.IsEmpty())
        {
            w.Write("RemoteCertIssuers={0} ", remoteCertIssuers_);
        }

        if (!defaultX509IssuerThumbprints_.IsEmpty())
        {
            w.Write("certIssuers='{0}' ", defaultX509IssuerThumbprints_);
        }

        w.Write("certChainFlags={0:x} ", X509CertChainFlags());
    }

    w.Write("isClientRoleInEffect={0} ", isClientRoleInEffect_);
    if (isClientRoleInEffect_)
    {
        if (!adminClientIdentities_.empty())
        {
            w.Write("adminClientIdentities='{0}' ", adminClientIdentities_);
        }

        if (!adminClientX509Names_.IsEmpty())
        {
            w.Write("adminClientX509Names='{0}' ", adminClientX509Names_);
        }

        if (!adminClientCertThumbprints_.Value().empty())
        {
            w.Write("adminClientCertThumbprints='{0}' ", adminClientCertThumbprints_);
        }
    }

    if (securityProvider_ == SecurityProvider::Claims)
    {
        w.Write("localClaimToken='{0}' ", localClaimToken_);
    }

    w.Write("claimBasedClientAuthEnabled={0} ", claimBasedClientAuthEnabled_);
    if (claimBasedClientAuthEnabled_)
    {
        w.Write("clientClaims='{0}' adminClientClaims='{1}' ", clientClaims_, adminClientClaims_);
    }

    w.WriteChar('}');
}

void SecuritySettings::Test_SetRawValues(SecurityProvider::Enum provider, ProtectionLevel::Enum protectionLevel)
{
    this->securityProvider_ = provider;
    this->protectionLevel_ = protectionLevel;
}

wstring SecuritySettings::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

bool SecuritySettings::IsClientRoleInEffect() const
{
    return isClientRoleInEffect_ || claimBasedClientAuthEnabled_;
}

RoleMask::Enum SecuritySettings::DefaultRemoteRole(bool inbound) const
{
    if (!inbound || IsRemotePeer()) return RoleMask::All;

    if (IsClientRoleInEffect()) return RoleMask::User;

    return defaultRemoteRole_;
}

SecuritySettings::IdentitySet const & SecuritySettings::AdminClientIdentities() const
{
    return adminClientIdentities_;
}

void SecuritySettings::EnableAdminRole(wstring const & adminClientIdList)
{
    isClientRoleInEffect_ = true;
    vector<wstring> adminClientIdentities;
    StringUtility::Split<wstring>(adminClientIdList, adminClientIdentities, L",", true);
    adminClientIdentities_.insert(adminClientIdentities.cbegin(), adminClientIdentities.cend());
    remoteIdentities_.insert(adminClientIdentities.cbegin(), adminClientIdentities.cend());
}

ErrorCode SecuritySettings::EnableAdminRole(
    wstring const & adminClientCertThumbprints,
    SecurityConfig::X509NameMap const & adminClientX509Names,
    wstring const & adminClientList)
{
    isClientRoleInEffect_ = true;

    auto error = adminClientCertThumbprints_.Set(adminClientCertThumbprints);
    if (!error.IsSuccess()) return error;

    remoteCertThumbprints_.Add(adminClientCertThumbprints_);

    if (!defaultX509IssuerThumbprints_.IsEmpty() && !adminClientX509Names.IsEmpty())
    {
        WriteError(TraceType, "EnableAdminRole: default issuer list '{0}' cannot be used together with X509Names '{1}'", defaultX509IssuerThumbprints_, adminClientX509Names);
        return ErrorCodeValue::InvalidX509NameList;
    }

    adminClientX509Names_ = adminClientX509Names;
    vector<wstring> adminClientCNs;
    StringUtility::Split<wstring>(adminClientList, adminClientCNs, L",", true);
    for (auto const & name : adminClientCNs)
    {
        adminClientX509Names_.Add(name, defaultX509IssuerThumbprints_);
    }

    // If issuer store is specified, overwrite all issuer whitelist values in adminClientX509Names_ with public keys obtained from the store
    if (!remoteCertIssuers_.IsEmpty())
    {
        X509IdentitySet clientIssuerPublicKeys;
        error = remoteCertIssuers_.GetPublicKeys(clientIssuerPublicKeys);
        if (!error.IsSuccess()) { return error; }

        adminClientX509Names_.OverwriteIssuers(clientIssuerPublicKeys);
    }

    remoteX509Names_.Add(adminClientX509Names_);

    return error;
}

void SecuritySettings::AddClientToAdminRole(std::wstring const & clientIdentity)
{
    isClientRoleInEffect_ = true;

    if (SecurityProvider::IsWindowsProvider(securityProvider_))
    {
        adminClientIdentities_.insert(clientIdentity);
        remoteIdentities_.insert(clientIdentity);
        return;
    }

    // SecurityProvider::Claims is not supported on server side
    KInvariant(securityProvider_ == SecurityProvider::Ssl);
    adminClientX509Names_.Add(clientIdentity, defaultX509IssuerThumbprints_);
    remoteX509Names_.Add(clientIdentity, defaultX509IssuerThumbprints_);
}

void SecuritySettings::AddAdminClientX509Names(SecurityConfig::X509NameMap const & names)
{
    adminClientX509Names_.Add(names);
    remoteX509Names_.Add(names);
    WriteInfo(
        TraceType,
        "AddAdminClientX509Names: after: adminClientX509Names_: {0}; remoteX509Names_: {1}",
        adminClientX509Names_, remoteX509Names_);
}

void SecuritySettings::AddAdminClientIdentities(IdentitySet const & identities)
{
    adminClientIdentities_.insert(identities.cbegin(), identities.cend());
    remoteIdentities_.insert(identities.cbegin(), identities.cend());
    WriteInfo(
        TraceType,
        "AddAdminClientIdentities: after: adminClientIdentities_: {0}; remoteIdentities_: {1}",
        adminClientIdentities_, remoteIdentities_);
}

SecuritySettings::IdentitySet const & SecuritySettings::RemoteIdentities() const
{
    return remoteIdentities_;
}

void SecuritySettings::AddRemoteIdentity(std::wstring const & value)
{
    remoteIdentities_.insert(value);
}

X509FindType::Enum SecuritySettings::X509FindType() const
{
    return x509FindValue_->Type();
}

SecurityProvider::Enum SecuritySettings::SecurityProvider() const
{
    return securityProvider_;
}

X509StoreLocation::Enum SecuritySettings::X509StoreLocation() const
{
    return x509StoreLocation_;
}

wstring const& SecuritySettings::X509StoreName() const
{
    return x509StoreName_;
}

X509FindValue::SPtr const& SecuritySettings::X509FindValue() const
{
    return x509FindValue_;
}

ProtectionLevel::Enum SecuritySettings::ProtectionLevel() const
{
    return protectionLevel_;
}

wstring const& SecuritySettings::RemoteSpn() const
{
    return remoteSpn_;
}

void SecuritySettings::SetRemoteSpn(std::wstring const & remoteSpn)
{
    remoteSpn_ = remoteSpn;
}

wstring const & SecuritySettings::LocalClaimToken() const
{
    return localClaimToken_;
}

ErrorCode SecuritySettings::StringToRoleClaims(std::wstring const & inputString, RoleClaims & roleClaims)
{
    // split "Claim && Claim && Claim ..."
    vector<wstring> claimVector;
    StringUtility::Split<wstring>(inputString, claimVector, L"&&", true);
    if (claimVector.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    for (wstring const & claim : claimVector)
    {
        if (!roleClaims.AddClaim(claim))
        {
            return ErrorCodeValue::InvalidArgument;
        }
    }

    return ErrorCode::Success();
}

bool SecuritySettings::ClaimListToRoleClaim(vector<ServiceModel::Claim> const& claimList, _Out_ RoleClaims &roleClaims)
{
    for (ServiceModel::Claim const& claim : claimList)
    {
        if (!roleClaims.AddClaim(claim.ClaimType, claim.Value))
        {
            return false;
        }
    }

    return true;
}

ErrorCode SecuritySettings::StringToRoleClaimsOrList(std::wstring const & inputString, RoleClaimsOrList & roleClaimsOrList)
{
    wstring inputCopy = inputString;
    StringUtility::TrimWhitespaces(inputCopy);
    if (inputCopy.empty())
    {
        return ErrorCode();
    }

    // split "RoleClaims || RoleClaims || RoleClaims ..."
    vector<wstring> roleClaimsVector;
    StringUtility::Split<wstring>(inputCopy, roleClaimsVector, L"||", true);
    if (roleClaimsVector.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    for (wstring const & roleClaimsEntry : roleClaimsVector)
    {
        RoleClaims newRoleClaimsEntry;
        auto error = StringToRoleClaims(roleClaimsEntry, newRoleClaimsEntry);
        if (!error.IsSuccess())
        {
            return error;
        }

        roleClaimsOrList.AddRoleClaims(newRoleClaimsEntry);
    }

    return ErrorCode();
}

ErrorCode SecuritySettings::EnableClaimBasedAuthOnClients(wstring const & clientClaimList, wstring const & adminClientClaimList)
{
    if (securityProvider_ != SecurityProvider::Ssl)
    {
        WriteError(TraceType, "SSL is required for claim-based client authorization: {0}", securityProvider_);

        // Claim based auth on clients requires SSL on server side
        return ErrorCodeValue::InvalidArgument;
    }

    auto error = StringToRoleClaimsOrList(clientClaimList, clientClaims_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringToRoleClaimsOrList(adminClientClaimList, adminClientClaims_);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Add admin client claims to client claims, so that callers do not have to add the same claim to both list
    error = StringToRoleClaimsOrList(adminClientClaimList, clientClaims_);
    if (!error.IsSuccess())
    {
        return error;
    }

    claimBasedClientAuthEnabled_ = true;
    isClientRoleInEffect_ = true;

    return error;
}

bool SecuritySettings::ClaimBasedClientAuthEnabled() const
{
    return claimBasedClientAuthEnabled_;
}

bool SecuritySettings::IdentitySet::operator == (IdentitySet const & rhs) const
{
    if (this->size() != rhs.size())
    {
        return false;
    }

    for (IdentitySet::const_iterator iter1 = cbegin(), iter2 = rhs.cbegin(); iter1 != cend(); ++iter1, ++iter2)
    {
        if (!StringUtility::AreEqualCaseInsensitive(*iter1, *iter2))
        {
            return false;
        }
    }

    return true;
}

bool SecuritySettings::IdentitySet::operator != (IdentitySet const & rhs) const
{
    return !(*this == rhs);
}

void SecuritySettings::IdentitySet::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    bool outputGenerated = false;
    for (auto const & identity : *this)
    {
        if (outputGenerated)
        {
            w.Write(", {0}", identity);
        }
        else
        {
            w.Write("{0}", identity);
            outputGenerated = true;
        }
    }
}

wstring SecuritySettings::IdentitySet::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

SecuritySettings::RoleClaimsOrList const & SecuritySettings::ClientClaims() const
{
    return clientClaims_;
}

SecuritySettings::RoleClaimsOrList const & SecuritySettings::AdminClientClaims() const
{
    return adminClientClaims_;
}

bool SecuritySettings::RoleClaims::AddClaim(wstring const & claim)
{
    // split "ClaimType=ClaimValue"
    vector<wstring> claimParts;
    StringUtility::Split<wstring>(claim, claimParts, L"=", true);
    if (claimParts.size() != 2)
    {
        return false;
    }

    return AddClaim(claimParts.front(), claimParts.back());
}

bool SecuritySettings::RoleClaims::AddClaim(wstring const & claimType, wstring const & claimValue)
{
    wstring type = claimType;
    StringUtility::TrimWhitespaces(type);
    wstring value = claimValue;
    StringUtility::TrimWhitespaces(value);
    if (type.empty() || value.empty())
    {
        return false;
    }

    auto iter = value_.find(type);
    if ((iter == value_.cend()) || !StringUtility::AreEqualCaseInsensitive(iter->second, value))
    {
        value_.emplace(pair<wstring, wstring>(move(type), move(value)));
    }

    return true;
}

bool SecuritySettings::RoleClaims::operator < (RoleClaims const & rhs) const
{
    return value_ < rhs.value_;
}

bool SecuritySettings::RoleClaims::operator == (RoleClaims const & rhs) const
{
    return (value_.size() == rhs.value_.size()) && this->Contains(rhs) && rhs.Contains(*this);
}

bool SecuritySettings::RoleClaims::operator != (RoleClaims const & rhs) const
{
    return !(*this == rhs);
}

bool SecuritySettings::RoleClaims::Contains(std::wstring const & claimType, std::wstring const & claimValue) const
{
    for (auto iter = value_.lower_bound(claimType); iter != value_.upper_bound(claimType); ++iter)
    {
        if (StringUtility::AreEqualCaseInsensitive(claimType, iter->first) &&
            StringUtility::AreEqualCaseInsensitive(claimValue, iter->second))
        {
            return true;
        }
    }

    return false;
}

bool SecuritySettings::RoleClaims::Contains(RoleClaims const & other) const
{
    for (auto const & claim : other.value_)
    {
        if (!Contains(claim.first, claim.second))
        {
            return false;
        }
    }

    return true;
}

bool SecuritySettings::RoleClaims::IsInRole(RoleClaimsOrList const & roleClaimsOrList) const
{
    for (auto const & requirement : roleClaimsOrList.Value())
    {
        if (this->Contains(requirement))
        {
            return true;
        }
    }

    return false;
}

multimap<wstring, wstring, IsLessCaseInsensitiveComparer<wstring>> const & SecuritySettings::RoleClaims::Value() const
{
    return value_;
}

void SecuritySettings::RoleClaims::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    bool outputGenerated = false;
    for (auto const & claim : value_)
    {
        if (outputGenerated)
        {
            w.Write(L" && ");
        }

        w.Write("{0}={1}", claim.first, claim.second);
        outputGenerated = true;
    }
}

wstring SecuritySettings::RoleClaims::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

void SecuritySettings::RoleClaimsOrList::AddRoleClaims(RoleClaims const & roleClaims)
{
    bool shouldAdd = true;
    for (auto existing = value_.cbegin(); existing != value_.cend();)
    {
        if (existing->Contains(roleClaims))
        {
            // the existing RoleClaims entry is more restrictive, thus should be replaced
            auto toDelete = existing++;
            value_.erase(toDelete);
            continue;
        }

        if (roleClaims.Contains(*existing))
        {
            // new entry is more restrictive, already covered by the existing entry
            shouldAdd = false;
        }

        ++existing;
    }

    if (shouldAdd)
    {
        value_.insert(roleClaims);
    }
}

bool SecuritySettings::RoleClaimsOrList::operator == (RoleClaimsOrList const & rhs) const
{
    return (value_.size() == rhs.value_.size()) && this->Contains(rhs) && rhs.Contains(*this);
}

bool SecuritySettings::RoleClaimsOrList::operator != (RoleClaimsOrList const & rhs) const
{
    return !(*this == rhs);
}

bool SecuritySettings::RoleClaimsOrList::Contains(RoleClaims const & roleClaimsEntry) const
{
    for (auto const & entry : value_)
    {
        if (entry == roleClaimsEntry)
        {
            return true;
        }
    }

    return false;
}

bool SecuritySettings::RoleClaimsOrList::Contains(RoleClaimsOrList const & other) const
{
    for (auto const & otherEntry : other.value_)
    {
        if (!Contains(otherEntry))
        {
            return false;
        }
    }

    return true;
}

set<SecuritySettings::RoleClaims> const & SecuritySettings::RoleClaimsOrList::Value() const
{
    return value_;
}

void SecuritySettings::RoleClaimsOrList::WriteTo(TextWriter & w, FormatOptions const &) const
{
    bool outputGenerated = false;
    for (auto const & roleClaims : value_)
    {
        if (outputGenerated)
        {
            w.Write(" || {0}", roleClaims);
        }
        else
        {
            w.Write("{0}", roleClaims);
            outputGenerated = true;
        }
    }
}

wstring SecuritySettings::RoleClaimsOrList::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

ThumbprintSet const & SecuritySettings::RemoteCertThumbprints() const
{
    return remoteCertThumbprints_;
}

ErrorCode SecuritySettings::SetRemoteCertThumbprints(std::wstring const & thumbprints)
{
    if ((securityProvider_ != SecurityProvider::Ssl) && (securityProvider_ != SecurityProvider::Claims))
    {
        WriteError("SetRemoteCertThumbprints", "thumbprint is not supported for {0}", securityProvider_);
        return ErrorCodeValue::InvalidOperation;
    }

    return remoteCertThumbprints_.Set(thumbprints);
}

ThumbprintSet const & SecuritySettings::AdminClientCertThumbprints() const
{
    return adminClientCertThumbprints_;
}

SecurityConfig::X509NameMap const & SecuritySettings::RemoteX509Names() const
{
    return remoteX509Names_;
}

SecurityConfig::IssuerStoreKeyValueMap const & SecuritySettings::RemoteCertIssuers() const
{
    return remoteCertIssuers_;
}

ErrorCode SecuritySettings::SetRemoteCertIssuerPublicKeys(Common::X509IdentitySet issuerPublicKeys)
{
    remoteX509Names_.OverwriteIssuers(issuerPublicKeys);
    return ErrorCodeValue::Success;
}

ErrorCode SecuritySettings::SetAdminClientCertIssuerPublicKeys(Common::X509IdentitySet issuerPublicKeys)
{
    adminClientX509Names_.OverwriteIssuers(issuerPublicKeys);
    return ErrorCodeValue::Success;
}

SecurityConfig::X509NameMap const & SecuritySettings::AdminClientX509Names() const
{
    return adminClientX509Names_;
}

DWORD SecuritySettings::X509CertChainFlags() const
{
    return x509CertChainFlagCallback_ ? x509CertChainFlagCallback_() : SecurityConfig::GetConfig().CrlCheckingFlag;
}

void SecuritySettings::SetX509CertChainFlagCallback(X509CertChainFlagCallback const& callback)
{
    // if X509 cert chain flag can be passed from public API, we can simply add a callback to return the flag passed in
    Invariant(!x509CertChainFlagCallback_);
    x509CertChainFlagCallback_ = callback;
}

bool SecuritySettings::FramingProtectionEnabled() const
{
#ifdef PLATFORM_UNIX
    return true;
#else
    return
        framingProtectionEnabledCallback_ ?
            framingProtectionEnabledCallback_() :
            (isRemotePeer_ ?
                SecurityConfig::GetConfig().FramingProtectionEnabledInPeerToPeerMode :
                SecurityConfig::GetConfig().FramingProtectionEnabled);
#endif
}

void SecuritySettings::SetFramingProtectionEnabledCallback(BooleanFlagCallback const & callback)
{
#ifdef PLATFORM_UNIX
    Assert::CodingError("not allowed");
#else
    framingProtectionEnabledCallback_ = callback;
#endif
}

bool SecuritySettings::ReadyNewSessionBeforeExpiration() const
{
    return
        readyNewSessionBeforeExpirationCallback_ ?
            readyNewSessionBeforeExpirationCallback_() : SecurityConfig::GetConfig().ReadyNewSessionBeforeExpiration;
}

void SecuritySettings::SetReadyNewSessionBeforeExpirationCallback(BooleanFlagCallback const & callback)
{
    readyNewSessionBeforeExpirationCallback_ = callback;
}

bool SecuritySettings::ShouldIgnoreCrlOfflineError(bool inbound) const
{
    if (crlOfflineIgnoreCallback_) return crlOfflineIgnoreCallback_(inbound);

    return inbound ? SecurityConfig::GetConfig().IgnoreCrlOfflineError : SecurityConfig::GetConfig().IgnoreSvrCrlOfflineError;
}

void SecuritySettings::SetCrlOfflineIgnoreCallback(CrlOfflineIgnoreCallback const & callback)
{
    Invariant(callback);
    crlOfflineIgnoreCallback_ = callback;
}

TimeSpan SecuritySettings::SessionDuration() const
{
    return sessionDurationCallback_? sessionDurationCallback_() : SecurityConfig::GetConfig().SessionExpiration;
}

ErrorCode SecuritySettings::UpdateRemoteIssuersIfNeeded(SecuritySettings &object)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (!object.remoteCertIssuers_.IsEmpty())
    {
        X509IdentitySet remoteCertIssuerPublicKeys;
        error = object.RemoteCertIssuers().GetPublicKeys(remoteCertIssuerPublicKeys);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "UpdateRemoteIssuersIfNeeded: remoteCertIssuers_.GetPublicKeys failed: {0}", error);
            return error;
        }
        object.SetRemoteCertIssuerPublicKeys(remoteCertIssuerPublicKeys);
    }

    return error;
}

#ifndef PLATFORM_UNIX

ErrorCode SecuritySettings::AddRemoteX509Name(PCCERT_CONTEXT certContext)
{
    wstring commonName;
    auto err = CryptoUtility::GetCertificateCommonName(certContext, commonName);
    if (!err.IsSuccess()) return err;

    X509PubKey::SPtr issuerPubKey;
    err = CryptoUtility::GetCertificateIssuerPubKey(certContext, issuerPubKey);
    if (!err.IsSuccess()) return err;

    remoteX509Names_.Add(commonName, issuerPubKey);
    return err;
}

ErrorCode SecuritySettings::AddAdminClientX509Name(PCCERT_CONTEXT certContext)
{
    wstring commonName;
    auto err = CryptoUtility::GetCertificateCommonName(certContext, commonName);
    if (!err.IsSuccess()) return err;

    X509PubKey::SPtr issuerPubKey;
    err = CryptoUtility::GetCertificateIssuerPubKey(certContext, issuerPubKey);
    if (!err.IsSuccess()) return err;

    adminClientX509Names_.Add(commonName, issuerPubKey);
   
    if (!remoteCertIssuers_.IsEmpty())
    {
        X509IdentitySet clientIssuerPublicKeys;
        err = remoteCertIssuers_.GetPublicKeys(clientIssuerPublicKeys);
        if (!err.IsSuccess()) { return err; }

        adminClientX509Names_.Add(commonName, clientIssuerPublicKeys);
    }

    remoteX509Names_.Add(adminClientX509Names_);
    return err;
}

#endif
