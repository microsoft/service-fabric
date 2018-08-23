// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)

#include "Common/CryptoUtility.Linux.h"
#include <src/Transport/TransportSecurity.Linux.h>

#endif

using namespace Common;
using namespace HttpGateway;
using namespace Transport;
using namespace HttpServer;

StringLiteral const TraceType("HttpCertificateAuthHandler");

void HttpCertificateAuthHandler::OnCheckAccess(AsyncOperationSPtr const& operation)
{
    auto thisOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);

	thisOperation->request_.BeginGetClientCertificate(
        thisOperation->RemainingTime,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnCertificateReadComplete(operation);
        },
        operation);    
}

void HttpCertificateAuthHandler::OnCertificateReadComplete(AsyncOperationSPtr const& operation)
{
	auto accessCheckOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation->Parent);   
    SECURITY_STATUS status = SEC_E_CERT_UNKNOWN;
#if !defined(PLATFORM_UNIX)    
    CertContextUPtr certContext;
    auto error = accessCheckOperation->request_.EndGetClientCertificate(operation, certContext);
#else
    SSL* sslContext;
    auto error = accessCheckOperation->request_.EndGetClientCertificate(operation, &sslContext); 
        
#endif    
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType, "Client certificate missing for uri: {0}, error: {1}.", accessCheckOperation->request_.GetUrl(), error);
        UpdateAuthenticationHeaderOnFailure(operation->Parent);
        accessCheckOperation->TryComplete(operation->Parent, ErrorCodeValue::InvalidCredentials);
        return;
    }

    SecuritySettingsSPtr settings = Settings();
#if !defined(PLATFORM_UNIX)

    status = SecurityUtil::VerifyCertificate(
        certContext.get(),
        settings->RemoteCertThumbprints(),
        settings->RemoteX509Names(),
        true,
        settings->X509CertChainFlags(),
        settings->ShouldIgnoreCrlOfflineError(true));
#else    
    
    auto remoteCertChain = SSL_get_peer_cert_chain(sslContext);
    if (!remoteCertChain)
    {
        WriteInfo(TraceType,  "SSL_get_peer_cert_chain returned null");
        status = E_FAIL;
	return;
    }

    CertContextUPtr remoteCert;
    X509StackShallowUPtr clientCertStack; //Use "Shallow" as SSL_get_peer_cert_chain does not 
    //increase ref count on X509 objects in the returned stack
    
        remoteCert = CertContextUPtr(SSL_get_peer_certificate(sslContext));
        if (!remoteCert)
        {
            WriteInfo(TraceType, "SSL_get_peer_certificate returned null");
            status = E_FAIL;
	    return;
        }

        clientCertStack = X509StackShallowUPtr(sk_X509_dup(remoteCertChain));
        sk_X509_insert(clientCertStack.get(), remoteCert.get(), 0);
        remoteCertChain = clientCertStack.get();
    

    
    
    UINT_PTR tracingContext = 0;
    wstring commonNameMatched;
    //remoteCertChain
    auto traceId = wformatString("{0:x}", tracingContext);
    //LINUXTODO, consider avoiding double validation on cert chain, use SSL_CTX_set_verify/SSL_set_verify to set a callback
    //to get cert verify erros, and pass them to  VerifyCertificate, so that VerifyCertificate just need to check allowlist
    auto err = LinuxCryptUtil().VerifyCertificate(
        traceId,
        remoteCertChain,
        settings->X509CertChainFlags(),
        settings->ShouldIgnoreCrlOfflineError(true),
        false,
        settings->RemoteCertThumbprints(),
        settings->RemoteX509Names(),
        true,
        &commonNameMatched);     
    
    WriteInfo( TraceType, "Verifying all roles completed with Error={0}", err);
    
    if(err.IsSuccess())
    {
      status = SEC_E_OK;      
    }
