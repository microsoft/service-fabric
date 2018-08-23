// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

int const LogManager::DefaultWriteCacheSizeMB = 300;
KStringView const LogManager::CopySuffix = L"_Copy";
KStringView const LogManager::BackupSuffix = L"_Backup";
KStringView const LogManager::LogFileNameSuffix = L"Log_";

LogManager::LogManager(
    __in PartitionedReplicaId const & traceId,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , isDisposed_(false)
    , logHeadRecordPosition_(0)
    , readersLock_()
    , baseLogFileAlias_(GenerateLogFileAlias(traceId.PartitionId, traceId.ReplicaId, GetThisAllocator()))
    , currentLogFileAlias_(GenerateLogFileAlias(traceId.PartitionId, traceId.ReplicaId, GetThisAllocator()))
    , logFileDirectoryPath_()
    , logicalLog_()
    , physicalLogWriter_()
    , emptyCallbackManager_()
    , earliestLogReader_(LogReaderRange::DefaultLogReaderRange())
    , logReaderRanges_(GetThisAllocator())
    , pendingLogHeadTruncationContext_()
    , invalidLogRecords_(InvalidLogRecords::Create(GetThisAllocator()))
    , perfCounters_(perfCounters)
    , healthClient_(healthClient)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"LogManager",
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(logReaderRanges_);
}

LogManager::~LogManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"LogManager",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        isDisposed_, 
        "{0}: Log manager object is not disposed on destructor",
        TraceId);
}

KString::CSPtr LogManager::GenerateLogFileAlias(
    __in KGuid const & partitionIdGuid,
    __in LONG64 const & replicaIdLong,
    __in KAllocator & allocator)
{
    // Construct the file name format:
    // "Log_PartitionId_ReplicaId"

    KString::SPtr logFile;
    NTSTATUS status = KString::Create(
        logFile,
        allocator,
        LogFileNameSuffix);
    THROW_ON_FAILURE(status);
    
    Common::Guid g(partitionIdGuid);
    wstring partitionId = g.ToString();
    if (!partitionId.empty() && partitionId.front() == L'{')
    {
        partitionId.erase(partitionId.begin());
    }

    if (!partitionId.empty() && partitionId.back() == L'}')
    {
        partitionId.pop_back();
    }

    KString::SPtr replicaId;
    status = KString::Create(
        replicaId,
        allocator, 
        KStringView::Max64BitNumString);
    THROW_ON_FAILURE(status);

    BOOLEAN result = replicaId->FromLONGLONG(replicaIdLong);
    ASSERT_IFNOT(
        result == TRUE,
        "Failed to convert replicaId:{0} to KString",
        replicaIdLong);

    result = logFile->Concat(partitionId.c_str());
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    result = logFile->Concat(L"_");
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    result = logFile->Concat(*replicaId);
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    result = logFile->SetNullTerminator();
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return KString::CSPtr(logFile.RawPtr());
}

KString::SPtr Data::LoggingReplicator::LogManager::Concat(
    __in KString const & str1,
    __in KStringView const & str2,
    __in KAllocator & allocator)
{
    KString::SPtr concatenatedString;
    NTSTATUS status = KString::Create(
        concatenatedString,
        allocator,
        str1);

    THROW_ON_FAILURE(status);

    BOOLEAN result = concatenatedString->Concat(str2);
    if (result == FALSE)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    result = concatenatedString->SetNullTerminator();
    if (result == FALSE)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return concatenatedString;
}

void LogManager::ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept
{
    NTSTATUS errorCode = loggedRecords.LogError;

    for (ULONG i = 0; i < loggedRecords.Count; i++)
    { 
        loggedRecords[i]->CompletedFlush(errorCode);
    }
}

void LogManager::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    if (physicalLogWriter_ != nullptr)
    {
        physicalLogWriter_->Dispose();
    }

    if (logicalLog_ != nullptr)
    {
        Awaitable<void> task = DisposeLogicalLog();
        ToTask(task);
    }

    if (emptyCallbackManager_ != nullptr)
    {
        emptyCallbackManager_->Dispose();
    }

    isDisposed_ = true;
}

Awaitable<void> LogManager::DisposeLogicalLog()
{
    KCoShared$ApiEntry()
    co_await logicalLog_->CloseAsync(CancellationToken::None);

    co_return;
}

