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

StringLiteral const TraceComponent("ProcessDeleteUploadSessionRequestAsyncOperation");

class ProcessUploadChunkRequestAsyncOperation::UploadChunkAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
public:

    UploadChunkAsyncOperation(
        RequestManager & requestManager,
        std::wstring stagingFullPath,
        Common::Guid sessionId,
        uint64 startPosition,
        uint64 endPosition,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , stagingFullPath_(stagingFullPath)
        , sessionId_(sessionId)
        , startPosition_(startPosition)
        , endPosition_(endPosition)
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadChunkAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = this->requestManager_.UploadSessionMap->UpdateUploadSessionMapEntry(
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            this->stagingFullPath_);

        this->TryComplete(thisSPtr, error);
    }

private:

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    std::wstring stagingFullPath_;
    Common::Guid sessionId_;
    uint64 startPosition_;
    uint64 endPosition_;
};


ProcessUploadChunkRequestAsyncOperation::ProcessUploadChunkRequestAsyncOperation(
    __in RequestManager & requestManager,
    UploadChunkRequest && uploadChunkRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, L"", OperationKind::UploadChunk, move(receiverContext), activityId, timeout, callback, parent)
    , sessionId_(uploadChunkRequest.SessionId)
    , startPosition_(uploadChunkRequest.StartPosition)
    , endPosition_(uploadChunkRequest.EndPosition)
    , stagingFullPath_(Path::Combine(requestManager.LocalStagingLocation, uploadChunkRequest.StagingRelativePath))
{
}

ProcessUploadChunkRequestAsyncOperation::~ProcessUploadChunkRequestAsyncOperation()
{
}

ErrorCode ProcessUploadChunkRequestAsyncOperation::ValidateRequest()
{
    if (this->sessionId_ == Common::Guid::Empty()
        || this->stagingFullPath_.empty()
        || this->startPosition_ > this->endPosition_)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (!File::Exists(this->stagingFullPath_))
    {
        return ErrorCodeValue::StagingFileNotFound;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessUploadChunkRequestAsyncOperation::BeginOperation(
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UploadChunkAsyncOperation>(
        this->RequestManagerObj,
        this->stagingFullPath_,
        this->sessionId_,
        this->startPosition_,
        this->endPosition_,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessUploadChunkRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = UploadChunkAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }

    return errorCode;
}
