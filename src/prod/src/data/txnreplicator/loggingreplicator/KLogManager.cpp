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
using namespace Data;

KLogManager::KLogManager(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders const & runtimeFolders,
    __in Log::LogManager & dataLogManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient)
    : LogManager(traceId, perfCounters, healthClient, transactionalReplicatorConfig)
    , runtimeFolders_(&runtimeFolders)
    , dataLogManager_(&dataLogManager)
    , dataLogManagerHandle_(nullptr)
    , physicalLogHandle_(nullptr)
    , logicalLogFileSuffix_(nullptr)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , ktlLoggerSharedLogConfig_(ktlLoggerSharedLogConfig)
    , perfCounters_(perfCounters)
    , healthClient_(healthClient)
{
}

KLogManager::~KLogManager()
{
}

KLogManager::SPtr KLogManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders const & runtimeFolders,
    __in Log::LogManager & appHostLogManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in KAllocator & allocator)
{
    KLogManager * pointer = _new(KTLLOGMANAGER_TAG, allocator) KLogManager(
        traceId,
        runtimeFolders,
        appHostLogManager,
        transactionalReplicatorConfig,
        ktlLoggerSharedLogConfig,
        perfCounters,
        healthClient);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return KLogManager::SPtr(pointer);
}

Awaitable<NTSTATUS> KLogManager::CreateCopyLogAsync(
    __in Epoch const startingEpoch,
    __in LONG64 startingLsn,
    __out IndexingLogRecord::SPtr & newHead)
{
    NTSTATUS status;
    IFlushCallbackProcessor::SPtr flushCallbackProcessor = physicalLogWriter_->CallbackManager->FlushCallbackProcessor;
    co_await CloseCurrentLogAsync();

    // Point the current log file alias to the copy log
    SetCurrentLogFileAliasToCopy();

    // Attempt to delete any current copy log associated with this replica
    KGuid aliasGuid;
    status = co_await physicalLogHandle_->ResolveAliasAsync(
        *currentLogFileAlias_,
        CancellationToken::None,
        aliasGuid);

    // Delete if alias resolved successfully
    if (NT_SUCCESS(status))
    {
        status = co_await physicalLogHandle_->DeleteLogicalLogAsync(
            aliasGuid,
            CancellationToken::None);

        if (!NT_SUCCESS(status))
        {
            EventSource::Events->KLMCreateCopyLog(
                TracePartitionId,
                ReplicaId,
                L"DeleteLogicalLogAsync",
                ToStringLiteral(*currentLogFileAlias_),
                ToCommonGuid(aliasGuid),
                status);
        }
    }

    // Create a new log file
    status = co_await CreateLogFileAsync(true, CancellationToken::None, logicalLog_);

    CO_RETURN_ON_FAILURE(status);

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

    IndexingLogRecord::SPtr firstIndexingRecord = IndexingLogRecord::Create(
        startingEpoch,
        startingLsn,
        nullptr,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    physicalLogWriter_->InsertBufferedRecord(*firstIndexingRecord);

    co_await physicalLogWriter_->FlushAsync(L"CreateCopyLogAsync");
    status = co_await firstIndexingRecord->AwaitFlush();

    logHeadRecordPosition_ = firstIndexingRecord->RecordPosition;

    newHead = move(firstIndexingRecord);

    co_return status;
}

Awaitable<NTSTATUS> KLogManager::DeleteLogAsync()
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

    //
    // The order that logs are deleted is important. The backup log should be deleted first, then the
    // copy log and finally the current log. The reason for this is that when a ChangeRole(None) occurs
    // the replicator will write a record in the current log which will indicate that the ChangeRole(None)
    // has occured so that if the process crashes and restarts, the ChangeRole(None) process can continue.
    // Otherwise, if the current log is deleted before the backup log, the backup log will become the 
    // current log and the information about ChangeRole(None) and other data is lost.
    //
    
    // Delete base backup log file
    KString::SPtr baseBackupLogFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        BackupSuffix,
        GetThisAllocator());

    co_await DeleteLogFileAsync(*baseBackupLogFileAlias, CancellationToken::None);

    // Delete base copy log file
    KString::SPtr baseCopyLogFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        CopySuffix,
        GetThisAllocator());

    co_await DeleteLogFileAsync(*baseCopyLogFileAlias, CancellationToken::None);

    co_await DeleteLogFileAsync(*baseLogFileAlias_, CancellationToken::None);

    // MCoskun: After deleting the log we clean the handles.
    // This is required to avoid leaking handles in cases where we delete the log and open a new one in the same Log Manager.
    // An example case is Restore.
    DisposeHandles();

    co_return status;
}

