// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

StringLiteral const TraceHttpEndpointSecurityProvider("HttpEndpointSecurityProvider");

HttpEndpointSecurityProvider::HttpEndpointSecurityProvider(
    ComponentRoot const & root)
    : RootedObject(root),
    FabricComponent(),
    httpEngine_(),
    httpEngineLock_(),
    portAclMap_()
{
}

HttpEndpointSecurityProvider::~HttpEndpointSecurityProvider()
{
}

ErrorCode HttpEndpointSecurityProvider::OnOpen()
{    
    WriteNoise(TraceHttpEndpointSecurityProvider, Root.TraceId, "Opening HttpEndpointSecurityProvider");    
    
    portAclMap_ = make_unique<PortAclMap>();

    auto error = InitializeHttpEngine();
    if (!error.IsSuccess())
    { 
        return error; 
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode HttpEndpointSecurityProvider::InitializeHttpEngine()
{
//LINUXTUDO
#if !defined(PLATFORM_UNIX)
    // initialize httpEngine_
    HTTPAPI_VERSION version = HTTPAPI_VERSION_1;
    auto win32Error = ::HttpInitialize(version, HTTP_INITIALIZE_CONFIG, NULL);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Hosting,
            TraceHttpEndpointSecurityProvider,
            Root.TraceId,
            "HttpInitialize");
    }

    {
        AcquireWriteLock lock(this->httpEngineLock_);
        this->httpEngine_ = make_shared<ResourceHolder<PVOID>>(
            nullptr,
            [] (ResourceHolder<PVOID> *) -> void
        {
            auto win32Error = ::HttpTerminate(HTTP_INITIALIZE_CONFIG, NULL);
            if (win32Error != ERROR_SUCCESS)
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "HttpTerminate failed. ErrorCode={0}",
                    ErrorCode::FromWin32Error(win32Error));            
            }
        });
    }
#endif
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode HttpEndpointSecurityProvider::OnClose()
{
    WriteNoise(TraceHttpEndpointSecurityProvider, Root.TraceId, "Closing HttpEndpointSecurityProvider");
    CleanupHttpEngine();

    return ErrorCode(ErrorCodeValue::Success);
}

void HttpEndpointSecurityProvider::OnAbort()
{
    WriteNoise(TraceHttpEndpointSecurityProvider, Root.TraceId, "Aborting HttpEndpointSecurityProvider.");
    CleanupHttpEngine();
}

void HttpEndpointSecurityProvider::CleanupHttpEngine()
{
    {
        AcquireWriteLock writeLock(this->httpEngineLock_);
        if (this->httpEngine_)
        {
            this->httpEngine_.reset();
        }
    }
}

ErrorCode HttpEndpointSecurityProvider::ConfigurePortAcls(
    wstring const & principalSid,
    UINT port, 
    bool isHttps,
    wstring const & additionalPrefix,
    wstring const & servicePackageId,
    wstring const & nodeId,
    bool isExplicitPort)
{
#if !defined(PLATFORM_UNIX)
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceHttpEndpointSecurityProvider,
            Root.TraceId,
            "Http port '{0}' cannot be ACL'd when endpoint provider is not opened.", port);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    wstring sddl;
    auto error = this->GetSddl(principalSid, sddl);
    if (!error.IsSuccess()) { return error; }
    
    vector<wstring> prefixes;
    prefixes.push_back(L"+");
    if (!additionalPrefix.empty())
    {
        prefixes.push_back(additionalPrefix);
    }
    if (isExplicitPort)
    {
        error = this->SetupSharedPortAcls(
            servicePackageId,
            nodeId,
            sddl,
            principalSid,
            port,
            isHttps,
            prefixes);
    }
    else
    {
        error = this->RegisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
    }
    if(!error.IsSuccess())
    {
        return error;
    }
#endif    
    return ErrorCode(ErrorCodeValue::Success);
}

