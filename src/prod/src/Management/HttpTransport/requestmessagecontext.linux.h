// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace web 
{
    namespace http
    {
        class http_request;
        class http_response;
        class http_headers;
    }
}

namespace HttpServer
{
    typedef std::unique_ptr<web::http::http_response> httpResponseUPtr;
    typedef std::unique_ptr<web::http::http_request> httpRequestUPtr; 
    typedef unsigned short status_code;

    class RequestMessageContext
        : public IRequestMessageContext
    {
    public:
        RequestMessageContext();

        RequestMessageContext(web::http::http_request & message);
        
        virtual ~RequestMessageContext();

        //
        // IRequestMessageContext methods.
        // 

        std::wstring const& GetVerb() const;

        std::wstring GetUrl() const;

        std::wstring GetSuffix() const;

        std::wstring GetClientRequestId() const;

        Common::ErrorCode GetRequestHeader(__in std::wstring const &headerName, __out std::wstring &headerValue) const;

        Common::ErrorCode GetClientToken(__out HANDLE &hToken) const;

        Common::AsyncOperationSPtr BeginGetClientCertificate(
            __in Common::TimeSpan const& timeout,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetClientCertificate(
        __in Common::AsyncOperationSPtr const& operation,
            __out SSL** sslContext) const;

        Common::ErrorCode GetRemoteAddress(__out std::wstring &remoteAddress) const;

        Common::AsyncOperationSPtr BeginGetMessageBody(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetMessageBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body) const;
            
        Common::AsyncOperationSPtr BeginGetFileFromUpload(
            __in ULONG fileSize,
            __in ULONG maxEntityBodyForUploadChunkSize,
            __in ULONG defaultEntityBodyForUploadChunkSize,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetFileFromUpload(
            __in Common::AsyncOperationSPtr const& operation,
            __out std::wstring & uniqueFileName) const;

        Common::ErrorCode SetResponseHeader(__in std::wstring const& headerName, __in std::wstring const& headerValue);

        Common::AsyncOperationSPtr BeginSendResponse(
            __in Common::ErrorCode operationStatus,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::AsyncOperationSPtr BeginSendResponse(
            __in USHORT statusCode,
            __in std::wstring description,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendResponse(
            __in Common::AsyncOperationSPtr const& operation);

        Common::ErrorCode TryParseRequest();

        static void HandleException(std::exception_ptr eptr, std::wstring const& clientRequestId);

    private:
        class GetFileFromUploadAsyncOperation;
        class SendResponseAsyncOperation;
        std::wstring url_;
        std::wstring suffix_;
        std::wstring verb_;
        std::wstring clientRequestId_;

        httpRequestUPtr requestUPtr_;
        httpResponseUPtr responseUPtr_;
    };
}
