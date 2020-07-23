// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace HttpServer;
using namespace HttpClient;
using namespace Transport;
using namespace HttpApplicationGateway;
using namespace Reliability;

class HttpApplicationGatewayImpl::CreateAndOpenAsyncOperation : public TimedAsyncOperation
{
public:
    CreateAndOpenAsyncOperation(
        __in FabricNodeConfigSPtr &config,
        TimeSpan &timeout,
        AsyncCallback const &callback)
        : TimedAsyncOperation(timeout, callback, AsyncOperationSPtr())
        , config_(config)
    {
    }

    static ErrorCode CreateAndOpenAsyncOperation::End(AsyncOperationSPtr const &operation, __out HttpApplicationGatewaySPtr & server)
    {
        auto thisPtr = AsyncOperation::End<CreateAndOpenAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            server = move(thisPtr->applicationGateway_);
        }

        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        applicationGateway_ = shared_ptr<HttpApplicationGatewayImpl>(new HttpApplicationGatewayImpl(config_));

        auto operation = applicationGateway_->BeginOpen(
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnOpenComplete(operation, false);
            },
            thisSPtr);

        OnOpenComplete(operation, true);
    }

private:

    void OnOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = applicationGateway_->EndOpen(operation);

        TryComplete(operation->Parent, error);
    }

    HttpApplicationGatewaySPtr applicationGateway_;
    FabricNodeConfigSPtr config_;
};

class HttpApplicationGatewayImpl::OpenAsyncOperation : public TimedAsyncOperation
{
public:
    OpenAsyncOperation(
        HttpApplicationGatewayImpl &owner,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        httpServerImplSPtr_ = make_shared<HttpServerImpl>(
            owner_, 
            owner_.GetListenAddress(), 
            HttpApplicationGatewayConfig::GetConfig().NumberOfParallelOperations,
            owner_.GetSecurityProvider(),
            LookasideAllocatorSettings(
                HttpApplicationGatewayConfig::GetConfig().NumberOfParallelOperations,
                HttpApplicationGatewayConfig::GetConfig().PercentofExtraAllocsToMaintain,
                HttpApplicationGatewayConfig::GetConfig().LookasideAllocationBlockSizeInKb * 1024));

        auto error = HttpClientImpl::CreateHttpClient(
            L"HttpApplicationGateway",
            owner_,
            owner_.defaultHttpClientSPtr_);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (StringUtility::AreEqualCaseInsensitive(owner_.protocol_, HttpCommon::HttpConstants::HttpsUriScheme))
        {
            // For SSL connections, read the client certificate to be used for requests sent by the application gateway.
            error = owner_.GetClientCertificateContext();
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
        }
        owner_.CreateServiceResolver();

        auto timeout = RemainingTime;
        auto operation = owner_.serviceResolver_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnServiceResolverOpenComplete(operation, false);
            },
            thisSPtr);

        OnServiceResolverOpenComplete(operation, true);

    }

private:

    void OnServiceResolverOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.serviceResolver_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            HttpApplicationGatewayEventSource::Trace->ServiceResolverOpenFailed(
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        auto timeout = RemainingTime;
        auto httpServerOpenOperation = httpServerImplSPtr_->BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnHttpServerOpenComplete(operation, false);
            },
            operation->Parent);

        OnHttpServerOpenComplete(httpServerOpenOperation, true);
    }

    void OnHttpServerOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = httpServerImplSPtr_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        owner_.httpServerSPtr_ = move(httpServerImplSPtr_);      

        owner_.RegisterMessageHandlers();

        TryComplete(operation->Parent, error);
    }

    HttpApplicationGatewayImpl &owner_;
    shared_ptr<HttpServerImpl> httpServerImplSPtr_;
};

class HttpApplicationGatewayImpl::CloseAsyncOperation : public TimedAsyncOperation
{
public:
    CloseAsyncOperation(
        HttpApplicationGatewayImpl &owner,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , httpServer_((HttpServerImpl&)(*owner_.httpServerSPtr_))
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto operation = httpServer_.BeginClose(
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnHttpServerCloseComplete(operation, false);
            },
            thisSPtr);

        OnHttpServerCloseComplete(operation, true);
    }

private:

    void OnHttpServerCloseComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = httpServer_.EndClose(operation);
        TryComplete(operation->Parent, error);
    }

    HttpApplicationGatewayImpl &owner_;
    HttpServerImpl& httpServer_;
};

AsyncOperationSPtr HttpApplicationGatewayImpl::BeginCreateAndOpen(
    __in FabricNodeConfigSPtr &config,
    TimeSpan &timeout,
    AsyncCallback const &callback)
{
    return AsyncOperation::CreateAndStart<CreateAndOpenAsyncOperation>(config, timeout, callback);
}

ErrorCode HttpApplicationGatewayImpl::EndCreateAndOpen(
    AsyncOperationSPtr const & operation,
    __out HttpApplicationGatewaySPtr & server)
{
    return CreateAndOpenAsyncOperation::End(operation, server);
}