#if !defined(PLATFORM_UNIX)
// Registers http endpoints with Http.sys. Please note that http.sys behaviors when it comes to url reservations
// If we reserve url http://<ipv4-adapter1>:10010/ for security principal A then 
// http.sys would prevent security principal B from opening http://<ipv4-adapter1>:10010/
// but it would still allow it to open http://<ipv4-adapter2>:10010/ or http://<localhost>:10010/
// Http.sys ensures isolation and requires one to use the strong prefix or weak prefix schemes for rules which are to be applied across adapters
// We dont use strong or weak prefix here since that requires addresses to be registered with listeners with strong and weak prefixes as well
// Also strong prefix will now work on azure since a service is only permitted to listen on port for a specific adapter. Azure does not permit 
// services to listen on all adapters
// for viewing ACLs one can use netsh http show urlacls
ErrorCode HttpEndpointSecurityProvider::RegisterEndpointWithHttpEngine(
    wstring const & sddl,
    UINT port,
    bool isHttps,
    vector<wstring> const & prefixes)
{
    AcquireReadLock lock(this->httpEngineLock_);
    if (this->httpEngine_ == nullptr) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

    WriteInfo(
        TraceHttpEndpointSecurityProvider,
        Root.TraceId,
        "RegisterEndpointWithHttpEngine: port={0}, Https={1}, sddl {2}",
        port,
        isHttps,
        sddl);

    // If the admin process did not cleanup previously,  the acl lingers and should be removed before adding a new one.
    // for local users,  since the sid cannot be known upfront,  we always delete the old acl if it exists.  It is upto 
    // the admin process to ensure that there is no clash of acls. Delete the acl for both http and https, since we wouldnt
    // know which config might be left behind by the pervious run.
    for (auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
    {

        wstring httpEndpoint = GetHttpEndpoint(port, *iter, !isHttps);

        HTTP_SERVICE_CONFIG_URLACL_SET configInfo = { 0 };
        configInfo.KeyDesc.pUrlPrefix = &httpEndpoint[0];

        auto win32Error = ::HttpDeleteServiceConfiguration(NULL, HttpServiceConfigUrlAclInfo, &configInfo, sizeof(HTTP_SERVICE_CONFIG_URLACL_SET), NULL);
        if (win32Error != NOERROR && win32Error != ERROR_FILE_NOT_FOUND)
        {
            WriteInfo(
                TraceHttpEndpointSecurityProvider,
                Root.TraceId,
                "HttpDeleteServiceConfiguration failed for URL ACL {0}. Win32Error={1}",
                httpEndpoint,
                win32Error);
        }

        httpEndpoint = GetHttpEndpoint(port, *iter, isHttps);
        configInfo.KeyDesc.pUrlPrefix = &httpEndpoint[0];

        win32Error = ::HttpDeleteServiceConfiguration(NULL, HttpServiceConfigUrlAclInfo, &configInfo, sizeof(HTTP_SERVICE_CONFIG_URLACL_SET), NULL);
        if (win32Error != NOERROR && win32Error != ERROR_FILE_NOT_FOUND)
        {
            WriteInfo(
                TraceHttpEndpointSecurityProvider,
                Root.TraceId,
                "HttpDeleteServiceConfiguration failed for URL ACL {0}. Win32Error={1}",
                httpEndpoint,
                win32Error);
        }

        configInfo.ParamDesc.pStringSecurityDescriptor = const_cast<WCHAR*>(&sddl[0]);

        win32Error = ::HttpSetServiceConfiguration(NULL, HttpServiceConfigUrlAclInfo, &configInfo, sizeof(HTTP_SERVICE_CONFIG_URLACL_SET), NULL);
        if (win32Error != NOERROR)
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(win32Error),
                TraceTaskCodes::Hosting,
                TraceHttpEndpointSecurityProvider,
                Root.TraceId,
                "HttpSetServiceConfiguration");
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}
#endif

ErrorCode HttpEndpointSecurityProvider::CleanupPortAcls(
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    wstring const & additionalPrefix,
    wstring const & servicePackageId,
    wstring const & nodeId,
    bool isExplicitPort)
{
#if !defined(PLATFORM_UNIX)
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceHttpEndpointSecurityProvider,
            Root.TraceId,
            "Http port ACL for port '{0}' cannot be removed when endpoint provider is not opened.", port);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    wstring sddl;
    auto error = this->GetSddl(principalSid, sddl);
    if (!error.IsSuccess()) { return error; }

    vector<wstring> prefixes;
    prefixes.push_back(L"+");
    if (!additionalPrefix.empty())
    {
        prefixes.push_back(additionalPrefix);
    }
    if (isExplicitPort)
    {
        error = CleanupSharedPortAcls(
            servicePackageId,
            nodeId,
            sddl,
            principalSid,
            port,
            isHttps,
            prefixes);
    }
    else
    {
        error = this->UnregisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
    }
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound)) { return error; }
#endif    
    return ErrorCode(ErrorCodeValue::Success);
}

