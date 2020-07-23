// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TXRStatefulServiceBase
{
    typedef std::function<Common::ErrorCode(Common::ByteBufferUPtr && body,Common::ByteBufferUPtr & responseBody)> HttpRequestProcessCallback;

    class TestClientRequestHandler
        : public HttpServer::IHttpRequestHandler
    {
    public:
        static std::shared_ptr<TestClientRequestHandler> Create(
            __in HttpRequestProcessCallback const & postRequestCallback);

        Common::AsyncOperationSPtr BeginProcessRequest(
            __in HttpServer::IRequestMessageContextUPtr request,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndProcessRequest(__in Common::AsyncOperationSPtr const& operation);

    private:
        class HandleRequestAsyncOperation;

        TestClientRequestHandler(
            __in HttpRequestProcessCallback const & postRequestCallback);

        HttpRequestProcessCallback postRequestCallback_;
    };

    typedef std::shared_ptr<TestClientRequestHandler> TestClientRequestHandlerSPtr;
}
