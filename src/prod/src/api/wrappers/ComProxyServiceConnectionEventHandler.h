// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyServiceConnectionEventHandler :
        public Common::ComponentRoot,
        public IServiceConnectionEventHandler
    {
        DENY_COPY(ComProxyServiceConnectionEventHandler);

    public:
        ComProxyServiceConnectionEventHandler(Common::ComPointer<IFabricServiceConnectionEventHandler> const & comImpl);

        virtual ~ComProxyServiceConnectionEventHandler();

        //
        // IServiceConnectionEventHandler
        virtual void OnConnected(std::wstring const & connectionAddress) ;

        virtual void OnDisconnected(
            std::wstring const & connectionAddress,
            Common::ErrorCode const &) ;
            
    private:
        Common::ComPointer<IFabricServiceConnectionEventHandler> const comImpl_;
    };
}
