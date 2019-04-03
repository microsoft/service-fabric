// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class GatewayRequestHandler 
        : public HttpServer::IHttpRequestHandler
    {
    public:
        GatewayRequestHandler(IHttpApplicationGateway & gateway)
            : gateway_(gateway)
        {
        }

        //
        // IHttpRequestHandler methods
        //
        Common::AsyncOperationSPtr BeginProcessRequest(
            __in HttpServer::IRequestMessageContextUPtr request,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndProcessRequest(Common::AsyncOperationSPtr const& operation);

        //
        // Begin processing the request
        //
        Common::AsyncOperationSPtr BeginProcessReverseProxyRequest(
            __in std::wstring const& traceId,
            __in HttpServer::IRequestMessageContextUPtr &request,
            __in Common::ByteBufferUPtr && body,
            std::wstring const & forwardingUri,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndProcessReverseProxyRequest(Common::AsyncOperationSPtr const& operation);

        //
        // Request forwarding
        // Take the raw PCCERT_CONTEXT here, ProcessRequestAsyncOperation will manage the lifetime of the CERT_CONTEXT (to handle re-resolve, retries).
        //
        Common::AsyncOperationSPtr BeginSendRequestToService(
            __in std::wstring const &traceId,
            __in HttpServer::IRequestMessageContextUPtr &requestFromClient,
            __in HttpClient::IHttpClientRequestSPtr &requestToService,
            __in KHashTable<KWString, KString::SPtr> &requestHeaders,
            __in Common::ByteBufferUPtr && body,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        //
        // Requests to service might not be idempotent. So they can be retried only 
        // if the service hasn't started processing the request. The out params
        // 'winHttpError' and 'safeToRetry' together should be used to decide if
        // the request should be retried or not.
        //
        Common::ErrorCode EndSendRequestToService(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError,
            __out bool *safeToRetry,
            __out std::wstring& failurePhase);

        Common::AsyncOperationSPtr BeginSendResponseToClient(
            __in std::wstring const &traceId,
            __in HttpServer::IRequestMessageContextUPtr &requestFromClient,
            __in HttpClient::IHttpClientRequestSPtr &requestToService,
            __in Common::DateTime const& startTime,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendResponseToClient(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError,
            __out std::wstring& failurePhase);

        static bool IsRetriableVerb(std::wstring const & verb);
        static bool IsRetriableError(ULONG winhttpError);
        static bool IsConnectionError(ULONG winhttpError);
        static bool IsRetriableError(Common::ErrorCode &error);

        //
        // Given a WinHTTP error code, convert it to an appropriate HTTP status code that the reverse proxy can
        // return to the client.
        //
        void ErrorCodeToHttpStatus(
            __in ULONG winhttpError,
            __out KHttpUtils::HttpResponseCode &httpStatus,
            __out std::wstring &httpStatusLine) const;

        //
        // KTL Http client calls this handler to verify the certificate presented by the service.
        // Context for this delegate: pointer to string representing the  trace ID to be printed by this static method.
        // pCertContext:  Pointer to the CERT_CONTEXT structure of the server certificate. KTL will pass this context
        // and own its memory management
        static BOOLEAN ValidateServerCertificate(PVOID Context, PCCERT_CONTEXT pCertContext);

        void AppendHeader(
            KString::SPtr &headers,
            KWString &headerName,
            KString::SPtr &headerValue,
            bool isLastHeader = false);

        void AppendHeader(
            KString::SPtr &headers,
            KWString &headerName,
            LPCWSTR headerValue,
            bool isLastHeader = false);

        void ProcessHeaders(
            __in KHashTable<KWString, KString::SPtr> &requestHeaders,
            __in KAllocator &allocator,
            __in KString::SPtr clientAddress,
            __in bool isWebSocketRequest,
            __in std::wstring &traceId,
            __out KString::SPtr &headers);

    private:
        class HandlerAsyncOperation;
        class ProcessRequestAsyncOperation;
        class SendRequestToServiceAsyncOperation;
        class SendResponseToClientAsyncOperation;
        class WebSocketCommunicationAsyncOperation;

        IHttpApplicationGateway & gateway_;
    };

    class CertValidationContext
    {
    public:
        CertValidationContext(std::wstring const & traceId, IHttpApplicationGatewayConfig const & config)
            : traceId_(traceId)
            , config_(config)
        {
        }

        std::wstring const & TraceId()
        {
            return traceId_;
        }

        IHttpApplicationGatewayConfig const & Config()
        {
            return config_;
        }

    private:
        std::wstring const & traceId_;
        IHttpApplicationGatewayConfig const & config_;
    };
}
