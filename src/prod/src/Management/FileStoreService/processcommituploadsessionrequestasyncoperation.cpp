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

    __declspec(property(get = get_ReplicatedStoreWrapper)) ReplicatedStoreWrapper & ReplicatedStoreWrapperObj;
    inline ReplicatedStoreWrapper & get_ReplicatedStoreWrapper() const { return *(this->requestManager_.ReplicaStoreWrapperUPtr); };

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        static int count = 0;
        ++count;
        if (FileStoreServiceConfig::GetConfig().EnableChaosDuringFileUpload && count == 19)
        {
            WriteWarning(
                TraceComponent,
                this->requestManager_.TraceId,
                "Chaos enabled: Sending ErrorCodeValue::NotFound to simulate failover for CommitUploadSessionAsyncOperation: {0} {1}",
                this->sessionId_,
                this->storeRelativePath_);

            this->TryComplete(thisSPtr, ErrorCodeValue::NotFound);
            return;
        }

        // Check if the file is already committed state for this session Id. If so, return success.
        FileMetadata metadata(this->storeRelativePath_);
        auto error = this->ReplicatedStoreWrapperObj.ReadExact(this->activityId_, metadata);
        if (error.IsSuccess() && metadata.UploadRequestId == sessionId_)
        {
            if (FileState::IsStable(metadata.State))
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                return;
            }
        }

        // Gather staged chunk files in order to assemble them
        std::vector<std::wstring> sortedStagingLocation;
        uint64 fileSize;
        error = this->GetSortedStagingLocation(sortedStagingLocation, fileSize);
        if (!error.IsSuccess())
        {
            UploadSessionMetadataSPtr uploadSessionMetadata;
            auto sessionMetadataError = GetUploadSessionMapEntry(uploadSessionMetadata);
            if (sessionMetadataError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "Getting Staging location failed. sessionId:{0} storePath:{1}: error:{2} UploadSessionMetadata:{3}.",
                    this->sessionId_,
                    this->storeRelativePath_,
                    error,
                    uploadSessionMetadata);
            }
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto joinChunksOperation = this->BeginJoinChunksAsyncOperation(
            sortedStagingLocation,
            fileSize,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnJoinChunkOperationCompleted(operation, false);
            },
            thisSPtr);

        this->OnJoinChunkOperationCompleted(joinChunksOperation, true);
    }

