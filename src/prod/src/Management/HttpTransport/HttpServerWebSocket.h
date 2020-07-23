// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
#if NTDDI_VERSION >= NTDDI_WIN8 

    //
    // Implements the websocket communication from a http server. This class is intentionally not thread safe.
    //

    class HttpServerWebSocket 
        : public HttpCommon::WebSocketHandler
    {
        DENY_COPY(HttpServerWebSocket)

    public:
        HttpServerWebSocket(
            __in IRequestMessageContext &clientRequest,
            __in KStringViewA const &supportedSubProtocols, // empty string means any protocol.
            __in_opt ULONG receiveBufferSize = 4096,
            __in_opt ULONG sendBufferSize = 4096,
            __in_opt ULONGLONG openHandshakeTimeoutMilliSeconds = 5000, // 5 seconds
            __in_opt ULONGLONG gracefulCloseTimeoutMilliSeconds = 10000, // 10 seconds - WINHTTP_OPTION_WEB_SOCKET_CLOSE_TIMEOUT
            __in_opt ULONGLONG pongKeepAlivePeriodMilliSeconds = 30000, // 30 seconds - WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL
            __in_opt ULONGLONG pingIntervalMilliSeconds = 0, // Dont verify the remote endpoint
            __in_opt ULONGLONG pongTimeoutMilliSeconds = 30000); // 30 seconds - number of milliseconds to wait for a PONG or CLOSE after sending a ping.

        // Initiates the websocket upgrade
        Common::AsyncOperationSPtr BeginOpen(
            HttpCommon::WebSocketCloseReceivedCallback const &closeReceivedCallback,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        // TODO: Should there be a status code and description to identify any errors
        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginClose(
            __in KWebSocket::CloseStatusCode &statusCode,
            __in KSharedBufferStringA::SPtr &statusReason,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const &operation);

        Common::ErrorCode GetRemoteCloseStatus(
            __out KWebSocket::CloseStatusCode &statusCode,
            __out KSharedBufferStringA::SPtr &statusReason);

        KWebSocket::ConnectionStatus GetConnectionStatus();

    private:

        class OpenAsyncOperation;
        class CloseAsyncOperation;

        Common::ErrorCode SetTimingConstants();

        IRequestMessageContext &clientRequest_;
        KStringViewA supportedSubProtocols_;
        ULONG receiveBufferSize_;
        ULONG sendBufferSize_;
        ULONGLONG openHandshakeTimeoutMilliSeconds_;
        ULONGLONG gracefulCloseTimeoutMilliSeconds_;
        ULONGLONG pongKeepAlivePeriodMilliSeconds_;
        ULONGLONG pingIntervalMilliSeconds_;
        ULONGLONG pongTimeoutMilliSeconds_;
        KHttpServerWebSocket::SPtr kWebSocket_;
    };

#endif
}
