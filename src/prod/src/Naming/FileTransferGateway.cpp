// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Client;
using namespace Transport;
using namespace std;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Management;
using namespace FileStoreService;
using namespace Api;
using namespace ClientServerTransport;
using namespace Naming;
using namespace Hosting2;

StringLiteral const TraceComponent("FileTransferGateway");

class FileTransferGateway::UploadChunkAsyncOperation
    : public AsyncOperation
    , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
{
    DENY_COPY(UploadChunkAsyncOperation)

public:
    UploadChunkAsyncOperation(
        __in FileTransferGateway & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        bool const shouldOverwrite,
        uint64 const fileSize,
        Guid const & operationId,
        ISendTarget::SPtr const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        ActivityTracedComponent(owner.Instance.ToString(), FabricActivityHeader(operationId)),
        owner_(owner),
        operationId_(operationId),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        shouldOverwrite_(shouldOverwrite),
        fileSize_(fileSize),
        target_(target),
        timeoutHelper_(timeout),
        completedOrCanceled_(false),
        fileChunkMaxSequenceId_(0)
    {
        tempFilePath_ = Path::Combine(owner.workingDir_, Guid::NewGuid().ToString());
    }

    virtual ~UploadChunkAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UploadChunkAsyncOperation>(operation);
        return thisPtr->Error;
    }

    // Process requests

    void ProcessCreateRequest(Common::AsyncOperationSPtr const & thisSPtr)
    {
        SendFileCreateMessageToFileStoreService(thisSPtr);
    }

    void ProcessChunkRequest(AsyncOperationSPtr const & thisSPtr, MessageUPtr && message, ReceiverContextUPtr && receiverContext)
    {
        if (completedOrCanceled_.load()) { return; }

        auto clonedMessage = message->Clone();
        unique_ptr<JobItem<FileTransferGateway>> temp = make_unique<FileChunkJobItem>(thisSPtr, move(clonedMessage), move(receiverContext));

        if (!this->fileChunkJobQueue_->Enqueue(move(temp)))
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "{0}: Could not enque message in JobQueue. storeRelativePath: {1}",
                operationId_,
                storeRelativePath_);
        }
    }

    void ProcessCommitRequest(Common::AsyncOperationSPtr const & thisSPtr)
    {
        if (completedOrCanceled_.load()) { return; }

        SendFileCommitMessageToFileStoreService(thisSPtr);
    }

    void ProcessCommitAckRequest(Common::AsyncOperationSPtr const & thisSPtr)
    {
        if (completedOrCanceled_.load()) { return; }

        Cleanup(thisSPtr);
    }

    void ProcessDeleteUploadSessionRequest(Common::AsyncOperationSPtr const & thisSPtr)
    {
        if (completedOrCanceled_.load()) { return; }

        SendDeleteUploadSessionToFileStoreService(thisSPtr);
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "OnStart(UploadChunkAsyncOperation): operationId:{0} TempFileName:{1}",
            operationId_,
            tempFilePath_);

        auto addError = owner_.pendingOperations_.TryAdd(operationId_, thisSPtr);
        if (!addError.IsSuccess())
        {
            WriteTrace(
                LogLevel::Error,
                TraceComponent,
                TraceId,
                "Unable to add to the pending operation map : error:{0} operationId:{1}  storePath:{2}",
                addError,
                operationId_,
                storeRelativePath_);

            TryComplete(thisSPtr, move(addError));
            return;
        }

        fileChunkJobQueue_ = make_unique<CommonJobQueue<FileTransferGateway>>(
            L"FileChunkJobQueue",
            owner_,
            false /* forceEnqueue */,
            ClientConfig::GetConfig().MaxFileChunkReceiverThreads);

        // Send file create message to file store service
        SendFileCreateMessageToFileStoreService(thisSPtr);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (!Error.IsError(ErrorCodeValue::OperationsPending))
        {
            if (File::Exists(tempFilePath_))
            {
                File::Delete2(tempFilePath_, true /*deleteReadyOnly*/);
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "Deleted FileTransfer temp path: tempFilepath:{0} storeRelativePath:{1} operationId:{2}",
                    tempFilePath_,
                    storeRelativePath_,
                    operationId_);
            }
        }
    }