void LogManager::SetCurrentLogFileAliasToCopy()
{
    KString::SPtr copyLogFileAlias;

    NTSTATUS status = KString::Create(
        copyLogFileAlias,
        GetThisAllocator(), 
        *baseLogFileAlias_);

    THROW_ON_FAILURE(status);
    
    BOOLEAN result = copyLogFileAlias->Concat(CopySuffix);
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    result = copyLogFileAlias->SetNullTerminator();
    if (!result)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    currentLogFileAlias_ = copyLogFileAlias.RawPtr();
}

Awaitable<NTSTATUS> LogManager::InitializeAsync(__in KString const & dedicatedLogPath)
{
    logFileDirectoryPath_ = &dedicatedLogPath;

    emptyCallbackManager_ = PhysicalLogWriterCallbackManager::Create(
        *PartitionedReplicaIdentifier,
        GetThisAllocator());

    emptyCallbackManager_->FlushCallbackProcessor = *this;

    co_await suspend_never{};
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogManager::OpenAsync(__out RecoveryPhysicalLogReader::SPtr & result)
{
    NTSTATUS status = co_await CreateLogFileAsync(true, CancellationToken::None, logicalLog_);

    CO_RETURN_ON_FAILURE(status);

    LONG64 logicalLogLength = logicalLog_->GetLength();

    if (logicalLogLength <= sizeof(int))
    {
        // No usable content in the log
        if (logicalLog_->GetWritePosition() > 0)
        {
            co_await logicalLog_->TruncateTail(0, CancellationToken());

            ASSERT_IFNOT(
                logicalLog_->GetLength() == 0, 
                "{0}: Logical log is not empty during open: {1}",
                TraceId,
                logicalLog_->GetLength());

            ASSERT_IFNOT(
                logicalLog_->GetWritePosition() == 0, 
                "{0}: Logical log write position is not zero: {1}",
                TraceId,
                logicalLog_->GetWritePosition());
        }

        PhysicalLogWriter::SPtr initializeLogWriter = PhysicalLogWriter::Create(
            *PartitionedReplicaIdentifier,
            *logicalLog_,
            *emptyCallbackManager_,
            DefaultWriteCacheSizeMB * 1024 * 1024,
            false,
            *invalidLogRecords_->Inv_LogRecord,
            perfCounters_,
            healthClient_,
            transactionalReplicatorConfig_,
            GetThisAllocator());

        IndexingLogRecord::SPtr zeroIndexRecord = IndexingLogRecord::CreateZeroIndexingLogRecord(
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());
        initializeLogWriter->InsertBufferedRecord(*zeroIndexRecord);

        UpdateEpochLogRecord::SPtr zeroUpdateEpochLogRecord = UpdateEpochLogRecord::CreateZeroUpdateEpochLogRecord(
            *invalidLogRecords_->Inv_PhysicalLogRecord, 
            GetThisAllocator());
        initializeLogWriter->InsertBufferedRecord(*zeroUpdateEpochLogRecord);

        BeginCheckpointLogRecord::SPtr zeroBeginCheckpointLogRecord = BeginCheckpointLogRecord::CreateZeroBeginCheckpointLogRecord(
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            *invalidLogRecords_->Inv_BeginTransactionOperationLogRecord,
            GetThisAllocator());

        initializeLogWriter->InsertBufferedRecord(*zeroBeginCheckpointLogRecord);

        BarrierLogRecord::SPtr oneBarrierLogRecord = BarrierLogRecord::CreateOneBarrierRecord(
            *invalidLogRecords_->Inv_PhysicalLogRecord, 
            GetThisAllocator());
        initializeLogWriter->InsertBufferedRecord(*oneBarrierLogRecord);

        EndCheckpointLogRecord::SPtr oneEndCheckpointLogRecord = EndCheckpointLogRecord::CreateOneEndCheckpointLogRecord(
            *zeroBeginCheckpointLogRecord,
            *zeroIndexRecord,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());

        initializeLogWriter->InsertBufferedRecord(*oneEndCheckpointLogRecord);

        CompleteCheckPointLogRecord::SPtr oneCompleteCheckpointLogRecord = CompleteCheckPointLogRecord::Create(
            Constants::OneLsn,
            *zeroIndexRecord,
            oneEndCheckpointLogRecord.RawPtr(),
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());

        initializeLogWriter->InsertBufferedRecord(*oneCompleteCheckpointLogRecord);

        co_await initializeLogWriter->FlushAsync(L"LogManager_OpenAsync");

        // This additional await is required to ensure the log record was actually flushed.
        // Without this, the flush async could succeed, but the log record flush could have failed due to a write error

        status = co_await oneCompleteCheckpointLogRecord->AwaitFlush();

        initializeLogWriter->Dispose();
    }

    CO_RETURN_ON_FAILURE(status);

    result = RecoveryPhysicalLogReader::Create(
        *this,
        GetThisAllocator());

    co_return status;
}

Awaitable<NTSTATUS> LogManager::OpenWithRestoreFilesAsync(
    __in KArray<KString::CSPtr> const & restoreFileArray,
    __in LONG64 blockSizeInKB,
    __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords,
    __in ktl::CancellationToken const & cancellationToken,
    __out KSharedPtr<RecoveryPhysicalLogReader> & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    const LONG64 blockSizeInBytes = blockSizeInKB * Constants::BytesInKBytes;

    status = co_await CreateLogFileAsync(true, cancellationToken, logicalLog_);

    CO_RETURN_ON_FAILURE(status);

    // No usable content in the log
    if (logicalLog_->GetWritePosition() > 0)
    {
        co_await logicalLog_->TruncateTail(0, CancellationToken());

        ASSERT_IFNOT(
            logicalLog_->GetLength() == 0,
            "{0}: Logical log is not empty during open: {1}",
            TraceId,
            logicalLog_->GetLength());

        ASSERT_IFNOT(
            logicalLog_->GetWritePosition() == 0,
            "{0}: Logical log write position is not zero: {1}",
            TraceId,
            logicalLog_->GetWritePosition());
    }

    // Initialize physical log writer with default maxwritecachesizeinbytes and recompute offsets = true
    // Ensures invalid offset is not read when restoring from previous version backups after the addition of new fields
    PhysicalLogWriter::SPtr logWriter = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        *emptyCallbackManager_,
        static_cast<LONG>(DefaultWriteCacheSizeMB * Constants::BytesInKBytes * Constants::BytesInKBytes),
        true,
        *invalidLogRecords_->Inv_LogRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        GetThisAllocator());

    KFinally([&] {logWriter->Dispose(); });

    LogRecordsMap::SPtr logRecordsMapSPtr = nullptr;
    LONG64 bufferedRecordsSizeBytes = 0;
    LONG64 backupRecordIndex = 0;

    for (KString::CSPtr logFilePath : restoreFileArray)
    {
        BackupLogFileAsyncEnumerator::SPtr enumeratorSPtr = nullptr;
        try
        {
            KWString backupLogFilePath(GetThisAllocator(), *logFilePath);

            CO_RETURN_ON_FAILURE(backupLogFilePath.Status());

            BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                *PartitionedReplicaIdentifier,
                backupLogFilePath,
                GetThisAllocator());

            co_await backupLogFileSPtr->ReadAsync(cancellationToken);
            enumeratorSPtr = backupLogFileSPtr->GetAsyncEnumerator();
        }
        catch (Exception const & exception)
        {
            ASSERT_IFNOT(enumeratorSPtr == nullptr, "{0}. Enumerator must be null if above threw.", TraceId);
            co_return exception.GetStatus();
        }

        try
        {
            // If this is the first record in the full backup log.
            if (logRecordsMapSPtr == nullptr)
            {
                bool hasFirstRecord = co_await enumeratorSPtr->MoveNextAsync(cancellationToken);
                ASSERT_IFNOT(hasFirstRecord, "{0}. Full backup log empty.", TraceId);

                // If the log is being restored.
                // First record must be a indexing log record. Flush it.
                LogRecord::SPtr firstRecordFromBackupLog = enumeratorSPtr->GetCurrent();
                ASSERT_IFNOT(firstRecordFromBackupLog != nullptr, "{0}. BackupLogEnumerator will never return null", TraceId);
                ASSERT_IFNOT(
                    LogRecord::IsInvalid(firstRecordFromBackupLog.RawPtr()) == false,
                    "{0}. First record read from the backup log cannot be invalid.",
                    TraceId);
                ASSERT_IFNOT(
                    firstRecordFromBackupLog->RecordType == LogRecordType::Indexing,
                    "{0}. First record read from the backup log must be indexing log record: Type: {1} LSN: {2} PSN: {3}",
                    TraceId,
                    firstRecordFromBackupLog->RecordType,
                    firstRecordFromBackupLog->Lsn,
                    firstRecordFromBackupLog->Psn);

                bufferedRecordsSizeBytes = logWriter->InsertBufferedRecord(*firstRecordFromBackupLog);
                backupRecordIndex++;

                // Note that indexingLogRecord->PreviousPhysicalRecord is an InvalidLogRecord.
                IndexingLogRecord::SPtr logHeadIndexingRecord = dynamic_cast<IndexingLogRecord *>(firstRecordFromBackupLog.RawPtr());

                // Used for linking the transactions.
                logRecordsMapSPtr = LogRecordsMap::Create(*PartitionedReplicaIdentifier, *logHeadIndexingRecord, invalidLogRecords, GetThisAllocator());

                logRecordsMapSPtr->ProcessLogRecord(*firstRecordFromBackupLog);

                EventSource::Events->RestoreRecord(
                    TracePartitionId,
                    ReplicaId,
                    static_cast<LONG64>(firstRecordFromBackupLog->RecordType),
                    firstRecordFromBackupLog->Lsn,
                    firstRecordFromBackupLog->Psn,
                    LONG64_MAX);
            }

            // Process remaining records in the backup log.
            LogRecord::SPtr logRecordSPtr = nullptr;
            while (co_await enumeratorSPtr->MoveNextAsync(cancellationToken))
            {
                logRecordSPtr = enumeratorSPtr->GetCurrent();
                logRecordsMapSPtr->ProcessLogRecord(*logRecordSPtr);

                // Insert the record
                bufferedRecordsSizeBytes += logWriter->InsertBufferedRecord(*logRecordSPtr);
                backupRecordIndex++;

                // TODO: Use a backup config for this flush size determination
                if (bufferedRecordsSizeBytes >= blockSizeInBytes)
                {
                    co_await logWriter->FlushAsync(L"Intermediate restore flush for file");

                    // This additional await is required to ensure the log record was indeed flushed.
                    // Without this, the flushasync could succeed, but the log record flush could have failed due to a write error
                    status = co_await logRecordSPtr->AwaitFlush();

                    CO_RETURN_ON_FAILURE(status);

                    bufferedRecordsSizeBytes = 0;
                }

                EventSource::Events->RestoreRecord(
                    TracePartitionId,
                    ReplicaId,
                    static_cast<LONG64>(logRecordSPtr->RecordType),
                    logRecordSPtr->Lsn,
                    logRecordSPtr->Psn,
                    logRecordSPtr->PreviousPhysicalRecord->Psn);
            }

            // Flush any remaining buffers.
            if (bufferedRecordsSizeBytes > 0)
            {
                co_await logWriter->FlushAsync(L"Final restore flush for file");

                // This additional await is required to ensure the log record was indeed flushed.
                // Without this, the flushasync could succeed, but the log record flush could have failed due to a write error
                status = co_await logRecordSPtr->AwaitFlush();

                CO_RETURN_ON_FAILURE(status);

                bufferedRecordsSizeBytes = 0;
            }
        }
        catch (Exception const & exception)
        {
            status = exception.GetStatus();
        }

        try
        {
            co_await enumeratorSPtr->CloseAsync();
        }
        catch (Exception const & exception)
        {
            co_return exception.GetStatus();
        }

        CO_RETURN_ON_FAILURE(status);
    }

    result = RecoveryPhysicalLogReader::Create(*this, GetThisAllocator());
    CO_RETURN_ON_FAILURE(result->Status());

    co_return status;
}

