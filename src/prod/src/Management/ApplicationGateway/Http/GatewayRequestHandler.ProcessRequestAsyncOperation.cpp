// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace HttpCommon;
using namespace HttpServer;
using namespace HttpClient;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Transport;

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnCancel()
{
    this->CancelResolveTimer();
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnCompleted()
{
    this->CancelResolveTimer();
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    if (!PrepareRequest(thisSPtr))
        return;

    if (forwardingUri_.empty())
    {
        DoResolve(thisSPtr);
        return;
    }

    if (IsWebSocketUpgradeRequest())
    {
        StartWebsocketCommunication(thisSPtr, forwardingUri_);
        return;
    }

    //
    // TODO: Currently we give send and receive timeouts as remaining time. This
    // has the following issue:
    // 1. Connect time is not accounted for in the send and receive time. So this OpenAsyncOperation
    //   might take longer. This should be made a timed async operation.
    //

    auto connectTimeout = owner_.gateway_.GetGatewayConfig().HttpRequestConnectTimeout;
    if (connectTimeout > timeoutHelper_.GetRemainingTime())
    {
        connectTimeout = timeoutHelper_.GetRemainingTime();
    }

    auto error = owner_.gateway_.CreateHttpClientRequest(
        forwardingUri_,
        connectTimeout, // Connect timeout
        timeoutHelper_.GetRemainingTime(), // Send timeout
        timeoutHelper_.GetRemainingTime(), // Receive timeout
        requestFromClient_->GetThisAllocator(),
        FALSE, // disable redirects - 30x status code is returned to the application.
        FALSE, // disable handling cookies at the reverse proxy.
        SecurityProvider::IsWindowsProvider(owner_.gateway_.GetSecurityProvider()), // With Windows Auth, enable auto logon
        requestToService_);

    if (!error.IsSuccess())
    {
        OnError(thisSPtr, error, L"CreateHttpClientRequest");
        return;
    }

    SendRequestToService(thisSPtr);
}

bool GatewayRequestHandler::ProcessRequestAsyncOperation::PrepareRequest(AsyncOperationSPtr const &thisSPtr)
{
    auto tAsyncOperation = dynamic_cast<IHttpApplicationGatewayHandlerOperation*>(thisSPtr->Parent.get());

    std::wstring serviceName = L"";
    if (forwardingUri_.empty())
    {
        tAsyncOperation->GetServiceName(serviceName);
    }

    if (!ParsedGatewayUri::TryParse(requestFromClient_, parsedUri_, serviceName))
    {
        OnError(thisSPtr, ErrorCodeValue::InvalidAddress, L"ParseRequestUri");
        return false;
    }

    auto remainingTime = parsedUri_->RequestTimeout;
    if (remainingTime == TimeSpan::Zero)
    {
        remainingTime = owner_.gateway_.GetGatewayConfig().DefaultHttpRequestTimeout;
    }
    timeoutHelper_.SetRemainingTime(remainingTime);

    auto error = requestFromClient_->GetAllRequestHeaders(requestHeaders_);
    if (!error.IsSuccess())
    {
        OnError(thisSPtr, ErrorCodeValue::InvalidArgument, L"ParseRequestHeader");
        return false;
    }

    if (forwardingUri_.empty())
    {
        std::unordered_map<std::wstring, std::wstring> headersToAdd;
        tAsyncOperation->GetAdditionalHeadersToSend(headersToAdd);

        // Append custom headers
        for (auto& header : headersToAdd)
        {
            KWString headerNameStr(requestFromClient_->GetThisAllocator());
            headerNameStr = KStringView(header.first.c_str());

            KString::SPtr headerValStr;
            auto status = KString::Create(
                headerValStr,
                requestFromClient_->GetThisAllocator(),
                header.second.c_str(),
                TRUE);

            if (!NT_SUCCESS(status))
            {
                HttpApplicationGatewayEventSource::Trace->CreateHeaderValStrError(
                    traceId_,
                    header.first,
                    status
                );
                continue;
            }

            requestHeaders_.Put(headerNameStr, headerValStr);
        }
    }

    return true;
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::ScheduleResolve(AsyncOperationSPtr const &thisSPtr)
{
    TimeSpan remainingTime = timeoutHelper_.GetRemainingTime();
    if (remainingTime <= TimeSpan::Zero)
    {
        OnError(thisSPtr, ErrorCodeValue::Timeout, L"ScheduleResolve");
        return;
    }

    Random r;
    TimeSpan timeout = TimeSpan::FromMilliseconds(
        owner_.gateway_.GetGatewayConfig().ResolveServiceBackoffInterval.TotalMilliseconds() * r.NextDouble());

    if (remainingTime < timeout)
    {
        timeout = remainingTime;
    }

    {
        AcquireExclusiveLock lock(resolveRetryTimerLock_);
        resolveRetryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
        {
            timer->Cancel();
            this->DoResolve(thisSPtr);
        },
            true); // allow concurrency

        resolveRetryTimer_->Change(timeout);
    }
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::DoResolve(AsyncOperationSPtr const &thisSPtr)
{
    if (timeoutHelper_.IsExpired)
    {
        OnError(thisSPtr, ErrorCodeValue::Timeout, L"BeginResolve");
        return;
    }

    auto remainingTime = timeoutHelper_.GetRemainingTime();
    if (remainingTime > owner_.gateway_.GetGatewayConfig().ResolveTimeout)
    {
        remainingTime = owner_.gateway_.GetGatewayConfig().ResolveTimeout;
    }

    auto operation = owner_.gateway_.GetServiceResolver().BeginPrefixResolveServicePartition(
        parsedUri_,
        previousRspResult_,
        traceId_,
        requestFromClient_->GetThisAllocator(),
        remainingTime,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnResolveComplete(operation, false);
    },
        thisSPtr);

    OnResolveComplete(operation, true);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnResolveComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    IResolvedServicePartitionResultPtr result;
    auto error = owner_.gateway_.GetServiceResolver().EndResolveServicePartition(operation, result);

    if (!error.IsSuccess())
    {
        if (owner_.IsRetriableError(error))
        {
            ScheduleResolve(operation->Parent);
            return;
        }
        else
        {
            OnError(operation->Parent, error, L"ResolveServicePartition");
            return;
        }
    }

    previousRspResult_ = result;

    wstring replicaEndpoint;
    error = owner_.gateway_.GetServiceResolver().GetReplicaEndpoint(result, parsedUri_->TargetReplicaSelector, replicaEndpoint);
    if (!error.IsSuccess())
    {
        if (owner_.IsRetriableError(error))
        {
            ScheduleResolve(operation->Parent);
            return;
        }
        else
        {
            OnError(operation->Parent, error, L"GetReplicaEndpoint");
            return;
        }
    }

    ServiceEndpointsList parsedEndpointList;
    if (ServiceEndpointsList::TryParse(replicaEndpoint, parsedEndpointList))
    {
        if (!parsedEndpointList.TryGetEndpoint(owner_.gateway_.GetGatewayConfig().SecureOnlyMode, owner_.gateway_.GetProtocolType(), parsedUri_->ListenerName, replicaEndpoint))
        {
            wstring traceStr = wformatString("SecureOnlyMode = {0}, gateway protocol = {1}, listenerName = {2}, replica endpoint = {3}",
                owner_.gateway_.GetGatewayConfig().SecureOnlyMode,
                owner_.gateway_.GetProtocol(),
                parsedUri_->ListenerName,
                replicaEndpoint);

            OnError(operation->Parent, ErrorCodeValue::EndpointNotFound, L"TryGetEndpoint", traceStr);
            return;
        }
    }

    // Resolve successful, read the body and forward.
    wstring forwardingUri;
    
    std::wstring serviceName = parsedUri_->ServiceName;
    if (serviceName.empty())
        serviceName = owner_.gateway_.GetServiceResolver().GetServiceName(result);
    if (!parsedUri_->GetUriForForwarding(
        serviceName,
        replicaEndpoint,
        forwardingUri))
    {
        wstring traceStr = wformatString("service name = {0}, replica endpoint = {1}",
            serviceName,
            replicaEndpoint);
        OnError(operation->Parent, ErrorCodeValue::InvalidAddress, L"UriForForwarding", traceStr);
        return;
    }

    KString::SPtr clientAddress;
    requestFromClient_->GetRemoteAddress(clientAddress);

    if (resolveCounter_ == 0)
    {
        owner_.gateway_.GetTraceEventSource().RequestReceivedFunc(
            traceId_,
            requestUrl_,
            requestFromClient_->GetVerb(),
            wstring(*clientAddress),
            forwardingUri,
            startTime_);
    }
    else
    {
        owner_.gateway_.GetTraceEventSource().RequestReresolvedFunc(
            traceId_,
            forwardingUri);
    }

    // Increment resolve counter to track the successful resolves
    resolveCounter_++;

    if (IsWebSocketUpgradeRequest())
    {
        StartWebsocketCommunication(operation->Parent, forwardingUri);
    }
    else
    {
        //
        // TODO: Currently we give send and receive timeouts as remaining time. This
        // has the following issue:
        // 1. Connect time is not accounted for in the send and receive time. So this OpenAsyncOperation
        //   might take longer. This should be made a timed async operation.
        //

        auto connectTimeout = owner_.gateway_.GetGatewayConfig().HttpRequestConnectTimeout;
        if (connectTimeout > timeoutHelper_.GetRemainingTime())
        {
            connectTimeout = timeoutHelper_.GetRemainingTime();
        }

        error = owner_.gateway_.CreateHttpClientRequest(
            forwardingUri,
            connectTimeout, // Connect timeout
            timeoutHelper_.GetRemainingTime(), // Send timeout
            timeoutHelper_.GetRemainingTime(), // Receive timeout
            requestFromClient_->GetThisAllocator(),
            FALSE, // disable redirects - 30x status code is returned to the application.
            FALSE, // disable handling cookies at the reverse proxy.
            SecurityProvider::IsWindowsProvider(owner_.gateway_.GetSecurityProvider()), // With Windows Auth, enable auto logon
            requestToService_);

        if (!error.IsSuccess())
        {
            OnError(operation->Parent, error, L"CreateHttpClientRequest");
            return;
        }

        SendRequestToService(operation->Parent);
    }
}

bool GatewayRequestHandler::ProcessRequestAsyncOperation::IsWebSocketUpgradeRequest()
{
    KString::SPtr upgradeHeaderValue;
    KWString kws = Utility::GetHeaderNameKString(HttpConstants::UpgradeHeader, requestFromClient_->GetThisAllocator());
    auto status = requestHeaders_.Get(
        kws,
        upgradeHeaderValue);

    if (NT_SUCCESS(status))
    {
        if (upgradeHeaderValue->CompareNoCase(KStringView(HttpConstants::WebSocketUpgradeHeaderValue->c_str())) == 0)
        {
            return true;
        }
        else
        {
            HttpApplicationGatewayEventSource::Trace->WebsocketUpgradeUnsupportedHeader(
                traceId_,
                wstring(*upgradeHeaderValue));
        }
    }

    return false;
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::StartWebsocketCommunication(
    AsyncOperationSPtr const &thisSPtr,
    wstring const &forwardingUri)
{
    if (timeoutHelper_.IsExpired)
    {
        OnError(thisSPtr, ErrorCodeValue::Timeout, L"StartWebsocketCommunication");
        return;
    }

    KString::SPtr headers;
    KString::SPtr clientAddress;
    auto error = requestFromClient_->GetRemoteAddress(clientAddress);
    clientAddress->MeasureThis();

    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->GetClientAddressError(
            traceId_,
            error);
    }

    owner_.ProcessHeaders(requestHeaders_, requestFromClient_->GetThisAllocator(), clientAddress, true, traceId_, headers);

    // Create a KDelegate for ValidateServerCertificate static method. The first parameter can contain any data that we want to pass to the delegate.
    // Pass the validation context containing traceId and gateway config, so that the delegate can 
    // 1. log traces with the corresponding Id.
    // 2. fetch certificate validation config options
    KHttpClientWebSocket::ServerCertValidationHandler handler((PVOID)&validationContext_, GatewayRequestHandler::ValidateServerCertificate);

    webSocketManager_ = move(make_shared<WebSocketManager>(
        traceId_,
        requestFromClient_,
        Utility::GetWebSocketUri(forwardingUri),
        owner_.gateway_.GetComponentRoot(),
        headers,
        owner_.gateway_.GetCertificateContext(),
        handler));

    auto operation = webSocketManager_->BeginOpen(
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnWebSocketUpgradeComplete(operation, false);
    },
        thisSPtr);

    OnWebSocketUpgradeComplete(operation, true);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnWebSocketUpgradeComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    bool safeToRetry;
    ULONG winHttpError;
    auto error = webSocketManager_->EndOpen(operation, &winHttpError, &safeToRetry);
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->WebsocketUpgradeError(
            traceId_,
            error,
            winHttpError,
            safeToRetry);

        if (safeToRetry)
        {
            webSocketManager_->AbortAndWaitForTermination();
            ScheduleResolve(operation->Parent);
        }
        else
        {
            webSocketManager_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const &abortOperation)
            {
                TryComplete(abortOperation->Parent, error);
                return;
            },
                operation->Parent);
        }

        return;
    }

    HttpApplicationGatewayEventSource::Trace->WebsocketUpgradeCompleted(
        traceId_);

    auto pingpongOperation = webSocketManager_->BeginForwardingIo(
        [this](AsyncOperationSPtr const &operation)
    {
        OnWebSocketCommunicationComplete(operation, false);
    },
        operation->Parent);

    OnWebSocketCommunicationComplete(pingpongOperation, true);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnWebSocketCommunicationComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = webSocketManager_->EndForwardingIo(operation);
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->WebsocketIOError(
            traceId_,
            error);

        webSocketManager_->AbortAndWaitForTerminationAsync(
            [this, error](AsyncOperationSPtr const &abortOperation)
        {
            TryComplete(abortOperation->Parent, error);
            return;
        },
            operation->Parent);

        return;
    }

    TryComplete(operation->Parent, error);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::SendRequestToService(AsyncOperationSPtr const &thisSPtr)
{
    if (timeoutHelper_.IsExpired)
    {
        OnError(thisSPtr, ErrorCodeValue::Timeout, L"BeginSendRequestToService");
        return;
    }

    auto operation = owner_.BeginSendRequestToService(
        traceId_,
        requestFromClient_,
        requestToService_,
        requestHeaders_,
        move(body_),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnSendRequestToServiceComplete(operation, false);
    },
        thisSPtr);

    OnSendRequestToServiceComplete(operation, true);
}

