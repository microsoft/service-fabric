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

StringLiteral const TraceComponent("ProcessUploadChunkContentRequestAsyncOperation");

class ProcessUploadChunkContentRequestAsyncOperation::UploadChunkContentAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
public:

    UploadChunkContentAsyncOperation(
        RequestManager & requestManager,
        Management::FileStoreService::UploadChunkContentRequest && uploadChunkContentRequest,
        Common::Guid sessionId,
        uint64 startPosition,
        uint64 endPosition,
        std::wstring stagingFullPath,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , uploadChunkContentRequest_(move(uploadChunkContentRequest))
        , sessionId_(sessionId)
        , startPosition_(startPosition)
        , endPosition_(endPosition)
        , stagingFullPath_(stagingFullPath)
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadChunkContentAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto error = WriteChunkFile();
        if (error.IsError(ErrorCodeValue::AlreadyExists))
        {
            // Just make sure that range exists in our metadata
            UploadSessionMetadataSPtr uploadSessionMetadata;
            error = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, uploadSessionMetadata);
            if (error.IsSuccess())
            {
                error = uploadSessionMetadata->CheckRange(startPosition_, endPosition_);
                if (error.IsSuccess())
                {
                    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                    return;
                }
                else
                {
                    // Update the session map
                }
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "Staging file exists but the metadata entry not found. sessionId:{0} error:{1} stagingFullPath:{2} startPosition:{3} endPosition:{4}.",
                    this->sessionId_,
                    error,
                    stagingFullPath_,
                    startPosition_,
                    endPosition_);

                this->TryComplete(thisSPtr, ErrorCodeValue::NotFound);
                return;
            }
        }
        else if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        error = this->requestManager_.UploadSessionMap->UpdateUploadSessionMapEntry(
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
                    "Getting Staging location failed. sessionId:{0} error:{1} UploadSessionMetadata:{2} startPosition:{3} endPosition:{4}.",
                    this->sessionId_,
                    error,
                    uploadSessionMetadata,
                    startPosition_,
                    endPosition_);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "Getting Staging location failed. sessionId:{0}: error:{1} startPosition:{2} endPosition:{3}",
                    this->sessionId_,
                    error,
                    startPosition_,
                    endPosition_);
            }
        }

        this->TryComplete(thisSPtr, move(error));
    }

private:

    ErrorCode WriteChunkFile()
    {
        ErrorCode error;
        FileWriterLock fileLock(stagingFullPath_);
        if (!fileLock.Acquire(TimeSpan::FromSeconds(Constants::StagingFileLockAcquireTimeoutInSeconds)).IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Unable to get file lock operationId:{0}: filename:{1}.",
                sessionId_,
                stagingFullPath_);

            return ErrorCodeValue::OperationsPending;
        }

        // Create chunk file if it doesn't exist already
        if (!File::Exists(stagingFullPath_))
        {
            auto fileUptr = make_unique<File>();
            error = fileUptr->TryOpen(stagingFullPath_, FileMode::CreateNew, FileAccess::Write, FileShare::Write);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "TryOpen for sessionId:{0} startPosition:endPosition {1}:{2} failed with {3} for file:{4}. ",
                    sessionId_,
                    startPosition_,
                    endPosition_,
                    error,
                    stagingFullPath_);
                return error;
            }

            DWORD bytesWritten;
            error = fileUptr->TryWrite2(uploadChunkContentRequest_.ChunkContent.data(), static_cast<int>(uploadChunkContentRequest_.ChunkContent.size()), bytesWritten);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: TryWrite2 for file:{1} failed with {2}. ",
                    sessionId_,
                    stagingFullPath_,
                    error);

                fileUptr->Close();

                if (File::Exists(stagingFullPath_))
                {
                    File::Delete2(stagingFullPath_, true /*deleteReadyOnly*/);
                    WriteInfo(
                        TraceComponent,
                        "Deleted temp staging file path due to write error: sessionId {0} startPosition:{1} endPosition:{2} error{3} stagingFilePath{4}",
                        sessionId_,
                        startPosition_,
                        endPosition_,
                        error, 
                        stagingFullPath_);
                }

                return error;
            }

            // For non-empty file, make sure content written is of expected size
            uint64 expectedSize = 0;
            if (!(startPosition_ == 0 && endPosition_ == 0))
            {
                expectedSize = (endPosition_ - startPosition_ + 1);
            }
            else
            {
                // Get the file size from the metadata since the expected size can be 0 or 1 byte
                UploadSessionMetadataSPtr uploadSessionMetadata;
                auto metadataError = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, uploadSessionMetadata);
                if (metadataError.IsSuccess())
                {
                    expectedSize = uploadSessionMetadata->FileSize;
                }
                else
                {
                    WriteWarning(
                        TraceComponent,
                        "Unable to get metadata for validation of chunk size for sessionId:{0} startPosition:{1} endPosition:{2} stagingFilePath:{3}",
                        sessionId_,
                        startPosition_,
                        endPosition_,
                        stagingFullPath_);

                    return metadataError;
                }
            }

            if (bytesWritten != expectedSize)
            {
                WriteWarning(
                    TraceComponent,
                    "Expected staging file size {0} bytes is not written to staging file. actual chunk size(bytes):{1} sessionId:{2} startPosition:{3} endPosition:{4} stagingFilePath:{5}",
                    expectedSize,
                    bytesWritten,
                    sessionId_,
                    startPosition_,
                    endPosition_,
                    stagingFullPath_);

                wstring msg = wformatString(
                    StringResource::Get(IDS_FSS_UnexpectedStagingChunkSize),
                    bytesWritten,
                    expectedSize);

                // Use retryable error so that file will be sent again
                return ErrorCode(ErrorCodeValue::OperationCanceled, move(msg));
            }

            fileUptr->Flush();
            fileUptr->Close();
        }
        else
        {
            // file exists: proceed to update metadata info about the chunk
            return ErrorCodeValue::AlreadyExists;
        }

        return error;
    }

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    Management::FileStoreService::UploadChunkContentRequest uploadChunkContentRequest_;
    Common::Guid sessionId_;
    uint64 startPosition_;
    uint64 endPosition_;
    std::wstring stagingFullPath_;
};


