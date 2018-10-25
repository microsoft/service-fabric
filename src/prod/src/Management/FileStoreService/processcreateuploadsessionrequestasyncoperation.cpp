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

StringLiteral const TraceComponent("ProcessCreateUploadSessionRequestAsyncOperation");

class ProcessCreateUploadSessionRequestAsyncOperation::CreateUploadSessionAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{

public:
    CreateUploadSessionAsyncOperation(
        RequestManager & requestManager,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        uint64 fileSize,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , storeRelativePath_(storeRelativePath)
        , sessionId_(sessionId)
        , fileSize_(fileSize)
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CreateUploadSessionAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = this->requestManager_.UploadSessionMap->CreateUploadSessionMapEntry(
            this->storeRelativePath_,
            this->sessionId_,
            this->fileSize_);

        this->TryComplete(thisSPtr, error);
    }

private:

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    std::wstring storeRelativePath_;
    Common::Guid sessionId_;
    uint64 fileSize_;
};

ProcessCreateUploadSessionRequestAsyncOperation::ProcessCreateUploadSessionRequestAsyncOperation(
    __in RequestManager & requestManager,
    CreateUploadSessionRequest && createRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, createRequest.StoreRelativePath, OperationKind::CreateUploadSession, move(receiverContext), activityId, timeout, callback, parent)
    , storeRelativePath_(createRequest.StoreRelativePath)
    , sessionId_(createRequest.SessionId)
    , fileSize_(createRequest.FileSize)
{
}

ProcessCreateUploadSessionRequestAsyncOperation::~ProcessCreateUploadSessionRequestAsyncOperation()
{
}

ErrorCode ProcessCreateUploadSessionRequestAsyncOperation::ValidateRequest()
{    
    if (this->sessionId_ == Common::Guid::Empty())
    {
        WriteInfo(
            TraceComponent,
            this->RequestManagerObj.TraceId,
            "Empty session Id");
        return ErrorCodeValue::InvalidArgument;
    }
    
    if (this->storeRelativePath_.empty())
    {
        WriteInfo(
            TraceComponent,
            this->RequestManagerObj.TraceId,
            "Empty store relative path");
        return ErrorCodeValue::InvalidArgument;
    }    

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessCreateUploadSessionRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CreateUploadSessionAsyncOperation>(
        this->RequestManagerObj,
        this->storeRelativePath_,
        this->sessionId_,
        this->fileSize_,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessCreateUploadSessionRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CreateUploadSessionAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }

    return errorCode;
}

void ProcessCreateUploadSessionRequestAsyncOperation::WriteTrace(__in Common::ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "CreateUploadSession request failed with error {0}, sessionId:{1}, storeRelativePath:{2}, fileSize:{3}",
            error,
            this->sessionId_,
            this->storeRelativePath_,
            this->fileSize_);
    }
}