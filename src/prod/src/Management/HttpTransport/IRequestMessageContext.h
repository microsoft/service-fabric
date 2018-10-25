// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    //
    // This represents the Http request received from the client. This supports chunked requests 
    // and responses.
    //
#if !defined(PLATFORM_UNIX)

    class IRequestMessageContext
    {
    public:
        virtual ~IRequestMessageContext() {};

        virtual KAllocator& GetThisAllocator() const = 0;

        //
        // Methods corresponding to the incoming client request.
        //
        virtual std::wstring const& GetVerb() const = 0;

        virtual std::wstring GetUrl() const = 0;

        virtual std::wstring GetSuffix() const = 0;

        virtual std::wstring GetClientRequestId() const = 0;

        virtual Common::ErrorCode GetRequestHeader(
            __in std::wstring const& headerName,
            __out std::wstring &headerValue) const = 0;

        virtual Common::ErrorCode GetAllRequestHeaders(__out KString::SPtr &) const = 0;

        virtual Common::ErrorCode GetAllRequestHeaders(__out KHashTable<KWString, KString::SPtr> &headers) const = 0;

        virtual KHttpUtils::HttpAuthType GetRequestAuthType() const = 0;

        virtual Common::AsyncOperationSPtr BeginGetMessageBody(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetMessageBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body) const = 0;

        //
        // Caller has to allocate the buffer indicated in KMemRef. The size of 
        // the buffer is given by KMemRef._Size. The KMemRef returned in EndGetMessageBodyChunk 
        // has the _Param member set to the number of bytes written.
        //
        virtual Common::AsyncOperationSPtr BeginGetMessageBodyChunk(
            __in KMemRef &memRef,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetMessageBodyChunk(
            __in Common::AsyncOperationSPtr const& operation,
            __out KMemRef &memRef) const = 0;

        //
        // The handle is cleaned up when the request goes out of scope.
        //
        virtual Common::ErrorCode GetClientToken(__out HANDLE &hToken) const = 0;

        virtual Common::AsyncOperationSPtr BeginGetClientCertificate(
            __in Common::TimeSpan const& timeout,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetClientCertificate(
            __in Common::AsyncOperationSPtr const& operation,            
            __out Common::CertContextUPtr &certContext) const = 0;

        virtual Common::ErrorCode GetRemoteAddress(__out std::wstring &remoteAddress) const = 0;
        
        virtual Common::ErrorCode GetRemoteAddress(__out KString::SPtr &remoteAddress) const = 0;

        //
        // Methods corresponding to the server response. 
        //

        virtual Common::ErrorCode SetResponseHeader(
            __in std::wstring const& headerName,
            __in std::wstring const& headerValue) = 0;

        virtual Common::ErrorCode SetAllResponseHeaders(KString::SPtr const&) = 0;

        virtual Common::AsyncOperationSPtr BeginSendResponse(
            __in Common::ErrorCode operationStatus,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::AsyncOperationSPtr BeginSendResponse(
            __in USHORT statusCode,
            __in std::wstring description,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendResponse(
            __in Common::AsyncOperationSPtr const& operation) = 0;

        virtual Common::AsyncOperationSPtr BeginSendResponseHeaders(
            __in USHORT statusCode,
            __in std::wstring description,
            __in bool bodyExists,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendResponseHeaders(
            Common::AsyncOperationSPtr const& operation) = 0;

        //
        // disconnect parameter can only be true when the isLastSegment parameter is true.
        //
        virtual Common::AsyncOperationSPtr BeginSendResponseChunk(
            __in KMemRef &memRef,
            bool isLastSegment,
            bool disconnect,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendResponseChunk(
            Common::AsyncOperationSPtr const& operation) = 0;
    };

#else

    class IRequestMessageContext
    {
    public:
        virtual ~IRequestMessageContext() {};

        //
        // Methods corresponding to the incoming client request.
        //
        virtual std::wstring const& GetVerb() const = 0;

        virtual std::wstring GetUrl() const = 0;

        virtual std::wstring GetSuffix() const = 0;

        virtual std::wstring GetClientRequestId() const = 0;

        virtual Common::ErrorCode GetRequestHeader(
            __in std::wstring const& headerName,
            __out std::wstring &headerValue) const = 0;

        virtual Common::AsyncOperationSPtr BeginGetMessageBody(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetMessageBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body) const = 0;

        virtual Common::AsyncOperationSPtr BeginGetFileFromUpload(
            __in ULONG fileSize,
            __in ULONG maxEntityBodyForUploadChunkSize,
            __in ULONG defaultEntityBodyForUploadChunkSize,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetFileFromUpload(
            __in Common::AsyncOperationSPtr const& operation,
            __out std::wstring & uniqueFileName) const = 0;
        //
        // The handle is cleaned up when the request goes out of scope.
        //
        virtual Common::ErrorCode GetClientToken(__out HANDLE &hToken) const = 0;

        virtual Common::AsyncOperationSPtr BeginGetClientCertificate(
			__in Common::TimeSpan const& timeout,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) const = 0;

        virtual Common::ErrorCode EndGetClientCertificate(
	    __in Common::AsyncOperationSPtr const& operation,
	    __out SSL** sslContext) const =0;

        virtual Common::ErrorCode GetRemoteAddress(__out std::wstring &remoteAddress) const = 0;

        virtual Common::ErrorCode SetResponseHeader(
            __in std::wstring const& headerName,
            __in std::wstring const& headerValue) = 0;

        virtual Common::AsyncOperationSPtr BeginSendResponse(
            __in Common::ErrorCode operationStatus,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::AsyncOperationSPtr BeginSendResponse(
            __in USHORT statusCode,
            __in std::wstring description,
            __in Common::ByteBufferUPtr buffer,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendResponse(
            __in Common::AsyncOperationSPtr const& operation) = 0;
    };
#endif

}
