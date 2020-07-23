// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Common;
using namespace Data::Utilities;
using namespace ktl;

Common::StringLiteral const TraceComponent("BackupManager");

KStringView const BackupManager::FullMetadataFileName(L"backup.metadata");
KStringView const BackupManager::IncrementalMetadataFileName(L"incremental.metadata");

KStringView const BackupManager::RestoreFolderName(L"restore");
KStringView const BackupManager::RestoreTokenName(L"restore.token");
KStringView const BackupManager::BackupFolderName(L"trbackup");
KStringView const BackupManager::ReplicatorBackupFolderName(L"lr");
KStringView const BackupManager::ReplicatorBackupLogName(L"backup.log");
KStringView const BackupManager::StateManagerBackupFolderName(L"sm");

#define NumberOfBytesInKB 1024

BackupManager::SPtr BackupManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in KString const & workFolder,
    __in TxnReplicator::IStateProviderManager & stateManager,
    __in IReplicatedLogManager & replicatedLogManager,
    __in ILogManager & logManager,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    BackupManager * pointer = _new(BACKUP_MANAGER_TAG, allocator) BackupManager(
        traceId,
        workFolder,
        stateManager,
        replicatedLogManager,
        logManager,
        transactionalReplicatorConfig,
        healthClient,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    THROW_ON_FAILURE(pointer->Status());
    return BackupManager::SPtr(pointer);
}

void BackupManager::Initialize(
    __in TxnReplicator::IBackupRestoreProvider & backupRestoreProvider,
    __in ICheckpointManager & checkpointManager,
    __in IOperationProcessor & operationProcessor)
{
    NTSTATUS status = backupRestoreProvider.GetWeakIfRef(backupRestoreProviderWRef_);
    THROW_ON_FAILURE(status);

    status = checkpointManager.GetWeakIfRef(checkpointManagerWRef_);
    THROW_ON_FAILURE(status);

    status = operationProcessor.GetWeakIfRef(operationProcessorWRef_);
    THROW_ON_FAILURE(status);
}

void BackupManager::SetLastCompletedBackupLogRecord(
    __in BackupLogRecord const & backupLogRecord)
{
    lastCompletedBackupLogRecordThreadSafeCache_.Put(&backupLogRecord);
}

BackupLogRecord::CSPtr BackupManager::GetLastCompletedBackupLogRecord() const
{
    return lastCompletedBackupLogRecordThreadSafeCache_.Get();
}

/// <summary>
/// Undo the last completed backup log record.
/// </summary>
void BackupManager::UndoLastCompletedBackupLogRecord()
{
    BackupLogRecord::CSPtr lastCompletedBackupLogRecord = lastCompletedBackupLogRecordThreadSafeCache_.Get();

    ASSERT_IFNOT(lastCompletedBackupLogRecord != nullptr && lastCompletedBackupLogRecord->IsZeroBackupLogRecord() == false, 
        "{0}: If there has been no completed backup log record, it cannot be undone.", 
        TraceId);

    EventSource::Events->UndoBackupLogRecord(
        TracePartitionId,
        ReplicaId,
        lastCompletedBackupLogRecord->Lsn,
        lastCompletedBackupLogRecord->Psn,
        lastCompletedBackupLogRecord->RecordPosition,
        lastCompletedBackupLogRecord->HighestBackedupEpoch.DataLossVersion,
        lastCompletedBackupLogRecord->HighestBackedupEpoch.ConfigurationVersion,
        lastCompletedBackupLogRecord->HighestBackedupLsn);

    // Note that given truncate tail can only undo one backup log record.
    // In the current implementation, if a replica goes through truncate tail and the primary does not replicate any other backup log records
    // this replica will be required to take a full backup when it becomes primary.
    // The probability can be reduced by keeping track of the last the completed backup log records.
    // In this way, if there is a LCB -1, LCB can become LCB -1 and backup in the last completed begin checkpoint otherwise.
    InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(GetThisAllocator());
    BackupLogRecord::SPtr zeroBackupLogRecord = BackupLogRecord::CreateZeroBackupLogRecord(*(invalidLogRecords->Inv_PhysicalLogRecord), GetThisAllocator());

    lastCompletedBackupLogRecordThreadSafeCache_.Put(zeroBackupLogRecord.RawPtr());
}

bool BackupManager::ShouldCleanState() const
{
    // Port Note: Difference due to locking change.
    if (isOnDataLossInProgress_ == true)
    {
        return false;
    }

    // If currently no restore is ongoing and restore token exists (replica is being opened) 
    // then logging replicator should initiate cleaning of the state since last restore was incomplete.
    return File::Exists(static_cast<wstring>(*restoreTokenFilePathCSPtr_));
}

void BackupManager::StateCleaningComplete()
{
    // Use Empty Guid for state cleaning completed.
    DeleteRestoreToken(Common::Guid::Empty());
}

bool BackupManager::IsLogUnavailableDueToRestore() const noexcept
{
    return isLogUnavailableDueToRestore_;
}

ktl::Awaitable<TxnReplicator::BackupInfo> BackupManager::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler)
{
    KShared$ApiEntry();

    auto result = co_await BackupAsync(
        backupCallbackHandler, 
        FABRIC_BACKUP_OPTION_FULL, 
        Common::TimeSpan::MaxValue, 
        CancellationToken::None);

    co_return result;
}

