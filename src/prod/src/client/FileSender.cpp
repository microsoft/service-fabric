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

StringLiteral const TraceComponent("FileSender");

class FileSender::FileJobItem : public JobItem < FileSender >
{
    DENY_COPY(FileJobItem)

public:
    FileJobItem(ProcessingCallback const & callback)
        : callback_(callback)
    {
    }

    virtual void Process(FileSender & fileSender)
    {
        UNREFERENCED_PARAMETER(fileSender);
        callback_();
    }

private:
    ProcessingCallback callback_;
};

class FileSender::SendAsyncOperation :
    public TimedAsyncOperation,
    private TextTraceComponent < TraceTaskCodes::FileTransfer >
{
    DENY_COPY(SendAsyncOperation)

public:
    SendAsyncOperation(
        FileSender & owner,
        Guid const & operationId,
        ISendTarget::SPtr const & sendTarget,
        wstring const & serviceName,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent),
        owner_(owner),
        operationId_(operationId),
        sendTarget_(sendTarget),
        currentGateway_(),
        serviceName_(serviceName),
        sourceFullPath_(sourceFullPath),
        storeRelativePath_(storeRelativePath),
        shouldOverwrite_(shouldOverwrite),
        progressHandler_(move(progressHandler)),
        completedOrCanceled_(false),
        fileSize_(0)
    {
    }

    virtual ~SendAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SendAsyncOperation>(operation);
        return thisPtr->Error;
    }

    void ProcessReply(AsyncOperationSPtr const & thisSPtr, ErrorCode const & errorCode)
    {
        if (errorCode.IsSuccess())
        {
            IFileStoreServiceClientProgressEventHandlerPtr progressHandler;
            {
                AcquireExclusiveLock lock(lock_);
                progressHandler = progressHandler_;
            }

            if (progressHandler.get() != nullptr)
            {
                progressHandler->IncrementReplicatedFiles(1);
            }
        }

        TryComplete(thisSPtr, errorCode);
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.State.Value != FabricComponentState::Opened)
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: FileSender is not in opened state.",
                operationId_);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidState);
            return;
        }

        TimedAsyncOperation::OnStart(thisSPtr);

        bool success = owner_.jobQueue_->Enqueue(
            make_unique<FileJobItem>([thisSPtr, this]() { this->StartProcessing(thisSPtr); }));
        if (!success)
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Failed to enque the operation to JobQueue.",
                operationId_);
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }
    }

    void OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();

        {
            AcquireExclusiveLock lock(lock_);
            completedOrCanceled_.store(true);
            owner_.activeSends_.Remove(operationId_);
            progressHandler_ = IFileStoreServiceClientProgressEventHandlerPtr();
        }

        WriteInfo(
            TraceComponent,
            owner_.traceId_,
            "{0}: Removed operationId from ActiveSends.",
            operationId_);

        operationCompleted_.Set();
    }