AsyncOperationSPtr HttpApplicationGatewayImpl::OnBeginOpen(
    __in TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    HttpApplicationGatewayEventSource::Trace->BeginOpen();
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpApplicationGatewayImpl::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    auto error = HttpApplicationGatewayImpl::OpenAsyncOperation::End(asyncOperation);
    HttpApplicationGatewayEventSource::Trace->EndOpen(error);
    return error;
}

AsyncOperationSPtr HttpApplicationGatewayImpl::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    HttpApplicationGatewayEventSource::Trace->BeginClose();
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpApplicationGatewayImpl::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    auto error = CloseAsyncOperation::End(asyncOperation);
    HttpApplicationGatewayEventSource::Trace->EndClose(error);
    return error;
}

void HttpApplicationGatewayImpl::OnAbort()
{
    if (httpServerSPtr_)
    {
        ((HttpServerImpl&)(*httpServerSPtr_)).Abort();
    }
}

ErrorCode HttpApplicationGatewayImpl::RegisterMessageHandlers()
{
    gatewayRequestHandlerSPtr_ = make_shared<GatewayRequestHandler>(*this);

    //
    // The request handler is for all requests to the gateway.
    //
    return httpServerSPtr_->RegisterHandler(L"/", gatewayRequestHandlerSPtr_);
}

ErrorCode HttpApplicationGatewayImpl::CreateHttpClientRequest(
    wstring const &clientUri,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    KAllocator &allocator,
    __in bool allowRedirects,
    __in bool enableCookies,
    __in bool enableWinauthAutoLogon,
    __out IHttpClientRequestSPtr &clientRequest)
{
    return defaultHttpClientSPtr_->CreateHttpRequest(
        clientUri, 
        connectTimeout,
        sendTimeout,
        receiveTimeout,
        allocator,
        clientRequest,
        allowRedirects,
        enableCookies,
        enableWinauthAutoLogon);
}

wstring HttpApplicationGatewayImpl::GetListenAddress()
{
    //
    // This routine constructs the url that the http server should listen on.
    //

    if (HttpApplicationGatewayConfig::GetConfig().StandAloneMode)
    {
        return ParseListenAddress(
            HttpApplicationGatewayConfig::GetConfig().Protocol,
            HttpApplicationGatewayConfig::GetConfig().ListenAddress);
    }
    else
    {
        return ParseListenAddress(
            fabricNodeConfigSPtr_->HttpApplicationGatewayProtocol,
            fabricNodeConfigSPtr_->HttpApplicationGatewayListenAddress);
    }
}

wstring HttpApplicationGatewayImpl::ParseListenAddress(wstring const &protocol, wstring const &listenAddress)
{
    wstring portString;
    wstring hostString;
    auto error = TcpTransportUtility::TryParseHostPortString(
        listenAddress,
        hostString,
        portString);

    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->ParseListenAddressFail(
            listenAddress,
            error);

        //
        // Crashing here, would give feedback to the user that there is an invalid address format.
        //

        Assert::CodingError(
            "Failed to parse port from HttpApplicationGatewayListenAddress : {0} ErrorCode : {1}",
            listenAddress,
            error);
    }

    wstring rootUrl = wformatString("{0}://+:{1}/", protocol, portString);

    return move(rootUrl);
}

void HttpApplicationGatewayImpl::CreateServiceResolver()
{
    if (HttpApplicationGatewayConfig::GetConfig().StandAloneMode)
    {
        serviceResolver_ = make_unique<DummyServiceResolver>();
    }
    else
    {
        serviceResolver_ = make_unique<ServiceResolverImpl>(fabricNodeConfigSPtr_);
    }
}

//
// From the ApplicationGateway/Http config, obtain the applicationGateway certificate.
// This will be used as the client certificate while communicating with applications/services over SSL.
//
ErrorCode HttpApplicationGatewayImpl::GetClientCertificateContext()
{
    SecurityProvider::Enum securityProvider;
    auto error = SecurityProvider::FromCredentialType(
        HttpApplicationGatewayConfig::GetConfig().GatewayAuthCredentialType, securityProvider);
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->ParseGatewayAuthCredentialTypeFail(error);
        return error;
    }

    if (securityProvider != SecurityProvider::Ssl)
    {
        HttpApplicationGatewayEventSource::Trace->SecurityProviderError(error);
        return error;
    }

    X509FindValue::SPtr x509FindValue;
    error = X509FindValue::Create(
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindType,
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValue,
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateFindValueSecondary,
        x509FindValue);

    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->ParseGatewayX509CertError(error);
        return error;
    }

    error = CryptoUtility::GetCertificate(
        X509Default::StoreLocation(),
        HttpApplicationGatewayConfig::GetConfig().GatewayX509CertificateStoreName,
        x509FindValue,
        clientCertContext_);

    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->GetGatewayCertificateError(error);
        return error;
    }

    return error;
}

