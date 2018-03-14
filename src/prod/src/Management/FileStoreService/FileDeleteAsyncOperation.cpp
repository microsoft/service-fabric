// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileDeleteAsyncOperation");

FileDeleteAsyncOperation::FileDeleteAsyncOperation(
    __in RequestManager & requestManager,
    std::wstring const & storeRelativePath,
    Common::ActivityId const & activityId,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : FileAsyncOperation(
        requestManager, 
        false, // useSimpleTx
        storeRelativePath, 
        false, // useTwoPhaseCommit
        activityId, 
        timeout, 
        callback, 
        parent)
    , isDirectory_(false)
    , fileVersionToDelete_()
{
}

FileDeleteAsyncOperation::~FileDeleteAsyncOperation()
{
}

ErrorCode FileDeleteAsyncOperation::TransitionToIntermediateState(StoreTransaction const & storeTx)
{
    vector<FileMetadata> metadataList;
    ErrorCode error;
    if(Directory::Exists(Path::Combine(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath)))
    {
        isDirectory_ = true;
        error = this->ReplicatedStoreWrapperObj.ReadPrefix(storeTx, this->NormalizedStoreRelativePath, metadataList);
    }
    else
    {
        FileMetadata metadata(this->StoreRelativePath);
        error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
        if(error.IsSuccess())
        {
            metadataList.push_back(metadata);
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::FileNotFound;
        }
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    if (metadataList.empty())
    {
        return ErrorCodeValue::FileNotFound;
    }
    
    for(auto iter = metadataList.begin(); iter != metadataList.end(); ++iter)
    {
        if(!FileState::IsStable(iter->State))
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Failed to update the metadata for delete since its not in a stable state: {0}",
                *iter);
            return ErrorCodeValue::FileUpdateInProgress;
        }

        fileVersionToDelete_ = iter->CurrentVersion;
        iter->State = FileState::Deleting;
        iter->CopyDesc = CopyDescription();

        error = storeTx.Update(*iter);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Updating the metadata for delete: {0}, Error:{1}",
            *iter,
            error);
        if(!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FileDeleteAsyncOperation::TransitionToReplicatingState(StoreTransaction const & storeTx)
{
    vector<FileMetadata> metadataList;
    ErrorCode error;
    if(isDirectory_)
    {
        error = this->ReplicatedStoreWrapperObj.ReadPrefix(storeTx, this->NormalizedStoreRelativePath, metadataList);
    }
    else
    {
        FileMetadata metadata(this->StoreRelativePath);
        error = this->ReplicatedStoreWrapperObj.ReadExact(storeTx, metadata);
        if(error.IsSuccess())
        {
            metadataList.push_back(metadata);
        }
    }
        
    if(!error.IsSuccess())
    {
        return error;
    }
        
    for (auto & metadata : metadataList)
    {            
        ASSERT_IF(metadata.State != FileState::Deleting, "Metata:{0} should be in Deleting state", metadata);
        error = storeTx.Delete(metadata);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Deleting Metadata:{0}, Error:{1}",
            metadata,
            error);
        if(!error.IsSuccess())
        {
            return error;
        }
    }    
    
    return ErrorCodeValue::Success;
}

ErrorCode FileDeleteAsyncOperation::TransitionToCommittedState(StoreTransaction const &)
{
    Assert::CodingError(TraceComponent, "{0} TransitionToCommittedState should be disabled", TraceId);
}

ErrorCode FileDeleteAsyncOperation::TransitionToRolledbackState(StoreTransaction const & storeTx)
{
    // TransitionToReplicatingState also deletes the files
    return this->TransitionToReplicatingState(storeTx);
}

AsyncOperationSPtr FileDeleteAsyncOperation::OnBeginFileOperation(
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    if (isDirectory_)
    {
        auto error = Directory::Delete(Path::Combine(this->RequestManagerObj.ReplicaObj.StoreRoot, this->StoreRelativePath), true, true);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Deleting directory {0}. Error:{1}",
            this->StoreRelativePath,
            error);
    }
    else
    {
        wstring versionedStoreRelativePath = Utility::GetVersionedFile(this->StoreRelativePath, fileVersionToDelete_.ToString());
        auto error = Utility::DeleteVersionedStoreFile(this->RequestManagerObj.ReplicaObj.StoreRoot, versionedStoreRelativePath);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "Deleting versioned files of {0}. Error:{1}",
            this->StoreRelativePath,
            error);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        callback,
        parent);
}

ErrorCode FileDeleteAsyncOperation::OnEndFileOperation(
    AsyncOperationSPtr const &operation)
{
    return CompletedAsyncOperation::End(operation);
}

void FileDeleteAsyncOperation::UndoFileOperation() { /*no-op*/ }

