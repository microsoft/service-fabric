// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReplicationComponent;
using namespace ServiceModel;

namespace Store
{
    GlobalWString EseReplicatedStore::LegacyFullCopyDirectorySuffix = make_global<wstring>(L"FCPY");
    GlobalWString EseReplicatedStore::LegacyPartialCopyDirectorySuffix = make_global<wstring>(L"PCPY");
    GlobalWString EseReplicatedStore::LegacyDropBackupDirectorySuffix = make_global<wstring>(L"DRP");

    GlobalWString EseReplicatedStore::FullCopyDirectorySuffix = make_global<wstring>(L"F");
    GlobalWString EseReplicatedStore::PartialCopyDirectorySuffix = make_global<wstring>(L"P");
    GlobalWString EseReplicatedStore::DropBackupDirectorySuffix = make_global<wstring>(L"D");

    GlobalWString EseReplicatedStore::PartialCopyCompletionMarkerFilename = make_global<wstring>(L"_completion_marker");

    StringLiteral const TraceComponent("EseReplicatedStore");

    EseReplicatedStore::EseReplicatedStore(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ReplicatorSettingsUPtr && replicatorSettings,
        Store::ReplicatedStoreSettings const & replicatedStoreSettings,
        EseLocalStoreSettings const & localStoreSettings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
        ComponentRoot const & root)
        : ReplicatedStore(
            partitionId,
            replicaId,
            move(replicatorSettings),
            replicatedStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            root)
        , settings_(localStoreSettings)
        , directoryRoot_()
        , directory_()
        , localStoreUPtr_()
    {
        this->InitializeDirectoryPath();
    }

    EseReplicatedStore::EseReplicatedStore(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ComPointer<IFabricReplicator> && replicator,
        Store::ReplicatedStoreSettings const & replicatedStoreSettings,
        EseLocalStoreSettings const & localStoreSettings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
        ComponentRoot const & root)
        : ReplicatedStore(
            partitionId,
            replicaId,
            move(replicator),
            replicatedStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            root)
        , settings_(localStoreSettings)
        , directoryRoot_()
        , directory_()
        , localStoreUPtr_()
    {
        this->InitializeDirectoryPath();
    }

    EseReplicatedStore::~EseReplicatedStore()
    {
    }

    unique_ptr<EseReplicatedStore> EseReplicatedStore::Create(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ReplicatorSettingsUPtr && replicatorSettings,
        Store::ReplicatedStoreSettings const & replicatedStoreSettings,
        EseLocalStoreSettings const & localStoreSettings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
        ComponentRoot const & root)
    {
        return unique_ptr<EseReplicatedStore>(new EseReplicatedStore(
            partitionId,
            replicaId,
            move(replicatorSettings),
            replicatedStoreSettings,
            localStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            root));
    }

    unique_ptr<EseReplicatedStore> EseReplicatedStore::Create(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ComPointer<IFabricReplicator> && replicator,
        Store::ReplicatedStoreSettings const & replicatedStoreSettings,
        EseLocalStoreSettings const & localStoreSettings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
        ComponentRoot const & root)
    {
        return unique_ptr<EseReplicatedStore>(new EseReplicatedStore(
            partitionId,
            replicaId,
            move(replicator),
            replicatedStoreSettings,
            localStoreSettings,
            storeEventHandler,
            secondaryEventHandler,
            root));
    }

    ILocalStoreUPtr const & EseReplicatedStore::get_LocalStore() const
    {
        return localStoreUPtr_;
    }

    void EseReplicatedStore::InitializeDirectoryPath()
    {
        directoryRoot_ = CreateLocalStoreRootDirectory(settings_.DbFolderPath, this->PartitionId);

        directory_ = CreateLocalStoreDatabaseDirectory(directoryRoot_, this->ReplicaId);
    }

    wstring EseReplicatedStore::CreateLocalStoreRootDirectory(wstring const & path, Guid const & partitionId)
    {
        return Path::Combine(path, wformatString("P_{0}", partitionId));
    }

    wstring EseReplicatedStore::CreateLocalStoreDatabaseDirectory(wstring const & root, FABRIC_REPLICA_ID replicaId)
    {
        return Path::Combine(root, wformatString("R_{0}", replicaId));
    }

    void EseReplicatedStore::CreateLocalStore()
    {
        localStoreUPtr_ = this->CreateLocalStore(directory_, true); // isMainStore
    }

