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
    shouldOverwrite_(uploadRequest.ShouldOverwrite)
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
    if(errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }
    return errorCode;
}

void ProcessUploadRequestAsyncOperation::OnRequestCompleted(Common::ErrorCode & error)
{   
    // Override the base to NOT overwrite 'NotPrimary' ErrorCode with 'FileStoreServiceNotReady' since the copying staging has to done at the client side.

    ConvertObjectClosedErrorCode(error);

    // Delete the staging file except for FileUpdateInProgress error. Client will retry on FileUpdateInProgress.
    if(!error.IsError(ErrorCodeValue::FileUpdateInProgress) && File::Exists(stagingFullPath_))
    {
        File::Delete2(stagingFullPath_, true /*deleteReadOnlyFiles*/);
    }
}