ProcessUploadChunkContentRequestAsyncOperation::ProcessUploadChunkContentRequestAsyncOperation(
    __in RequestManager & requestManager,
    Management::FileStoreService::UploadChunkContentRequest && uploadChunkContentRequest,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, L"", OperationKind::UploadChunkContent, move(receiverContext), activityId, timeout, callback, parent)
    , uploadChunkContentRequest_(move(uploadChunkContentRequest))
{
    wstring tempChunkFileLocation(wformatString("{0}_{1}", uploadChunkContentRequest_.SessionId, uploadChunkContentRequest_.StartPosition));
    stagingFullPath_ = Path::Combine(requestManager.LocalStagingLocation, tempChunkFileLocation);
#if !defined(PLATFORM_UNIX)
    // This takes care of staging file path that is greater than MAX_PATH chars.
    // For more info: https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file
    stagingFullPath_.insert(0, L"\\\\?\\");
#endif
    sessionId_ = uploadChunkContentRequest_.SessionId;
    startPosition_ = uploadChunkContentRequest_.StartPosition;
    endPosition_ = uploadChunkContentRequest_.EndPosition;
}

ProcessUploadChunkContentRequestAsyncOperation::~ProcessUploadChunkContentRequestAsyncOperation()
{
}

ErrorCode ProcessUploadChunkContentRequestAsyncOperation::ValidateRequest()
{
    if (uploadChunkContentRequest_.SessionId == Common::Guid::Empty()
        || uploadChunkContentRequest_.StartPosition > uploadChunkContentRequest_.EndPosition)
    {
        WriteWarning(
            TraceComponent,
            "UploadChunkContent validate request failed. Returning InvalidArgument error. sessionId:{0}, startPostion:{1}, endPosition:{2}",
            uploadChunkContentRequest_.SessionId,
            uploadChunkContentRequest_.StartPosition,
            uploadChunkContentRequest_.EndPosition);

        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessUploadChunkContentRequestAsyncOperation::BeginOperation(
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UploadChunkContentAsyncOperation>(
        this->RequestManagerObj,
        move(uploadChunkContentRequest_),
        this->sessionId_,
        this->startPosition_,
        this->endPosition_,
        this->stagingFullPath_,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessUploadChunkContentRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = UploadChunkContentAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }

    return errorCode;
}

void ProcessUploadChunkContentRequestAsyncOperation::OnRequestCompleted(__inout ErrorCode & error)
{
    ProcessRequestAsyncOperation::OnRequestCompleted(error);

    // For error case, delete the staging file
    // For error with FileStoreServiceNotReady, gateway would retry to upload to the new primary
    // For error with operationsPending which means it is unable to obtain file lock then do not delete the staging file
    if (!this->stagingFullPath_.empty() && 
        !error.IsSuccess() && 
        !error.IsError(ErrorCodeValue::FileStoreServiceNotReady) &&
        !error.IsError(ErrorCodeValue::OperationsPending))
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

void ProcessUploadChunkContentRequestAsyncOperation::WriteTrace(ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "UploadChunkContent request failed with error {0}, sessionId:{1}, startPostion:{2}, endPosition:{3}",
            error,
            this->sessionId_,
            this->startPosition_,
            this->endPosition_);
    }
}
