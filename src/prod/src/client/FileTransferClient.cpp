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
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileTransferClient");
StringLiteral const RetryTimerTag("FileTransferClient.RetryTimer");

class FileTransferClient::BaseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Client>
{
    DENY_COPY(BaseAsyncOperation)

public:
    BaseAsyncOperation(
        FileTransferClient & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        ErrorCode const & passThroughError)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        passThroughError_(passThroughError),
        operationId_(Guid::NewGuid()),
        lock_(),
        completedOrCanceled_(false),
        retryTimer_()
    {
    }

    TimeSpan const GetRemainingTime() const { return timeoutHelper_.GetRemainingTime(); }

    virtual void ProcessReply(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        TryComplete(thisSPtr, error);
    }

protected:
    virtual void OnStartOperation(AsyncOperationSPtr const & thisSPtr) = 0;

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (passThroughError_.IsSuccess())
        {
            this->StartOperation(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, passThroughError_);
        }
    }    

    virtual void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(lock_);
            completedOrCanceled_.store(true);
            snap = move(retryTimer_);
        }

        if (snap)
        {
            snap->Cancel();
        }

        owner_.pendingOperations_.Remove(operationId_);
    }

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
    {
        TimeSpan delay = ClientConfig::GetConfig().RetryBackoffInterval;
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::OperationCanceled;
        }

        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_.load())
            {
                retryTimer_ = Timer::Create(
                    RetryTimerTag,
                    [this, thisSPtr, callback](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    callback(thisSPtr);
                });
                retryTimer_->Change(delay);
            }
        }

        return ErrorCodeValue::Success;
    }

private:
    void StartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_.load()) { return; }

            error = owner_.pendingOperations_.TryAdd(
                operationId_,
                thisSPtr);
        }

        if (error.IsSuccess())
        {
            this->OnStartOperation(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

protected:
    FileTransferClient & owner_;
    Guid const operationId_;
    TimeoutHelper timeoutHelper_;
    ErrorCode passThroughError_;

    ExclusiveLock lock_;
    TimerSPtr retryTimer_;
    Common::atomic_bool completedOrCanceled_;
};

class FileTransferClient::UploadAsyncOperation
    : public BaseAsyncOperation
{
    DENY_COPY(UploadAsyncOperation)

public:
    UploadAsyncOperation(
        FileTransferClient & owner,
        wstring const & serviceName,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        ErrorCode const & passThroughError)
        : BaseAsyncOperation(owner, timeout, callback, parent, passThroughError),
        serviceName_(serviceName),
        sourceFullPath_(sourceFullPath),
        storeRelativePath_(storeRelativePath),
        shouldOverwrite_(shouldOverwrite),
        progressHandler_(move(progressHandler))
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

    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {    
        auto operation = owner_.fileSender_->BeginSendFileToGateway(
            operationId_,
            serviceName_,
            sourceFullPath_,
            storeRelativePath_,
            shouldOverwrite_,
            move(progressHandler_),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnFileSendCompleted(operation); },
            thisSPtr);

        bool cancelOperation = true;
        {
            AcquireExclusiveLock lock(this->lock_);
            if (!completedOrCanceled_.load())
            {
                senderAsyncOperation_ = operation;
                cancelOperation = false;
            }
        }

        if (cancelOperation)
        {
            operation->Cancel();
            return;
        }
    }

    void OnCompleted()
    {
        BaseAsyncOperation::OnCompleted();

        TimerSPtr timer;
        AsyncOperationSPtr senderAsyncOperation;
        {
            AcquireExclusiveLock lock(lock_);
            timer = move(timer_);
            senderAsyncOperation = move(senderAsyncOperation_);
        }

        if (timer)
        {
            timer->Cancel();
        }

        if (senderAsyncOperation)
        {
            senderAsyncOperation->Cancel();
        }
    }

private:
    void OnFileSendCompleted(AsyncOperationSPtr const & operation)
    {
        {
            AcquireExclusiveLock lock(this->lock_);
            senderAsyncOperation_.reset();
        }

        auto error = owner_.fileSender_->EndSendFileToGateway(operation);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        auto thisSPtr = operation->Parent;
        TimerSPtr timer = Timer::Create(
            "FileUploadTimeout",
            [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                TryComplete(thisSPtr, ErrorCodeValue::Timeout);
            },
            false);

        bool cancelTimer = false;
        {
            AcquireExclusiveLock lock(lock_);
            if (completedOrCanceled_.load())
            {
                cancelTimer = true;
            }
            else
            {
                timer_ = move(timer);
            }
        }

        if (cancelTimer)
        {
            timer->Cancel();
        }
    }

private:
    wstring const serviceName_;
    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;

    TimerSPtr timer_;
    AsyncOperationSPtr senderAsyncOperation_;
};

