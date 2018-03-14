// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace Management::FileStoreService;


StringLiteral const TraceComponent("ProcessCheckExistenceRequestAsyncOperation");
GlobalWString ProcessCheckExistenceRequestAsyncOperation::DirectoryMarkerFileName = make_global<wstring>(L"_.dir");

ProcessCheckExistenceRequestAsyncOperation::ProcessCheckExistenceRequestAsyncOperation(
    __in RequestManager & requestManager,
    ImageStoreBaseRequest && checkExistenceRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, checkExistenceRequest.StoreRelativePath, OperationKind::CheckExistence, move(receiverContext), activityId, timeout, callback, parent)
    , exists_(false)
{
}

ProcessCheckExistenceRequestAsyncOperation::~ProcessCheckExistenceRequestAsyncOperation()
{
}

AsyncOperationSPtr ProcessCheckExistenceRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    FileMetadata metadata(this->StoreRelativePath);
    ErrorCode error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, metadata);
    if (error.IsSuccess())
    {
        exists_ = FileState::IsStable(metadata.State);
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        FileMetadata folderMetadata(Path::Combine(this->StoreRelativePath, *ProcessCheckExistenceRequestAsyncOperation::DirectoryMarkerFileName));
        error = this->ReplicatedStoreWrapperObj.ReadExact(this->ActivityId, folderMetadata);
        if (error.IsSuccess())
        {
            exists_ = FileState::IsStable(folderMetadata.State);
        }
    }

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "FileNotFound={0}",
            this->StoreRelativePath);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
}

ErrorCode ProcessCheckExistenceRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CompletedAsyncOperation::End(asyncOperation);
    if(errorCode.IsSuccess())
    {
        ImageStoreContentExistsReplyMessageBody replyMessageBody(exists_);
        reply = FileStoreServiceMessage::GetClientOperationSuccess(replyMessageBody, this->ActivityId);
    }

    return errorCode;
}
