// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileCopyAsyncOperation");

class FileCopyAsyncOperation::CopyAsyncOperation : public AsyncOperation
{
public:
    CopyAsyncOperation(
        FileCopyAsyncOperation & owner,
        wstring const & sourceStoreFullPath,
        wstring const & destinationStoreFullPath,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , sourceStoreFullPath_(sourceStoreFullPath)
        , destinationStoreFullPath_(destinationStoreFullPath)
        , retryCount_(0)
    {
    }

    ~CopyAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<CopyAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        wstring directory = Path::GetDirectoryName(destinationStoreFullPath_);
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
            this->CopyFile(thisSPtr);
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
                "Could not enqueue CopyFile to JobQueue.");

            TryComplete(thisSPtr, ErrorCodeValue::NotPrimary);
            return;
        }
    }

private:
    void CopyFile(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = File::Copy(sourceStoreFullPath_, destinationStoreFullPath_, true);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            owner_.TraceId,
            "Copy - Source: {0}, destination: {1}, Error: {2}, Count: {3}",
            sourceStoreFullPath_,
            destinationStoreFullPath_,
            error,
            retryCount_);

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
    FileCopyAsyncOperation & owner_;
    uint retryCount_;

    wstring const sourceStoreFullPath_;
    wstring const destinationStoreFullPath_;
};

FileCopyAsyncOperation::FileCopyAsyncOperation(
    __in RequestManager & requestManager,
    wstring const & sourceStoreRelativePath,
    StoreFileVersion const & sourceFileVersion,
    wstring const & destinationStoreRelativePath,
    StoreFileVersion const & destinationFileVersion,
    bool shouldOverwrite,    
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : FileAsyncOperation(
        requestManager, 
        true, // useSimpleTx
        destinationStoreRelativePath, 
        FileStoreServiceConfig::GetConfig().EnableTStore || Store::StoreConfig::GetConfig().EnableTStore , // useTwoPhaseCommit
        activityId, 
        timeout, 
        callback, 
        parent),
    sourceStoreRelativePath_(sourceStoreRelativePath),
    sourceStoreFullPath_(Utility::GetVersionedFileFullPath(requestManager.ReplicaObj.StoreRoot, sourceStoreRelativePath, sourceFileVersion)),
    sourceFileVersion_(sourceFileVersion),
    destinationStoreFullPath_(Utility::GetVersionedFileFullPath(requestManager.ReplicaObj.StoreRoot, destinationStoreRelativePath, destinationFileVersion)),
    shouldOverwrite_(shouldOverwrite),
    destinationFileVersion_(destinationFileVersion)
{
}

FileCopyAsyncOperation::~FileCopyAsyncOperation()
{
}

ErrorCode FileCopyAsyncOperation::TransitionToIntermediateState(StoreTransaction const & storeTx)
{
    // check if source is present
    FileMetadata sourceMetadata(this->sourceStoreRelativePath_);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, sourceMetadata);
    if ((error.IsSuccess() && sourceMetadata.CurrentVersion != sourceFileVersion_) || 
        error.IsError(ErrorCodeValue::NotFound))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Source file could not be found or version did not match. SourceMetadata:{0}, ExpectedSourceVersion:{1}, ErrorCode:{2}",
            sourceMetadata,
            sourceFileVersion_,
            error);
        return ErrorCodeValue::FileNotFound;
    }
    
    if (!error.IsSuccess())
    {
        return error;
    }

    // Source is present. Continue process Copy
    CopyDescription copyDesc(sourceStoreRelativePath_, sourceFileVersion_);    
    FileMetadata metadata(this->StoreRelativePath, destinationFileVersion_, FileState::Updating, copyDesc);
    bool doesExist = false;
    error = this->ReplicatedStoreWrapperObj.TryReadOrInsertIfNotFound(this->ActivityId, storeTx, metadata, doesExist);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "TryReadOrInsertIfNotFound:{0}, Error:{1}, DoesExist:{2}",
        metadata,
        error,
        doesExist);
    if (!error.IsSuccess())
    {
        return error;
    }

    if(doesExist)
    {
        if (!FileState::IsStable(metadata.State))
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Failed to update the metadata for upload since its not in a stable state: {0}",
                metadata);
            return ErrorCodeValue::FileUpdateInProgress;
        }

        if (!shouldOverwrite_)
        {
            return ErrorCodeValue::FileAlreadyExists;
        }

        metadata.State = FileState::Updating;
        metadata.PreviousVersion = metadata.CurrentVersion;
        metadata.CurrentVersion = destinationFileVersion_;

        metadata.CopyDesc = copyDesc;

        error = storeTx.Update(metadata);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Updating an existing metadata:{0}, Error:{1}",
            metadata,
            error);
    }

    return error;
}

ErrorCode FileCopyAsyncOperation::TransitionToReplicatingState(StoreTransaction const & storeTx)
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
    if (!error.IsSuccess())
    {
        return error;
    }

    if (metadata.PreviousVersion != StoreFileVersion::Default)
    {
        wstring previousFilePath = Utility::GetVersionedFileFullPath(
            this->RequestManagerObj.ReplicaObj.StoreRoot,
            metadata.StoreRelativeLocation,
            metadata.PreviousVersion);
        if (File::Exists(previousFilePath))
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

ErrorCode FileCopyAsyncOperation::TransitionToCommittedState(StoreTransaction const & storeTx)
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
    if (!error.IsSuccess())
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

ErrorCode FileCopyAsyncOperation::TransitionToRolledbackState(StoreTransaction const & storeTx)
{
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
    if (!error.IsSuccess())
    {
        return error;
    }

    ASSERT_IF(FileState::IsStable(metadata.State), "Rollback should only be called when state is stable. Metadata:{0}", metadata);

    if (metadata.PreviousVersion == StoreFileVersion::Default
        || !File::Exists(Utility::GetVersionedFileFullPath(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath, metadata.PreviousVersion)))
    {
        error = storeTx.Delete(metadata);
    }
    else
    {
        metadata.CurrentVersion = metadata.PreviousVersion;
        metadata.PreviousVersion = StoreFileVersion::Default;
        metadata.State = FileState::Committed;

        metadata.CopyDesc = CopyDescription();

        error = storeTx.Update(metadata);
    }

    return error;
}

AsyncOperationSPtr FileCopyAsyncOperation::OnBeginFileOperation(
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<CopyAsyncOperation>(
        *this,
        sourceStoreFullPath_,
        destinationStoreFullPath_,
        callback,
        parent);
}

ErrorCode FileCopyAsyncOperation::OnEndFileOperation(
    AsyncOperationSPtr const &operation)
{
    return CopyAsyncOperation::End(operation);
}

void FileCopyAsyncOperation::UndoFileOperation()
{
    if (File::Exists(destinationStoreFullPath_))
    {
        auto error = File::Delete2(destinationStoreFullPath_, true /*deleteReadOnlyFiles*/);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to perform delete during rollback of upload. Path:{0}, Error:{1}",
                destinationStoreFullPath_,
                error);
        }
    }
}
