// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

extern void LR_Test_InitializeKtlConfig(
    __in KAllocator & allocator,
    __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

extern std::wstring GetWorkingDirectory();

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;
    using namespace Data::TestCommon;

    StringLiteral const TraceComponent = "LogTruncationManagerTests";
    wstring const TestLogFolder = L"LogTruncationManagerTests_log";

    class EmptyFlushProcessor
        : public KObject<EmptyFlushProcessor>
        , public KShared<EmptyFlushProcessor>
        , public KWeakRefType<EmptyFlushProcessor>
        , public IFlushCallbackProcessor
    {
        K_FORCE_SHARED(EmptyFlushProcessor)
        K_SHARED_INTERFACE_IMP(IFlushCallbackProcessor)
        K_WEAKREF_INTERFACE_IMP(IFlushCallbackProcessor, EmptyFlushProcessor)

    public:
        static EmptyFlushProcessor::SPtr Create(__in KAllocator & allocator)
        {
            return _new(KTL_TAG_TEST, allocator)EmptyFlushProcessor();
        }

        void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override
        {
            // Do not process log records with an error 
            ASSERT_IF(loggedRecords.LogError != STATUS_SUCCESS, "No flush error expected in LogTruncationManager.Test.cpp");

            LogRecord::SPtr currentLogRecord;
            for(ULONG i = 0; i < loggedRecords.Count; i++)
            {
                lastFlushedPsn_ = loggedRecords[i]->Psn;
            }
        }

        void WaitForFlushedPSN(__in LONG64 psn)
        {
            while(true)
            {
                if (lastFlushedPsn_ >= psn)
                {
                    return;
                }

                Sleep(100);
            }
        }

    private:
        LONG64 lastFlushedPsn_ = 0;
    };

    EmptyFlushProcessor::EmptyFlushProcessor()
    {
    }

    EmptyFlushProcessor::~EmptyFlushProcessor()
    {
    }

    class LogTruncationManagerWrapper
        : public KObject<LogTruncationManagerWrapper>
        , public KShared<LogTruncationManagerWrapper>
    {
        K_FORCE_SHARED(LogTruncationManagerWrapper)
    public:

        LogTruncationManagerWrapper(
            __in EmptyFlushProcessor::SPtr emptyFlushProcessor,
            __in LogTruncationManager::SPtr truncationManager,
            __in ReplicatedLogManager::SPtr replicatedLogManager,
            __in Data::Log::LogManager::SPtr dataLogManager)
            : dataLogManager_(dataLogManager)
            , emptyFlushProcessor_(emptyFlushProcessor)
            , isInitialized_(true)
            , logTruncationManager_(truncationManager)
            , replicatedLogManager_(replicatedLogManager)
        {
        }

        static LogTruncationManagerWrapper::SPtr CreateLogTruncationManagerWrapperAsync(
            __in PartitionedReplicaId::SPtr partitionedReplicaId,
            __in InvalidLogRecords::SPtr invalidLogRecords,
            __in TRInternalSettingsSPtr const & config,
            __in KAllocator & allocator)
        {
            // Create recovered or checkpoint state
            RecoveredOrCopiedCheckpointState::SPtr recoveredOrCopiedCheckpointState_ = RecoveredOrCopiedCheckpointState::Create(allocator);

            // Create API fault utility
            ApiFaultUtility::SPtr apiFaultUtility = ApiFaultUtility::Create(allocator);

            // Create RoleContextDrainState
            auto testPartition = TestStatefulServicePartition::Create(allocator);
            RoleContextDrainState::SPtr roleContextDrainState = RoleContextDrainState::Create(*testPartition, allocator);

            TestStateReplicator::SPtr testStateReplicator = TestStateReplicator::Create(*apiFaultUtility, allocator);

            Data::Log::LogManager::SPtr dataLogManager;
            LogManager::SPtr derivedLogManager = nullptr;

            TxnReplicator::TRPerformanceCountersSPtr counters = TRPerformanceCounters::CreateInstance(
                partitionedReplicaId->TracePartitionId.ToString(),
                partitionedReplicaId->ReplicaId);

            CODING_ERROR_ASSERT(config->Test_LoggingEngine == Constants::Test_Ktl_LoggingEngine);

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            LR_Test_InitializeKtlConfig(
                allocator,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::Create(
                allocator,
                dataLogManager);

            NTSTATUS status = SyncAwait(dataLogManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KTLLOGGER_SHARED_LOG_SETTINGS slSettings = { L"", L"", 0, 0, 0 };

            KtlLoggerSharedLogSettingsUPtr tmp2;
            KtlLoggerSharedLogSettings::FromPublicApi(slSettings, tmp2);

            TxnReplicator::SLInternalSettingsSPtr slConfig = SLInternalSettings::Create(
                move(tmp2));

            TestHealthClientSPtr healthClient = TestHealthClient::Create();

            TxnReplicator::IRuntimeFolders::SPtr runtimeFolders = Data::TestCommon::TestRuntimeFolders::Create(GetWorkingDirectory().c_str(), allocator);

            derivedLogManager = KLogManager::Create(
                *partitionedReplicaId,
                *runtimeFolders,
                *dataLogManager,
                config,
                slConfig,
                counters,
                healthClient,
                allocator).RawPtr();

            CODING_ERROR_ASSERT(derivedLogManager != nullptr);

            KString::SPtr workDirKstr = KString::Create(GetWorkingDirectory().c_str(), allocator);
            CODING_ERROR_ASSERT(workDirKstr != nullptr);

            // Initialize log manager with input log path
            SyncAwait(derivedLogManager->InitializeAsync(*workDirKstr));

            // Open FileLogManager and assign recovery log reader
            RecoveryPhysicalLogReader::SPtr recoveryLogReader = nullptr;
            SyncAwait(derivedLogManager->OpenAsync(recoveryLogReader));

            // Assign current log head record and current tail record
            LogRecord::SPtr currentRecord = SyncAwait(recoveryLogReader->GetNextLogRecord(0));
            IndexingLogRecord::SPtr currentHeadIndexingLogRecord = dynamic_cast<IndexingLogRecord *>(currentRecord.RawPtr());

            currentRecord = SyncAwait(recoveryLogReader->GetNextLogRecord(currentRecord->RecordPosition + 8 + currentRecord->RecordLength));
            currentRecord = SyncAwait(recoveryLogReader->GetNextLogRecord(currentRecord->RecordPosition + 8 + currentRecord->RecordLength));

            BeginCheckpointLogRecord::SPtr bcRecord = dynamic_cast<BeginCheckpointLogRecord *>(currentRecord.RawPtr());

            currentRecord = SyncAwait(recoveryLogReader->SeekToLastRecord());
            PhysicalLogRecord::SPtr tailRecordAtStartPhysical = dynamic_cast<PhysicalLogRecord *>(currentRecord.RawPtr());
            CompleteCheckPointLogRecord::SPtr ccRecord = dynamic_cast<CompleteCheckPointLogRecord *>(currentRecord.RawPtr());

            LogRecord::SPtr physicalRecordEc = SyncAwait(recoveryLogReader->GetPreviousLogRecord(ccRecord->RecordPosition));
            EndCheckpointLogRecord::SPtr ecRecord = dynamic_cast<EndCheckpointLogRecord *>(physicalRecordEc.RawPtr());

            SyncAwait(recoveryLogReader->IndexPhysicalRecords(tailRecordAtStartPhysical));

            EmptyFlushProcessor::SPtr processor = EmptyFlushProcessor::Create(allocator);

            // Create a callback manager instantiated with the EmptyFlushCallback
            PhysicalLogWriterCallbackManager::SPtr callbackManager = PhysicalLogWriterCallbackManager::Create(
                *partitionedReplicaId,
                allocator);
            callbackManager->FlushCallbackProcessor = *processor;

            // Prepare to log
            derivedLogManager->PrepareToLog(*currentRecord, *callbackManager);

            // Cast Derived LogManager type to LogManager::SPtr to be used in ReplicatedLogManager::Create
            LogManager::SPtr logManager = derivedLogManager.RawPtr();

            // Create replicated log manager
            ReplicatedLogManager::SPtr replicatedLogManager = ReplicatedLogManager::Create(
                *partitionedReplicaId,
                *logManager,
                *recoveredOrCopiedCheckpointState_,
                currentHeadIndexingLogRecord.RawPtr(),
                *roleContextDrainState,
                *testStateReplicator,
                *invalidLogRecords,
                allocator);

            // Create and initialize log truncation manager
            LogTruncationManager::SPtr logTruncationManager_ = LogTruncationManager::Create(
                *partitionedReplicaId,
                *replicatedLogManager,
                config,
                allocator);

            LogTruncationManagerWrapper * logTruncationMgrWrapper = _new(KTL_TAG_TEST, allocator)LogTruncationManagerWrapper(
                processor,
                logTruncationManager_,
                replicatedLogManager,
                dataLogManager);

            // Append information and flush
            replicatedLogManager->Information(InformationEvent::Recovered);

            replicatedLogManager->Reuse(
                *bcRecord->CurrentProgressVector,
                ecRecord.RawPtr(),
                ecRecord.RawPtr(),
                replicatedLogManager->LastInformationRecord.RawPtr(),
                *currentHeadIndexingLogRecord,
                currentHeadIndexingLogRecord->CurrentEpoch,
                currentRecord->Lsn);

            SyncAwait(replicatedLogManager->InnerLogManager->FlushAsync(L"CreateLogTruncationManagerAsync"));

            recoveryLogReader->Dispose();

            return LogTruncationManagerWrapper::SPtr(logTruncationMgrWrapper);
        }

        static LogTruncationManagerWrapper::SPtr CreateLogTruncationManagerWrapper(
            __in PartitionedReplicaId::SPtr partitionedReplicaId,
            __in InvalidLogRecords::SPtr invalidLogRecords,
            __in KAllocator & allocator,
            __in ULONG checkpointSizeInMb = TestCheckpointSizeInMB,
            __in ULONG minLogSizeInMb = TestMinLogSizeInMB,
            __in ULONG truncationThresholdFactor = TestTruncationThresholdFactor,
            __in ULONG throttlingThresholdFactor = TestThrottlingThresholdFactor)
        {
            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            txrSettings.CheckpointThresholdInMB = checkpointSizeInMb;
            txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
            txrSettings.MinLogSizeInMB = minLogSizeInMb;
            txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
            txrSettings.TruncationThresholdFactor = truncationThresholdFactor;
            txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
            txrSettings.ThrottlingThresholdFactor = throttlingThresholdFactor;
            txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;
            txrSettings.MaxStreamSizeInMB = TestStreamSizeInMB;
            txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;

            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);
            
            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(move(tmp), make_shared<TransactionalReplicatorConfig>());

            return 
                CreateLogTruncationManagerWrapperAsync(
                    partitionedReplicaId,
                    invalidLogRecords,
                    config,
                    allocator);
        }

        static ULONG const LogTruncationManagerWrapper::TestStreamSizeInMB = 200;
        static ULONG const LogTruncationManagerWrapper::TestCheckpointSizeInMB = 50;
        static ULONG const LogTruncationManagerWrapper::TestMinLogSizeInMB = 2;
        static ULONG const LogTruncationManagerWrapper::TestTruncationThresholdFactor = 2;
        static ULONG const LogTruncationManagerWrapper::TestThrottlingThresholdFactor = 4;
        static ULONG const LogTruncationManagerWrapper::DefaultStreamSizeInMb = 10;

        __declspec(property(get = get_TruncationManager)) LogTruncationManager::SPtr InnerTruncationManager;
        LogTruncationManager::SPtr get_TruncationManager() const
        {
            return logTruncationManager_;
        }

        __declspec(property(get = get_ReplicatedLogManager)) ReplicatedLogManager::SPtr ReplicatedLogManager;
        ReplicatedLogManager::SPtr get_ReplicatedLogManager() const
        {
            return replicatedLogManager_;
        }

        __declspec(property(get = get_InnerLogManager)) LogManager::SPtr InnerLogManager;
        LogManager::SPtr get_InnerLogManager() const
        {
            return replicatedLogManager_->InnerLogManager;
        }

        __declspec(property(get = get_InnerLogRecordsMap)) LogRecordsMap::SPtr InnerLogRecordsMap;
        LogRecordsMap::SPtr get_InnerLogRecordsMap() const
        {
            return logRecordsMap_;
        }

        __declspec(property(get = get_ProposedLogHeadRecord)) IndexingLogRecord::SPtr ProposedLogHeadRecord;
        IndexingLogRecord::SPtr get_ProposedLogHeadRecord() const
        {
            return proposedLogheadRecord_;
        }

        IndexingLogRecord::SPtr GenerateLogHead(
            __in PhysicalLogRecord & invalidRecord,
            __in KAllocator & allocator)
        {
            Epoch logHeadEpoch = Epoch(1, 2);

            IndexingLogRecord::SPtr indexingRecord = IndexingLogRecord::Create(
                logHeadEpoch, 
                InnerLogManager->CurrentLogTailRecord->Lsn, 
                replicatedLogManager_->LastLinkedPhysicalRecord.RawPtr(), 
                invalidRecord, 
                allocator);

            InnerLogManager->InsertBufferedRecord(*indexingRecord);
            proposedLogheadRecord_ = indexingRecord;

            return indexingRecord;
        }

        Awaitable<IndexingLogRecord::SPtr> StartTruncateHeadAsync(
            __in InvalidLogRecords & invalidRecords,
            __in KAllocator & allocator)
        {
            ASSERT_IFNOT(
                replicatedLogManager_->LastInProgressTruncateHeadRecord == nullptr,
                "LastInProgressTruncateHeadRecord must be null before LogTruncationManagerWrapper::StartTruncateHead");

            Epoch tailEpoch = Epoch(1, 2);
            replicatedLogManager_->SetTailEpoch(tailEpoch);
            replicatedLogManager_->Index();
            co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"StartTruncateHead");

            TruncateHeadLogRecord::SPtr truncateHead = TruncateHeadLogRecord::Create(
                *replicatedLogManager_->LastIndexRecord,
                InnerLogManager->CurrentLogTailRecord->Lsn,
                replicatedLogManager_->LastLinkedPhysicalRecord.RawPtr(), 
                *invalidRecords.Inv_PhysicalLogRecord,
                false,  // isStable
                0,      // Zero periodic truncation timestamp
                allocator);

            replicatedLogManager_->InsertTruncateHeadCallerHoldsLock(*truncateHead, *replicatedLogManager_->LastIndexRecord);

            co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"StartTruncateHead");
            co_return replicatedLogManager_->LastIndexRecord;
        }

        Awaitable<void> FlushAsync(__in KStringView const & initiator)
        {
            co_await InnerLogManager->FlushAsync(initiator);
            co_return;
        }

        Awaitable<void> CompleteTruncateHeadAsync()
        {
            ASSERT_IFNOT(
                replicatedLogManager_->LastInProgressTruncateHeadRecord != nullptr,
                "LastInProgressTruncateHeadRecord must be set before LogTruncationManagerWrapper::CompleteTruncateHeadAsync");

            co_await InnerLogManager->ProcessLogHeadTruncationAsync(*replicatedLogManager_->LastInProgressTruncateHeadRecord)->GetAwaitable();
            replicatedLogManager_->OnCompletePendingLogHeadTruncation();

            co_return;
        }

        Awaitable<void> BlockSecondaryPumpIfNeeded(__in LONG64 lsn, __in AwaitableCompletionSource<void>::SPtr tcs)
        {
            co_await logTruncationManager_->BlockSecondaryPumpIfNeeded(lsn);
            tcs->TrySet();
        }

        Awaitable<void> SignalCheckpointAfterDelay(__in ULONG delayInMs, __in KAllocator & allocator)
        {
            // Delay 1000 seconds
            co_await KTimer::StartTimerAsync(allocator, 0, delayInMs, nullptr);

            // Signal that the last in progress checkpoint is valid
            replicatedLogManager_->LastInProgressCheckpointRecord->CompletedProcessing();
            replicatedLogManager_->OnCompletePendingCheckpoint();

            co_return;
        }

        void Index()
        {
            replicatedLogManager_->Index();
        }

        void InsertDataRecord(
            __in ULONG sizeInBytes,
            __in InvalidLogRecords & invalidRecords,
            __in KAllocator & allocator) const
        {
            AtomicRedoOperation::SPtr tx = AtomicRedoOperation::CreateAtomicRedoOperation(-1, false, allocator);

            KBuffer::SPtr emptyBuffer;
            KBuffer::Create(sizeInBytes, emptyBuffer, allocator);

            KArray<KBuffer::CSPtr> buffers(allocator);
            buffers.Append(emptyBuffer.RawPtr());

            OperationData::CSPtr operationData = OperationData::Create(buffers, allocator);
            OperationLogRecord::SPtr atomicOperationLogRecord = OperationLogRecord::Create(
                *tx,
                nullptr,
                operationData.RawPtr(),
                nullptr,
                *invalidRecords.Inv_PhysicalLogRecord,
                *invalidRecords.Inv_TransactionLogRecord,
                allocator);

            atomicOperationLogRecord->ParentTransactionRecord = nullptr;

            InnerLogManager->InsertBufferedRecord(*atomicOperationLogRecord);
        }

        void StartCheckpoint(
            __in PartitionedReplicaId & traceId,
            __in InvalidLogRecords & invalidRecords,
            __in KAllocator & allocator,
            __in bool invalidRecordPosition = true)
        {
            ASSERT_IFNOT(
                replicatedLogManager_->LastInProgressCheckpointRecord == nullptr,
                "LastInProgressCheckpointRecord must be null before LogTruncationManagerWrapper::StartCheckpoint");

            Epoch logHeadEpoch = Epoch(1, 2);

            IndexingLogRecord::SPtr indexingLogRecord = IndexingLogRecord::Create(
                logHeadEpoch,
                InnerLogManager->CurrentLogTailRecord->Lsn,
                replicatedLogManager_->LastLinkedPhysicalRecord.RawPtr(), 
                *invalidRecords.Inv_PhysicalLogRecord,
                allocator);

            InnerLogManager->InsertBufferedRecord(*indexingLogRecord);
            logRecordsMap_ = LogRecordsMap::Create(traceId, *indexingLogRecord, invalidRecords, allocator);

            // Previous physical record and record position must be null and invalid for flush and write
            BeginCheckpointLogRecord::SPtr record = TestLogRecordUtility::CreateBeginCheckpointLogRecord(&invalidRecords, allocator, true, false, true);

            if (!invalidRecordPosition)
            {
                record->RecordPosition = replicatedLogManager_->CurrentLogHeadRecord->RecordPosition;
            }

            replicatedLogManager_->InsertBeginCheckpoint(*record);
        }

        void EndAndCompleteCheckpoint()
        {
            ASSERT_IFNOT(
                replicatedLogManager_->LastInProgressCheckpointRecord != nullptr,
                "LastInProgressCheckpointRecord must be set before LogTruncationManagerWrapper::EndAndCompleteCheckpoint");

            replicatedLogManager_->EndCheckpoint(*replicatedLogManager_->LastInProgressCheckpointRecord);
            replicatedLogManager_->CompleteCheckpoint();
            replicatedLogManager_->OnCompletePendingCheckpoint();
        }

        bool ShouldCheckpointPrimary(__out KArray<BeginTransactionOperationLogRecord::SPtr> & abortTxList)
        {
            return InnerTruncationManager->ShouldCheckpointOnPrimary(*logRecordsMap_->TransactionMapValue, abortTxList);
        }

        bool ShouldCheckpointSecondary()
        {
            return logTruncationManager_->ShouldCheckpointOnSecondary(*logRecordsMap_->TransactionMapValue);
        }

        void CleanupResources(
            __in KStringView & initiator,
            __in bool clearPhysicalLogWriterRecords = false)
        {
            if (clearPhysicalLogWriterRecords)
            {
                ClearRecords();
            }

            // Flush remaining records and dispose LogManager
            SyncAwait(FlushAsync(initiator));

            // Wait for callback processing to complete
            emptyFlushProcessor_->WaitForFlushedPSN(InnerLogManager->PhysicalLogWriter->CurrentLogTailRecord->Psn);

            InnerLogManager->Dispose();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            Sleep(1000);
            
            if (dataLogManager_ != nullptr)
            {
                SyncAwait(dataLogManager_->CloseAsync(CancellationToken::None));
            }

            emptyFlushProcessor_.Reset();

            Directory::Delete_WithRetry(TestLogFolder, true, true);
        }

        void ClearRecords()
        {
            ReplicatedLogManager->InnerLogManager->PhysicalLogWriter->Test_ClearRecords();
        }

    private:

        bool isInitialized_;
        Data::Log::LogManager::SPtr dataLogManager_;
        LogTruncationManager::SPtr logTruncationManager_;
        ReplicatedLogManager::SPtr replicatedLogManager_;
        IndexingLogRecord::SPtr proposedLogheadRecord_;
        LogRecordsMap::SPtr logRecordsMap_;
        EmptyFlushProcessor::SPtr emptyFlushProcessor_;
    };


    LogTruncationManagerWrapper::LogTruncationManagerWrapper()
    {
    }

    LogTruncationManagerWrapper::~LogTruncationManagerWrapper()
    {
    }

    class LogTruncationManagerTests
    {
    protected:
        LogTruncationManagerTests()
        {
        }

        void EndTest()
        {
            prId_.Reset();
            invalidRecords_.Reset();
        }

        void InitializeTest()
        {
            KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
            invalidRecords_ = InvalidLogRecords::Create(allocator);
        }

        AwaitableCompletionSource<void>::SPtr GetAwaitableCompletionSource() 
        {
            KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

            AwaitableCompletionSource<void>::SPtr acs = nullptr;
            NTSTATUS status = AwaitableCompletionSource<void>::Create(allocator, 0, acs);
            
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            return acs;
        }

        KGuid pId_;
        FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
        InvalidLogRecords::SPtr invalidRecords_;
    };

    BOOST_FIXTURE_TEST_SUITE(LogTruncationManagerTestsSuite, LogTruncationManagerTests)

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_ProposedHeadIsNotFlushed_False)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_ProposedHeadIsNotFlushed_False")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_ProposedHeadIsNotFlushed_False";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // IndexingLogRecord added to buffer but not flushed
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);
            
            // The truncate process is ready for completion
            bool isGoodLogHeadCandidate = logTruncationManager->InnerTruncationManager->IsGoodLogHeadCandidate(*proposedLogHead);

            VERIFY_ARE_EQUAL(isGoodLogHeadCandidate, false);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_ProposedHeadTooCloseToCurrentHead_False)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_ProposedHeadTooCloseToCurrentHead_False")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_ProposedHeadTooCloseToCurrentHead_False";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // IndexingLogRecord added to buffer
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);

            // Proposed log head record is flushed
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // The truncate process is ready for completion
            bool isGoodLogHeadCandidate = logTruncationManager->InnerTruncationManager->IsGoodLogHeadCandidate(*proposedLogHead);

            VERIFY_ARE_EQUAL(isGoodLogHeadCandidate, false);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_0)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_0")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_0";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;
            ULONG worthyTruncationSizeInBytes = 1024 * 1024;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // Make the truncation worth it
            logTruncationManager->InsertDataRecord(worthyTruncationSizeInBytes, *invalidRecords_, allocator);
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);

            // Note that proposed head is the current tail.
            // Flush to ensure that proposeHead has a valid position - making it applicable.
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // The truncate process is ready for completion
            bool isGoodLogHeadCandidate = logTruncationManager->InnerTruncationManager->IsGoodLogHeadCandidate(*proposedLogHead);

            VERIFY_ARE_EQUAL(isGoodLogHeadCandidate, false);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_1)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_1")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_ProposedHeadTooCloseToTail_False_1";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG worthyTruncationSizeInBytes = 1024 * 1024;

            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // Make the truncation worth it
            logTruncationManager->InsertDataRecord(worthyTruncationSizeInBytes, *invalidRecords_, allocator);
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);

            // Ensure that if proposed head were to be accepted, it would not leave enough log (> min log size)
            logTruncationManager->InsertDataRecord(minLogSizeInMB * 1024 * 1024 / 2, *invalidRecords_, allocator);

            // Flush to ensure that proposeHead has a valid position - making it applicable.
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // The truncate process is ready for completion
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->IsGoodLogHeadCandidate(*proposedLogHead));

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_ProposedHeadJustRight_True)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_ProposedHeadJustRight_True")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_ProposedHeadJustRight_True";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG worthyTruncationSizeInBytes = 1024 * 1024;
            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // Make the truncation worth it
            logTruncationManager->InsertDataRecord(worthyTruncationSizeInBytes, *invalidRecords_, allocator);
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);

            // Ensure that if proposed head were to be accepted, it would leave enough log (> min log size)
            logTruncationManager->InsertDataRecord(minLogSizeInMB * 1024 * 1024, *invalidRecords_, allocator);

            // Flush to ensure that proposeHead has a valid position - making it applicable.
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // The truncate process is ready for completion
            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->IsGoodLogHeadCandidate(*proposedLogHead));

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(IsGoodLogHeadCandidate_LogHeadCalculator)
    {
        TEST_TRACE_BEGIN("IsGoodLogHeadCandidate_LogHeadCalculator")
        {
            InitializeTest();
            wstring testName = L"IsGoodLogHeadCandidate_LogHeadCalculator";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;
            ULONG worthyTruncationSizeInBytes = 1024 * 1024;
            ULONG throttlingThresholdFactor = LogTruncationManagerWrapper::TestThrottlingThresholdFactor;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            // Make the truncation worth it
            logTruncationManager->InsertDataRecord(worthyTruncationSizeInBytes, *invalidRecords_, allocator);
            IndexingLogRecord::SPtr proposedLogHead = logTruncationManager->GenerateLogHead(*invalidRecords_->Inv_PhysicalLogRecord, allocator);

            // Ensure that if proposed head were to be accepted, it would leave enough log (> min log size)
            logTruncationManager->InsertDataRecord(minLogSizeInMB * 1024 * 1024, *invalidRecords_, allocator);

            // Flush to ensure that proposeHead has a valid position - making it applicable.
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // The truncate process is ready for completion
            // Retrieve the log head calculator delegate function
            ReplicatedLogManager::IsGoodLogHeadCandidateCalculator goodLogHeadCalculator;
            goodLogHeadCalculator = logTruncationManager->InnerTruncationManager->GetGoodLogHeadCandidateCalculator();

            bool isGoodLogHeadCandidate = goodLogHeadCalculator(*proposedLogHead);

            // Verify the IsGoodLogHeadCandidateCalculator behaves as expected, and returned true
            VERIFY_IS_TRUE(isGoodLogHeadCandidate);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldIndex_False_LastIndexRecordNull)
    {
        TEST_TRACE_BEGIN("ShouldIndex_False_LastIndexRecordNull")
        {
            wstring testName = L"ShouldIndex_False_LastIndexRecordNull";
            KStringView initiator = KStringView(testName.c_str());

            InitializeTest();
            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldIndex_False_InvalidLastRecordPosition)
    {
        TEST_TRACE_BEGIN("ShouldIndex_False_InvalidLastRecordPosition")
        {
            wstring testName = L"ShouldIndex_LastIndexRecordNull";
            KStringView initiator = KStringView(testName.c_str());

            InitializeTest();
            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            logTruncationManager->Index();

            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldIndex_True)
    {
        TEST_TRACE_BEGIN("ShouldIndex_True")
        {
            wstring testName = L"ShouldIndex_True";
            KStringView initiator = KStringView(testName.c_str());

            InitializeTest();
            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            logTruncationManager->Index();

            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            logTruncationManager->InsertDataRecord(LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            
            // Flush indexing and inserted records
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Verify indexing threshold has been passed
            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldIndex());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldTruncateHead_TruncationInProgress_False)
    {
        TEST_TRACE_BEGIN("ShouldTruncateHead_TruncationInProgress_False")
        {
            wstring testName = L"ShouldTruncateHead_TruncationInProgress_False";
            KStringView initiator = KStringView(testName.c_str());

            InitializeTest();
            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            logTruncationManager->InsertDataRecord(LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->StartTruncateHeadAsync( *invalidRecords_,allocator));
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());
            SyncAwait(logTruncationManager->CompleteTruncateHeadAsync());
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldTruncateHead_LogSizeIsAboveTruncationThreshold_True)
    {
        TEST_TRACE_BEGIN("ShouldTruncateHead_LogSizeIsAboveTruncationThreshold_True")
        {
            InitializeTest();
            wstring testName = L"ShouldTruncateHead_LogSizeIsAboveTruncationThreshold_True";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor);

            logTruncationManager->InsertDataRecord(LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator);
            logTruncationManager->EndAndCompleteCheckpoint();
            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldTruncateHead_LogSizeIsLessThanTruncationThreshold_False)
    {
        TEST_TRACE_BEGIN("ShouldTruncateHead_LogSizeIsLessThanTruncationThreshold_False")
        {
            InitializeTest();
            wstring testName = L"ShouldTruncateHead_LogSizeIsLessThanTruncationThreshold_False";
            KStringView initiator = KStringView(testName.c_str());

            ULONG currentLogSize = 3 * 1024 * 1024;
            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor);

            logTruncationManager->InsertDataRecord(currentLogSize, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator);
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            logTruncationManager->EndAndCompleteCheckpoint();
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldTruncateHead_LogSizeIsEmpty_False)
    {
        TEST_TRACE_BEGIN("ShouldTruncateHead_LogSizeIsEmpty_False")
        {
            InitializeTest();
            wstring testName = L"ShouldTruncateHead_LogSizeIsEmpty_False";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = 2;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor);

            SyncAwait(logTruncationManager->FlushAsync(initiator));

            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator);
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            logTruncationManager->EndAndCompleteCheckpoint();
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldTruncateHead());

            // Flush remaining records and dispose LogManager
            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(OldTx_OnlyNewTx)
    {
        TEST_TRACE_BEGIN("OldTx_OnlyNewTx")
        {
            InitializeTest();
            wstring testName = L"OldTx_OnlyNewTx";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            BeginTransactionOperationLogRecord::SPtr recordOne = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *invalidRecords_, allocator, true, true, true);
            BeginTransactionOperationLogRecord::SPtr recordTwo = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *invalidRecords_, allocator, true, true, true);

            recordOne->RecordPosition = 1;
            recordOne->Lsn = 1;
            recordTwo->RecordPosition = 2;
            recordTwo->Lsn = 2;

            TransactionMap::SPtr txMap = TransactionMap::Create(*prId_, allocator);
            txMap->CreateTransaction(*recordOne);
            txMap->CreateTransaction(*recordTwo);

            KArray<BeginTransactionOperationLogRecord::SPtr> oldTxns = logTruncationManager->InnerTruncationManager->GetOldTransactions(*txMap);
            VERIFY_IS_TRUE(oldTxns.Count() == 0);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(OldTx_TwoOldTx)
    {
        TEST_TRACE_BEGIN("OldTx_TwoOldTx")
        {
            InitializeTest();
            wstring testName = L"OldTx_TwoOldTx";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(prId_, invalidRecords_, allocator);

            BeginTransactionOperationLogRecord::SPtr recordOne = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *invalidRecords_, allocator, true, true, true);
            BeginTransactionOperationLogRecord::SPtr recordTwo = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *invalidRecords_, allocator, true, true, true);
            BeginTransactionOperationLogRecord::SPtr recordThree = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *invalidRecords_, allocator, true, true, true);

            recordOne->RecordPosition = 1;
            recordOne->Lsn = 1;
            recordTwo->RecordPosition = 2;
            recordTwo->Lsn = 2;
            recordThree->Lsn = 3;

            TransactionMap::SPtr txMap = TransactionMap::Create(*prId_, allocator);
            txMap->CreateTransaction(*recordOne);
            txMap->CreateTransaction(*recordTwo);
            txMap->CreateTransaction(*recordThree);

            logTruncationManager->InsertDataRecord(10 * LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            ULONG64 oldTxOffset = 
                logTruncationManager->InnerLogManager->CurrentLogTailPosition + 
                logTruncationManager->InnerLogManager->BufferedBytes + 
                logTruncationManager->InnerLogManager->PendingFlushBytes + 
                logTruncationManager->InnerLogManager->CurrentLogTailRecord->ApproximateSizeOnDisk - 1;

            recordThree->RecordPosition = oldTxOffset;

            logTruncationManager->InnerLogManager->SetLogHeadRecordPosition(50);

            // The third transaction is not older than the oldTxOffset, only two transactions should be valid
            KArray<BeginTransactionOperationLogRecord::SPtr> oldTxns = logTruncationManager->InnerTruncationManager->GetOldTransactions(*txMap);
            VERIFY_IS_TRUE(oldTxns.Count() == 2);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockOperationsPrimary_LogUseGreaterThanThrottlingThreshold)
    {
        TEST_TRACE_BEGIN("ShouldBlockOperationsPrimary_LogUseGreaterThanThrottlingThreshold")
        {
            InitializeTest();
            wstring testName = L"ShouldBlockOperationsPrimary_LogUseGreaterThanThrottlingThreshold";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = LogTruncationManagerWrapper::TestTruncationThresholdFactor;
            ULONG throttlingThresholdFactor = 3;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            for (ULONG i = 0; i < throttlingThresholdFactor - 1; i++)
            {
                logTruncationManager->InsertDataRecord(minLogSizeInMB * 1024 * 1024, *invalidRecords_, allocator);
                SyncAwait(logTruncationManager->FlushAsync(initiator));

                VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
                VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);
                VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());
            }

            logTruncationManager->InsertDataRecord(minLogSizeInMB * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockOperationsOnPrimary_EmptyLog)
    {
        TEST_TRACE_BEGIN("ShouldBlockOperationsOnPrimary_EmptyLog")
        {
            InitializeTest();

            wstring testName = L"ShouldBlockOperationsOnPrimary_EmptyLog";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockOperationsOnPrimary_PendingTruncationOf79PercentFull)
    {
        TEST_TRACE_BEGIN("ShouldBlockOperationsOnPrimary_PendingTruncationOf79PercentFull")
        {
            InitializeTest();
            wstring testName = L"ShouldBlockOperationsOnPrimary_PendingTruncationOf79PercentFull";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            DOUBLE logUsage = 0.79 * LogTruncationManagerWrapper::DefaultStreamSizeInMb;

            logTruncationManager->InsertDataRecord((ULONG)logUsage * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockOperationsOnPrimary_BeforeAndAfterTruncation_TrueThenFalse)
    {
        TEST_TRACE_BEGIN("ShouldBlockOperationsOnPrimary_BeforeAndAfterTruncation_TrueThenFalse")
        {
            InitializeTest();
            wstring testName = L"ShouldBlockOperationsOnPrimary_BeforeAndAfterTruncation_TrueThenFalse";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = LogTruncationManagerWrapper::TestTruncationThresholdFactor;
            ULONG throttlingThresholdFactor = 3;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            logTruncationManager->InsertDataRecord(minLogSizeInMB * throttlingThresholdFactor * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());

            // Start truncation
            SyncAwait(logTruncationManager->StartTruncateHeadAsync( *invalidRecords_,allocator));

            // Finish truncation
            SyncAwait(logTruncationManager->CompleteTruncateHeadAsync());

            // Operations on primary should be allowed after truncation
            VERIFY_IS_FALSE(logTruncationManager->InnerTruncationManager->ShouldBlockOperationsOnPrimary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockSecondary_NoCompletedCheckpointBlockingSecondary)
    {
        TEST_TRACE_BEGIN("ShouldBlockSecondary_NoCompletedCheckpointBlockingSecondary")
        {     
            wstring testName = L"ShouldBlockSecondary_NoCompletedCheckpointBlockingSecondary";
            KStringView initiator = KStringView(testName.c_str());

            InitializeTest();

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            ktl::AwaitableCompletionSource<void>::SPtr acs = GetAwaitableCompletionSource();

            VERIFY_IS_TRUE(status == STATUS_SUCCESS);

            Awaitable<void> blockSecondaryPumpTask = logTruncationManager->BlockSecondaryPumpIfNeeded(1, acs);

            // Verify the block secondary task has completed
            // This indicates BlockSecondaryPump exited before AwaitProcessing
            VERIFY_IS_TRUE(acs->IsCompleted());

            SyncAwait(blockSecondaryPumpTask);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockSecondary_NoPendingCheckpointAndTruncation)
    {
        TEST_TRACE_BEGIN("ShouldBlockSecondary_NoPendingCheckpointAndTruncation")
        {
            InitializeTest();

            wstring testName = L"ShouldBlockSecondary_NoPendingCheckpointAndTruncation";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            ULONG currentLogSize = 3 * 1024 * 1024;
            logTruncationManager->InsertDataRecord(currentLogSize, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Perform checkpoint to ensure LastCompletedBeginCheckpointRecord is non-null
            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator);
            logTruncationManager->EndAndCompleteCheckpoint();

            ktl::AwaitableCompletionSource<void>::SPtr acs = GetAwaitableCompletionSource();

            VERIFY_IS_TRUE(status == STATUS_SUCCESS);

            Awaitable<void> blockSecondaryPumpTask = logTruncationManager->BlockSecondaryPumpIfNeeded(1, acs);

            // Verify the block secondary task has completed
            // This indicates BlockSecondaryPump exited before AwaitProcessing
            VERIFY_IS_TRUE(acs->IsCompleted());
            SyncAwait(blockSecondaryPumpTask);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldBlockSecondary_AwaitProcessingPendingCheckpoint)
    {
        TEST_TRACE_BEGIN("ShouldBlockSecondary_AwaitProcessingPendingCheckpoint")
        {
            InitializeTest();

            wstring testName = L"ShouldBlockSecondary_AwaitProcessingPendingCheckpoint";
            KStringView initiator = KStringView(testName.c_str());

            ULONG minLogSizeInMB = 2;
            ULONG checkpointSizeInMB = 1;
            ULONG truncationThresholdFactor = LogTruncationManagerWrapper::TestTruncationThresholdFactor;
            ULONG throttlingThresholdFactor = 3;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointSizeInMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);

            logTruncationManager->InsertDataRecord(minLogSizeInMB * throttlingThresholdFactor * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->PendingFlushBytes == 0);
            VERIFY_IS_TRUE(logTruncationManager->InnerLogManager->BufferedBytes == 0);

            // Create earliest pending tx record
            BeginTransactionOperationLogRecord::SPtr beginTxRecord = BeginTransactionOperationLogRecord::Create(
                LogRecordType::Enum::BeginTransaction,
                100,
                1,
                *invalidRecords_->Inv_PhysicalLogRecord,
                *invalidRecords_->Inv_TransactionLogRecord,
                allocator);

            beginTxRecord->RecordEpoch = Epoch(1, 0);

            // Create last backup record
            KGuid guid;
            guid.CreateNew();

            LONG64 highestBackedUpLsn = 1;
            UINT backupLogRecordCount = 1;
            UINT backupLogSize = 10;

            BackupLogRecord::SPtr backupLogRecord = BackupLogRecord::Create(
                guid,
                Epoch(1, 0),
                highestBackedUpLsn,
                backupLogRecordCount,
                backupLogSize,
                *invalidRecords_->Inv_PhysicalLogRecord,
                allocator);

            // Create progress vector
            KArray<ProgressVectorEntry> beginCheckpointProgressVectorEntries = TestLogRecordUtility::ProgressVectorEntryArray(allocator, 10, false);
            ProgressVector::SPtr recordVector = ProgressVector::Create(allocator);
            TestLogRecordUtility::PopulateProgressVectorFromEntries(*recordVector, beginCheckpointProgressVectorEntries);
            LONG64 lsn = 3;

            // Create a linked physical record
            PhysicalLogRecord::SPtr linkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(
                *invalidRecords_->Inv_PhysicalLogRecord,
                allocator,
                LoggingReplicatorTests::LinkedPhysicalRecordType::CompleteCheckpoint);

            // Create begin checkpoint log record
            BeginCheckpointLogRecord::SPtr beginCheckpointLogRecord = BeginCheckpointLogRecord::Create(
                false,
                *recordVector,
                beginTxRecord.RawPtr(),
                Epoch(1, 1),
                Epoch(1, 1),
                lsn,
                linkedPhysicalRecord.RawPtr(),
                *invalidRecords_->Inv_PhysicalLogRecord,
                *backupLogRecord,
                0,
                0,
                0,
                allocator);

            logTruncationManager->ReplicatedLogManager->InsertBeginCheckpoint(*beginCheckpointLogRecord);
            logTruncationManager->ReplicatedLogManager->EndCheckpoint(*beginCheckpointLogRecord);

            Stopwatch stopWatch;

            // Start a background task that waits 1 second then signals that the in progress checkpoint is valid 
            Awaitable<void> delayedAwaitProcessing = logTruncationManager->SignalCheckpointAfterDelay(1000, allocator);
            ToTask(delayedAwaitProcessing);

            // Measure the duration of BlockSecondaryPumpIsNeeded(lastStableLsn=4)
            // In the correct execution path, BlockSecondaryPumpIsNeeded will block on LogRecord::AwaitProcessing 
            // until signalled by the delayedAwaitProcessing Awaitable<void>
            stopWatch.Start();
                
            // Block secondary pump at a greater lsn than the initial record
            SyncAwait(logTruncationManager->InnerTruncationManager->BlockSecondaryPumpIfNeeded(4));

            stopWatch.Stop();

            // Verify that LogTruncationManager was blocked until AwaitProcessing has been signalled
            VERIFY_IS_TRUE(stopWatch.ElapsedMilliseconds > 500);

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldCheckpointPrimary_PendingCheckpointOf20PercentLog)
    {
        TEST_TRACE_BEGIN("ShouldCheckpointPrimary_PendingCheckpointOf20PercentLog")
        {
            InitializeTest();

            wstring testName = L"ShouldCheckpointPrimary_PendingCheckpointOf20PercentLog";
            KStringView initiator = KStringView(testName.c_str());

            KArray<BeginTransactionOperationLogRecord::SPtr> resultsList(allocator);

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            DOUBLE logUsage = 0.2;
            logTruncationManager->InsertDataRecord((ULONG)logUsage * LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Start the checkpoint process
            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator, false);

            // Should not checkpoint, currently in progress
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointPrimary(resultsList));

            // Finish the checkpoint process
            logTruncationManager->EndAndCompleteCheckpoint();

            // Should not checkpoint, not enough bytes written
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointPrimary(resultsList));

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldCheckpointPrimary_PendingCheckpointOf80PercentLogAndThenCheckpoint)
    {
        TEST_TRACE_BEGIN("ShouldCheckpointPrimary_PendingCheckpointOf80PercentLogAndThenCheckpoint")
        {
            InitializeTest();

            wstring testName = L"ShouldCheckpointPrimary_PendingCheckpointOf80PercentLogAndThenCheckpoint";
            KStringView initiator = KStringView(testName.c_str());

            KArray<BeginTransactionOperationLogRecord::SPtr> resultsList(allocator);
            ULONG checkpointThresholdInMb = 2;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointThresholdInMb);

            logTruncationManager->InsertDataRecord(LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Start the checkpoint process
            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator, false);

            // Should not checkpoint, currently in progress
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointPrimary(resultsList));

            // Finish the checkpoint process
            logTruncationManager->EndAndCompleteCheckpoint();

            // Confirm checkpoint 
            VERIFY_IS_TRUE(logTruncationManager->ShouldCheckpointPrimary(resultsList));

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldCheckpointSecondary_PendingCheckpointOf20PercentLog)
    {
        TEST_TRACE_BEGIN("ShouldCheckpointSecondary_PendingCheckpointOf20PercentLog")
        {
            InitializeTest();

            wstring testName = L"ShouldCheckpointSecondary_PendingCheckpointOf20PercentLog";
            KStringView initiator = KStringView(testName.c_str());

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator);

            DOUBLE logUsage = 0.2;
            logTruncationManager->InsertDataRecord((ULONG)logUsage * LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Start the checkpoint process
            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator, false);

            // Should not checkpoint, no completed checkpoints
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointSecondary());

            // Finish the checkpoint process
            logTruncationManager->EndAndCompleteCheckpoint();

            // Should not checkpoint, not enough bytes written
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointSecondary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ShouldCheckpointSecondary_PendingCheckpointOf80PercentLogAndThenCheckpoint)
    {
        TEST_TRACE_BEGIN("ShouldCheckpointSecondary_PendingCheckpointOf80PercentLogAndThenCheckpoint")
        {
            InitializeTest();

            wstring testName = L"ShouldCheckpointSecondary_PendingCheckpointOf80PercentLogAndThenCheckpoint";
            KStringView initiator = KStringView(testName.c_str());

            ULONG checkpointThresholdInMb = 2;

            LogTruncationManagerWrapper::SPtr logTruncationManager = LogTruncationManagerWrapper::CreateLogTruncationManagerWrapper(
                prId_,
                invalidRecords_,
                allocator,
                checkpointThresholdInMb);

            logTruncationManager->InsertDataRecord(LogTruncationManagerWrapper::DefaultStreamSizeInMb * 1024 * 1024, *invalidRecords_, allocator);
            SyncAwait(logTruncationManager->FlushAsync(initiator));

            // Start the checkpoint process
            logTruncationManager->StartCheckpoint(*prId_, *invalidRecords_, allocator, false);

            // Should not checkpoint, no completed checkpoints
            VERIFY_IS_FALSE(logTruncationManager->ShouldCheckpointSecondary());

            // Finish the checkpoint process
            logTruncationManager->EndAndCompleteCheckpoint();

            // Should not checkpoint, not enough bytes written
            VERIFY_IS_TRUE(logTruncationManager->ShouldCheckpointSecondary());

            // Ensure all resources are disposed before end of test
            logTruncationManager->CleanupResources(initiator, true);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
