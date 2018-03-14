// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace
{
    const StringLiteral TraceType("SecurityCredentials");

    atomic_uint64 objCount(0);

    struct SecurityCredentialsLess
    {
        bool operator() (SecurityCredentialsSPtr const & a, SecurityCredentialsSPtr const & b) const
        {
            return a->Expiration() > b->Expiration();
        }
    };

    void TraceSortedVector(std::vector<SecurityCredentialsSPtr> const & v)
    {
        if (v.size() < 2) return;

        SecurityCredentials::WriteInfo(TraceType, "credentials sorted: {0}", SecurityCredentials::VectorsToString(v));
    }

    void SortVector(std::vector<SecurityCredentialsSPtr>* v)
    {
        if (!v) return;

        std::sort(v->begin(), v->end(), SecurityCredentialsLess());
        TraceSortedVector(*v);
    }

    void MergeSortedVector(vector<SecurityCredentialsSPtr>* s1, vector<SecurityCredentialsSPtr> const & s2)
    {
        if (!s1) return;

        vector<SecurityCredentialsSPtr> merged;
        std::merge(
            s1->begin(),
            s1->end(),
            s2.cbegin(),
            s2.cend(),
            std::back_inserter(merged),
            SecurityCredentialsLess());

        *s1 = std::move(merged);
        TraceSortedVector(*s1);
    }
}

std::wstring SecurityCredentials::VectorsToString(vector<SecurityCredentialsSPtr> const & v)
{
    wstring s;
    StringWriter sw(s);
    for (auto const & e : v)
    {
        sw.Write("({0},{1}) ", e->X509CertThumbprint(), e->Expiration());
    }

    return s;
}

SecurityCredentials::SecurityCredentials()
{
#ifdef PLATFORM_UNIX

    // SSL_library_init() is required or SSL_CTX_new() will fail
    auto forceInitSSL = LinuxCryptUtil();

    // Despite of version numbers in the name, SSLv23_method is the general-purpose version-flexible SSL/TLS methods.
    // The actual protocol version used will be negotiated to the highest version mutually supported by the client and
    // the server. https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_new.html
    // LINUXTODO consider replacing SSLv23_method with TLS_method after upgrading to openssl 1.1 or newer, to avoid confusion. 
    sslCtx_ = SSL_CTX_new(SSLv23_method());

    if (sslCtx_ == NULL)
    {
        auto err = LinuxCryptUtil().GetOpensslErr();
        Assert::CodingError("SSL_CTX_new failed: {0}:{1}", err.ReadValue(), err.Message);
    }

    auto retval = SSL_CTX_set_default_verify_paths(sslCtx_);
    if (!retval)
    {
        auto err = LinuxCryptUtil().GetOpensslErr();
        Assert::CodingError("SSL_CTX_set_default_verify_paths failed: {0}:{1}", err.ReadValue(), err.Message);
    }
#endif

    SecInvalidateHandle(&credentials_);
    auto count = ++objCount;
    WriteInfo(TraceType, "ctor: object count = {0}", count);
}

_Use_decl_annotations_
ErrorCode SecurityCredentials::AcquireWin(
    wstring const & securityPrincipal,
    SecurityProvider::Enum securityProvider,
    bool outboundOnly,
    PVOID authenticationData,
    vector<SecurityCredentialsSPtr> & credentialsList)
{
    credentialsList.clear();

#ifdef PLATFORM_UNIX
    Assert::CodingError("Not implemented");
    return ErrorCode();
#else
    auto credentials =  SecurityCredentialsSPtr(new SecurityCredentials());
    wchar_t * principal = securityPrincipal.empty() ? nullptr : const_cast<wchar_t *>(securityPrincipal.c_str());
    wchar_t * package = SecurityProvider::GetSspiProviderName(securityProvider);
    ULONG credentialUse = outboundOnly ? SECPKG_CRED_OUTBOUND : SECPKG_CRED_BOTH;

    TimeStamp expiration;
    SECURITY_STATUS status = ::AcquireCredentialsHandle(
        principal,
        package,
        credentialUse,
        nullptr,
        authenticationData,
        nullptr,
        nullptr,
        &credentials->credentials_,
        &expiration);

    if (status != SEC_E_OK)
    {
        credentials.reset();
        SecurityCredentials::WriteError(
            TraceType,
            "AcquireCredentialsHandle({0}) failed on principal \"{1}\": 0x{2:x}",
            securityProvider,
            securityPrincipal,
            (uint)status);

        return ErrorCode::FromHResult(status);
    }

    auto err = LocalTimeStampToDateTime(expiration, credentials->expiration_);
    if (err.IsSuccess())
    {
        credentialsList.emplace_back(move(credentials));
    }

    return err;
#endif
}

