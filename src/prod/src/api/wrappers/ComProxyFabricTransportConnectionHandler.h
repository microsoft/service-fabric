// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyFabricTransportConnectionHandler :
        public Common::ComponentRoot,
        public IServiceConnectionHandler
    {
        DENY_COPY(ComProxyFabricTransportConnectionHandler);

    public:
        ComProxyFabricTransportConnectionHandler(Common::ComPointer<IFabricTransportConnectionHandler> const & comImpl);

        virtual ~ComProxyFabricTransportConnectionHandler();

        //
        // IServiceConnectionHandler
        //
        Common::AsyncOperationSPtr ComProxyFabricTransportConnectionHandler::BeginProcessConnect(
            IClientConnectionPtr clientConnection,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode ComProxyFabricTransportConnectionHandler::EndProcessConnect(
            Common::AsyncOperationSPtr const &  operation);

        Common::AsyncOperationSPtr ComProxyFabricTransportConnectionHandler::BeginProcessDisconnect(
            std::wstring const & clientId,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode ComProxyFabricTransportConnectionHandler::EndProcessDisconnect(
            Common::AsyncOperationSPtr const &  operation);
                
    private:
        class ConnectAsyncOperation;
        class DisconnectAsyncOperation;
        Common::ComPointer<IFabricTransportConnectionHandler> const comImpl_;
    };
}
