// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Factory that creates ComReplicator objects.
        // It uses a shared transport and configuration objects
        // for all replicators.
        class ComReplicatorFactory : public Common::RootedObject, public IReplicatorFactory
        {
            DENY_COPY(ComReplicatorFactory)
        public:
            ComReplicatorFactory(Common::ComponentRoot const & root);
            ComReplicatorFactory(ReplicatorFactoryConstructorParameters & parameters);
            virtual ~ComReplicatorFactory();
            Common::ErrorCode Open(std::wstring const & nodeId);
            
            HRESULT CreateReplicator(
                FABRIC_REPLICA_ID replicaId,
                __in IFabricStatefulServicePartition * partition,
                __in IFabricStateProvider * stateProvider,
                __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
                BOOLEAN hasPersistedState,
                __in IReplicatorHealthClientSPtr && healthClient,
                __out IFabricStateReplicator **stateReplicator);

            HRESULT CreateReplicatorV1Plus(
                FABRIC_REPLICA_ID replicaId,
                __in IFabricStatefulServicePartition * partition,
                __in IFabricStateProvider * stateProvider,
                __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
                BOOLEAN hasPersistedState,
                BOOLEAN batchEnabled,
                __in IReplicatorHealthClientSPtr && healthClient,
                __out IFabricStateReplicator **stateReplicator);

            void Test_GetFactoryDetails(__out std::vector<std::wstring> & replicatorAddresses);

        protected:
            virtual Common::ErrorCode OnOpen();
            virtual Common::ErrorCode OnClose();
            virtual void OnAbort();

        private:
            HRESULT CreateReplicatorConfigAndTransport(
                __in Common::Guid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in BOOLEAN hasPersistedState,
                __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
                __out std::shared_ptr<REInternalSettings> & config,
                __out ReplicationTransportSPtr & transport);

            bool IsPowerOf2(int value) { return 0 == (value & (value-1)); }

            // Map of all transports by the specified address.
            // For port 0, a port chosen by the transport will be selected.
            // If another replicator must be created with that port value,
            // it will fail.
            // Keep shared pointers to the transports so they are kept alive
            // for the lifetime of the factory. If the transport stays alive 
            // only as long as there are replicators registered, it may be closed
            // and restarted immediately, which may lead to AddressAlreadyInUse exceptions
            // because the socket is not immediately reusable.
            typedef std::pair<std::wstring, ReplicationTransportSPtr> TransportPair;
            typedef std::map<std::wstring, ReplicationTransportSPtr> TransportMap;
            TransportMap transports_;
                        
            RWLOCK(REComReplicatorFactory, lock_);
            std::wstring nodeId_; // id of the node to which the FabricRuntime containing this replicator factory is connected to.
        };

    } // end namespace ReplicationComponent
} // end namespace Reliability

