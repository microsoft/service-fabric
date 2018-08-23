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

    StringLiteral const TraceComponent = "LogManagerTest";
    wstring const TestLogFolder = L"log";
    KStringView const CopySuffix = L"_Copy";

    class KLogManagerTests
    {
    protected:
        KLogManagerTests()
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
            lastFlushedPsn_ = 0;
        }

        ~KLogManagerTests()
        {
        }

        void EndTest();

        Awaitable<void> CreateLogManager();
        Awaitable<void> ShutdownLogManager();

        TestHealthClientSPtr healthClient_;
        TxnReplicator::TRInternalSettingsSPtr config_;
        InvalidLogRecords::SPtr invalidRecords_;
        PhysicalLogWriterCallbackManager::SPtr callbackManager_;
        LONG64 lastFlushedPsn_;
        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        LogManager::SPtr logManager_;
        Data::Log::LogManager::SPtr dataLogManager_;
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
            static FlushProcessor::SPtr Create(__in KAllocator & allocator, KLogManagerTests * parent)
            {
                return _new(KTL_TAG_TEST, allocator)FlushProcessor(parent);
            }

            void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override
            {
                parent_->FlushCallback(loggedRecords);
            }

        private:
            FlushProcessor(KLogManagerTests * parent)
                : parent_(parent)
            {
            }

            KLogManagerTests * parent_;
        };

        FlushProcessor::SPtr processor_;
    };

    KLogManagerTests::FlushProcessor::~FlushProcessor()
    {
    }

    void KLogManagerTests::EndTest()
    {
        // Close/Delete the KTLLogManager in Data::LoggingReplicator
        SyncAwait(logManager_->DeleteLogAsync());
        logManager_->Dispose();

        // Close the Data::LogicalLog::LogManager
        SyncAwait(dataLogManager_->CloseAsync(CancellationToken::None));

        Trace.WriteInfo(TraceComponent,
            "{0} KLogManagerTests::EndTest - log deleted and closed",
            prId_->TraceId);

        logManager_.Reset();
        prId_.Reset();
        callbackManager_->Dispose();
        callbackManager_.Reset();
        invalidRecords_.Reset();
        processor_.Reset();
        dataLogManager_.Reset();
        config_.reset();
        healthClient_.reset();
    }

    void KLogManagerTests::FlushCallback(LoggedRecords const & records)
    {
        for (ULONG i = 0; i < records.Count; i++)
        {
            Trace.WriteInfo(TraceComponent, "{0} Flushed Psn: {1}", prId_->TraceId, records[i]->Psn);
            VERIFY_ARE_EQUAL(lastFlushedPsn_ + 1, records[i]->Psn);
            lastFlushedPsn_++;
            records[i]->CompletedFlush(STATUS_SUCCESS);
        }
    }

    Awaitable<void> KLogManagerTests::CreateLogManager()
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
        invalidRecords_ = InvalidLogRecords::Create(allocator);

        auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();
        KtlLogger::SharedLogSettingsSPtr sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));

        LR_Test_InitializeKtlConfig(
            allocator,
            sharedLogSettings);
        
        Data::Log::LogManager::Create(
            allocator,
            dataLogManager_);
        
        NTSTATUS status = SyncAwait(dataLogManager_->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
        txrSettings.MaxStreamSizeInMB = 100;
        txrSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;

        TransactionalReplicatorSettingsUPtr tmp;
        TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

        config_ = TRInternalSettings::Create(
            move(tmp),
            make_shared<TransactionalReplicatorConfig>());


        KTLLOGGER_SHARED_LOG_SETTINGS slSettings = { L"", L"", 0, 0, 0 };
        KtlLoggerSharedLogSettingsUPtr tmp2;
        KtlLoggerSharedLogSettings::FromPublicApi(slSettings, tmp2);

        TxnReplicator::SLInternalSettingsSPtr slConfig = SLInternalSettings::Create(
            move(tmp2));

        TxnReplicator::TRPerformanceCountersSPtr counters = TRPerformanceCounters::CreateInstance(
            prId_->TracePartitionId.ToString(),
            prId_->ReplicaId);

        TestHealthClientSPtr healthClient = TestHealthClient::Create();

        TxnReplicator::IRuntimeFolders::SPtr runtimeFolders = Data::TestCommon::TestRuntimeFolders::Create(GetWorkingDirectory().c_str(), allocator);

        logManager_ = KLogManager::Create(
            *prId_,
            *runtimeFolders,
            *dataLogManager_,
            config_,
            slConfig,
            counters,
            healthClient_,
            allocator).RawPtr();

        CODING_ERROR_ASSERT(logManager_ != nullptr);

        KString::SPtr workDirKstr = KString::Create(GetWorkingDirectory().c_str(), allocator);
        CODING_ERROR_ASSERT(workDirKstr != nullptr);

        co_await logManager_->InitializeAsync(*workDirKstr);

        processor_ = FlushProcessor::Create(allocator, this);
        callbackManager_ = PhysicalLogWriterCallbackManager::Create(
            *prId_,
            allocator);
        callbackManager_->FlushCallbackProcessor = *processor_;

        CODING_ERROR_ASSERT(callbackManager_ != nullptr);
    }

    Awaitable<void> KLogManagerTests::ShutdownLogManager()
    {
        co_return;
    }
    
    BOOST_FIXTURE_TEST_SUITE(KLogManagerTestsSuite, KLogManagerTests)

    BOOST_AUTO_TEST_CASE(VerifyEmptyNewLogContents)
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
            SyncAwait(ShutdownLogManager());
        }
    }

    BOOST_AUTO_TEST_CASE(CreateCopyLog_Verify)
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

            KString::CSPtr basealias = LogManager::GenerateLogFileAlias(prId_->PartitionId, prId_->ReplicaId, allocator);

            BOOL result;
            KString::SPtr copyAlias;
            KString::Create(copyAlias, allocator, *basealias);
            copyAlias->Concat(CopySuffix);

            KString::SPtr trace;
            status = KString::Create(trace, allocator, (ULONG)TraceComponent.size());
            result = trace->CopyFromAnsi(
                TraceComponent.begin(),
                (ULONG)TraceComponent.size());

            VERIFY_IS_TRUE(result);

            KLogManager::SPtr ktlManager = down_cast<KLogManager, LogManager>(logManager_);
            Data::Log::IPhysicalLogHandle::SPtr physicalLogHandle = ktlManager->PhysicalLogHandle;

            // Resolve base alias
            KGuid baseLogicalLogId;
            status = SyncAwait(
                        physicalLogHandle->ResolveAliasAsync(
                        *basealias,
                        CancellationToken::None,
                        baseLogicalLogId));

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Resolve copy alias
            KGuid copyLogicalLogId;
            status = SyncAwait(
                physicalLogHandle->ResolveAliasAsync(
                    *copyAlias,
                    CancellationToken::None,
                    copyLogicalLogId));

            Trace.WriteInfo(
                TraceComponent,
                "{0} CreateCopyLog_Verify | ResolveAliasAsync: Base Alias = {1}, Status = {2}, Base Log Id = {3}, Copy Alias = {4}, Copy Log Id = {5}",
                prId_->TraceId,
                ToStringLiteral(*basealias),
                status,
                Common::Guid(baseLogicalLogId),
                ToStringLiteral(*copyAlias),
                Common::Guid(copyLogicalLogId));

            // Retrieve handle to base log
            Data::Log::ILogicalLog::SPtr baseLog;
            status = 
                SyncAwait(
                    physicalLogHandle->OpenLogicalLogAsync(
                        baseLogicalLogId,
                        CancellationToken::None,
                        baseLog));

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            Trace.WriteInfo(
                TraceComponent,
                "{0} CreateCopyLog_Verify | OpenLogicalLogAsync: Status = {1}",
                prId_->TraceId,
                status);

            // Retrieve handle to copy log
            Data::Log::ILogicalLog::SPtr copyLog;
            status =
                SyncAwait(
                    physicalLogHandle->OpenLogicalLogAsync(
                        copyLogicalLogId,
                        CancellationToken::None,
                        copyLog));

            // Since the copy log is open, this name collision is expected
            VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_COLLISION);

            // Close the open copy log
            SyncAwait(logManager_->CloseLogAsync());

            // Try again to retrieve the copy log handle

            status =
                SyncAwait(
                    physicalLogHandle->OpenLogicalLogAsync(
                        copyLogicalLogId,
                        CancellationToken::None,
                        copyLog));

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            Trace.WriteInfo(
                TraceComponent,
                "{0} CreateCopyLog_Verify | OpenLogicalLogAsync: Status = {1}",
                prId_->TraceId,
                status);

            Data::Log::ILogicalLogReadStream::SPtr baseReadStream;
            status = baseLog->CreateReadStream(baseReadStream, 1);

            Data::Log::ILogicalLogReadStream::SPtr copyReadStream;
            status = copyLog->CreateReadStream(copyReadStream, 1);

            Trace.WriteInfo(
                TraceComponent,
                "{0} CreateCopyLog_Verify | CreateReadStream: Status = {1}, Position = {2}, Length = {3}",
                prId_->TraceId,
                status,
                baseReadStream->Position,
                baseReadStream->Length);

            SyncAwait(logManager_->FlushAsync(L"T"));

            baseReadStream->SetPosition(baseReadStream->GetLength());
            copyReadStream->SetPosition(copyReadStream->GetLength());

            // baseLogTailRecord record is expected to be equal to the last record before the copy log was created
            // In this case that is the above 'TailRecord'
            LogRecord::SPtr tmpBaseRecord = SyncAwait(
                LogRecord::ReadPreviousRecordAsync(
                    *baseReadStream,
                    *invalidRecords_,
                    allocator));

            // Assert the last record in the base log is a complete checkpoint record
            VERIFY_IS_TRUE(tmpBaseRecord->RecordType == LogRecordType::CompleteCheckpoint);
            CompleteCheckPointLogRecord::SPtr baseLogTailRecord(dynamic_cast<CompleteCheckPointLogRecord *>(tmpBaseRecord.RawPtr()));

            LogRecord::SPtr tmpCopyRecord = SyncAwait(
                LogRecord::ReadPreviousRecordAsync(
                    *copyReadStream,
                    *invalidRecords_,
                    allocator));

            // Assert the last record in the copy log is an indexing record
            VERIFY_IS_TRUE(tmpCopyRecord->RecordType == LogRecordType::Indexing);
            IndexingLogRecord::SPtr copyLogTailRecord(dynamic_cast<IndexingLogRecord *>(tmpCopyRecord.RawPtr()));

            Trace.WriteInfo(
                TraceComponent,
                "{0} CreateCopyLog_Verify | ReadPreviousRecordAsync: Status = {1}, Type={2}, LSN={3}, PSN={4}, Position={5}",
                prId_->TraceId,
                status,
                baseLogTailRecord->RecordType,
                baseLogTailRecord->Lsn,
                baseLogTailRecord->Psn,
                baseLogTailRecord->RecordPosition);

            bool baseLogTailRecordsAreEqual = baseLogTailRecord->Test_Equals(dynamic_cast<LogRecord &>(*tailRecord));
            VERIFY_IS_TRUE(baseLogTailRecordsAreEqual);

            bool copyLogTailRecordsAreEqual = copyLogTailRecord->Test_Equals(dynamic_cast<LogRecord &>(*copyLogHead));
            VERIFY_IS_TRUE(copyLogTailRecordsAreEqual);

            // Cleanup resources used in this test
            SyncAwait(copyReadStream->CloseAsync());
            SyncAwait(baseReadStream->CloseAsync());
            SyncAwait(copyLog->CloseAsync());
            SyncAwait(baseLog->CloseAsync());
            SyncAwait(ShutdownLogManager());
        }
    }

    BOOST_AUTO_TEST_CASE(RenameCopyLog_Verify)
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

            KString::CSPtr basealias = LogManager::GenerateLogFileAlias(prId_->PartitionId, prId_->ReplicaId, allocator);
            SyncAwait(logManager_->RenameCopyLogAtomicallyAsync());

            VERIFY_IS_NOT_NULL(logManager_->CurrentLogTailRecord);
            VERIFY_ARE_EQUAL(logManager_->CurrentLogTailRecord->Lsn, 5000);

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
            SyncAwait(ShutdownLogManager());
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
