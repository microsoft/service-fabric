// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTOPERATIONPROCESSOR_TAG 'PpOT'

TestOperationProcessor::TestOperationProcessor(
    __in PartitionedReplicaId const & traceId,
    __in int minDelay, 
    __in int maxDelay, 
    __in int seed,
    __in TRInternalSettingsSPtr const & config,
    __in bool useParallelDispatcher,
    __in KAllocator & allocator)
    : IOperationProcessor()
    , KObject()
    , KShared()
    , KWeakRefType<TestOperationProcessor>()
    , recordsDispatcher_()
    , groupCommits_(allocator)
    , barrierCount_(0)
    , immediateCalledCount_(0)
    , normalCalledCount_(0)
    , processingRecordsCount_(0)
    , processedRecordsCount_(0)
    , updateDispatchedBarrierTaskCount_(0)
    , random_(seed)
    , minDelay_(minDelay)
    , maxDelay_(maxDelay)
{
    if (!useParallelDispatcher)
    {
        if (Common::DateTime::Now().Ticks % 2 == 0)
        {
            recordsDispatcher_ = SerialLogRecordsDispatcher::Create(traceId, *this, config, allocator);
        }
        else
        {
            recordsDispatcher_ = ParallelLogRecordsDispatcher::Create(traceId, *this, config, allocator);
        }
    }
    else
    {
        recordsDispatcher_ = ParallelLogRecordsDispatcher::Create(traceId, *this, config, allocator);
    }

    groupCommits_.Append(TestGroupCommitValidationResult());

    CODING_ERROR_ASSERT(minDelay_ <= maxDelay_);
}

TestOperationProcessor::~TestOperationProcessor()
{
}

TestOperationProcessor::SPtr TestOperationProcessor::Create(
    __in PartitionedReplicaId const & traceId,
    __in int minDelay, 
    __in int maxDelay, 
    __in int seed,
    __in TxnReplicator::TRInternalSettingsSPtr const & config,
    __in KAllocator & allocator)
{
    TestOperationProcessor * testOperationProcessor = _new(TESTOPERATIONPROCESSOR_TAG, allocator)TestOperationProcessor(traceId, minDelay, maxDelay, seed, config, false, allocator);
    CODING_ERROR_ASSERT(testOperationProcessor != nullptr);

    return TestOperationProcessor::SPtr(testOperationProcessor);
}

TestOperationProcessor::SPtr TestOperationProcessor::CreateWithParallelDispatcher(
    __in PartitionedReplicaId const & traceId,
    __in int minDelay, 
    __in int maxDelay, 
    __in int seed,
    __in TxnReplicator::TRInternalSettingsSPtr const & config,
    __in KAllocator & allocator)
{
    TestOperationProcessor * testOperationProcessor = _new(TESTOPERATIONPROCESSOR_TAG, allocator)TestOperationProcessor(traceId, minDelay, maxDelay, seed, config, true, allocator);
    CODING_ERROR_ASSERT(testOperationProcessor != nullptr);

    return TestOperationProcessor::SPtr(testOperationProcessor);
}


void TestOperationProcessor::Close()
{
    // Release the shared pointer to break the cycle
    recordsDispatcher_.Reset();
}

bool TestOperationProcessor::WaitForProcessingToComplete(
    __in UINT targetProcessedRecord,
    __in Common::TimeSpan const & timeout)
{
    Common::Stopwatch stopWatch;
    stopWatch.Start();

    do
    {
        Sleep(100);

        if (processedRecordsCount_ >= targetProcessedRecord && processingRecordsCount_ == 0)
        {
            return true;
        }

        if (stopWatch.Elapsed > timeout)
        {
            return false;
        }
    } while (true);
}

bool TestOperationProcessor::WaitForBarrierProcessingToComplete(
    __in UINT expectedBarrierCount, 
    __in Common::TimeSpan const & timeout)
{
    Common::Stopwatch stopWatch;
    stopWatch.Start();

    do
    {
        Sleep(100);

        if (processingRecordsCount_ == 0 && barrierCount_ == expectedBarrierCount)
        {
            return true;
        }

        if (stopWatch.Elapsed > timeout)
        {
            return false;
        }
    } while (true);
}

void TestOperationProcessor::Unlock(__in LogicalLogRecord & record)
{
    record.CompletedProcessing();
}

