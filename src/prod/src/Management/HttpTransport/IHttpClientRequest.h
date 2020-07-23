// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    //
    // This represents the Http request that the client sends to a server. 
    // This supports chunked sends and receives. 
    //
    class IHttpClientRequest
    {
    public:
        virtual ~IHttpClientRequest() {}

        virtual KAllocator& GetThisAllocator() const = 0;

        //
        // Client Request
        //
        virtual std::wstring GetRequestUrl() const = 0;
        virtual BOOLEAN IsSecure() const = 0;

        virtual void SetVerb(std::wstring const &verb) = 0;
        virtual void SetVerb(KHttpUtils::OperationType verb) = 0;

        virtual Common::ErrorCode SetRequestHeaders(std::wstring const &headers) = 0;
        virtual Common::ErrorCode SetRequestHeaders(__in KString::SPtr const &HeaderBlock) = 0;

        virtual Common::ErrorCode SetRequestHeader(
            std::wstring const& headerName,
            std::wstring const& headerValue) = 0;

        virtual Common::ErrorCode SetRequestHeader(
            __in KStringView & headerName,
            __in KStringView & headerValue) = 0;

        virtual Common::ErrorCode SetClientCertificate(__in PCCERT_CONTEXT pCertContext) = 0;

        virtual Common::ErrorCode SetServerCertValidationHandler(__in KHttpCliRequest::ServerCertValidationHandler handler) = 0;

        // Sends headers and body.
        virtual Common::AsyncOperationSPtr BeginSendRequest(
            Common::ByteBufferUPtr body,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::AsyncOperationSPtr BeginSendRequest(
            KBuffer::SPtr &&body,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        // TODO: might need to get additional info to lookup incase of auth failures/challenges
        virtual Common::ErrorCode EndSendRequest(
            __in Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError) = 0;


        virtual Common::AsyncOperationSPtr BeginSendRequestHeaders(
            ULONG totalRequestLength,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendRequestHeaders(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError) = 0;

        //
        // The _Address member of memRef gives the address and the _Param member gives the number of bytes
        // of data to send.
        //
        virtual Common::AsyncOperationSPtr BeginSendRequestChunk(
            __in KMemRef &memRef,
            bool isLastSegment,
            bool disconnect,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndSendRequestChunk(
            Common::AsyncOperationSPtr const& operation,
            __out ULONG *winHttpError) = 0;

        //
        // Server response
        //
        virtual Common::ErrorCode GetResponseStatusCode(
            __out USHORT *statusCode,
            __out std::wstring &description) const = 0;

        virtual Common::ErrorCode GetResponseHeader(
            __in std::wstring const& headerName,
            __out std::wstring &headerValue) const = 0;
        
        virtual Common::ErrorCode GetResponseHeader(
            __in std::wstring const& headerName,
            __out KString::SPtr &headerValue) const = 0;

        virtual Common::ErrorCode GetAllResponseHeaders(__out std::wstring &headers) = 0;
        virtual Common::ErrorCode GetAllResponseHeaders(__out KString::SPtr &headerValue) = 0;

        virtual Common::AsyncOperationSPtr BeginGetResponseBody(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndGetResponseBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out Common::ByteBufferUPtr &body)= 0;

        virtual Common::ErrorCode EndGetResponseBody(
            __in Common::AsyncOperationSPtr const& operation,
            __out KBuffer::SPtr &body) = 0;

        //
        // Caller has to allocate the buffer indicated in KMemRef. The size of 
        // the buffer is given by KMemRef._Size. The KMemRef returned in EndGetResponseChunk 
        // has the _Param member set to the number of bytes written.
        //
        virtual Common::AsyncOperationSPtr BeginGetResponseChunk(
            __in KMemRef &mem,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndGetResponseChunk(
            Common::AsyncOperationSPtr const& operation,
            __out KMemRef &mem,
            __out ULONG *winHttpError) = 0;
    };
}
