// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    //
    // This implementation supports chunked sends and receives. In case of chunked sends, 
    // it is upto the user to set the Transfer-Encoding header and send the chunks in the 
    // correct chunk encoded form.
    //
    class ClientRequest 
        : public IHttpClientRequest
        , public std::enable_shared_from_this<ClientRequest>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ClientRequest)

    public:

        static Common::ErrorCode CreateClientRequest(
            KHttpClient::SPtr &client,
            std::wstring const &requestUri,
            __in KAllocator &ktlAllocator,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            bool allowRedirects,
            bool enableCookies,
            bool enableWinauthAutoLogon,
            __out IHttpClientRequestSPtr &clientRequestSPtr);

        virtual ~ClientRequest() {}

        Common::ErrorCode Initialize(
            KHttpClient::SPtr &client,
            __in KAllocator &ktlAllocator,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            bool allowRedirects,
            bool enableCookies,
            bool enableWinauthAutoLogon);

        //
        // Client Request
        //

        KAllocator& GetThisAllocator() const;

        std::wstring GetRequestUrl() const;

        BOOLEAN IsSecure() const;

        void SetVerb(std::wstring const &verb);
        
        void SetVerb(KHttpUtils::OperationType verb);

        Common::ErrorCode SetRequestHeader(
            std::wstring const& headerName,
            std::wstring const& headerValue);

        Common::ErrorCode SetRequestHeader(
            __in KStringView & headerName,
            __in KStringView & headerValue);

        Common::ErrorCode SetRequestHeaders(std::wstring const &headers);
        Common::ErrorCode SetRequestHeaders(__in KString::SPtr const &HeaderBlock);

        Common::ErrorCode SetClientCertificate(PCCERT_CONTEXT cert_context);

        Common::ErrorCode SetServerCertValidationHandler(__in KHttpCliRequest::ServerCertValidationHandler handler);

        Common::AsyncOperationSPtr BeginSendRequest(
            Common::ByteBufferUPtr body,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::AsyncOperationSPtr BeginSendRequest(
            KBuffer::SPtr &&body,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        // TODO: might need to get additional info to lookup incase of auth failures/challenges
        Common::ErrorCode EndSendRequest(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError);

        //
        // Multipart send
        //
        Common::AsyncOperationSPtr BeginSendRequestHeaders(
            ULONG totalRequestlength,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendRequestHeaders(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError);

        //
        // The _Address member of memRef gives the address and the _Param member gives the number of bytes
        // of data to send.
        //
        Common::AsyncOperationSPtr BeginSendRequestChunk(
            __in KMemRef &memRef,
            bool isLastSegment,
            bool disconnect,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendRequestChunk(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError);

        //
        // Server response
        //
        Common::ErrorCode GetResponseStatusCode(
            __out USHORT *statusCode,
            __out std::wstring &description) const;

        Common::ErrorCode GetResponseHeader(
            __in std::wstring const& headerName,
            __out std::wstring &headerValue) const;

        Common::ErrorCode GetResponseHeader(
            __in std::wstring const& headerName,
            __out KString::SPtr &headerValue) const;

        Common::ErrorCode GetAllResponseHeaders(__out std::wstring &headers);
        Common::ErrorCode GetAllResponseHeaders(__out KString::SPtr &headerValue);

        //
        // Fails with ErrorCodeValue::NotFound if there is no entity body.
        //
        Common::AsyncOperationSPtr BeginGetResponseBody(
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndGetResponseBody(
            Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body);

        Common::ErrorCode EndGetResponseBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out KBuffer::SPtr &body);

        //
        // Caller of this API has to allocate the buffer indicated in KMemRef. The size of 
        // the buffer is given by KMemRef._Size. The KMemRef returned in EndGetResponseChunk 
        // has the _Param member set to the number of bytes written.
        //
        Common::AsyncOperationSPtr BeginGetResponseChunk(
            __in KMemRef &mem,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndGetResponseChunk(
            Common::AsyncOperationSPtr const& operation,
            __out KMemRef &mem,
            __out ULONG *winHttpError);

    private:
        ClientRequest(std::wstring const &requestUri)
            : requestUri_(requestUri)
            , enable_shared_from_this()
        {}

        class SendRequestAsyncOperation;
        class SendRequestChunkAsyncOperation;
        class SendRequestHeaderAsyncOperation;
        class GetResponseBodyAsyncOperation;
        class GetResponseBodyChunkAsyncOperation;

        std::wstring requestUri_;
        std::wstring requestVerb_;

        KHttpCliRequest::SPtr kHttpRequest_;
        KHttpCliRequest::AsyncRequestDataContext::SPtr asyncRequestDataContext_;
        mutable KHttpCliRequest::Response::SPtr kHttpResponse_;
        KNetworkEndpoint::SPtr kEndpoint_;
        ULONG winHttpError_;
    };
}