void LogManager::PrepareToLog(
    __in LogRecord & tailRecord,
    __in PhysicalLogWriterCallbackManager & callbackManager)
{
    ASSERT_IFNOT(
        physicalLogWriter_ == nullptr || physicalLogWriter_->IsDisposed,
        "{0}: PrepareToLog : physicalLogWriter_ must be disposed or nullptr",
        TraceId);

    physicalLogWriter_ = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        callbackManager,
        DefaultWriteCacheSizeMB * 1024 * 1024,
        false,
        tailRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        STATUS_SUCCESS,
        GetThisAllocator());
}

void LogManager::PrepareToClose()
{
    // It is possible the physicallogwriter is null if there was an incomplete CR->None during which we crashed and the next open skipped recovery due to this
    // It is only at the end of recovery that we set the physical log writer to a valid object
    if (physicalLogWriter_ == nullptr)
    {
        return;
    }

    physicalLogWriter_->PrepareToClose();
}

LONG64 LogManager::InsertBufferedRecord(__in LogRecord & record)
{
    return physicalLogWriter_->InsertBufferedRecord(record);
}

Awaitable<void> LogManager::FlushAsync(KStringView const & initiator)
{
    // Since many components do not await on the flush, add the api entry
    // to ensure the object lives as long as this call is completed
    KCoShared$ApiEntry()

    co_await physicalLogWriter_->FlushAsync(initiator);
    co_return;
}

