// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("FileLogManager");

const PWCHAR FileLogManager::logFileNameSuffix_ = L".log";

KString::SPtr FileLogManager::GetFullPathToLog(
    KString const & folder,
    KString const & fileName,
    KAllocator & allocator)
{
    NTSTATUS status;
    KString::SPtr fullFileName;
#if !defined(PLATFORM_UNIX)
    status = KString::Create(fullFileName, allocator, L"\\??\\");
#else
    status = KString::Create(fullFileName, allocator, L"");
#endif
    ASSERT_IFNOT(
        NT_SUCCESS(status), 
        "Failed to create prefix with {0:x}",
        status);

    BOOLEAN result = fullFileName->Concat(folder);
    ASSERT_IFNOT(
        result == TRUE, 
        "Failed to concatenate folder name");

    result = fullFileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    ASSERT_IFNOT(
        result == TRUE, 
        "Failed to concatenate path separator");

    result = fullFileName->Concat(fileName);
    ASSERT_IFNOT(
        result == TRUE, 
        "Failed to concatenate filename");

    result = fullFileName->Concat(logFileNameSuffix_);
    ASSERT_IFNOT(
        result == TRUE, 
        "Failed to concatenate '.log'");

    fullFileName->SetNullTerminator();

    return fullFileName;
}

FileLogManager::FileLogManager(
    __in PartitionedReplicaId const & traceId,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : LogManager(traceId, perfCounters, healthClient, transactionalReplicatorConfig)
    , perfCounters_(perfCounters)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , healthClient_(healthClient)
{
}

FileLogManager::~FileLogManager()
{
}

FileLogManager::SPtr FileLogManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    FileLogManager * pointer = _new(FILELOGMANAGER_TAG, allocator)FileLogManager(traceId, perfCounters, healthClient, transactionalReplicatorConfig);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return FileLogManager::SPtr(pointer);
}

Awaitable<NTSTATUS> FileLogManager::CreateCopyLogAsync(
    __in Epoch const startingEpoch,
    __in LONG64 startingLsn,
    __out IndexingLogRecord::SPtr & newHead)
{
    IFlushCallbackProcessor::SPtr flushCallbackProcessor = physicalLogWriter_->CallbackManager->FlushCallbackProcessor;

    NTSTATUS status = co_await CloseCurrentLogAsync();

    CO_RETURN_ON_FAILURE(status);
    
    SetCurrentLogFileAliasToCopy();

    KString::SPtr fullPathToLog = GetFullPathToLog(*logFileDirectoryPath_, *currentLogFileAlias_, GetThisAllocator());
    Common::ErrorCode ec = Common::File::Delete2(fullPathToLog->operator LPCWSTR());

    if (!ec.IsSuccess())
    {
        EventSource::Events->FileLogManagerDeleteLogFailed(
            TracePartitionId,
            ReplicaId,
            ec,
            ToStringLiteral(*currentLogFileAlias_));
    }

    status = co_await CreateLogFileAsync(false, CancellationToken::None, logicalLog_);

    PhysicalLogWriterCallbackManager::SPtr callbackManager = PhysicalLogWriterCallbackManager::Create(
        *PartitionedReplicaIdentifier,
        GetThisAllocator());

    callbackManager->FlushCallbackProcessor = *flushCallbackProcessor;

    physicalLogWriter_ = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        *callbackManager,
        LogManager::DefaultWriteCacheSizeMB * 1024 * 1024,
        false,
        *invalidLogRecords_->Inv_LogRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        GetThisAllocator());

    IndexingLogRecord::SPtr logHead = IndexingLogRecord::Create(
        startingEpoch,
        startingLsn,
        nullptr,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    physicalLogWriter_->InsertBufferedRecord(*logHead);

    co_await physicalLogWriter_->FlushAsync(L"CreateCopyLogAsync");
    status = co_await logHead->AwaitFlush();

    CO_RETURN_ON_FAILURE(status);

    logHeadRecordPosition_ = logHead->RecordPosition;

    newHead = move(logHead);

    co_return status;
}

Awaitable<NTSTATUS> FileLogManager::DeleteLogAsync()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (physicalLogWriter_ != nullptr)
    {
        physicalLogWriter_->Dispose();
    }

    if (logicalLog_ != nullptr)
    {
        status = co_await CloseLogAsync();

        CO_RETURN_ON_FAILURE(status);
    }

    KString::SPtr backupFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        BackupSuffix,
        GetThisAllocator());

    EventSource::Events->FileLogManagerDeleteFile(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(*backupFileAlias));

    DeleteLogFileAsync(*backupFileAlias);

    KString::SPtr copyFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        CopySuffix,
        GetThisAllocator());

    EventSource::Events->FileLogManagerDeleteFile(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(*copyFileAlias));

    DeleteLogFileAsync(*copyFileAlias);

    EventSource::Events->FileLogManagerDeleteFile(
       TracePartitionId,
        ReplicaId,
        ToStringLiteral(*baseLogFileAlias_));

    DeleteLogFileAsync(*baseLogFileAlias_);

    co_return status;
}

void FileLogManager::DeleteLogFileAsync(KString const & fileName)
{
    KString::SPtr fullPath = GetFullPathToLog(
        *logFileDirectoryPath_, 
        fileName, 
        GetThisAllocator());
        
    if (Common::File::Exists(fullPath->operator LPCWSTR()))
    {
        Common::ErrorCode ec = Common::File::Delete2(fullPath->operator LPCWSTR());

        if (!ec.IsSuccess())
        {
            EventSource::Events->FileLogManagerDeleteFileFailed(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(fileName),
                ec);
        }
    }
}

