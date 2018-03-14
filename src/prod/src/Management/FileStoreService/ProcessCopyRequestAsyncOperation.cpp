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

StringLiteral const TraceComponent("ProcessCopyRequestAsyncOperation");

ProcessCopyRequestAsyncOperation::ProcessCopyRequestAsyncOperation(
    __in RequestManager & requestManager,
    CopyRequest && copyRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, copyRequest.DestinationStoreRelativePath, OperationKind::Copy, move(receiverContext), activityId, timeout, callback, parent)
    , sourceStoreRelativePath_(copyRequest.SourceStoreRelativePath)
    , sourceFileVersion_(copyRequest.SourceFileVersion)
    , shouldOverwrite_(copyRequest.ShouldOverwrite)
{
}

ProcessCopyRequestAsyncOperation::~ProcessCopyRequestAsyncOperation()
{
}

ErrorCode ProcessCopyRequestAsyncOperation::ValidateRequest()
{
    if (this->StoreRelativePath.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (this->sourceStoreRelativePath_.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (this->sourceFileVersion_ == StoreFileVersion::Default)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessCopyRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<FileCopyAsyncOperation>(
        this->RequestManagerObj,
        this->sourceStoreRelativePath_,
        this->sourceFileVersion_,
        this->StoreRelativePath,
        this->RequestManagerObj.GetNextFileVersion() /*destinationFileVersion*/,
        shouldOverwrite_,        
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessCopyRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = FileAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }
    return errorCode;
}