private:

    class FileChunkJobItem : public JobItem<FileTransferGateway>
    {
        DENY_COPY(FileChunkJobItem)

    public:
        FileChunkJobItem(AsyncOperationSPtr const & thisSPtr, MessageUPtr && message, ReceiverContextUPtr && receiverContext)
            : thisSPtr_(thisSPtr)
            , message_(move(message))
            , receiverContext_(move(receiverContext))
        {
        }

        virtual void Process(FileTransferGateway &)
        {
            auto uploadChunkAsyncOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(thisSPtr_);

            FileSequenceHeader sequenceHeader;
            if (!message_->Headers.TryReadFirst(sequenceHeader))
            {
                WriteWarning(
                    TraceComponent,
                    "ProcessFileChunk",
                    "Dropping message for operationId:{0}. Failed to get FileSequenceHeader.",
                    uploadChunkAsyncOperation->operationId_);
                return;
            }

            uploadChunkAsyncOperation->ProcessFileContent(
                thisSPtr_,
                move(message_),
                sequenceHeader.SequenceNumber,
                sequenceHeader.BufferSize);
        }

    private:
        AsyncOperationSPtr thisSPtr_;
        MessageUPtr message_;
        ReceiverContextUPtr receiverContext_;
    };

    ErrorCode WriteChunkFile(wstring const & chunkFileLocation, vector<BYTE> const & writeBuffer, uint64 const sequenceId)
    {
        ErrorCode error = ErrorCodeValue::Success;
        FileWriterLock fileLock(chunkFileLocation);
        if (!fileLock.Acquire(TimeSpan::FromSeconds(Constants::ChunkFileLockAcquireTimeoutInSeconds)).IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to get file lock operationId:{0}: sequenceId:{1}. Filename: {2}.",
                operationId_,
                sequenceId,
                chunkFileLocation);

            return ErrorCodeValue::OperationsPending;
        }

        // Create chunk file if it doesn't exist already
        if (!File::Exists(chunkFileLocation))
        {
            auto fileUptr = make_unique<File>();
            error = fileUptr->TryOpen(chunkFileLocation, FileMode::CreateNew, FileAccess::Write, FileShare::Write);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "{0}: TryOpen for sequenceId:{1} failed with {2}. ",
                    operationId_,
                    sequenceId,
                    error);
                return error;
            }

            ErrorCode writeError;
            DWORD bytesWritten;
            writeError = fileUptr->TryWrite2(writeBuffer.data(), static_cast<int>(writeBuffer.size()), bytesWritten);
            if (!writeError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "{0}: TryWrite2 for sequenceId:{1} failed with {2}. ",
                    operationId_,
                    sequenceId,
                    writeError);

                fileUptr->Close();

                if (File::Exists(chunkFileLocation))
                {
                    File::Delete2(chunkFileLocation, true /*deleteReadyOnly*/);
                    WriteInfo(
                        TraceComponent,
                        TraceId,
                        "Deleted FileTransfer temp path: tempFilepath {0} storeRelativePath:{1} operationId:{2}",
                        chunkFileLocation,
                        storeRelativePath_,
                        operationId_);
                }

                return writeError;
            }

            fileUptr->Flush();
            fileUptr->Close();
        }
        else
        {
            // file exists: proceed to UploadChunksToFileStoreService
        }

        return error;
    }

    void ProcessFileContent(
        AsyncOperationSPtr const & thisSPtr,
        MessageUPtr && message,
        uint64 const sequenceId,
        uint64 const totalBufferSize)
    {
        WriteNoise(
            TraceComponent,
            TraceId,
            "{0}: Received SequenceNumber:{1}.",
            operationId_,
            sequenceId);

        // This is just providing a way to switch to old behavior(SMB copy) in case of any issues with message based upload
        if (FileStoreServiceConfig::GetConfig().UseChunkContentInTransportMessage)
        {
            auto chunkMessage = message->Clone();
            UploadChunkContentToFileStoreService(thisSPtr, sequenceId, move(chunkMessage));
        }
        else
        {
            // Uses SMB copy for chunk contents to transfer to FSS(P)
            vector<const_buffer> body;
            if (!message->GetBody(body))
            {
                WriteWarning(
                    TraceComponent,
                    "ProcessFileChunk",
                    "Dropping message for operationId:{0}. GetBody() failed with message status {1}.",
                    operationId_,
                    message->Status);
                return;
            }

            vector<BYTE> writeBuffer;
            writeBuffer.reserve(totalBufferSize);
            for (auto i = 0; i < body.size(); ++i)
            {
                auto bufLen = body[i].len;
                writeBuffer.insert(writeBuffer.end(), &(body[i].buf[0]), &(body[i].buf[bufLen]));
            }

            wstring tempChunkLocation(wformatString("{0}.{1}", tempFilePath_, sequenceId));
            auto error = WriteChunkFile(tempChunkLocation, writeBuffer, sequenceId);
            if (!error.IsSuccess())
            {
                owner_.ReplyToSender(target_, operationId_, sequenceId, error, timeoutHelper_.GetRemainingTime());
                return;
            }

            UploadChunksToFileStoreService(thisSPtr, sequenceId, tempChunkLocation);
        }
    }

    void SendFileCreateMessageToFileStoreService(AsyncOperationSPtr const& thisSPtr)
    {
        auto operation = owner_.fileStoreServiceClient_->BeginCreateUploadSession(
            owner_.fssPartitionGuid_,
            storeRelativePath_,
            operationId_,
            fileSize_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCreateUploadSessionCompleted(operation, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnCreateUploadSessionCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCreateUploadSessionCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndCreateUploadSession(operation);
        if (!owner_.pendingOperations_.Contains(operationId_))
        {
            return;
        }

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(SendFileCreateMessageToFileStoreService): error:{0}, storeRelativePath:{1}, operationId:{2}",
            error,
            storeRelativePath_,
            operationId_);

        owner_.ReplyToSender(target_, operationId_, std::numeric_limits<uint64>::max(), move(error), timeoutHelper_.GetRemainingTime());
        return;
    }

    void UploadChunksToFileStoreService(AsyncOperationSPtr const& thisSPtr, uint64 const sequenceId, wstring const & tempChunkLocation)
    {
        auto maxAllowedBytes = static_cast<size_t>(
            static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) * ServiceModelConfig::GetConfig().MessageContentBufferRatio);

        uint64 startPosition = 0, endPosition = 0;
        if (fileSize_ != 0)
        {
            startPosition = (sequenceId * maxAllowedBytes);
            uint64 expectedEndposition = (startPosition + maxAllowedBytes - 1);
            endPosition = expectedEndposition < static_cast<uint64>(fileSize_ - 1) ? expectedEndposition : static_cast<uint64>(fileSize_ - 1);
        }

        // Update the max chunk sequence number
        UpdateMaxSequenceId(sequenceId);

        WriteNoise(
            TraceComponent,
            TraceId,
            "BeginUploadChunk(UploadChunksToFileStoreService): storeRelativePath:{0}, operationId:{1}",
            storeRelativePath_,
            operationId_);

        auto operation = owner_.fileStoreClient_->BeginUploadChunk(
            tempChunkLocation,
            operationId_,
            startPosition,
            endPosition,
            timeoutHelper_.GetRemainingTime(),
            [this, sequenceId](AsyncOperationSPtr const & operation)
        {
            this->OnUploadChunkCompleted(operation, false /*expectedCompletedSynchronously*/, sequenceId);
        },
            thisSPtr);
        OnUploadChunkCompleted(operation, true /*expectedCompletedSynchronously*/, sequenceId);
    }

    void OnUploadChunkCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously,
        uint64 const sequenceId)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreClient_->EndUploadChunk(operation);
        if (!owner_.pendingOperations_.Contains(operationId_))
        {
            return;
        }

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(OnUploadChunkCompleted): error:{0}, storeRelativePath:{1}, operationId:{2} sequenceId:{3}",
            error,
            storeRelativePath_,
            operationId_,
            sequenceId);

        owner_.ReplyToSender(target_, operationId_, sequenceId, move(error), timeoutHelper_.GetRemainingTime());
        return;
    }

    void UploadChunkContentToFileStoreService(AsyncOperationSPtr const& thisSPtr, uint64 const sequenceId, MessageUPtr && message)
    {
        auto maxAllowedBytes = static_cast<size_t>(
            static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) * ServiceModelConfig::GetConfig().MessageContentBufferRatio);

        uint64 startPosition = 0, endPosition = 0;
        if (fileSize_ != 0)
        {
            startPosition = (sequenceId * maxAllowedBytes);
            uint64 expectedEndposition = (startPosition + maxAllowedBytes - 1);
            endPosition = expectedEndposition < static_cast<uint64>(fileSize_ - 1) ? expectedEndposition : static_cast<uint64>(fileSize_ - 1);
        }

        // Update the max chunk sequence number
        UpdateMaxSequenceId(sequenceId);

        WriteNoise(
            TraceComponent,
            TraceId,
            "BeginUploadChunkContent(UploadChunkContentToFileStoreService): storeRelativePath:{0}, operationId:{1}",
            storeRelativePath_,
            operationId_);

        Management::FileStoreService::UploadChunkContentDescription uploadChunkContentDescription(operationId_, startPosition, endPosition);
        auto operation = owner_.fileStoreServiceClient_->BeginUploadChunkContent(
            owner_.fssPartitionGuid_,
            message,
            uploadChunkContentDescription,
            timeoutHelper_.GetRemainingTime(),
            [this, sequenceId](AsyncOperationSPtr const & operation)
        {
            this->OnUploadChunkContentCompleted(operation, sequenceId, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnUploadChunkContentCompleted(operation, sequenceId, true /*expectedCompletedSynchronously*/);
    }

    void OnUploadChunkContentCompleted(
        AsyncOperationSPtr const & operation,
        uint64 const sequenceId,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndUploadChunkContent(operation);
        if (!owner_.pendingOperations_.Contains(operationId_))
        {
            return;
        }

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(OnUploadChunkContentCompleted): error:{0}, storeRelativePath:{1}, operationId:{2} sequenceId:{3}",
            error,
            storeRelativePath_,
            operationId_,
            sequenceId);

        owner_.ReplyToSender(target_, operationId_, sequenceId, move(error), timeoutHelper_.GetRemainingTime());
        return;
    }

    void SendFileCommitMessageToFileStoreService(AsyncOperationSPtr const& thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            TraceId,
            "BeginCommitUploadSession(SendFileCommitMessageToFileStoreService): storeRelativePath:{0}, operationId:{1}",
            storeRelativePath_,
            operationId_);

        auto operation = owner_.fileStoreServiceClient_->BeginCommitUploadSession(
            owner_.fssPartitionGuid_,
            storeRelativePath_,
            operationId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCommitUploadSessionCompleted(operation, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnCommitUploadSessionCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCommitUploadSessionCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndCommitUploadSession(operation);
        if (!owner_.pendingOperations_.Contains(operationId_))
        {
            return;
        }

        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(SendFileCommitMessageToFileStoreService): error:{0}, storeRelativePath:{1}, operationId:{2}",
            error,
            storeRelativePath_,
            operationId_);

        owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);
    }

    void SendDeleteUploadSessionToFileStoreService(AsyncOperationSPtr const& thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            TraceId,
            "BeginDeleteUploadSession(SendDeleteUploadSessionToFileStoreService): storeRelativePath:{0}, operationId:{1}",
            storeRelativePath_,
            operationId_);

        auto operation = owner_.fileStoreServiceClient_->BeginDeleteUploadSession(
            owner_.fssPartitionGuid_,
            operationId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteUploadSessionCompleted(operation, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnDeleteUploadSessionCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnDeleteUploadSessionCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndDeleteUploadSession(operation);
        if (owner_.pendingOperations_.Contains(operationId_))
        {
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                TraceId,
                "End(SendDeleteUploadSessionToFileStoreService): error:{0}, storeRelativePath:{1}, operationId:{2}",
                error,
                storeRelativePath_,
                operationId_);

            completedOrCanceled_.store(true);
            owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);

            if (owner_.pendingOperations_.Remove(operationId_))
            {
                // Cancel all other pending operations
                DeleteChunkTempFiles();
                if (operation)
                {
                    operation->Parent->Cancel(true);

                    WriteNoise(
                        TraceComponent,
                        TraceId,
                        "SendDeleteUploadSessionToFileStoreService done: storeRelativePath:{0}, operationId:{1}",
                        storeRelativePath_,
                        operationId_);
                }
            }
        }
        else
        {
            //  It is possible that some request may have been sent after the FSS upload completion. We can ignore it.
        }
    }

    void DeleteChunkTempFiles()
    {
        for (auto i = 0; i <= fileChunkMaxSequenceId_.load(); ++i)
        {
            wstring tempChunkLocation(wformatString("{0}.{1}", tempFilePath_, i));
            FileWriterLock fileLock(tempChunkLocation);
            {
                if (!fileLock.Acquire(TimeSpan::FromSeconds(Constants::ChunkFileLockAcquireTimeoutInSeconds)).IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        TraceId,
                        "Unable to get file lock operationId:{0}: sequenceId:{1}. filename:{2}.",
                        operationId_,
                        i,
                        tempChunkLocation);

                    return;
                }

                if (File::Exists(tempChunkLocation))
                {
                    auto deleteError = File::Delete2(tempChunkLocation, true /*deleteReadyOnly*/);
                    WriteInfo(
                        TraceComponent,
                        TraceId,
                        "Deleted FileTransfer temp path: tempFilepath:{0} storeRelativePath:{1} operationId:{2} error:{3}",
                        tempChunkLocation,
                        storeRelativePath_,
                        operationId_,
                        deleteError);
                }
            }
        }
    }

    void Cleanup(AsyncOperationSPtr const & operation)
    {
        SendDeleteUploadSessionToFileStoreService(operation);
    }

    void UpdateMaxSequenceId(uint64 const sequenceId)
    {
        // Update the max chunk sequence number
        bool done = false;
        LONG currentSequenceId = static_cast<LONG>(sequenceId);
        do
        {
            LONG currentMaxSequenceId = static_cast<LONG>(fileChunkMaxSequenceId_.load());
            if (currentMaxSequenceId < currentSequenceId)
            {
                done = fileChunkMaxSequenceId_.compare_exchange_weak(currentMaxSequenceId, currentSequenceId);
            }
            else
            {
                done = true;
            }
        } while (!done);
    }

