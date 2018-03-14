// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStatefulServiceFactory
    {
    public:
        virtual ~IStatefulServiceFactory() {};

        virtual Common::ErrorCode CreateReplica(
            std::wstring const & serviceType,
            Common::NamingUri const & serviceName,
            std::vector<byte> const & initializationData, 
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            __out IStatefulServiceReplicaPtr & serviceReplica) = 0;
    };
}
