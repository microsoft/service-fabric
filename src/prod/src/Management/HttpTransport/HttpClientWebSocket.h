// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
#if NTDDI_VERSION >= NTDDI_WIN8 

    class HttpClientWebSocket
        : public HttpCommon::WebSocketHandler
    {
        DENY_COPY(HttpClientWebSocket)

    public:
        HttpClientWebSocket::HttpClientWebSocket(
            __in std::wstring const &requestUri,
            __in std::wstring const &supportedSubProtocols, // empty string means any protocol.
            __in KAllocator &ktlAllocator,
            __in_opt ULONG receiveBufferSize = 4096,
            __in_opt ULONG sendBufferSize = 4096,
            __in_opt ULONGLONG openHandshakeTimeoutMilliSeconds = 5000, // 5 seconds
            __in_opt ULONGLONG gracefulCloseTimeoutMilliSeconds = 10000, // 10 seconds - WINHTTP_OPTION_WEB_SOCKET_CLOSE_TIMEOUT
            __in_opt KString::SPtr headers = nullptr, // headers to be sent in the websocket handshake
            __in_opt PCCERT_CONTEXT pCertContext = NULL,
            __in_opt KHttpClientWebSocket::ServerCertValidationHandler handler = NULL,
            __in_opt bool allowRedirects = true,
            __in_opt bool enableCookies = true
            );

        Common::AsyncOperationSPtr BeginOpen(
            HttpCommon::WebSocketCloseReceivedCallback const &closeReceivedCallback,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

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

        Common::ErrorCode GetActiveSubprotocol(
            __out KStringViewA &activeSubProtocol);

        KWebSocket::ConnectionStatus GetConnectionStatus();

        ULONG GetWinHttpError();

        ULONG GetHttpResponseCode();

        Common::ErrorCode GetResponseHeader(
            __in std::wstring const& headerName,
            __out std::wstring &headerValue);

    private:

        class OpenAsyncOperation;
        class CloseAsyncOperation;

        Common::ErrorCode SetTimingConstants();

        std::wstring supportedSubProtocols_;
        ULONG receiveBufferSize_;
        ULONG sendBufferSize_;
        ULONGLONG openHandshakeTimeoutMilliSeconds_;
        ULONGLONG gracefulCloseTimeoutMilliSeconds_;
        KString::SPtr headers_;

        KHttpClientWebSocket::SPtr kWebSocket_;
        KAllocator& ktlAllocator_;

        PCCERT_CONTEXT pCertContext_;
        KHttpClientWebSocket::ServerCertValidationHandler serverCertValidationHandler_;

        bool allowRedirects_;
        bool enableCookies_;
    };

#endif
}