private:
    void StartProcessing(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        IFileStoreServiceClientProgressEventHandlerPtr progressHandler;
        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_.load()) { return; }

            error = owner_.activeSends_.TryAdd(
                operationId_,
                thisSPtr);

            WriteTrace(
                error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
                TraceComponent,
                owner_.traceId_,
                "{0}: Added operationId to ActiveSends: {1}. SourceFullPath: {2}, StoreRelativePath: {3}",
                operationId_,
                error,
                sourceFullPath_,
                storeRelativePath_);

            progressHandler = progressHandler_;
        }

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        File file;
        error = file.TryOpen(
            sourceFullPath_,
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read,
            FileAttributes::SequentialScan);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Failed to open file '{0}' because of {1}.",
                operationId_,
                sourceFullPath_,
                error);
            this->TryComplete(thisSPtr, error);
            return;
        }

        fileSize_ = file.size();
        size_t maxAllowedBytes = Utility::GetMessageContentThreshold();
        int64 remainingBytesToSend = fileSize_;
        uint64 sequenceNumber = 0;

        if (progressHandler.get() != nullptr)
        {
            progressHandler->IncrementTotalTransferItems(remainingBytesToSend);
            progressHandler->IncrementTotalFiles(1);
        }

        bool isLast = false;
        while (!completedOrCanceled_.load() && !isLast)
        {
            DWORD bytesRead;

            int bufferSize = static_cast<int>(maxAllowedBytes);
            if (remainingBytesToSend <= numeric_limits<int>::max())
            {
                int remainingBytes = static_cast<int>(remainingBytesToSend);
                if (bufferSize > remainingBytes)
                {
                    bufferSize = remainingBytes;
                }
            }

            shared_ptr<vector<BYTE>> buffer = make_shared<vector<BYTE>>(bufferSize);
            error = file.TryRead2(buffer->data(), bufferSize, bytesRead);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: TryRead of '{1}' failed with {2}.",
                    operationId_,
                    sourceFullPath_,
                    error);
                this->TryComplete(thisSPtr, error);
                return;
            }

            TESTASSERT_IF(bufferSize != static_cast<int>(bytesRead), "BufferSize should be equal to BytesRead. BufferSize={0}, BytesRead={1}", bufferSize, bytesRead);
            if (bufferSize != static_cast<int>(bytesRead))
            {
                buffer->resize(bytesRead);
            }

            remainingBytesToSend -= bytesRead;
            isLast = (remainingBytesToSend == 0);

            ASSERT_IF(remainingBytesToSend < 0, "remainingBytesToSend cannot be negative");

            error = UploadBytes(buffer, sequenceNumber, isLast);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: UploadBytes failed with {1}.",
                    operationId_,
                    error);
                this->TryComplete(thisSPtr, error);
                return;
            }

            if (progressHandler.get() != nullptr)
            {
                progressHandler->IncrementTransferCompletedItems(bytesRead);
            }
            
            sequenceNumber++;
        }

        operationCompleted_.WaitOne();
    }

    ErrorCode UploadBytes(shared_ptr<vector<BYTE>> const & buffer, uint64 const sequenceNumber, bool const isLast)
    {
        ErrorCode error;
        do
        {
            ClientServerRequestMessageUPtr fileContentMessage = FileTransferTcpMessage::GetFileContentMessage(
                buffer,
                sequenceNumber,
                isLast,
                operationId_,
                Actor::FileReceiver);
            AddRequiredHeaders(*fileContentMessage, sequenceNumber);

            WriteNoise(
                TraceComponent,
                owner_.traceId_,
                "{0}: Sending {1} bytes with SequenceNumber {2}.",
                operationId_,
                buffer->size(),
                sequenceNumber);

            if (sendTarget_)
            {
                if (owner_.transport_)
                {
                    error = owner_.transport_->SendOneWay(sendTarget_, move(fileContentMessage), RemainingTime);
                }
                else
                {
                    wstring traceMessage = wformatString("BeginSendFileToClient cannot be called from client");
                    WriteError(TraceComponent, owner_.traceId_, "{0}", traceMessage);
                    Assert::TestAssert("{0}", traceMessage);

                    error = ErrorCodeValue::OperationFailed;
                }
            }
            else
            {
                if (owner_.connection_)
                {
                    error = owner_.connection_->SendOneWayToGateway(move(fileContentMessage), currentGateway_, RemainingTime);
                }
                else
                {
                    wstring traceMessage = wformatString("BeginSendFileToGateway cannot be called from gateway");
                    WriteError(TraceComponent, owner_.traceId_, "{0}", traceMessage);
                    Assert::TestAssert("{0}", traceMessage);

                    error = ErrorCodeValue::OperationFailed;
                }
            }

            if (!error.IsSuccess() && IsRetryable(error))
            {
                auto interval = min<int64>(ClientConfig::GetConfig().RetryBackoffInterval.TotalMilliseconds(), this->RemainingTime.TotalMilliseconds());

                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: SendOneWay during UploadBytes failed with {1}. Retrying after {2} milliseconds.",
                    operationId_,
                    error,
                    interval);
                ::Sleep(static_cast<DWORD>(interval));
            }
            else
            {
                break;
            }
        } while (!completedOrCanceled_.load());

        return error;
    }

    void AddRequiredHeaders(ClientServerRequestMessage & message, uint64 const sequenceNumber)
    {
        // Add Timeout and FileUploadRequest header only for the first message
        if (sequenceNumber == 0)
        {
            message.Headers.Replace(TimeoutHeader(this->RemainingTime));
            message.Headers.Replace(FileUploadRequestHeader(serviceName_, storeRelativePath_, shouldOverwrite_));
        }

        if (owner_.includeClientVersionHeaderInMessage_)
        {
            message.Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);
        }
    }

    bool IsRetryable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
        case ErrorCodeValue::TransportSendQueueFull:
        case ErrorCodeValue::NotReady:
            return true;
        }

        return false;
    }

private:
    wstring const serviceName_;
    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;
    Guid const operationId_;

    ISendTarget::SPtr sendTarget_;
    unique_ptr<GatewayDescription> currentGateway_;

    ExclusiveLock lock_;
    ManualResetEvent operationCompleted_;
    Common::atomic_bool completedOrCanceled_;

    FileSender & owner_;
    int64 fileSize_;
};

