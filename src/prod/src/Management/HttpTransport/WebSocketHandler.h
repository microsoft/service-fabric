// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpCommon
{
#if NTDDI_VERSION >= NTDDI_WIN8 

    class WebSocketHandler
        : public IWebSocketHandler
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(WebSocketHandler)

    public:

        virtual Common::AsyncOperationSPtr BeginOpen(
            HttpCommon::WebSocketCloseReceivedCallback const &closeReceivedCallback,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        // TODO: Should there be a status code and description to identify any errors
        virtual Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const &operation) = 0;

        Common::AsyncOperationSPtr BeginSendFragment(
            __in KBuffer::SPtr &buffer,
            __in ULONG bytesToSend,
            __in bool isFinalFragment,
            __in KWebSocket::MessageContentType &contentType,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndSendFragment(
            Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginReceiveFragment(
            __in KBuffer::SPtr &allocatedBuffer,
            __in Common::AsyncCallback const &callback,
            __in Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndReceiveFragment(
            __in Common::AsyncOperationSPtr const &operation,
            __out KBuffer::SPtr &buffer,
            __out ULONG *bytesReceived,
            __out bool &isFinalFragment,
            __out KWebSocket::MessageContentType &contentType);

        virtual Common::AsyncOperationSPtr BeginClose(
            __in KWebSocket::CloseStatusCode &statusCode,
            __in KSharedBufferStringA::SPtr &statusReason,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const &operation) = 0;

        Common::ErrorCode GetRemoteCloseStatus(
            __out KWebSocket::CloseStatusCode &statusCode,
            __out KSharedBufferStringA::SPtr &statusReason) = 0;

    protected:
        WebSocketHandler(
            Common::StringLiteral const &traceType,
            std::wstring const &requestUri)
            : traceType_(traceType)
            , requestUri_(requestUri)
        {
        }

        // Called by the ktl websocket layer when websocket close is received.
        VOID CloseReceived(__in_opt KWebSocket& WebSocket);

        std::wstring requestUri_;
        HttpCommon::WebSocketCloseReceivedCallback closeReceivedCallback_;
        KWebSocket::ReceiveFragmentOperation::SPtr receiveFragmentKtlOperation_;
        KWebSocket::SendFragmentOperation::SPtr sendFragmentKtlOperation_;
    private:

        class ReceiveFragmentAsyncOperation;
        class SendFragmentAsyncOperation;

        Common::StringLiteral traceType_;
    };

#endif
}
