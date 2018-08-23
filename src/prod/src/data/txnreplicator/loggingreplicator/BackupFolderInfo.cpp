// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Common;
using namespace Data::Utilities;
using namespace ktl;

Common::StringLiteral const TraceComponent("BackupFolderInfo");

BackupFolderInfo::SPtr BackupFolderInfo::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KGuid const & id,
    __in KString const & backupFolder,
    __in KAllocator & allocator,
    __in bool test_doNotVerifyLogFile)
{
    BackupFolderInfo * pointer = _new(BACKUP_FOLDER_INFO_TAG, allocator) BackupFolderInfo(
        traceId,
        id,
        backupFolder,
        test_doNotVerifyLogFile);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    THROW_ON_FAILURE(pointer->Status());
    return BackupFolderInfo::SPtr(pointer);
}

TxnReplicator::Epoch BackupFolderInfo::get_HighestBackedupEpoch() const
{
    ULONG count = metadataArray_.Count();
    ASSERT_IFNOT(count >= 1, "{0}: Number of backup folders must be greater than or equal to 1", TraceId);

    ULONG lastIndex = count - 1;
    BackupMetadataFile::CSPtr lastMetadata = metadataArray_[lastIndex];

    return lastMetadata->BackupEpoch;
}

FABRIC_SEQUENCE_NUMBER BackupFolderInfo::get_HighestBackedupLSN() const
{
    ULONG count = metadataArray_.Count();
    ASSERT_IFNOT(count >= 1, "{0}: Number of backup folders must be greater than or equal to 1", TraceId);

    ULONG lastIndex = count - 1;
    BackupMetadataFile::CSPtr lastMetadata = metadataArray_[lastIndex];

    return lastMetadata->BackupLSN;
}

KArray<BackupMetadataFile::CSPtr> const & BackupFolderInfo::get_BackupMetadataArray() const
{
    return metadataArray_;
}

KString::CSPtr BackupFolderInfo::get_StateManagerBackupFolderPath() const
{
    return stateManagerBackupFolderPath_;
}

KArray<KString::CSPtr> const & BackupFolderInfo::get_LogPathList() const
{
    return logFilePathArray_;
}

KString::SPtr BackupFolderInfo::get_FullBackupFolderPath() const
{
    return fullBackupFolderPath_;
}

// Algorithm:
// 1. Scan the folder to find all backups and create corresponding sorted arrays.
// 2. Trim the chain so that it only contains the longest chain.
// 3. Verify whether or not the input folder is restore-able.
ktl::Awaitable<void> BackupFolderInfo::AnalyzeAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    wstring backupFolderPath(*backupFolderCSPtr_);

    {
        wstring fullMetadataFileName(BackupManager::FullMetadataFileName);
        StringCollection allFiles = Directory::GetFiles(backupFolderPath, fullMetadataFileName, true, false);
        if (allFiles.size() == 0)
        {
            EventSource::Events->BackupFolderWarning(
                TracePartitionId,
                ReplicaId,
                L"Full backup cound not be found.",
                backupFolderPath,
                SF_STATUS_MISSING_FULL_BACKUP);

            throw Exception(SF_STATUS_MISSING_FULL_BACKUP);
        }

        if (allFiles.size() > 1)
        {
            EventSource::Events->BackupFolderWarning(
                TracePartitionId,
                ReplicaId,
                L"Too many full backups found.",
                backupFolderPath,
                SF_STATUS_INVALID_OPERATION);

            throw Exception(SF_STATUS_INVALID_OPERATION);
        }

        wstring fullBackupDirectoryName = Path::GetDirectoryName(allFiles[0]);
        co_await AddFolderAsync(
            FABRIC_BACKUP_OPTION_FULL, 
            fullBackupDirectoryName, 
            allFiles[0], 
            cancellationToken);
    }

    {
        wstring incrementalMetadataFileName(BackupManager::IncrementalMetadataFileName);
        StringCollection incrementalMetadataFiles = Directory::GetFiles(backupFolderPath, incrementalMetadataFileName, true, false);

        for (wstring incrementalPath : incrementalMetadataFiles)
        {
            wstring incrementalBackupDirectory = Path::GetDirectoryName(incrementalPath);
            co_await AddFolderAsync(
                FABRIC_BACKUP_OPTION_INCREMENTAL, 
                incrementalBackupDirectory, 
                incrementalPath, 
                cancellationToken);
        }
    }

    Trim();

    co_await VerifyAsync(cancellationToken);

    co_return;
}

