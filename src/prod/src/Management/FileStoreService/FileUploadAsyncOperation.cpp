// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileUploadAsyncOperation");

class FileUploadAsyncOperation::MoveAsyncOperation : public AsyncOperation
{
public:
    MoveAsyncOperation(
        FileUploadAsyncOperation & owner,
        wstring const & stagingFullPath, 
        wstring const & storeFullPath, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , stagingFullPath_(stagingFullPath)
        , storeFullPath_(storeFullPath)
        , retryCount_(0)
    {
    }

    ~MoveAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<MoveAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        wstring directory = Path::GetDirectoryName(storeFullPath_);
        if (!Directory::Exists(directory))
        {
            auto error = Directory::Create2(directory);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "Unable to create directory: {0}. Error:{1}",
                    directory,
                    error);
                TryComplete(thisSPtr, error);
                return;
            }
        }

        auto jobItem = make_unique<DefaultTimedJobItem<RequestManager>>(
            owner_.TimeoutHelperObj.GetRemainingTime(),
            [this, thisSPtr](RequestManager &)
            {
                this->MoveFile(thisSPtr);
            },
            [this, thisSPtr](RequestManager &)
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "Operation timed out in job queue");
                this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
            });

        bool success = owner_.RequestManagerObj.fileOperationProcessingJobQueue_->Enqueue(move(jobItem));
        if (!success)
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "Could not enqueue MoveFile to JobQueue.");
                
            TryComplete(thisSPtr, ErrorCodeValue::NotPrimary);
            return;
        }
    }

private:
    void MoveFile(AsyncOperationSPtr const & thisSPtr)
    {

#if defined(PLATFORM_UNIX)
        wstring stagingPath(stagingFullPath_), storePath(storeFullPath_);
        std::replace(stagingPath.begin(),stagingPath.end(), '\\', '/');
        std::replace(storePath.begin(),storePath.end(), '\\', '/');
        auto error = File::MoveTransacted(stagingPath, storePath, true);
        WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                owner_.TraceId,
                "MoveTransacted - Source: {0}, destination: {1}, Error: {2}, Count: {3}",
                stagingPath,
                storePath,
                error,
                retryCount_);
#else
        auto error = File::MoveTransacted(stagingFullPath_, storeFullPath_, true);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            owner_.TraceId,
            "MoveTransacted - Source: {0}, destination: {1}, Error: {2}, Count: {3}",
            stagingFullPath_,
            storeFullPath_,
            error,
            retryCount_);

#endif
        // ERROR_SHARING_VIOLATION is a retryable error
        if (error.IsSuccess() || 
            (!owner_.IsSharingViolationError(error)) ||
            retryCount_ >= FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
        {
            TryComplete(thisSPtr, error);
            return;
        }

        retryCount_++;

        error = owner_.TryScheduleRetry(
            thisSPtr,
            [this](AsyncOperationSPtr const & thisSPtr) { this->OnStart(thisSPtr); });
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
    }

private:
    FileUploadAsyncOperation & owner_;
    uint retryCount_;

    wstring const stagingFullPath_;
    wstring const storeFullPath_;
};

FileUploadAsyncOperation::FileUploadAsyncOperation(
    __in RequestManager & requestManager,
    wstring const & stagingFullPath,
    wstring const & storeRelativePath,
    bool shouldOverwrite,
    Guid const & uploadRequestId,
    StoreFileVersion const & fileVersion,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : FileAsyncOperation(
        requestManager, 
        true, // useSimpleTx
        storeRelativePath, 
        FileStoreServiceConfig::GetConfig().EnableTStore || Store::StoreConfig::GetConfig().EnableTStore, // useTwoPhaseCommit
        activityId, 
        timeout, 
        callback, 
        parent),
    stagingFullPath_(stagingFullPath),
    storeFullPath_(Utility::GetVersionedFileFullPath(requestManager.ReplicaObj.StoreRoot, storeRelativePath, fileVersion)),
    shouldOverwrite_(shouldOverwrite),
    uploadRequestId_(uploadRequestId),
    fileVersion_(fileVersion)
{
}

FileUploadAsyncOperation::~FileUploadAsyncOperation()
{
}

bool FileUploadAsyncOperation::CheckIfPreviousVersionExists()
{
    // For version which didn't have the UploadRequestId field in the metadata (for v<=6.2), random guid will be generated and used for comparison
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);
    if (error.IsSuccess() && FileState::IsStable(metadata.State) && metadata.UploadRequestId != this->uploadRequestId_)
    {
        WriteInfo(
            TraceComponent,
            "FileUploadAsyncOperation::TransitionToIntermediateState Previous version exists for file {0} newUploadId:{1}",
            metadata,
            this->uploadRequestId_
        );
        return true;
    }

    return false;
}