_Use_decl_annotations_ ErrorCode SecurityCredentials::AcquireSsl(
    CertContextUPtr const & certContext,
    ULONG credentialUse,
    SecurityCredentialsSPtr & credentials)
{
    auto pCertContext = certContext.get();
    credentials = SecurityCredentialsSPtr(new SecurityCredentials());
    ErrorCode error;

#ifdef PLATFORM_UNIX

    if (!certContext) return error;

    LinuxCryptUtil cryptUtil;
    error = cryptUtil.ReadPrivateKey(certContext, credentials->credentials_);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, "AcquireSsl: failed to read private key: {0}", error);
        return error;
    }

    auto retval = SSL_CTX_use_certificate(credentials->sslCtx_, pCertContext);
    if (retval != 1)
    {
        error = cryptUtil.GetOpensslErr();
        WriteError(TraceType, "AcquireSsl: SSL_CTX_use_certificate failed: {0}:{1}", error, error.Message);
        return error;
    }

//uncomment if needs to load issuer certificates from custom location
//    vector<CertContextUPtr> issuerChain;
//    error = cryptUtil.LoadIssuerChain(pCertContext, issuerChain);
//    if (!error.IsSuccess())
//    {
//        return error;
//    }
//
//    for(auto & issuer : issuerChain)
//    {
//        WriteInfo(TraceType, "AcquireSsl: SSL_CTX_add_extra_chain_cert"); 
//        auto retval = SSL_CTX_add_extra_chain_cert(credentials->sslCtx_, X509Untrack(issuer));
//        if (!retval)
//        {
//            error = cryptUtil.GetOpensslErr();
//            WriteError(TraceType, "AcquireSsl: SSL_CTX_add_extra_chain_cert failed: {0}:{1}", error, error.Message);
//        }
//    }

    retval = SSL_CTX_use_PrivateKey(credentials->sslCtx_, credentials->credentials_.get()); 
    if (retval != 1)
    {
        error = cryptUtil.GetOpensslErr();
        WriteError(TraceType, "AcquireSsl: SSL_CTX_use_PrivateKey failed: {0}:{1}", error, error.Message);
        return error;
    }

    retval = SSL_CTX_check_private_key(credentials->sslCtx_);
    if(retval != 1)
    {
        error = cryptUtil.GetOpensslErr();
        WriteError(TraceType, "AcquireSsl: SSL_CTX_check_private_key failed: {0}:{1}", error, error.Message);
        return error;
    }

    cryptUtil.GetCertificateNotAfterDate(pCertContext, credentials->expiration_);

#else

    SCHANNEL_CRED sslCredentials = {};
    sslCredentials.dwVersion = SCHANNEL_CRED_VERSION;
    // Disable automatic certificate checking since we explicitly verify client/server certificates.
    // May need to revisit this when we need to map certificate to Windows security principal
    sslCredentials.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION;

    if (pCertContext)
    {
        sslCredentials.cCreds = 1;
        sslCredentials.paCred = &pCertContext;
    }
    else
    {
        SecurityCredentials::WriteInfo(TraceType, "Null client certificate");
        sslCredentials.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION;
    }

    TimeStamp expiration;
    SECURITY_STATUS status = ::AcquireCredentialsHandle(
        nullptr,
        UNISP_NAME,
        credentialUse,
        nullptr,
        &sslCredentials,
        nullptr,
        nullptr,
        &credentials->credentials_,
        &expiration);

    if (status != SEC_E_OK)
    {
        WriteError(TraceType, "AcquireCredentialsHandle({0}) failed: 0x{1:x}", UNISP_NAME, (uint)status);
        error = ErrorCode::FromHResult(status);
        return error;
    }

    credentials->expiration_ = DateTime((FILETIME const &)expiration); // Schannel returns expiration in UTC

