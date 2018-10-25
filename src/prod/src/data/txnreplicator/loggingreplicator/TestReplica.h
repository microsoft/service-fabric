// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestReplica final
        : public KObject<TestReplica>
        , public KShared<TestReplica>
        , public Data::LoggingReplicator::IFlushCallbackProcessor
        , public Data::LoggingReplicator::ICompletedRecordsProcessor
        , public KWeakRefType<TestReplica>
    {
        K_FORCE_SHARED(TestReplica)
        K_SHARED_INTERFACE_IMP(IFlushCallbackProcessor)
        K_WEAKREF_INTERFACE_IMP(IFlushCallbackProcessor, TestReplica)
        K_SHARED_INTERFACE_IMP(ICompletedRecordsProcessor)
        K_WEAKREF_INTERFACE_IMP(ICompletedRecordsProcessor, TestReplica)

    public:
        static TestReplica::SPtr Create(
            __in KGuid & pId,
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords,
            __in bool isPrimary,
            __in_opt TestStateProviderManager * stateManager,
            __in ApiFaultUtility & apiFaultUtility,
            __in KAllocator & allocator);

        static Common::ErrorCode DeleteWorkingDirectory();

        TestReplica(
            __in KGuid & pId,
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords,
            __in bool isPrimary,
            __in_opt TestStateProviderManager * stateManager,
            __in ApiFaultUtility & apiFaultUtility,
            __in KAllocator & allocator);

        __declspec(property(get = get_StateManager)) TestStateProviderManager::SPtr StateManager;
        TestStateProviderManager::SPtr get_StateManager() const
        {
            return testStateManager_;
        }

        __declspec(property(get = get_TestTransactionManager)) TestTransactionManager::SPtr TestTransactionManager;
        TestTransactionManager::SPtr get_TestTransactionManager() const
        {
            return testTransactionManager_;
        }

        __declspec(property(get = get_TransactionManager)) Data::LoggingReplicator::TransactionManager::SPtr TransactionManager;
        Data::LoggingReplicator::TransactionManager::SPtr get_TransactionManager() const
        {
            return transactionManager_;
        }

        __declspec(property(get = get_ReplicatedLogManager)) Data::LoggingReplicator::ReplicatedLogManager::SPtr ReplicatedLogManager;
        Data::LoggingReplicator::ReplicatedLogManager::SPtr get_ReplicatedLogManager() const
        {
            return replicatedLogManager_;
        }

        __declspec(property(get = get_CheckpointManager)) Data::LoggingReplicator::CheckpointManager::SPtr CheckpointManager;
        Data::LoggingReplicator::CheckpointManager::SPtr get_CheckpointManager() const
        {
            return checkpointManager_;
        }

        __declspec(property(get = get_RecoveryManager)) Data::LoggingReplicator::RecoveryManager::SPtr RecoveryManager;
        Data::LoggingReplicator::RecoveryManager::SPtr get_RecoveryManager() const
        {
            return recoveryManager_;
        }

        __declspec(property(get = get_PartitionId, put = set_PartitionId)) Data::Utilities::PartitionedReplicaId::SPtr PartitionId;
        Data::Utilities::PartitionedReplicaId::SPtr get_PartitionId() const
        {
            return prId_;
        }

        void set_PartitionId(Data::Utilities::PartitionedReplicaId::SPtr prId)
        {
            prId_ = prId;
        }

        __declspec(property(get = get_ReplicaId, put = set_ReplicaId)) ::FABRIC_REPLICA_ID ReplicaId;
        ::FABRIC_REPLICA_ID get_ReplicaId() const
        {
            return rId_;
        }

        void set_ReplicaId(int rId)
        {
            rId_ = rId;
        }

        __declspec(property(get = get_RecoveredOrCopiedCheckpointState)) Data::LoggingReplicator::RecoveredOrCopiedCheckpointState::SPtr RecoveredOrCopiedCheckpointState;
        Data::LoggingReplicator::RecoveredOrCopiedCheckpointState::SPtr get_RecoveredOrCopiedCheckpointState() const
        {
            return recoveredOrCopiedCheckpointState_;
        }

        __declspec(property(get = get_OperationProcessor)) Data::LoggingReplicator::OperationProcessor::SPtr OperationProcessor;
        Data::LoggingReplicator::OperationProcessor::SPtr get_OperationProcessor() const
        {
            return operationProcessor_;
        }

        __declspec(property(get = get_RoleContextDrainState)) Data::LoggingReplicator::RoleContextDrainState::SPtr RoleContextDrainState;
        Data::LoggingReplicator::RoleContextDrainState::SPtr get_RoleContextDrainState() const
        {
            return roleContextDrainState_;
        }

        __declspec(property(get = get_StateReplicator)) TestStateReplicator::SPtr StateReplicator;
        TestStateReplicator::SPtr get_StateReplicator() const
        {
            return testStateReplicator_;
        }

        __declspec(property(get = get_LogManager)) Data::LoggingReplicator::LogManager::SPtr LogManager;
        Data::LoggingReplicator::LogManager::SPtr get_LogManager() const
        {
            return logManager_;
        }

        __declspec(property(get = get_DataLogManager)) Data::Log::LogManager::SPtr DataLogManager;
        Data::Log::LogManager::SPtr get_DataLogManager() const
        {
            return dataLogManager_;
        }

        __declspec(property(get = get_CallbackManager)) Data::LoggingReplicator::PhysicalLogWriterCallbackManager::SPtr CallbackManager;
        Data::LoggingReplicator::PhysicalLogWriterCallbackManager::SPtr get_CallbackManager() const
        {
            return callbackManager_;
        }

        __declspec(property(get = get_RecordsDispatcher)) Data::LoggingReplicator::LogRecordsDispatcher::SPtr RecordsDispatcher;
        Data::LoggingReplicator::LogRecordsDispatcher::SPtr get_RecordsDispatcher() const
        {
            return recordsDispatcher_;
        }

        __declspec(property(get = get_TestLogTruncationManager)) TestLogTruncationManager::SPtr TestLogTruncationManager;
        TestLogTruncationManager::SPtr get_TestLogTruncationManager() const
        {
            return testLogTruncationManager_;
        }

        __declspec(property(get = get_LogTruncationManager)) Data::LoggingReplicator::LogTruncationManager::SPtr LogTruncationManager;
        Data::LoggingReplicator::LogTruncationManager::SPtr get_LogTruncationManager() const
        {
            return logTruncationManager_;
        }

        __declspec(property(get = get_ApiFaultUtility)) ApiFaultUtility::SPtr ApiFaultUtility;
        ApiFaultUtility::SPtr get_ApiFaultUtility() const
        {
            return apiFaultUtility_;
        }

        __declspec(property(get = get_BackupManager)) Data::LoggingReplicator::BackupManager::SPtr BackupManager;
        Data::LoggingReplicator::BackupManager::SPtr get_BackupManager() const
        {
            return backupManager_;
        }

        __declspec(property(get = get_LastFlushedPsn, put = set_LastFlushedPsn)) LONG64 LastFlushedPsn;
        LONG64 get_LastFlushedPsn() const
        {
            return lastFlushedPsn_;
        }

        void set_LastFlushedPsn(LONG64 psn)
        {
            lastFlushedPsn_ = psn;
        }

        __declspec(property(get = get_AssertInFlushCallback, put = set_AssertInFlushCallback)) bool AssertInFlushCallback;
        bool get_AssertInFlushCallback() const
        {
            return assertInFlushCallback_;
        }
        void set_AssertInFlushCallback(bool val)
        {
            assertInFlushCallback_ = val;
        }

        __declspec(property(get = get_UseFileLogManager, put = set_UseFileLogManager)) bool UseFileLogManager;
        bool get_UseFileLogManager() const
        {
            return useFileLogManager_;
        }
        void set_UseFileLogManager(bool val)
        {
            useFileLogManager_ = val;
        }

        __declspec(property(get = get_DedicatedLogPath, put = set_DedicatedLogPath)) KString const & DedicatedLogPath;
        KString const & get_DedicatedLogPath() const
        {
            return *dedicatedLogPath_;
        }
        void set_DedicatedLogPath(KString const & val)
        {
            NTSTATUS status = KString::Create(
                dedicatedLogPath_,
                *allocator_,
                val,
                true);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        __declspec(property(get = get_Config)) TxnReplicator::TRInternalSettingsSPtr & TxnReplicatorConfig;
        TxnReplicator::TRInternalSettingsSPtr get_Config()
        {
            return config_;
        }

        KString::CSPtr CreateFileName(
            __in wstring const & folderName,
            __in KAllocator & allocator)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

            BOOL result = PathAppendW(currentDirectoryPathCharArray, folderName.c_str());
            CODING_ERROR_ASSERT(result == TRUE);

            KString::SPtr fileName = nullptr;

#if !defined(PLATFORM_UNIX)
            status = KString::Create(fileName, allocator, L"\\??\\");
#else
            status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return fileName.RawPtr();
        }

        void ProcessFlushedRecordsCallback(__in Data::LoggingReplicator::LoggedRecords const & loggedRecords) noexcept override;

        void ProcessedLogicalRecord(__in Data::LogRecordLib::LogicalLogRecord & logicalRecord) noexcept override;

        void ProcessedPhysicalRecord(__in Data::LogRecordLib::PhysicalLogRecord & physicalRecord) noexcept override;

        Data::LogRecordLib::CopyContext::SPtr CreateACopyContext();

        Data::LogRecordLib::CopyContext::SPtr CreateACopyContextWReplicaProgressVector();

        Data::LogRecordLib::CopyContext::SPtr CreateACopyContext(
            __in Data::LogRecordLib::ProgressVector & progressVector,
            __in LONG64 logTailLsn,
            __in LONG64 latestRecoveredAtomicRedoOperationLsn,
            __in LONG64 logHeadRecordLsn);

        Data::LogRecordLib::CopyContext::SPtr CreateACopyContext(
            __in LONG64 replicaId,
            __in LONG64 logTailLsn,
            __in LONG64 latestRecoveredAtomicRedoOperationLsn,
            __in LONG64 epochDataLossNumber,
            __in LONG64 epochConfigurationNumber,
            __in LONG64 startingLogicalSequenceNumber,
            __in LONG64 logHeadRecordLsn,
            __out Common::DateTime & timeStamp);

        ktl::Awaitable<void> CloseAndQuiesceReplica();

        void Initialize(__in int seed);

        void Initialize(
            __in int seed, 
            __in bool skipChangeRoleToPrimary, 
            __in bool delayApis,
            __in bool useTestLogTruncationManager = true,
            __in TRANSACTIONAL_REPLICATOR_SETTINGS * publicSettings = nullptr);

        void Initialize(
            __in int seed, 
            __in bool skipChangeRoleToPrimary);

        ktl::Awaitable<void> InitializeAsync(
            __in int seed,
            __in bool skipChangeRoleToPrimary,
            __in bool delayApis,
            __in bool useTestLogTruncationManager = true,
            __in TRANSACTIONAL_REPLICATOR_SETTINGS * publicSettings = nullptr);

        void EndTest(__in bool reset);
        void EndTest(__in bool reset, __in bool cleanupLog);
        ktl::Awaitable<void> EndTestAsync(__in bool reset, __in bool cleanupLog);
        ktl::Awaitable<void> CreateLogManager();
        void DelayApis();

        TxnReplicator::IRuntimeFolders::CSPtr GetRuntimeFolders();

    private:

        ktl::Awaitable<void> WaitForLogFlushUptoLsn(__in LONG64 upToLsn);
        Data::LoggingReplicator::CopyStream::WaitForLogFlushUpToLsnCallback GetWaitForLogFlushUpToLsnCallback();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        Data::Utilities::PartitionedReplicaId::SPtr prId_;
        KAllocator* allocator_;
        LONG64 lastFlushedPsn_;
        Common::atomic_long physicalCallbackCount_;
        Common::atomic_long logicalCallbackCount_;

        Data::LogRecordLib::InvalidLogRecords::SPtr invalidLogRecords_;
        Data::LoggingReplicator::RecoveredOrCopiedCheckpointState::SPtr recoveredOrCopiedCheckpointState_;
        Data::LoggingReplicator::RoleContextDrainState::SPtr roleContextDrainState_;
        TestStateProviderManager::SPtr testStateManager_;
        TestLoggingReplicatorToVersionManager::SPtr versionManager_;
        Data::LoggingReplicator::BackupManager::SPtr backupManager_;
        Data::LoggingReplicator::CheckpointManager::SPtr checkpointManager_;
        Data::LoggingReplicator::OperationProcessor::SPtr operationProcessor_;
        Data::LoggingReplicator::LogRecordsDispatcher::SPtr recordsDispatcher_;
        Data::LoggingReplicator::LogManager::SPtr logManager_;
        Data::LoggingReplicator::PhysicalLogWriterCallbackManager::SPtr callbackManager_;
        Data::LoggingReplicator::ReplicatedLogManager::SPtr replicatedLogManager_;
        Data::LoggingReplicator::TransactionMap::SPtr transactionMap_;
        TestLogTruncationManager::SPtr testLogTruncationManager_;
        Data::LoggingReplicator::RecoveryManager::SPtr recoveryManager_;
        Data::LoggingReplicator::LogTruncationManager::SPtr logTruncationManager_;

        TestBackupRestoreProvider::SPtr testBackupRestoreProvider_;
        TestStateReplicator::SPtr testStateReplicator_;
        Data::LoggingReplicator::TransactionManager::SPtr transactionManager_;
        TestTransactionManager::SPtr testTransactionManager_;
        Data::TestCommon::TestStatefulServicePartition::SPtr testPartition_;
        LoggingReplicatorTests::ApiFaultUtility::SPtr apiFaultUtility_;
        Data::Log::LogManager::SPtr dataLogManager_;
        TxnReplicator::TRInternalSettingsSPtr config_;
        bool useFileLogManager_;
        bool isPrimary_;
        bool assertInFlushCallback_;

        TxnReplicator::TRPerformanceCountersSPtr perfCounters_;
        KString::SPtr dedicatedLogPath_;
        TxnReplicator::IRuntimeFolders::CSPtr runtimeFolders_;

        TestHealthClientSPtr healthClient_;
    };
}
