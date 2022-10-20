// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpCommon;
using namespace HttpServer;
using namespace HttpClient;
using namespace HttpApplicationGateway;
using namespace Transport;

StringLiteral const TraceType("WebSocketManager");

class WebSocketManager::OpenAsyncOperation : public AllocableAsyncOperation
{
public:
    OpenAsyncOperation(
        WebSocketManager &owner,
        TimeSpan const &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , owner_(owner)
        , traceId_(owner.traceId_)
        , safeToRetry_(false)
        , timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, ULONG *winHttpError, bool *safeToRetry)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        *winHttpError = thisPtr->winHttpError_;
        *safeToRetry = thisPtr->safeToRetry_;

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        wstring websocketSubprotocolValue;
        auto error = owner_.requestFromClient_->GetRequestHeader(
            *HttpConstants::WebSocketSubprotocolHeader,
            websocketSubprotocolValue);

        if (!error.IsSuccess())
        {
            if (!error.IsError(ErrorCodeValue::NotFound))
            {
                WriteWarning(
                    TraceType,
                    "{0} Got error {1} when reading subprotocol header",
                    traceId_,
                    error);

                // TODO: Add the error message to status description.
                OnChannelToServiceOpenFailed(thisSPtr, error);
                return;
            }
        }

        WriteNoise(
            TraceType,
            "{0} Initiating websocket connection to with subprotocol {1}",
            owner_.traceId_,
            websocketSubprotocolValue);

        error = owner_.TransitionToOpening();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0} Transition to Opening failed",
                traceId_);

            OnChannelToServiceOpenFailed(thisSPtr, error);
            return;
        }

        ULONGLONG openHandshakeTimeout =
            timeoutHelper_.GetRemainingTime().TotalMilliseconds() < HttpApplicationGatewayConfig::GetConfig().WebSocketOpenHandshakeTimeout ?
            timeoutHelper_.GetRemainingTime().TotalMilliseconds() : HttpApplicationGatewayConfig::GetConfig().WebSocketOpenHandshakeTimeout;

        owner_.channelToService_ = make_shared<HttpClientWebSocket>(
            owner_.serviceEndpoint_,
            websocketSubprotocolValue,
            owner_.requestFromClient_->GetThisAllocator(),
            HttpApplicationGatewayConfig::GetConfig().WebSocketReceiveBufferSize,
            HttpApplicationGatewayConfig::GetConfig().WebSocketSendBufferSize,
            openHandshakeTimeout,
            HttpApplicationGatewayConfig::GetConfig().WebSocketCloseHandshakeTimeout,
            owner_.headers_,
            owner_.certContext_,
            owner_.serverCertValidationHandler_,
            false, // disable redirects
            false); // disable cookies

        auto operation = owner_.channelToService_->BeginOpen(
            []()
        {
            // no-op. Close processing happens when send or receive from this channel fails.
        },
            [this](AsyncOperationSPtr const &operation)
        {
            OnChannelToServiceOpenComplete(operation, false);
        },
            thisSPtr);

        OnChannelToServiceOpenComplete(operation, true);
    }

