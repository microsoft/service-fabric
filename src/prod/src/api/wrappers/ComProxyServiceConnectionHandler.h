// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyServiceConnectionHandler :
        public Common::ComponentRoot,
        public IServiceConnectionHandler
    {
        DENY_COPY(ComProxyServiceConnectionHandler);

    public:
        ComProxyServiceConnectionHandler(Common::ComPointer<IFabricServiceConnectionHandler> const & comImpl);

        virtual ~ComProxyServiceConnectionHandler();

        //
        // IServiceConnectionHandler
        //
        Common::AsyncOperationSPtr ComProxyServiceConnectionHandler::BeginProcessConnect(
            IClientConnectionPtr clientConnection,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
  
        Common::ErrorCode ComProxyServiceConnectionHandler::EndProcessConnect(
            Common::AsyncOperationSPtr const &  operation);
    
        Common::AsyncOperationSPtr ComProxyServiceConnectionHandler::BeginProcessDisconnect(
            std::wstring const & clientId,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
     
        Common::ErrorCode ComProxyServiceConnectionHandler::EndProcessDisconnect(
            Common::AsyncOperationSPtr const &  operation);
     

            
    private:
        class ConnectAsyncOperation;
        class DisconnectAsyncOperation;
        Common::ComPointer<IFabricServiceConnectionHandler> const comImpl_;
    };
}