private:
    Guid const operationId_;
    wstring const serviceName_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    wstring tempFilePath_;
    uint64 fileSize_;

    ISendTarget::SPtr target_;
    TimeoutHelper timeoutHelper_;
    FileTransferGateway & owner_;
    Common::atomic_bool completedOrCanceled_;

    // Max sequence id received for the file which obtained during upload chunk request.
    // This is primarily used for deleting the chunk temp files. Since taking lock in the upload chunk path is expensive,
    // this max sometimes may not reflect the actual max. But that is fine, since cleanup here is a best effort cleanup.
    // Any files that are not cleaned up, will be cleaned up when gateway closes.
    Common::atomic_long fileChunkMaxSequenceId_;

    // Transport threads just queue message to this queue for performance.
    // Threadpool threads for the queue process the chunks and upload to FSS
    std::unique_ptr<Common::CommonJobQueue<FileTransferGateway>> fileChunkJobQueue_;
};

class FileTransferGateway::UploadAsyncOperation
    : public AsyncOperation
    , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
{
    DENY_COPY(UploadAsyncOperation)

public:
    UploadAsyncOperation(
        FileTransferGateway & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        bool const shouldOverwrite,
        Guid const & operationId,
        ISendTarget::SPtr const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        ActivityTracedComponent(owner.Instance.ToString(), FabricActivityHeader(operationId)),
        owner_(owner),
        operationId_(operationId),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        shouldOverwrite_(shouldOverwrite),
        target_(target),
        tempFilePath_(Path::Combine(owner.workingDir_, Guid::NewGuid().ToString())),
        timeoutHelper_(timeout)
    {
    }

    virtual ~UploadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UploadAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Being(ReceiveFile): TempFileName:{0}",
            tempFilePath_);

        auto operation = owner_.fileReceiver_->BeginReceiveFile(
            operationId_,
            tempFilePath_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnReceiveCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnReceiveCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (File::Exists(tempFilePath_))
        {
            File::Delete2(tempFilePath_, true /*deleteReadyOnly*/);
        }
    }

private:
    void OnReceiveCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileReceiver_->EndReceiveFile(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(ReceiveFile): Error:{0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, move(error));
            return;
        }

        UploadToFileStoreService(operation->Parent);
    }

    void UploadToFileStoreService(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(UploadToFileStoreService): StoreRelativePath:{0}, ShouldOverwrite:{1}",
            storeRelativePath_,
            shouldOverwrite_);

        auto operation = owner_.fileStoreClient_->BeginUpload(
            tempFilePath_,
            storeRelativePath_,
            shouldOverwrite_,
            Api::IFileStoreServiceClientProgressEventHandlerPtr(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUploadCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnUploadCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnUploadCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreClient_->EndUpload(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(UploadToFileStoreService): Error:{0}",
            error);

        owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_, true);

        TryComplete(operation->Parent, move(error));
    }

