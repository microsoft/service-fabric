// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        enum NameType
        {
            Valid = 0,
            Null = 1,
            Empty = 2,
            Schemaless = 3,
            Random = 4,
            Hierarchy = 5
        };

    public:
        StateManagerTestBase();
        StateManagerTestBase(__in bool hasPersistedState);
        StateManagerTestBase(__in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig, __in bool hasPersistedState);

        virtual ~StateManagerTestBase();

    protected:
        TestTransactionManager::SPtr CreateReplica(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in TxnReplicator::IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition);

        TestTransactionManager::SPtr CreateVolatileReplica(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in TxnReplicator::IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition);

        TestTransactionManager::SPtr CreateReplica(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in TxnReplicator::IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition,
            __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
            __in bool hasPersistedState);

        KUri::CSPtr GetStateProviderName(
            __in NameType nameType, 
            __in FABRIC_STATE_PROVIDER_ID parentID = 0, 
            __in FABRIC_STATE_PROVIDER_ID childrenID = 0);

        KAllocator& GetAllocator();

        ktl::Awaitable<void> PopulateAsync(
            __in KArray<KUri::CSPtr> const & nameList,
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr);
        
        ktl::Awaitable<void> DepopulateAsync(
            __in KArray<KUri::CSPtr> const & nameList,
            __in_opt TxnReplicator::Transaction * const transaction = nullptr);

        KArray<LONG64> PopulateStateProviderIdArray(
            __in KArray<KUri::CSPtr> const & nameList);

        void VerifyExist(
            __in KArray<KUri::CSPtr> const & nameList, 
            __in bool isExistanceExpected,
            __in_opt Data::StateManager::IStateManager * const stateManager = nullptr,
            __in bool isInitParameterNull = true);

        void DisableStateProviderCheckpointing(
            __in KArray<KUri::CSPtr> const & nameList);

    protected:
        Data::StateManager::NamedOperationData::CSPtr GenerateNamedMetadataOperationData(
            __in LONG64 stateProviderId,
            __in KUri const & name,
            __in Data::StateManager::ApplyType::Enum applyType,
            __in_opt Data::StateManager::SerializationMode::Enum serializationMode = Data::StateManager::SerializationMode::Native) noexcept;

        Data::StateManager::RedoOperationData::CSPtr GenerateRedoOperationData(
            __in KStringView const & typeName, 
            __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
            __in_opt LONG64 parentId = Data::StateManager::Constants::EmptyStateProviderId,
            __in_opt Data::StateManager::SerializationMode::Enum serializationMode = Data::StateManager::SerializationMode::Native) noexcept;

        ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAsync(
            __in ktl::AwaitableCompletionSource<void> & tcs,
            __in LONG64 applyLSN,
            __in TxnReplicator::TransactionBase const & transactionBase,
            __in TxnReplicator::ApplyContext::Enum applyContext,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const dataPtr,
            __in_opt Data::StateManager::IStateManager * const stateManager = nullptr);

        ktl::Awaitable<KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr> ApplyAddAsync(
            __in KArray<KUri::CSPtr> const & nameList,
            __in ULONG32 startingOffset,
            __in ULONG32 count,
            __in LONG64 startingTransactionId,
            __in LONG64 startingLSN,
            __in LONG64 startingStateProviderId,
            __in TxnReplicator::ApplyContext::Enum applyContext,
            __in_opt Data::StateManager::SerializationMode::Enum serializationMode = Data::StateManager::SerializationMode::Native);

        ktl::Awaitable<KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr> ApplyRemoveAsync(
            __in KArray<KUri::CSPtr> const & nameList,
            __in ULONG32 startingOffset,
            __in ULONG32 count,
            __in LONG64 startingTransactionId,
            __in LONG64 startingLSN,
            __in LONG64 startingStateProviderId,
            __in TxnReplicator::ApplyContext::Enum applyContext);

        ktl::Awaitable<KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr> ApplyMultipleAsync(
            __in KArray<KUri::CSPtr> const & nameList,
            __in KArray<LONG64> const & stateProviderIdList,
            __in ULONG32 startingOffset,
            __in ULONG32 count,
            __in LONG64 startingTransactionId,
            __in LONG64 startingLSN,
            __in Data::StateManager::ApplyType::Enum applyType,
            __in TxnReplicator::ApplyContext::Enum applyContext,
            __in_opt Data::StateManager::IStateManager * const stateManager = nullptr);

        void Unlock(
            __in KSharedArray<TxnReplicator::OperationContext::CSPtr> const & context,
            __in_opt Data::StateManager::IStateManager * const stateManager);

    private:
        TxnReplicator::TransactionalReplicatorSettingsUPtr CreateNativeTRSettings();

    private:
        Data::StateManager::StateManager::SPtr CreateStateManager(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in TxnReplicator::IRuntimeFolders & runtimeFolders,
            __in Data::Utilities::IStatefulPartition & partition,
            __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
            __in bool hasPersistedState);

    protected:
        KtlSystem* underlyingSystem_ = nullptr;
        Common::Random random_ = Common::Random();

        // Input
        TestTransactionManager::SPtr testTransactionalReplicatorSPtr_;
        TestStateProviderFactory::SPtr stateProviderFactorySPtr_;

        Data::Utilities::PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr_;
        TxnReplicator::IRuntimeFolders::SPtr runtimeFolders_;
        Data::Utilities::IStatefulPartition::SPtr partitionSPtr_;
    };
}
