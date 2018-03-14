// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComProxyStatefulServiceFactory
    {
        DENY_COPY(ComProxyStatefulServiceFactory);

    public:
        ComProxyStatefulServiceFactory(Common::ComPointer<::IFabricStatefulServiceFactory> const & factory)
            : factory_(factory)
        {
        }

        Common::ErrorCode CreateReplica(
            std::wstring const & serviceType, 
            std::wstring const & serviceName, 
            std::vector<byte> const & initParams, 
            Common::Guid const & partitionId,
            const FABRIC_REPLICA_ID replicaId, 
            Common::ComPointer<IFabricStatefulServiceReplica> & service);
            
    private:
        Common::ComPointer<::IFabricStatefulServiceFactory> const factory_;
    };
}