private:
    Guid const operationId_;
    wstring const serviceName_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    wstring const tempFilePath_;

    ISendTarget::SPtr target_;

    TimeoutHelper timeoutHelper_;
    FileTransferGateway & owner_;
};

class FileTransferGateway::DownloadAsyncOperation
    : public AsyncOperation
    , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
{
    DENY_COPY(DownloadAsyncOperation)

public:
    DownloadAsyncOperation(
        FileTransferGateway & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        StoreFileVersion const & storeFileVersion,
        vector<std::wstring> && availableShares,
        Guid const & operationId,
        ISendTarget::SPtr const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        ActivityTracedComponent(owner.Instance.ToString(), FabricActivityHeader(operationId)),
        owner_(owner),
        operationId_(operationId),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        storeFileVersion_(storeFileVersion),
        availableShares_(move(availableShares)),
        target_(target),
        tempFilePath_(File::GetTempFileNameW(owner.workingDir_)),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DownloadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(DownloadFromFileStoreService): StoreRelativePath:{0}, StoreFileVersion:{1}, AvailableShareCount:{2}",
            storeRelativePath_,
            storeFileVersion_,
            availableShares_.size());

        auto operation = owner_.fileStoreClient_->BeginDownload(
            storeRelativePath_,
            tempFilePath_,
            storeFileVersion_,
            availableShares_,
            IFileStoreServiceClientProgressEventHandlerPtr(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadCompleted(operation, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnDownloadCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (File::Exists(tempFilePath_))
        {
            File::Delete2(tempFilePath_, true /*deleteReadyOnly*/);
        }
    }

private:
    void OnDownloadCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreClient_->EndDownload(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(DownloadFromFileStoreService): Error:{0}",
            error);

        if (!error.IsSuccess())
        {
            owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);
            TryComplete(operation->Parent, move(error));
            return;
        }

        SendFileToClient(operation->Parent);
    }

    void SendFileToClient(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(SendToClient): StoreRelativePath:{0}, SourcrFileName:{1}",
            storeRelativePath_,
            tempFilePath_);

        auto operation = owner_.fileSender_->BeginSendFileToClient(
            operationId_,
            target_,
            serviceName_,
            tempFilePath_,
            storeRelativePath_,
            true,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) 
            { 
                this->OnSendCompleted(operation, false /*expectedCompletedSynchronously*/); 
            },
            thisSPtr);
        OnSendCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnSendCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileSender_->EndSendFileToClient(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(SendToClient): Error:{0}",
            error);
        if (!error.IsSuccess())
        {
            owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);
        }

        TryComplete(operation->Parent, move(error));
    }

