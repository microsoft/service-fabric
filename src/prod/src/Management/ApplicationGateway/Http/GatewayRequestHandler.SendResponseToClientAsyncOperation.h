// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class GatewayRequestHandler::SendResponseToClientAsyncOperation
        : public Common::AllocableAsyncOperation
    {
        DENY_COPY(SendResponseToClientAsyncOperation)
    public:
        SendResponseToClientAsyncOperation(
            std::wstring const &traceId,
            __in HttpServer::IRequestMessageContextUPtr &requestFromClient,
            __in HttpClient::IHttpClientRequestSPtr &requestToService,
            __in Common::DateTime const& startTime,
            IHttpApplicationGateway & gateway,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : AllocableAsyncOperation(callback, parent)
            , traceId_(traceId)
            , requestFromClient_(requestFromClient)
            , requestToService_(requestToService)
            , requestStartTime_(startTime)
            , gateway_(gateway)
            , winHttpError_(0)
            , chunkedEncoding_(false)
            , isLastSegment_(false)
        {
        }

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out ULONG *winHttpError,
            __out std::wstring & failurePhase);

    protected:
        void OnStart(Common::AsyncOperationSPtr const &thisSPtr);

    private:

        Common::ErrorCode SetHeaders();

        void SendHeaders(Common::AsyncOperationSPtr const &thisSPtr);
        void OnSendHeadersComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void ReadAndForwardBodyChunk(Common::AsyncOperationSPtr const &thisSPtr);
        void OnReadBodyChunkComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);
        void OnForwardBodyChunkComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void FinishSendRequest(Common::AsyncOperationSPtr const &thisSPtr);
        void OnFinishSendRequestComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void OnError(Common::AsyncOperationSPtr const &thisSPtr, Common::ErrorCode error);
        void OnDisconnect(Common::AsyncOperationSPtr const &thisSPtr, ULONG winHttpError);

        KMemRef GetReadBuffer();
        KMemRef GetBufferForForwarding(__in KMemRef &memRef);
        KMemRef GetEndOfFileBuffer();

        HttpServer::IRequestMessageContextUPtr &requestFromClient_;
        HttpClient::IHttpClientRequestSPtr &requestToService_;
        Common::DateTime requestStartTime_;
        KBuffer::SPtr bodyBuffer_;
        bool isLastSegment_;
        bool chunkedEncoding_;
        ULONG winHttpError_;
        std::wstring traceId_;
        // Operation that failed in the send response to client pipeline, used for tracing purposes
        std::wstring failurePhase_;
        IHttpApplicationGateway & gateway_;
    };
}