ErrorCode FileUploadAsyncOperation::TransitionToIntermediateState(StoreTransaction const & storeTx)
{
    bool previousVersionExists = CheckIfPreviousVersionExists();

    FileMetadata metadata(this->StoreRelativePath, fileVersion_, FileState::Updating, this->uploadRequestId_);
    bool doesExist = false;
    auto error = this->ReplicatedStoreWrapperObj.TryReadOrInsertIfNotFound(this->ActivityId, storeTx, metadata, doesExist);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "FileUploadAsyncOperation::TransitionToIntermediateState - TryReadOrInsertIfNotFound:{0}, Error:{1}, DoesExist:{2}",
        metadata,
        error,
        doesExist);
    if(!error.IsSuccess())
    {
        return error;
    }

    if(doesExist)
    {
        if (!previousVersionExists)
        {
            if (FileState::IsStable(metadata.State))
            {
                return ErrorCodeValue::AlreadyExists;
            }
            else
            {
                // There can't be two active threads setting the metadata to "Updating" state which means previous transaction is interrupted.
                // It is possible that previous transaction might have been failed leaving the state in 'updating' state.
                // Return success here to  get to stable state for metadata.

                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "Some previous transaction has left metadata in non-terminal state : {0} storePath{1}",
                    metadata,
                    storeFullPath_);
                return ErrorCodeValue::Success;
            }
        }

        if(!FileState::IsStable(metadata.State))
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Failed to update the metadata for upload since its not in a stable state: {0} storePath{1}",
                metadata,
                storeFullPath_);
            return ErrorCodeValue::FileUpdateInProgress;
        }

        if(!shouldOverwrite_)
        {
            return ErrorCodeValue::FileAlreadyExists;
        }

        metadata.State = FileState::Updating;
        metadata.PreviousVersion = metadata.CurrentVersion;
        metadata.CurrentVersion = fileVersion_;
        metadata.UploadRequestId = uploadRequestId_;

        metadata.CopyDesc = CopyDescription();
        
        error = storeTx.Update(metadata);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Updating an existing metadata:{0}, Error:{1}",
            metadata,
            error);

        if(!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FileUploadAsyncOperation::TransitionToReplicatingState(StoreTransaction const & storeTx)
{
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "ReadExact:{0}, Error:{1}",
        metadata,
        error);
    if(!error.IsSuccess())
    {
        return error;
    }

    if(metadata.PreviousVersion != StoreFileVersion::Default)
    {
        wstring previousFilePath = Utility::GetVersionedFileFullPath(
            this->RequestManagerObj.ReplicaObj.StoreRoot, 
            metadata.StoreRelativeLocation, 
            metadata.PreviousVersion);
        if(File::Exists(previousFilePath))
        {
            error = File::Delete2(previousFilePath, true /*deleteReadOnlyFiles*/);
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                TraceId,
                "Deleting previous version of the file. PreviousFile:{0}, Error:{1}",
                previousFilePath,
                error);
        }
    }

    metadata.State = this->UseTwoPhaseCommit ? FileState::Replicating : FileState::Available_V1;
    error = storeTx.Update(metadata);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "Updating metadata to {0}:{1}, Error:{2}",
        metadata.State,
        metadata,
        error);
    return error;
}

ErrorCode FileUploadAsyncOperation::TransitionToCommittedState(StoreTransaction const & storeTx)
{
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "ReadExact:{0}, Error:{1}",
        metadata,
        error);
    if(!error.IsSuccess())
    {
        return error;
    }

    metadata.State = FileState::Committed;
    error = storeTx.Update(metadata);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "Updating metadata to Committed:{0}, Error:{1}",
        metadata,
        error);
    return error;
}

ErrorCode FileUploadAsyncOperation::TransitionToRolledbackState(StoreTransaction const & storeTx)
{
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
    if(!error.IsSuccess())
    {
        return error;
    }

    ASSERT_IF(FileState::IsStable(metadata.State), "Rollback should only be called when state is stable. Metadata:{0}", metadata);

    if(metadata.PreviousVersion == StoreFileVersion::Default
        || !File::Exists(Utility::GetVersionedFileFullPath(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath, metadata.PreviousVersion)))
    {
        error = storeTx.Delete(metadata);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to delete metadata during rollback:{0}, Error:{1}",
                metadata,
                error);
        }
    }
    else
    {
        metadata.CurrentVersion = metadata.PreviousVersion;
        metadata.PreviousVersion = StoreFileVersion::Default;
        metadata.State = FileState::Committed;
        metadata.UploadRequestId = this->uploadRequestId_;
        metadata.CopyDesc = CopyDescription();

        error = storeTx.Update(metadata);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to restore metadata during rollback:{0}, Error:{1}",
                metadata,
                error);
        }
    }

    return error;
}

AsyncOperationSPtr FileUploadAsyncOperation::OnBeginFileOperation(
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    static int chaosCount = 0;
    if (FileStoreServiceConfig::GetConfig().EnableChaosDuringFileUpload)
    {
        // This is to test multiple commit requests from the client
        ++chaosCount;
        if (chaosCount % 11 == 0 && this->StoreRelativePath.find(L"xml") != std::string::npos)
        {
            Sleep(35000);
        }
    }

    return AsyncOperation::CreateAndStart<MoveAsyncOperation>(
        *this,
        stagingFullPath_,
        storeFullPath_,
        callback,
        parent);
}

ErrorCode FileUploadAsyncOperation::OnEndFileOperation(
    AsyncOperationSPtr const &operation)
{    
    return MoveAsyncOperation::End(operation);
}

void FileUploadAsyncOperation::UndoFileOperation()
{
    if(File::Exists(storeFullPath_))
    {
        auto error = File::Delete2(storeFullPath_, true /*deleteReadOnlyFiles*/);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to perform delete during rollback of upload. Path:{0}, Error:{1}",
                storeFullPath_,
                error);
        }
    }
}