// Summary: This method backs up a replicator.
//
// Algorithm:
// 1. Argument validation & Prepare the backup.
// 2. Backup SM if Full. Get log records and backup.
// 3. Write backup metadata.
// 4. Ensure that the backed up data is stabilized. (Replicated and await processing of a barrier).
// 5. Call the user callback where they need to upload to external store.
// 6. Replicate a backup log record indicating the latest backed up LSN.
// 
// TODO: Not implementing timeout - waiting for timer solution.
// Port Note: The backup folder are calculated at construction instead of during this call.
ktl::Awaitable<TxnReplicator::BackupInfo> BackupManager::BackupAsync(
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Generate backup id for this backup.
    Common::Guid backupId = GenerateBackupId();
    Common::Guid parentBackupId = Common::Guid();

    // These variables used to do step trace, time duration + accumulated message.
    int64 argValidateDuration = -1;
    int64 stateManagerBackupDuration = -1;
    int64 getLogRecordsAndBackupDuration = -1;
    int64 writeMetadataDuration = -1;
    int64 ensureStableDuration = -1;
    int64 userCallBackDuration = -1;
    int64 replicateBackupLogRecordDuration = -1;
    int64 lastStepElapsedTime = 0;
    Stopwatch backupStopwatch;
    backupStopwatch.Start();

    // Can throw SF_STATUS_BACKUP_IN_PROGRESS
    // Step 1: Argument validation & Prepare the backup.
    // Acquire backup API lock. Used to ensure there is at max one backup at any given point of time.
    co_await this->AcquireEntryLock(backupId, timeout, LockRequester::Backup);
    KFinally([&] { ReleaseEntryLock(); });

    // Argument validation
    ASSERT_IF(backupOption == FABRIC_BACKUP_OPTION_INVALID, "{0}: FABRIC_BACKUP_OPTION_INVALID requested.", TraceId);
    ThrowIfFullBackupIsMissing(backupOption, backupId);

    // Accept the backup call.
    EventSource::Events->IBM_BackupAsyncStart(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        backupOption,
        transactionalReplicatorConfig_->EnableIncrementalBackupsAcrossReplicas,
        Epoch::InvalidEpoch().DataLossVersion,
        Epoch::InvalidEpoch().ConfigurationVersion,
        FABRIC_INVALID_SEQUENCE_NUMBER,
        ToStringLiteral(*backupFolderCSPtr_));

    // Ensure that the replicator backup folder has been created and clean.
    co_await PrepareBackupFolderAsync(backupOption, backupId);

    // Update step 1 time duration and trace.
    argValidateDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = backupStopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_BackupAsync(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId, 
        Epoch::InvalidEpoch().DataLossVersion,
        Epoch::InvalidEpoch().ConfigurationVersion,
        FABRIC_INVALID_SEQUENCE_NUMBER,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    // Step 2: Backup SM if Full. Get log records and backup.
    IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecords = nullptr;
    if (backupOption == FABRIC_BACKUP_OPTION_FULL)
    {
        // This call will be drained in case of change role or close.
        logRecords = co_await BackupStateManagerAndGetReplicatorLogsToBeBackedUpAsync(
            backupId,
            *smBackupFolderCSPtr_,
            timeout,
            cancellationToken);

        stateManagerBackupDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    }
    else
    {
        logRecords = co_await GetIncrementalLogRecordsAsync(
            backupId,
            timeout,
            cancellationToken);
    }

    ReplicatorBackup replicatorBackupInfo = ReplicatorBackup::Invalid();
    {
        KFinally([&] {logRecords->Dispose(); });

        replicatorBackupInfo = co_await BackupReplicatorAsync(backupId, backupOption, *replicatorLogBackupFilePathCSPtr_, *logRecords, cancellationToken);
    }

    // This is the first time when we have accurate information of how much space the log records took.
    // Common reason for this is that records are serialized as logical and incremental filters records.
    ThrowIfAccumulatedIncrementalBackupLogsIsTooLarge(backupOption, backupId, replicatorBackupInfo);

    // Update step 2 time duration and trace.
    getLogRecordsAndBackupDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = backupStopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_BackupAsync(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        Epoch::InvalidEpoch().DataLossVersion,
        Epoch::InvalidEpoch().ConfigurationVersion,
        FABRIC_INVALID_SEQUENCE_NUMBER,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    BackupLogRecord::CSPtr previousBackupLogRecord = lastCompletedBackupLogRecordThreadSafeCache_.Get();
    parentBackupId = backupOption == FABRIC_BACKUP_OPTION_FULL ?
        backupId :
        Common::Guid(previousBackupLogRecord->BackupId);

    // Step 3: Write backup metadata
    status = co_await WriteBackupMetadataFileAsync(
        backupOption,
        backupId,
        parentBackupId, 
        replicatorBackupInfo,
        cancellationToken);
    THROW_ON_FAILURE(status);

    // Update step 3 time duration and trace.
    writeMetadataDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = backupStopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_BackupAsync(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.DataLossNumber,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
        replicatorBackupInfo.LsnOfHighestBackedUpLogRecord,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    // Step 4: Ensure that the backed up data is stabilized. (Replicated and await processing of a barrier)
    // Wait for the highest backed up lsn to be replicated to ensure no false progress.
    // This is only required when we have incremental backup.
    {
        ICheckpointManager::SPtr checkpointManager = GetCheckpointManager();
        if (replicatorBackupInfo.LsnOfHighestBackedUpLogRecord > checkpointManager->LastStableLsn)
        {
            co_await ReplicateAndAwaitBarrierLogRecordAsync(backupId);
        }
    }

    // Update step 4 time duration and trace.
    ensureStableDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = backupStopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_BackupAsync(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.DataLossNumber,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
        replicatorBackupInfo.LsnOfHighestBackedUpLogRecord,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    // Step 5: Call the user callback where they need to upload to external store.
    BackupVersion backupVersion(replicatorBackupInfo.EpochOfHighestBackedUpLogRecord, replicatorBackupInfo.LsnOfHighestBackedUpLogRecord);
    BackupVersion startBackupVersion(replicatorBackupInfo.EpochOfFirstBackedUpLogRecord, replicatorBackupInfo.LsnOfFirstLogicalLogRecord);
    
    BackupInfo backupInfo(
        *backupFolderCSPtr_, 
        backupOption, 
        backupVersion, 
        startBackupVersion, 
        backupId,
        parentBackupId,
        GetThisAllocator());
    THROW_ON_FAILURE(backupInfo.Status());

    // Port Note: If return false from callback, this method will simply throw SF_STATUS_INVALID_OPERATION
    status = co_await CallBackupCallbackAsync(backupId, backupInfo, backupCallbackHandler, cancellationToken);
    THROW_ON_FAILURE(status);

    // Update step 5 time duration and trace.
    userCallBackDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = backupStopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_BackupAsync(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.DataLossNumber,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
        replicatorBackupInfo.LsnOfHighestBackedUpLogRecord,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    // Step 6: Replicate a backup log record indicating the latest backed up LSN.
    status = co_await ReplicateBackupLogRecordAsync(backupId, replicatorBackupInfo);
    THROW_ON_FAILURE(status);
    
    replicateBackupLogRecordDuration = backupStopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    EventSource::Events->IBM_BackupAsyncFinish(
        TracePartitionId,
        ReplicaId,
        backupId,
        parentBackupId,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.DataLossNumber,
        replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
        replicatorBackupInfo.LsnOfHighestBackedUpLogRecord,
        argValidateDuration,
        stateManagerBackupDuration,
        getLogRecordsAndBackupDuration,
        writeMetadataDuration,
        ensureStableDuration,
        userCallBackDuration,
        replicateBackupLogRecordDuration);

    co_return backupInfo;
}

ktl::Awaitable<void> BackupManager::RestoreAsync(
    __in KString const & backupFolder)
{
    KShared$ApiEntry();

    co_await RestoreAsync(backupFolder, FABRIC_RESTORE_POLICY_SAFE, TimeSpan::MaxValue, CancellationToken::None);
    co_return;
}

ktl::Awaitable<void> BackupManager::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(timeout);

    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ThrowIfOnDataLossIsNotInflight();

    Guid restoreId = Guid::NewGuid();
    KGuid kRestoreId(restoreId.AsGUID());

    // These variables used to do step trace, time duration + accumulated message.
    int64 argValidateDuration = -1;
    int64 stateManagerRestoreDuration = -1;
    int64 replicatorRestoreDuration = -1;
    int64 becomePrimaryDuration = -1;
    int64 lastStepElapsedTime = 0;
    Stopwatch stopwatch;
    stopwatch.Start();

    // Step 1: Argument validation + Prepare for restore
    // in Windows input backupFolder might not have GDN (//??//) prefix.
    // This causes problems later on. This ensures that backupFolderPath always has GDN prefixed when required
    KString::SPtr snapBackupFolderPath(KPath::CreatePath(backupFolder, GetThisAllocator()));

    BackupFolderInfo::SPtr backupFolderInfoSPtr = BackupFolderInfo::Create(
        *PartitionedReplicaIdentifier, 
        kRestoreId, 
        *snapBackupFolderPath, 
        GetThisAllocator());
    co_await backupFolderInfoSPtr->AnalyzeAsync(cancellationToken);

    // Can throw SF_STATUS_INVALID_OPERATION
    // Acquire backup API lock. Used to ensure there is at max one backup at any given point of time.
    co_await this->AcquireEntryLock(restoreId, timeout, LockRequester::Restore);
    KFinally([&] { ReleaseEntryLock(); });

    // Caller holds lock.
    ThrowIfInvalidState(restoreId);
    ThrowIfNotSafe(restorePolicy, *backupFolderInfoSPtr, restoreId);

    EventSource::Events->IBM_RestoreAsyncStart(
        TracePartitionId,
        ReplicaId,
        restoreId,
        backupFolderInfoSPtr->HighestBackedupEpoch.ConfigurationVersion,
        backupFolderInfoSPtr->HighestBackedupEpoch.DataLossVersion,
        backupFolderInfoSPtr->HighestBackedupLSN,
        backupFolderInfoSPtr->LogPathList.Count(), 
        ToStringLiteral(*snapBackupFolderPath));

    IBackupRestoreProvider::SPtr loggingReplicator = GetLoggingReplicator();
    status = co_await loggingReplicator->CancelTheAbortingOfTransactionsIfNecessaryAsync();
    THROW_ON_FAILURE(status);

    co_await CreateRestoreTokenAsync(restoreId);

    // Update step 1 time duration and trace.
    argValidateDuration = stopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = stopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_RestoreAsync(
        TracePartitionId,
        ReplicaId,
        restoreId,
        argValidateDuration,
        stateManagerRestoreDuration,
        replicatorRestoreDuration,
        becomePrimaryDuration);

    // Step 2: StateManager Restore
    status = co_await stateManagerSPtr_->RestoreCheckpointAsync(*backupFolderInfoSPtr->StateManagerBackupFolderPath, cancellationToken);
    THROW_ON_FAILURE(status);

    // Update step 2 time duration and trace.
    stateManagerRestoreDuration = stopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = stopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_RestoreAsync(
        TracePartitionId,
        ReplicaId,
        restoreId,
        argValidateDuration,
        stateManagerRestoreDuration,
        replicatorRestoreDuration,
        becomePrimaryDuration);

    Epoch dataLossEpoch = replicatedLogManagerSPtr_->CurrentLogTailEpoch;

    // Step 3: Replicator Restore
    // Close the replicator, delete log.
    status = co_await loggingReplicator->CloseAsync();
    THROW_ON_FAILURE(status);

    ASSERT_IFNOT(isLogUnavailableDueToRestore_ == false, "{0}: isLogUnavailableDueToRestore is excepted to be false", TraceId);
    isLogUnavailableDueToRestore_ = true;

    status = co_await loggingReplicator->PrepareForRestoreAsync();
    THROW_ON_FAILURE(status);

    status = co_await RecoverRestoreDataAsync(*loggingReplicator, restoreId, *backupFolderInfoSPtr);
    THROW_ON_FAILURE(status);

    // Port Note: Finish restore before changing role.
    status = co_await FinishRestoreAsync(*loggingReplicator, dataLossEpoch);
    THROW_ON_FAILURE(status);

    isLogUnavailableDueToRestore_ = false;

    // Update step 3 time duration and trace.
    replicatorRestoreDuration = stopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    lastStepElapsedTime = stopwatch.ElapsedMilliseconds;
    EventSource::Events->IBM_RestoreAsync(
        TracePartitionId,
        ReplicaId,
        restoreId,
        argValidateDuration,
        stateManagerRestoreDuration,
        replicatorRestoreDuration,
        becomePrimaryDuration);

    // Step 4: Reset the replica role and primary state. These would have been reset during recovery
    status = co_await loggingReplicator->BecomePrimaryAsync(true);
    THROW_ON_FAILURE(status);

    // Port Note: Bug in managed: State Manager is primary for the duration of the restore.
    // This causes its role checks to be incorrect.
    // Now, SM sets it role to Unknown as part of RestoreCheckpointAsync and becomes primary here.
    CancellationToken & tmpCancellationToken = const_cast<CancellationToken &>(cancellationToken);
    co_await stateManagerSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, tmpCancellationToken);

    bool restoreTokenExists = CheckIfRestoreTokenExists();
    ASSERT_IFNOT(restoreTokenExists, "{0}: Restore token must exist.", TraceId);
    DeleteRestoreToken(restoreId);

    becomePrimaryDuration = stopwatch.ElapsedMilliseconds - lastStepElapsedTime;
    EventSource::Events->IBM_RestoreAsyncFinish(
        TracePartitionId,
        ReplicaId,
        restoreId,
        argValidateDuration,
        stateManagerRestoreDuration,
        replicatorRestoreDuration,
        becomePrimaryDuration);
}

void BackupManager::EnableBackup() noexcept
{
    K_LOCK_BLOCK(backupDrainLock_)
    {
        ASSERT_IFNOT(isBackupDrainablePhaseAllowed_ == false, "{0}: Backup could not have been already allowed.", TraceId);
        ASSERT_IFNOT(isBackupPhaseInflight_ == false, "{0}: Backup phase could not be inflight.", TraceId);
        ASSERT_IFNOT(drainBackupCallBackAcsSPtr_ == nullptr, "{0}: Drainable callback could not be set.", TraceId);
        
        isBackupDrainablePhaseAllowed_ = true;
    }
}

ktl::Awaitable<void> BackupManager::DisableBackupAndDrainAsync()
{
    ktl::AwaitableCompletionSource<void>::SPtr drainBackupAcs = nullptr;

    K_LOCK_BLOCK(backupDrainLock_)
    {
        isBackupDrainablePhaseAllowed_ = false;
        if (isBackupPhaseInflight_ == true)
        {
            ASSERT_IFNOT(drainBackupCallBackAcsSPtr_ == nullptr, "{0}: this.drainBackupCallBackTcs must be null", TraceId);

            // This operation can throw.
            drainBackupCallBackAcsSPtr_ = CompletionTask::CreateAwaitableCompletionSource<void>(BACKUP_MANAGER_TAG, GetThisAllocator());
            drainBackupAcs = drainBackupCallBackAcsSPtr_;
        }
    }

    if (drainBackupAcs != nullptr)
    {
        co_await drainBackupAcs->GetAwaitable();
    }
}

void BackupManager::EnableRestore() noexcept
{
    ASSERT_IFNOT(isOnDataLossInProgress_ == false, "{0}: OnDataloss could not have been already in-flight.", TraceId);
    isOnDataLossInProgress_ = true;
}

void BackupManager::DisableRestore() noexcept
{
    ASSERT_IFNOT(isOnDataLossInProgress_ == true, "{0}: OnDataloss must hnot have been already in-flight", TraceId);
    isOnDataLossInProgress_ = false;
}

NTSTATUS BackupManager::DeletePersistedState() noexcept
{
    ErrorCode errorCode;

    wstring backupFolder(*backupFolderCSPtr_);
    if (Directory::Exists(backupFolder))
    {
        errorCode = Directory::Delete_WithRetry(backupFolder, true, true);
        if (errorCode.IsSuccess() == false)
        {
            return StatusConverter::Convert(errorCode.ToHResult());
        }
    }

    wstring restoreTokenFolder(*restoreTokenFolderPathCSPtr_);
    if (Directory::Exists(restoreTokenFolder))
    {
        errorCode = Directory::Delete_WithRetry(restoreTokenFolder, true, true);
        if (errorCode.IsSuccess() == false)
        {
            return StatusConverter::Convert(errorCode.ToHResult());
        }
    }

    return STATUS_SUCCESS;
}

KString::CSPtr BackupManager::CreateBackupFolderPath(
    __in KString const & workFolder,
    __in KGuid const & partitionId,
    __in LONG64 replicaId,
    __in KAllocator & allocator)
{
    ASSERT_IF(workFolder.IsNull() || workFolder.IsEmpty(), "{0}:{1} CreateBackupFolderPath: WorkFolder cannot be nullptr or empty", Common::Guid(partitionId), replicaId);
    ASSERT_IF(wcslen(workFolder) == 0, "{0}:{1} workFolder cannot be empty", Common::Guid(partitionId), replicaId);

    KString::SPtr workDirectoryPath = KPath::CreatePath(workFolder, allocator);

    Common::Guid commonPartitionId(partitionId);
    wstring partitionIdString = commonPartitionId.ToString('D');
    KPath::CombineInPlace(*workDirectoryPath, partitionIdString.c_str());

    KLocalString<20> replicaIdString;
    BOOLEAN boolStatus = replicaIdString.FromLONGLONG(replicaId);
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateBackupFolderPath FromLONGLONG failed.", Common::Guid(partitionId), replicaId);

    KPath::CombineInPlace(*workDirectoryPath, replicaIdString);

    KPath::CombineInPlace(*workDirectoryPath, BackupFolderName);

    return workDirectoryPath.RawPtr();
}

KString::CSPtr BackupManager::CreateRestoreFolderPath(
    __in KString const & workFolder,
    __in KGuid const & partitionId,
    __in LONG64 replicaId,
    __in KAllocator & allocator)
{
    ASSERT_IF(workFolder.IsNull() || workFolder.IsEmpty(), "{0}:{1} CreateRestoreFolderPath: WorkFolder cannot be nullptr or empty", Common::Guid(partitionId), replicaId);
    ASSERT_IF(wcslen(workFolder) == 0, "{0}:{1} workFolder cannot be empty", Common::Guid(partitionId), replicaId);

    KString::SPtr workDirectoryPath = KPath::CreatePath(workFolder, allocator);

    // Restore Folder name first for managed equivalence.
    KPath::CombineInPlace(*workDirectoryPath, RestoreFolderName);

    Common::Guid commonPartitionId(partitionId);
    wstring partitionIdString = commonPartitionId.ToString('D');

    KPath::CombineInPlace(*workDirectoryPath, partitionIdString.c_str());

    KLocalString<20> replicaIdString;
    BOOLEAN boolStatus = replicaIdString.FromLONGLONG(replicaId);
    ASSERT_IFNOT(boolStatus == TRUE, "{0}:{1} CreateRestoreFolderPath FromLONGLONG failed.", Common::Guid(partitionId), replicaId);

    KPath::CombineInPlace(*workDirectoryPath, replicaIdString);
    return workDirectoryPath.RawPtr();
}

Common::Guid BackupManager::GenerateBackupId()
{
    while (true)
    {
        Common::Guid backupId = Common::Guid::NewGuid();
        if (backupId != Common::Guid::Empty())
        {
            return backupId;
        }
    }
}

void BackupManager::ThrowIfFullBackupIsMissing(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::Guid const & backupId)
{
    if (backupOption == FABRIC_BACKUP_OPTION_FULL)
    {
        return;
    }

    BackupLogRecord::CSPtr lastCompletedBackupLogRecord = lastCompletedBackupLogRecordThreadSafeCache_.Get();

    Epoch snapOfCurrentEpoch = replicatedLogManagerSPtr_->CurrentLogTailEpoch;

    // If the partition has not take a full backup yet, then incremental backup cannot be allowed.
    if (lastCompletedBackupLogRecord->IsZeroBackupLogRecord())
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId, 
            ReplicaId,
            backupId,
            L"ErrorCode",
            SF_STATUS_MISSING_FULL_BACKUP,
            L"Missing full backup (SF_STATUS_MISSING_FULL_BACKUP)");

        throw ktl::Exception(SF_STATUS_MISSING_FULL_BACKUP);
    }

    // If the incremental backup request has been made in the same epoch as the previous backup,
    // Then allow the incremental backup.
    if (snapOfCurrentEpoch == lastCompletedBackupLogRecord->HighestBackedupEpoch)
    {
        return;
    }

    if (transactionalReplicatorConfig_->EnableIncrementalBackupsAcrossReplicas)
    {
        wstring message = Common::wformatString(
            L"lastCompletedBackupLogRecord->HighestBackedupEpoch:<{0}: {1}>, currentEpoch:<{2}: {3}>",
            lastCompletedBackupLogRecord->HighestBackedupEpoch.DataLossVersion,
            lastCompletedBackupLogRecord->HighestBackedupEpoch.ConfigurationVersion,
            snapOfCurrentEpoch.DataLossVersion,
            snapOfCurrentEpoch.ConfigurationVersion);

        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"ErrorCode",
            SF_STATUS_MISSING_FULL_BACKUP,
            message);

        // If the incremental backup request has been made in different data loss number as the previous backup, 
        // do not allow the incremental backup.
        // Allowing this case increases complexities in terms of potentially possible restore folders. 
        // E.g.
        // 1->2->3->4
        //    |->5->6
        // Note Epoch at incremental backup 4 could be ahead of 6 even though 6 was taken at a later time.
        // This is because cluster which 2 is restored back to before starting the incremental backup 5 only verifies that new data loss number
        // is greater than 2 not 4.
        // Note that progress vector may have been trimmed. If trimmed, than we cannot do the incremental backup.
        ProgressVectorEntry vectorEntry = replicatedLogManagerSPtr_->ProgressVectorValue->Find(lastCompletedBackupLogRecord->HighestBackedupEpoch);
        bool result = vectorEntry != ProgressVectorEntry();
        if (result && snapOfCurrentEpoch.DataLossVersion == lastCompletedBackupLogRecord->HighestBackedupEpoch.DataLossVersion)
        {
            return;
        }
    }

    EventSource::Events->IBM_BackupAsyncFinishError(
        TracePartitionId,
        ReplicaId,
        backupId,
        L"ErrorCode",
        SF_STATUS_MISSING_FULL_BACKUP,
        L"Missing full backup (SF_STATUS_MISSING_FULL_BACKUP)");

    throw ktl::Exception(SF_STATUS_MISSING_FULL_BACKUP);
}