Awaitable<void> KLogManager::DeleteLogFileAsync(
    __in KString const & alias,
    __in CancellationToken & cancellationToken)
{
    NTSTATUS status;
    KGuid logId;

    EventSource::Events->KLMDeleteLogFileStart(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(alias));

    status = co_await physicalLogHandle_->ResolveAliasAsync(alias, cancellationToken, logId);

    if (!NT_SUCCESS(status))
    {
        EventSource::Events->KLMDeleteLogFileFailed(
            TracePartitionId,
            ReplicaId,
            L"Resolve",
            ToStringLiteral(alias),
            status);

        co_return;
    }

    status = co_await physicalLogHandle_->DeleteLogicalLogAsync(logId, cancellationToken);

    if (!NT_SUCCESS(status))
    {
        EventSource::Events->KLMDeleteLogFileFailed(
            TracePartitionId,
            ReplicaId,
            L"Delete",
            ToStringLiteral(alias),
            status);
    }

    co_return;
}

Awaitable<NTSTATUS> KLogManager::InitializeAsync(KString const & dedicatedLogPath)
{
    NTSTATUS status;
   
    if (dataLogManagerHandle_ == nullptr)
    {
        // Retrieve ILogManagerHandle
        status = co_await dataLogManager_->GetHandle(
            *PartitionedReplicaIdentifier,
            runtimeFolders_->get_WorkDirectory(),
            CancellationToken::None,
            dataLogManagerHandle_);

        CO_RETURN_ON_FAILURE(status);
    }

    KString::CSPtr dedicatedLogPathSPtr = &dedicatedLogPath;

    if (dataLogManagerHandle_->Mode == Data::Log::KtlLoggerMode::OutOfProc)
    {
        //
        // Map the dedicated log path if we are in a container and currently using the driver.
        //
        // When running in a container, the container will have a different file system namespace as the
        // host. The filenames are generated from within the container, however the KTLLogger driver will 
        // create them while running in the host namespace. In order for the files to be created in the 
        // correct location, the filename needs to be mapped from the container namespace to the host
        // namespace. This mapping is done here.
        //
        // Fabric provides the mapping into the host namespace in the environment variable named
        // "Fabric_HostApplicationDirectory"
        //
        
        std::wstring fabricHostApplicationDirectory;
        bool res = Common::Environment::GetEnvironmentVariableW(Constants::FabricHostApplicationDirectory, fabricHostApplicationDirectory, Common::NOTHROW());
        if (res)
        {
            EventSource::Events->KLMMappedDedicatedLogPathForContainer(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(dedicatedLogPath),
                ToStringLiteral(fabricHostApplicationDirectory));

            // Not copying directly to dedicatedLogPathSPtr since it is const
            KString::SPtr copyToKString;
            status = KString::Create(copyToKString, GetThisAllocator(), fabricHostApplicationDirectory.c_str(), TRUE);
            if (!NT_SUCCESS(status))
            {
                EventSource::Events->KLMError(
                    TracePartitionId,
                    ReplicaId,
                    L"InitializeAsync: KString::Create",
                    Common::Guid::Empty(),
                    ToStringLiteral(fabricHostApplicationDirectory),
                    status);

                co_return status;
            }

            dedicatedLogPathSPtr = copyToKString.RawPtr();
        }
    }

    status = co_await __super::InitializeAsync(*dedicatedLogPathSPtr);

    CO_RETURN_ON_FAILURE(status);

    // Loads const values as KString for use in concatenation
    InitializeLogSuffix();

    status = co_await OpenPhysicalLogHandleAsync();

    co_return status;
}

void KLogManager::Dispose()
{
    if (IsDisposed)
    {
        return;
    }

    __super::Dispose();

    DisposeHandles();
}

void KLogManager::InitializeLogSuffix()
{
    NTSTATUS status;
    BOOLEAN result;

    // Initialize physical log file suffix
    status = KString::Create(
        logicalLogFileSuffix_,
        GetThisAllocator(),
        (ULONG)Constants::SFLogSuffix.size());

    THROW_ON_FAILURE(status);

    result = logicalLogFileSuffix_->CopyFromAnsi(
        Constants::SFLogSuffix.begin(),
        (ULONG)Constants::SFLogSuffix.size());

    if (result == FALSE)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }
}