#if !defined(PLATFORM_UNIX)
ErrorCode HttpEndpointSecurityProvider::UnregisterEndpointWithHttpEngine(
    wstring const & sddl,
    UINT port,
    bool isHttps,
    vector<wstring> const & prefixes)
{
    AcquireReadLock lock(this->httpEngineLock_);
    if (this->httpEngine_ == nullptr) { return ErrorCode(ErrorCodeValue::ObjectClosed); }


    WriteNoise(
        TraceHttpEndpointSecurityProvider,
        Root.TraceId,
        "UnregisterEndpointWithHttpEngine: port={0}, Https={1}, sddl {2}",
        port,
        isHttps,
        sddl);

    for (auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
    {
        wstring httpEndpoint = GetHttpEndpoint(port, *iter, isHttps);

        HTTP_SERVICE_CONFIG_URLACL_SET configInfo = { 0 };
        configInfo.KeyDesc.pUrlPrefix = &httpEndpoint[0];
        configInfo.ParamDesc.pStringSecurityDescriptor = const_cast<WCHAR*>(&sddl[0]);

        auto win32Error = HttpDeleteServiceConfiguration(NULL, HttpServiceConfigUrlAclInfo, &configInfo, sizeof(HTTP_SERVICE_CONFIG_URLACL_SET), NULL);
        if (win32Error != NOERROR && win32Error != ERROR_FILE_NOT_FOUND)
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(win32Error),
                TraceTaskCodes::Hosting,
                TraceHttpEndpointSecurityProvider,
                Root.TraceId,
                "HttpDeleteServiceConfiguration");
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}
#endif

ErrorCode HttpEndpointSecurityProvider::ConfigurePortCertificate(
    UINT port,
    wstring const & principalSid,
    bool isSharedPort,
    bool cleanup,
    wstring const& sslCertificateFindValue,
    wstring const& sslCertStoreName,
    X509FindType::Enum sslCertificateFindType,
    wstring const & servicePackageId,
    wstring const & nodeId)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (isSharedPort)
    {
        PortCertRefSPtr portRef;
        
        if (cleanup)
        {
            error = this->portAclMap_->GetCertRef(
                port,
                principalSid,
                sslCertificateFindValue,
                sslCertStoreName,
                sslCertificateFindType,
                portRef);
            if (error.IsSuccess() &&
                portRef != nullptr)
            {
                error = portRef->RemovePortCertIfNeeded(
                    nodeId,
                    servicePackageId,
                    [this](UINT port, wstring const & x509FindValue, wstring const & x509StoreName, Common::X509FindType::Enum x509FindType, std::wstring const &) {
                    return this->CleanupPortCertificate(port, x509FindValue, x509StoreName, x509FindType);
                });

                if (portRef->IsClosed)
                {
                    auto err = portAclMap_->RemoveCertRef(port);
                    WriteTrace(
                        err.ToLogLevel(),
                        TraceHttpEndpointSecurityProvider,
                        Root.TraceId,
                        "RemoveCertRef for httpsPort={0}, certStoreName {1}, certfindvalue {2}, error {3}",
                        port,
                        sslCertStoreName,
                        sslCertificateFindValue,
                        err);
                }
            }
            else
            {
                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    return ErrorCodeValue::Success;
                }
            }
        }
        else
        {
            error = this->portAclMap_->GetOrAddCertRef(
                port,
                principalSid,
                sslCertificateFindValue,
                sslCertStoreName,
                sslCertificateFindType, 
                portRef);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    Root.TraceId,
                    "ConfigurePortCertificate: httpsPort={0}, certStoreName {1}, certfindvalue {2}, error {3}",
                    port,
                    sslCertStoreName,
                    sslCertificateFindValue,
                    error);
                return error;
            }

            error = portRef->ConfigurePortCertIfNeeded(
                nodeId,
                servicePackageId,
                [this](UINT port, wstring const & x509FindValue, wstring const & x509StoreName, Common::X509FindType::Enum x509FindType, wstring const &) {
                return this->ConfigurePortCertificate(port, x509FindValue, x509StoreName, x509FindType);
            });
            //If we fail to configure endpoint cert, remove cert from map.
            if (portRef->IsClosed)
            {
                portAclMap_->RemoveCertRef(port);
            }
            
        }
    }
    else
    {
        if (cleanup)
        {
            error = this->CleanupPortCertificate(port, sslCertificateFindValue, sslCertStoreName, sslCertificateFindType);
        }
        else
        {
            error = this->ConfigurePortCertificate(port, sslCertificateFindValue, sslCertStoreName, sslCertificateFindType);
        }
    }
    return error;
}

