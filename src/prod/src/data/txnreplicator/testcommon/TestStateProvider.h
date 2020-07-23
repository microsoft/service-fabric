// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestStateProvider
            : public KObject<TestStateProvider>
            , public KShared<TestStateProvider>
            , public IStateProvider2
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            K_FORCE_SHARED(TestStateProvider)
            K_SHARED_INTERFACE_IMP(IStateProvider2)

        public:
            class UndoOperationData;

        public:
            static Common::WStringLiteral const TypeName;
            static wstring const FabricTestStateProviderNamePrefix; 

            // The starting version number is 1 (Instead of 0) because in fabric test, when there is a race to insert a new key (new SP),
            // the thread which loses the race must return version_mismatch error code with the newer version
            //
            // Since the absence of a key is represented by 0, creating a key(SP) must be different from 0 and is hence 1.
            // We can be guaranteed that no tx starts at 1 as the replicator's starting lsn is 2
            static const FABRIC_SEQUENCE_NUMBER StartingVersionNumber = 1;

        public:
            static SPtr Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __in KAllocator & allocator);

            // Note: Add this ctor for Backup and Restore back compatible test to avoid throwing file not exist exception of the wrapper.
            // In the managed code, reliable dictionary is a wrapper of tsore, so when we do the back compatible test, dataStore is one 
            // actually restored from the file and we should ignore the wrapper.
            static SPtr Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __in bool backCompatibleTest,
                __in KAllocator & allocator);

        public: // Test Properties
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const { return stateProviderId_; }

            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const { return partitionId_; };
            
            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; };
            
        public: // Test APIs
            ktl::Awaitable<void> GetAsync(
                __out FABRIC_SEQUENCE_NUMBER & version,
                __out KString::SPtr & value);

            ktl::Awaitable<void> GetAsync(
                __in Common::TimeSpan const & timeout,
                __out FABRIC_SEQUENCE_NUMBER & version,
                __out KString::SPtr & value);

            ktl::Awaitable<bool> TryUpdateAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KStringView const & newValue,
                __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
                __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

            ktl::Awaitable<bool> TryUpdateAsync(
                __in Common::TimeSpan const & timeout,
                __in TxnReplicator::Transaction & transaction,
                __in KStringView const & newValue,
                __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
                __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

            ktl::Awaitable<bool> TryRemoveMyself(
                __in TxnReplicator::Transaction & transaction,
                __in TxnReplicator::ITransactionalReplicator & replicator,
                __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
                __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

            ktl::Awaitable<bool> TryRemoveMyself(
                __in Common::TimeSpan const & timeout,
                __in TxnReplicator::Transaction & transaction,
                __in TxnReplicator::ITransactionalReplicator & replicator,
                __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
                __in_opt FABRIC_SEQUENCE_NUMBER oldVersion);

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
                __in LONG64 logicalSequenceNumber,
                __in TransactionBase const & transactionBase,
                __in ApplyContext::Enum applyContext,
                __in_opt Data::Utilities::OperationData const * const metadataPtr,
                __in_opt Data::Utilities::OperationData const * const dataPtr) override;

            void Unlock(__in OperationContext const & operationContext) override;

        public:
            TestStateProvider(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __in bool isBackCompatibleTest);
            
        private:
            ktl::Awaitable<void> AcquireLock(
                __in Common::TimeSpan const & timeout,
                std::wstring const & caller);

            // CheckpointFile will save the CurrentVersion and CurrentValue
            ktl::Awaitable<void> CreateCheckpointFileAsync(
                __in KString const & value,
                __in FABRIC_SEQUENCE_NUMBER version,
                __in bool writeCurrentFile);

            ktl::Awaitable<bool> OpenCheckpointFileAsync(__in KWString & filePath);

            ktl::Awaitable<void> SafeFileReplaceAsync(
                __in KString const & checkpointFileName,
                __in KString const & temporaryCheckpointFileName,
                __in KString const & backupCheckpointFileName,
                __in ktl::CancellationToken const & cancellationToken,
                __in KAllocator & allocator);

            // Helper function to concat fileName to the workDirectory 
            KString::CSPtr GetCheckpointFilePath(__in KStringView const & fileName);

            void ThrowIfNotReadable() const;

            void TraceAndThrowOnFailure(
                __in NTSTATUS status,
                __in wstring const & traceInfo) const;

            KString::SPtr ReadInitializationParamaters();

        private:
            enum LifeState
            {
                Created = 0,
                Initialized = 1,
                Open = 2,
                Primary = 3,
                Idle = 4,
                Secondary = 5,
                None = 6,
                Closed = 7,
            };

            enum CheckpointState
            {
                Invalid = 0,
                Restored = 1,
                Recovered = 2,
                Prepared = 3,
                Performed = 4,
                Completed = 5,
                Removed = 6,
            };

        private: // Constants   
            KStringView const CurrentCheckpointPrefix = L"TestStateProvider.cpt";
            KStringView const TempCheckpointPrefix = L"TestStateProvider.tmp";
            KStringView const BackupCheckpointPrefix = L"TestStateProvider.bkp";

        private:
            bool const isBackCompatibleTest_;

        private: 
            KString::CSPtr workFolder_;
            KString::CSPtr tempCheckpointFilePath_;
            KString::CSPtr currentCheckpointFilePath_;
            KString::CSPtr backupCheckpointFilePath_;

        private:
            LifeState lifeState_ = Created;
            CheckpointState checkpointState_ = Invalid;

            bool isBeginSettingCurrentStateCalled_ = false;
            bool isEndSettingCurrentStateCalled = false;

            Data::Utilities::AsyncLock::SPtr lock_;

            // State
            FABRIC_SEQUENCE_NUMBER currentVersion_ = StartingVersionNumber;
            KString::SPtr currentValue_;
            Data::Utilities::BinaryWriter serializer_;

            Data::Utilities::ThreadSafeCSPtrCache<VersionedState> lastPrepareCheckpointData_;
            Data::Utilities::ThreadSafeCSPtrCache<VersionedState> lastCheckpointedData_;

            // Checkpoint LSNs.
            FABRIC_SEQUENCE_NUMBER consumedCopyCheckpointLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            FABRIC_SEQUENCE_NUMBER performCheckpointLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            FABRIC_SEQUENCE_NUMBER completeCheckpointLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

        private:
            KUri::SPtr name_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
            KGuid partitionId_;
            FABRIC_REPLICA_ID replicaId_;

            bool hasPersistedState_ = true;

            Data::Utilities::OperationData::CSPtr initializationParameters_;
            KWeakRef<ITransactionalReplicator>::SPtr replicatorWRef_;
        };
    }
}
