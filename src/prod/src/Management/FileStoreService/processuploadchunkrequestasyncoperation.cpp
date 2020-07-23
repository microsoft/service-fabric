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

StringLiteral const TraceComponent("ProcessUploadChunkRequestAsyncOperation");

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
        static int count = -1;
        static bool incorrectUpdateSentOnce = false;
        ++count;

        if (FileStoreServiceConfig::GetConfig().EnableChaosDuringFileUpload)
        {
            int fileChunkSize = 3145728;
            if (!incorrectUpdateSentOnce && this->startPosition_ == fileChunkSize * 10) // seq. Id - 10
            {
                incorrectUpdateSentOnce = true;
                WriteWarning(
                    TraceComponent,
                    this->requestManager_.TraceId,
                    "Chaos enabled: Sending success without updating metadata : UploadChunkAsyncOperation: {0} start:end={1}:{2}",
                    this->sessionId_,
                    this->startPosition_,
                    this->endPosition_);

                // Do not update the metadata to simulate failure of missing chunk
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);

                return;
            }

            if (count == 19)
            {
                WriteWarning(
                    TraceComponent,
                    this->requestManager_.TraceId,
                    "Chaos enabled: Sending ErrorCodeValue::NotFound to simulate failover for UploadChunkAsyncOperation: {0} start:end={1}:{2}",
                    this->sessionId_,
                    this->startPosition_,
                    this->endPosition_);

                this->TryComplete(thisSPtr, ErrorCodeValue::NotFound);

                return;
            }

            if (this->startPosition_ == fileChunkSize * 3)
            {
                // TO handle multiple request
                Sleep(15000);
            }
        }

        ErrorCode error = this->requestManager_.UploadSessionMap->UpdateUploadSessionMapEntry(
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            this->stagingFullPath_);

        if (!error.IsSuccess())
        {
            UploadSessionMetadataSPtr uploadSessionMetadata;
            auto sessionMetadataError = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, uploadSessionMetadata);
            if (sessionMetadataError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "Getting Staging location failed. sessionId:{0} error:{1} UploadSessionMetadata:{2}.",
                    this->sessionId_,
                    error,
                    uploadSessionMetadata);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "Getting Staging location failed. sessionId:{0}: error:{1}",
                    this->sessionId_,
                    error);
            }
        }

        this->TryComplete(thisSPtr, move(error));
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
{
    stagingFullPath_ = Path::Combine(requestManager.LocalStagingLocation, uploadChunkRequest.StagingRelativePath);
#if !defined(PLATFORM_UNIX)
    // This takes care of staging file path that is greater than MAX_PATH chars.
    // For more info: https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file
    stagingFullPath_.insert(0, L"\\\\?\\");
#endif
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
        WriteNoise(
            TraceComponent,
            "RequestManager",
            "UploadChunkAsyncOperation: stagingFileNotFound: {0} start:end={1}:{2} path={3}",
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            this->stagingFullPath_);

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

void ProcessUploadChunkRequestAsyncOperation::OnRequestCompleted(__inout ErrorCode & error)
{
    ProcessRequestAsyncOperation::OnRequestCompleted(error);

    // For error case, delete the staging file
    // For error with FileStoreServiceNotReady, gateway would retry to upload to the new primary
    if (!this->stagingFullPath_.empty() && 
        !error.IsSuccess() && 
        !error.IsError(ErrorCodeValue::FileStoreServiceNotReady))
    {
        // Delete staging file
        if (File::Exists(this->stagingFullPath_))
        {
            auto deleteError = File::Delete2(this->stagingFullPath_, true);
            if (!deleteError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "Deleting staged chunk file:{0}, deleteError:{1} originalError:{2}",
                    this->stagingFullPath_,
                    deleteError,
                    error);
            }
        }
    }
}

void ProcessUploadChunkRequestAsyncOperation::WriteTrace(ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "UploadChunk request failed with error {0}, sessionId:{1}, startPostion:{2}, endPosition:{3}",
            error,
            this->sessionId_,
            this->startPosition_,
            this->endPosition_);
    }
}
