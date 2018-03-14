// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("FileStreamFullCopyManager");

//
// *** ArchiveFileContext
//
// There can be multiple archive files in use at any time (for multiple full copies).
// This context tracks usage of a given archive file for the purposes of cleanup.
//
class FileStreamFullCopyManager::ArchiveFileContext
    : public PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>
{
public:

    ArchiveFileContext(
        Store::PartitionedReplicaId const & traceId,
        wstring const & fileName,
        ::FABRIC_SEQUENCE_NUMBER lsn)
        : PartitionedReplicaTraceComponent(traceId)
        , fileName_(fileName)
        , lsn_(lsn)
        , usageCount_(0)
    {
    }

    __declspec(property(get=get_Lsn)) ::FABRIC_SEQUENCE_NUMBER Lsn;
    ::FABRIC_SEQUENCE_NUMBER get_Lsn() const { return lsn_; }

    ErrorCode TryCreateCopyContext_CallerHoldsLock(
        ActivityId const & activityId,
        __out FileStreamFullCopyContextSPtr & result)
    {
        ReplicaActivityId traceId(this->PartitionedReplicaId, activityId);

        result.reset();

        File archiveFile;

        auto error = archiveFile.TryOpen(
            fileName_,
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read,
            FileAttributes::Normal);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to open {1}: error={2}",
                traceId,
                fileName_,
                error);

            return error;
        }

        int64 fileSize;

        error = archiveFile.GetSize(fileSize);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to get size {1}: error={2}",
                traceId,
                fileName_,
                error);

            return error;
        }

        result = make_shared<FileStreamFullCopyContext>(
            traceId,
            move(archiveFile),
            lsn_,
            fileSize);

        auto count = ++usageCount_;

        WriteInfo(
            TraceComponent,
            "{0}: attached full copy context {1}: usage={2}",
            traceId,
            fileName_,
            count);

        return ErrorCodeValue::Success;
    }

    ErrorCode TryReleaseCopyContext_CallerHoldsLock(
        ActivityId const & activityId,
        __out size_t & usageCount)
    {
        ReplicaActivityId traceId(this->PartitionedReplicaId, activityId);

        ErrorCode error;

        if (usageCount_ <= 0)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: ArchiveFileContext usage count is {1}", traceId, usageCount_);

            error = this->TryCleanup(traceId);
        }
        else
        {
            auto count = --usageCount_;

            WriteInfo(
                TraceComponent,
                "{0}: released full copy context {1}: usage={2}",
                traceId,
                fileName_,
                count);

            if (count == 0)
            {
                error = this->TryCleanup(traceId);
            }
        }

        usageCount = usageCount_;

        return error;
    }

private:

    ErrorCode TryCleanup(ReplicaActivityId const & traceId)
    {
        int maxRetryCount = StoreConfig::GetConfig().DeleteDatabaseRetryCount;
        int retryCount = maxRetryCount;
        bool success = false;

        while (!success)
        {
            if (!File::Exists(fileName_))
            {
                success = true;

                break;
            }

            auto error = File::Delete2(fileName_);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: deleted full copy archive {1}",
                    traceId,
                    fileName_);

                success = true;
            }
            else if (error.IsError(ErrorCodeValue::AccessDenied) && --retryCount > 0)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} couldn't delete {1}: error={2} retry={3} max={4}",
                    this->TraceId,
                    fileName_,
                    error,
                    retryCount,
                    maxRetryCount);

                Sleep(StoreConfig::GetConfig().DeleteDatabaseRetryDelayMilliseconds);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to delete {1}: error={2}",
                    traceId,
                    fileName_,
                    error);

                return error;
            }
        }

        return ErrorCodeValue::Success;
    }

    wstring fileName_;
    ::FABRIC_SEQUENCE_NUMBER lsn_;
    size_t usageCount_;
};

//
// *** GetFileStreamCopyContextAsyncOperation
//