ktl::Awaitable<void> BackupManager::PrepareBackupFolderAsync(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::Guid backupId)
{
    KShared$ApiEntry();

    bool backupDirectoryExists = Directory::Exists(static_cast<LPCWSTR>(*backupFolderCSPtr_));
    if (backupDirectoryExists)
    {
        Common::ErrorCode errorCode = Directory::Delete(static_cast<wstring>(*backupFolderCSPtr_), true);
        if (errorCode.IsSuccess() == false)
        {
            EventSource::Events->IBM_BackupAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                backupId,
                L"ErrorCode",
                errorCode.ToHResult(),
                L"The existing backup folder could not be deleted.");

            throw ktl::Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    int retryCount = 0;
    while (true)
    {
        if (backupOption == FABRIC_BACKUP_OPTION_FULL)
        {
            bool smFolderExists = Directory::Exists(static_cast<wstring>(*smBackupFolderCSPtr_));
            if (smFolderExists == false)
            {
                Common::ErrorCode errorCode = Directory::Create2(static_cast<wstring>(*smBackupFolderCSPtr_));
                if (errorCode.IsSuccess() == false)
                {
                    if (errorCode.IsError(ErrorCodeValue::AccessDenied))
                    {
                        if (retryCount == MaxRetryCount)
                        {
                            EventSource::Events->IBM_BackupAsyncFinishError(
                                TracePartitionId,
                                ReplicaId,
                                backupId,
                                L"HResult",
                                errorCode.ToHResult(),
                                L"Backup folder for state manager could not be created.");

                            throw ktl::Exception(StatusConverter::Convert(errorCode.ToHResult()));
                        }

                        ++retryCount;

                        // MCoskun: Because the starting backoff time is high, use linear backoff instead of exponential.
                        NTSTATUS status = co_await KTimer::StartTimerAsync(
                            GetThisAllocator(), 
                            GetThisAllocationTag(), 
                            StartingBackoffInMilliSeconds * retryCount, 
                            nullptr);
                        THROW_ON_FAILURE(status);

                        continue;
                    }

                    EventSource::Events->IBM_BackupAsyncFinishError(
                        TracePartitionId,
                        ReplicaId,
                        backupId,
                        L"HResult",
                        errorCode.ToHResult(),
                        L"Backup folder for state manager could not be created.");

                    throw ktl::Exception(StatusConverter::Convert(errorCode.ToHResult()));
                }
            }
        }

        Common::ErrorCode errorCode = Directory::Create2(static_cast<wstring>(*lrBackupFolderCSPtr_));
        if (errorCode.IsSuccess() == false)
        {
            if (errorCode.IsError(ErrorCodeValue::AccessDenied))
            {
                if (retryCount == MaxRetryCount)
                {
                    EventSource::Events->IBM_BackupAsyncFinishError(
                        TracePartitionId,
                        ReplicaId,
                        backupId,
                        L"HResult",
                        errorCode.ToHResult(),
                        L"Backup folder for logging replicator could not be created.");

                    throw ktl::Exception(StatusConverter::Convert(errorCode.ToHResult()));
                }

                ++retryCount;

                // MCoskun: Because the starting backoff time is high, use linear backoff instead of exponential.
                NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), GetThisAllocationTag(), StartingBackoffInMilliSeconds, nullptr);
                THROW_ON_FAILURE(status);

                continue;
            }

            EventSource::Events->IBM_BackupAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                backupId,
                L"HResult",
                errorCode.ToHResult(),
                L"Backup folder for logging replicator could not be created.");

            throw ktl::Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }

        co_return;
    }
}

