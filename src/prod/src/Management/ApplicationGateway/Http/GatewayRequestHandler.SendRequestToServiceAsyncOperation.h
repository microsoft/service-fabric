// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class GatewayRequestHandler::SendRequestToServiceAsyncOperation
        : public Common::AllocableAsyncOperation
    {
        DENY_COPY(SendRequestToServiceAsyncOperation)
    public:
        SendRequestToServiceAsyncOperation(
            std::wstring const &traceId,
            __in HttpServer::IRequestMessageContextUPtr &requestFromClient,
            __in HttpClient::IHttpClientRequestSPtr &requestToService,
            __in KHashTable<KWString, KString::SPtr> &requestHeaders,
            __in Common::ByteBufferUPtr body,
            __in GatewayRequestHandler &owner,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : AllocableAsyncOperation(callback, parent)
            , traceId_(traceId)
            , requestFromClient_(requestFromClient)
            , requestToService_(requestToService)
            , requestHeaders_(requestHeaders)
            , gateway_(owner.gateway_)
            , fullRequestBody_(move(body))
            , owner_(owner)
            , verb_(requestFromClient->GetVerb())
            , winHttpError_(0)
            , safeToRetry_(false)
            , chunkedEncoding_(false)
            , validationContext_(traceId_, owner_.gateway_.GetGatewayConfig())
        {
        }

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out ULONG *winHttpError,
            __out bool *safeToRetry,
            __out std::wstring & failurePhase);

    protected:
        void OnStart(Common::AsyncOperationSPtr const &thisSPtr);

    private:

        Common::ErrorCode SetHeaders();
        void ProcessHeaders(__out KString::SPtr &headers);

        void OnSendHeadersComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void ReadAndForwardBodyChunk(Common::AsyncOperationSPtr const &thisSPtr);
        void OnReadBodyChunkComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);
        void ForwardBodyChunk(Common::AsyncOperationSPtr const &thisSPtr, KMemRef &currentChunk);
        void OnForwardBodyChunkComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void FinishSendRequest(Common::AsyncOperationSPtr const &thisSPtr);
        void OnFinishSendRequestComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void FinishSendRequestWithBody(Common::AsyncOperationSPtr const &thisSPtr, KMemRef &fullBodyChunk);
        void OnFinishSendRequestWithBodyComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void OnError(Common::AsyncOperationSPtr const &thisSPtr, Common::ErrorCode error);

        KMemRef GetReadBuffer();
        KMemRef GetBufferForForwarding(__in KMemRef &memRef);
        KMemRef GetEndOfFileBuffer();

        std::wstring traceId_;
        HttpServer::IRequestMessageContextUPtr &requestFromClient_;
        HttpClient::IHttpClientRequestSPtr &requestToService_;
        std::wstring verb_;
        ULONG winHttpError_;
        // Operation that failed in the request send pipeline, used for tracing purposes
        std::wstring failurePhase_; 
        bool safeToRetry_;
        bool chunkedEncoding_;
        KBuffer::SPtr bodyBuffer_;
        KHashTable<KWString, KString::SPtr> &requestHeaders_;
        IHttpApplicationGateway & gateway_;
        Common::ByteBufferUPtr fullRequestBody_;
        GatewayRequestHandler &owner_;
        CertValidationContext validationContext_;
    };
}