    void EseReplicatedStore::ReleaseLocalStore()
    {
        // EseLocalStore must block destruction to
        // drain pending ESE transactions. 
        //
        // ReplicatedStore depends on holding an outstanding 
        // local store transaction in order to keep 
        // local store references valid to avoid coarse-grained 
        // locking when dropping it.
        //
        localStoreUPtr_.reset();
    }

    // Auto defragmentation/compaction is disabled for all side databases
    //
    ILocalStoreUPtr EseReplicatedStore::CreateTemporaryLocalStore()
    {
        return this->CreateLocalStore(directory_, false); // isMainStore
    }

    ILocalStoreUPtr EseReplicatedStore::CreateFullCopyLocalStore()
    {
        return this->CreateLocalStore(this->GetFullCopyDirectoryName(), false); // isMainStore
    }

    ILocalStoreUPtr EseReplicatedStore::CreatePartialCopyLocalStore()
    {
        return this->CreateLocalStore(this->GetPartialCopyDirectoryName(), false); // isMainStore
    }

    ILocalStoreUPtr EseReplicatedStore::CreateRebuildLocalStore()
    {
        // Re-use drop directory name as database rebuild source
        //
        return this->CreateLocalStore(this->GetDropBackupDirectoryName(), false); // isMainStore
    }

