// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;
using namespace Naming;

StringLiteral const TraceComponent("FileReceiver");


class FileReceiver::ReceiveAsyncOperation :
public TimedAsyncOperation,
protected TextTraceComponent<TraceTaskCodes::FileTransfer>
{
    DENY_COPY(ReceiveAsyncOperation)

public:
    ReceiveAsyncOperation(
        FileReceiver & owner,
        Guid const & operationId,
        wstring const & destinationFullPath,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , operationId_(operationId)
        , destinationFullPath_(destinationFullPath)
        , tempLocation_(wformatString("{0}.{1}", destinationFullPath, SequenceNumber::GetNext()))
        , completedOrCanceled_(false)
        , sequenceId_(0)
        , fileUPtr_()
    {
        fileUPtr_ = make_unique<File>();

        jobQueue_ = make_unique<CommonJobQueue<FileReceiver>>(
            L"FileReceiverJobQueue",
            owner_,
            false /* forceEnqueue */,
            1 /* maxThreads - Single thread for messages to be processed synchronously */);
    }

    virtual ~ReceiveAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ReceiveAsyncOperation>(operation);
        return thisPtr->Error;
    }

    void ProcessMessage(AsyncOperationSPtr const & thisSPtr, MessageUPtr && message, ReceiverContextUPtr && receiverContext)
    {
        auto clonedMessage = message->Clone();
        unique_ptr<JobItem<FileReceiver>> temp = make_unique<FileJobItem>(thisSPtr, move(clonedMessage), move(receiverContext));
        
        if (!this->jobQueue_->Enqueue(move(temp)))
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Could not enque message in JobQueue.",
                FabricActivityHeader::FromMessage(*message).ActivityId);
        }
    }

    void ProcessMessage(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        TryComplete(thisSPtr, error);
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        ErrorCode error;
        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_) { return; }

            error = owner_.activeDownloads_.TryAdd(
                operationId_,
                thisSPtr);
            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                owner_.traceId_,
                "{0}: Added operationId to ActiveDownload: {1}.",
                operationId_,
                error);

            if (error.IsSuccess())
            {
                error = fileUPtr_->TryOpen(tempLocation_, FileMode::CreateNew, FileAccess::Write, FileShare::Write);
            }
        }

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
    }

    void OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();

        jobQueue_->Close();

        unique_ptr<File> fileSPtr;
        {
            AcquireExclusiveLock lock(lock_);
            completedOrCanceled_ = true;
            owner_.activeDownloads_.Remove(operationId_);
            fileSPtr = move(fileUPtr_);
        }

        WriteInfo(
            TraceComponent,
            owner_.traceId_,
            "{0}: Removed operationId from ActiveDownload.",
            operationId_);

        if (fileSPtr)
        {
            fileSPtr->Close();
        }

        if (File::Exists(tempLocation_))
        {
            File::Delete2(tempLocation_);
        }
    }

private:
    void ProcessFileContent(
        AsyncOperationSPtr const & thisSPtr,
        vector<Common::const_buffer> && buffers,
        uint64 const sequenceId,
        bool const isLast,
        uint64 const totalBufferSize,
        ISendTarget::SPtr const & sendTarget)
    {
        WriteNoise(
            TraceComponent,
            owner_.traceId_,
            "{0}: Received SequenceNumber:{1}.",
            operationId_,
            sequenceId);

        if (sequenceId_.load() != sequenceId)
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Dropping file content since the sequence number is unexpected. Expected={1}, Received={2}.",
                operationId_,
                sequenceId_.load(),
                sequenceId);

            owner_.ReplyToSender(sendTarget, operationId_, ErrorCode(ErrorCodeValue::OperationFailed), RemainingTime);
            return;
        }

        vector<BYTE> writeBuffer;
        writeBuffer.reserve(totalBufferSize);
        for (int i = 0; i < buffers.size(); ++i)
        {
            auto bufLen = buffers[i].len;
            writeBuffer.insert(writeBuffer.end(), &(buffers[i].buf[0]), &(buffers[i].buf[bufLen]));
        }

        ErrorCode writeError;
        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_)
            {
                // The operation is already completed
                // Do not process any more messages
                return;
            }

            ASSERT_IFNOT(fileUPtr_, "The file handle should be present");

            DWORD bytesWritten;
            writeError = fileUPtr_->TryWrite2(writeBuffer.data(), static_cast<int>(writeBuffer.size()), bytesWritten);

            if (isLast)
            {
                fileUPtr_->Close();
                fileUPtr_.reset();
            }
        }

        if (!writeError.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: TryWrite2 for SequenceId:{1} failed with {2}. ",
                operationId_,
                sequenceId,
                writeError);
            owner_.ReplyToSender(sendTarget, operationId_, writeError, RemainingTime);
            TryComplete(thisSPtr, writeError);
            return;
        }

        sequenceId_++;

        if (isLast)
        {
            auto error = File::MoveTransacted(tempLocation_, destinationFullPath_, true);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: MoveTransacted failed with {1}. Source:{2}, Destination:{3}.",
                    operationId_,
                    error,
                    tempLocation_,
                    destinationFullPath_);
            }

            owner_.ReplyToSender(sendTarget, operationId_, error, RemainingTime);

            TryComplete(thisSPtr, error);
        }
    }

