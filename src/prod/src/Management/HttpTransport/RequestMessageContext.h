// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    //
    // This supports chunked receives and sends. Incase of chunked responses, currently, 
    // it is upto the user to make sure that the Transfer-Encoding header is set and the 
    // chunked responses are in the correct chunk encoding form.
    //
    class RequestMessageContext : public IRequestMessageContext
    {
    public:
        RequestMessageContext(KHttpServerRequest::SPtr &request);
        virtual ~RequestMessageContext();

        //
        // IRequestMessageContext methods.
        // 

        KAllocator& GetThisAllocator() const;

        std::wstring const& GetVerb() const;

        std::wstring GetUrl() const;

        std::wstring GetSuffix() const;

        std::wstring GetClientRequestId() const;

        Common::ErrorCode GetRequestHeader(__in std::wstring const &headerName, __out std::wstring &headerValue) const;

        Common::ErrorCode GetAllRequestHeaders(__out KString::SPtr &) const;

        Common::ErrorCode GetAllRequestHeaders(__out KHashTable<KWString, KString::SPtr> &headers) const;

        KHttpUtils::HttpAuthType GetRequestAuthType() const;

        Common::ErrorCode GetClientToken(__out HANDLE &hToken) const;

        Common::AsyncOperationSPtr BeginGetClientCertificate(
            __in Common::TimeSpan const& timeout,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetClientCertificate(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::CertContextUPtr &certContext) const;

        Common::ErrorCode GetRemoteAddress(__out std::wstring &remoteAddress) const;

        Common::ErrorCode GetRemoteAddress(__out KString::SPtr &remoteAddress) const;

        Common::AsyncOperationSPtr BeginGetMessageBody(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetMessageBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body) const;

        Common::AsyncOperationSPtr BeginGetMessageBodyChunk(
            __in KMemRef &memRef,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const;

        Common::ErrorCode EndGetMessageBodyChunk(
            __in Common::AsyncOperationSPtr const& operation,
            __out KMemRef &memRef) const;

        Common::ErrorCode SetResponseHeader(__in std::wstring const& headerName, __in std::wstring const& headerValue);

        Common::ErrorCode SetAllResponseHeaders(KString::SPtr const&);

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

        Common::AsyncOperationSPtr BeginSendResponseHeaders(
            __in USHORT statusCode,
            __in std::wstring description,
            __in bool bodyExists,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendResponseHeaders(
            Common::AsyncOperationSPtr const& operation);

        //
        // The _Address member of memRef gives the address and the _Param member gives the number of bytes
        // of data to send.
        //
        Common::AsyncOperationSPtr BeginSendResponseChunk(
            KMemRef &memRef,
            bool isLastSegment,
            bool disconnect,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndSendResponseChunk(
            Common::AsyncOperationSPtr const& operation);

        Common::ErrorCode TryParseRequest();

        // For use by the websocket code
        KHttpServerRequest::SPtr GetInnerRequest()
        {
            return kHttpRequest_;
        }

    private:

        Common::ErrorCode SetHeaderInternal(std::wstring const &);
        Common::ErrorCode GetHeadersInternal() const;

        void GetHeaderNameValue(
            std::wstring const &headerString,
            __out std::wstring &headerName,
            __out std::wstring &headerValue) const;

        class ReadBodyAsyncOperation;
        class ReadBodyChunkAsyncOperation;
        class SendResponseAsyncOperation;
        class SendResponseHeadersAsyncOperation;
        class SendResponseBodyChunkAsyncOperation;
        class ReadCertificateAsyncOperation;

        KHttpServerRequest::SPtr kHttpRequest_;
        mutable KHttpServerRequest::AsyncResponseDataContext::SPtr ktlAsyncReqDataContext_;
        std::wstring url_;
        std::wstring suffix_;
        std::wstring verb_;
        std::wstring clientRequestId_;
        mutable KString::SPtr headers_;

        mutable std::shared_ptr<std::map<std::wstring, std::wstring>> headersMap_;
    };
}