class FileStreamFullCopyManager::GetFileStreamCopyContextAsyncOperation
    : public AsyncOperation
    , public ReplicaActivityTraceComponent<TraceTaskCodes::ReplicatedStore>
{
public:
    GetFileStreamCopyContextAsyncOperation(
        Store::ReplicaActivityId const & traceId,
        __in FileStreamFullCopyManager & owner,
        ::FABRIC_SEQUENCE_NUMBER upToLsn,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , ReplicaActivityTraceComponent(traceId)
        , owner_(owner)
        , upToLsn_(upToLsn)
        , copyContextResult_()
    {
    }

    __declspec(property(get=get_ReplicatedStore)) ReplicatedStore & Store;
    ReplicatedStore & get_ReplicatedStore() const { return owner_.replicatedStore_; }

    __declspec(property(get=get_UpToLsn)) ::FABRIC_SEQUENCE_NUMBER UpToLsn;
    ::FABRIC_SEQUENCE_NUMBER get_UpToLsn() const { return upToLsn_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out FileStreamFullCopyContextSPtr & result)
    {
        auto casted = AsyncOperation::End<GetFileStreamCopyContextAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            result = casted->copyContextResult_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->TryAttachToExistingArchiveFile(thisSPtr);
    }

private:

    void TryAttachToExistingArchiveFile(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        bool createNewArchive = false;
        uint fileSequenceNumber = 0;

        {
            AcquireWriteLock lock(owner_.lock_);

            if (!owner_.archiveFilesByLsn_.empty())
            {
                // Use existing archive file if its LSN satifies the target LSN
                // requirement of the new incoming full copy
                //
                for (auto const & pair : owner_.archiveFilesByLsn_)
                {
                    auto const & archiveFileContext = pair.second;

                    if (archiveFileContext->Lsn >= upToLsn_)
                    {
                        WriteInfo(
                            TraceComponent,
                            "{0}: attaching to archive file (LSN): existing={1} target={2}",
                            this->TraceId,
                            archiveFileContext->Lsn,
                            upToLsn_);

                        error = archiveFileContext->TryCreateCopyContext_CallerHoldsLock(this->ActivityId, copyContextResult_);

                        break;
                    }
                    else
                    {
                        WriteInfo(
                            TraceComponent,
                            "{0}: skip archive file (LSN): existing={1} target={2}",
                            this->TraceId,
                            archiveFileContext->Lsn,
                            upToLsn_);
                    }
                }
            }

            if (error.IsSuccess() && !copyContextResult_)
            {
                // Only one ESE backup is allowed at a time per ESE instance (per replica)
                //
                if (owner_.isBackupActive_)
                {
                    auto maxWaiters = StoreConfig::GetConfig().MaxFileStreamFullCopyWaiters;
                    auto currentWaiters = owner_.backupWaiters_.size();

                    WriteInfo(
                        TraceComponent,
                        "{0}: backup for full copy already active: LSN={1} waiters={2}/{3}",
                        this->TraceId,
                        upToLsn_,
                        currentWaiters,
                        maxWaiters);

                    if (maxWaiters < 0 || currentWaiters < maxWaiters)
                    {
                        auto casted = dynamic_pointer_cast<GetFileStreamCopyContextAsyncOperation>(thisSPtr);

                        owner_.backupWaiters_.push_back(move(casted));
                    }
                    else
                    {
                        error = ErrorCodeValue::MaxFileStreamFullCopyWaiters;
                    }
                }
                else
                {
                    createNewArchive = true;
                    fileSequenceNumber = ++owner_.archiveFileSequenceNumber_;

                    owner_.isBackupActive_ = true;
                }
            }
        } // lock

        if (!error.IsSuccess() || copyContextResult_)
        {
            this->TryComplete(thisSPtr, error);
        }
        else if (createNewArchive)
        {
            error = this->CreateNewArchive(thisSPtr, fileSequenceNumber);

            this->ReleaseBackupWaiters(thisSPtr);

            this->TryComplete(thisSPtr, error);
        }
    }

    ErrorCode CreateNewArchive(AsyncOperationSPtr const &, uint fileSequenceNumber)
    {
        auto baseDir = Path::Combine(
            this->Store.GetFileStreamFullCopyRootDirectory(),
            wformatString("{0}", fileSequenceNumber));

        auto backupDir = wformatString("{0}b", baseDir);
        auto archiveFileName = wformatString("{0}z", baseDir);

        WriteInfo(
            TraceComponent,
            "{0}: creating backup for full copy: dir={1} LSN={2}",
            this->TraceId,
            backupDir,
            upToLsn_);

        Stopwatch stopwatch;

        stopwatch.Start();

        if (Directory::Exists(backupDir))
        {
            auto error = Directory::Delete(backupDir, true);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to delete {1}: error={2}",
                    this->TraceId,
                    backupDir,
                    error);

                return error;
            }
        }

        {
            shared_ptr<ScopedActiveBackupState> activeBackupFlag;
            if (!this->Store.TryAcquireActiveBackupState(StoreBackupRequester::System, activeBackupFlag))
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: backup already in progress",
                    this->TraceId);

                return ErrorCodeValue::BackupInProgress;
            }

            auto error = this->Store.DisableIncrementalBackup();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to reset incremental backup state: error={1}",
                    this->TraceId,
                    error);

                return error;
            }

            AcquireReadLock lock(this->Store.DropLocalStoreLock);

            if (this->Store.IsLocalStoreClosed)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            error = this->Store.LocalStore->Backup(backupDir, StoreBackupOption::Full);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to backup {1}: error={2}",
                    this->TraceId,
                    backupDir,
                    error);

                return error;
            }
        }

        auto elapsedBackup = stopwatch.Elapsed;

        WriteInfo(
            TraceComponent,
            "{0}: creating archive for full copy: archive={1} LSN={2}",
            this->TraceId,
            archiveFileName,
            upToLsn_);

        if (File::Exists(archiveFileName))
        {
            auto error = File::Delete2(archiveFileName);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to delete {1}: error={2}",
                    this->TraceId,
                    archiveFileName,
                    error);

                return error;
            }
        }

        auto error = Directory::CreateArchive(backupDir, archiveFileName);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to archive {1} -> {2}: error={3}",
                this->TraceId,
                backupDir,
                archiveFileName,
                error);

            return error;
        }

        auto elapsedArchive = stopwatch.Elapsed;

        if (Directory::Exists(backupDir))
        {
            error = Directory::Delete(backupDir, true);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to delete {1}: error={2}",
                    this->TraceId,
                    backupDir,
                    error);

                return error;
            }
        }

        auto archiveFileContext = make_shared<ArchiveFileContext>(
            Store::PartitionedReplicaId(this->PartitionId, this->ReplicaId),
            archiveFileName,
            upToLsn_);
        {
            AcquireWriteLock lock(owner_.lock_);

            error = archiveFileContext->TryCreateCopyContext_CallerHoldsLock(this->ActivityId, copyContextResult_);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed attach to archive file {1}: error={2}",
                    this->TraceId,
                    archiveFileName,
                    error);

                return error;
            }

            auto inserted = owner_.archiveFilesByLsn_.insert(ArchiveFilePair(upToLsn_, archiveFileContext)).second;

            if (!inserted)
            {
                TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} ArchiveFileContext already exists: LSN={1}", this->TraceId, upToLsn_);
            }
        }

        auto elapsedOpen = stopwatch.Elapsed;

        WriteInfo(
            TraceComponent,
            "{0}: created full copy archive - time: backup={1} archive={2} open={3} LSN={4}",
            this->TraceId,
            elapsedBackup,
            elapsedArchive,
            elapsedOpen,
            upToLsn_);

        return error;
    }

    void ReleaseBackupWaiters(AsyncOperationSPtr const &)
    {
        vector<shared_ptr<GetFileStreamCopyContextAsyncOperation>> backupWaiters;
        {
            AcquireWriteLock lock(owner_.lock_);

            owner_.isBackupActive_ = false;

            backupWaiters.swap(owner_.backupWaiters_);
        }

        WriteInfo(
            TraceComponent,
            "{0}: releasing {1} backup waiters",
            this->TraceId,
            backupWaiters.size());

        auto ownerRoot = owner_.CreateComponentRoot();
        auto storeRoot = owner_.replicatedStore_.Root.CreateComponentRoot();

        for (auto const & waiter : backupWaiters)
        {
            // Schedule thread for each waiter since TryAttachToExistingArchiveFile may
            // complete into ComCopyOperationEnumerator, which will then call back into
            // FileStreamFullCopyContext::ReadNextFileStreamChunk. We want to dispatch
            // these subsequent copies to occur in parallel without blocking on reading
            // any file chunks.
            //
            Threadpool::Post([waiter, ownerRoot, storeRoot] { waiter->TryAttachToExistingArchiveFile(waiter); });
        }
    }

    FileStreamFullCopyManager & owner_;
    ::FABRIC_SEQUENCE_NUMBER upToLsn_;
    FileStreamFullCopyContextSPtr copyContextResult_;
};

