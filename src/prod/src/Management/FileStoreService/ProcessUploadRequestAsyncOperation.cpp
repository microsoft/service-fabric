// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("ProcessUploadRequestAsyncOperation");

ProcessUploadRequestAsyncOperation::ProcessUploadRequestAsyncOperation(
    __in RequestManager & requestManager,
    UploadRequest && uploadRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    : ProcessRequestAsyncOperation(requestManager, uploadRequest.StoreRelativePath, OperationKind::Upload, move(receiverContext), activityId, timeout, callback, parent),
    stagingFullPath_(Path::Combine(requestManager.LocalStagingLocation, uploadRequest.StagingRelativePath)),
    shouldOverwrite_(uploadRequest.ShouldOverwrite),
    uploadRequestId_(Guid::NewGuid())
{
}

ProcessUploadRequestAsyncOperation::~ProcessUploadRequestAsyncOperation()
{
}

ErrorCode ProcessUploadRequestAsyncOperation::ValidateRequest()
{    
    if(this->StoreRelativePath.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if(!File::Exists(stagingFullPath_))
    {
        return ErrorCodeValue::StagingFileNotFound;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessUploadRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{       
    return AsyncOperation::CreateAndStart<FileUploadAsyncOperation>(
        this->RequestManagerObj,
        stagingFullPath_,
        this->StoreRelativePath,
        shouldOverwrite_,
        this->uploadRequestId_,
        this->RequestManagerObj.GetNextFileVersion(),
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessUploadRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = FileAsyncOperation::End(asyncOperation);

    // For simple transaction, transactions are processed in groups.
    // If one of them fail, all of the transactions are canceled.
    // It is possible for a transaction to be inactive resulting in incomplete rollback of metadata where metadata is left in unstable state.
    // In case of error, unstable state metadata entry should not be present in the store and must be deleted (in a separate transaction).
    if (!errorCode.IsSuccess())
    {
        if (DeleteIfMetadataNotInStableState(asyncOperation->Parent))
        {
            WriteWarning(
                TraceComponent,
                "Metadata was not in stable state and was deleted for  storeRelativePath:{0}, Error:{1}",
                this->StoreRelativePath,
                errorCode);
        }
    }

    if(errorCode.IsSuccess() || errorCode.IsError(ErrorCodeValue::AlreadyExists))
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
        return ErrorCodeValue::Success;
    }

    return errorCode;
}


void ProcessUploadRequestAsyncOperation::OnRequestCompleted(__inout Common::ErrorCode & error)
{
    // Override the base to NOT overwrite 'NotPrimary' ErrorCode with 'FileStoreServiceNotReady' since the copying staging has to done at the client side.
    ConvertObjectClosedErrorCode(error);

    // Delete the staging file except for FileUpdateInProgress error. Client will retry on FileUpdateInProgress.
    DeleteStagingFile(error);
}

void ProcessUploadRequestAsyncOperation::DeleteStagingFile(Common::ErrorCode const & error)
{
    if (!error.IsError(ErrorCodeValue::FileUpdateInProgress) && File::Exists(stagingFullPath_))
    {
        auto deleteError = File::Delete2(stagingFullPath_, true /*deleteReadOnlyFiles*/);
        if (!deleteError.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Deleting staging file failed: Store:{0}, stagingFullPath:{1} deleteError{2} originalError{3}",
                this->StoreRelativePath,
                stagingFullPath_,
                deleteError,
                error);
        }
    }
}