ErrorCode HttpEndpointSecurityProvider::ConfigurePortCertificate(
    UINT port,
    wstring const& sslCertificateFindValue,
    wstring const& sslCertStoreName,
    X509FindType::Enum sslCertificateFindType)
{
//LINUXTUDO
#if defined(PLATFORM_UNIX)
    return ErrorCode(ErrorCodeValue::Success);
#else
    AcquireReadLock lock(this->httpEngineLock_);
    if (this->httpEngine_ == nullptr) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

    WriteInfo(
        TraceHttpEndpointSecurityProvider,
        Root.TraceId,
        "ConfigurePortCertificate: httpsPort={0}, certStoreName {1}, thumbprint {2}",
        port,
        sslCertStoreName,
        sslCertificateFindValue);


    return CertificateConfigurationHelper(
        true,
        port,
        sslCertificateFindValue,
        sslCertStoreName,
        sslCertificateFindType);
#endif        
}

ErrorCode HttpEndpointSecurityProvider::CleanupPortCertificate(
    UINT port,
    wstring const& sslCertificateFindValue,
    wstring const& sslCertStoreName,
    X509FindType::Enum sslCertificateFindType)
{
//LINUXTUDO
#if defined(PLATFORM_UNIX)
    return ErrorCode(ErrorCodeValue::Success);
#else
    AcquireReadLock lock(this->httpEngineLock_);
    if (this->httpEngine_ == nullptr) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

    WriteInfo(
        TraceHttpEndpointSecurityProvider,
        Root.TraceId,
        "CleanupPortCertificate: httpsPort={0}, certStoreName {1}, thumbprint {2}",
        port,
        sslCertStoreName,
        sslCertificateFindValue);

    return CertificateConfigurationHelper(
        false,
        port,
        sslCertificateFindValue,
        sslCertStoreName,
        sslCertificateFindType);
#endif        
}

