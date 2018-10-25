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
    using namespace Data::Log;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;

    #define PLW_TEST_TAG 'WlPT'

    StringLiteral const TraceComponent = "PhysicalLogWriterTest";
    NTSTATUS const PLW_TEST_EXCEPTION = STATUS_INSUFFICIENT_RESOURCES;

    class PhysicalLogWriterTests
    {
    protected:

        PhysicalLogWriterTests() 
            : maxWaitDurationInMs_(10000)
            , lastFlushedPsn_(0)
            , assertInFlushCallback_(true)
        {
        }

        void EndTest();

        Awaitable<void> CreatePLWAsync(
            PartitionedReplicaId const & traceId,
            wstring const & fileName);

        Awaitable<void> CreateAndFlushLogHead();

        Awaitable<void> InsertAndFlush(
            LogRecord & record,
            KStringView const & flusher);

        Awaitable<LogRecord::SPtr> CreateLogRecordsAsync(
            ULONG count,
            KStringView const flusher);

        LogRecord::SPtr InsertLogRecords(__in ULONG recordCount, __out ULONG & expectedBufferSize, __out LONG64 & currentBufferSize);

        void WaitForRecordFlushToPSN(__in LONG64 targetPsn);
        void WaitForRecordFlush(__in bool waitForCompleteFlush = true, __in LONG64 tailPsn = -1);

        Awaitable<LogRecord::SPtr> GetCurrentLogHeadRecord(__in ULONG64 recordPosition = 0);

        const LONG64 maxWaitDurationInMs_;

        bool assertInFlushCallback_;
        IndexingLogRecord::SPtr logHead_;
        InvalidLogRecords::SPtr invalidRecords_;
        FaultyFileLog::SPtr fileLog_;
        PhysicalLogWriterCallbackManager::SPtr callbackManager_;
        LONG64 lastFlushedPsn_;
        PhysicalLogWriter::SPtr writer_;
        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
        KSharedArray<NTSTATUS>::SPtr flushCallbackExceptionArray_;

    private:
        LogRecord::SPtr CreateRandomLogRecord();
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
            static FlushProcessor::SPtr Create(__in KAllocator & allocator, PhysicalLogWriterTests * parent)
            {
                return _new(PLW_TEST_TAG, allocator)FlushProcessor(parent);
            }

            void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override
            {
                parent_->FlushCallback(loggedRecords);
            }

        private:
            FlushProcessor(PhysicalLogWriterTests * parent)
                : parent_(parent)
            {
            }

            PhysicalLogWriterTests * parent_;
        };

        FlushProcessor::SPtr flushProcessor_;
    };

    PhysicalLogWriterTests::FlushProcessor::~FlushProcessor()
    {
    }

    void PhysicalLogWriterTests::EndTest()
    {
        ErrorCode er = Common::Directory::Delete_WithRetry(GetWorkingDirectory(), true, true);
        ASSERT_IFNOT(er.IsSuccess(), "Failed to delete working directory");

        prId_.Reset();
        writer_->Dispose();
        writer_.Reset();
        callbackManager_.Reset();
        fileLog_.Reset();
        invalidRecords_.Reset();
        logHead_.Reset();
        flushCallbackExceptionArray_.Reset();
        flushProcessor_.Reset();
    }

    void PhysicalLogWriterTests::FlushCallback(LoggedRecords const & records)
    {
        if (!NT_SUCCESS(records.LogError))
        {
            flushCallbackExceptionArray_->Append(records.LogError);
        }

        for (ULONG i = 0; i < records.Count; i++)
        {
            if (prId_ == nullptr) 
            {
                return;
            }

            if (assertInFlushCallback_)
            {
                Trace.WriteInfo(TraceComponent, "{0} Flushed Psn: {1}, lastFlushedPsn_={2}", prId_->TraceId, records[i]->Psn, lastFlushedPsn_);
                VERIFY_ARE_EQUAL(lastFlushedPsn_ + 1, records[i]->Psn);
            }
            else 
            {
                Trace.WriteInfo(TraceComponent, "{0} Flushed Psn: {1} [Assert disabled]", prId_->TraceId, records[i]->Psn);
            }

            lastFlushedPsn_++;
        }
    }

    Awaitable<void> PhysicalLogWriterTests::CreateAndFlushLogHead()
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
        logHead_ = IndexingLogRecord::Create(
            Epoch(1, 1),
            1,
            nullptr,
            *invalidRecords_->Inv_PhysicalLogRecord,
            allocator);

        CODING_ERROR_ASSERT(logHead_ != nullptr);

        co_await InsertAndFlush(*logHead_, L"CreateAndFlushLogHead");
        co_return;
    }

    Awaitable<void> PhysicalLogWriterTests::InsertAndFlush(
        LogRecord & record,
        KStringView const & flusher)
    {
        writer_->InsertBufferedRecord(record);
        co_await writer_->FlushAsync(flusher);
        co_return;
    }

    Awaitable<LogRecord::SPtr> PhysicalLogWriterTests::CreateLogRecordsAsync(
        ULONG count,
        KStringView const flusher)
    {
        LogRecord::SPtr tailRecord = nullptr;
        ULONG flushInterval = (rand() % count) + 1;

        Trace.WriteInfo(TraceComponent, "{0} CreateLogRecordsAsync: Flush Interval: {1}", prId_->TraceId, flushInterval);

        for (ULONG i = 0; i < count; i++)
        {
            tailRecord = CreateRandomLogRecord();
            writer_->InsertBufferedRecord(*tailRecord);

            if (i == flushInterval)
            {
                co_await writer_->FlushAsync(flusher);
            }
        }

        co_await writer_->FlushAsync(flusher);

        co_return tailRecord;
    }

    LogRecord::SPtr PhysicalLogWriterTests::InsertLogRecords(
        __in ULONG recordCount,
        __out ULONG & expectedBufferSize,
        __out LONG64 & currentBufferSize)
    {
        expectedBufferSize = 0;
        currentBufferSize = 0;

        LogRecord::SPtr recordToFlush = nullptr;

        for (ULONG i = 0; i < recordCount; i++)
        {
            recordToFlush = CreateRandomLogRecord();

            // Insert the record into the buffer
            currentBufferSize = writer_->InsertBufferedRecord(*recordToFlush);

            // Update the expected buffer size
            expectedBufferSize += (recordToFlush->ApproximateSizeOnDisk);

            // Verify the current size is equal to the expected value
            VERIFY_ARE_EQUAL(expectedBufferSize, currentBufferSize);

            // Verify the current buffer size is equal to the exposed property
            VERIFY_ARE_EQUAL(currentBufferSize, writer_->BufferedRecordsBytes);
        }

        return recordToFlush;
    }

    LogRecord::SPtr PhysicalLogWriterTests::CreateRandomLogRecord()
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
 
        InformationLogRecord::SPtr infoRecord = InformationLogRecord::Create(
            rand(),
            nullptr,
            *invalidRecords_->Inv_PhysicalLogRecord,
            InformationEvent::Enum::Recovered,
            allocator);

        return infoRecord.RawPtr();
    }

    Awaitable<void> PhysicalLogWriterTests::CreatePLWAsync(
        PartitionedReplicaId const & traceId,
        std::wstring const & fileName)
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
        NTSTATUS status;

        // Create the data structure to monitor logged exceptions
        flushCallbackExceptionArray_ = _new(PLW_TEST_TAG, allocator)KSharedArray<NTSTATUS>();

        auto cwd = GetWorkingDirectory();

        if (!Common::Directory::Exists(cwd))
        {
            Common::Directory::Create(cwd);
        }

        auto filePath = cwd + Path::GetPathSeparatorWstr() + L"PLW_" + fileName + L".log";
        File::Delete(filePath, false);

        auto path = KPath::CreatePath(filePath.c_str(), allocator);
        KWString kstring = KWString(allocator, static_cast<LPCWSTR>(*path));

        status = FaultyFileLog::Create(traceId, allocator, fileLog_);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        status = co_await fileLog_->OpenAsync(kstring);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        flushProcessor_ = FlushProcessor::Create(allocator, this);
        callbackManager_ = PhysicalLogWriterCallbackManager::Create(
            traceId,
            allocator);

        CODING_ERROR_ASSERT(callbackManager_ != nullptr);
        callbackManager_->FlushCallbackProcessor = *flushProcessor_;

        invalidRecords_ = InvalidLogRecords::Create(allocator);

        CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

        TxnReplicator::TRPerformanceCountersSPtr counters = TRPerformanceCounters::CreateInstance(
            prId_->TracePartitionId.ToString(),
            prId_->ReplicaId);

        TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
        TransactionalReplicatorSettingsUPtr tmp;
        TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

        TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
            move(tmp),
            make_shared<TransactionalReplicatorConfig>());

        TestHealthClientSPtr healthClient = TestHealthClient::Create();

        writer_ = PhysicalLogWriter::Create(
            traceId,
            *fileLog_,
            *callbackManager_,
            300,
            false,
            *invalidRecords_->Inv_LogRecord,
            counters,
            healthClient,
            config,
            allocator);

        CODING_ERROR_ASSERT(writer_ != nullptr);

        co_return;
    }

    void PhysicalLogWriterTests::WaitForRecordFlushToPSN(__in LONG64 targetPsn) 
    {
        WaitForRecordFlush(false, targetPsn);
    }

    void PhysicalLogWriterTests::WaitForRecordFlush(__in bool waitForCompleteFlush, __in LONG64 targetPsn)
    {
        int count = 0;
        int const sleepDuration = 10;
        bool targetPsnExists = targetPsn > 0;

        while(true)
        {
            Trace.WriteInfo(
                TraceComponent,
                "{0} WaitForAllRecordstoFlush: Count = {1}",
                prId_->TraceId,
                count);

            count++;

            if (targetPsnExists) 
            {
                if (lastFlushedPsn_ >= targetPsn && !waitForCompleteFlush)
                {
                    return;
                }
                else if (lastFlushedPsn_ >= targetPsn && waitForCompleteFlush && writer_->IsCompletelyFlushed)
                {
                    return;
                }
            }
            else 
            {
                if (writer_->IsCompletelyFlushed)
                {
                    return;
                }
            }
                
            Sleep(sleepDuration);
            VERIFY_IS_TRUE(count * sleepDuration < maxWaitDurationInMs_);
        }
    }

    Awaitable<LogRecord::SPtr> PhysicalLogWriterTests::GetCurrentLogHeadRecord(__in ULONG64 recordPosition)
    {
        ILogicalLogReadStream::SPtr rs;
        NTSTATUS status = fileLog_->CreateReadStream(rs, 1);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
        
        rs->SetPosition(recordPosition);
        LogRecord::SPtr currentHead = nullptr;

        currentHead = co_await LogRecord::ReadNextRecordAsync(
            *rs,
            *invalidRecords_,
            underlyingSystem_->NonPagedAllocator());

        status = co_await rs->CloseAsync();
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        co_return currentHead;
    }

    BOOST_FIXTURE_TEST_SUITE(PhysicalLogWriterTestsSuite, PhysicalLogWriterTests)

    BOOST_AUTO_TEST_CASE(OpenClose)
    {
        TEST_TRACE_BEGIN("OpenClose")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"OpenClose"));
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(OneLogRecord)
    {
        TEST_TRACE_BEGIN("OneLogRecord")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"OneLogRecord"));
            SyncAwait(this->CreateAndFlushLogHead());

            auto tailRecord = SyncAwait(CreateLogRecordsAsync(1, L"OneLogRecord"));

            VERIFY_ARE_EQUAL(writer_->CurrentLogTailRecord->Psn, tailRecord->Psn);

            SyncAwait(fileLog_->CloseAsync());

            WaitForRecordFlushToPSN(tailRecord->Psn);
        }
    }

    BOOST_AUTO_TEST_CASE(HundredEntries)
    {
        TEST_TRACE_BEGIN("HundredEntries")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"HundredEntries"));
            SyncAwait(this->CreateAndFlushLogHead());

            auto tailRecord = SyncAwait(CreateLogRecordsAsync(100, L"HundredEntries"));

            VERIFY_ARE_EQUAL(writer_->CurrentLogTailRecord->Psn, tailRecord->Psn);

            SyncAwait(fileLog_->CloseAsync());
            WaitForRecordFlushToPSN(tailRecord->Psn);
        }
    }

    BOOST_AUTO_TEST_CASE(MultiThreaded)
    {
        TEST_TRACE_BEGIN("MultiThreaded")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"MultiThreaded"));
            SyncAwait(this->CreateAndFlushLogHead());
            
            KArray<Awaitable<LogRecord::SPtr>> tasks(allocator);
            status = STATUS_SUCCESS;
             
            for (ULONG i = 0; i < 100; i++)
            {
                Awaitable<LogRecord::SPtr> task = CreateLogRecordsAsync(10, L"MultiThreaded");
                status = tasks.Append(Ktl::Move(task));
                CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            }

            for (ULONG i = 0; i < tasks.Count(); i++)
            {
                SyncAwait(tasks[i]);
            }

            auto tailRecord = SyncAwait(CreateLogRecordsAsync(1, L"MultiThreadedLast"));

            VERIFY_ARE_EQUAL(writer_->CurrentLogTailRecord->Psn, tailRecord->Psn);
            SyncAwait(fileLog_->CloseAsync());
            WaitForRecordFlushToPSN(tailRecord->Psn);
        }
    }

    BOOST_AUTO_TEST_CASE(SetTailRecord_LogicalRecord)
    {
        TEST_TRACE_BEGIN("SetTailRecord_LogicalRecord")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"SetTailRecord_LogicalRecord"));

            // Tail after inserting 20 log records
            LogRecord::SPtr initialTailRecord = SyncAwait(CreateLogRecordsAsync(20, L"SetTailRecord_LogicalRecord"));

            // Create a new logical record
            UpdateEpochLogRecord::SPtr newTailOperationLogRecord = UpdateEpochLogRecord::Create(
                Epoch(1, 1),
                1,
                *invalidRecords_->Inv_PhysicalLogRecord,
                underlyingSystem_->NonPagedAllocator());

            newTailOperationLogRecord->Lsn = rand();
            LogRecord::SPtr newTailLogRecord = newTailOperationLogRecord.RawPtr();

            // Insert the new log tail into the buffer and flush
            SyncAwait(InsertAndFlush(*newTailLogRecord, L"SetTailRecord_LogicalRecord"));

            // Set logical record as tail
            writer_->SetTailRecord(*newTailLogRecord);

            // Confirm the log tail record was updated
            VERIFY_IS_TRUE(writer_->CurrentLogTailRecord->LogRecord::Test_Equals(*newTailLogRecord));

            // Confirm the new log tail record is not set as the current last physical record
            LogRecord::SPtr currentLastPhysicalRecord = writer_->CurrentLastPhysicalRecord.RawPtr();
            VERIFY_IS_FALSE(currentLastPhysicalRecord->LogRecord::Test_Equals(*newTailLogRecord));

            // Confirm the last physical record is unchanged after the logical record insertion
            VERIFY_IS_TRUE(initialTailRecord->LogRecord::Test_Equals(*currentLastPhysicalRecord));

            WaitForRecordFlushToPSN(currentLastPhysicalRecord->Psn);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(SetTailRecord_PhysicalRecord)
    {
        TEST_TRACE_BEGIN("SetTailRecord_PhysicalRecord")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"SetTailRecord_PhysicalRecord"));

            // Tail after inserting 20 log records
            LogRecord::SPtr initialTailRecord = SyncAwait(CreateLogRecordsAsync(20, L"SetTailRecord_PhysicalRecord"));

            // Create a new physical record
            IndexingLogRecord::SPtr newTailOperationLogRecord = IndexingLogRecord::Create(
                Epoch(2, 3),
                rand(),
                nullptr,
                *invalidRecords_->Inv_PhysicalLogRecord,
                allocator);

            LogRecord::SPtr newTailLogRecord = newTailOperationLogRecord.RawPtr();

            // Insert the new log tail into the buffer and flush
            SyncAwait(InsertAndFlush(*newTailLogRecord, L"SetTailRecord_PhysicalRecord"));

            // Set physical record as tail
            writer_->SetTailRecord(*newTailLogRecord);

            // Confirm the log tail record was updated
            VERIFY_IS_TRUE(writer_->CurrentLogTailRecord->LogRecord::Test_Equals(*newTailLogRecord));

            // Confirm the new log tail record is equal to the current last physical record
            LogRecord::SPtr currentLastPhysicalRecord = writer_->CurrentLastPhysicalRecord.RawPtr();
            VERIFY_IS_TRUE(currentLastPhysicalRecord->LogRecord::Test_Equals(*newTailLogRecord));

            // Confirm the new log tail record is equal to the inserted physical record
            VERIFY_IS_TRUE(newTailLogRecord->LogRecord::Test_Equals(*currentLastPhysicalRecord));

            WaitForRecordFlushToPSN(currentLastPhysicalRecord->Psn);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailRecord)
    {
        TEST_TRACE_BEGIN("TruncateTailRecord")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"TruncateTailRecord"));

            // Insert 20 log records
            SyncAwait(CreateLogRecordsAsync(20, L"TruncateTailRecord"));

            // Create a new physical record
            IndexingLogRecord::SPtr targetTailOperationLogRecord = IndexingLogRecord::Create(
                Epoch(2, 3),
                rand(),
                nullptr,
                *invalidRecords_->Inv_PhysicalLogRecord,
                allocator);

            LogRecord::SPtr targetTailLogRecord = targetTailOperationLogRecord.RawPtr();

            // Insert the new log tail into the buffer and flush
            SyncAwait(InsertAndFlush(*targetTailLogRecord, L"TruncateTailRecord"));

            // Insert 20 additional log records
            LogRecord::SPtr lastFlushedRecord = SyncAwait(CreateLogRecordsAsync(20, L"TruncateTailRecord"));

            // Wait for all records thus far to be flushed
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Verify the target is not currently the tail record
            VERIFY_IS_FALSE(targetTailLogRecord->LogRecord::Test_Equals(*writer_->CurrentLogTailRecord));

            // Truncate the tail to the target record
            SyncAwait(writer_->TruncateLogTail(*targetTailLogRecord));

            // Verify the target is equal to the current tail record
            VERIFY_IS_TRUE(targetTailLogRecord->LogRecord::Test_Equals(*writer_->CurrentLogTailRecord));

            WaitForRecordFlushToPSN(targetTailLogRecord->Psn);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadRecord)
    {
        TEST_TRACE_BEGIN("TruncateHeadRecord")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"TruncateHeadRecord"));

            // Insert 20 log records
            SyncAwait(CreateLogRecordsAsync(20, L"TruncateHeadRecord"));

            // Create a new physical record
            IndexingLogRecord::SPtr targetHeadIndexingLogRecord = IndexingLogRecord::Create(
                Epoch(2, 3),
                rand(),
                nullptr,
                *invalidRecords_->Inv_PhysicalLogRecord,
                allocator);

            LogRecord::SPtr targetHeadLogRecord = targetHeadIndexingLogRecord.RawPtr();

            // Insert the new record into the buffer and flush
            SyncAwait(InsertAndFlush(*targetHeadLogRecord, L"TruncateHeadLogRecord"));

            // Retrieve the current log head record
            LogRecord::SPtr currentHeadRecord = SyncAwait(GetCurrentLogHeadRecord(0));

            // Verify the current log head is not equal to the target
            VERIFY_IS_FALSE(currentHeadRecord->LogRecord::Test_Equals(*targetHeadLogRecord));

            // Insert 20 log records
            LogRecord::SPtr lastFlushedRecord = SyncAwait(CreateLogRecordsAsync(20, L"TruncateHeadLogRecord"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Truncate the log head
            SyncAwait(writer_->TruncateLogHeadAsync(targetHeadIndexingLogRecord->RecordPosition));
            
            currentHeadRecord = SyncAwait(GetCurrentLogHeadRecord(targetHeadIndexingLogRecord->RecordPosition));

            // Verify the current log head is equal to the target
            VERIFY_IS_TRUE(currentHeadRecord->LogRecord::Test_Equals(*targetHeadLogRecord));

            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBufferedRecordBytes)
    {
        TEST_TRACE_BEGIN("VerifyBufferedRecordBytes")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyBufferedRecordBytes"));

            ULONG recordCount = 30;

            // Total buffer size as calculated in the test
            ULONG expectedBufferSize = 0;

            // Total buffer size returned by PhysicalLogWriter
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush the buffer
            SyncAwait(writer_->FlushAsync(L"VerifyBufferedRecordBytes"));

            // Verify the buffer has been flushed and no records are pending
            VERIFY_ARE_EQUAL(writer_->BufferedRecordsBytes, 0);
            VERIFY_ARE_EQUAL(writer_->PendingFlushRecordsBytes, 0);

            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBufferedAndPendingRecordBytes)
    {
        TEST_TRACE_BEGIN("VerifyBufferedAndPendingRecordBytes")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyBufferedAndPendingRecordBytes"));

            ULONG recordCount = 30;

            // Total buffer size as calculated in the test
            ULONG expectedBufferSize = 0;

            // Total buffer size returned by PhysicalLogWriter
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush the buffer
            SyncAwait(writer_->FlushAsync(L"VerifyBufferedAndPendingRecordBytes"));

            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);
            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();

            // Verify the buffer has been flushed and no records are pending
            VERIFY_ARE_EQUAL(writer_->BufferedRecordsBytes, 0);
            VERIFY_ARE_EQUAL(writer_->PendingFlushRecordsBytes, 0);

            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBufferedAndPendingRecordBytes_MultipleFlushes)
    {
        TEST_TRACE_BEGIN("VerifyBufferedAndPendingRecordBytes_MultipleFlushes")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyBufferedAndPendingRecordBytes_MultipleFlushes"));

            ULONG recordCount = 30;

            // Total buffer size as calculated in the test
            ULONG expectedBufferSize = 0;

            // Total buffer size returned by PhysicalLogWriter
            LONG64 currentBufferSize = 0;

            // Add records to the buffer
            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);
            
            // Flush the buffer in the background
            Awaitable<void> flushTask = writer_->FlushAsync(L"VerifyBufferedAndPendingRecordBytes_MultipleFlushes");

            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Insert records after the flush has started, but before completion
            lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Verify the buffer currently contains the second batch of inserted records
            VERIFY_ARE_EQUAL(writer_->BufferedRecordsBytes, currentBufferSize);
            SyncAwait(flushTask);

            // Flush the buffer in the background
            flushTask = writer_->FlushAsync(L"VerifyBufferedAndPendingRecordBytes_MultipleFlushes");

            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);
            WaitForRecordFlush();

            // Verify the buffer has been flushed and no records are pending
            VERIFY_ARE_EQUAL(writer_->BufferedRecordsBytes, 0);
            VERIFY_ARE_EQUAL(writer_->PendingFlushRecordsBytes, 0);

            SyncAwait(flushTask);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyFlushCompletionTask)
    {
        TEST_TRACE_BEGIN("VerifyFlushCompletionTask")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyFlushCompletionTask"));

            ULONG recordCount = 30;

            // Total buffer size as calculated in the test
            ULONG expectedBufferSize = 0;

            // Total buffer size returned by PhysicalLogWriter
            LONG64 currentBufferSize = 0;

            // Add records to the buffer
            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush the buffer in the background
            Awaitable<void> flushAwaitable = writer_->FlushAsync(L"VerifyFlushCompletionTask");

            // Await on the flush completion task used to notify components issuing group commit
            AwaitableCompletionSource<void>::SPtr flushCompletionTask = writer_->GetFlushCompletionTask();
            Awaitable<void> flushTcsAwaitable = flushCompletionTask->GetAwaitable();

            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);
            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();

            // Verify the PhysicalLogWriter is completely flushed
            VERIFY_IS_TRUE(writer_->IsCompletelyFlushed);

            SyncAwait(flushAwaitable);
            SyncAwait(flushTcsAwaitable);
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyClosedException)
    {
        TEST_TRACE_BEGIN("VerifyClosedException")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyFlushException_AppendAsync"));

            // Insert 5 log records
            ULONG recordCount = 5;
            ULONG expectedBufferSize = 0;
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush the buffer
            SyncAwait(writer_->FlushAsync(L"VerifyFlushException_AppendAsync"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Prepare to close the PhysicalLogWriter and confirm the ClosedError has been set
            writer_->PrepareToClose();
            CODING_ERROR_ASSERT(writer_->ClosedError == SF_STATUS_OBJECT_CLOSED);

            // Create a record to insert into the buffer after the ClosedError has been set
            InformationLogRecord::SPtr infoRecordToInsertAfterClose = InformationLogRecord::Create(
                rand(),
                logHead_.RawPtr(),
                *invalidRecords_->Inv_PhysicalLogRecord,
                InformationEvent::Enum::Recovered,
                allocator);

            // Disable callback asserts as the records in the callback will not have been assigned PSNs
            assertInFlushCallback_ = false;
            LogRecord::SPtr logRecordToInsertAfterClose = infoRecordToInsertAfterClose.RawPtr();

            // Insert the record
            writer_->InsertBufferedRecord(*logRecordToInsertAfterClose);

            // Wait for logRecordToInsertAfterClose to be flushed 
            // Use the Psn of the last successfully flushed record + 1 as the last inserted record will NOT have a PSN
            // The below is awaiting for the next callback from the PhysicalLogWriterCallbackManager, regardless of PSN
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn + 1);

            // Confirm an exception occurred when inserting a record after close
            CODING_ERROR_ASSERT(flushCallbackExceptionArray_->Count() == 1);

            // Confirm the exception is an expected type
            NTSTATUS ex = (*flushCallbackExceptionArray_)[0];
            CODING_ERROR_ASSERT(ex == SF_STATUS_OBJECT_CLOSED);

            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyLoggingException_AppendAsync_SingleFlush)
    {
        TEST_TRACE_BEGIN("VerifyLoggingException_AppendAsync_SingleFlush")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyLoggingException_AppendAsync_SingleFlush"));

            // Insert 10 log records
            ULONG recordCount = 10;
            ULONG expectedBufferSize = 0;
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush the buffer
            SyncAwait(writer_->FlushAsync(L"VerifyLoggingException_AppendAsync_SingleFlush"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Confirm no exception occurred during the first flush
            CODING_ERROR_ASSERT(flushCallbackExceptionArray_->Count() == 0);

            // Configure exception throw in AppendAsync during flush
            fileLog_->ThrowExceptionInAppendAsync = true;

            // Insert 10 more log records
            lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);
            
            // Flush the buffer one time with exceptions expected
            SyncAwait(writer_->FlushAsync(L"VerifyLoggingException_AppendAsync_SingleFlush"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Confirm an exception occurred during flush
            CODING_ERROR_ASSERT(flushCallbackExceptionArray_->Count() == 1);

            // Verify the exception type is expected
            NTSTATUS ex = (*flushCallbackExceptionArray_)[0];
            CODING_ERROR_ASSERT(ex == PLW_TEST_EXCEPTION);

            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyLoggingException_AppendAsync_MultipleFlushes)
    {
        TEST_TRACE_BEGIN("VerifyLoggingException_AppendAsync_MultipleFlushes")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyLoggingException_AppendAsync_MultipleFlushes"));

            // Insert 10 log records to form batch 1
            ULONG recordCount = 10;
            ULONG expectedBufferSize = 0;
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Delay the AppendAsync callback by 5ms for each record in the next flush
            fileLog_->AppendAsyncDelayInMs = 5;

            // Flush batch 1 in the background
            Awaitable<void> flushTask = writer_->FlushAsync(L"VerifyLoggingException_AppendAsync_MultipleFlushes");
            ToTask(flushTask);

            // Allow exception throw in AppendAsync during flush
            fileLog_->ThrowExceptionInAppendAsync = true;

            // Insert 10 more log records to form batch 2
            lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush batch 2 
            SyncAwait(writer_->FlushAsync(L"VerifyLoggingException_AppendAsync_MultipleFlushes"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Confirm an exception was thrown during each batch flush
            CODING_ERROR_ASSERT(flushCallbackExceptionArray_->Count() == 2);

            // Verify the exception type is expected
            for(ULONG i = 0; i < flushCallbackExceptionArray_->Count(); i++)
            {
                NTSTATUS ex = (*flushCallbackExceptionArray_)[i];
                CODING_ERROR_ASSERT(ex == PLW_TEST_EXCEPTION);
            }

            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyLoggingException_FlushWithMarkerAsync_MultipleFlushes)
    {
        TEST_TRACE_BEGIN("VerifyLoggingException_FlushWithMarkerAsync_MultipleFlushes")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyLoggingException_FlushWithMarkerAsync_MultipleFlushes"));

            // Insert 10 log records to form batch 1
            ULONG recordCount = 10;
            ULONG expectedBufferSize = 0;
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Delay the FlushWithMarkerAsyncDelay callback by 30ms
            fileLog_->FlushWithMarkerAsyncDelayInMs = 30;
            fileLog_->ThrowExceptionInFlushWithMarkerAsync = true;

            // Flush batch 1 in the background
            Awaitable<void> flushTask = writer_->FlushAsync(L"VerifyLoggingException_FlushWithMarkerAsync_MultipleFlushes");
            ToTask(flushTask);

            // Insert 10 more log records to form batch 2
            lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Flush batch 2 
            SyncAwait(writer_->FlushAsync(L"VerifyLoggingException_FlushWithMarkerAsync_MultipleFlushes"));
            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);

            // Confirm an exception was thrown during each batch flush
            CODING_ERROR_ASSERT(flushCallbackExceptionArray_->Count() == 2);

            // Verify the exception type is expected
            for (ULONG i = 0; i < flushCallbackExceptionArray_->Count(); i++)
            {
                NTSTATUS ex = (*flushCallbackExceptionArray_)[i];
                CODING_ERROR_ASSERT(ex == PLW_TEST_EXCEPTION);
            }

            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyShouldThrottleWrites)
    {
        TEST_TRACE_BEGIN("VerifyShouldThrottleWrites")
        {
            SyncAwait(this->CreatePLWAsync(*prId_, L"VerifyShouldThrottleWrites"));

            // Insert 10 log records to form batch 1
            // Total size of 10 records is 560 bytes
            // The PLW max write cache size in bytes is 300
            ULONG recordCount = 10;
            ULONG expectedBufferSize = 0;
            LONG64 currentBufferSize = 0;

            LogRecord::SPtr lastFlushedRecord = InsertLogRecords(recordCount, expectedBufferSize, currentBufferSize);

            // Delay the FlushWithMarkerAsyncDelay callback by 5 seconds
            fileLog_->FlushWithMarkerAsyncDelayInMs = 5000;
            fileLog_->ThrowExceptionInFlushWithMarkerAsync = true;

            // Initiate flush
            Awaitable<void> flushTask = writer_->FlushAsync(L"VerifyShouldThrottleWrites");

            // Confirm that ShouldThrottleWrites == true while FlushWithMarkerAsync is delayed
            VERIFY_IS_TRUE(writer_->ShouldThrottleWrites);

            // Await flush completion
            SyncAwait(flushTask);

            // Confirm throttling is no longer needed
            VERIFY_IS_FALSE(writer_->ShouldThrottleWrites);

            WaitForRecordFlushToPSN(lastFlushedRecord->Psn);
            // Wait for complete flush of PhysicalLogWriter
            WaitForRecordFlush();
            SyncAwait(fileLog_->CloseAsync());
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
