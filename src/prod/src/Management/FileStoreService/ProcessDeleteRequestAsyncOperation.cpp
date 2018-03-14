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

ProcessDeleteRequestAsyncOperation::ProcessDeleteRequestAsyncOperation(
    __in RequestManager & requestManager,
    ImageStoreBaseRequest && deleteRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
    : ProcessRequestAsyncOperation(requestManager, deleteRequest.StoreRelativePath, OperationKind::Delete, move(receiverContext), activityId, timeout, callback, parent)
{
}

ProcessDeleteRequestAsyncOperation::~ProcessDeleteRequestAsyncOperation()
{
}

ErrorCode ProcessDeleteRequestAsyncOperation::ValidateRequest()
{    
    if(this->StoreRelativePath.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    static const wstring reservedStoreName = L"Store", reservedWindowsFabricStoreName = L"WindowsFabricStore";
    if (StringUtility::CompareCaseInsensitive(this->StoreRelativePath, reservedStoreName) == 0 ||
        StringUtility::CompareCaseInsensitive(this->StoreRelativePath, reservedWindowsFabricStoreName) == 0)
    {
        return ErrorCodeValue::ImageBuilderReservedDirectoryError;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessDeleteRequestAsyncOperation::BeginOperation(                                
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{       
    return AsyncOperation::CreateAndStart<FileDeleteAsyncOperation>(
        this->RequestManagerObj, 
        this->StoreRelativePath,        
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessDeleteRequestAsyncOperation::EndOperation(
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
