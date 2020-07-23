// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Integration
    {
        class Replica final
            : Common::ComponentRoot
            , public KObject<Replica>
            , public KShared<Replica>
        {
            K_FORCE_SHARED(Replica);

        public:
            enum SerializationVersion
            {
                Managed = 0,
                Native = 1,
            };

        public:
            static Replica::SPtr Create(
                __in KGuid const & pId,
                __in LONG64 replicaId,
                __in wstring const & workFolder,
                __in Data::Log::LogManager & logManager,
                __in KAllocator & allocator,
                __in_opt Data::StateManager::IStateProvider2Factory * const stateProviderFactory = nullptr,
                __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS const * const settings = nullptr,
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler = nullptr,
                __in_opt BOOLEAN hasPersistedState = TRUE);

        public:
            __declspec(property(get = get_TxnReplicator)) TxnReplicator::ITransactionalReplicator::SPtr TxnReplicator;
            TxnReplicator::ITransactionalReplicator::SPtr get_TxnReplicator() const;

        public:
            ktl::Awaitable<void> OpenAsync();
            ktl::Awaitable<void> ChangeRoleAsync(
                __in FABRIC_EPOCH epoch,
                __in FABRIC_REPLICA_ROLE newRole);

            ktl::Awaitable<NTSTATUS> OnDataLossAsync(
                __out BOOLEAN & isStateChanged);

            ktl::Awaitable<void> CloseAsync();

        public:
            void SetReadStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus);
            void SetWriteStatus(__in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus);

        private:
            Replica(
                __in KGuid const & pId,
                __in LONG64 replicaId,
                __in wstring const & workFolder,
                __in Data::Log::LogManager & logManager,
                __in Data::StateManager::IStateProvider2Factory * stateProviderFactory,
                __in TRANSACTIONAL_REPLICATOR_SETTINGS const * const settings,
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler,
                __in_opt BOOLEAN hasPersistedState = TRUE);

        private:
            Data::TestCommon::TestComStatefulServicePartition::SPtr partition_;
            Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
            TxnReplicator::ITransactionalReplicator::SPtr transactionalReplicator_;
            Reliability::ReplicationComponent::IReplicatorFactoryUPtr replicatorFactory_;
            TxnReplicator::ITransactionalReplicatorFactoryUPtr transactionalReplicatorFactory_;
        };
    }
}
