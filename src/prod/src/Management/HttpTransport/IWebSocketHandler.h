// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpCommon
{
    typedef std::function<void()> WebSocketCloseReceivedCallback;

    //
    // This represents a websocket that can be used to send and receive requests. Currently only fragment send
    // and receives are exposed.
    //

    class IWebSocketHandler
    {
    public:
        virtual ~IWebSocketHandler() {}

        virtual Common::AsyncOperationSPtr BeginOpen(
            __in WebSocketCloseReceivedCallback const &closeReceivedCallback,
            __in Common::AsyncCallback const &callback,
            __in Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndOpen(
            __in Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginSendFragment(
            __in KBuffer::SPtr &buffer,
            __in ULONG bytesToSend,
            __in bool isFinalFragment,
            __in KWebSocket::MessageContentType &contentType,
            __in Common::AsyncCallback const &callback,
            __in Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndSendFragment(
            __in Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginReceiveFragment(
            __in KBuffer::SPtr &allocatedBuffer,
            __in Common::AsyncCallback const &callback,
            __in Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndReceiveFragment(
            __in Common::AsyncOperationSPtr const &operation,
            __out KBuffer::SPtr &buffer,
            __out ULONG *bytesReceived,
            __out bool &isFinalFragment,
            __out KWebSocket::MessageContentType &contentType) = 0;

        virtual Common::AsyncOperationSPtr BeginClose(
            __in KWebSocket::CloseStatusCode &statusCode,
            __in KSharedBufferStringA::SPtr &statusReason,
            __in Common::AsyncCallback const &callback,
            __in Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndClose(
            __in Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::ErrorCode GetRemoteCloseStatus(
            __out KWebSocket::CloseStatusCode &statusCode,
            __out KSharedBufferStringA::SPtr &statusReason) = 0;

        virtual KWebSocket::ConnectionStatus GetConnectionStatus() = 0;
    };
}