ErrorCode HttpEndpointSecurityProvider::CertificateConfigurationHelper(
    bool setup,
    UINT port,
    wstring const& sslCertificateFindValue,
    wstring const& sslCertStoreName,
    X509FindType::Enum sslCertificateFindType)
{
//LINUXTUDO
#if !defined(PLATFORM_UNIX)
    // If a certificate thumbprint/subjectname is specified, configure/cleanup the ssl settings.
    if (!sslCertificateFindValue.empty())
    {
        Thumbprint thumbprint;
        if (sslCertificateFindType == X509FindType::FindByThumbprint)
        {
            auto error = thumbprint.Initialize(sslCertificateFindValue);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "Invalid thumbprint {0} - {1}",
                    sslCertificateFindValue,
                    error);

                return ErrorCodeValue::InvalidX509Thumbprint;
            }
        }
        else
        {
            TESTASSERT_IF(sslCertificateFindType != X509FindType::FindBySubjectName);
            shared_ptr<X509FindValue> subjectName;
            auto error = X509FindValue::Create(sslCertificateFindType, sslCertificateFindValue, subjectName);
            if (!error.IsSuccess())
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "Invalid X509 find value {0} - {1}",
                    sslCertificateFindValue,
                    error);

                return error;
            }

            CertContextUPtr certContext;
            error = CryptoUtility::GetCertificate(
                X509Default::StoreLocation(),
                sslCertStoreName,
                subjectName,
                certContext);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "GetCertificate failed: {0}",
                    error);

                return error;
            }

            error = thumbprint.Initialize(certContext.get());
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "Failed to get certificate thumbprint: {0}",
                    error);

                return error;
            }
        }


        HTTP_SERVICE_CONFIG_SSL_SET sslConfig = {0};
        Endpoint httpEndPoint(L"0.0.0.0", port);
        Guid appId = Guid::NewGuid();

        sslConfig.KeyDesc.pIpPort = const_cast<sockaddr*>(httpEndPoint.AsSockAddr);
        sslConfig.ParamDesc.pSslHash = ((PCRYPT_HASH_BLOB)thumbprint.Value())->pbData;
        sslConfig.ParamDesc.SslHashLength = ((PCRYPT_HASH_BLOB)thumbprint.Value())->cbData;
        sslConfig.ParamDesc.pSslCertStoreName = const_cast<LPWSTR>(sslCertStoreName.data());

        // Client certificate revocation checking policies are managed by the httpgateway. So
        // no need to do client certificate revocation here.
        sslConfig.ParamDesc.DefaultCertCheckMode = 1;
        sslConfig.ParamDesc.AppId = appId.AsGUID();

        auto win32Error = HttpDeleteServiceConfiguration(
            NULL,
            HttpServiceConfigSSLCertInfo,
            &sslConfig,
            sizeof(HTTP_SERVICE_CONFIG_SSL_SET),
            NULL);

        if (win32Error != NOERROR && win32Error != ERROR_FILE_NOT_FOUND)
        {
            WriteNoise(
                TraceHttpEndpointSecurityProvider,
                Root.TraceId,
                "HttpDeleteServiceConfiguration failed for SSL CONFIG. Win32Error={0}",
                win32Error);
        }

        if (setup)
        {
            win32Error = HttpSetServiceConfiguration(
                NULL,
                HttpServiceConfigSSLCertInfo,
                &sslConfig,
                sizeof(HTTP_SERVICE_CONFIG_SSL_SET),
                NULL);

            if (win32Error != NOERROR)
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(win32Error),
                    TraceTaskCodes::Hosting,
                    TraceHttpEndpointSecurityProvider,
                    Root.TraceId,
                    "HttpSetServiceConfiguration");
            }
        }
    }
#endif
    return ErrorCode::Success();
}

ErrorCode HttpEndpointSecurityProvider::GetSddl(
    wstring const & principalSid, 
    __out wstring & sddl)
{
    WriteNoise(
        TraceHttpEndpointSecurityProvider,
        "GetSddl for principalSid={0}.",
        principalSid);

    SidSPtr sid;
    auto error = BufferedSid::CreateSPtrFromStringSid(principalSid, sid);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceHttpEndpointSecurityProvider,
            "Failed to get Sid for principalSid {0}",
            principalSid);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    error = sid->GetAllowDacl(sddl);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceHttpEndpointSecurityProvider,
            "Failed to create SDDL for principalSid {0}. ErrorCode={1}",
            principalSid,
            error);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

wstring HttpEndpointSecurityProvider::GetHttpEndpoint(UINT port, wstring const & prefix, bool isHttps)
{
    wstringstream formatstream;

    if (isHttps)
    {
        formatstream << L"https://"  << prefix + L":" << port << L"/";
    }
    else
    {
        formatstream << L"http://" << prefix + L":" << port << L"/";
    }

    return formatstream.str();
}

#if !defined(PLATFORM_UNIX)
ErrorCode HttpEndpointSecurityProvider::SetupSharedPortAcls(
    std::wstring const & servicePackageId,
    std::wstring const & nodeId,
    std::wstring const & sddl,
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    std::vector<std::wstring> const & prefixes)
{
    PortAclRefSPtr portAclRef;
    auto error = this->portAclMap_->GetOrAdd(
        principalSid,
        port,
        isHttps,
        prefixes,
        portAclRef);
    if (error.IsSuccess())
    {
        error = portAclRef->ConfigurePortAclsIfNeeded(
            nodeId,
            servicePackageId,
            sddl,
            [this](wstring const & sddl, UINT port, bool isHttps, vector<wstring> const & prefixes) {
            return this->RegisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
        });

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceHttpEndpointSecurityProvider,
                "Failed to configure ACL for port {0} principalSid {1}. ErrorCode={2}",
                port,
                principalSid,
                error);
            auto err = portAclRef->RemovePortAclsIfNeeded(
                nodeId,
                servicePackageId,
                sddl,
                [this](wstring const & sddl, UINT port, bool isHttps, vector<wstring> const & prefixes) {
                return this->UnregisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
            });
            if (!err.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "Failed to cleanup ACL for port {0} principalSid {1}. ErrorCode={2}",
                    port,
                    principalSid,
                    error);
            }
            if (portAclRef->IsClosed)
            {
                error = portAclMap_->Remove(port);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceHttpEndpointSecurityProvider,
                        "Failed to remove port entry from map for port {0}. ErrorCode={1}",
                        port,
                        error);
                }
            }
        }
    }
    return error;
}