class FileTransferClient::DownloadAsyncOperation
    : public BaseAsyncOperation
{
    DENY_COPY(DownloadAsyncOperation)

public:
    DownloadAsyncOperation(
        FileTransferClient & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        wstring const & destinationFullPath,
        StoreFileVersion const & version,
        vector<wstring> const & availableShares,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        ErrorCode const & passThroughError)
        : BaseAsyncOperation(owner, timeout, callback, parent, passThroughError),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        destinationFullPath_(destinationFullPath),
        version_(version),
        availableShares_(availableShares)
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
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.fileReceiver_->BeginReceiveFile(
            operationId_,
            destinationFullPath_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnFileReceiveCompleted(operation); },
            thisSPtr);

        bool cancelOperation = true;
        {
            AcquireExclusiveLock lock(this->lock_);
            if (!completedOrCanceled_.load())
            {
                receiverAsyncOperation_ = operation;
                cancelOperation = false;
            }
        }

        if (cancelOperation)
        {
            operation->Cancel();
            return;
        }

        SendDownloadMessage(thisSPtr);
    }

    void OnCompleted()
    {
        BaseAsyncOperation::OnCompleted();

        AsyncOperationSPtr receiverAsyncOperation;
        {
            AcquireExclusiveLock lock(lock_);
            receiverAsyncOperation = move(receiverAsyncOperation_);
        }

        if (receiverAsyncOperation)
        {
            receiverAsyncOperation->Cancel();
        }

        BaseAsyncOperation::OnCompleted();
    }

private:
    void OnFileReceiveCompleted(AsyncOperationSPtr const & operation)
    {
        {
            AcquireExclusiveLock lock(this->lock_);
            receiverAsyncOperation_.reset();
        }

        auto error = owner_.fileReceiver_->EndReceiveFile(operation);

        TryComplete(operation->Parent, error);
    }

    void SendDownloadMessage(AsyncOperationSPtr const & thisSPtr)
    {
        ClientServerRequestMessageUPtr message = BuildMessage();
        
        unique_ptr<GatewayDescription> unused;
        auto error = owner_.connection_->SendOneWayToGateway(move(message), unused, timeoutHelper_.GetRemainingTime());

        if(!error.IsSuccess() && IsRetryable(error))
        {
            error = TryScheduleRetry(
                thisSPtr,
                [this](AsyncOperationSPtr const & thisSPtr) { this->SendDownloadMessage(thisSPtr); });               
        }

        if (!error.IsSuccess())
        {
            // Could not send download message and hence cancel the
            // operation on FileReceiver
            AsyncOperationSPtr operation;
            {
                AcquireExclusiveLock lock(this->lock_);
                operation = move(receiverAsyncOperation_);
            }

            if (operation) { operation->Cancel(); }

            TryComplete(thisSPtr, error);
            return;
        }
    }

    ClientServerRequestMessageUPtr BuildMessage()
    {
        auto message = FileTransferTcpMessage::GetFileDownloadMessage(
            serviceName_,
            storeRelativePath_,
            version_,
            availableShares_,
            operationId_,
            Actor::FileTransferGateway);
        message->Headers.Replace(TimeoutHeader(timeoutHelper_.GetRemainingTime()));
        message->Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);

        return move(message);
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
    wstring const storeRelativePath_;
    wstring const destinationFullPath_;
    StoreFileVersion const version_;
    vector<wstring> availableShares_;

    AsyncOperationSPtr receiverAsyncOperation_;
};


FileTransferClient::FileTransferClient(
    ClientConnectionManagerSPtr const & connection,
    ComponentRoot const & root)
    : RootedObject(root)
    , connection_(connection)
    , transport_(connection->Transport)
    , fileSender_()
    , fileReceiver_()
    , pendingOperations_()
{
    fileSender_ = FileSender::CreateOnClient(
        connection_,
        true,
        this->Root.TraceId,
        this->Root);

    fileReceiver_ = make_unique<FileReceiver>(
        transport_,
        true,
        L"" /*working dir*/,
        this->Root.TraceId,        
        this->Root);
}

ErrorCode FileTransferClient::OnOpen()
{  
    auto error = fileSender_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "FileSender failed to Open. Error:{0}",
            error);
        return error;
    }

    error = fileReceiver_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "FileReceiver failed to Open. Error:{0}",
            error);
        return error;
    }

    this->RegisterMessageHandlers();

    return ErrorCodeValue::Success;
}

ErrorCode FileTransferClient::OnClose()
{
    this->UnregisterMessageHandlers();

    auto error = fileSender_->Close();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "FileSender failed to Close. Error:{0}",
            error);
        return error;
    }

    error = fileReceiver_->Close();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "FileReceiver failed to Close. Error:{0}",
            error);
        return error;
    }

    pendingOperations_.Close();

    return ErrorCodeValue::Success;
}

void FileTransferClient::OnAbort()
{
    this->UnregisterMessageHandlers();

    fileSender_->Abort();

    fileReceiver_->Abort();

    pendingOperations_.Close();
}