//
// NOTE ON THE RETRY MECHANISM
// If the service replica moves between the time we queried the endpoint and the time we sent the request, we backoff, query the
// endpoint again and try to resend the request to the new endpoint. We determine the replica movement based on the conditions
// below.
// 1. Connection error when making the initial connection - This happens if there is no one listening on that address.
// 2. Response status code 404(NotFound)- This can be returned for the following cases,
//      a) Client requested a resource from the service and the service is returning 404(NotFound) to indicate that
//         the resource is no longer present.
//      b) For http.sys based services when the service uses wild card based url reservations(+ or *), if there is another replica using the same port
//         but a different path suffix, then http.sys would return 404(NotFound) to indicate that it didnt find a listener corresponding
//         to the path requested.
//    2a should be returned to the client where as 2b should trigger a reresolve and retry. 
//    In order to distinguish 2a from 2b, services should set the header ServiceFabricHttpHeaderValueNotFound when returning NotFound for the case 2a.
// 3. Response status code 503(ServiceUnavailable) - This can be returned for the following cases,
//     a) Service is busy and is not able to process the request.
//     b) For http.sys based services, when the service uses a non wild card based url reservation and registration, if there was no listener found for the path suffix requested.
//    We reresolve and retry in both the cases. We will not be able to distinguish 3a from 3b, so we perform an extra resolve call on both cases.
// 4. Response status code 410(Gone) - This is returned by the asp.net middleware that we ship as a part of the asp.net based communication listeners in our sdk.
//

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnSendRequestToServiceComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ULONG winHttpError;
    bool safeToRetry;
    wstring failurePhase;
    auto error = owner_.EndSendRequestToService(operation, &winHttpError, &safeToRetry, failurePhase);
    if (!error.IsSuccess())
    {
        //
        // Case #1 above
        //
        if (safeToRetry && (owner_.IsRetriableError(error) || owner_.IsRetriableError(winHttpError)))
        {
            HttpApplicationGatewayEventSource::Trace->ForwardRequestErrorRetrying(
                traceId_,
                failurePhase,
                error,
                HttpUtil::WinHttpCodeToString(winHttpError));
            ScheduleResolve(operation->Parent);
            return;
        }
        else
        {
            if (owner_.IsConnectionError(winHttpError))
            {
                // If we have already sent some data(not safe to retry the request) and 
                // there is a connection error when sending to service, terminate the client connection.
                OnDisconnect(operation->Parent, winHttpError, failurePhase);
            }
            else
            {
                USHORT statusCode = 0;
                wstring statusDescription;
                auto statusCodeError = requestToService_->GetResponseStatusCode(&statusCode, statusDescription);
                if (!statusCodeError.IsSuccess() || statusCode == 0)
                {
                    if (winHttpError != STATUS_SUCCESS)
                    {
                        owner_.ErrorCodeToHttpStatus(winHttpError, (KHttpUtils::HttpResponseCode&)statusCode, statusDescription);
                        wstring additionalTraceStr = wformatString(L"internal error = {0}", HttpUtil::WinHttpCodeToString(winHttpError));
                        OnForwardRequestError(operation->Parent, statusCode, statusDescription, failurePhase, additionalTraceStr);
                    }
                    else
                    {
                        OnError(operation->Parent, error, failurePhase);
                    }
                }
                else
                {
                    OnForwardRequestError(operation->Parent, statusCode, statusDescription, failurePhase);
                }
            }

            return;
        }
    }

    //
    // Successfully sent the request to service, now process the response.
    //

    USHORT statusCode;
    wstring statusDescription;
    requestToService_->GetResponseStatusCode(&statusCode, statusDescription);

    HttpApplicationGatewayEventSource::Trace->ServiceResponse(
        traceId_,
        statusCode,
        statusDescription);

    if (statusCode == KHttpUtils::NotFound
        && owner_.IsRetriableVerb(requestFromClient_->GetVerb()))
    {
        //
        // Case #2 above
        //
        wstring headerValue;
        error = requestToService_->GetResponseHeader(*Constants::ServiceFabricHttpHeaderName, headerValue);
        if (!error.IsSuccess() ||
            (headerValue != *Constants::ServiceFabricHttpHeaderValueNotFound))
        {
            HttpApplicationGatewayEventSource::Trace->ServiceResponseReresolve(
                traceId_,
                statusCode,
                statusDescription);

            DisconnectAndScheduleResolve(operation->Parent);
            return;
        }
    }
    else if (statusCode == KHttpUtils::ServiceUnavailable
        && owner_.IsRetriableVerb(requestFromClient_->GetVerb()))
    {
        //
        // Case #3 above
        //
        HttpApplicationGatewayEventSource::Trace->ServiceResponseReresolve(
            traceId_,
            statusCode,
            statusDescription);

        DisconnectAndScheduleResolve(operation->Parent);
        return;
    }
    else if (statusCode == HttpConstants::StatusCodeGone
        && owner_.IsRetriableVerb(requestFromClient_->GetVerb()))
    {
        //
        // Case #4 above
        //
        HttpApplicationGatewayEventSource::Trace->ServiceResponseReresolve(
            traceId_,
            statusCode,
            statusDescription);

        DisconnectAndScheduleResolve(operation->Parent);
        return;
    }

    SendResponseToClient(operation->Parent);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::SendResponseToClient(AsyncOperationSPtr const &thisSPtr)
{
    auto operation = owner_.BeginSendResponseToClient(
        traceId_,
        requestFromClient_,
        requestToService_,
        startTime_,
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnSendResponseToClientComplete(operation, false);
    },
        thisSPtr);

    OnSendResponseToClientComplete(operation, true);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnSendResponseToClientComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ULONG winHttpError;
    wstring failurePhase;
    auto error = owner_.EndSendResponseToClient(operation, &winHttpError, failurePhase);
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->SendResponseError(
            traceId_,
            failurePhase,
            error,
            HttpUtil::WinHttpCodeToString(winHttpError));

        KString::SPtr clientAddress;
        requestFromClient_->GetRemoteAddress(clientAddress);

        HttpApplicationGatewayEventSource::Trace->SendResponseErrorWithDetails(
            traceId_,
            requestUrl_,
            requestFromClient_->GetVerb(),
            wstring(*clientAddress),
            startTime_,
            requestToService_->GetRequestUrl(),
            failurePhase,
            error,
            HttpUtil::WinHttpCodeToString(winHttpError));
    }

    TryComplete(operation->Parent, error);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::CancelResolveTimer()
{
    TimerSPtr snap;
    {
        AcquireExclusiveLock lock(resolveRetryTimerLock_);
        snap = move(this->resolveRetryTimer_);
    }

    if (snap)
    {
        snap->Cancel();
    }
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnError(
    __in AsyncOperationSPtr const& thisSPtr, __in ErrorCode error,
    __in std::wstring const& processRequestPhase,
    __in std::wstring const& additionalTraceStr)
{
    ByteBufferUPtr emptyBody;

    KString::SPtr clientAddress;
    requestFromClient_->GetRemoteAddress(clientAddress);

    if (resolveCounter_ == 0)
    {
        HttpApplicationGatewayEventSource::Trace->ProcessRequestErrorPreSend(
            traceId_,
            requestUrl_,
            requestFromClient_->GetVerb(),
            wstring(*clientAddress),
            startTime_,
            error,
            error.Message,
            processRequestPhase,
            additionalTraceStr);

        HttpApplicationGatewayEventSource::Trace->ProcessRequestErrorPreSendWithDetails(
            traceId_,
            requestUrl_,
            requestFromClient_->GetVerb(),
            wstring(*clientAddress),
            startTime_,
            error,
            error.Message,
            processRequestPhase,
            additionalTraceStr);
    }
    else
    {
        HttpApplicationGatewayEventSource::Trace->ProcessRequestError(
            traceId_,
            resolveCounter_,
            error,
            error.Message,
            processRequestPhase,
            additionalTraceStr);

        HttpApplicationGatewayEventSource::Trace->ProcessRequestErrorWithDetails(
            traceId_,
            requestUrl_,
            requestFromClient_->GetVerb(),
            wstring(*clientAddress),
            startTime_,
            requestToService_ != nullptr ? requestToService_->GetRequestUrl() : L"",
            resolveCounter_,
            error,
            error.Message,
            processRequestPhase,
            additionalTraceStr);
    }

    AsyncOperationSPtr operation = requestFromClient_->BeginSendResponse(
        error,
        move(emptyBody),
        [this, error](AsyncOperationSPtr const& operation)
    {
        this->requestFromClient_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnDisconnect(
    AsyncOperationSPtr const& thisSPtr,
    ULONG winHttpError,
    std::wstring const& failurePhase)
{
    HttpApplicationGatewayEventSource::Trace->ForwardRequestConnectionError(
        traceId_,
        failurePhase,
        HttpUtil::WinHttpCodeToString(winHttpError));

    KString::SPtr clientAddress;
    requestFromClient_->GetRemoteAddress(clientAddress);

    HttpApplicationGatewayEventSource::Trace->ForwardRequestConnectionErrorWithDetails(
        traceId_,
        requestUrl_,
        requestFromClient_->GetVerb(),
        wstring(*clientAddress),
        startTime_,
        requestToService_->GetRequestUrl(),
        failurePhase,
        HttpUtil::WinHttpCodeToString(winHttpError));

    KMemRef memref(0, nullptr);

    AsyncOperationSPtr operation = requestFromClient_->BeginSendResponseChunk(
        memref,
        true, // isLastSegment
        true, // disconnect
        [this](AsyncOperationSPtr const& operation)
    {
        this->requestFromClient_->EndSendResponseChunk(operation);
        this->TryComplete(operation->Parent, ErrorCodeValue::Success);
    },
        thisSPtr);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::OnForwardRequestError(
    AsyncOperationSPtr const& thisSPtr,
    USHORT statusCode,
    wstring const &description,
    wstring const& processRequestPhase,
    wstring const& additionalTraceStr)
{
    ByteBufferUPtr emptyBody;

    HttpApplicationGatewayEventSource::Trace->ForwardRequestError(
        traceId_,
        statusCode,
        description,
        processRequestPhase,
        additionalTraceStr);

    KString::SPtr clientAddress;
    requestFromClient_->GetRemoteAddress(clientAddress);

    HttpApplicationGatewayEventSource::Trace->ForwardRequestErrorWithDetails(
        traceId_,
        requestUrl_,
        requestFromClient_->GetVerb(),
        wstring(*clientAddress),
        startTime_,
        requestToService_->GetRequestUrl(),
        statusCode,
        description,
        processRequestPhase,
        additionalTraceStr);

    AsyncOperationSPtr operation = requestFromClient_->BeginSendResponse(
        statusCode,
        description,
        move(emptyBody),
        [this](AsyncOperationSPtr const& operation)
    {
        auto error = this->requestFromClient_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

void GatewayRequestHandler::ProcessRequestAsyncOperation::DisconnectAndScheduleResolve(AsyncOperationSPtr const &thisSPtr)
{
    // This just disconnects the request to service to complete the underlying KHttpCliRequest (to prevent leaks)
    // It does not complete any of the GatewayRequestHandler asyncs.

    auto buffer = KMemRef(0, nullptr);
    requestToService_->BeginSendRequestChunk(
        buffer,
        false,
        true, // disconnect
        [this](AsyncOperationSPtr const &operation)
    {
        ULONG winHttpError;
        requestToService_->EndSendRequestChunk(operation, &winHttpError);

        // Now that requestToService_ has been disconnected, schedule resolve. 
        this->ScheduleResolve(operation->Parent);
    },
        thisSPtr);
}