private:
    class FileJobItem : public JobItem<FileReceiver>
    {
        DENY_COPY(FileJobItem)
    
    public:
        FileJobItem(AsyncOperationSPtr const & thisSPtr, MessageUPtr && message, ReceiverContextUPtr && receiverContext)
            : thisSPtr_(thisSPtr)
            , message_(move(message))
            , receiverContext_(move(receiverContext))
        {
        }
    
        virtual void Process(FileReceiver & fileReceiver)
        {
            auto receiveOperation = AsyncOperation::Get<ReceiveAsyncOperation>(thisSPtr_);
    
            FileSequenceHeader sequenceHeader;
            if (!message_->Headers.TryReadFirst(sequenceHeader))
            {
                WriteWarning(
                    TraceComponent,
                    fileReceiver.traceId_,
                    "Dropping message for OperationId:{0}. Failed to get FileSequenceHeader.",
                    receiveOperation->operationId_);
                return;
            }
    
            vector<const_buffer> body;
            if (!message_->GetBody(body))
            {
                WriteWarning(
                    TraceComponent,
                    fileReceiver.traceId_,
                    "Dropping message for OperationId:{0}. GetBody() failed with message status {1}.",
                    message_->Status);
                return;
            }
    
            receiveOperation->ProcessFileContent(
                move(thisSPtr_),
                move(body),
                sequenceHeader.SequenceNumber,
                sequenceHeader.IsLast,
                sequenceHeader.BufferSize,
                receiverContext_->ReplyTarget);
        }
    
    private:
        AsyncOperationSPtr thisSPtr_;
        MessageUPtr message_;
        ReceiverContextUPtr receiverContext_;
    };

    Guid const operationId_;
    wstring const tempLocation_;
    wstring const destinationFullPath_;

    atomic_uint64 sequenceId_;

    ExclusiveLock lock_;
    unique_ptr<File> fileUPtr_;
    bool completedOrCanceled_;

    std::unique_ptr<Common::CommonJobQueue<FileReceiver>> jobQueue_;

    FileReceiver & owner_;
};


FileReceiver::FileReceiver(
    std::shared_ptr<Client::ClientServerTransport> const &transport,
    bool const includeClientVersionHeaderInMessage,
    wstring const & workingDir,
    wstring const & traceId,
    ComponentRoot const & root)
    : RootedObject(root)
    , transport_(transport)
    , includeClientVersionHeaderInMessage_(includeClientVersionHeaderInMessage)
    , workingDir_(workingDir)
    , traceId_(traceId)
    , activeDownloads_()
{    
}

ErrorCode FileReceiver::OnOpen()
{
    return ErrorCodeValue::Success;
}

ErrorCode FileReceiver::OnClose()
{
    auto pendingOperations = activeDownloads_.Close();
    for (auto & operation : pendingOperations)
    {
        operation.second->Cancel();
    }

    return ErrorCodeValue::Success;
}

void FileReceiver::OnAbort()
{
    OnClose();
}

ErrorCode FileReceiver::ReplyToSender(Transport::ISendTarget::SPtr const & sendTarget, Guid const & operationId, ErrorCode const & error, TimeSpan timeout)
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

    if (includeClientVersionHeaderInMessage_)
    {
        message->Headers.Replace(*ClientProtocolVersionHeader::SingleFileUploadVersionHeader);
    }

    auto sendError = transport_->SendOneWay(sendTarget, move(message), timeout);
    if(!sendError.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "Failed to send reply to sender for OperationId:{0}. SendOneWay failed with {1}",
            operationId,
            error);
    }

    return sendError;
}

void FileReceiver::ProcessMessage(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "Dropping message for OperationId:{0} since FileReceiver is not in opened state.",
            FabricActivityHeader::FromMessage(*message).ActivityId);
        return;
    }

    Guid operationId = Transport::FabricActivityHeader::FromMessage(*message).ActivityId.Guid;

    AsyncOperationSPtr operation;
    if (!activeDownloads_.TryGet(operationId, operation))
    {
        WriteInfo(
            TraceComponent,
            traceId_,
            "Dropping message for OperationId:{0} with Action:{1} and Actor:{2}. The operation is not longer pending in FileReceiver.",
            operationId,
            message->Action,
            message->Actor);

        ReplyToSender(receiverContext->ReplyTarget, operationId, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    auto receiveOperation = AsyncOperation::Get<ReceiveAsyncOperation>(operation);

    if (message->Action == FileTransferTcpMessage::FileContentAction)
    {
        receiveOperation->ProcessMessage(operation, move(message), move(receiverContext));
    }
    else if (message->Action == FileTransferTcpMessage::ClientOperationFailureAction)
    {
        ErrorCode error;
        if (!FileTransferTcpMessage::TryGetErrorOnClientOperationFailureAction(*message, error))
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "TryGetErrorOnClientOperationFailureAction failed to get the ErrorCode. Dropping message for OperationId:{0} with Action:{1} and Actor:{2}.",
                operationId,
                message->Action,
                message->Actor);
            return;
        }

        receiveOperation->ProcessMessage(operation, error);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "Action:'{0}' is not supported by Actor:{1}. Dropping message for OperationId:{2}.",
            message->Action,
            message->Actor,
            FabricActivityHeader::FromMessage(*message).ActivityId);
    }
}

AsyncOperationSPtr FileReceiver::BeginReceiveFile(
    Guid const & operationId,
    wstring const & destinationFullPath,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ReceiveAsyncOperation>(
        *this,
        operationId,
        destinationFullPath,
        timeout,
        callback,
        parent);
}

ErrorCode FileReceiver::EndReceiveFile(AsyncOperationSPtr const &operation)
{
    return ReceiveAsyncOperation::End(operation);
}
