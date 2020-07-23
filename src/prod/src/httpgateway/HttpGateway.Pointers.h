// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{ 
    class HttpGatewayImpl;
    typedef std::shared_ptr<HttpGatewayImpl> HttpGatewayImplSPtr;

    class RequestHandlerBase;
    typedef std::shared_ptr<RequestHandlerBase> RequestHandlerBaseSPtr;

    class DefaultHandler;
    typedef std::unique_ptr<DefaultHandler> DefaultHandlerUPtr;

    class NodesHandler;
    typedef std::unique_ptr<NodesHandler> NodesHandlerUPtr;

    class ApplicationsHandler;
    typedef std::shared_ptr<ApplicationsHandler> ApplicationsHandlerSPtr;

    class HttpAuthHandler;
    typedef std::shared_ptr<HttpAuthHandler> HttpAuthHandlerSPtr;

    class ApplicationTypesHandler;
    typedef std::unique_ptr<ApplicationTypesHandler> ApplicationTypesHandlerUPtr;

    class ServicesHandler;
    typedef std::unique_ptr<ServicesHandler> ServicesHandlerUPtr;

    class PartitionsHandler;
    typedef std::unique_ptr<PartitionsHandler> PartitionsHandlerUPtr;    

    class ReplicasHandler;
    typedef std::unique_ptr<ReplicasHandler> ReplicasHandlerUPtr;

    class FabricClientWrapper;
    typedef std::shared_ptr<FabricClientWrapper> FabricClientWrapperSPtr;

    class ImageStoreHandler;
    typedef std::shared_ptr<ImageStoreHandler> ImageStoreHandlerSPtr;
    class ToolsHandler;
    typedef std::shared_ptr<ToolsHandler> ToolsHandlerSPtr;

    class NetworksHandler;
    typedef std::unique_ptr<NetworksHandler> NetworksHandlerUPtr;
};