//
// *** FileStreamFullCopyManager
//

FileStreamFullCopyManager::FileStreamFullCopyManager(__in ReplicatedStore & owner)
    : ComponentRoot()
    , PartitionedReplicaTraceComponent(owner.PartitionedReplicaId)
    , replicatedStore_(owner)
    , lock_()
    , isBackupActive_(false)
    , backupWaiters_()
    , archiveFilesByLsn_()
    , archiveFileSequenceNumber_(0)
{
}

AsyncOperationSPtr FileStreamFullCopyManager::BeginGetFileStreamCopyContext(
    ActivityId const & activityId,
    ::FABRIC_SEQUENCE_NUMBER upToLsn,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetFileStreamCopyContextAsyncOperation>(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        *this,
        upToLsn,
        callback,
        parent);
}

ErrorCode FileStreamFullCopyManager::EndGetFileStreamCopyContext(
    AsyncOperationSPtr const & operation,
    __out shared_ptr<FileStreamFullCopyContext> & result)
{
    return GetFileStreamCopyContextAsyncOperation::End(operation, result);
}

ErrorCode FileStreamFullCopyManager::ReleaseFileStreamCopyContext(
    ActivityId const & activityId,
    shared_ptr<FileStreamFullCopyContext> && copyContext)
{
    AcquireWriteLock lock(lock_);

    auto iter = archiveFilesByLsn_.find(copyContext->Lsn);

    if (iter != archiveFilesByLsn_.end())
    {
        size_t usageCount = 0;
        auto error = iter->second->TryReleaseCopyContext_CallerHoldsLock(
            activityId,
            usageCount);

        if (!error.IsSuccess()) { return error; }

        if (usageCount == 0)
        {
            archiveFilesByLsn_.erase(copyContext->Lsn);
        }
    }
    else
    {
        ReplicaActivityId traceId(this->PartitionedReplicaId, activityId);

        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: ArchiveFileContext not found: LSN={1}", traceId, copyContext->Lsn);
    }

    return ErrorCodeValue::Success;
}
