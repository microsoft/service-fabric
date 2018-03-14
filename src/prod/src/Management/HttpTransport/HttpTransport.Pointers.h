// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// Server portion
//

namespace HttpServer
{
    class IHttpServer;
    typedef std::shared_ptr<IHttpServer> IHttpServerSPtr;

    class IHttpRequestHandler;
    typedef std::shared_ptr<IHttpRequestHandler> IHttpRequestHandlerSPtr;

    class IRequestMessageContext;
    typedef std::shared_ptr<IRequestMessageContext> IRequestMessageContextSPtr;
    typedef std::unique_ptr<IRequestMessageContext> IRequestMessageContextUPtr;

    class RequestMessageContext;
    typedef std::unique_ptr<RequestMessageContext> RequestMessageContextUPtr;

    class HttpServerWebSocket;
    typedef std::shared_ptr<HttpServerWebSocket> HttpServerWebSocketSPtr;
}

//
// Client portion
//

namespace HttpClient
{
    class IHttpClientRequest;
    typedef std::shared_ptr<IHttpClientRequest> IHttpClientRequestSPtr;

    class IHttpClient;
    typedef std::shared_ptr<IHttpClient> IHttpClientSPtr;

    class ClientRequest;
    typedef std::shared_ptr<ClientRequest> ClientRequestSPtr;

    class HttpClientWebSocket;
    typedef std::shared_ptr<HttpClientWebSocket> HttpClientWebSocketSPtr;
}

//
// Common
//

namespace HttpCommon
{
    class IWebSocketHandler;
    typedef std::shared_ptr<IWebSocketHandler> IWebSocketHandlerSPtr;
}