RecordProcessingMode::Enum TestOperationProcessor::IdentifyProcessingModeForRecord(__in LogRecord const & record)
{
    RecordProcessingMode::Enum processingMode = RecordProcessingMode::Enum::Normal;

    switch (record.RecordType)
    {
    case LogRecordType::Enum::EndCheckpoint:
    case LogRecordType::Enum::TruncateTail:
    case LogRecordType::Enum::Indexing:
    case LogRecordType::Enum::UpdateEpoch:
    case LogRecordType::Enum::Information:
        processingMode = RecordProcessingMode::Enum::ProcessImmediately;
        break;

    case LogRecordType::Enum::Barrier:
    case LogRecordType::Enum::TruncateHead:
    case LogRecordType::Enum::BeginCheckpoint:
        processingMode = RecordProcessingMode::Enum::ApplyImmediately;
        break;

    case LogRecordType::Enum::BeginTransaction:
    case LogRecordType::Enum::Operation:
    case LogRecordType::Enum::EndTransaction:
        processingMode = RecordProcessingMode::Enum::Normal;
        break;

    default:
        CODING_ERROR_ASSERT(false);
        break;
    }

    return processingMode;
}

Awaitable<void> TestOperationProcessor::ImmediatelyProcessRecordAsync(
    __in LogRecord & logRecord,
    __in NTSTATUS flushErrorCode,
    __in RecordProcessingMode::Enum processingMode)
{
    if (minDelay_ != 0 && maxDelay_ != 0)
    {
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TESTOPERATIONPROCESSOR_TAG, random_.Next(minDelay_, maxDelay_), nullptr);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
    }

    InterlockedIncrement((volatile LONG*)&immediateCalledCount_);

    if (logRecord.RecordType == LogRecordType::Enum::Barrier)
    {
        groupCommits_[groupCommits_.Count() - 1].Lsn = logRecord.Lsn;
        groupCommits_.Append(TestGroupCommitValidationResult());
        InterlockedIncrement((volatile LONG*)&barrierCount_);
    }

    InterlockedIncrement((volatile LONG*)&processedRecordsCount_);
    InterlockedDecrement((volatile LONG*)&processingRecordsCount_);

    logRecord.CompletedApply(STATUS_SUCCESS);
    co_return;
}

void TestOperationProcessor::PrepareToProcessLogicalRecord()
{
    InterlockedIncrement((volatile LONG*)&processingRecordsCount_);
}
        
void TestOperationProcessor::PrepareToProcessPhysicalRecord()
{
    InterlockedIncrement((volatile LONG*)&processingRecordsCount_);
}

Awaitable<void> TestOperationProcessor::ProcessLoggedRecordAsync(__in LogRecord & logRecord)
{
    if (minDelay_ != 0 && maxDelay_ != 0)
    {
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), TESTOPERATIONPROCESSOR_TAG, random_.Next(minDelay_, maxDelay_), nullptr);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
    }

    InterlockedIncrement((volatile LONG*)&normalCalledCount_);

    if (logRecord.RecordType == LogRecordType::Enum::Barrier)
    {
        groupCommits_[groupCommits_.Count() - 1].Lsn = logRecord.Lsn;
        groupCommits_.Append(TestGroupCommitValidationResult());
        InterlockedIncrement((volatile LONG*)&barrierCount_);
    }
    else if (logRecord.RecordType == LogRecordType::Enum::BeginTransaction)
    {
        groupCommits_[groupCommits_.Count() - 1].IncrementPendingTxCount();
    }
    else if (logRecord.RecordType == LogRecordType::Enum::EndTransaction)
    {
        groupCommits_[groupCommits_.Count() - 1].DecrementPendingTxCount();

        EndTransactionLogRecord const * endTx = dynamic_cast<EndTransactionLogRecord const *>(&logRecord);
        CODING_ERROR_ASSERT(endTx != nullptr);
        
        if (endTx->IsCommitted)
        {
            groupCommits_[groupCommits_.Count() - 1].IncrementCommittedTxCount();
        }
        else
        {
            groupCommits_[groupCommits_.Count() - 1].IncrementAbortedTxCount();
        }
    }

    InterlockedIncrement((volatile LONG*)&processedRecordsCount_);
    InterlockedDecrement((volatile LONG*)&processingRecordsCount_);

    co_return;
}

void TestOperationProcessor::UpdateDispatchingBarrierTask(__in CompletionTask & barrierTask)
{
    UNREFERENCED_PARAMETER(barrierTask);
    InterlockedIncrement((volatile LONG*)&updateDispatchedBarrierTaskCount_);
}

// Notification APIs
NTSTATUS TestOperationProcessor::RegisterTransactionChangeHandler(
    __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept
{
    UNREFERENCED_PARAMETER(transactionChangeHandler);
    CODING_ASSERT("NOT IMPLEMENTED");
}

NTSTATUS TestOperationProcessor::UnRegisterTransactionChangeHandler() noexcept
{
    CODING_ASSERT("NOT IMPLEMENTED");
}