private:

    ErrorCode GetUploadSessionMapEntry(__inout UploadSessionMetadataSPtr &uploadSessionMetadata)
    {
        auto error = this->requestManager_.UploadSessionMap->GetUploadSessionMapEntry(this->sessionId_, uploadSessionMetadata);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Unable to get session map data. sessionId:{0} storePath:{1}: error:{2} ",
                this->sessionId_,
                this->storeRelativePath_,
                error);
        }

        return error;
    }

    ErrorCode GetSortedStagingLocation(__out std::vector<std::wstring> & sortedStagingLocation, __out uint64 & fileSize)
    {
        fileSize = 0;
        UploadSessionMetadataSPtr metadata;
        ErrorCode error = GetUploadSessionMapEntry(metadata);
        if (!error.IsSuccess())
        {
            return error;
        }

        fileSize = metadata->FileSize;

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
        uint64 const& fileSize,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        this->joinedFileName_ = Path::Combine(this->requestManager_.get_LocalStagingLocation(), Common::Guid::NewGuid().ToString());

#if !defined(PLATFORM_UNIX)
        wstring longpathPrefix(L"\\\\?\\");
        this->joinedFileName_.insert(0, longpathPrefix);
#endif

        if (fileSize == 0)
        {
            File::Touch(this->joinedFileName_);
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
        }

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
                    auto chunkBuffer = std::make_unique<char[]>(size);
                    chunkStream.read(chunkBuffer.get(), size);
                    joinedFile.write(chunkBuffer.get(), size);
                    chunkStream.close();
                }
                else
                {
                    joinedFile.close();

                    bool exists = File::Exists(*it);
                    WriteWarning(
                        TraceComponent,
                        "Opening of staging file failed:{0} exists:{1}",
                        *it,
                        exists);

                    if (File::Exists(this->joinedFileName_))
                    {
                        auto deleteError = File::Delete2(this->joinedFileName_, false);
                        if (!deleteError.IsSuccess())
                        {
                            WriteWarning(
                                TraceComponent,
                                "Deleting joined file:{0}, Error:{1}",
                                this->joinedFileName_,
                                deleteError);
                        }
                    }

                    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::NotFound, callback, parent);
                }
            }
            joinedFile.close();
        }
        else
        {
            bool exists = File::Exists(this->joinedFileName_);
            WriteWarning(
                TraceComponent,
                "Opening of merge file failed:{0}, exists:{1}",
                this->joinedFileName_,
                exists);

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::NotFound, callback, parent);
        }

        // Validate file size after joining all the staged files
        auto mergedFile = make_unique<File>();
        auto openError = mergedFile->TryOpen(
            this->joinedFileName_,
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read,
#if defined(PLATFORM_UNIX)
            FileAttributes::Normal
#else
            FileAttributes::ReadOnly
#endif
        );

        if (!openError.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->requestManager_.TraceId,
                "Can't validate size for file {0}. Open file failed with {1}.",
                this->joinedFileName_,
                openError);
        }
        else
        {
            uint64 mergedFileSize = static_cast<uint64>(mergedFile->size());
            mergedFile->Close2();

            if (mergedFileSize != fileSize)
            {
                WriteWarning(
                    TraceComponent,
                    this->requestManager_.TraceId,
                    "Merged filesize:{0} didn't match the expected size:{1} for SessionId:{2} storeRelativePath:{3} mergedFileName:{4} totalChunkLocation:{5}",
                    mergedFileSize,
                    fileSize,
                    this->sessionId_,
                    this->storeRelativePath_,
                    this->joinedFileName_,
                    sortedStagingLocation.size());

                wstring msg = wformatString(
                    StringResource::Get(IDS_FSS_UnexpectedMergeSize),
                    this->storeRelativePath_,
                    fileSize,
                    mergedFileSize);

                ErrorCode invalidArgError(ErrorCodeValue::InvalidArgument, move(msg));
                // Return invalid argument since client will try to resend the file again.
                return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(move(invalidArgError), callback, parent);
            }            
        }

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
            this->sessionId_,
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

        if (!error.IsSuccess())
        {
            if (File::Exists(this->joinedFileName_))
            {
                auto deleteError = File::Delete2(this->joinedFileName_, false);
                if (!deleteError.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        "Deleting joined file:{0}, Error:{1}",
                        this->joinedFileName_,
                        deleteError);
                }
            }
        }

        this->TryComplete(thisSPtr, move(error));
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

    // For simple transaction, transactions are processed in groups.
    // If one of them fail, all of the transactions are canceled.
    // It is possible for a transaction to be inactive resulting in incomplete rollback of metadata where metadata is left in unstable state.
    // In case of error, unstable state metadata entry should not be present in the store and must be deleted (in a separate transaction).
    if (!errorCode.IsSuccess())
    {
        if (DeleteIfMetadataNotInStableState(asyncOperation))
        {
            WriteWarning(
                TraceComponent,
                "Metadata was not in stable state and was deleted for sessionId:{0} storeRelativePath:{1}, Error:{2}",
                this->sessionId_,
                this->StoreRelativePath,
                errorCode);
        }
    }

    if (errorCode.IsSuccess() || errorCode.IsError(ErrorCodeValue::AlreadyExists))
    {
        reply = FileStoreServiceMessage::GetClientOperationSuccess(this->ActivityId);
        return ErrorCodeValue::Success;
    }

    return errorCode;
}

void ProcessCommitUploadSessionRequestAsyncOperation::WriteTrace(ErrorCode const &error)
{
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "CommitUploadSession request failed with error {0}, sessionId:{1}",
            error,
            this->sessionId_);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "CommitUploadSession request Succeeded with error {0}, sessionId:{1}",
            error,
            this->sessionId_);
    }
}
