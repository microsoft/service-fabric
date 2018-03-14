// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class IReplicatorFactory : public Common::FabricComponent
        {
            DENY_COPY(IReplicatorFactory);

        public:
            IReplicatorFactory() {}

            virtual HRESULT CreateReplicator(
                FABRIC_REPLICA_ID replicaId,
                __in IFabricStatefulServicePartition * partition,
                __in IFabricStateProvider * stateProvider,
                __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
                BOOLEAN hasPersistedState,
                __in IReplicatorHealthClientSPtr && healthClient,
                __out IFabricStateReplicator **stateReplicator) = 0;

            virtual Common::ErrorCode Open(std::wstring const & nodeId) = 0;

            virtual ~IReplicatorFactory() {}
        };

        struct ReplicatorFactoryConstructorParameters
        {
            Common::ComponentRoot const * Root;
        };

        typedef IReplicatorFactoryUPtr ReplicatorFactoryFactoryFunctionPtr(ReplicatorFactoryConstructorParameters &);

        ReplicatorFactoryFactoryFunctionPtr ReplicatorFactoryFactory;
    }
}