// Summary: Adds a folder into the BackupFolderInfo.
// 
// Port Note: Merged incremental and full to reduce code.
// Port Note: Upper layer passes the directory as well as the metadata file path to avoid recomputation.
// Note: Replicate backup log record is the following step of user callback, it may throw transient exception and leads
// to user re-try the BackupAsync call. In this case, two same backups may be uploaded to the same folder, so we allow
// two same keys(BackupVersion) added to the array.
ktl::Awaitable<void> BackupFolderInfo::AddFolderAsync(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in wstring const & backupDirectoryPath,
    __in wstring const & backupMetadataFilePath,
    __in ktl::CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (backupOption == FABRIC_BACKUP_OPTION_FULL)
    {
        ASSERT_IFNOT(stateManagerBackupFolderPath_ == nullptr, "{0}: There can only be one full backup.", TraceId);
        ASSERT_IFNOT(File::Exists(backupMetadataFilePath), "{0}: Metadata must exist.", TraceId);

        wstring smBackupFolderName(BackupManager::StateManagerBackupFolderName);
        wstring stateManagerBackupFolderPath = Path::Combine(
            backupDirectoryPath,
            smBackupFolderName);

        bool smFolderExists = Directory::Exists(stateManagerBackupFolderPath);
        if (smFolderExists == false)
        {
            EventSource::Events->BackupFolderWarning(
                TracePartitionId,
                ReplicaId,
                L"Full backup state manager folder could not be found.",
                stateManagerBackupFolderPath,
                STATUS_INVALID_PARAMETER);

            // TODO: This exception needs to be considered.
            // Port Note: This is ArgumentException in managed.
            throw Exception(STATUS_INVALID_PARAMETER);
        }

        // Set the full backup information: backup folder path and sm folder path.
        status = KString::Create(fullBackupFolderPath_, GetThisAllocator(), backupDirectoryPath.c_str());
        ThrowIfNecessary(status, L"AddFolderAsync: KString::Create for full backup folder path.");

        {
            KString::SPtr stateManagerBackupFolderPathSPtr;
            status = KString::Create(stateManagerBackupFolderPathSPtr, GetThisAllocator(), stateManagerBackupFolderPath.c_str());
            ThrowIfNecessary(status, L"AddFolderAsync: KString::Create for sm folder path.");

            stateManagerBackupFolderPath_ = stateManagerBackupFolderPathSPtr.RawPtr();
        }
    }
    else 
    {
        ASSERT_IFNOT(backupOption == FABRIC_BACKUP_OPTION_INCREMENTAL, "{0}: Invalid backup option. {1}", TraceId, static_cast<int>(backupOption));
        ASSERT_IFNOT(stateManagerBackupFolderPath_ != nullptr, "{0}: There must at least be one full backup.Full backup must be added first.", TraceId);
    }

    KWString metadataFilePath(GetThisAllocator(), backupMetadataFilePath.c_str());
    BackupMetadataFile::SPtr backupMetadataFile = nullptr;
    status = BackupMetadataFile::Create(
        *PartitionedReplicaIdentifier, 
        metadataFilePath, 
        GetThisAllocator(),
        backupMetadataFile);
    THROW_ON_FAILURE(status);

    status = co_await backupMetadataFile->ReadAsync(id_, CancellationToken::None);
    THROW_ON_FAILURE(status);

    BackupVersion version(backupMetadataFile->BackupEpoch.GetFabricEpoch(), backupMetadataFile->BackupLSN);

    // Note: Binary search for empty array (full backup case) returns immediately.
    LONG32 index = Sort<BackupVersion>::BinarySearch(version, true, BackupVersion::Compare, backupVersionArray_);

    // If item is found, the right position will be returned, otherwise, a negative number that 
    // is the bitwise complement of the index of the next element which is larger than the item or, if there is no
    // larger element, the bitwise complement of list count.
    LONG32 insertPos = index < 0 ? ~index : index;

    // Port Note: Instead of using two SortedList, I use one array for the key and two arrays for the two types of values.
    // This reduces the number of arrays. In future, this could be turned into a data structure.
    // Insert version to version list.
    status = backupVersionArray_.InsertAt(insertPos, version);
    ThrowIfNecessary(status, L"AddFolderAsync: Version list insert.");

    // Insert the metadata in the metadata list in the corresponding section.
    status = metadataArray_.InsertAt(insertPos, backupMetadataFile.RawPtr());
    ThrowIfNecessary(status, L"AddFolderAsync: Metadata list insert.");

    KString::SPtr logFilePath = nullptr;
    status = KString::Create(logFilePath, GetThisAllocator(), backupDirectoryPath.c_str());
    ThrowIfNecessary(status, L"AddFolderAsync: String creation for log path.");

    KPath::CombineInPlace(*logFilePath, BackupManager::ReplicatorBackupFolderName);
    KPath::CombineInPlace(*logFilePath, BackupManager::ReplicatorBackupLogName);

    // Insert the path in the log path list in the corresponding section.
    status = logFilePathArray_.InsertAt(insertPos, logFilePath.RawPtr());
    ThrowIfNecessary(status, L"AddFolderAsync: log file list insert.");

    co_return;
}