#endif

    if (status == SEC_E_OK)
    {
        // If RBAC is enabled, get the role.
        if (isClientRoleInEffect_)
        {
#if !defined(PLATFORM_UNIX)
	  
	   status = SecurityUtil::VerifyCertificate(
                certContext.get(),
                settings->AdminClientCertThumbprints(),
                settings->AdminClientX509Names(),
                false,
                settings->X509CertChainFlags(),
                settings->ShouldIgnoreCrlOfflineError(true));
#else

	    wstring adminCommonNameMatched;
	    // remoteCertChain
	    err = LinuxCryptUtil().VerifyCertificate(
            traceId,
            remoteCertChain,
            settings->X509CertChainFlags(),
            settings->ShouldIgnoreCrlOfflineError(true),
            false,
            settings->AdminClientCertThumbprints(),
            settings->AdminClientX509Names(),
            false,
            &adminCommonNameMatched);
    
	    WriteInfo( TraceType, "Verifying admin role completed with Error={0}", err);
	    
            if(err.IsSuccess())
	    {
	      status = SEC_E_OK;	      
	    }
	    else
	    {
	      // TODO: assign correct SECURITY_STATUS
	      status = SEC_E_CERT_UNKNOWN;
	    }

#endif
            if (status == SEC_E_OK)
            {
                role_ = RoleMask::Admin;
            }
            else
            {	     
                role_ = RoleMask::User;
            }
        }
        else
        {
            role_ = RoleMask::None;
        }

        accessCheckOperation->TryComplete(operation->Parent, ErrorCodeValue::Success);
        return;
    }
    else
    {
        WriteInfo(TraceType, "Invalid Client certificate for uri: {0}", accessCheckOperation->request_.GetUrl());

        UpdateAuthenticationHeaderOnFailure(operation->Parent);
        accessCheckOperation->TryComplete(operation->Parent, ErrorCodeValue::InvalidCredentials);
    }
}

Transport::SecuritySettingsSPtr HttpCertificateAuthHandler::Settings()
{
    AcquireReadLock grab(credentialLock_); // read lock for reading security_
    return securitySettings_;
}

ErrorCode HttpCertificateAuthHandler::OnInitialize(SecuritySettings const& securitySettings)
{
    AcquireWriteLock grab(credentialLock_);

    securitySettings_ = make_shared<SecuritySettings>(securitySettings);
    StartCertMonitorTimerIfNeeded_CallerHoldingWLock();

    WriteInfo(TraceType, "SecuritySettings: {0}", securitySettings);
    return ErrorCode::Success();
}

ErrorCode HttpCertificateAuthHandler::OnSetSecurity(SecuritySettings const& securitySettings)
{
    return OnInitialize(securitySettings);
}

void HttpCertificateAuthHandler::UpdateAuthenticationHeaderOnFailure(Common::AsyncOperationSPtr const& operation)
{
    auto thisOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);
    thisOperation->httpStatusOnError_ = Constants::StatusUnauthorized;
}

void HttpCertificateAuthHandler::StartCertMonitorTimerIfNeeded_CallerHoldingWLock()
{
    auto certMonitorInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;

    if (certMonitorTimer_ || certMonitorInterval <= TimeSpan::Zero)
    {
        WriteInfo(TraceType, "Skipping certificate monitor start");
        return;
    }

    WriteInfo(TraceType, "Starting certificate monitor, interval = {0}", certMonitorInterval);

    static StringLiteral const timerTag("HttpCertificateAuthCertificateMonitor");
    certMonitorTimer_ = Timer::Create(
        timerTag,
        [this](TimerSPtr const &) { CertMonitorTimerCallback(); },
        false);

    certMonitorTimer_->SetCancelWait();

    Random r;
    TimeSpan dueTime = TimeSpan::FromMilliseconds(certMonitorInterval.TotalMilliseconds() * r.NextDouble());
    certMonitorTimer_->Change(dueTime, certMonitorInterval);
}

void HttpCertificateAuthHandler::CertMonitorTimerCallback()
{
    /* The list here is only refreshed if public keys of the respective issuer certificates change in the computer certificate store.
    For config upgrades, this code path is not executed - the config handlers are triggered and a new TransportSecurity object is set.
    */

    X509IdentitySet latestRemoteIssuerPublicKeys;
    bool isRemoteIssuerChanged = false;
    SecuritySettings newSettings;
    SecuritySettingsSPtr settings = Settings();
    {
        // Check if new remote issuers are added to certificate store
        if (!settings->RemoteCertIssuers().IsEmpty())
        {
            X509IdentitySet currentRemoteIssuerPublicKeys = settings->RemoteX509Names().GetAllIssuers();
            settings->RemoteCertIssuers().GetPublicKeys(latestRemoteIssuerPublicKeys);

            if (currentRemoteIssuerPublicKeys != latestRemoteIssuerPublicKeys)
            {
                isRemoteIssuerChanged = true;
                newSettings = SecuritySettings(*settings);
            }
        }
    } // release ReadLock since its not required anymore. SetSecurity acquires a WriteLock.

    if (isRemoteIssuerChanged)
    {
        newSettings.SetRemoteCertIssuerPublicKeys(latestRemoteIssuerPublicKeys);
        if (settings->IsClientRoleInEffect())
        {
            newSettings.SetAdminClientCertIssuerPublicKeys(latestRemoteIssuerPublicKeys);
        }
        SetSecurity(newSettings);
    }
}