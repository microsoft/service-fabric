// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IClientConnectionEventHandler
    {
    public:
        virtual ~IClientConnectionEventHandler() {};

        virtual Common::ErrorCode OnConnected(Naming::GatewayDescription const &) = 0;

        virtual Common::ErrorCode OnDisconnected(Naming::GatewayDescription const &, Common::ErrorCode const &) = 0;

        virtual Common::ErrorCode OnClaimsRetrieval(std::shared_ptr<Transport::ClaimsRetrievalMetadata> const &, __out std::wstring & token) = 0;
    };
}
