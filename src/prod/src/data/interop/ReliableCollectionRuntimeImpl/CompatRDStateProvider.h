// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Interop
    {
        // In current managed data stack DistributedDictionary StateProvider is just a wrapper state provider.
        // It has TStore as child state provider which state manager is aware of. This class is used to mimic 
        // the same behavior for data upgrade and rolling update scenarios. Instance of this class is 
        // created corresponding to DistributedDictionary state provider. This class has TStore as a child 
        // state provider. C# layer only interacts with TStore SP and not with instance of this class.
        class CompatRDStateProvider
            : public KObject<CompatRDStateProvider>
            , public KShared<CompatRDStateProvider>
            , public TxnReplicator::IStateProvider2
            , public IStateProviderInfo
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::RCR>
        {
            K_FORCE_SHARED(CompatRDStateProvider)
            K_SHARED_INTERFACE_IMP(IStateProvider2)
            K_SHARED_INTERFACE_IMP(IStateProviderInfo)

        public:
            class UndoOperationData;

        private:
            static const wstring RelativePathForDataStore;

        public:
            static NTSTATUS Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __in KAllocator & allocator,
                __out CompatRDStateProvider::SPtr& dummyStateProvider);


        public: 
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const { return stateProviderId_; }

            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const { return partitionId_; };
            
            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; };

            __declspec(property(get = get_DataStore)) Data::TStore::IStore<KString::SPtr, KBuffer::SPtr>::SPtr DataStore;
            Data::TStore::IStore<KString::SPtr, KBuffer::SPtr>::SPtr get_DataStore() const {
                Data::TStore::IStore<KString::SPtr, KBuffer::SPtr>::SPtr dataStore(dataStore_);
                return dataStore;
            };

        public: // IStateProvider APIs.
            KUriView const GetName() const override;

            KArray<TxnReplicator::StateProviderInfo> GetChildren(__in KUriView const & rootName) override;

        public:
            KString* GetLangTypeInfo(KStringView const & lang) const override;

            NTSTATUS SetLangTypeInfo(KStringView const & lang, KStringView const & typeInfo) override;

            StateProviderKind GetKind() const override;

        public: // State Provider Life cycle operations
            VOID Initialize(
                __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
                __in KStringView const & workFolder,
                __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children) override;

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

            TxnReplicator::OperationDataStream::SPtr GetCurrentState() override;

            ktl::Awaitable<void> BeginSettingCurrentStateAsync() override;

            ktl::Awaitable<void> SetCurrentStateAsync(
                __in LONG64 stateRecordNumber,
                __in Data::Utilities::OperationData const & operationData,
                __in ktl::CancellationToken const& cancellationToken) override;

            ktl::Awaitable<void> EndSettingCurrentStateAsync(
                __in ktl::CancellationToken const& cancellationToken) override;

        public: // Remove State Provider operations. Table lock.
            ktl::Awaitable<void> PrepareForRemoveAsync(
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in ktl::CancellationToken const & cancellationToken) override;

        public: // State Provider Operations
            ktl::Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAsync(
                __in LONG64 logicalSequenceNumber,
                __in TxnReplicator::TransactionBase const & transactionBase,
                __in TxnReplicator::ApplyContext::Enum applyContext,
                __in_opt Data::Utilities::OperationData const * const metadataPtr,
                __in_opt Data::Utilities::OperationData const * const dataPtr) override;

            void Unlock(__in TxnReplicator::OperationContext const & operationContext) override;

        private:
            void TraceAndThrowOnFailure(
                __in NTSTATUS status,
                __in wstring const & traceInfo) const;

            CompatRDStateProvider(
                __in Data::StateManager::FactoryArguments const & factoryArguments);
            
            KUri::CSPtr name_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
            KGuid partitionId_;
            FABRIC_REPLICA_ID replicaId_;
            Data::TStore::IStore<KString::SPtr, KBuffer::SPtr>* dataStore_;
            KString::CSPtr typename_;

            Data::Utilities::OperationData::CSPtr initializationParameters_;
        };
    }
}