AwaitableCompletionSource<void>::SPtr LogManager::GetFlushCompletionTask()
{
    return physicalLogWriter_->GetFlushCompletionTask();
}

Awaitable<NTSTATUS> LogManager::PerformLogTailTruncationAsync(LogRecord & newTailRecord)
{
    NTSTATUS status = co_await physicalLogWriter_->TruncateLogTail(newTailRecord);

    co_return status;
}

Awaitable<NTSTATUS> LogManager::CloseLogAsync()
{
    NTSTATUS status = STATUS_SUCCESS;
    Data::Log::ILogicalLog::SPtr logicalLog = Ktl::Move(logicalLog_);

    if (logicalLog != nullptr)
    {
        status = co_await logicalLog->CloseAsync(CancellationToken());
    }

    co_return status;
}

Awaitable<NTSTATUS> LogManager::CloseCurrentLogAsync()
{
    ASSERT_IFNOT(
        logReaderRanges_.Count() == 0, 
        "{0}: Log reader range count is not zero during close: {1}",
        TraceId,
        logReaderRanges_.Count());

    physicalLogWriter_->Dispose();

    NTSTATUS status = co_await CloseLogAsync();

    co_return status;
}

AwaitableCompletionSource<void>::SPtr LogManager::ProcessLogHeadTruncationAsync(__in TruncateHeadLogRecord & truncateHeadRecord)
{
    AwaitableCompletionSource<void>::SPtr tcs = CompletionTask::CreateAwaitableCompletionSource<void>(LOGMANAGER_TAG, GetThisAllocator());

    PendingLogHeadTruncationContext::SPtr truncationContext = PendingLogHeadTruncationContext::Create(
        GetThisAllocator(),
        *tcs,
        truncateHeadRecord);
    
    K_LOCK_BLOCK(readersLock_)
    {
        if (earliestLogReader_.StartingRecordPosition < truncateHeadRecord.HeadRecord->RecordPosition)
        {
            ASSERT_IFNOT(
                pendingLogHeadTruncationContext_ == nullptr,
                "{0}: ProcessLogHeadTruncationAsync : pendingLogHeadTruncationContext_ already exists",
                TraceId);

            pendingLogHeadTruncationContext_ = truncationContext;
            
            EventSource::Events->ProcessLogHeadTruncationWaiting(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(earliestLogReader_.ReaderName));

            return tcs;
        }

        logHeadRecordPosition_ = truncateHeadRecord.HeadRecord->RecordPosition;
    }

    PerformLogHeadTruncationAsync(*truncationContext);

    return tcs;
}