FileSender::FileSender(
    ClientConnectionManagerSPtr const & connection,
    bool const includeClientVersionHeaderInMessage,
    wstring const & traceId,
    ComponentRoot const & root)
    : RootedObject(root)
    , connection_(connection)
    , transport_()
    , includeClientVersionHeaderInMessage_(includeClientVersionHeaderInMessage)
    , traceId_(traceId)
    , jobQueue_()
{    
}

FileSender::FileSender(
    ClientServerTransportSPtr const & transport,
    bool const includeClientVersionHeaderInMessage,
    wstring const & traceId,
    ComponentRoot const & root)
    : RootedObject(root)
    , connection_()
    , transport_(transport)
    , includeClientVersionHeaderInMessage_(includeClientVersionHeaderInMessage)
    , traceId_(traceId)
    , jobQueue_()
{
}

unique_ptr<FileSender> FileSender::CreateOnClient(
    ClientConnectionManagerSPtr const & connection, 
    bool const includeClientVersionHeaderInMessage,
    std::wstring const & traceId,
    Common::ComponentRoot const & root)
{
    return unique_ptr<FileSender>(new FileSender(connection, includeClientVersionHeaderInMessage, traceId, root));
}

unique_ptr<FileSender> FileSender::CreateOnGateway(
    ClientServerTransportSPtr const & transport,
    bool const includeClientVersionHeaderInMessage,
    std::wstring const & traceId,
    Common::ComponentRoot const & root)
{
    return unique_ptr<FileSender>(new FileSender(transport, includeClientVersionHeaderInMessage, traceId, root));
}

ErrorCode FileSender::OnOpen()
{
    jobQueue_ = make_unique<CommonJobQueue<FileSender>>(
        L"FileSenderJobQueue",
        *this,
        false /* forceEnqueue */,
        ClientConfig::GetConfig().MaxFileSenderThreads /* maxThreads */);

    return ErrorCodeValue::Success;
}

ErrorCode FileSender::OnClose()
{
    jobQueue_->Close();

    auto pendingOperations = activeSends_.Close();
    for(auto & operation : pendingOperations)
    {
        operation.second->Cancel();
    }

    return ErrorCodeValue::Success;
}

void FileSender::OnAbort()
{
    OnClose();
}

void FileSender::ProcessMessage(Transport::MessageUPtr && message, ReceiverContextUPtr &&)
{
    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;

    AsyncOperationSPtr operation;
    if (!activeSends_.TryGet(operationId, operation))
    {
        WriteInfo(
            TraceComponent,
            traceId_,
            "Dropping message for OperationId:{0} with Action:{1} and Actor:{2}. The operation is not longer pending in FileSender",
            operationId,
            message->Action,
            message->Actor);
        return;
    }

    auto sendOperation = AsyncOperation::Get<SendAsyncOperation>(operation);

    if (message->Action == FileTransferTcpMessage::ClientOperationFailureAction)
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

        sendOperation->ProcessReply(operation, error);
    }
    else if (message->Action == FileTransferTcpMessage::ClientOperationSuccessAction)
    {
        sendOperation->ProcessReply(operation, ErrorCodeValue::Success);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "Action:'{0}' is not supported by Actor:{1}. Dropping message for OperationId:{2}.",
            message->Action,
            message->Actor,
            operationId);
        return;
    }
}

Common::AsyncOperationSPtr FileSender::BeginSendFileToGateway(
    Common::Guid const & operationId,
    std::wstring const & serviceName,
    std::wstring const & sourceFullPath,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<SendAsyncOperation>(
        *this,
        operationId,
        nullptr,
        serviceName,
        sourceFullPath,
        storeRelativePath,
        shouldOverwrite,
        move(progressHandler),
        timeout,
        callback,
        parent);
}

ErrorCode FileSender::EndSendFileToGateway(Common::AsyncOperationSPtr const &operation)
{
    return SendAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr FileSender::BeginSendFileToClient(
    Common::Guid const & operationId,
    ISendTarget::SPtr const & sendTarget,
    std::wstring const & serviceName,
    std::wstring const & sourceFullPath,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<SendAsyncOperation>(
        *this,
        operationId,
        sendTarget,
        serviceName,
        sourceFullPath,
        storeRelativePath,
        shouldOverwrite,
        IFileStoreServiceClientProgressEventHandlerPtr(),
        timeout,
        callback,
        parent);
}

ErrorCode FileSender::EndSendFileToClient(Common::AsyncOperationSPtr const &operation)
{
    return SendAsyncOperation::End(operation);
}

ErrorCode FileSender::EndSendFile(Common::AsyncOperationSPtr const &operation)
{
    return SendAsyncOperation::End(operation);
}
