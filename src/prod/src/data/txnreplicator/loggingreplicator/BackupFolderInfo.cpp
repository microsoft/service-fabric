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
    __in KAllocator & allocator)
{
    BackupFolderInfo * pointer = _new(BACKUP_FOLDER_INFO_TAG, allocator) BackupFolderInfo(
        traceId,
        id,
        backupFolder);

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

    co_await VerifyAsync(cancellationToken);

    co_return;
}

// Summary: Adds a folder into the BackupFolderInfo.
//
// Port Note: Merged incremental and full to reduce code.
// Port Note: Upper layer passes the directory as well as the metadata file path to avoid recomputation.
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
    ASSERT_IFNOT(index <= 0, "{0}: Insert failed.", TraceId);

    // Port Note: Instead of using two SortedList, I use one array for the key and two arrays for the two types of values.
    // This reduces the number of arrays. In future, this could be turned into a data structure.
    // Insert version to version list.
    status = backupVersionArray_.InsertAt(~index, version);
    ThrowIfNecessary(status, L"AddFolderAsync: Version list insert.");

    // Insert the metadata in the metadata list in the corresponding section.
    status = metadataArray_.InsertAt(~index, backupMetadataFile.RawPtr());
    ThrowIfNecessary(status, L"AddFolderAsync: Metadata list insert.");

    KString::SPtr logFilePath = nullptr;
    status = KString::Create(logFilePath, GetThisAllocator(), backupDirectoryPath.c_str());
    ThrowIfNecessary(status, L"AddFolderAsync: String creation for log path.");

    KPath::CombineInPlace(*logFilePath, BackupManager::ReplicatorBackupFolderName);
    KPath::CombineInPlace(*logFilePath, BackupManager::ReplicatorBackupLogName);

    // Insert the path in the log path list in the corresponding section.
    status = logFilePathArray_.InsertAt(~index, logFilePath.RawPtr());
    ThrowIfNecessary(status, L"AddFolderAsync: log file list insert.");

    co_return;
}

ktl::Awaitable<void> BackupFolderInfo::VerifyAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    ASSERT_IF(stateManagerBackupFolderPath_ == nullptr, "{0}: Must at least contain one backup.", TraceId);
    ASSERT_IFNOT(backupVersionArray_.Count() > 0, "{0}: Must at least contain one backup.", TraceId);

    // Port Note: Added a method for readability.
    VerifyMetadataList();

    // Note: This is an expensive verification.
    co_await VerifyLogFilesAsync(cancellationToken);

    co_return;
}

void BackupFolderInfo::VerifyMetadataList()
{
    ASSERT_IFNOT(
        backupVersionArray_.Count() == metadataArray_.Count(),
        "{0}: backupVersionList_.Count() {1} == metadataList_.Count() {2}",
        TraceId,
        backupVersionArray_.Count(),
        metadataArray_.Count());

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
    __in KString const & backupFolder)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , id_(id)
    , backupFolderCSPtr_(&backupFolder)
    , backupVersionArray_(GetThisAllocator())
    , metadataArray_(GetThisAllocator())
    , logFilePathArray_(GetThisAllocator())
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