ktl::Awaitable<IAsyncEnumerator<LogRecord::SPtr>::SPtr> BackupManager::BackupStateManagerAndGetReplicatorLogsToBeBackedUpAsync(
    __in Common::Guid backupId,
    __in KString const & stateManagerBackupFolderPath,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecords = nullptr;

    KString::SPtr lockTakerName = nullptr;
    status = KString::Create(lockTakerName, GetThisAllocator(), backupId.ToString().c_str());
    THROW_ON_FAILURE(status);

    ICheckpointManager::SPtr checkpointManagerSPtr = GetCheckpointManager();

    // KDuration is in ticks.
    KDuration duration(timeout.Ticks);
    bool backupConsistencyLockIsAcquired = co_await checkpointManagerSPtr->AcquireBackupAndCopyConsistencyLockAsync(*lockTakerName, duration, cancellationToken);
    if (backupConsistencyLockIsAcquired == false)
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            SF_STATUS_TIMEOUT,
            L"Could not acquire the backup consistency lock.");

        throw ktl::Exception(SF_STATUS_TIMEOUT);
    }

    KFinally([&] {checkpointManagerSPtr->ReleaseBackupAndCopyConsistencyLock(*lockTakerName); });

    status = StartDrainableBackupPhase();
    if (NT_SUCCESS(status) == false)
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            status,
            L"StartDrainableBackupPhase failed.");

        throw ktl::Exception(status);
    }

    {
        KFinally([&] { CompleteDrainableBackupPhase(); });

        IndexingLogRecord::CSPtr indexinLogRecord = this->GetIndexingLogRecordForFullBackup(*replicatedLogManagerSPtr_);
        bool isApiLockAcquired = co_await checkpointManagerSPtr->AcquireStateManagerApiLockAsync(*lockTakerName, duration, cancellationToken);
        if (isApiLockAcquired == false)
        {
            EventSource::Events->IBM_BackupAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                backupId,
                L"status",
                status,
                L"AcquireStateManagerApiLockAsync failed.");

            throw ktl::Exception(status);
        }
        
        {
            KFinally([&] {checkpointManagerSPtr->ReleaseStateManagerApiLock(*lockTakerName); });

            status = co_await stateManagerSPtr_->BackupCheckpointAsync(stateManagerBackupFolderPath, cancellationToken);
            THROW_ON_FAILURE(status);
        }

        logRecords = GetLogRecordsCallerHoldsLock(*indexinLogRecord, *lockTakerName);
    }

    co_return logRecords;
}

