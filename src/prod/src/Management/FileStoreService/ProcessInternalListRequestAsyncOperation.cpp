// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace Management::FileStoreService;

ProcessInternalListRequestAsyncOperation::ProcessInternalListRequestAsyncOperation(
    __in RequestManager & requestManager,
    ListRequest && listRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    : ProcessRequestAsyncOperation(requestManager, listRequest.StoreRelativePath, OperationKind::InternalList, move(receiverContext), activityId, timeout, callback, parent)
    , isPresent_(false)
    , state_()
    , currentVersion_()
{
}

ProcessInternalListRequestAsyncOperation::~ProcessInternalListRequestAsyncOperation()
{
}

ErrorCode ProcessInternalListRequestAsyncOperation::ValidateRequest()
{
    if(this->StoreRelativePath.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessInternalListRequestAsyncOperation::BeginOperation(                                
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{       
    auto error = this->ListMetadata();
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ErrorCode ProcessInternalListRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CompletedAsyncOperation::End(asyncOperation);
    if(errorCode.IsSuccess())
    {
        InternalListReply listReply(isPresent_, state_, currentVersion_, this->RequestManagerObj.StoreShareLocation);
        reply = FileStoreServiceMessage::GetClientOperationSuccess(listReply, this->ActivityId);
    }

    return errorCode;
}

ErrorCode ProcessInternalListRequestAsyncOperation::ListMetadata()
{        
    // list the specific file
    FileMetadata metadata(this->StoreRelativePath);
    auto error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);

    if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }

    if(error.IsError(ErrorCodeValue::NotFound))
    {
        isPresent_ = false;
        return ErrorCodeValue::Success;
    }

    isPresent_ = true;
    state_ = metadata.State;
    currentVersion_ = metadata.CurrentVersion;

    return ErrorCodeValue::Success;
}