/// <summary>
/// Function that trims the backup chain to only include the longest chain (removes all unnecessary divergent links).
/// This is to be able to turn on backup stitching across replicas without requiring customer code change.
/// As such it only handles issues in the backup chain that can be caused by stitching.
/// Specifically a shared link can be present in a backup because a replica uploaded the backup folder but failed to 
/// log/replicate the backup log record.
/// 
/// Invariants (for all backups)
/// 1. A backup folder customer has uploaded, can never have false progress.
/// 2. All backup data loss numbers across all links in the chain must be same.
/// 3. First chain in a valid backup chain must start with a full backup and can contain only one full backup.
/// 
/// We can only trim when the input chain obeys the following rules
/// 1. There can be multiple divergent chains from each divergence point, but only one of the divergent chains can have more than one link.
/// 
/// </summary>
/// <devnote>
/// Terminology
/// Backup Link:        A backup folder that participates in a backup chain.
/// Backup Chain:       A set of backup folders that start with a full backup and set of incremental backups that each build on the previous backup.
/// Divergent Links:    One or more Backup Links that have the same parent.
/// Shared Link:        A backup link that is parent of multiple backup links.
/// </devnote>
void BackupFolderInfo::Trim()
{
    ASSERT_IFNOT(
        backupVersionArray_.Count() != 0 && backupVersionArray_.Count() == metadataArray_.Count() && backupVersionArray_.Count() == logFilePathArray_.Count(),
        "{0}: backupVersionArray_.Count() {1} metadataArray_.Count() {2} logFilePathArray_.Count() {3}, Backup folder: {4}",
        TraceId,
        backupVersionArray_.Count(),
        metadataArray_.Count(),
        logFilePathArray_.Count(),
        backupFolderCSPtr_->operator LPCWSTR());

    LoggingReplicator::BackupMetadataFile::CSPtr headMetadataFile = metadataArray_[0];
    if (headMetadataFile->BackupOption != FABRIC_BACKUP_OPTION_FULL)
    {
        LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
            ToStringLiteral(
                wformatString(
                    L"Folder: {0} Beginning of the backup chain must be full backup. BackupId: {1} ParentId: {2} DL: {3} CN: {4} LSN: {5}",
                    backupFolderCSPtr_->operator LPCWSTR(),
                    Common::Guid(headMetadataFile->BackupId),
                    Common::Guid(headMetadataFile->ParentBackupId),
                    headMetadataFile->BackupEpoch.DataLossVersion, 
                    headMetadataFile->BackupEpoch.ConfigurationVersion,
                    headMetadataFile->BackupLSN)),
            STATUS_INVALID_PARAMETER);

        throw Exception(STATUS_INVALID_PARAMETER);
    }

    // If there are less than two backup folders, there is no chain to trim.
    if (metadataArray_.Count() < 2)
    {
        return;
    }

    // The last backup in the sorted list must be the one with the newest state since it is sorted using the BackupVersion.
    LoggingReplicator::BackupMetadataFile::CSPtr currentMetadataFile = metadataArray_[metadataArray_.Count() - 1];
    LONG32 nextIndex = metadataArray_.Count() - 2;

    // Go back through the chain and remove an divergent branches.
    // Note that head (0) is handled specially because instead of removing we throw an exception:
    // Head which is suppose to be the full backup cannot be trimmed.
    while (nextIndex >= 0)
    {
        LoggingReplicator::BackupMetadataFile::CSPtr nextMetadataFile = metadataArray_[nextIndex];
        ASSERT_IFNOT(
            nextIndex == 0 || nextMetadataFile->BackupOption == FABRIC_BACKUP_OPTION_INCREMENTAL,
            "{0}: nextIndex is {1} BackupOption is {2} Backup folder: {3}",
            TraceId,
            nextIndex,
            static_cast<int>(nextMetadataFile->BackupOption),
            backupFolderCSPtr_->operator LPCWSTR());

        // Enforcing invariant 2: If data loss number has changed, this is cannot be trimmed or restored.
        if (currentMetadataFile->BackupEpoch.DataLossVersion != nextMetadataFile->BackupEpoch.DataLossVersion)
        {
            LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                ToStringLiteral(
                    wformatString(
                        L"Folder: {0} A backup chain cannot contain backups from different data loss numbers. BackupId: {1} ParentId: {2} DL: {3} CN: {4} LSN: {5}",
                        backupFolderCSPtr_->operator LPCWSTR(),
                        Common::Guid(headMetadataFile->BackupId),
                        Common::Guid(headMetadataFile->ParentBackupId),
                        headMetadataFile->BackupEpoch.DataLossVersion,
                        headMetadataFile->BackupEpoch.ConfigurationVersion,
                        headMetadataFile->BackupLSN)),
                STATUS_INVALID_PARAMETER);

            throw Exception(STATUS_INVALID_PARAMETER);
        }

        // Enforcing weakly invariant 1.
        // Note that this also enforces rule 1.
        if (currentMetadataFile->BackupLSN < nextMetadataFile->BackupLSN)
        {
            LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                ToStringLiteral(
                    wformatString(
                        L"Folder: {0} Backups corrupted. Backup lsn order does not match. CurrentMetadataFile BackupLSN: {1} NextMetadataFile BackupLSN: {2}",
                        backupFolderCSPtr_->operator LPCWSTR(),
                        headMetadataFile->BackupLSN,
                        nextMetadataFile->BackupLSN)),
                STATUS_INVALID_PARAMETER);

            throw Exception(STATUS_INVALID_PARAMETER);
        }

        // If appropriate chain link, continue;
        if (currentMetadataFile->ParentBackupId == nextMetadataFile->BackupId)
        {
            currentMetadataFile = nextMetadataFile;
            --nextIndex;
            continue;
        }

        // If the longest chain does not link into head (full) than throw since full backup cannot be removed.
        if (nextIndex == 0)
        {
            LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                ToStringLiteral(
                    wformatString(
                        L"Folder: {0} Incremental backup chain broken. Parent id {1} must match backup id {2}.",
                        backupFolderCSPtr_->operator LPCWSTR(),
                        Common::Guid(headMetadataFile->BackupId),
                            Common::Guid(currentMetadataFile->ParentBackupId))),
                STATUS_INVALID_PARAMETER);

            throw Exception(STATUS_INVALID_PARAMETER);
        }

        // Remove the divergent duplicate link.
        EventSource::Events->BackupFolderWarning(
            TracePartitionId,
            ReplicaId,
            ToStringLiteral(
                wformatString(
                    L"Trimming the following backup chain link. Index: {0} NextMetadataFile: BackupId: {1} ParentBackupId: {2}.",
                    nextIndex,
                    Common::Guid(nextMetadataFile->BackupId),
                        Common::Guid(nextMetadataFile->ParentBackupId))),
            backupFolderCSPtr_->operator LPCWSTR(),
            STATUS_SUCCESS);

        metadataArray_.Remove(nextIndex);
        logFilePathArray_.Remove(nextIndex);
        --nextIndex;
    }
}