/// <summary>
/// Gets the log records for incremental backup.
/// </summary>
/// <returns>Physical log restoreLogStream.</returns>
/// <remarks>
/// Must move all records since the newest Indexing Log Records that is older than the last completed begin checkpoint and
/// earliest begin transaction record.
/// Note that caller does not hold lock.
/// So the stream can be truncated until creation of physical log reader (and isValid).
/// </remarks>
ktl::Awaitable<IAsyncEnumerator<LogRecord::SPtr>::SPtr> BackupManager::GetIncrementalLogRecordsAsync(
    __in Common::Guid backupId,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(timeout);

    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    BackupLogRecord::CSPtr lastCompletedBackupLogRecord = lastCompletedBackupLogRecordThreadSafeCache_.Get();
    if (lastCompletedBackupLogRecord->Lsn == 0)
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            SF_STATUS_MISSING_FULL_BACKUP,
            L"No full backup has been taken in the partition (since last restore).");

        throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
    }

    // #10635451: Not making this part drainable causes null reference since
    // log manager may be deleted while this function is running.
    status = StartDrainableBackupPhase();
    if (NT_SUCCESS(status) == false)
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            status,
            L"StartDrainableBackupPhase failed.");

        throw ktl::Exception(status);
    }

    {
        KFinally([&] { CompleteDrainableBackupPhase(); });

        LogRecord::CSPtr startingLogRecord = FindFirstLogRecordForIncrementalBackup(lastCompletedBackupLogRecord->HighestBackedupLsn, backupId);

        // Tail log record must be snapped to avoid races since it is not under a lock.
        LogRecord::SPtr tailLogRecord = logManagerSPtr_->CurrentLogTailRecord;
        ASSERT_IFNOT(
            tailLogRecord->Lsn > lastCompletedBackupLogRecord->HighestBackedupLsn,
            "{0}: Tail record {1} must be larger than the last highest backed up LSN {2}",
            TraceId,
            tailLogRecord->Lsn,
            lastCompletedBackupLogRecord->HighestBackedupLsn);

        // Usability: We get all the logs to make sure every log that was created before backup was called is in the backup.
        // Note that we use CurrentLogTailRecord.RecordPosition as the end position which is guaranteed to be flushed.
        // Note that this code depends on this method setting read ahead for performance.
        IPhysicalLogReader::SPtr physicalLogReader = logManagerSPtr_->GetPhysicalLogReader(
            startingLogRecord->RecordPosition,
            tailLogRecord->RecordPosition,
            tailLogRecord->Lsn,
            KStringView(backupId.ToString().c_str()),
            LogReaderType::Backup);

        if (physicalLogReader->IsValid == false)
        {
            EventSource::Events->IBM_BackupAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                backupId,
                L"status",
                SF_STATUS_MISSING_FULL_BACKUP,
                wformatString(
                    "Log records since the last highest backed up record {0} have been truncated",
                    startingLogRecord->Lsn));

            throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
        }

        IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecordEnumeratorSPtr = nullptr;
        {
            // Dispose log reader as soon as we have the log reader who will instead hold the truncation.
            KFinally([&] {physicalLogReader->Dispose(); });
            logRecordEnumeratorSPtr = physicalLogReader->GetLogRecords(
                *PartitionedReplicaIdentifier,
                KStringView(backupId.ToString().c_str()),
                LogReaderType::Backup,
                transactionalReplicatorConfig_->ReadAheadCacheSizeInKb * 1024,
                healthClient_,
                transactionalReplicatorConfig_);
        }

        while (co_await logRecordEnumeratorSPtr->MoveNextAsync(cancellationToken) == true)
        {
            if (logRecordEnumeratorSPtr->GetCurrent()->Lsn == lastCompletedBackupLogRecord->HighestBackedupLsn)
            {
                co_return logRecordEnumeratorSPtr;
            }
        }
    }

    CODING_ASSERT("{0}: Highest backup lsn must be in the enumerator.", TraceId);    
    throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
}

IAsyncEnumerator<LogRecord::SPtr>::SPtr BackupManager::GetLogRecordsCallerHoldsLock(
    __in LogRecord const & logRecord,
    __in KString const & lockTakerName)
{
    ULONG64 startingRecordPosition = logRecord.RecordPosition;

    // Tail log record must be snapped to avoid races since it is not under a lock.
    LogRecord::SPtr tailLogRecord = logManagerSPtr_->CurrentLogTailRecord;

    // Usability: We get all the logs to make sure every log that was created before backup was called is in the backup.
    // Note that we use CurrentLogTailRecord.RecordPosition as the end position which is guaranteed to be flushed.
    IPhysicalLogReader::SPtr physicalLogReader = logManagerSPtr_->GetPhysicalLogReader(
        startingRecordPosition,
        tailLogRecord->RecordPosition,
        tailLogRecord->Lsn,
        lockTakerName,
        LogReaderType::Backup);

    ASSERT_IFNOT(
        physicalLogReader->IsValid == true,
        "{0}: Due to caller lock, startingRecordPosition {1} could not have been truncated.",
        TraceId,
        startingRecordPosition);

    KFinally([&] {physicalLogReader->Dispose(); });

    return physicalLogReader->GetLogRecords(
        *PartitionedReplicaIdentifier,
        lockTakerName, 
        LogReaderType::Backup,
        transactionalReplicatorConfig_->ReadAheadCacheSizeInKb * 1024,
        healthClient_,
        transactionalReplicatorConfig_);
}

/// <summary>
/// Gets the indexing log record which will be starting record for the full backup.
/// </summary>
/// <returns>
/// The first indexing log record before the last pending being transaction of the last completed checkpoint.
/// </returns>
/// <remarks>
/// Caller holds backup and copy consistency lock.
/// </remarks>
/// <devnote>
/// Algorithm:
/// 1. Find the last completed begin checkpoint and the relevant earliest pending transaction.
/// 2. Use the backwards physical log record link to find the first indexing log record less than earliest pending transaction.
/// </devnote>
IndexingLogRecord::CSPtr BackupManager::GetIndexingLogRecordForFullBackup(
    __in IReplicatedLogManager & replicatedLogManager)
{
    // Stage 1: Find the last completed begin checkpoint and the relevant earliest pending transaction.
    Data::LogRecordLib::EndCheckpointLogRecord::SPtr lastCompeltedEndCheckpointLogRecord = replicatedLogManager.LastCompletedEndCheckpointRecord;
    ASSERT_IFNOT(
        lastCompeltedEndCheckpointLogRecord != nullptr && LogRecord::IsInvalid(lastCompeltedEndCheckpointLogRecord.RawPtr()) == false, 
        "{0} EndCheckpointLogRecord is always expected to be valid.", 
        TraceId);

    BeginCheckpointLogRecord::SPtr completedBeginCheckpointRecord = lastCompeltedEndCheckpointLogRecord->LastCompletedBeginCheckpointRecord;
    ASSERT_IFNOT(
        completedBeginCheckpointRecord != nullptr && LogRecord::IsInvalid(completedBeginCheckpointRecord.RawPtr()) == false,
        "{0} BeginCheckpointLogRecord is always expected to be valid.",
        TraceId);

    IndexingLogRecord::SPtr previousIndexingRecord = nullptr;
    PhysicalLogRecord::SPtr previousPhysicalRecord = completedBeginCheckpointRecord.RawPtr();

    BeginTransactionOperationLogRecord::SPtr earliestPendingBeginTransactionLogRecord = completedBeginCheckpointRecord->EarliestPendingTransaction;

    // Stage 2: Use the backwards physical log record link to find the first indexing log record less than earliest pending transaction.
    do
    {
        // Search back for the last indexing log record.
        do
        {
            PhysicalLogRecord::SPtr tmp = previousPhysicalRecord;
            previousPhysicalRecord = previousPhysicalRecord->PreviousPhysicalRecord;

            // This method must be called under the backup and copy consistency lock.
            // Because of this a racing truncate cannot happen between stage 1 and stage 2.
            // In this light, below code asserts that there is an indexing record with LSN < EarliestPendingTransaction
            // below the LastCompletedBeginCheckpointRecord.
            ASSERT_IFNOT(
                LogRecord::IsInvalid(previousPhysicalRecord.RawPtr()) == false,
                "{0}: Physical record before PSN {1} must exist.",
                TraceId,
                tmp->Psn);
        } while (previousPhysicalRecord->RecordType != LogRecordType::Indexing);

        previousIndexingRecord = dynamic_cast<IndexingLogRecord *>(previousPhysicalRecord.RawPtr());

        // Check to make sure that the indexing record is older that the earliest pending begin transaction record.
        // If there is no pending begin transaction record, we are done since we started scanning back from last completed begin checkpoint.
        if (earliestPendingBeginTransactionLogRecord == nullptr)
        {
            break;
        }

        // If indexing record is not old enough, keep scanning back.
        if (previousIndexingRecord->RecordPosition >= earliestPendingBeginTransactionLogRecord->RecordPosition)
        {
            continue;
        }

        break;
    } while (true);

    ASSERT_IFNOT(
        previousIndexingRecord != nullptr,
        "{0}: Previous Indexing Record to start the backup from must not be null",
        TraceId);

    return previousIndexingRecord.RawPtr();
}

