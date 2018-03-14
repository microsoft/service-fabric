// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <fstream>

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("ProcessCommitUploadSessionRequestAsyncOperation");

class ProcessCommitUploadSessionRequestAsyncOperation::CommitUploadSessionAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
public:

    CommitUploadSessionAsyncOperation(
        RequestManager & requestManager,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        Common::ActivityId const & activityId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , requestManager_(requestManager)
        , timeoutHelper_(timeout)
        , storeRelativePath_(storeRelativePath)
        , sessionId_(sessionId)
        , activityId_(activityId)
        , joinedFileName_()
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitUploadSessionAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        std::vector<std::wstring> sortedStagingLocation;
        ErrorCode error = this->GetSortedStagingLocation(sortedStagingLocation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto joinChunksOperation = this->BeginJoinChunksAsyncOperation(
            sortedStagingLocation,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnJoinChunkOperationCompleted(operation, false);
            },
            thisSPtr);

        this->OnJoinChunkOperationCompleted(joinChunksOperation, true);
    }

private:

    ErrorCode GetSortedStagingLocation(__out std::vector<std::wstring> & sortedStagingLocation)
    {
        UploadSessionMetadataSPtr metadata;
        ErrorCode error = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, metadata);
        if (!error.IsSuccess())
        {
            return error;
        }
 
        error = metadata->GetSortedStagingLocation(sortedStagingLocation);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (sortedStagingLocation.size() == 0)
        {
            return ErrorCodeValue::NotFound;
        }

        return ErrorCodeValue::Success;
    }

    AsyncOperationSPtr BeginJoinChunksAsyncOperation(
        std::vector<std::wstring> const & sortedStagingLocation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        this->joinedFileName_ = Path::Combine(this->requestManager_.get_LocalStagingLocation(), Common::Guid::NewGuid().ToString());
        ofstream joinedFile;
#if !defined(PLATFORM_UNIX)
        joinedFile.open(this->joinedFileName_ , ios::out | ios::binary | ios::app);
#else
        std::string joinedFileName(this->joinedFileName_.begin(), this->joinedFileName_.end());
        joinedFile.open(joinedFileName.c_str(), ios::out | ios::binary | ios::app);
#endif
        if (joinedFile.is_open())
        {     
            for (auto it = sortedStagingLocation.begin(); it != sortedStagingLocation.end(); ++it)
            {
                ifstream chunkStream;
                string chunkName((*it).begin(), (*it).end());
                chunkStream.open(chunkName, ios::in | ios::binary);
                if (chunkStream.is_open())
                {
                    chunkStream.seekg(0, ios::end);
                    uint64 size = chunkStream.tellg();
                    chunkStream.seekg(ios::beg);
                    char *chunkBuffer = new char[size];
                    chunkStream.read(chunkBuffer, size);
                    joinedFile.write(chunkBuffer, size);
                    delete(chunkBuffer);
                    chunkStream.close();
                }
            }
        }

        joinedFile.close();
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    void OnJoinChunkOperationCompleted(
        Common::AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        auto error = CompletedAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto fileUploadOperation = AsyncOperation::CreateAndStart<FileUploadAsyncOperation>(
            this->requestManager_,
            this->joinedFileName_,
            this->storeRelativePath_,
            true,
            this->requestManager_.GetNextFileVersion(),
            this->activityId_,
            this->timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnFileUploadOperationCompleted(operation, false); },
            thisSPtr);

        this->OnFileUploadOperationCompleted(fileUploadOperation, true);
    }

    void OnFileUploadOperationCompleted(
        Common::AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        ErrorCode error = FileUploadAsyncOperation::End(operation);
        this->TryComplete(thisSPtr, error);
    }

    RequestManager & requestManager_;
    TimeoutHelper timeoutHelper_;
    Common::Guid sessionId_;
    std::wstring storeRelativePath_;
    std::wstring joinedFileName_;
    Common::ActivityId activityId_;
};

ProcessCommitUploadSessionRequestAsyncOperation::ProcessCommitUploadSessionRequestAsyncOperation(
    __in RequestManager & requestManager,
    UploadSessionRequest && request,
    IpcReceiverContextUPtr && receiverContext,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(requestManager, request.StoreRelativePath, OperationKind::CommitUploadSession, move(receiverContext), activityId, timeout, callback, parent)
    , sessionId_(request.SessionId)
{
}

ProcessCommitUploadSessionRequestAsyncOperation::~ProcessCommitUploadSessionRequestAsyncOperation()
{
}

ErrorCode ProcessCommitUploadSessionRequestAsyncOperation::ValidateRequest()
{    
    if (this->sessionId_ == Common::Guid::Empty())
    {
        WriteInfo(
            TraceComponent,
            this->RequestManagerObj.TraceId,
            "Empty session Id");
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ProcessCommitUploadSessionRequestAsyncOperation::BeginOperation(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CommitUploadSessionAsyncOperation>(
        this->RequestManagerObj,
        this->StoreRelativePath,
        this->sessionId_,
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        callback,
        parent);
}

ErrorCode ProcessCommitUploadSessionRequestAsyncOperation::EndOperation(
    __out Transport::MessageUPtr & reply,
    Common::AsyncOperationSPtr const & asyncOperation)
{
    auto errorCode = CommitUploadSessionAsyncOperation::End(asyncOperation);
    if (errorCode.IsSuccess())
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
    }

    return errorCode;
}
