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

StringLiteral const TraceComponent("ProcessListUploadSessionRequestAsyncOperation");

class ProcessListUploadSessionRequestAsyncOperation::ListUploadSessionBySessionIdAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
    DENY_COPY(ListUploadSessionBySessionIdAsyncOperation);

public:
    ListUploadSessionBySessionIdAsyncOperation(
        RequestManager & requestManager,
        Common::Guid sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , sessionId_(sessionId)
    {
        
    }
        
    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ListUploadSessionBySessionIdAsyncOperation>(operation)->Error;
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & operation,
        __out UploadSessionInfo & uploadSession)
    {
        auto casted = AsyncOperation::End<ListUploadSessionBySessionIdAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            uploadSession = std::move(casted->uploadSession_);
        }

        return casted->Error;
    }

protected :

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        if (this->sessionId_ == Common::Guid::Empty())
        {
            WriteWarning(
                TraceComponent,
                requestManager_.TraceId,
                "Empty session ID");
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        UploadSessionMetadataSPtr metadata;
        ErrorCode error = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, metadata);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        std::vector<UploadSessionInfo::ChunkRangePair> expectedRanges;
        metadata->GetNextExpectedRanges(expectedRanges);
        UploadSessionInfo uploadSession(metadata->StoreRelativePath, this->sessionId_, metadata->FileSize, metadata->LastModifiedTime, std::move(expectedRanges));
        this->uploadSession_ = std::move(uploadSession);
        this->TryComplete(thisSPtr, error);
    }

private:

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    Common::Guid sessionId_;
    Management::FileStoreService::UploadSessionInfo uploadSession_;
};

class ProcessListUploadSessionRequestAsyncOperation::ListUploadSessionByRelativePathAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
    DENY_COPY(ListUploadSessionByRelativePathAsyncOperation);

public:
    ListUploadSessionByRelativePathAsyncOperation(
        RequestManager & requestManager,
        std::wstring storeRelativePath,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , storeRelativePath_(std::move(storeRelativePath))
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ListUploadSessionByRelativePathAsyncOperation>(operation)->Error;
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & operation,
        __out std::vector<UploadSessionInfo> & uploadSessions)
    {
        auto casted = AsyncOperation::End<ListUploadSessionByRelativePathAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            uploadSessions = std::move(casted->uploadSessions_);
        }

        return casted->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        if (this->storeRelativePath_.empty())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        std::vector<UploadSessionMetadataSPtr> metadata;
        ErrorCode error = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->storeRelativePath_, metadata);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        std::vector<UploadSessionInfo::ChunkRangePair> expectedRanges;
        for (std::vector<UploadSessionMetadataSPtr>::const_iterator it = metadata.begin(); it != metadata.end(); ++it)
        {
            it->get()->GetNextExpectedRanges(expectedRanges);
            this->uploadSessions_.push_back(UploadSessionInfo(this->storeRelativePath_, it->get()->SessionId, it->get()->FileSize, it->get()->LastModifiedTime, std::move(expectedRanges)));
        }

        this->TryComplete(thisSPtr, error);
    }

private:

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    std::wstring storeRelativePath_;
    std::vector<Management::FileStoreService::UploadSessionInfo> uploadSessions_;
};

ProcessListUploadSessionRequestAsyncOperation::ProcessListUploadSessionRequestAsyncOperation(
    __in RequestManager & requestManager,
    UploadSessionRequest && request,
    Transport::IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, L"", OperationKind::ListUploadSession, move(receiverContext), activityId, timeout, callback, parent)
    , sessionId_(request.SessionId)
    , storeRelativePath_(request.StoreRelativePath)
{
}

ProcessListUploadSessionRequestAsyncOperation::~ProcessListUploadSessionRequestAsyncOperation()
{
}

ErrorCode ProcessListUploadSessionRequestAsyncOperation::ValidateRequest()
{
    if (this->sessionId_ == Common::Guid::Empty() && this->storeRelativePath_.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessListUploadSessionRequestAsyncOperation::BeginOperation(
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    if (this->storeRelativePath_.empty())
    {
        return AsyncOperation::CreateAndStart<ListUploadSessionBySessionIdAsyncOperation>(
            this->RequestManagerObj,
            this->sessionId_,
            this->timeoutHelper_.GetRemainingTime(),
            callback,
            parent);
    }
    else
    {
        return AsyncOperation::CreateAndStart<ListUploadSessionByRelativePathAsyncOperation>(
            this->RequestManagerObj,
            this->storeRelativePath_,
            this->timeoutHelper_.GetRemainingTime(),
            callback,
            parent); 
    }
}

ErrorCode ProcessListUploadSessionRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode errorCode;
    if (this->storeRelativePath_.empty())
    {
        UploadSessionInfo uploadSession;
        errorCode = ListUploadSessionBySessionIdAsyncOperation::End(asyncOperation, uploadSession);
        if (errorCode.IsSuccess())
        {
            this->uploadSessions_.push_back(uploadSession);
        }
    }
    else
    {
        errorCode = ListUploadSessionByRelativePathAsyncOperation::End(asyncOperation, this->uploadSessions_);
    }

    if (!errorCode.IsSuccess() && !errorCode.IsError(ErrorCodeValue::NotFound))
    {
        return errorCode;
    }

    UploadSession uploadSession(std::move(UploadSession(this->uploadSessions_)));
    reply = FileStoreServiceMessage::GetClientOperationSuccess(uploadSession, this->ActivityId);

    return ErrorCodeValue::Success;;
}

void ProcessListUploadSessionRequestAsyncOperation::WriteTrace(ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "ListUploadSession request failed with error {0}, sessionId:{1}, storeRelativePath:{2}",
            error,
            this->sessionId_,
            this->storeRelativePath_);
    }
}