ktl::Awaitable<ReplicatorBackup> BackupManager::BackupReplicatorAsync(
    __in Common::Guid backupId,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in KString const & replicatorBackupFolder,
    __in Data::Utilities::IAsyncEnumerator<LogRecord::SPtr> & logRecordsAsyncEnumerator,
    __in CancellationToken const & cancellationToken)
{
    // Port Note: Difference with managed
    // We do not need to verify that call back is not nullptr.
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    
    KWString backupLogFilePath(GetThisAllocator(), replicatorBackupFolder.ToUNICODE_STRING());

    BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(*PartitionedReplicaIdentifier, backupLogFilePath, GetThisAllocator());
    Stopwatch stopWatch;

    BackupLogRecord::CSPtr lastCompletedBackupLogRecord = lastCompletedBackupLogRecordThreadSafeCache_.Get();

    status = StartDrainableBackupPhase();
    if (NT_SUCCESS(status) == false)
    {
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            status,
            L"Could not acquire the api lock.");

        throw ktl::Exception(status);
    }

    {
        KFinally([&] { CompleteDrainableBackupPhase(); });

        // Port Note: Difference with managed
        // We do not need to dispose the logRecordsAsyncEnumerator since it is disposed above.
        {
            if (backupOption == FABRIC_BACKUP_OPTION_FULL)
            {
                co_await backupLogFileSPtr->WriteAsync(
                    logRecordsAsyncEnumerator, 
                    *lastCompletedBackupLogRecord,
                    cancellationToken);
            }
            else if (backupOption == FABRIC_BACKUP_OPTION_INCREMENTAL)
            {
                IncrementalBackupLogRecordsAsyncEnumerator::SPtr incrementalBackupEnumeratorSPtr = IncrementalBackupLogRecordsAsyncEnumerator::Create(
                    logRecordsAsyncEnumerator, 
                    *lastCompletedBackupLogRecord,
                    *replicatedLogManagerSPtr_, 
                    GetThisAllocator());

                KFinally([&] {incrementalBackupEnumeratorSPtr->Dispose(); });

                co_await backupLogFileSPtr->WriteAsync(
                    *incrementalBackupEnumeratorSPtr,
                    *lastCompletedBackupLogRecord,
                    cancellationToken);
            }
        }
    }

    LONG64 accumulatedSize = backupOption == FABRIC_BACKUP_OPTION_FULL ? 
        backupLogFileSPtr->Size :
        (lastCompletedBackupLogRecord->BackupLogSize * NumberOfBytesInKB) + backupLogFileSPtr->Size;

    co_return ReplicatorBackup(
        backupLogFileSPtr->Count,
        backupLogFileSPtr->Size,
        accumulatedSize,
        backupLogFileSPtr->IndexingRecordEpoch.GetFabricEpoch(),
        backupLogFileSPtr->IndexingRecordLSN,
        backupLogFileSPtr->LastBackedUpEpoch.GetFabricEpoch(),
        backupLogFileSPtr->LastBackedUpLSN);
}

// Find the physical log record before the first log record to be incrementally backed up.
//
// Port Note: (Managed difference) In managed we re-use the code for getting this starting from the last completed checkpoint (like full backup).
// However, if incremental backups are coming in at high rate that can be much earlier.
// This could cause unnecessary IO reading the ahead in the log until we find the log record to backup.
LogRecord::CSPtr BackupManager::FindFirstLogRecordForIncrementalBackup(
    __in FABRIC_SEQUENCE_NUMBER highestBackedUpLSN,
    __in Common::Guid const & backupId)
{
    PhysicalLogRecord::SPtr previousPhysicalRecord(logManagerSPtr_->CurrentLogTailRecord->PreviousPhysicalRecord);
    ASSERT_IF(LogRecord::IsInvalid(previousPhysicalRecord.RawPtr()), "{0}: Last physical log record cannot be invalid.", TraceId);

    // Search back for the last physical log record that is less than the highestBackedupLSN.
    while (previousPhysicalRecord->Lsn >= highestBackedUpLSN)
    {
        previousPhysicalRecord = previousPhysicalRecord->PreviousPhysicalRecord;
        if (LogRecord::IsInvalid(previousPhysicalRecord.RawPtr()))
        {
            EventSource::Events->IBM_BackupAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                backupId,
                L"status",
                SF_STATUS_MISSING_FULL_BACKUP,
                wformatString(
                    L"Physical record before the last highest backed up record is already truncated ({0})", highestBackedUpLSN));

            throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
        }
    }

    return previousPhysicalRecord.RawPtr();
}

void BackupManager::ThrowIfAccumulatedIncrementalBackupLogsIsTooLarge(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::Guid const & backupId,
    __in ReplicatorBackup const & replicatorBackup)
{
    if (backupOption != FABRIC_BACKUP_OPTION_FULL)
    {
        return;
    }

    LONG64 snapOfMaxStreamSizeInMB = transactionalReplicatorConfig_->MaxStreamSizeInMB; 
    if (replicatorBackup.AccumulatedLogSizeInMB >= snapOfMaxStreamSizeInMB)
    {
        // TODO: We could add a more specific error code.
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            SF_STATUS_MISSING_FULL_BACKUP,
            wformatString(
                L"Accumulated log size in MB ({0}) would be higher than MaxStreamSizeInMB ({1}).",
                replicatorBackup.AccumulatedLogSizeInMB,
                snapOfMaxStreamSizeInMB));

        throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
    }

    LONG64 maxAccumulatedBackupLogSizeInMB = transactionalReplicatorConfig_->MaxAccumulatedBackupLogSizeInMB;
    if (replicatorBackup.AccumulatedLogSizeInMB >= maxAccumulatedBackupLogSizeInMB)
    {
        // TODO: We could add a more specific error code.
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            backupId,
            L"status",
            SF_STATUS_MISSING_FULL_BACKUP,
            wformatString(
                L"Accumulated log size in MB ({0}) would be higher than MaxAccumulatedBackupLogSizeInMB ({1}).",
                replicatorBackup.AccumulatedLogSizeInMB,
                maxAccumulatedBackupLogSizeInMB));

        throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
    }

    return;
}

//
// Summary: Writes the backup metadata file.
// 
Awaitable<NTSTATUS> BackupManager::WriteBackupMetadataFileAsync(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::Guid backupId,
    __in Common::Guid parentBackupId,
    __in ReplicatorBackup const & replicatorBackup,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = StartDrainableBackupPhase();
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"WriteBackupMetadataFileAsync: StartDrainableBackupPhase failed", backupId, status);
    KFinally([&] { CompleteDrainableBackupPhase(); });

    KWString metadataPath = backupOption == FABRIC_BACKUP_OPTION_FULL ?
        KWString(GetThisAllocator(), *fullMetadataFilePathCSPtr_) :
        KWString(GetThisAllocator(), *incrementalMetadataFilePathCSPtr_);

    BackupMetadataFile::SPtr backupMetadataSPtr = nullptr;
    status = BackupMetadataFile::Create(
        *PartitionedReplicaIdentifier,
        metadataPath,
        GetThisAllocator(),
        backupMetadataSPtr);
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"WriteBackupMetadataFileAsync: BackupMetadataFile::Create failed", backupId, status);

    status = co_await backupMetadataSPtr->WriteAsync(
        backupOption,
        KGuid(parentBackupId.AsGUID()),
        KGuid(backupId.AsGUID()),
        PartitionId,
        ReplicaId,
        replicatorBackup.EpochOfFirstBackedUpLogRecord,
        replicatorBackup.LsnOfFirstLogicalLogRecord,
        replicatorBackup.EpochOfHighestBackedUpLogRecord,
        replicatorBackup.LsnOfHighestBackedUpLogRecord,
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    co_return status;
}

//
// Summary: Calls the backup callback under drainable and traces any failure.
//
// TODO: We need to use api monitor here.
Awaitable<NTSTATUS> BackupManager::CallBackupCallbackAsync(
    __in Common::Guid const & backupId,
    __in TxnReplicator::BackupInfo const & backupInfo,
    __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = StartDrainableBackupPhase();
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"CallBackupCallbackAsync: StartDrainableBackupPhase failed", backupId, status);

    KFinally([&] { CompleteDrainableBackupPhase(); });

    bool backupUploaded = false;
    try
    {
        backupUploaded = co_await backupCallbackHandler.UploadBackupAsync(backupInfo, cancellationToken);
    }
    catch (Exception const & e)
    {
        wstring message = Common::wformatString(
            L"{0}:{1} {2}: User API UploadBackupAsync failed with {3}", 
            TracePartitionId, 
            ReplicaId, 
            TraceComponent, 
            e.GetStatus());
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(message, backupId, e.GetStatus());
    }

    if (backupUploaded == false)
    {
        wstring message = Common::wformatString(
            L"{0}:{1} {2}: User API UploadBackupAsync failed to upload and returned false.", 
            TracePartitionId, 
            ReplicaId, 
            TraceComponent);
        BM_TRACE_AND_CO_RETURN(message, backupId, SF_STATUS_INVALID_OPERATION);
    }

    co_return status;
}