private:
    Guid const operationId_;
    wstring const serviceName_;
    wstring const storeRelativePath_;
    wstring const tempFilePath_;

    StoreFileVersion storeFileVersion_;
    vector<wstring> availableShares_;
    ISendTarget::SPtr target_;

    TimeoutHelper timeoutHelper_;
    FileTransferGateway & owner_;
};

FileTransferGateway::FileTransferGateway(
    Federation::NodeInstance const &instance,
    IDatagramTransportSPtr &transport,
    DemuxerUPtr &demuxer,
    ComponentRoot const & root)
    : RootedObject(root)
    , instance_(instance)
    , transport_(transport)
    , demuxer_(demuxer)
    , pendingOperations_()
{
    nativeImageStoreEnabled_ = IsNativeImageStoreEnabled();
}

Common::ErrorCode FileTransferGateway::OnOpen()
{
    if (!nativeImageStoreEnabled_)
    {
        WriteInfo(
            TraceComponent,
            "NativeImageStore is not enabled. Returning Open without registering handlers.");
        return ErrorCodeValue::Success;
    }

    FabricNodeConfigSPtr fabricNodeConfig = make_shared<FabricNodeConfig>();
    workingDir_ = Path::Combine(fabricNodeConfig->WorkingDir, Constants::FileTransferGatewayDirectory);

    if (!Directory::Exists(workingDir_))
    {
        auto error = Directory::Create2(workingDir_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Failed to create directory '{0}' because of {1}.",
                workingDir_,
                error);
            return error;
        }
    }

    clientServerTransport_ = make_shared<Client::ClientServerTcpTransport>(
        this->Root,
        L"FileTransferGateway",
        transport_);
    auto error = clientServerTransport_->Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    fileSender_ = FileSender::CreateOnGateway(
        clientServerTransport_,
        false /*includeClientVersionHeaderInMessage*/,
        instance_.Id.ToString() /*TraceId*/,
        this->Root);

    fileReceiver_ = make_unique<FileReceiver>(
        clientServerTransport_,
        false /*includeClientVersionHeaderInMessage*/,
        workingDir_,
        instance_.Id.ToString() /*TraceId*/,
        this->Root);

    error = ClientFactory::CreateLocalClientFactory(
        fabricNodeConfig,
        clientFactorPtr_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "CreateLocalClientFactory failed. Error:{0}",
            error);

        return error;
    }

    fileStoreClient_ = make_shared<InternalFileStoreClient>(
        *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
        clientFactorPtr_);

    error = clientFactorPtr_->CreateInternalFileStoreServiceClient(fileStoreServiceClient_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "CreateInternalFileStoreServiceClient failed. Error:{0}",
            error);

        return error;
    }

    if (!Guid::TryParse(Constants::FileStoreServiceSingletonPartitionGuid, fssPartitionGuid_))
    {
        WriteWarning(
            TraceComponent,
            "Unable to parse fss partition guid from string:{0}",
            Constants::FileStoreServiceSingletonPartitionGuid);

        Assert::TestAssert("Unable to parse guid: {0}", Constants::FileStoreServiceSingletonPartitionGuid);
        return ErrorCodeValue::OperationFailed;
    }

    error = fileSender_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "FiledSender failed to Open. Error:{0}",
            error);
        return error;
    }


    error = fileReceiver_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "FileReceiver failed to Open. Error:{0}",
            error);
        return error;
    }

    this->RegisterMessageHandlers();

    return ErrorCodeValue::Success;
}

Common::ErrorCode FileTransferGateway::OnClose()
{
    if (!nativeImageStoreEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    this->UnregisterMessageHandlers();

    ErrorCode error;
    if (fileSender_)
    {
        error = fileSender_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FiledSender failed to Close. Error:{0}",
                error);
            return error;
        }
    }

    if (fileReceiver_)
    {
        error = fileReceiver_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FiledReceiver failed to Close. Error:{0}",
                error);
            return error;
        }
    }

    CleanupUploadChunks();
    return clientServerTransport_->Close();
}