Awaitable<NTSTATUS> FileLogManager::CreateLogFileAsync(
    __in bool createNew,
    __in CancellationToken const & cancellationToken,
    __out ILogicalLog::SPtr & result)
{
    UNREFERENCED_PARAMETER(createNew);
    UNREFERENCED_PARAMETER(cancellationToken);

    KString::SPtr fullLogFileName = GetFullPathToLog(
        *logFileDirectoryPath_,
        *currentLogFileAlias_,
        GetThisAllocator());
    
    if (!Common::File::Exists(fullLogFileName->operator LPCWSTR()))
    {
        // If the current log does not exist, use the backup file if it exists
        KString::SPtr backupFileAlias = LogManager::Concat(
            *baseLogFileAlias_,
            BackupSuffix,
            GetThisAllocator());

        KString::SPtr backupFullLogFileName = GetFullPathToLog(
            *logFileDirectoryPath_,
            *backupFileAlias,
            GetThisAllocator());

        if (Common::File::Exists(backupFullLogFileName->operator LPCWSTR()))
        {
            // If the backup file exists, rename it to the current log file name.
            Common::File::Move(backupFullLogFileName->operator LPCWSTR(), fullLogFileName->operator LPCWSTR());
        }
    }
    
    FaultyFileLog::SPtr fileLog = nullptr;
    NTSTATUS status = FaultyFileLog::Create(
        *PartitionedReplicaIdentifier,
        GetThisAllocator(), 
        fileLog);

    CO_RETURN_ON_FAILURE(status);
    
    KWString fileName(GetThisAllocator(), *fullLogFileName);
    status = co_await fileLog->OpenAsync(fileName);

    CO_RETURN_ON_FAILURE(status);
    
    result = fileLog.RawPtr();
    co_return status;
}

Awaitable<NTSTATUS> FileLogManager::RenameCopyLogAtomicallyAsync()
{
    ASSERT_IFNOT(
        IsCompletelyFlushed == true,
        "{0}: replicatedLogManager.LogManager.IsCompleteFlushed should be true when renaming copy log atomically",
        TraceId);

    LogRecord::SPtr tailRecord = physicalLogWriter_->CurrentLogTailRecord;
    PhysicalLogWriterCallbackManager::SPtr callBackManager = physicalLogWriter_->CallbackManager;
    physicalLogWriter_->ResetCallbackManager = true;
    NTSTATUS closedError = physicalLogWriter_->ClosedError;
    NTSTATUS status = STATUS_SUCCESS;

    status = co_await CloseCurrentLogAsync();

    if (!NT_SUCCESS(status))
    {
        EventSource::Events->FileLogManagerRenameCopyLogAtomicallyException(
            TracePartitionId,
            ReplicaId,
            status,
            "");

        callBackManager->Dispose();
        co_return status;
    }

    KString::SPtr fullCurrentLogFileName = GetFullPathToLog(
        *logFileDirectoryPath_,
        *currentLogFileAlias_,
        GetThisAllocator());

    KString::SPtr fullBaseLogFileName = GetFullPathToLog(
        *logFileDirectoryPath_,
        *baseLogFileAlias_,
        GetThisAllocator());

    KString::SPtr backupFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        BackupSuffix,
        GetThisAllocator());

    KString::SPtr fullBackupLogFileName = GetFullPathToLog(
        *logFileDirectoryPath_,
        *backupFileAlias,
        GetThisAllocator());
    
    Common::ErrorCode ec = Common::File::Replace(
        fullCurrentLogFileName->operator LPCWSTR(),
        fullBaseLogFileName->operator LPCWSTR(),
        fullBackupLogFileName->operator LPCWSTR(),
        false);

    if (!ec.IsSuccess())
    {
        EventSource::Events->FileLogManagerRenameCopyLogAtomicallyException(
            TracePartitionId,
            ReplicaId,
            STATUS_ABANDONED,
            "");

        callBackManager->Dispose();
        co_return STATUS_ABANDONED;
    }

    currentLogFileAlias_ = baseLogFileAlias_;

    status = co_await CreateLogFileAsync(false, CancellationToken::None, logicalLog_);

    if (!NT_SUCCESS(status))
    {
        EventSource::Events->FileLogManagerRenameCopyLogAtomicallyException(
            TracePartitionId,
            ReplicaId,
            status,
            "");

        callBackManager->Dispose();
        co_return status;
    }

    // write cursor is auto-position to eos

    physicalLogWriter_ = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        *callBackManager,
        LogManager::DefaultWriteCacheSizeMB * 1024 * 1024,
        false,
        *tailRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        closedError,
        GetThisAllocator());

    EventSource::Events->FileLogManagerRenameCopyLogAtomically(
        TracePartitionId,
        ReplicaId,
        logHeadRecordPosition_,
        tailRecord->RecordType,
        tailRecord->Lsn,
        tailRecord->Psn,
        tailRecord->RecordPosition);

    ASSERT_IFNOT(
        IsCompletelyFlushed == true,
        "{0}: replicatedLogManager.LogManager.IsCompleteFlushed should be true when renaming copy log atomically",
        TraceId);

    co_return status;
}