void KLogManager::DisposeHandles() noexcept
{
    if (physicalLogHandle_ != nullptr)
    {
        physicalLogHandle_->Dispose();
        physicalLogHandle_ = nullptr;
    }

    if (dataLogManagerHandle_ != nullptr)
    {
        dataLogManagerHandle_->Dispose();
        dataLogManagerHandle_ = nullptr;
    }
}

NTSTATUS KLogManager::StringToGuid(
    __in std::wstring const & guidString,
    __out KGuid& guid
)
{
    bool success = false;
    size_t len;

    auto hr = StringCchLength(guidString.c_str(), MAX_PATH, &len);
    if (SUCCEEDED(hr))
    {
        std::wstring containerId = guidString;

        //
        // get rid of { and } if present
        //
        if (!containerId.empty() && containerId.front() == L'{')
        {
            containerId.erase(containerId.begin());
        }

        if (!containerId.empty() && containerId.back() == L'}')
        {
            containerId.pop_back();
        }

        Common::Guid g;
        success = Common::Guid::TryParse(containerId, g);

        if (success)
        {
            ::GUID const& gg = g.AsGUID();
            guid = gg;
        }
    }
    
    return success ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

Awaitable<NTSTATUS> KLogManager::OpenPhysicalLogHandleAsync()
{
    NTSTATUS status = STATUS_SUCCESS;
    KGuid containerId;
    KString::SPtr containerPath;

    // TODO: Consider moving into logical log
    if (ktlLoggerSharedLogConfig_->ContainerPath == L"")
    {
        status = co_await dataLogManagerHandle_->OpenPhysicalLogAsync(
            CancellationToken::None,
            physicalLogHandle_);
    }
    else
    {
        status = StringToGuid(ktlLoggerSharedLogConfig_->ContainerId, containerId);

        CO_RETURN_ON_FAILURE(status);

        status = KString::Create(containerPath, GetThisAllocator(), ktlLoggerSharedLogConfig_->ContainerPath.c_str(), TRUE);

        CO_RETURN_ON_FAILURE(status);

        status = co_await dataLogManagerHandle_->OpenPhysicalLogAsync(
            *containerPath,
            containerId,          
            CancellationToken::None,
            physicalLogHandle_);        
    }
    
    bool ignoreErrors = true;

    // Log container does not exist so let's create it
    if(physicalLogHandle_ == nullptr)
    {
        // TODO: Consider moving into logical log
        if (ktlLoggerSharedLogConfig_->ContainerPath == L"")
        {
            status = co_await dataLogManagerHandle_->CreateAndOpenPhysicalLogAsync(
                CancellationToken::None,
                physicalLogHandle_);
        } 
        else 
        {
            status = co_await dataLogManagerHandle_->CreateAndOpenPhysicalLogAsync(
                *containerPath,
                containerId,
                ktlLoggerSharedLogConfig_->LogSize,
                ktlLoggerSharedLogConfig_->MaximumNumberStreams,
                ktlLoggerSharedLogConfig_->MaximumRecordSize,
                (Log::LogCreationFlags)ktlLoggerSharedLogConfig_->CreateFlags,
                CancellationToken::None,
                physicalLogHandle_);            
        }

        if (!NT_SUCCESS(status))
        {
            //
            // If the create log returns any error other than STATUS_OBJECT_NAME_COLLISION or an UnauthorizedAccessException
            // then we want to propagate this exception up as this will provide best chance for users to determine 
            // the cause that the replica did not come up.
            //
            // STATUS_OBJECT_NAME_COLLISION and the UnauthorizedAccessException are both benign exceptions. This is due to the fact
            // that there can be multiple replicas that are opening at the same time and each of them will try to ensure
            // that the shared log is created. When they race, one of them may create the shared log file and managed code
            // data structures while the other "loses" the race. STATUS_OBJECT_NAME_COLLISION will come from the file system while
            // UnauthorizedAccessException will come from the logical log managed code in OnCreatePhysicalLog
            //

            if (status != STATUS_OBJECT_NAME_COLLISION)
            {
                ignoreErrors = false;

                EventSource::Events->KLMError(
                    TracePartitionId,
                    ReplicaId,
                    L"OpenPhysicalLogHandleAsync: CreateAndOpenPhysicalLogAsync",
                    Common::Guid::Empty(),
                    L"",
                    status);
            }
            else
            {
                // Since create failed with collision, it must have already been created. Open it again
                // TODO: Workaround until GetOrAdd Semantics is added from log api
                // TODO: Consider moving into logical log
                if (ktlLoggerSharedLogConfig_->ContainerPath == L"")
                {
                    status = co_await dataLogManagerHandle_->OpenPhysicalLogAsync(
                        CancellationToken::None,
                        physicalLogHandle_);
                } 
                else 
                {
                    status = co_await dataLogManagerHandle_->OpenPhysicalLogAsync(
                        *containerPath,
                        containerId,
                        CancellationToken::None,
                        physicalLogHandle_);        
                }
            }
        }
    }

    if(!ignoreErrors &&
       !NT_SUCCESS(status))
    {
        co_return status;
    }

    ASSERT_IF(
        physicalLogHandle_ == nullptr,
        "PhysicalLogHandle must be valid after OpenPhysicalLogHandleAsync");

    // Return success and ignore any error code intentionally
    co_return STATUS_SUCCESS; 
}

Awaitable<NTSTATUS> KLogManager::CreateLogFileAsync(
    __in bool createNew,
    __in CancellationToken const & cancellationToken,
    __out Log::ILogicalLog::SPtr & result)
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(
        logicalLog_ == nullptr || logicalLog_->IsFunctional == false,
        "(this.logicalLog == null) || (this.logicalLog.IsFunctional == false) Is logical log present: {0}",
        logicalLog_ == nullptr);

    // Generate backup and copy log alias' based on the current log file alias
    // The current log file alias is updated to _Copy before CreateLogFileAsync is called in CreateCopyLog
    KString::SPtr backupLogFileAlias = LogManager::Concat(
        *currentLogFileAlias_,
        BackupSuffix,
        GetThisAllocator());

    KString::SPtr copyLogFileAlias = LogManager::Concat(
        *currentLogFileAlias_,
        CopySuffix,
        GetThisAllocator());

    // NOTE: Only works to fully recover when currentLogFileAlias == baseLogFileAlias
    // Attempt to recover log alias' if present and open logical log
    // If alias' are not found, create new log
    KGuid currentLogId;
    status = co_await physicalLogHandle_->RecoverAliasLogsAsync(
        *copyLogFileAlias,
        *currentLogFileAlias_,
        *backupLogFileAlias,
        cancellationToken,
        currentLogId);

    Log::ILogicalLog::SPtr log = nullptr;

    // If recover log succeeded, open the log
    if (NT_SUCCESS(status))
    {
        status = co_await physicalLogHandle_->OpenLogicalLogAsync(
            currentLogId,
            cancellationToken,
            log);

        if (!NT_SUCCESS(status))
        {
            EventSource::Events->KLMError(
                TracePartitionId,
                ReplicaId,
                L"CreateLogFileAsync: OpenLogicalLogAsync",
                ToCommonGuid(currentLogId),
                ToStringLiteral(*currentLogFileAlias_),
                status);

            co_return status;
        }
        
        EventSource::Events->KLMOpenLogicalLog(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(*currentLogFileAlias_),
            ToCommonGuid(currentLogId),
            status);
    }
    // If any error other than NOT_FOUND, return 
    else if (status != STATUS_NOT_FOUND)
    {
        EventSource::Events->KLMError(
            TracePartitionId,
            ReplicaId,
            L"CreateLogFileAsync: RecoverAliasLogsAsync",
            Common::Guid::Empty(),
            ToStringLiteral(*currentLogFileAlias_),
            status);

        co_return status;
    }

    if (log == nullptr)
    {
        if (!createNew)
        {
            EventSource::Events->KLMLogFileNotFound(
                TracePartitionId,
                ReplicaId);
        }

        // Assume open failed above - try creation of new logical log
        // Create new logical log id
        KGuid newLogicalLog;
        newLogicalLog.CreateNew();
        
        // Convert new logical log id to PWCHAR
        BOOLEAN boolResult;
        const KWString newLogicalLogStr(GetThisAllocator(), newLogicalLog);
        PWCHAR newLogicalLogStrPtr = (PWCHAR)newLogicalLogStr;

        // Create new full log file name
        KString::SPtr fullLogFileName = KPath::CreatePath(*logFileDirectoryPath_, GetThisAllocator());
        KPath::CombineInPlace(*fullLogFileName, newLogicalLogStrPtr);

        boolResult = fullLogFileName->Concat(*logicalLogFileSuffix_);
        if (boolResult == FALSE)
        {
            co_return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        EventSource::Events->KLMCreateLog(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(*fullLogFileName),
            ToCommonGuid(newLogicalLog),
            ToStringLiteral(*currentLogFileAlias_));

        LONG64 maximumStreamSizeInBytes = transactionalReplicatorConfig_->MaxStreamSizeInMB * 1024 * 1024;

        // Attempt to open
        status = co_await physicalLogHandle_->CreateAndOpenLogicalLogAsync(
            newLogicalLog,
            currentLogFileAlias_,
            *fullLogFileName,
            nullptr,
            maximumStreamSizeInBytes,
            (ULONG)transactionalReplicatorConfig_->MaxRecordSizeInKB * 1024,
            transactionalReplicatorConfig_->OptimizeLogForLowerDiskUsage ? 
                Log::LogCreationFlags::UseSparseFile : 
                Log::LogCreationFlags::UseNonSparseFile,
            cancellationToken,
            log);

        if (!NT_SUCCESS(status))
        {
            EventSource::Events->KLMError(
                TracePartitionId,
                ReplicaId,
                L"CreateLogFileAsync",
                ToCommonGuid(newLogicalLog),
                ToStringLiteral(*currentLogFileAlias_),
                status);
            
            co_return status;
        }
    }

    ASSERT_IF(
        log == nullptr,
        "{0}: Logical log cannot be null after CreateLogFileAsync",
        TraceId);

    if (transactionalReplicatorConfig_->OptimizeForLocalSSD) 
    {
        // Only write to the dedicated log
        status = co_await log->ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken::None);

        CO_RETURN_ON_FAILURE(status);
    }

    result = move(log);
    co_return status;
}