void FileTransferGateway::OnAbort()
{
    if (!nativeImageStoreEnabled_)
    {
        return;
    }

    this->UnregisterMessageHandlers();

    if (fileSender_)
    {
        fileSender_->Abort();
    }

    if (fileReceiver_)
    {
        fileReceiver_->Abort();
    }

    CleanupUploadChunks();
}

void FileTransferGateway::CleanupUploadChunks()
{
    auto pendingOperations = pendingOperations_.Close();
    for (auto &v : pendingOperations)
    {
        v.second->Cancel();
    }

    auto error = Directory::Delete(this->workingDir_, true, true);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "Unable to delete FileTransfer location during Close. path:{0}, error:{1}",
            this->workingDir_,
            error);
    }
}

bool FileTransferGateway::IsNativeImageStoreEnabled() const
{
    return StringUtility::StartsWith<wstring>(
        ManagementConfig::GetConfig().ImageStoreConnectionString.GetPlaintext(),
        Management::ImageStore::Constants::NativeImageStoreSchemaTag) || Management::ImageStore::ImageStoreServiceConfig::GetConfig().Enabled;
}

void FileTransferGateway::RegisterMessageHandlers()
{
    auto selfRoot = this->Root.CreateComponentRoot();
    demuxer_->RegisterMessageHandler(
        Actor::FileTransferGateway,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
        OnFileStoreGatewayManagerMessageReceived(move(message), move(receiverContext));
        },
        false /*dispatchOnTrasportThread*/);

    demuxer_->RegisterMessageHandler(
        Actor::FileSender,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
            OnFileSenderMessageReceived(move(message), move(receiverContext));
        },
        false /*dispatchOnTrasportThread*/);

    demuxer_->RegisterMessageHandler(
        Actor::FileReceiver,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
            OnFileReceiverMessageReceived(move(message), move(receiverContext));
        },
        true /*dispatchOnTrasportThread*/);
}

void FileTransferGateway::UnregisterMessageHandlers()
{
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileTransferGateway);
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileSender);
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileReceiver);
}

void FileTransferGateway::OnFileStoreGatewayManagerMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileTransferClient, operationId);
        return;
    }

    if (message->Action == FileTransferTcpMessage::FileDownloadAction)
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                operationId,
                message->MessageId);
            return;
        }

        FileDownloadMessageBody body;
        if (!message->GetBody(body))
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: Failed to get FileDownloadMessageBody. Dropping MessageId:{1}.",
                operationId,
                message->MessageId);
            return;
        }

        if (body.ServiceName != SystemServiceApplicationNameHelper::PublicImageStoreServiceName)
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: ServiceName:{1} is not expected. Dropping MessageId:{2}.",
                operationId,
                body.ServiceName,
                message->MessageId);
            return;
        }

        ProcessFileDownloadRequest(
            move(body.ServiceName),
            move(body.StoreRelativePath),
            move(body.StoreFileVersion),
            move(body.AvailableShares),
            move(operationId),
            move(receiverContext->ReplyTarget),
            timeoutHeader.Timeout);
    }
}

void FileTransferGateway::OnFileSenderMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileReceiver, operationId);
        return;
    }

    fileSender_->ProcessMessage(move(message), move(receiverContext));
}