#endif

    if (!pCertContext) return error;

    error = credentials->x509CertThumbprint_.Initialize(pCertContext);
    if (!error.IsSuccess()) return error;

    if (!SecurityConfig::GetConfig().SkipX509CredentialExpirationChecking &&
        credentials->expiration_ <= DateTime::Now())
    {

        SecurityCredentials::WriteInfo(TraceType, "certificate credential {0} expired at {1}, will be skipped", credentials->x509CertThumbprint_, credentials->expiration_);
        credentials.reset();
        return ErrorCodeValue::InvalidCredentials;
    }

    SecurityCredentials::WriteInfo(
        TraceType,
        "certificate credential {0} loaded successfully",
        credentials->x509CertThumbprint_);

    credentials->x509PubKey_ = make_unique<X509PubKey>(pCertContext);
    SecurityCredentials::WriteInfo(TraceType, "local X509 public key: {0}", *(credentials->x509PubKey_));

    return error;
}

_Use_decl_annotations_
ErrorCode SecurityCredentials::AcquireAnonymousSslClient(vector<SecurityCredentialsSPtr> & credentials)
{
    SecurityCredentialsSPtr cred;
    auto err = AcquireSsl(nullptr, SECPKG_CRED_OUTBOUND, cred);
    if (err.IsSuccess())
    {
        credentials.emplace_back(move(cred));
    }

    return err;
}

bool SecurityCredentials::SortedVectorsEqual(vector<SecurityCredentialsSPtr> const & a, vector<SecurityCredentialsSPtr> const & b)
{
    return std::equal(
        a.cbegin(),
        a.cend(),
        b.cbegin(),
        b.cend(),
        [](SecurityCredentialsSPtr const& l, SecurityCredentialsSPtr const& r) { return l->X509CertThumbprint() == r->X509CertThumbprint(); });
}

_Use_decl_annotations_ ErrorCode SecurityCredentials::AcquireSsl(
    CertContextUPtr const & certContext,
    vector<SecurityCredentialsSPtr> * credentials,
    vector<SecurityCredentialsSPtr> * svrCredentials)
{
    Thumbprint thumbprint;
    auto error = thumbprint.Initialize(certContext.get());
    if (!error.IsSuccess()) { return error; }

    if (credentials)
    {
        SecurityCredentialsSPtr cred;
        error = AcquireSsl(certContext, SECPKG_CRED_OUTBOUND, cred);
        if (!error.IsSuccess()) { return error; }
        cred->x509CertThumbprint_ = thumbprint;
        credentials->emplace_back(move(cred));
    }

    if (svrCredentials)
    {
        SecurityCredentialsSPtr cred;
        error = AcquireSsl(certContext, SECPKG_CRED_INBOUND, cred);
        if (!error.IsSuccess()) { return error; }
        cred->x509CertThumbprint_ = thumbprint;
        svrCredentials->emplace_back(move(cred));
    }

    return error;
 }

