// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class WebSocketIoForwarder
        : public Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>
    {
        DENY_COPY(WebSocketIoForwarder)
    public:

        WebSocketIoForwarder(
            HttpCommon::IWebSocketHandlerSPtr const &readChannel,
            HttpCommon::IWebSocketHandlerSPtr const &writeChannel,
            std::wstring traceId,
            ULONG fragmentBufferSize = 4096,
            ULONG bufferPoolSize = 1)
            : readChannelSPtr_(readChannel)
            , writeChannelSPtr_(writeChannel)
            , traceId_(traceId)
            , fragmentBufferSize_(fragmentBufferSize)
            , bufferPoolSize_(bufferPoolSize)
        {
            InitializeBuffers();
        }

        __declspec(property(get = get_TraceId)) std::wstring const &TraceId;
        std::wstring const& get_TraceId() const{ return traceId_; }

        Common::AsyncOperationSPtr BeginForwardingIo(
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        // If the IO was completed because one of the channels was closed, this returns
        // the close status code and reason as well.
        Common::ErrorCode EndForwardingIo(
            Common::AsyncOperationSPtr const &operation,
            __out KWebSocket::CloseStatusCode &statusCode,
            __out KSharedBufferStringA::SPtr &statusReason);

    private:
        void InitializeBuffers();

        class ForwardIoAsyncOperation;

        HttpCommon::IWebSocketHandlerSPtr readChannelSPtr_;
        HttpCommon::IWebSocketHandlerSPtr writeChannelSPtr_;
        std::wstring traceId_;
        ULONG bufferPoolSize_;
        ULONG fragmentBufferSize_;
        KBuffer::SPtr fragmentBuffer_;
    };
}
