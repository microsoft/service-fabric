//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class GatewaysResourceHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(GatewaysResourceHandler)

    public:

        GatewaysResourceHandler(HttpGatewayImpl & server);
        virtual ~GatewaysResourceHandler();

        Common::ErrorCode Initialize();

    private:

        void CreateOrUpdateGateway(Common::AsyncOperationSPtr const &);
        void OnCreateOrUpdateGatewayComplete(Common::AsyncOperationSPtr const &, bool);

        void DeleteGateway(Common::AsyncOperationSPtr const &);
        void OnDeleteGatewayComplete(Common::AsyncOperationSPtr const &, bool);

        void GetAllGateways(Common::AsyncOperationSPtr const &);
        void OnGetAllGatewaysComplete(Common::AsyncOperationSPtr const &, bool);

        void GetGatewayByName(Common::AsyncOperationSPtr const &);
        void OnGetGatewayByNameComplete(Common::AsyncOperationSPtr const &, bool);
    };
}
