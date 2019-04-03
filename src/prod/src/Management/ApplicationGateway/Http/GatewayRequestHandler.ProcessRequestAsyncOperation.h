// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class GatewayRequestHandler::ProcessRequestAsyncOperation
        : public Common::AllocableAsyncOperation
    {
        DENY_COPY(ProcessRequestAsyncOperation)
    public:
        ProcessRequestAsyncOperation(
            GatewayRequestHandler &owner,
            HttpServer::IRequestMessageContextUPtr &request,
            Common::ByteBufferUPtr body,
            std::wstring const & forwardingUri,
            std::wstring const &traceId,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : AllocableAsyncOperation(callback, parent)
            , owner_(owner)
            , requestFromClient_(request)
            , body_(std::move(body))
            , requestToService_()
            , timeoutHelper_(Common::TimeSpan::MaxValue)
            , previousRspResult_()
            , resolveRetryTimerLock_()
            , forwardingUri_(forwardingUri)
            , traceId_(traceId)
            , startTime_(Common::DateTime::Now())
            , requestHeaders_(requestFromClient_->GetThisAllocator())
            , resolveCounter_(0)
            , validationContext_(traceId_, owner_.gateway_.GetGatewayConfig())
        {
            requestUrl_ = requestFromClient_->GetUrl();
        }

    protected:
        void OnStart(Common::AsyncOperationSPtr const &thisSPtr);
        void OnCancel();
        void OnCompleted();

    private:
        bool PrepareRequest(Common::AsyncOperationSPtr const &operation);

        void DoResolve(Common::AsyncOperationSPtr const &thisSPtr);
        void ScheduleResolve(Common::AsyncOperationSPtr const &thisSPtr);
        void OnResolveComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void StartWebsocketCommunication(Common::AsyncOperationSPtr const &thisSPtr, wstring const &forwardingUri);
        void OnWebSocketUpgradeComplete(Common::AsyncOperationSPtr const &thisSPtr, bool expectedCompletedSynchronously);
        void OnWebSocketCommunicationComplete(Common::AsyncOperationSPtr const &thisSPtr, bool expectedCompletedSynchronously);
        void OnWebSocketCloseComplete(Common::AsyncOperationSPtr const &thisSPtr, bool expectedCompletedSynchronously);

        void SendRequestToService(Common::AsyncOperationSPtr const &thisSPtr);
        void OnSendRequestToServiceComplete(Common::AsyncOperationSPtr const &thisSPtr, bool expectedCompletedSynchronously);

        void SendResponseToClient(Common::AsyncOperationSPtr const &thisSPtr);
        void OnSendResponseToClientComplete(Common::AsyncOperationSPtr const &thisSPtr, bool expectedCompletedSynchronously);

        // The processRequestPhase and additionalTraceStr are for tracing purposes.

        // Use this overload for any fabric error codes.Any trace logged by this API will denote error in request processing logic).
        void OnError(Common::AsyncOperationSPtr const& thisSPtr, Common::ErrorCode error, std::wstring const& processRequestPhase = L"", std::wstring const& additionalTraceStr = L"");

        // Use this overload to forward failure status code returned by the service. Avoid translating a fabric error code to status code to use this overload. 
        // Any trace by this API will imply service sent that status code. or an error occurred while interacting with the service
        void OnForwardRequestError(Common::AsyncOperationSPtr const& thisPtr, USHORT statusCode, std::wstring const &statusDescription, std::wstring const& processRequestPhase = L"", std::wstring const& additionalTraceStr = L"");
        void OnDisconnect(Common::AsyncOperationSPtr const& thisSPtr, ULONG winHttpError, std::wstring const& failurePhase = L"");

        void DisconnectAndScheduleResolve(Common::AsyncOperationSPtr const& thisSPtr);

        void CancelResolveTimer();

        bool IsWebSocketUpgradeRequest();

        GatewayRequestHandler &owner_;
        HttpServer::IRequestMessageContextUPtr &requestFromClient_;
        Common::ByteBufferUPtr body_;
        std::wstring requestUrl_;
        const std::wstring forwardingUri_;
        HttpClient::IHttpClientRequestSPtr requestToService_;
        ParsedGatewayUriSPtr parsedUri_;
        Common::TimeoutHelper timeoutHelper_;
        Api::IResolvedServicePartitionResultPtr previousRspResult_;

        Common::TimerSPtr resolveRetryTimer_;
        Common::RwLock resolveRetryTimerLock_;
        WebSocketManagerSPtr webSocketManager_;

        std::wstring traceId_;
        // Capture the time when we started processing this request, for tracing purposes
        Common::DateTime startTime_;
        // Track the number of resolve attempts that were successful for this request 
        // The reason for the re-resolves in such cases would be some retriable request processing/forwarding error
        // This counter is mainly to trace the resolve count in critical channel instead of emitting a trace per resolve.
        uint resolveCounter_;
        KHashTable<KWString, KString::SPtr> requestHeaders_;
        CertValidationContext validationContext_;
    };

}