void FileTransferGateway::OnFileReceiverMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileSender, operationId);
        return;
    }

    if (message->Action == FileTransferTcpMessage::FileContentAction)
    {
        ClientProtocolVersionHeader versionHeader;
        message->Headers.TryReadFirst(versionHeader);

        // Check if we have to use chunk based protocol or the old protocol (single file upload)
        if (!versionHeader.IsChunkedFileUploadSupported())
        {
            // If single file upload protocol is used, make sure that NTLM authentication is not disabled.
            if (FileStoreServiceConfig::GetConfig().DisableNtlmAuthentication)
            {
                WriteError(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: Trying to use single file upload protocol when FSS NTLM is disabled.",
                    operationId);

                ErrorCode error(ErrorCodeValue::AccessDenied, wformatString("{0}", StringResource::Get(IDS_FSS_CLIENT_FSSNtlmAuthenticationDisabled)));
                this->SendResponseToClient(error, receiverContext->ReplyTarget, Actor::FileSender, operationId);
                return;
            }

            // If FileUploadRequestHeader is found in the message
            // then it is a new request to upload a file
            FileUploadRequestHeader uploadRequestHeader;
            if (message->Headers.TryGetAndRemoveHeader(uploadRequestHeader))
            {
                if (uploadRequestHeader.ServiceName != *SystemServiceApplicationNameHelper::PublicImageStoreServiceName)
                {
                    WriteWarning(
                        TraceComponent,
                        Instance.ToString(),
                        "{0}: ServiceName:{1} is not expected. Dropping MessageId:{2}.",
                        operationId,
                        uploadRequestHeader.ServiceName,
                        message->MessageId);
                    return;
                }

                TimeoutHeader timeoutHeader;
                if (!message->Headers.TryReadFirst(timeoutHeader))
                {
                    WriteWarning(
                        TraceComponent,
                        Instance.ToString(),
                        "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                        operationId,
                        message->MessageId);
                    return;
                }

                this->ProcessFileUploadRequest(
                    uploadRequestHeader.ServiceName,
                    uploadRequestHeader.StoreRelativePath,
                    uploadRequestHeader.ShouldOverwrite,
                    operationId,
                    receiverContext->ReplyTarget,
                    timeoutHeader.Timeout);
            }

            fileReceiver_->ProcessMessage(move(message), move(receiverContext));
        }
        else
        {
            AsyncOperationSPtr operation;
            if (pendingOperations_.TryGet(operationId, operation))
            {
                auto uploadChunkOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(operation);
                uploadChunkOperation->ProcessChunkRequest(operation, move(message), move(receiverContext));
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    this->Root.TraceId,
                    "Dropping message for operationId:{0} with Action:{1} and Actor:{2}. Sending reply to the client that operationId is not found.",
                    operationId,
                    message->Action,
                    message->Actor);

                FileSequenceHeader sequenceHeader;
                message->Headers.TryReadFirst(sequenceHeader);

                TimeoutHeader timeoutHeader;
                if (!message->Headers.TryReadFirst(timeoutHeader))
                {
                    WriteWarning(
                        TraceComponent,
                        this->Root.TraceId,
                        "While dropping message for operationId:{0} failed to get timeout.",
                        operationId);

                    return;
                }

                ReplyToSender(receiverContext->ReplyTarget, operationId, sequenceHeader.SequenceNumber, ErrorCodeValue::NotFound, timeoutHeader.GetRemainingTime());
                return;
            }
        }
    }
    else if (message->Action == FileTransferTcpMessage::FileUploadCommitAction)
    {
        AsyncOperationSPtr operation;
        if (pendingOperations_.TryGet(operationId, operation))
        {
            auto uploadChunkOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(operation);
            uploadChunkOperation->ProcessCommitRequest(operation);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                this->Root.TraceId,
                "Dropping message for operationId:{0} with Action:{1} and Actor:{2}. Sending reply to the client that operationId is not found.",
                operationId,
                message->Action,
                message->Actor);

            SendResponseToClient(ErrorCodeValue::NotFound, receiverContext->ReplyTarget, Actor::FileTransferClient, operationId);
            return;
        }
    }
    else if (message->Action == FileTransferTcpMessage::FileCreateAction)
    {
        // If FSS doesn't accept chunk based upload, then report error ConnectionConfirmWaitExpired 
        // where client would retry with different protocol
        if (!FileStoreServiceConfig::GetConfig().AcceptChunkUpload)
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: Trying to use chunk based file upload protocol when it is disabled by the FileStoreService.",
                operationId);

            TimeoutHeader timeoutHeader;
            if (!message->Headers.TryReadFirst(timeoutHeader))
            {
                WriteWarning(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                    operationId,
                    message->MessageId);
                return;
            }

            ReplyToSender(receiverContext->ReplyTarget, operationId, std::numeric_limits<uint64>::max(), ErrorCodeValue::ConnectionConfirmWaitExpired, timeoutHeader.GetRemainingTime());
            return;
        }

        // If FileUploadRequestHeader is found in the message
        // then it is a new request to upload a file
        FileUploadCreateRequestHeader fileUploadCreateRequestHeader;
        if (message->Headers.TryGetAndRemoveHeader(fileUploadCreateRequestHeader))
        {
            if (fileUploadCreateRequestHeader.ServiceName != *SystemServiceApplicationNameHelper::PublicImageStoreServiceName)
            {
                WriteWarning(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: ServiceName:{1} is not expected. Dropping MessageId:{2}.",
                    operationId,
                    fileUploadCreateRequestHeader.ServiceName,
                    message->MessageId);
                return;
            }

            TimeoutHeader timeoutHeader;
            if (!message->Headers.TryReadFirst(timeoutHeader))
            {
                WriteWarning(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                    operationId,
                    message->MessageId);
                return;
            }

            this->ProcessFileChunkUploadRequest(
                fileUploadCreateRequestHeader.ServiceName,
                fileUploadCreateRequestHeader.StoreRelativePath,
                fileUploadCreateRequestHeader.ShouldOverwrite,
                fileUploadCreateRequestHeader.FileSize,
                operationId,
                receiverContext->ReplyTarget,
                timeoutHeader.Timeout);
        }
    }
    else if (message->Action == FileTransferTcpMessage::FileUploadCommitAckAction)
    {
        AsyncOperationSPtr operation;
        if (pendingOperations_.TryGet(operationId, operation))
        {
            auto uploadChunkOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(operation);
            uploadChunkOperation->ProcessCommitAckRequest(operation);
        }
        else
        {
            WriteNoise(
                TraceComponent,
                this->Root.TraceId,
                "Dropping message for operationId:{0} with Action:{1} and Actor:{2}. The operation is no longer pending in FileTransferGateway",
                operationId,
                message->Action,
                message->Actor);
            return;
        }
    }
    else if (message->Action == FileTransferTcpMessage::FileUploadDeleteSessionAction)
    {
        AsyncOperationSPtr operation;
        if (pendingOperations_.TryGet(operationId, operation))
        {
            auto uploadChunkOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(operation);
            uploadChunkOperation->ProcessDeleteUploadSessionRequest(operation);
        }
        else
        {
            WriteNoise(
                TraceComponent,
                this->Root.TraceId,
                "Dropping message for operationId:{0} with Action:{1} and Actor:{2}. The operation is no longer pending in FileTransferGateway",
                operationId,
                message->Action,
                message->Actor);
            return;
        }
    }
    else
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "{0}: Action:'{1}' is not supported by Actor:{2}. Dropping MessageId:{3}.",
            operationId,
            message->Action,
            message->Actor,
            message->MessageId);
    }
}

