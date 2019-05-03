// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // Handler for EventsStore requests.
    //
    class EventsStoreHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(EventsStoreHandler)

    public:

        EventsStoreHandler(HttpGatewayImpl & server);
        virtual ~EventsStoreHandler() = default;

        Common::ErrorCode Initialize();

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            __in HttpServer::IRequestMessageContextUPtr request,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

    private:
        class EventsStoreHandlerAsyncOperation
            : public RequestHandlerBase::HandlerAsyncOperation
        {
            DENY_COPY(EventsStoreHandlerAsyncOperation);

        public:
            EventsStoreHandlerAsyncOperation(
                RequestHandlerBase & owner,
                HttpServer::IRequestMessageContextUPtr messageContext,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);

            virtual void GetRequestBody(Common::AsyncOperationSPtr const& thisSPtr) override;
        };

        void ProcessReverseProxyRequest(__in Common::AsyncOperationSPtr const& thisSptr);
        void OnProcessReverseProxyRequestComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
    };
}