Task LogManager::PerformLogHeadTruncationAsync(__in PendingLogHeadTruncationContext & truncationContext)
{
    KCoShared$ApiEntry()

    PhysicalLogWriter::SPtr physicalLogWriter = physicalLogWriter_;
    PendingLogHeadTruncationContext::SPtr localContext = &truncationContext;
    NTSTATUS status = STATUS_SUCCESS;

    ULONG32 freeBackwardlinkCallCount = 0;
    ULONG32 freeBackwardlinkSuceededCallCount = 0;

    localContext->NewHead->OnTruncateHead(
        *invalidLogRecords_,
        freeBackwardlinkCallCount,
        freeBackwardlinkSuceededCallCount);

    EventSource::Events->ProcessLogHeadTruncationDone(
        TracePartitionId,
        ReplicaId,
        localContext->NewHead->Lsn,
        localContext->NewHead->Psn,
        localContext->NewHead->RecordPosition,
        freeBackwardlinkCallCount,
        freeBackwardlinkSuceededCallCount);

    status = co_await physicalLogWriter->TruncateLogHeadAsync(localContext->ProposedPosition);

    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}: Failed to Truncate Log Head at {1} with NTSTATUS: {2:x}",
        TraceId,
        localContext->ProposedPosition,
        status);

    localContext->Tcs->Set();
}

