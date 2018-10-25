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

class ProcessDeleteUploadSessionRequestAsyncOperation::DeleteUploadSessionAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{

public:

    DeleteUploadSessionAsyncOperation(
        RequestManager & requestManager,
        Common::Guid const & sessionId,
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
        return AsyncOperation::End<DeleteUploadSessionAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        std::vector<std::wstring> stagingLocation;
        ErrorCode error = this->requestManager_.UploadSessionMap->DeleteUploadSessionMapEntry(this->sessionId_, stagingLocation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(
            TraceComponent,
            this->requestManager_.TraceId,
            "Total staging locations {0}",
            stagingLocation.size());

        for (auto it = stagingLocation.begin(); it != stagingLocation.end(); ++it)
        {
            if (File::Exists(*it))
            {
                error = File::Delete2(*it, false);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        this->requestManager_.TraceId,
                        "Deleting '{0}' failed with error {1}",
                        *it,
                        error);
                }
            }
        }

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
       
private:

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    Common::Guid sessionId_;
};


ProcessDeleteUploadSessionRequestAsyncOperation::ProcessDeleteUploadSessionRequestAsyncOperation(
    __in RequestManager & requestManager,
    DeleteUploadSessionRequest && deleteRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, L"", OperationKind::DeleteUploadSession, move(receiverContext), activityId, timeout, callback, parent)
    , sessionId_(deleteRequest.SessionId)
{
}

ProcessDeleteUploadSessionRequestAsyncOperation::~ProcessDeleteUploadSessionRequestAsyncOperation()
{
}

ErrorCode ProcessDeleteUploadSessionRequestAsyncOperation::ValidateRequest()
{    
    if (this->sessionId_ == Common::Guid::Empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessDeleteUploadSessionRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteUploadSessionAsyncOperation>(
        this->RequestManagerObj,
        this->sessionId_,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessDeleteUploadSessionRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = DeleteUploadSessionAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }

    return errorCode;
}

void ProcessDeleteUploadSessionRequestAsyncOperation::WriteTrace(__in Common::ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "DeleteUploadSession request failed with error {0}, sessionId:{1}",
            error,
            this->sessionId_);
    }
}