// Replicated a backup log record.
//
// Port Note: (Managed difference) This has been moved in to the backup manager.
Awaitable<NTSTATUS> BackupManager::ReplicateBackupLogRecordAsync(
    __in Common::Guid const & backupId,
    __in ReplicatorBackup const & replicatorBackup) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    TxnReplicator::Epoch highestBackedUpEpoch(replicatorBackup.EpochOfHighestBackedUpLogRecord);

    BackupLogRecord::SPtr backupLogRecord = BackupLogRecord::Create(
        KGuid(backupId.AsGUID()),
        highestBackedUpEpoch,
        replicatorBackup.LsnOfHighestBackedUpLogRecord,
        static_cast<UINT>(replicatorBackup.LogCount),
        static_cast<UINT>(replicatorBackup.AccumulatedLogSizeInKB),
        *invalidLogRecordsSPtr_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    // TODO: Can return error
    // MCoskun: Order of the following operations is important.
    // First, we need to make sure backupLogRecord is before the barrier log record that is inserted below
    // Second, we need to insert a barrier log record, flush and await replication.
    // Third, we need to await the replication and flush of backuplogRecord (can complete out of order) and call unlock.
    LONG64 bufferedBytes = 0;
    status = replicatedLogManagerSPtr_->ReplicateAndLog(*backupLogRecord, bufferedBytes);
    CO_RETURN_ON_FAILURE(status);

    // Unlock must be called even if barrier replication failed. Otherwise close can get stuck.
    status = co_await ReplicateAndAwaitBarrierLogRecordAsync(backupId);    
    if (NT_SUCCESS(status))
    {
        status = co_await UnlockOnceApplyAndReplicationCompletesAsync(*backupLogRecord);
    }
    else
    {
        co_await UnlockOnceApplyAndReplicationCompletesAsync(*backupLogRecord);
    }

    co_return status;
}

Awaitable<NTSTATUS> BackupManager::ReplicateAndAwaitBarrierLogRecordAsync(
    __in Common::Guid const & backupId) noexcept
{
    KShared$ApiEntry();

    ICheckpointManager::SPtr checkpointManager = GetCheckpointManager();
    BarrierLogRecord::SPtr barrierRecordSPtr;
    int retryCount = 0;

    while (true)
    {
        NTSTATUS status = checkpointManager->ReplicateBarrier(barrierRecordSPtr);
        if (NT_SUCCESS(status))
        {
            break;
        }

        if (status == SF_STATUS_RECONFIGURATION_PENDING)
        {
            NTSTATUS timerStatus = co_await KTimer::StartTimerAsync(
                GetThisAllocator(),
                GetThisAllocationTag(),
                StartingBackoffInMilliSeconds * (++retryCount),
                nullptr);

            if (NT_SUCCESS(timerStatus) == false)
            {
                // If create timer failed, just return SF_STATUS_RECONFIGURATION_PENDING.
                CO_RETURN_ON_FAILURE(status);
            }

            if (retryCount % 4 == 0)
            {
                EventSource::Events->IBM_BackupAsyncFinishError(
                    TracePartitionId,
                    ReplicaId,
                    backupId,
                    L"ErrorCode",
                    SF_STATUS_RECONFIGURATION_PENDING,
                    L"ReplicateAndAwaitBarrierLogRecordAsync: Replicate failed since reconfiguration, will retry until timeout");
            }

            continue;
        }

        CO_RETURN_ON_FAILURE(status);
    }

    co_await logManagerSPtr_->FlushAsync(KStringView(backupId.ToString().c_str()));

    co_await barrierRecordSPtr->AwaitProcessing();
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> BackupManager::UnlockOnceApplyAndReplicationCompletesAsync(
    __in BackupLogRecord & backupLogRecord) noexcept
{
    IOperationProcessor::SPtr operationProcessor = GetOperationProcessor();

    NTSTATUS status1 = co_await backupLogRecord.AwaitApply();
    NTSTATUS status2 = co_await backupLogRecord.AwaitReplication();

    operationProcessor->Unlock(backupLogRecord);
    co_return (status1 || status2);
}

Awaitable<NTSTATUS> BackupManager::RecoverRestoreDataAsync(
    __in TxnReplicator::IBackupRestoreProvider & loggingReplicator,
    __in Common::Guid & restoreId,
    __in BackupFolderInfo const & backupFolderInfo) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    TxnReplicator::RecoveryInformation recoveryInformation;
    status = co_await loggingReplicator.OpenAsync(&backupFolderInfo, recoveryInformation);
    CO_RETURN_ON_FAILURE(status);

    if (recoveryInformation.ShouldSkipRecoveryDueToIncompleteChangeRoleNone)
    {
        // TODO: Trace.
        co_return SF_STATUS_INVALID_OPERATION;
    }

    if (testFlagFailRestoreData_)
    {
        // TODO; Trace
        co_return SF_STATUS_COMMUNICATION_ERROR;
    }

    status = co_await loggingReplicator.PerformRecoveryAsync(recoveryInformation);
    co_return status;
}

Awaitable<NTSTATUS> BackupManager::FinishRestoreAsync(
    __in TxnReplicator::IBackupRestoreProvider & loggingReplicator,
    __in TxnReplicator::Epoch & dataLossEpoch) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    co_await loggingReplicator.PerformUpdateEpochAsync(dataLossEpoch, replicatedLogManagerSPtr_->CurrentLogTailLsn);
    replicatedLogManagerSPtr_->Information(InformationEvent::RestoredFromBackup);

    co_await logManagerSPtr_->FlushAsync(L"FinishRestoreAsync");
    status = co_await replicatedLogManagerSPtr_->LastInformationRecord->AwaitProcessing();
    CO_RETURN_ON_FAILURE(status);

    // TODO: Await records processor.

    co_return status;
}

bool BackupManager::CheckIfRestoreTokenExists()
{
    wstring restorePath(*restoreTokenFilePathCSPtr_);
    return File::Exists(restorePath);
}

ktl::Awaitable<void> BackupManager::CreateRestoreTokenAsync(
    __in Common::Guid const & id)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    wstring restoreTokenPath(*restoreTokenFolderPathCSPtr_);

    if (Directory::Exists(restoreTokenPath) == false)
    {
        ErrorCode errorCode = Directory::Create2(restoreTokenPath);
        if (errorCode.IsSuccess() == false)
        {
            status = StatusConverter::Convert(errorCode.ToHResult());
            EventSource::Events->IBM_RestoreAsyncFinishError(
                TracePartitionId,
                ReplicaId,
                id,
                status,
                wformatString(
                    L"Failed to create restore token folder {0}",
                    restoreTokenPath));

            throw Exception(status);
        }
    }

    bool restoreTokenExists(CheckIfRestoreTokenExists());
    ASSERT_IF(
        restoreTokenExists == true,
        "{0}:{1} Restore Id: {2} Restore token already exists: {3}",
        TracePartitionId,
        ReplicaId,
        id,
        wstring(*restoreTokenFilePathCSPtr_));

    KWString tokenPath(GetThisAllocator(), *restoreTokenFilePathCSPtr_);
    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        tokenPath,
        FALSE,
        KBlockFile::eCreateAlways,
        KBlockFile::eInheritFileSecurity,
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    if (NT_SUCCESS(status) == false)
    {
        EventSource::Events->IBM_RestoreAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            id,
            status, 
            wformatString(
                L"Failed to create restore token. Token path: {0}",
                ToStringLiteral(*restoreTokenFilePathCSPtr_)));

        throw Exception(status);
    }

    // Here a KFinally is not actually required. This is for future proofing the function.
    KFinally([&] {fileSPtr->Close(); });

    EventSource::Events->RestoreTokenCreated(
        TracePartitionId,
        ReplicaId,
        id,
        ToStringLiteral(*restoreTokenFilePathCSPtr_));
}

void BackupManager::DeleteRestoreToken(
    __in Common::Guid const & id)
{
    if (testFlagNoOpDeleteRestoreToken_)
    {
        EventSource::Events->BackupRestoreWarning(
            TracePartitionId,
            ReplicaId,
            false,
            id,
            wformatString(
                L"Nooping deleting restore token since testFlagNoOpDeleteRestoreToken_ is on. {0}",
                ToStringLiteral(*restoreTokenFilePathCSPtr_)));

        return;
    }

    ASSERT_IFNOT(
        CheckIfRestoreTokenExists() == true, 
        "{0}: RestoreId: {1}. Restore token must exist {2}",
        TraceId,
        id.ToString(),
        ToStringLiteral(*restoreTokenFilePathCSPtr_));

    wstring restorePath(*restoreTokenFilePathCSPtr_);
    ErrorCode errorCode = File::Delete2(restorePath, true);
    if (errorCode.IsSuccess() == false)
    {
        NTSTATUS status = StatusConverter::Convert(errorCode.ToHResult());
        EventSource::Events->IBM_RestoreAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            id,
            status,
            wformatString(
                L"Failed to delete the Restore Token {0}",
                ToStringLiteral(*restoreTokenFilePathCSPtr_)));

        throw Exception(status);
    }

    EventSource::Events->RestoreTokenDeleted(
        TracePartitionId,
        ReplicaId,
        id,
        ToStringLiteral(*restoreTokenFilePathCSPtr_));
}