bool FileTransferGateway::ValidateRequiredHeaders(Message & message)
{
    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "FabricActivityHeader is not found in the message. MessageId: {0}",
            message.MessageId);
        return false;
    }

    ClientProtocolVersionHeader versionHeader;
    if (!message.Headers.TryReadFirst(versionHeader))
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "Message has missing protocol version header: expected = '{1}'. ActivityId:{2}",
            ClientProtocolVersionHeader::CurrentVersionHeader,
            activityHeader);
        return false;
    }

    if (!versionHeader.IsCompatibleVersion())
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "Unsupported protocol version: expected = '{1}' received = '{2}'. ActivityId:{3}",
            ClientProtocolVersionHeader::CurrentMajorVersion,
            versionHeader,
            activityHeader);
        return false;
    }

    return true;
}

bool FileTransferGateway::AccessCheck(Message & message)
{
    bool success;
    if (message.Action == FileTransferTcpMessage::FileDownloadAction)
    {
        success = message.IsInRole(ClientAccessConfig::GetConfig().FileDownloadRoles);
    }
    else if (message.Action == FileTransferTcpMessage::FileContentAction)
    {
        success = message.IsInRole(ClientAccessConfig::GetConfig().FileContentRoles);
    }
    else
    {
        success = message.IsInRole(RoleMask::Admin);
    }

    if (!success)
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "RoleBased AccessCheck failed for Action:'{0}. ActivityId:{1}",
            message.Action,
            FabricActivityHeader::FromMessage(message).ActivityId.Guid);
    }

    return success;
}

void FileTransferGateway::ProcessFileChunkUploadRequest(
    std::wstring const & serviceName,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    uint64 const fileSize,
    Common::Guid const & operationId,
    ISendTarget::SPtr const & target,
    TimeSpan const & timeout)
{
    if (!this->pendingOperations_.Contains(operationId))
    {
        AsyncOperation::CreateAndStart<UploadChunkAsyncOperation>(
            *this,
            serviceName,
            storeRelativePath,
            shouldOverwrite,
            fileSize,
            operationId,
            target,
            timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            UploadChunkAsyncOperation::End(operation);
        },
            this->Root.CreateAsyncOperationRoot());
    }
    else
    {
        AsyncOperationSPtr operation;
        if (pendingOperations_.TryGet(operationId, operation))
        {
            auto uploadChunkOperation = AsyncOperation::Get<UploadChunkAsyncOperation>(operation);
            uploadChunkOperation->ProcessCreateRequest(operation);
        }
    }
}

void FileTransferGateway::ProcessFileUploadRequest(
    std::wstring const & serviceName,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    Common::Guid const & operationId,
    ISendTarget::SPtr const & target,
    TimeSpan const & timeout)
{
    AsyncOperation::CreateAndStart<UploadAsyncOperation>(
        *this,
        serviceName,
        storeRelativePath,
        shouldOverwrite,
        operationId,
        target,
        timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        auto error = UploadAsyncOperation::End(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

void FileTransferGateway::ProcessFileDownloadRequest(
    std::wstring const & serviceName,
    std::wstring const & storeRelativePath,
    Management::FileStoreService::StoreFileVersion const & storeFileVersion,
    std::vector<std::wstring> && availableShares,
    Common::Guid const & operationId,
    ISendTarget::SPtr const & target,
    TimeSpan const & timeout)
{
    AsyncOperation::CreateAndStart<DownloadAsyncOperation>(
        *this,
        serviceName,
        storeRelativePath,
        storeFileVersion,
        move(availableShares),
        operationId,
        target,
        timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        auto error = DownloadAsyncOperation::End(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

ErrorCode FileTransferGateway::ReplyToSender(Transport::ISendTarget::SPtr const & sendTarget, Guid const & operationId, uint64 const sequenceId, ErrorCode const & error, TimeSpan timeout)
{
    ClientServerRequestMessageUPtr message;
    if (error.IsSuccess())
    {
        message = FileTransferTcpMessage::GetSuccessMessage(operationId, Transport::Actor::FileSender);
    }
    else
    {
        message = FileTransferTcpMessage::GetFailureMessage(error, operationId, Transport::Actor::FileSender);
    }
    message->Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);

    auto maxAllowedBytes = static_cast<size_t>(
        static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) * ServiceModelConfig::GetConfig().MessageContentBufferRatio);
    message->Headers.Replace(FileSequenceHeader(sequenceId, false, maxAllowedBytes));

    auto sendError = transport_->SendOneWay(sendTarget, move(message)->GetTcpMessage(), timeout);
    if (!sendError.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "Failed to send reply to sender for operationId:{0}. SendOneWay failed with {1}",
            operationId,
            error);
    }

    return sendError;
}

void FileTransferGateway::SendResponseToClient(ErrorCode const & error, ISendTarget::SPtr target, Actor::Enum const actor, Guid const operationId, bool useSingleFileUploadProtocolVersion)
{
    MessageUPtr replyMessage;
    if (error.IsSuccess())
    {
        replyMessage = FileTransferTcpMessage::GetSuccessMessage(operationId, actor)->GetTcpMessage();
    }
    else
    {
        replyMessage = FileTransferTcpMessage::GetFailureMessage(error, operationId, actor)->GetTcpMessage();
    }

    if (!useSingleFileUploadProtocolVersion)
    {
        replyMessage->Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);
    }
    else
    {
        replyMessage->Headers.Replace(*ClientProtocolVersionHeader::SingleFileUploadVersionHeader);
    }

    auto sendError = transport_->SendOneWay(
        target,
        move(replyMessage));
    if (!sendError.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "SendOneWay in AsyncOperation failed with {0}. operationId:{1}",
            sendError,
            operationId);
    }
}

