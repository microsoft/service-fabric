// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class WebSocketManager
        : public Common::StateMachine
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>
        , public std::enable_shared_from_this<WebSocketManager>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::HttpApplicationGateway>::WriteTrace;

        DENY_COPY(WebSocketManager)

        STATEMACHINE_STATES_05(Created, Opening, Opened, Processing, Closed);
        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening);
        STATEMACHINE_TRANSITION(Processing, Opened);
        STATEMACHINE_TRANSITION(Closed, Opened | Processing);
        STATEMACHINE_ABORTED_TRANSITION(Opening | Opened | Processing);
        STATEMACHINE_TERMINAL_STATES(Aborted | Closed);

    public:

        //
        // If WebSocketManager open fails because of connection error to the service, the GatewayRequestHandler
        // can re-resolve and retry the request. So the ownership requestmessagecontext is not transferred
        // to WebSocketManager. TODO: make this cleaner
        //
        WebSocketManager(
            std::wstring const &traceId,
            __in HttpServer::IRequestMessageContextUPtr &requestFromClient,
            std::wstring const &serviceEndpoint,
            Common::ComponentRoot const &root,
            KString::SPtr headers,
            PCCERT_CONTEXT certContext,
            KHttpClientWebSocket::ServerCertValidationHandler handler)
            : StateMachine(Created)
            , traceId_(traceId)
            , requestFromClient_(requestFromClient)
            , serviceEndpoint_(serviceEndpoint)
            , statusCode_(KWebSocket::CloseStatusCode::NormalClosure)
            , channelToClientClosed_(false)
            , channelToServiceClosed_(false)
            , root_(root)
            , headers_(headers)
            , certContext_(certContext)
            , serverCertValidationHandler_(handler)
        {
        }

        Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan const &timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const &operation,
            __out ULONG *winHttpError,
            __out bool *safeToRetry);

        Common::AsyncOperationSPtr BeginForwardingIo(
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndForwardingIo(Common::AsyncOperationSPtr const &operation);

    protected:
        virtual void OnAbort();

    private:
        void CloseChannels(Common::AsyncOperationSPtr const &parent);
        void OnCloseChannelsCompleted(Common::AsyncOperationSPtr const &operation);

        class OpenAsyncOperation;
        class ProcessAsyncOperation;

        HttpServer::IRequestMessageContextUPtr &requestFromClient_;
        std::wstring serviceEndpoint_;
        ULONG winHttpError_;

        std::wstring traceId_;
        HttpServer::HttpServerWebSocketSPtr channelToClient_;
        HttpClient::HttpClientWebSocketSPtr channelToService_;
        bool channelToClientClosed_;
        KWebSocket::CloseStatusCode statusCode_;
        KSharedBufferStringA::SPtr statusReason_;
        bool channelToServiceClosed_;
        KString::SPtr headers_;
        PCCERT_CONTEXT certContext_;
        KHttpClientWebSocket::ServerCertValidationHandler serverCertValidationHandler_;
        Common::atomic_long closeOperationCount_;
        Common::RwLock channelLock_;
        Common::ComponentRoot const &root_;
    };
}