Awaitable<NTSTATUS> KLogManager::RenameCopyLogAtomicallyAsync()
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(
        IsCompletelyFlushed == true,
        "{0}: replicatedLogManager.LogManager.IsCompleteFlushed should be true when renaming copy log atomically",
        TraceId);

    // Retrieve head and tail records of the copy log
    LogRecord::SPtr tailRecord = PhysicalLogWriter->CurrentLogTailRecord;
    PhysicalLogWriterCallbackManager::SPtr callbackManager = PhysicalLogWriter->CallbackManager;
    PhysicalLogWriter->ResetCallbackManager = true;
    NTSTATUS closedError = PhysicalLogWriter->ClosedError;

    // Now, close the copy log
    status = co_await CloseCurrentLogAsync();

    CO_RETURN_ON_FAILURE(status);

    // Create backup log alias
    KString::SPtr backupLogFileAlias = LogManager::Concat(
        *baseLogFileAlias_,
        BackupSuffix,
        GetThisAllocator());

    // Intent is to replace the data log (LogFileName) with the current copy log (currentLogFileName)
    // Steps to do this:
    // 1. Delete any logical log with an alias LogFileName + "backup"
    // 2. Change the alias of the logical log with the guid associated with LogFileName to baseLogFileName + "backup"
    // 3. Change the alias of the logical log with guid associated with currentLogFilename to baseLogFileName
    status = co_await physicalLogHandle_->ReplaceAliasLogsAsync(
        *currentLogFileAlias_,
        *baseLogFileAlias_,
        *backupLogFileAlias,
        CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        EventSource::Events->KLMRenameCopyLogError(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(*currentLogFileAlias_),
            ToStringLiteral(*baseLogFileAlias_),
            ToStringLiteral(*backupLogFileAlias),
            status);

        co_return status;
    }

    // Opens using currentLogFileAlias
    currentLogFileAlias_ = baseLogFileAlias_;
    status = co_await CreateLogFileAsync(false, CancellationToken::None, logicalLog_);

    CO_RETURN_ON_FAILURE(status);

    // Write cursor is auto-positioned to eos
    physicalLogWriter_ = PhysicalLogWriter::Create(
        *PartitionedReplicaIdentifier,
        *logicalLog_,
        *callbackManager,
        DefaultWriteCacheSizeMB * 1024 * 1024,
        false,
        *tailRecord,
        perfCounters_,
        healthClient_,
        transactionalReplicatorConfig_,
        closedError,
        GetThisAllocator());
    
    EventSource::Events->KLMRenameCopyLog(
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
