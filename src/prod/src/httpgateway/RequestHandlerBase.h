// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // This is the base class that can be extended to write http request handlers. This class creates
    // a top level async operation - HandlerAsyncOperation when it gets a BeginProcessRequest call. The 
    // derived types implement Initialize() method and register the actual logic for each uri they support. The 
    // life time of the request message context is tied to the top level async operation that this
    // class creates.
    //

    class RequestHandlerBase
        : public HttpServer::IHttpRequestHandler
        , public std::enable_shared_from_this<RequestHandlerBase>
    {
        DENY_COPY(RequestHandlerBase)

    public:

        template<typename handler>
        static Common::ErrorCode Create(
            __in HttpGatewayImpl &server, 
            __out HttpServer::IHttpRequestHandlerSPtr &handlerSPtr)
        {
            RequestHandlerBaseSPtr handlerBaseSPtr = std::make_shared<handler>(server);

            auto error = handlerBaseSPtr->Initialize();
            if (error.IsSuccess())
            {
                handlerSPtr = move(handlerBaseSPtr);
            }

            return error;
        }

        RequestHandlerBase(HttpGatewayImpl & server);
        virtual ~RequestHandlerBase();

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            __in HttpServer::IRequestMessageContextUPtr request,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        virtual Common::ErrorCode EndProcessRequest(
            __in Common::AsyncOperationSPtr const& operation);

    protected:

        virtual Common::ErrorCode Initialize() = 0;

        std::wstring GetLocation(std::wstring const &collection, std::wstring &id);

        class HandlerAsyncOperation;

        HttpGatewayImpl & server_;

        //
        // Represents a table of the URI's(suffix, verb, queryparams, handler routine) that this
        // handler can process.
        //
        std::vector<HandlerUriTemplate> validHandlerUris;
        bool allowHierarchicalEntityKeys = true;

    private:

    };
}