ErrorCode HttpEndpointSecurityProvider::CleanupSharedPortAcls(
    std::wstring const & servicePackageId,
    std::wstring const & nodeId,
    std::wstring const & sddl,
    std::wstring const & principalSid,
    UINT port,
    bool,
    std::vector<std::wstring> const &)
{
    PortAclRefSPtr portAclRef;
    auto error = portAclMap_->Find(principalSid, port, portAclRef);
    if (error.IsSuccess())
    {
        error = portAclRef->RemovePortAclsIfNeeded(
            nodeId,
            servicePackageId,
            sddl,
            [this](wstring const & sddl, UINT port, bool isHttps, vector<wstring> const & prefixes) {
            return this->UnregisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
        });
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceHttpEndpointSecurityProvider,
                "Failed to remove ACL for port {0} principalSid {1}. ErrorCode={2}",
                port,
                principalSid,
                error);
        }
        if (portAclRef->IsClosed)
        {
            error = portAclMap_->Remove(port);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceHttpEndpointSecurityProvider,
                    "Failed to remove port entry from map for port {0}. ErrorCode={1}",
                    port,
                    error);
            }
        }
    }
    return error;
}
#endif

void HttpEndpointSecurityProvider::CleanupAllForNode(wstring const & nodeId)
{
//LINUXTUDO
#if !defined(PLATFORM_UNIX)
    vector<PortAclRefSPtr> portAcls;
    vector<PortCertRefSPtr> portCertRefs;
    portAclMap_->GetAllPortRefs(portAcls, portCertRefs);
    for (auto it = portAcls.begin(); it != portAcls.end(); it++)
    {
        auto portAclRef = *it;
        wstring sddl;
        auto error = this->GetSddl(portAclRef->PrincipalSid, sddl);
        WriteTrace(
            error.ToLogLevel(),
            TraceHttpEndpointSecurityProvider,
            "Failed to get Sddl for port {0}, principalSid {1}, error {2}",
            portAclRef->Port,
            portAclRef->PrincipalSid,
            error);

        error = portAclRef->RemoveAllForNode(
            nodeId,
            sddl,
            [this](wstring const & sddl, UINT port, bool isHttps, vector<wstring> const & prefixes) {
            return this->UnregisterEndpointWithHttpEngine(sddl, port, isHttps, prefixes);
        });

        WriteTrace(
            error.ToLogLevel(),
            TraceHttpEndpointSecurityProvider,
            "Failed to remove remove portaclref port {0}, node {1}, error {2}",
            portAclRef->Port,
            nodeId,
            error);

        if (portAclRef->IsClosed)
        {
            portAclMap_->Remove(portAclRef->Port);
        }
    }

    for (auto it = portCertRefs.begin(); it != portCertRefs.end(); it++)
    {
       auto portCertRef = *it;

       auto error = portCertRef->RemoveAllForNode(
            nodeId,
            [this](UINT port, wstring const & x509FindValue, wstring const & x509StoreName, Common::X509FindType::Enum x509FindType, std::wstring const &) {
            return this->CleanupPortCertificate(port, x509FindValue, x509StoreName, x509FindType).ReadValue(); });

        WriteTrace(
            error.ToLogLevel(),
            TraceHttpEndpointSecurityProvider,
            "Failed to remove remove portcertref port {0}, node {1}, error {2}",
            portCertRef->Port,
            nodeId,
            error);

        if (portCertRef->IsClosed)
        {
            portAclMap_->Remove(portCertRef->Port);
        }
    }

#endif
}