ktl::Awaitable<void> BackupFolderInfo::VerifyAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    ASSERT_IF(stateManagerBackupFolderPath_ == nullptr, "{0}: Must at least contain one backup.", TraceId);
    ASSERT_IFNOT(backupVersionArray_.Count() > 0, "{0}: Must at least contain one backup.", TraceId);

    // Port Note: Added a method for readability.
    VerifyMetadataList();

    if (test_doNotVerifyLogFile_)
    {
        EventSource::Events->BackupFolderWarning(
            TracePartitionId,
            ReplicaId,
            L"Skipping log file validation because test setting is turned on",
            backupFolderCSPtr_->operator LPCWSTR(),
            STATUS_SUCCESS);
    }
    else
    {
        // Note: This is an expensive verification.
        co_await VerifyLogFilesAsync(cancellationToken);
    }

    co_return;
}

void BackupFolderInfo::VerifyMetadataList()
{
    ASSERT_IFNOT(
        metadataArray_.Count() == logFilePathArray_.Count(),
        "{0}: metadataArray_.Count() {1} == logFilePathArray_.Count() {2}",
        TraceId,
        metadataArray_.Count(),
        logFilePathArray_.Count());

    BackupMetadataFile::CSPtr lastBackupMetadataFile = nullptr;
    for (BackupMetadataFile::CSPtr metadataFile : metadataArray_)
    {
        // lastBackupMetadataFile != nullptr only for the full backup metadata.
        if (lastBackupMetadataFile == nullptr)
        {
            // If verifying the full backup metadata, we need to ensure it is linking to itself.
            if (metadataFile->ParentBackupId != metadataFile->BackupId)
            {
                LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                    ToStringLiteral(
                        wformatString(
                            L"Full backup metadata is corrupted: ParentId {0} must match backupId {1}",
                            Common::Guid(metadataFile->ParentBackupId),
                            Common::Guid(metadataFile->BackupId))),
                    STATUS_INTERNAL_DB_CORRUPTION);

                throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
            }
        }
        else
        {
            // If incremental backup, we expect it to link back.
            if (lastBackupMetadataFile->BackupId != metadataFile->ParentBackupId)
            {
                LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                    ToStringLiteral(
                        wformatString(
                        L"Broken backup chain: BackupId {0} != ParentBackupId {1}",
                        Common::Guid(lastBackupMetadataFile->BackupId),
                        Common::Guid(metadataFile->ParentBackupId))),
                    STATUS_INVALID_PARAMETER);

                throw Exception(STATUS_INVALID_PARAMETER);
            }

            // If incremental backup, we expect its LSN to be higher than the previous chain.
            if (lastBackupMetadataFile->BackupLSN >= metadataFile->BackupLSN)
            {
                LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
                    ToStringLiteral(
                        wformatString(
                        L"Broken backup chain: previous BackupLSN {0} < ParentBackupId {1}",
                        lastBackupMetadataFile->BackupLSN,
                        metadataFile->BackupLSN)),
                    STATUS_INTERNAL_DB_CORRUPTION);

                throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
            }
        }

        // Set last.
        lastBackupMetadataFile = metadataFile;
    }
}