private:
    void OnChannelToServiceOpenComplete(
        AsyncOperationSPtr const &operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.channelToService_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            winHttpError_ = owner_.channelToService_->GetWinHttpError();
            auto httpResponseCode = owner_.channelToService_->GetHttpResponseCode();
            WriteWarning(
                TraceType,
                "{0} WebSocket connection to the service endpoint failed with {1}, winhttpError {2}, httpResponseCode {3}",
                traceId_,
                error,
                winHttpError_,
                httpResponseCode);

            // This is the implementation detail of kwebsocket:
            // K_STATUS_WEBSOCKET_STANDARD_VIOLATION :
            // 1. This can be returned if HTTP status code for the handshake is != HTTP_STATUS_SWITCH_PROTOCOLS (upgrade not successful).
            // 2. This is returned when the requested uri is no longer present during websocket connect - translated from
            // HTTP_STATUS_NOT_FOUND. 
            if (error.IsNtStatus(K_STATUS_WEBSOCKET_STANDARD_VIOLATION))
            {
                if (httpResponseCode != 0) // ChannelToService failed due to invalid HTTP status code returned from service
                {
                    // Re-resolve for 404 without X-ServiceFabric header.
                    if (httpResponseCode == KHttpUtils::NotFound
                        && GatewayRequestHandler::IsRetriableVerb(owner_.requestFromClient_->GetVerb()))
                    {
                        wstring headerValue;
                        auto innerError = owner_.channelToService_->GetResponseHeader(*Constants::ServiceFabricHttpHeaderName, headerValue);
                        if (!innerError.IsSuccess() ||
                            (headerValue != *Constants::ServiceFabricHttpHeaderValueNotFound))
                        {
                            WriteWarning(
                                TraceType,
                                "{0} Websocket connection, re-resolving due to httpResponseCode {1} without X-ServiceFabric header.",
                                traceId_,
                                httpResponseCode);

                            OnChannelToServiceOpenFailedWithRetriableError(operation->Parent, error);
                        }
                        else
                        {
                            OnChannelToServiceOpenFailed(operation->Parent, httpResponseCode);
                        }
                    } // Re-resolve for 410, 503,
                    else if (httpResponseCode == KHttpUtils::ServiceUnavailable
                        || httpResponseCode == KHttpUtils::Gone
                        && GatewayRequestHandler::IsRetriableVerb(owner_.requestFromClient_->GetVerb()))
                    {
                        OnChannelToServiceOpenFailedWithRetriableError(operation->Parent, error);
                    }
                    else
                    {
                        OnChannelToServiceOpenFailed(operation->Parent, httpResponseCode);
                    }
                }
                else
                {
                    winHttpError_ = ERROR_WINHTTP_CONNECTION_ERROR;
                    OnChannelToServiceOpenFailedWithRetriableError(operation->Parent, ErrorCodeValue::NameNotFound);
                }
                return;
            }
            else if (GatewayRequestHandler::IsRetriableError(winHttpError_))
            {
                // This has to be translated to gateway timeout indicating that there was 
                // some issue with the upstream connection, upon which clients backoff
                // and retry.
                OnChannelToServiceOpenFailedWithRetriableError(operation->Parent, ErrorCodeValue::Timeout);
                return;
            }
            else
            {
                OnChannelToServiceOpenFailed(operation->Parent, error);
                return;
            }
        }

        KStringViewA activeSubprotocol;
        error = owner_.channelToService_->GetActiveSubprotocol(activeSubprotocol);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0} Getting active subprotocol failed for WS connection to service failed with {1}",
                traceId_,
                error);

            TryComplete(operation->Parent, error);
            return;
        }

        WriteNoise(
            TraceType,
            "{0} Begin accept websocket connection with subprotocol",
            traceId_);

        owner_.channelToClient_ = make_shared<HttpServerWebSocket>(
            *owner_.requestFromClient_, 
            activeSubprotocol,
            HttpApplicationGatewayConfig::GetConfig().WebSocketReceiveBufferSize,
            HttpApplicationGatewayConfig::GetConfig().WebSocketSendBufferSize,
            HttpApplicationGatewayConfig::GetConfig().WebSocketOpenHandshakeTimeout,
            HttpApplicationGatewayConfig::GetConfig().WebSocketCloseHandshakeTimeout);

        auto openOperation = owner_.channelToClient_->BeginOpen(
            []()
            {
            // no-op. Close processing happens when send or receive from this channel fails.
            },
            [this](AsyncOperationSPtr const &operation)
            {
                OnChannelToClientWebSocketUpgradeComplete(operation, false);
            },
            operation->Parent);

            OnChannelToClientWebSocketUpgradeComplete(openOperation, true);
    }

    void OnChannelToClientWebSocketUpgradeComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.channelToClient_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0} Upgrading client request to websocket connection failed with {1}",
                traceId_,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0} Transition to Opened failed {1}",
                traceId_,
                error);
        }

        TryComplete(operation->Parent, error);
    }

    //
    // The owner of the WebSocketManager will transition the statemachine to aborted state.
    // We need to respond to the client request here, because the owner of WebSocketManager,
    // might not know how far we got in the websocket negotiation - whether the target service
    // rejected the websocket setup or if something else failed during websocket setup.
    //
    void OnChannelToServiceOpenFailed(AsyncOperationSPtr const &thisSPtr, ErrorCode error)
    {
        ByteBufferUPtr emptyBody;

        // Respond to client with the failure and then complete the operation.
        AsyncOperationSPtr operation = owner_.requestFromClient_->BeginSendResponse(
            error,
            move(emptyBody),
            [this, error](AsyncOperationSPtr const& operation)
            {
                this->owner_.requestFromClient_->EndSendResponse(operation);
                this->TryComplete(operation->Parent, error);
            },
            thisSPtr);
    }

    //
    // The owner of the WebSocketManager will transition the statemachine to aborted state.
    // We need to respond to the client request here with the status code returned from the service.
    //

    void OnChannelToServiceOpenFailed(AsyncOperationSPtr const &thisSPtr, ULONG statusCode)
    {
        ByteBufferUPtr emptyBody;

        AsyncOperationSPtr operation = owner_.requestFromClient_->BeginSendResponse(
            (USHORT)statusCode,
            L"", // TODO: Fetch status description from KTL request and propagate it back to the client.
            move(emptyBody),
            [this](AsyncOperationSPtr const& operation)
            {
                auto error = this->owner_.requestFromClient_->EndSendResponse(operation);
                this->TryComplete(operation->Parent, error);
            },
            thisSPtr);
    }

    //
    // The owner of the WebSocketManager will transition the statemachine to aborted state.
    //
    void OnChannelToServiceOpenFailedWithRetriableError(AsyncOperationSPtr const &thisSPtr, ErrorCode error)
    {
        safeToRetry_ = true;
        TryComplete(thisSPtr, error);
    }

    WebSocketManager &owner_;
    bool safeToRetry_;
    ULONG winHttpError_;
    wstring &traceId_;
    TimeoutHelper timeoutHelper_;
};

