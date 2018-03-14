// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStatelessServiceFactory
    {
    public:
        virtual ~IStatelessServiceFactory() {};

        virtual Common::ErrorCode CreateInstance(
            std::wstring const & serviceType,
            Common::NamingUri const & serviceName,
            std::vector<byte> const & initializationData, 
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId,
            __out IStatelessServiceInstancePtr & serviceInstance);
    };
}