ktl::Awaitable<void> BackupManager::AcquireEntryLock(
    __in Common::Guid const & id,
    __in Common::TimeSpan timeout,
    __in LockRequester requester)
{
    bool isAcquired = false;
    try
    {
        isAcquired = co_await backupAndRestoreLock_->AcquireWriteLockAsync(static_cast<ULONG32>(timeout.TotalMilliseconds()));
    }
    catch (Exception const &)
    {
        // Note: Ignore the exception here to keep the same behavior as before.
        // In the Backup case throw SF_STATUS_BACKUP_IN_PROGRESS, in the restore case throw SF_STATUS_INVALID_OPERATION.
    }

    if (isAcquired == false)
    {
        // TODO: Consider adding SF_STATUS_BACKUP_OR_RESTORE_IN_PROGRESS
        NTSTATUS status = requester == LockRequester::Backup ? SF_STATUS_BACKUP_IN_PROGRESS : SF_STATUS_INVALID_OPERATION;
        
        EventSource::Events->IBM_BackupAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            id,
            L"status",
            status,
            L"AcquireEntryLock failed");

        throw ktl::Exception(status);
    }
}

void BackupManager::ReleaseEntryLock()
{
    backupAndRestoreLock_->ReleaseWriteLock();
}

NTSTATUS BackupManager::StartDrainableBackupPhase() noexcept
{
    K_LOCK_BLOCK(backupDrainLock_)
    {
        if (isBackupDrainablePhaseAllowed_ == false)
        {
            return SF_STATUS_NOT_PRIMARY;
        }

        ASSERT_IFNOT(
            isBackupPhaseInflight_ == false,
            "{0}: backup could not be inflight.",
            TraceId);

        isBackupPhaseInflight_ = true;
    }

    return STATUS_SUCCESS;
}

void BackupManager::CompleteDrainableBackupPhase() noexcept
{
    K_LOCK_BLOCK(backupDrainLock_)
    {
        ASSERT_IFNOT(
            isBackupPhaseInflight_ == true,
            "{0}: Backup Callback must be in - flight if this is called.",
            TraceId);

        isBackupPhaseInflight_ = false;

        if (drainBackupCallBackAcsSPtr_ != nullptr)
        {
            ASSERT_IFNOT(
                isBackupDrainablePhaseAllowed_ == false,
                "{0}: Backup must not be allowed if draining is initiated.",
                TraceId);

            drainBackupCallBackAcsSPtr_->Set();
            drainBackupCallBackAcsSPtr_ = nullptr;
        }
    }
}

void BackupManager::ThrowIfOnDataLossIsNotInflight()
{
    if (isOnDataLossInProgress_ == false)
    {
        // TODO: We should make the SF_STATUS_ONDATALOSS_NOT_INFLIGHT.
        throw Exception(SF_STATUS_INVALID_OPERATION);
    }
}

void BackupManager::ThrowIfInvalidState(
    __in Common::Guid const & restoreId)
{
    bool isTokenExists = CheckIfRestoreTokenExists();
    if (isTokenExists == true)
    {
        EventSource::Events->IBM_RestoreAsyncFinishError(
            TracePartitionId,
            ReplicaId,
            restoreId,
            SF_STATUS_INVALID_OPERATION,
            L"A non-retry-able RestoreAsync exception has been retried.");

        throw Exception(SF_STATUS_INVALID_OPERATION);
    }
}

void BackupManager::ThrowIfNotSafe(
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in BackupFolderInfo const & backupFolderInfo,
    __in Common::Guid const & restoreId)
{
    if (restorePolicy != FABRIC_RESTORE_POLICY_SAFE)
    {
        return;
    }

    FABRIC_SEQUENCE_NUMBER currentLogTailLsn = replicatedLogManagerSPtr_->CurrentLogTailLsn;
    Epoch currentTailEpoch = replicatedLogManagerSPtr_->GetEpoch(currentLogTailLsn);

    if (currentTailEpoch < backupFolderInfo.HighestBackedupEpoch)
    {
        return;
    }

    if ((currentTailEpoch == backupFolderInfo.HighestBackedupEpoch) && currentLogTailLsn < backupFolderInfo.HighestBackedupLSN)
    {
        return;
    }

    EventSource::Events->IBM_RestoreAsyncFinishError(
        TracePartitionId,
        ReplicaId,
        restoreId,
        STATUS_INVALID_PARAMETER,
        wformatString(
            L"Restore failed safety check. Current: <{0},{1}> {2}. Restore Folder: <{3},{4}> {5}.",
            currentTailEpoch.DataLossVersion,
            currentTailEpoch.ConfigurationVersion,
            currentLogTailLsn,
            backupFolderInfo.HighestBackedupEpoch.DataLossVersion,
            backupFolderInfo.HighestBackedupEpoch.ConfigurationVersion,
            backupFolderInfo.HighestBackedupLSN));

    // TODO: Consider adding SF_STATUS_RESTORE_NOT_SAFE error code.
    throw Exception(STATUS_INVALID_PARAMETER);
}

IBackupRestoreProvider::SPtr BackupManager::GetLoggingReplicator()
{
    TxnReplicator::IBackupRestoreProvider::SPtr backupRestoreProviderSPtr = backupRestoreProviderWRef_->TryGetTarget();
    ASSERT_IFNOT(backupRestoreProviderSPtr != nullptr, "{0}: Backup restore provider has been released.", TraceId);
    return Ktl::Move(backupRestoreProviderSPtr);
}

ICheckpointManager::SPtr BackupManager::GetCheckpointManager()
{
    ICheckpointManager::SPtr checkpointManagerSPtr = checkpointManagerWRef_->TryGetTarget();
    ASSERT_IFNOT(checkpointManagerSPtr != nullptr, "{0}: Checkpoint Manager has been released.", TraceId);
    return Ktl::Move(checkpointManagerSPtr);
}

IOperationProcessor::SPtr BackupManager::GetOperationProcessor()
{
    IOperationProcessor::SPtr operationProcssorSPtr = operationProcessorWRef_->TryGetTarget();
    ASSERT_IFNOT(operationProcssorSPtr != nullptr, "{0}: Operation Processor has been released.", TraceId);
    return Ktl::Move(operationProcssorSPtr);
}

void BackupManager::TraceException(
    __in Common::Guid const & backupId,
    __in std::wstring const & message,
    __in Exception & exception) const
{
    KDynStringA stackString(GetThisAllocator());
    bool getStack = exception.ToString(stackString);
    ASSERT_IFNOT(getStack, "{0}: KDynStringA allocation failed in TraceException", TraceId);

    EventSource::Events->IBM_BackupAsyncFinishError(
        TracePartitionId,
        ReplicaId,
        backupId,
        L"Status",
        exception.GetStatus(),
        message);
}

BackupManager::BackupManager(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KString const & workFolder,
    __in TxnReplicator::IStateProviderManager & stateManager,
    __in IReplicatedLogManager & replicatedLogManager,
    __in ILogManager & logManager,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in InvalidLogRecords & invalidLogRecords)
    : KShared()
    , KObject()
    , IBackupManager()
    , PartitionedReplicaTraceComponent(traceId)
    , stateManagerSPtr_(&stateManager)
    , replicatedLogManagerSPtr_(&replicatedLogManager)
    , logManagerSPtr_(&logManager)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , healthClient_(healthClient)
    , invalidLogRecordsSPtr_(&invalidLogRecords)
    , backupFolderCSPtr_(CreateBackupFolderPath(workFolder, traceId.PartitionId, traceId.ReplicaId, GetThisAllocator()))
    , lrBackupFolderCSPtr_(KPath::Combine(*backupFolderCSPtr_, ReplicatorBackupFolderName, GetThisAllocator()).RawPtr())
    , replicatorLogBackupFilePathCSPtr_(KPath::Combine(*lrBackupFolderCSPtr_, ReplicatorBackupLogName, GetThisAllocator()).RawPtr())
    , smBackupFolderCSPtr_(KPath::Combine(*backupFolderCSPtr_, StateManagerBackupFolderName, GetThisAllocator()).RawPtr())
    , fullMetadataFilePathCSPtr_(KPath::Combine(*backupFolderCSPtr_, FullMetadataFileName, GetThisAllocator()).RawPtr())
    , incrementalMetadataFilePathCSPtr_(KPath::Combine(*backupFolderCSPtr_, IncrementalMetadataFileName, GetThisAllocator()).RawPtr())
    , restoreTokenFolderPathCSPtr_(CreateRestoreFolderPath(workFolder, traceId.PartitionId, traceId.ReplicaId, GetThisAllocator()))
    , restoreTokenFilePathCSPtr_(KPath::Combine(*restoreTokenFolderPathCSPtr_, RestoreTokenName, GetThisAllocator()).RawPtr())
    , lastCompletedBackupLogRecordThreadSafeCache_(BackupLogRecord::CreateZeroBackupLogRecord(*(invalidLogRecords.Inv_PhysicalLogRecord), GetThisAllocator()).RawPtr())
{
    NTSTATUS status = ReaderWriterAsyncLock::Create(GetThisAllocator(), BACKUP_MANAGER_TAG, backupAndRestoreLock_);
    SetConstructorStatus(status);

    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"BackupManager",
        reinterpret_cast<uintptr_t>(this));
}

BackupManager::~BackupManager()
{
    backupAndRestoreLock_->Close();
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"BackupManager",
        reinterpret_cast<uintptr_t>(this));
}