class WebSocketManager::ProcessAsyncOperation : public AllocableAsyncOperation
{
public:
    ProcessAsyncOperation(
        WebSocketManager &owner,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , owner_(owner)
        , traceId_(owner.traceId_)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<ProcessAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    //
    // When this operation is cancelled the child operations are automatically cancelled.
    //
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto error = owner_.TransitionToProcessing();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0} Transition to Processing failed {1}",
                traceId_,
                error);
            
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            "{0} Starting Websocket I/O",
            traceId_);

        clientToServiceIoForwarderSPtr_ = make_shared<WebSocketIoForwarder>(
            owner_.channelToClient_,
            owner_.channelToService_,
            owner_.traceId_ + L"_ClientToService");

        serviceToClientIoForwarderSPtr_ = make_shared<WebSocketIoForwarder>(
            owner_.channelToService_,
            owner_.channelToClient_,
            owner_.traceId_ + L"_ServiceToClient");

        {
            AcquireWriteLock writeLock(pendingIoLock_);
            pendingIoCount_ = 2;
        }

        clientToServiceIoForwarderSPtr_->BeginForwardingIo(
            [this](AsyncOperationSPtr const &operation)
            {
                OnClientToServiceIoComplete(operation);
            },
            thisSPtr);

        serviceToClientIoForwarderSPtr_->BeginForwardingIo(
            [this](AsyncOperationSPtr const &operation)
            {
            OnServiceToClientIoComplete(operation);
            },
            thisSPtr);
    }