ktl::Awaitable<void> BackupFolderInfo::VerifyLogFilesAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    ASSERT_IFNOT(
        backupVersionArray_.Count() == logFilePathArray_.Count(),
        "{0}: backupVersionList_.Count() {1} == logFilePathList_.Count() {2}",
        TraceId,
        backupVersionArray_.Count(),
        logFilePathArray_.Count());

    for (KString::CSPtr backupLogFilePath : logFilePathArray_)
    {
        KWString path(GetThisAllocator(), backupLogFilePath->ToUNICODE_STRING());
        BackupLogFile::SPtr backupLogFile = BackupLogFile::Create(*PartitionedReplicaIdentifier, path, GetThisAllocator());
        co_await backupLogFile->ReadAsync(cancellationToken);
        co_await backupLogFile->VerifyAsync();
    }

    co_return;
}

void BackupFolderInfo::TraceException(
    __in Common::WStringLiteral const & message,
    __in Exception & exception) const
{
    EventSource::Events->BackupFolderWarning(
        TracePartitionId,
        ReplicaId,
        message,
        backupFolderCSPtr_->operator LPCWSTR(),
        exception.GetStatus());
}

void BackupFolderInfo::ThrowIfNecessary(
    __in NTSTATUS status,
    __in std::wstring const & message) const
{
    if (NT_SUCCESS(status))
    {
        return;
    }

    LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
        ToStringLiteral(message),
        status);

    throw ktl::Exception(status);
}

BackupFolderInfo::BackupFolderInfo(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KGuid const & id,
    __in KString const & backupFolder,
    __in bool test_doNotVerifyLogFile)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , id_(id)
    , backupFolderCSPtr_(&backupFolder)
    , backupVersionArray_(GetThisAllocator())
    , metadataArray_(GetThisAllocator())
    , logFilePathArray_(GetThisAllocator())
    , test_doNotVerifyLogFile_(test_doNotVerifyLogFile)
{
    if (NT_SUCCESS(backupVersionArray_.Status()) == false)
    {
        SetConstructorStatus(backupVersionArray_.Status());
        return;
    }

    if (NT_SUCCESS(metadataArray_.Status()) == false)
    {
        SetConstructorStatus(metadataArray_.Status());
        return;
    }

    if (NT_SUCCESS(logFilePathArray_.Status()) == false)
    {
        SetConstructorStatus(logFilePathArray_.Status());
        return;
    }
}

BackupFolderInfo::~BackupFolderInfo()
{
}