bool LogManager::AddLogReader(
    __in LONG64 startingLsn,
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType)
{
    ASSERT_IFNOT(
        startingRecordPosition == 0 || logHeadRecordPosition_ <= startingRecordPosition,
        "{0}: Starting position: {1} is less than log head position: {2}",
        TraceId,
        startingRecordPosition, 
        logHeadRecordPosition_);
    
    EventSource::Events->AddLogReader(
        TracePartitionId,
        ReplicaId,
        startingRecordPosition,
        endingRecordPosition,
        ToStringLiteral(readerName));

    K_LOCK_BLOCK(readersLock_)
    {
        if (logHeadRecordPosition_ > startingRecordPosition)
        {
            return false;
        }

        ULONG i = 0;

        for (i = 0; i < logReaderRanges_.Count(); i++)
        {
            LogReaderRange & readerRange = logReaderRanges_[i];

            if (startingRecordPosition < readerRange.StartingRecordPosition)
            {
                break;
            }

            if (startingRecordPosition == readerRange.StartingRecordPosition)
            {
                readerRange.AddRef();

                // If a fullcopy reader comes in at the same position of a non-full copy reader, over-write the type as full-copy reader
                // This is important to determine safe LSN for converting soft deletes to hard deletes in dynamic state manager

                if (readerType == LogReaderType::Enum::FullCopy)
                {
                    readerRange.ReaderType = readerType;
                }

                return true;
            }
        }

        LogReaderRange newRange(
            startingLsn,
            startingRecordPosition,
            readerName,
            readerType);

        NTSTATUS status = logReaderRanges_.InsertAt(i, newRange);
        THROW_ON_FAILURE(status);

        if (i == 0)
        {
            earliestLogReader_ = newRange;
        }
    }

    ASSERT_IFNOT(
        endingRecordPosition == MAXLONG64 || physicalLogWriter_->CurrentLogTailRecord->RecordPosition >= endingRecordPosition,
        "{0}: Ending record position:{1} should be less than current tail record position: {2}",
        TraceId,
        endingRecordPosition, 
        physicalLogWriter_->CurrentLogTailRecord->RecordPosition);

    return true;
}

void LogManager::RemoveLogReader(__in ULONG64 startingRecordPosition)
{
    EventSource::Events->RemoveLogReader(
        TracePartitionId,
        ReplicaId,
        startingRecordPosition);

    K_LOCK_BLOCK(readersLock_)
    {
        for (ULONG i = 0; i < logReaderRanges_.Count(); i++)
        {
            LogReaderRange & readerRange = logReaderRanges_[i];

            if (readerRange.StartingRecordPosition == startingRecordPosition)
            {
                if (readerRange.Release() > 0)
                {
                    return;
                }

                BOOLEAN result = logReaderRanges_.Remove(i);
                ASSERT_IFNOT(
                    result,
                    "{0}: RemoveLogReader : Failed to remove {1} from logReaderRange_",
                    TraceId,
                    i);

                if (i == 0)
                {
                    earliestLogReader_ =
                        logReaderRanges_.Count() > 0 ?
                        logReaderRanges_[0] :
                        LogReaderRange::DefaultLogReaderRange();
                }

                // check if removing this reader could end up in completing a pending log head truncation

                if (pendingLogHeadTruncationContext_ != nullptr &&
                    earliestLogReader_.StartingRecordPosition >= pendingLogHeadTruncationContext_->ProposedPosition)
                {
                    logHeadRecordPosition_ = pendingLogHeadTruncationContext_->ProposedPosition;

                    PendingLogHeadTruncationContext::SPtr pendingTruncationContext = Ktl::Move(pendingLogHeadTruncationContext_);
                    
                    EventSource::Events->RemoveLogReaderInitiateTruncation(
                        TracePartitionId,
                        ReplicaId,
                        logHeadRecordPosition_);

                    PerformLogHeadTruncationAsync(*pendingTruncationContext);
                }

                return;
            }
        }
    }

    ASSERT_IFNOT(
        false,
        "{0}: RemoveLogReader : Failed to return during reader enumeration",
        TraceId);
}