_Use_decl_annotations_ ErrorCode SecurityCredentials::AcquireSsl(
    X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<X509FindValue> const & findValue,
    vector<SecurityCredentialsSPtr> * credentials,
    vector<SecurityCredentialsSPtr> * svrCredentials)
{
    Invariant(credentials || svrCredentials);
    if (credentials) credentials->clear();
    if (svrCredentials) svrCredentials->clear();

    CertContexts certs;
    ErrorCode error = CryptoUtility::GetCertificate(
        certStoreLocation,
        certStoreName,
        findValue,
        certs);

    if (error.IsSuccess())
    {
        // certificates are sorted according to expiration in descending order, if there are more than one matches
        for (auto const & cert : certs)
        {
#ifdef PLATFORM_UNIX
            SecurityCredentialsSPtr cred;
            error = AcquireSsl(cert, SECPKG_CRED_OUTBOUND, cred);
            if (!error.IsSuccess()) continue;

            if (credentials) credentials->emplace_back(cred);
            if (svrCredentials) svrCredentials->emplace_back(cred);
#else
            if (credentials)
            {
                SecurityCredentialsSPtr cred;
                error = AcquireSsl(cert, SECPKG_CRED_OUTBOUND, cred);
                if (!error.IsSuccess()) continue;
                credentials->emplace_back(move(cred));
            }

            if (svrCredentials)
            {
                SecurityCredentialsSPtr cred;
                error = AcquireSsl(cert, SECPKG_CRED_INBOUND, cred);
                if (!error.IsSuccess()) continue;
                svrCredentials->emplace_back(move(cred));
            }
#endif
        }
    }

    bool credentialLoaded = ((credentials && !credentials->empty()) || (svrCredentials && !svrCredentials->empty()));
    if (credentialLoaded) error.Reset();

    SortVector(credentials);
    SortVector(svrCredentials);

    if (!findValue || !findValue->Secondary())
    {
        return error;
    }

    if (error.IsSuccess() && !SecurityConfig::GetConfig().UseSecondaryIfNewer)
    {
        return error;
    }

    if (!error.IsSuccess())
    {
        // copmletely rely on secondary as primary load failed
        return AcquireSsl(
            certStoreLocation,
            certStoreName,
            findValue->Secondary(),
            credentials,
            svrCredentials);
    }

    vector<SecurityCredentialsSPtr> credentials2;
    vector<SecurityCredentialsSPtr> svrCredentials2;
    auto secondaryLoaded = AcquireSsl(
        certStoreLocation,
        certStoreName,
        findValue->Secondary(),
        credentials ? (&credentials2) : nullptr,
        svrCredentials ? (&svrCredentials2) : nullptr);

    if (secondaryLoaded.IsSuccess())
    {
        MergeSortedVector(credentials, credentials2);
        MergeSortedVector(svrCredentials, svrCredentials2);
    }

    return error;
}

SecurityCredentials::~SecurityCredentials()
{
#ifdef PLATFORM_UNIX
    if (sslCtx_)
    {
        SSL_CTX_free(sslCtx_);
    }
#else
    if (this->IsValid())
    {
        FreeCredentialsHandle(&credentials_);
    }
#endif
    auto count = --objCount;
    WriteInfo(TraceType, "dtor: object count = {0}", count);
}

uint64 SecurityCredentials::ObjCount()
{
    return objCount.load();
}

PCredHandle SecurityCredentials::Credentials()
{
    return &credentials_;
}

DateTime SecurityCredentials::Expiration() const
{
    return expiration_;
}

Thumbprint const & SecurityCredentials::X509CertThumbprint() const
{
    return x509CertThumbprint_;
}

#ifdef PLATFORM_UNIX

SSL_CTX* SecurityCredentials::SslCtx() const
{
    return sslCtx_;
}

#else

bool SecurityCredentials::IsValid() const
{
    return SecIsValidHandle(&this->credentials_);
}

_Use_decl_annotations_
ErrorCode SecurityCredentials::LocalTimeStampToDateTime(TimeStamp localTimestamp, DateTime & dateTime)
{
    FILETIME filetime;
    if (::LocalFileTimeToFileTime((FILETIME const*)(&localTimestamp), &filetime) == 0)
    {
        dateTime = DateTime::Zero;
        return ErrorCode::FromWin32Error();
    }

    dateTime = DateTime(filetime);
    return ErrorCode();
}

#endif