private:

    void OnClientToServiceIoComplete(AsyncOperationSPtr const &operation)
    {
        KWebSocket::CloseStatusCode statusCode;
        KSharedBufferStringA::SPtr statusReason;
        auto error = clientToServiceIoForwarderSPtr_->EndForwardingIo(
            operation,
            statusCode,
            statusReason);

        WriteInfo(
            TraceType,
            "{0} WebSocket I/O completed with error {1}, StatusCode {2}",
            clientToServiceIoForwarderSPtr_->TraceId,
            error,
            (USHORT)statusCode);

        {
            AcquireWriteLock writeLock(pendingIoLock_);
            pendingIoCount_--;
            if (pendingIoCount_ != 1)
            {
                // Just return here as there is already a close in progress. That close completion
                // will complete the ProcessAsyncOperation.
                return;
            }
        }

        owner_.statusCode_ = statusCode;
        owner_.statusReason_ = statusReason;
        owner_.CloseChannels(operation->Parent);
    }

    void OnServiceToClientIoComplete(AsyncOperationSPtr const &operation)
    {
        KWebSocket::CloseStatusCode statusCode;
        KSharedBufferStringA::SPtr statusReason;
        auto error = serviceToClientIoForwarderSPtr_->EndForwardingIo(
            operation,
            statusCode,
            statusReason);

        WriteInfo(
            TraceType,
            "{0} WebSocket I/O completed with error {1}, StatusCode {2}",
            serviceToClientIoForwarderSPtr_->TraceId,
            error,
            (USHORT)statusCode);

        {
            AcquireWriteLock writeLock(pendingIoLock_);
            pendingIoCount_--;
            if (pendingIoCount_ != 1)
            {
                // Just return here as there is already a close in progress. That close completion
                // will complete the ProcessAsyncOperation.
                return;
            }
        }

        owner_.statusCode_ = statusCode;
        owner_.statusReason_ = statusReason;
        owner_.CloseChannels(operation->Parent);
    }

    WebSocketManager &owner_;
    wstring &traceId_;
    WebSocketIoForwarderSPtr clientToServiceIoForwarderSPtr_;
    WebSocketIoForwarderSPtr serviceToClientIoForwarderSPtr_;
    RwLock pendingIoLock_;
    uint pendingIoCount_;
};

AsyncOperationSPtr WebSocketManager::BeginOpen(
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AllocableAsyncOperation::CreateAndStart<OpenAsyncOperation>(
        requestFromClient_->GetThisAllocator(), 
        *this, 
        timeout, 
        callback, 
        parent);
}

ErrorCode WebSocketManager::EndOpen(AsyncOperationSPtr const &operation, ULONG *winHttpError, bool *safeToRetry)
{
    return OpenAsyncOperation::End(operation, winHttpError, safeToRetry);
}

AsyncOperationSPtr WebSocketManager::BeginForwardingIo(
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AllocableAsyncOperation::CreateAndStart<ProcessAsyncOperation>(
        requestFromClient_->GetThisAllocator(),
        *this, 
        callback, 
        parent);
}

ErrorCode WebSocketManager::EndForwardingIo(AsyncOperationSPtr const &operation)
{
    return ProcessAsyncOperation::End(operation);
}

void WebSocketManager::CloseChannels(AsyncOperationSPtr const &parent)
{
    //
    // This could be called from  abort, so capture the thisptr to keep the object alive till 
    // both the channels are closed.
    //
    auto thisSPtr = shared_from_this();

    WriteInfo(
        TraceType,
        "{0} CloseChannels",
        traceId_);


    // Depending on which channel is not closed, send close message to that channel.
    bool clientChannelClosed;
    bool channelToServiceClosed;
    {
        AcquireWriteLock writeLock(channelLock_);
        clientChannelClosed = channelToClientClosed_;
        if (!clientChannelClosed)
        {
            closeOperationCount_++;
            channelToClientClosed_ = true;
        }

        channelToServiceClosed = channelToServiceClosed_;
        if (!channelToServiceClosed)
        {
            closeOperationCount_++;
            channelToServiceClosed_ = true;
        }
    }

    if (!clientChannelClosed && channelToClient_)
    {
        channelToClient_->BeginClose(
            statusCode_,
            statusReason_,
            [this, thisSPtr](AsyncOperationSPtr const &operation)
            {
                channelToClient_->EndClose(operation);
                closeOperationCount_--;
                OnCloseChannelsCompleted(operation->Parent);
            },
            parent);
    }

    if (!channelToServiceClosed && channelToService_)
    {
        channelToService_->BeginClose(
            statusCode_,
            statusReason_,
            [this, thisSPtr](AsyncOperationSPtr const &operation)
            {
                channelToService_->EndClose(operation);
                closeOperationCount_--;
                OnCloseChannelsCompleted(operation->Parent); 
            },
            parent);
    }

    OnCloseChannelsCompleted(parent);
}

void WebSocketManager::OnAbort()
{
    WriteInfo(
        TraceType,
        "{0} OnAbort",
        traceId_);

    CloseChannels(root_.CreateAsyncOperationRoot());
}

void WebSocketManager::OnCloseChannelsCompleted(AsyncOperationSPtr const &operation)
{
    if (closeOperationCount_.load() == 0)
    {
        operation->TryComplete(operation, ErrorCodeValue::Success);
    }
}