void LogManager::SetLogHeadRecordPosition(ULONG64 newHeadPosition)
{
    logHeadRecordPosition_ = newHeadPosition;
}

LONG64 LogManager::GetEarliestFullCopyStartingLsn()
{
    LONG64 earliestFullCopyStartingLsn = MAXLONG64;

    K_LOCK_BLOCK(readersLock_)
    {
        for (ULONG i = 0; i < logReaderRanges_.Count(); i++)
        {
            LogReaderRange & readerRange = logReaderRanges_[i];

            if (readerRange.ReaderType == LogReaderType::Enum::FullCopy &&
                readerRange.FullCopyStartingLsn < earliestFullCopyStartingLsn)
            {
                earliestFullCopyStartingLsn = readerRange.FullCopyStartingLsn;

                // The first reader need not be the earliest as the list is NOT sorted by starting LSN of the enumerator
                // Continue searhing for the smallest value
            }
        }
    }

    return earliestFullCopyStartingLsn;
}

LogReaderRange LogManager::GetEarliestLogReaderRange()
{
    LogReaderRange range;

    K_LOCK_BLOCK(readersLock_)
    {
        range = earliestLogReader_;
    }

    return range;
}

IPhysicalLogReader::SPtr LogManager::GetPhysicalLogReader(
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in LONG64 startingLsn,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType)
{
    IPhysicalLogReader::SPtr logReader = nullptr;

    do
    {
        logReader = PhysicalLogReader::Create(
            *this,
            startingRecordPosition,
            endingRecordPosition,
            startingLsn,
            readerName,
            readerType,
            GetThisAllocator());

        if (logReader->IsValid)
        {
            break;
        }
    } while (true);

    return logReader;
}

Data::Log::ILogicalLogReadStream::SPtr LogManager::CreateReaderStream()
{
    Data::Log::ILogicalLogReadStream::SPtr readerStream = nullptr;

    NTSTATUS status = logicalLog_->CreateReadStream(readerStream, 0);

    THROW_ON_FAILURE(status);

    return readerStream;
}

void LogManager::SetSequentialAccessReadSize(
    __in Data::Log::ILogicalLogReadStream & readStream,
    __in LONG64 sequentialReadSize)
{
    LONG readAhead = 0;

    if (sequentialReadSize > MAXLONG)
    {
        readAhead = MAXLONG;
    }
    else
    {
        readAhead = (LONG)sequentialReadSize;
    }

    logicalLog_->SetSequentialAccessReadSize(readStream, readAhead);
}

LogManager::PendingLogHeadTruncationContext::PendingLogHeadTruncationContext(
    __in AwaitableCompletionSource<void> & tcs,
    __in TruncateHeadLogRecord & truncateHeadRecord)
    : tcs_(&tcs)
    , truncateHeadRecord_(&truncateHeadRecord)
{
}

LogManager::PendingLogHeadTruncationContext::~PendingLogHeadTruncationContext()
{
}

LogManager::PendingLogHeadTruncationContext::SPtr LogManager::PendingLogHeadTruncationContext::Create(
    __in KAllocator & allocator,
    __in AwaitableCompletionSource<void> & tcs,
    __in TruncateHeadLogRecord & truncateHeadRecord)
{
    LogManager::PendingLogHeadTruncationContext * pointer = _new(LOGMANAGER_TAG, allocator)
        LogManager::PendingLogHeadTruncationContext(
            tcs, 
            truncateHeadRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return LogManager::PendingLogHeadTruncationContext::SPtr(pointer);
}
