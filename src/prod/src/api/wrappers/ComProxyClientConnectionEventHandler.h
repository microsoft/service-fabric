// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyClientConnectionEventHandler :
        public Common::ComponentRoot,
        public IClientConnectionEventHandler
    {
        DENY_COPY(ComProxyClientConnectionEventHandler);

    public:
        ComProxyClientConnectionEventHandler(Common::ComPointer<IFabricClientConnectionEventHandler> const & comImpl);

        virtual ~ComProxyClientConnectionEventHandler();

        //
        // IClientConnectionEventHandler
        //
        Common::ErrorCode OnConnected(Naming::GatewayDescription const &);

        Common::ErrorCode OnDisconnected(Naming::GatewayDescription const &, Common::ErrorCode const &);

        Common::ErrorCode OnClaimsRetrieval(std::shared_ptr<Transport::ClaimsRetrievalMetadata> const &, __out std::wstring &);
            
    private:
        Common::ComPointer<IFabricClientConnectionEventHandler> const comImpl_;
        Common::ScopedHeap heap_;
    };
}
