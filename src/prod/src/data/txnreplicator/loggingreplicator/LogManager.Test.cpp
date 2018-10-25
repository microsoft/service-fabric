// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

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

    StringLiteral const TraceComponent = "LogManagerTest";

    class LogManagerTests
    {
    protected:
        LogManagerTests()
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
            lastFlushedPsn_ = 0;
        }

        ~LogManagerTests()
        {
        }

        void EndTest();

        Awaitable<void> CreateLogManager();

        wstring cwd_;
        InvalidLogRecords::SPtr invalidRecords_;
        PhysicalLogWriterCallbackManager::SPtr callbackManager_;
        LONG64 lastFlushedPsn_;
        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        LogManager::SPtr logManager_;
        TestHealthClientSPtr healthClient_;
        TxnReplicator::TRInternalSettingsSPtr config_;

        KtlSystem * underlyingSystem_;

    private:
        void FlushCallback(LoggedRecords const & records);

        class FlushProcessor
            : public KObject<FlushProcessor>
            , public KShared<FlushProcessor>
            , public KWeakRefType<FlushProcessor>
            , public IFlushCallbackProcessor
        {
            K_FORCE_SHARED(FlushProcessor)
            K_SHARED_INTERFACE_IMP(IFlushCallbackProcessor)
            K_WEAKREF_INTERFACE_IMP(IFlushCallbackProcessor, FlushProcessor)

        public:
            static FlushProcessor::SPtr Create(__in KAllocator & allocator, LogManagerTests * parent)
            {
                return _new(KTL_TAG_TEST, allocator)FlushProcessor(parent);
            }

            void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override
            {
                parent_->FlushCallback(loggedRecords);
            }

        private:
            FlushProcessor(LogManagerTests * parent)
                : parent_(parent)
            {
            }

            LogManagerTests * parent_;
        };

        FlushProcessor::SPtr processor_;
    };

    LogManagerTests::FlushProcessor::~FlushProcessor()
    {
    }

    void LogManagerTests::EndTest()
    {
        logManager_->Dispose();
        logManager_.Reset();
        prId_.Reset();
        callbackManager_->Dispose();
        callbackManager_.Reset();
        invalidRecords_.Reset();
        processor_.Reset();
    }

    void LogManagerTests::FlushCallback(LoggedRecords const & records)
    {
        for (ULONG i = 0; i < records.Count; i++)
        {
            Trace.WriteInfo(TraceComponent, "{0} Flushed Psn: {1}", prId_->TraceId, records[i]->Psn);
            VERIFY_ARE_EQUAL(lastFlushedPsn_ + 1, records[i]->Psn);
            lastFlushedPsn_++;
            records[i]->CompletedFlush(STATUS_SUCCESS);
        }
    }

    Awaitable<void> LogManagerTests::CreateLogManager()
    {
        // TODO: Create ktl or other log managers here later
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

        TxnReplicator::TRPerformanceCountersSPtr counters = TRPerformanceCounters::CreateInstance(
            prId_->TracePartitionId.ToString(),
            prId_->ReplicaId);

        TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
        TransactionalReplicatorSettingsUPtr tmp;
        TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

        config_ = TRInternalSettings::Create(
            move(tmp),
            make_shared<TransactionalReplicatorConfig>());

        healthClient_ = TestHealthClient::Create();

        logManager_ = FileLogManager::Create(
            *prId_,
            counters,
            healthClient_,
            config_,
            allocator).RawPtr();

        CODING_ERROR_ASSERT(logManager_ != nullptr);

        cwd_ = GetWorkingDirectory();
        KString::SPtr kstring = KString::Create(cwd_.c_str(), allocator);
        CODING_ERROR_ASSERT(kstring != nullptr);

        co_await logManager_->InitializeAsync(*kstring);

        processor_ = FlushProcessor::Create(allocator, this);
        callbackManager_ = PhysicalLogWriterCallbackManager::Create(
            *prId_,
            allocator);
        callbackManager_->FlushCallbackProcessor = *processor_;

        CODING_ERROR_ASSERT(callbackManager_ != nullptr);
    }
    
    BOOST_FIXTURE_TEST_SUITE(LogManagerTestsSuite, LogManagerTests, * boost::unit_test::disabled())

    BOOST_AUTO_TEST_CASE(VerifyEmptyNewLogContents, * boost::unit_test::disabled())
    {
        TEST_TRACE_BEGIN("VerifyEmptyNewLogContents")

        {
            SyncAwait(CreateLogManager());
            RecoveryPhysicalLogReader::SPtr recoveryLogReader = nullptr;
            SyncAwait(logManager_->OpenAsync(recoveryLogReader));
            CODING_ERROR_ASSERT(recoveryLogReader != nullptr);
            
            LogRecord::SPtr tailRecord = SyncAwait(recoveryLogReader->SeekToLastRecord());
            CODING_ERROR_ASSERT(tailRecord != nullptr);
            LogRecord::SPtr headRecord = tailRecord;
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::CompleteCheckpoint);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::OneLsn);

            headRecord = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::EndCheckpoint);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::OneLsn);

            headRecord = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::Barrier);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::OneLsn);

            headRecord = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::BeginCheckpoint);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::ZeroLsn);

            headRecord = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::UpdateEpoch);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::ZeroLsn);

            headRecord = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(headRecord != nullptr);
            VERIFY_ARE_EQUAL(headRecord->RecordType, LogRecordType::Enum::Indexing);
            VERIFY_ARE_EQUAL(headRecord->Lsn, Constants::ZeroLsn);

            LogRecord::SPtr tailRecord2 = SyncAwait(recoveryLogReader->GetPreviousLogRecord(headRecord->RecordPosition));
            CODING_ERROR_ASSERT(tailRecord2 == nullptr);

            PhysicalLogRecord::SPtr physicalTail = dynamic_cast<PhysicalLogRecord *>(tailRecord.RawPtr());
            CODING_ERROR_ASSERT(physicalTail != nullptr);
            SyncAwait(recoveryLogReader->IndexPhysicalRecords(physicalTail));
            recoveryLogReader->Dispose();

            VERIFY_ARE_EQUAL(physicalTail->RecordType, LogRecordType::Enum::Indexing);
            VERIFY_ARE_EQUAL(physicalTail->Lsn, Constants::ZeroLsn);
 
            physicalTail = physicalTail->NextPhysicalRecord;
            CODING_ERROR_ASSERT(physicalTail != nullptr);
            VERIFY_ARE_EQUAL(physicalTail->RecordType, LogRecordType::Enum::BeginCheckpoint);
            VERIFY_ARE_EQUAL(physicalTail->Lsn, Constants::ZeroLsn);

            physicalTail = physicalTail->NextPhysicalRecord;
            CODING_ERROR_ASSERT(physicalTail != nullptr);
            VERIFY_ARE_EQUAL(physicalTail->RecordType, LogRecordType::Enum::EndCheckpoint);
            VERIFY_ARE_EQUAL(physicalTail->Lsn, Constants::OneLsn);

            physicalTail = physicalTail->NextPhysicalRecord;
            CODING_ERROR_ASSERT(physicalTail != nullptr);
            VERIFY_ARE_EQUAL(physicalTail->RecordType, LogRecordType::Enum::CompleteCheckpoint);
            VERIFY_ARE_EQUAL(physicalTail->Lsn, Constants::OneLsn);

            physicalTail = physicalTail->NextPhysicalRecord;
            CODING_ERROR_ASSERT(physicalTail != nullptr);
            VERIFY_ARE_EQUAL(physicalTail->Lsn, Constants::InvalidLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(CreateCopyLog_Verify, * boost::unit_test::disabled())
    {
        TEST_TRACE_BEGIN("CreateCopyLog_Verify")

        {
            SyncAwait(CreateLogManager());

            RecoveryPhysicalLogReader::SPtr recoveryLogReader = nullptr;
            SyncAwait(logManager_->OpenAsync(recoveryLogReader));
            CODING_ERROR_ASSERT(recoveryLogReader != nullptr);

            LogRecord::SPtr tailRecord = SyncAwait(recoveryLogReader->SeekToLastRecord());
            recoveryLogReader->Dispose();

            CODING_ERROR_ASSERT(tailRecord != nullptr);
            logManager_->PrepareToLog(*tailRecord, *callbackManager_);
            
            Epoch e(100, 100);
            IndexingLogRecord::SPtr copyLogHead = nullptr;
            SyncAwait(logManager_->CreateCopyLogAsync(e, 1000, copyLogHead));
            VERIFY_ARE_EQUAL(copyLogHead->Lsn, 1000);
            VERIFY_ARE_EQUAL(copyLogHead->CurrentEpoch, e);

            KString::SPtr cwd = KString::Create(cwd_.c_str(), allocator);
            KString::CSPtr basealias = LogManager::GenerateLogFileAlias(prId_->PartitionId, prId_->ReplicaId, allocator);
            KString::SPtr logFile = FileLogManager::GetFullPathToLog(*cwd, *basealias, allocator);

            bool originalLogExists = File::Exists(logFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(originalLogExists, true);
            
            KString::SPtr copyLogFileAlias = KString::Create(*basealias, allocator);
            copyLogFileAlias->Concat(L"_Copy");
            KString::SPtr copyLogFile = FileLogManager::GetFullPathToLog(*cwd, *copyLogFileAlias, allocator);

            bool copyLogExists = File::Exists(copyLogFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(copyLogExists, true);
        }
    }

    BOOST_AUTO_TEST_CASE(RenameCopyLog_Verify, * boost::unit_test::disabled())
    {
        TEST_TRACE_BEGIN("RenameCopyLog_Verify")

        {
            SyncAwait(CreateLogManager());

            RecoveryPhysicalLogReader::SPtr recoveryLogReader = nullptr;
            SyncAwait(logManager_->OpenAsync(recoveryLogReader));
            CODING_ERROR_ASSERT(recoveryLogReader != nullptr);

            LogRecord::SPtr tailRecord = SyncAwait(recoveryLogReader->SeekToLastRecord());
            recoveryLogReader->Dispose();
            recoveryLogReader.Reset();

            CODING_ERROR_ASSERT(tailRecord != nullptr);
            logManager_->PrepareToLog(*tailRecord, *callbackManager_);
            
            Epoch e(200, 200);
            IndexingLogRecord::SPtr copyLogHead = nullptr;
            SyncAwait(logManager_->CreateCopyLogAsync(e, 5000, copyLogHead));
            VERIFY_ARE_EQUAL(copyLogHead->Lsn, 5000);
            VERIFY_ARE_EQUAL(copyLogHead->CurrentEpoch, e);

            KString::SPtr cwd = KString::Create(cwd_.c_str(), allocator);
            KString::CSPtr basealias = LogManager::GenerateLogFileAlias(prId_->PartitionId, prId_->ReplicaId, allocator);
            KString::SPtr logFile = FileLogManager::GetFullPathToLog(*cwd, *basealias, allocator);

            bool originalLogExists = File::Exists(logFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(originalLogExists, true);
            
            KString::SPtr copyLogFileAlias = KString::Create(*basealias, allocator);
            copyLogFileAlias->Concat(L"_Copy");
            KString::SPtr copyLogFile = FileLogManager::GetFullPathToLog(*cwd, *copyLogFileAlias, allocator);

            bool copyLogExists = File::Exists(copyLogFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(copyLogExists, true);

            SyncAwait(logManager_->RenameCopyLogAtomicallyAsync());
            originalLogExists = File::Exists(logFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(originalLogExists, true);

            copyLogExists = File::Exists(copyLogFile->operator LPCWSTR());
            VERIFY_ARE_EQUAL(copyLogExists, true);

            VERIFY_IS_NOT_NULL(logManager_->CurrentLogTailRecord);
            VERIFY_ARE_EQUAL(logManager_->CurrentLogTailRecord->Lsn, 5000);
            
            // Verify tail of the log even by reading from disk 
            KString::SPtr readerName = KString::Create(L"Test", allocator);

            IPhysicalLogReader::SPtr reader = logManager_->GetPhysicalLogReader(
                0,
                logManager_->CurrentLogTailPosition,
                5000,
                *readerName,
                LogReaderType::Enum::Backup);

            IAsyncEnumerator<LogRecord::SPtr>::SPtr records = reader->GetLogRecords(
                *prId_,
                *readerName,
                LogReaderType::Enum::Backup,
                0,
                healthClient_,
                config_);

            reader->Dispose();
            reader.Reset();

            bool moved = SyncAwait(records->MoveNextAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(moved, true);

            LogRecord::SPtr record = records->GetCurrent();

            VERIFY_ARE_EQUAL(record->RecordType, LogRecordType::Enum::Indexing);
            VERIFY_ARE_EQUAL(record->Lsn, 5000);

            moved = SyncAwait(records->MoveNextAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(moved, false);

            records->Dispose();
            records.Reset();
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
