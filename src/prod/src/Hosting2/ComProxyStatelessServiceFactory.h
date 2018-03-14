// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComProxyStatelessServiceFactory
    {
        DENY_COPY(ComProxyStatelessServiceFactory);

    public:
        ComProxyStatelessServiceFactory(Common::ComPointer<::IFabricStatelessServiceFactory> const & factory)
            : factory_(factory)
        {
        }

        Common::ErrorCode CreateInstance(
            std::wstring const & serviceType, 
            std::wstring const & serviceName, 
            std::vector<byte> const & initializationData, 
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId, 
            __out Common::ComPointer<::IFabricStatelessServiceInstance> & service);
            
    private:
        Common::ComPointer<::IFabricStatelessServiceFactory> const factory_;
    };
}