void FileTransferClient::RegisterMessageHandlers()
{
    auto selfRoot = this->Root.CreateComponentRoot();
    connection_->Transport->RegisterMessageHandler(
        Actor::Enum::FileTransferClient,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiveContext)
        {
            this->OnFileStoreClientManagerMessageReceived(move(message), move(receiveContext));
        },
        false /*dispatchOnTrasportThread*/);

    connection_->Transport->RegisterMessageHandler(
        Actor::Enum::FileSender,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiveContext)
        {
            this->OnFileSenderMessageReceived(move(message), move(receiveContext));
        },
        false /*dispatchOnTrasportThread*/);

    // We SHOULD dispatch on transport thread since it is required that all
    // file parts are received in-order by FileReceiver
    //
    connection_->Transport->RegisterMessageHandler(
        Actor::Enum::FileReceiver,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiveContext)
        {
            this->OnFileReceiverMessageReceived(move(message), move(receiveContext));
        },
        true /*dispatchOnTrasportThread*/);
}

void FileTransferClient::UnregisterMessageHandlers()
{
    connection_->Transport->UnregisterMessageHandler(Actor::Enum::FileTransferClient);
    connection_->Transport->UnregisterMessageHandler(Actor::Enum::FileSender);
    connection_->Transport->UnregisterMessageHandler(Actor::Enum::FileReceiver);
}

bool FileTransferClient::ValidateRequiredHeaders(Message & message)
{
    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "FabricActivityHeader is not found in the message. MessageId: {0}",
            message.MessageId);
        return false;
    }

    return true;
}

void FileTransferClient::OnFileStoreClientManagerMessageReceived(MessageUPtr && message, ReceiverContextUPtr &&)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = Transport::FabricActivityHeader::FromMessage(*message).ActivityId.Guid;

    ErrorCode error;
    if (message->Action == FileTransferTcpMessage::ClientOperationFailureAction)
    {
        if (!FileTransferTcpMessage::TryGetErrorOnClientOperationFailureAction(*message, error))
        {
            WriteWarning(
                TraceComponent,
                this->Root.TraceId,
                "TryGetErrorOnClientOperationFailureAction failed to get the ErrorCode. Dropping message for OperationId:{0} with Action:{1} and Actor:{2}.",
                operationId,
                message->Action,
                message->Actor);
            return;
        }
    }
    else if (message->Action == FileTransferTcpMessage::ClientOperationSuccessAction)
    {
        error = ErrorCodeValue::Success;
    }
    else
    {
        WriteWarning(
            TraceComponent,
            this->Root.TraceId,
            "Action:'{0}' is not supported by Actor:{1}. Dropping MessageId:{2}.",
            message->Action,
            message->Actor,
            message->MessageId);
        return;
    }

    AsyncOperationSPtr operation;
    if (pendingOperations_.TryGet(operationId, operation))
    {
        auto uploadOperation = AsyncOperation::Get<BaseAsyncOperation>(operation);
        uploadOperation->ProcessReply(operation, error);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            this->Root.TraceId,
            "Dropping message for OperationId:{0} with Action:{1} and Actor:{2}. The operation is not longer pending in FileTransferClient",
            operationId,
            message->Action,
            message->Actor);
        return;
    }
}

void FileTransferClient::OnFileSenderMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    fileSender_->ProcessMessage(move(message), move(receiverContext));
}

void FileTransferClient::OnFileReceiverMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiveContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    fileReceiver_->ProcessMessage(move(message), move(receiveContext));
}

AsyncOperationSPtr FileTransferClient::BeginUploadFile(
    wstring const & serviceName,
    wstring const & sourceFullPath,
    wstring const & storeRelativePath,
    bool shouldOverwrite,
    IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent,
    ErrorCode const & passThroughError)
{
    return AsyncOperation::CreateAndStart<UploadAsyncOperation>(
        *this,
        serviceName,
        sourceFullPath,
        storeRelativePath,
        shouldOverwrite,
        move(progressHandler),
        timeout,
        callback,
        parent,
        passThroughError);
}

ErrorCode FileTransferClient::EndUploadFile(AsyncOperationSPtr const &operation)
{
    return UploadAsyncOperation::End(operation);
}

AsyncOperationSPtr FileTransferClient::BeginDownloadFile(
    wstring const & serviceName,
    wstring const & storeRelativePath,
    wstring const & destinationFullPath,
    StoreFileVersion const & version,
    vector<wstring> const & availableShares,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent,
    ErrorCode const & passThroughError)
{
    return AsyncOperation::CreateAndStart<DownloadAsyncOperation>(
        *this,
        serviceName,
        storeRelativePath,
        destinationFullPath,
        version,
        availableShares,
        timeout,
        callback,
        parent,
        passThroughError);
}

ErrorCode FileTransferClient::EndDownloadFile(AsyncOperationSPtr const &operation)
{
    return DownloadAsyncOperation::End(operation);
}
