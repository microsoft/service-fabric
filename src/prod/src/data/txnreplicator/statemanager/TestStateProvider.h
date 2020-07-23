// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestStateProvider : 
        public KAsyncServiceBase, 
        public TxnReplicator::IStateProvider2,
        public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TestSession>
    {
        K_FORCE_SHARED(TestStateProvider)
        K_SHARED_INTERFACE_IMP(IStateProvider2)

    public:
        static KStringView const TypeName;

    public:
        static NTSTATUS TestStateProvider::Create(
            __in Data::StateManager::FactoryArguments const & factoryArguments,
            __in KAllocator & allocator,
            __in FaultStateProviderType::FaultAPI faultyAPI,
            __out TestStateProvider::SPtr & result);

        static Data::Utilities::OperationData::SPtr CreateInitializationParameters(
            __in ULONG childrenCount,
            __in KAllocator & allocator);

    public:
        __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
        FABRIC_STATE_PROVIDER_ID get_StateProviderId() const;

        __declspec(property(get = get_FaultyAPI, put = put_FaultyAPI)) FaultStateProviderType::FaultAPI FaultyAPI;
        FaultStateProviderType::FaultAPI get_FaultyAPI() const { return faultyAPI_; };
        void put_FaultyAPI(__in FaultStateProviderType::FaultAPI faultyAPI) { faultyAPI_ = faultyAPI; }

        __declspec(property(get = get_Checkpointing, put = put_Checkpointing)) bool Checkpointing;
        bool get_Checkpointing() const { return checkpointing_; };
        void put_Checkpointing(__in bool checkpointing) { checkpointing_ = checkpointing; }

        __declspec(property(get = get_PartitionId)) KGuid PartitionId;
        KGuid get_PartitionId() const { return partitionId_; };

        __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; };

        __declspec(property(get = get_RecoverCheckpoint)) bool RecoverCheckpoint;
        bool get_RecoverCheckpoint() const { return recoverCheckpoint_; };

        __declspec(property(get = get_UserOperationDataArray)) KArray<Data::Utilities::OperationData::CSPtr> UserOperationDataArray;
        KArray<Data::Utilities::OperationData::CSPtr> get_UserOperationDataArray() const { return userOperationDataArray_; };

        __declspec(property(get = get_UserMetadataOperationData)) Data::Utilities::OperationData::CSPtr UserMetadataOperationData;
        Data::Utilities::OperationData::CSPtr get_UserMetadataOperationData() const { return userMetadataOperationData_; };

        __declspec(property(get = get_UserRedoOrUndoOperationData)) Data::Utilities::OperationData::CSPtr UserRedoOrUndoOperationData;
        Data::Utilities::OperationData::CSPtr get_UserRedoOrUndoOperationData() const { return userRedoOrUndoOperationData_; };

    public:
        KUriView const GetName() const override;

        KArray<TxnReplicator::StateProviderInfo> GetChildren(__in KUriView const & rootName) override;

    public: // Test Apis
        ktl::Awaitable<void> AddAsync(
            __in TxnReplicator::Transaction & transaction,
            __in TestOperation const & testOperation);
        ktl::Awaitable<void> AddAsync(
            __in TxnReplicator::AtomicOperation & atomicOperation,
            __in TestOperation const & testOperation);
        ktl::Awaitable<void> AddAsync(
            __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
            __in TestOperation const & testOperation);

        // Test only, used to verify the initialization parameters
        Data::Utilities::OperationData::CSPtr GetInitializationParameters() const;

    public:
        void ExpectCommit(__in LONG64 transactionId);
        void ExpectCheckpoint(
            __in LONG64 checkpointLSN,
            __in_opt bool completeCheckpointOnly = false);
        void Verify(
            __in_opt bool verifyCheckpoint = false,
            __in_opt bool verifyCompleteCheckpoint = false);

        // Verify function is used to verify the expected committed transaction is applied.
        // The Verify logic is to check every transaction in the expected committed map must
        // be also in the applied map. But in the concurrent test, it leads to a problem 
        // that thread A set expected and applied, but right before thread A verify call, thread B
        // added a transaction to the expected map, in this case, the verify will fail because 
        // thread B's transaction is set to expected but not applied.
        // So we introduce another Verify function to verify the specific expected committed 
        // transaction must be applied.
        void Verify(__in LONG64 transactionId);

        void VerifyRecoveredStates(__in KArray<LONG64> const & expectedArray);

    public: // State Provider Life cycle operations
        VOID Initialize(
            __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
            __in KStringView const & workFolder,
            __in_opt KSharedArray<IStateProvider2::SPtr> const * const children) override;

        ktl::Awaitable<void> OpenAsync(__in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE newRole,
            __in ktl::CancellationToken const & cancellationToken) override;

        ktl::Awaitable<void> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;
        void Abort() noexcept override;

    public:
        void PrepareCheckpoint(__in LONG64 checkpointLSN) override;

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

    public: // Remove State Provider operations.
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

    public: // Test
        KStringView GetInputTypeName() const;

    protected:
        VOID OnServiceOpen() override;
        VOID OnServiceClose() override;

    private:
        TestStateProvider::TestStateProvider(
            __in Data::StateManager::FactoryArguments const & factoryArguments,
            __in FaultStateProviderType::FaultAPI faultyAPI);

    private:
        ktl::Awaitable<void> CloseInternalAsync(
            __in ktl::CancellationToken const & cancellationToken);

        // CheckpointFile will save CheckpointLSN
        ktl::Awaitable<void> WriteAsync();
        ktl::Awaitable<void> ReadAsync(__in KWString & filePath);

        ktl::Awaitable<void> SafeFileReplaceAsync(
            __in KString const & checkpointFileName,
            __in KString const & temporaryCheckpointFileName,
            __in KString const & backupCheckpointFileName);

    private:
        void AddOperationToMap(
            __in LONG64 transactionId,
            __in TestOperation const & testOperation,
            __inout Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr> & map);

        void VerifyOperationList(
            __in KSharedArray<TestOperation> const & listOne,
            __in KSharedArray<TestOperation> const & listTwo) const;

        void VerifyUnlocked(
            __in KSharedArray<TestOperation> const & listOne) const;

    private:
        void ThrowExceptionIfFaultyAPI(__in FaultStateProviderType::FaultAPI currentAPI) const;

        // Helper function to concat fileName to the workDirectory 
        KString::CSPtr GetCheckpointFilePath(__in KStringView const & fileName);

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

    private:
        LONG64 const UnknownLSN = FABRIC_AUTO_SEQUENCE_NUMBER;
        KStringView const CurrentCheckpointPrefix = L"TestStateProvider.cpt";
        KStringView const TempCheckpointPrefix = L"TestStateProvider.tmp";
        KStringView const BackupCheckpointPrefix = L"TestStateProvider.bkp";

    private:
        LifeState lifeState_ = Created;
        CheckpointState checkpointState_ = Invalid;
        bool isBeginSettingCurrentStateCalled_ = false;
        bool isEndSettingCurrentStateCalled_ = false;
        LONG64 lastAcquiredLockTxnId = -1;

        // Note: Hierarchy bug, the child has been added to the MetadataManager but did not call OpenAsync 
        // and RecoverCheckpointAsync on it. Previous recovery test just check whether the state provider
        // exist. Add this flag so we can tell whether the RecoverCheckpointAsync is called. 
        // In the RecoverCheckpointAsync we assert if the liftState is not Open.
        bool recoverCheckpoint_ = false;
        LONG64 copiedLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

        // #10485130: Temperory config used to disable the WriteAsync call in PerformCheckpointAsync for perf tests
        bool checkpointing_ = true;

        bool hasPersistedState_ = true;

    private:
        KString::CSPtr workFolder_;
        KString::CSPtr tempCheckpointFilePath_;
        KString::CSPtr currentCheckpointFilePath_;
        KString::CSPtr backupCheckpointFilePath_;

    private:
        KUriView name_;
        KStringView typeName_;
        KGuid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        FABRIC_STATE_PROVIDER_ID stateProviderId_;
        FaultStateProviderType::FaultAPI faultyAPI_;

        Data::Utilities::OperationData::CSPtr initializationParameters_;
        KArray<Data::Utilities::OperationData::CSPtr> userOperationDataArray_;

        // The lock used for preventing the state provider is removed during 
        // the add call.
        Data::Utilities::ReaderWriterAsyncLock::SPtr lock_;

        Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::SPtr infligtOperationMaps_;
        Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::SPtr expectedToBeAppliedMap_;
        Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<TestOperation>::SPtr>::SPtr appliedMap_;

        Data::Utilities::OperationData::CSPtr userMetadataOperationData_;
        Data::Utilities::OperationData::CSPtr userRedoOrUndoOperationData_;

        KArray<LONG64> prepareCheckpointArray; 
        KArray<LONG64> performCheckpointArray;
        KArray<LONG64> completeCheckpointArray;
        KArray<LONG64> recoverCheckpointArray;

        KArray<LONG64> expectedPrepareCheckpointArray;
        KArray<LONG64> expectedPerformCheckpointArray;
        KArray<LONG64> expectedCompleteCheckpointArray;
    };
}