    ErrorCode EseReplicatedStore::CreateLocalStoreFromFullCopyData()
    {
        if (Directory::Exists(this->GetFullCopyDirectoryName()))
        {
            auto srcDir = this->GetFullCopyDirectoryName();
            auto targetDir = directory_;

            int maxRetryCount = StoreConfig::GetConfig().SwapDatabaseRetryCount;
            int retryCount = maxRetryCount;
            bool success = false;

            while (!success)
            {
                auto error = Directory::Rename(srcDir, targetDir, true); // overwrite

                if (error.IsSuccess())
                {
                    success = true;
                }
                else if (error.IsError(ErrorCodeValue::AccessDenied) && --retryCount > 0)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} couldn't swap local store {1} -> {2}: error={3} retry={4} max={5}",
                        this->TraceId,
                        srcDir,
                        targetDir,
                        error,
                        retryCount,
                        maxRetryCount);

                    Sleep(StoreConfig::GetConfig().SwapDatabaseRetryDelayMilliseconds);
                }
                else
                {
                    return error;
                }

            } // swap retry loop
        } // if copy directory exists

        this->CreateLocalStore();

        return ErrorCodeValue::Success;
    }

    ErrorCode EseReplicatedStore::MarkPartialCopyLocalStoreComplete()
    {
        auto partialCopyDirectoryName = this->GetPartialCopyDirectoryName();
        if (Directory::Exists(partialCopyDirectoryName))
        {
            auto markerFilename = this->GetPartialCopyCompletionMarkerFilename();

            File file;
            auto error = file.TryOpen(
                markerFilename,
                FileMode::OpenOrCreate,
                FileAccess::Write,
                FileShare::None,
                FileAttributes::None);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} failed open partial copy completion marker file '{1}': error={2}",
                    this->TraceId,
                    markerFilename,
                    error);

                return error;
            }

            vector<byte> buffer;
            buffer.push_back(0);

            file.Seek(0, SeekOrigin::Begin);
            file.Write(buffer.data(), static_cast<int>(buffer.size()));

            error = file.Close2();

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} failed write partial copy completion marker file '{1}': error={2}",
                    this->TraceId,
                    markerFilename,
                    error);
            }

            return error;
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} directory not found '{1}'",
                this->TraceId,
                partialCopyDirectoryName);
                
            return ErrorCodeValue::DirectoryNotFound;
        }
    }

    ErrorCode EseReplicatedStore::TryRecoverPartialCopyLocalStore(__out ILocalStoreUPtr & result)
    {
        result = nullptr;

        if (File::Exists(this->GetPartialCopyCompletionMarkerFilename()))
        {
            result = this->CreatePartialCopyLocalStore();

            return ErrorCodeValue::Success;
        }
        else if (Directory::Exists(this->GetPartialCopyDirectoryName()))
        {
            return this->DeletePartialCopyDatabases();
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    ErrorCode EseReplicatedStore::DeleteFullCopyDatabases()
    {
        auto error = this->DeleteDirectory(this->GetLegacyFullCopyDirectoryName());
        error = ErrorCode::FirstError(this->DeleteDirectory(this->GetFullCopyDirectoryName()), error);
        error = ErrorCode::FirstError(this->DeleteDirectory(this->GetFileStreamFullCopyRootDirectory()), error);
        return error;
    }

    ErrorCode EseReplicatedStore::DeletePartialCopyDatabases()
    {
        return this->DeleteDirectory(this->GetPartialCopyDirectoryName());
    }

    ErrorCode EseReplicatedStore::BackupAndDeleteDatabase()
    {
        wstring rootDir = this->GetDropBackupDirectoryName();

        wstring srcDir = directory_;
        wstring targetDir = Path::Combine(rootDir, wformatString("{0}", DateTime::Now().Ticks));

        if (Directory::Exists(srcDir))
        {
            if (!Directory::Exists(rootDir))
            {
                auto error = Directory::Create2(rootDir);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} BackupAndDeleteDatabase: failed to create '{1}'",
                        this->TraceId,
                        rootDir);

                    return error;
                }
            }

            auto error = Directory::Rename(srcDir, targetDir);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} BackupAndDeleteDatabase: moved '{1}' -> '{2}'",
                    this->TraceId,
                    srcDir,
                    targetDir);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} BackupAndDeleteDatabase: failed to move '{1}' -> '{2}': error={3}",
                    this->TraceId,
                    srcDir,
                    targetDir,
                    error);
            }

            return error;
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    void EseReplicatedStore::BestEffortDeleteAllCopyDatabases()
    {
        this->BestEffortDeleteDirectory(this->GetLegacyFullCopyDirectoryName());
        this->BestEffortDeleteDirectory(this->GetLegacyPartialCopyDirectoryName());
        this->BestEffortDeleteDirectory(this->GetLegacyDropBackupDirectoryName());

        this->BestEffortDeleteDirectory(this->GetFullCopyDirectoryName());
        this->BestEffortDeleteDirectory(this->GetPartialCopyDirectoryName());
        this->BestEffortDeleteDirectory(this->GetDropBackupDirectoryName());
        this->BestEffortDeleteDirectory(this->GetFileStreamFullCopyRootDirectory());
    }

    ErrorCode EseReplicatedStore::PreCopyNotification(std::vector<std::wstring> const & types)
    {
        return dynamic_cast<EseLocalStore*>(localStoreUPtr_.get())->PrefetchDatabaseTypes(types);
    }
    
    ErrorCode EseReplicatedStore::PostCopyNotification()
    {
        return dynamic_cast<EseLocalStore*>(localStoreUPtr_.get())->ResetOptimizeTableScans();
    }

    ErrorCode EseReplicatedStore::CheckPreOpenLocalStoreHealth(__out bool & reportedHealth)
    {
        reportedHealth = false;

        auto threshold = settings_.CompactionThresholdInMB;
        if (threshold <= 0) { return ErrorCodeValue::Success; }

        auto casted = dynamic_cast<EseLocalStore*>(localStoreUPtr_.get());

        int64 fileSize = 0;
        auto error = casted->GetOpenFileSize(fileSize);

        if (error.IsSuccess() && casted->ShouldAutoCompact(fileSize))
        {
            this->ReportHealth(
                SystemHealthReportCode::REStore_AutoCompaction,
                wformatString(GET_STORE_RC(AutoCompaction), fileSize, threshold),
                TimeSpan::MaxValue);

            reportedHealth = true;
        }

        return error;
    }

    ErrorCode EseReplicatedStore::CheckPostOpenLocalStoreHealth(bool cleanupHealth)
    {
        auto threshold = settings_.CompactionThresholdInMB;
        if (threshold <= 0) { return ErrorCodeValue::Success; }

        auto casted = dynamic_cast<EseLocalStore*>(localStoreUPtr_.get());

        int64 fileSize = 0;
        auto error = casted->GetOpenFileSize(fileSize);
        if (error.IsSuccess())
        {
            // Delete auto compaction health report if the database file size was successfully reduced.
            // Otherwise, update the description to warn that the database file size remains large
            // even after auto-compaction.
            //
            if (casted->ShouldAutoCompact(fileSize))
            {
                this->ReportHealth(
                    SystemHealthReportCode::REStore_AutoCompaction,
                    wformatString(GET_STORE_RC(Large_Database_File), fileSize, threshold),
                    TimeSpan::MaxValue);
            }
            else if (cleanupHealth)
            {
                this->ReportHealth(
                    SystemHealthReportCode::REStore_AutoCompaction,
                    wformatString(GET_STORE_RC(AutoCompaction), fileSize, threshold),
                    TimeSpan::FromSeconds(1));
            }
        }

        return error;
    }

    ErrorCode EseReplicatedStore::DeleteDirectory(wstring const & directory)
    {
        if (Directory::Exists(directory))
        {
            auto error = Directory::Delete(directory, true);

            auto msg = wformatString(
                "{0} delete directory {1}: error={2}",
                this->TraceId,
                directory,
                error);

            if (error.IsSuccess())
            {
                WriteInfo(TraceComponent, "{0}", msg);
            }
            else
            {
                WriteWarning(TraceComponent, "{0}", msg);
            }

            return error;
        }

        return ErrorCodeValue::Success;
    }

    void EseReplicatedStore::BestEffortDeleteDirectory(wstring const & directory)
    {
        this->DeleteDirectory(directory).ReadValue();
    }

    wstring EseReplicatedStore::GetLocalStoreRootDirectory()
    {
        return directoryRoot_;
    }

    wstring EseReplicatedStore::GetFileStreamFullCopyRootDirectory()
    {
        // Only one replica per partition and the directory root is already
        // scoped by partition ID, so we don't need to include the replica ID.
        //
        return Path::Combine(directoryRoot_, L"BF");
    }

    ILocalStoreUPtr EseReplicatedStore::CreateLocalStore(wstring const & dir, bool isMainStore)
    {
        auto flags = EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN;
        flags |= EseLocalStore::LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN;

        if (isMainStore)
        {
            return make_unique<EseLocalStore>(dir, settings_, flags);
        }
        else
        {
            EseLocalStoreSettings copySettings = settings_;
            copySettings.MaxDefragFrequency = TimeSpan::Zero;
            copySettings.CompactionThresholdInMB = 0;

            return make_unique<EseLocalStore>(dir, copySettings, flags);
        }
    }

    ErrorCode EseReplicatedStore::CreateEnumerationByPrimaryIndex(
        ILocalStoreUPtr const & store, 
        std::wstring const & typeStart,
        std::wstring const & keyStart,
        __out TransactionSPtr & txSPtr,
        __out EnumerationSPtr & enumSPtr)
    {
        auto const & targetStore = (store.get() != nullptr ? store : localStoreUPtr_);

        auto error = targetStore->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        auto casted = dynamic_cast<EseLocalStore*>(targetStore.get());
        return casted->CreateEnumerationByPrimaryIndex(txSPtr, typeStart, keyStart, enumSPtr);
    }

    bool EseReplicatedStore::ShouldRestartOnWriteError(ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::StoreNeedsDefragment))
        {
            auto threshold = settings_.CompactionThresholdInMB;
            if (threshold > 0)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} attempting to trigger auto-compaction: CompactionThresholdInMB={1}",
                    this->TraceId,
                    threshold);

                return true;
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} cannot trigger auto-compaction: CompactionThresholdInMB={1}",
                    this->TraceId,
                    threshold);
            }
        }

        return false;
    }

    wstring EseReplicatedStore::GetLegacyFullCopyDirectoryName() const
    {
        return wformatString("{0}_{1}", directory_, LegacyFullCopyDirectorySuffix);
    }

    wstring EseReplicatedStore::GetLegacyPartialCopyDirectoryName() const
    {
        return wformatString("{0}_{1}", directory_, LegacyPartialCopyDirectorySuffix);
    }

    wstring EseReplicatedStore::GetLegacyDropBackupDirectoryName() const
    {
        return wformatString("{0}_{1}", directory_, LegacyDropBackupDirectorySuffix);
    }

    wstring EseReplicatedStore::GetFullCopyDirectoryName() const
    {
        return wformatString("{0}{1}", directory_, FullCopyDirectorySuffix);
    }

    wstring EseReplicatedStore::GetPartialCopyDirectoryName() const
    {
        return wformatString("{0}{1}", directory_, PartialCopyDirectorySuffix);
    }

    wstring EseReplicatedStore::GetDropBackupDirectoryName() const
    {
        return wformatString("{0}{1}", directory_, DropBackupDirectorySuffix);
    }

    wstring EseReplicatedStore::GetPartialCopyCompletionMarkerFilename() const
    {
        return Path::Combine(this->GetPartialCopyDirectoryName(), PartialCopyCompletionMarkerFilename);
    }
}
