// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyStatelessServiceFactory :
        public Common::ComponentRoot,
        public IStatelessServiceFactory
    {
        DENY_COPY(ComProxyStatelessServiceFactory);

    public:
        ComProxyStatelessServiceFactory(Common::ComPointer<IFabricStatelessServiceFactory> const & comImpl);
        virtual ~ComProxyStatelessServiceFactory();
            
        Common::ErrorCode CreateInstance(
            std::wstring const & serviceType, 
            std::wstring const & serviceName, 
            std::vector<byte> const & initializationData, 
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId, 
            __out IStatelessServiceInstancePtr & serviceInstance);
            
    private:
        Common::ComPointer<::IFabricStatelessServiceFactory> const comImpl_;
    };
}
