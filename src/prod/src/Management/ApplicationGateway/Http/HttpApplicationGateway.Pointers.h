// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class HttpApplicationGatewayImpl;
    typedef std::shared_ptr<HttpApplicationGatewayImpl> HttpApplicationGatewaySPtr;

    class ParsedGatewayUri;
    typedef std::shared_ptr<ParsedGatewayUri> ParsedGatewayUriSPtr;

    class WebSocketManager;
    typedef std::shared_ptr<WebSocketManager> WebSocketManagerSPtr;

    class WebSocketIoForwarder;
    typedef std::shared_ptr<WebSocketIoForwarder> WebSocketIoForwarderSPtr;

    class IServiceResolver;
    typedef std::unique_ptr<IServiceResolver> IServiceResolverUPtr;

    class IHttpApplicationGatewayConfig;

    class IHttpApplicationGatewayEventSource;

	class GatewayRequestHandler;
}
