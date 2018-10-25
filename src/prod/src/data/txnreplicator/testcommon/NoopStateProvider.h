// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class NoopStateProvider
            : public KObject<NoopStateProvider>
            , public KShared<NoopStateProvider>
            , public IStateProvider2
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            K_FORCE_SHARED(NoopStateProvider);
            K_SHARED_INTERFACE_IMP(IStateProvider2)

        public:
            static KStringView const TypeName;

        public:
            static SPtr Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __in KAllocator & allocator);

        public: // Test Properties
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const { return stateProviderId_; }

            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const { return PartitionedReplicaIdentifier->PartitionId; };

            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return PartitionedReplicaIdentifier->ReplicaId; };

        public: // Test APIs
            NTSTATUS AddOperation(
                __in TxnReplicator::Transaction & transaction,
                __in Data::Utilities::OperationData const * const metadata,
                __in Data::Utilities::OperationData const * const undo,
                __in Data::Utilities::OperationData const * const redo,
                __in TxnReplicator::OperationContext const * const context);

        public: // Test Verify initialization parameters
            Data::Utilities::OperationData::CSPtr GetInitializationParameters() const;

        public: // IStateProvider APIs.
            KUriView const GetName() const override;

            KArray<StateProviderInfo> GetChildren(__in KUriView const & rootName) override;

        public: // State Provider Life cycle operations
            VOID Initialize(
                __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
                __in KStringView const & workFolder,
                __in_opt KSharedArray<IStateProvider2::SPtr> const * const children) override;

            ktl::Awaitable<void> OpenAsync(__in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> ChangeRoleAsync(
                __in FABRIC_REPLICA_ROLE newRole,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;
            void Abort() noexcept override;

        public: // IStateProvide2 persisted state related APIs.
            void PrepareCheckpoint(__in FABRIC_SEQUENCE_NUMBER checkpointLSN) override;

            ktl::Awaitable<void> PerformCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> CompleteCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> RecoverCheckpointAsync(
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> BackupCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> RestoreCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<void> RemoveStateAsync(
                __in ktl::CancellationToken const & cancellationToken) override;

            OperationDataStream::SPtr GetCurrentState() override;

            ktl::Awaitable<void> BeginSettingCurrentStateAsync() override;

            ktl::Awaitable<void> SetCurrentStateAsync(
                __in LONG64 stateRecordNumber,
                __in Data::Utilities::OperationData const & operationData,
                __in ktl::CancellationToken const& cancellationToken) override;

            ktl::Awaitable<void> EndSettingCurrentStateAsync(
                __in ktl::CancellationToken const& cancellationToken) override;

        public: // Remove State Provider operations. Table lock.
            ktl::Awaitable<void> PrepareForRemoveAsync(
                __in TransactionBase const & transactionBase,
                __in ktl::CancellationToken const & cancellationToken) override;

        public: // State Provider Operations
            ktl::Awaitable<OperationContext::CSPtr> ApplyAsync(
                __in FABRIC_SEQUENCE_NUMBER logicalSequenceNumber,
                __in TransactionBase const & transactionBase,
                __in ApplyContext::Enum applyContext,
                __in_opt Data::Utilities::OperationData const * const metadataPtr,
                __in_opt Data::Utilities::OperationData const * const dataPtr) override;

            void Unlock(__in OperationContext const & operationContext) override;

        public:
            NoopStateProvider(
                __in Data::StateManager::FactoryArguments const & factoryArguments);

        private:
            void TraceAndThrowOnFailure(
                __in NTSTATUS status,
                __in wstring const & traceInfo) const;

        private: // Constants   
            FABRIC_STATE_PROVIDER_ID const stateProviderId_;
            Data::Utilities::OperationData::CSPtr const initializationParameters_;

        private:
            KString::CSPtr workFolder_;
            KUri::SPtr name_;
            KWeakRef<ITransactionalReplicator>::SPtr replicatorWRef_;
        };
    }
}
