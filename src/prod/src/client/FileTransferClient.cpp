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
StringLiteral const CommitSendTimerTag("FileTransferClient.CommitSendTimer");
StringLiteral const SingleFileUploadTimerTag("FileTransferClient.SingleFileUploadTimer");

Common::atomic_uint64 FileTransferClient::totalChunkBasedUploads_;

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
            completedOrCanceled_ = true;
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
        return TryScheduleRetry(thisSPtr, callback, ClientConfig::GetConfig().RetryBackoffInterval);
    }

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, TimeSpan delay)
    {
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::Timeout;
        }

        {
            AcquireExclusiveLock lock(lock_);

            if (!completedOrCanceled_)
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

    virtual bool IsRetryable(ErrorCode const & error)
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
    void StartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        {
            AcquireExclusiveLock lock(lock_);

            if (completedOrCanceled_) { return; }

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
    bool completedOrCanceled_;
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
        progressHandler_(move(progressHandler)),
        timer_(),
        commitResponseSendCount_(0),
        gotFileUploadCommitResponse_(false),
        retryResendFileCount_(0),
        useChunkBasedUpload_(owner_.UseChunkBasedUpload)
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

    virtual void ProcessReply(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error) override
    {
        if (!useChunkBasedUpload_.load())
        {
            BaseAsyncOperation::ProcessReply(thisSPtr, error);
            return;
        }

        // For chunk based upload
        if (error.IsError(ErrorCodeValue::OperationsPending))
        {
            if (commitResponseSendCount_.load() >= ClientConfig::GetConfig().FileUploadCommitRetryAttempt)
            {
                owner_.WriteError(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Didn't receive FileUploadCommit response for operationId:{0} StorePath:{1} Errorcode:{2} RetryAttempt:{3}",
                    operationId_,
                    this->storeRelativePath_,
                    error,
                    commitResponseSendCount_.load());

                ConvertErrorAndTryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            }

            // Ignore this error. Request is under processing in FSS
            return;
        }

        bool expected = false;
        if (gotFileUploadCommitResponse_.compare_exchange_strong(expected, true))
        {
            owner_.WriteInfo(
                TraceComponent,
                owner_.Root.TraceId,
                "Got FileUploadCommit response for operationId:{0} storeRelativePath:{1} error:{2} sendCommitCount:{3} TotalChunkBasedUploads:{4}",
                operationId_,
                this->storeRelativePath_,
                error,
                commitResponseSendCount_.load(),
                owner_.TotalChunkBasedUploads);

            if (error.IsSuccess())
            {
                // Update progress handler for replicated files
                IFileStoreServiceClientProgressEventHandlerPtr progressHandler;
                {
                    AcquireExclusiveLock lock(lock_);
                    progressHandler = progressHandler_;
                }

                if (progressHandler.get() != nullptr)
                {
                    progressHandler->IncrementReplicatedFiles(1);
                }

                SendFileUploadActionMessage(thisSPtr, FileTransferTcpMessage::FileUploadCommitAckAction);
                ConvertErrorAndTryComplete(thisSPtr, move(error));
            }
            else if (IsRetryableErrorOnCommitOperation(error) && retryResendFileCount_.load() < ClientConfig::GetConfig().FileUploadResendRetryAttempt)
            {
                RetrySendingFile(thisSPtr, move(error));
            }
            else if (commitResponseSendCount_.load() >= ClientConfig::GetConfig().FileUploadCommitRetryAttempt)
            {
                SendFileUploadActionMessage(thisSPtr, FileTransferTcpMessage::FileUploadDeleteSessionAction);
                ConvertErrorAndTryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            }
            else
            {
                SendFileUploadActionMessage(thisSPtr, FileTransferTcpMessage::FileUploadDeleteSessionAction);
                ConvertErrorAndTryComplete(thisSPtr, move(error));
            }

            return;
        }
    }

protected:

    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {

        IFileStoreServiceClientProgressEventHandlerPtr progressHandler;
        {
            AcquireExclusiveLock lock(lock_);
            progressHandler = progressHandler_;
        }

        auto operation = owner_.fileSender_->BeginSendFileToGateway(
            operationId_,
            serviceName_,
            sourceFullPath_,
            storeRelativePath_,
            shouldOverwrite_,
            useChunkBasedUpload_.load(),
            retryResendFileCount_.load() > 0,
            move(progressHandler),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnFileSendCompleted(operation, this->operationId_); },
            thisSPtr);

        bool cancelOperation = true;
        {
            AcquireExclusiveLock lock(this->lock_);
            if (!completedOrCanceled_)
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
            progressHandler_ = IFileStoreServiceClientProgressEventHandlerPtr();
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
    void OnFileSendCompleted(AsyncOperationSPtr const & operation, Guid const &operationId)
    {
        {
            AcquireExclusiveLock lock(this->lock_);
            senderAsyncOperation_.reset();
        }

        bool isUsingChunkBasedUpload;
        auto error = owner_.fileSender_->EndSendFileToGateway(operation, operationId, isUsingChunkBasedUpload);

        if (!isUsingChunkBasedUpload)
        {
            HandleSingleFileUploadCompletion(operation->Parent, error);
        }
        else
        {
            HandleChunkBasedUploadCompletion(operation->Parent, error);
        }
    }

    bool IsRetryableErrorOnFileChunkSendOperation(ErrorCode const &error)
    {
        switch (error.ReadValue())
        {
            case ErrorCodeValue::SendFailed:
            case ErrorCodeValue::ConnectionConfirmWaitExpired:
            case ErrorCodeValue::NotFound:
            case ErrorCodeValue::HostingServiceTypeNotRegistered:
            case ErrorCodeValue::OperationCanceled:
            case ErrorCodeValue::GatewayUnreachable:
                return true;
            default:
                return false;
        }
    }

    bool IsRetryableErrorOnCommitOperation(ErrorCode const &error)
    {
        // This could happen when FSS primary fails over (NotFound) or if there is a missing chunk(InvalidArgument)
        // If gateway crashes (GatewayNotReachable) or fails over, it would send NotFound error for the non-existent request
        switch (error.ReadValue())
        {
            case ErrorCodeValue::InvalidArgument:
            case ErrorCodeValue::NotFound:
            case ErrorCodeValue::HostingServiceTypeNotRegistered:
            case ErrorCodeValue::OperationCanceled:
            case ErrorCodeValue::GatewayUnreachable:
                return true;
            default:
                return false;
        }
    }

    void RetrySendingFile(AsyncOperationSPtr const & thisSPtr, ErrorCode const &error)
    {
        owner_.WriteInfo(
            TraceComponent,
            owner_.Root.TraceId,
            "Retrying file send for the operationId:{0} storeRelativePath:{1} error:{2} retryAttempt:{3}",
            operationId_,
            storeRelativePath_,
            error,
            retryResendFileCount_.load());

        CancelFileCommitUploadTimer();
        ++retryResendFileCount_;
        OnStartOperation(thisSPtr);
        return;
    }

    void HandleSingleFileUploadCompletion(AsyncOperationSPtr const & thisSPtr, ErrorCode const &error)
    {
        if (!error.IsSuccess())
        {
            ConvertErrorAndTryComplete(thisSPtr, move(error));
            return;
        }

        AcquireExclusiveLock lock(lock_);
        if (completedOrCanceled_) { return; }
        if (timer_) { return; }

        timer_ = Timer::Create(
            SingleFileUploadTimerTag,
            [this, thisSPtr](TimerSPtr const & timer)
        {
            timer->Cancel();
            ConvertErrorAndTryComplete(thisSPtr, ErrorCodeValue::Timeout);
        },
            false);
    }

    void HandleChunkBasedUploadCompletion(AsyncOperationSPtr const & thisSPtr, ErrorCode & error)
    {
        // Retry for cases where unable to send the file or FSS returns NotFound because of failover happened while transferring chunks
        if (IsRetryableErrorOnFileChunkSendOperation(error) &&
            retryResendFileCount_.load() < ClientConfig::GetConfig().FileUploadResendRetryAttempt)
        {
            // This is to make sure we don't aggressively switch to old protocol 
            // if there is already successful completion using new protocol
            if (error.IsError(ErrorCodeValue::ConnectionConfirmWaitExpired) && 
                (owner_.TotalChunkBasedUploads == 0 || 
                (owner_.TotalChunkBasedUploads > 0 && retryResendFileCount_.load() == ClientConfig::GetConfig().SwitchUploadProtocolResendRetryAttempt)))
            {
                owner_.WriteInfo(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Switching to old protocol and resending as we might be targeting older version of the cluster or FSS not accepting chunk based requests for operationId:{0} storeRelativePath:{1} after retryAttempt:{2} totalChunkUpload:{3}",
                    operationId_,
                    storeRelativePath_,
                    retryResendFileCount_.load(),
                    owner_.TotalChunkBasedUploads);

                // Update protocol to use single file upload only for this file
                useChunkBasedUpload_.store(false);

                // This changes protocol in the file sender for the remaining files if a certain condition is met
                if (owner_.fileSender_->SwitchToSingleFileUploadProtocol(true))
                {
                    owner_.UseChunkBasedUpload = false;
                }
            }
            else
            {
                owner_.fileSender_->SwitchToSingleFileUploadProtocol(false);
            }

            owner_.WriteWarning(
                TraceComponent,
                owner_.Root.TraceId,
                "Resending the file again for operationId:{0} storeRelativePath:{1} after a delay of {2} seconds, retryAttempt:{3}",
                operationId_,
                storeRelativePath_,
                ClientConfig::GetConfig().FileUploadResendRetryBackoffInterval,
                retryResendFileCount_.load());

            ++retryResendFileCount_;
            error = TryScheduleRetry(
                thisSPtr,
                [this](AsyncOperationSPtr const & thisSPtr) { this->OnStartOperation(thisSPtr); },
                ClientConfig::GetConfig().FileUploadResendRetryBackoffInterval);

            if (!error.IsSuccess())
            {
                ConvertErrorAndTryComplete(thisSPtr, move(error));
            }

            return;
        }
        else if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::SendFailed))
            {
                owner_.WriteWarning(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Tried sending the file multiple times but didn't succeed. operationId:{0} storeRelativePath:{1} after a delay of {2} second, retryAttempt:{3}, chunkUpload:{4} error:{5}",
                    operationId_,
                    storeRelativePath_,
                    ClientConfig::GetConfig().RetryBackoffInterval,
                    retryResendFileCount_.load(),
                    useChunkBasedUpload_.load(),
                    error);
            }
            else
            {
                owner_.WriteWarning(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Sending file failed for OperationId:{0} storeRelativePath:{1} chunkUpload:{2} error:{3}",
                    operationId_,
                    storeRelativePath_,
                    useChunkBasedUpload_.load(),
                    error);
            }

            owner_.fileSender_->SwitchToSingleFileUploadProtocol(false);
            SendFileUploadActionMessage(thisSPtr, FileTransferTcpMessage::FileUploadDeleteSessionAction);
            ConvertErrorAndTryComplete(thisSPtr, move(error));
            return;
        }

        // Success case
        {
            AcquireExclusiveLock lock(lock_);
            if (completedOrCanceled_) { return; }
            owner_.fileSender_->SwitchToSingleFileUploadProtocol(false);
            gotFileUploadCommitResponse_.store(false);
            if (timer_) { return; }

            commitResponseSendCount_.store(0);
            timer_ = Timer::Create(
                CommitSendTimerTag,
                [this, thisSPtr](TimerSPtr const &)
            {
                if (!gotFileUploadCommitResponse_.load())
                {
                    SendFileUploadActionMessage(thisSPtr, FileTransferTcpMessage::FileUploadCommitAction);
                }
            },
                false);

            auto interval = min<int64>(
                (int64)(commitResponseSendCount_.load() + 1) * ClientConfig::GetConfig().FileUploadCommitRetryInterval.TotalMilliseconds(), 
                timeoutHelper_.GetRemainingTime().TotalMilliseconds());
            timer_->Change(TimeSpan::Zero, TimeSpan::FromMilliseconds(static_cast<double>(interval)));
        }
    }

    void AddRequiredHeaders(ClientServerRequestMessage & message)
    {
        message.Headers.Replace(TimeoutHeader(timeoutHelper_.GetRemainingTime()));
        message.Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);
        message.Headers.Add(MessageIdHeader());
    }

    void SendFileUploadActionMessage(AsyncOperationSPtr const & thisSPtr, Common::GlobalWString const &fileUploadActionString)
    {
        ErrorCode error;
        bool done = false, sendFailure = false;
        int retryAttempt = 0;
        do
        {
            {
                AcquireExclusiveLock lock(lock_);
                if (completedOrCanceled_) { return; }
            }

            ClientServerRequestMessageUPtr message;
            if (fileUploadActionString == FileTransferTcpMessage::FileUploadDeleteSessionAction)
            {
                message = FileTransferTcpMessage::GetFileUploadDeleteSessionMessage(
                    operationId_,
                    Actor::FileReceiver);
            }
            else if (fileUploadActionString == FileTransferTcpMessage::FileUploadCommitAction)
            {
                message = FileTransferTcpMessage::GetFileUploadCommitMessage(
                    operationId_,
                    Actor::FileReceiver);
            }
            else if (fileUploadActionString == FileTransferTcpMessage::FileUploadCommitAckAction)
            {
                message = FileTransferTcpMessage::GetFileUploadCommitAckMessage(
                    operationId_,
                    Actor::FileReceiver);
            }
            else
            {
                owner_.WriteWarning(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Unexpected action to send for operationId:{0} storeRelativePath:{1}",
                    operationId_,
                    storeRelativePath_);
                TESTASSERT_IF(true, "Unknown upload action {0} in FileTransferClient::SendFileUploadActionMessage", fileUploadActionString);
                return;
            }

            AddRequiredHeaders(*message);
            if (owner_.connection_)
            {
                unique_ptr<GatewayDescription> unused;
                error = owner_.connection_->SendOneWayToGateway(move(message), unused, timeoutHelper_.GetRemainingTime());
            }
            else
            {
                error = ErrorCodeValue::CannotConnect;
            }

            if (error.IsSuccess())
            {
                if (fileUploadActionString == FileTransferTcpMessage::FileUploadCommitAction)
                {
                    owner_.WriteNoise(
                        TraceComponent,
                        owner_.Root.TraceId,
                        "Sending message action:{0} for the {1} time for operationId:{2} storeRelativePath:{3}",
                        fileUploadActionString,
                        commitResponseSendCount_.load(),
                        operationId_,
                        storeRelativePath_);

                    ++commitResponseSendCount_;
                }

                done = true;
            }
            else if (IsRetryable(error))
            {
                if (retryAttempt < ClientConfig::GetConfig().SendMessageActionRetryAttempt)
                {
                    Sleep(static_cast<DWORD>(ClientConfig::GetConfig().SendMessageActionRetryBackoffInterval.TotalMilliseconds()));
                    ++retryAttempt;
                    done = false;
                }
                else
                {
                    done = true;
                }

            }
            else
            {
                sendFailure = true;
                done = true;
            }
        } while (!done);

        if (sendFailure)
        {
            owner_.WriteWarning(
                TraceComponent,
                owner_.Root.TraceId,
                "Could not send {0} due to SendOneWayToGateway failed with error:{1}. OperationId:{2} storeRelativePath:{3}",
                fileUploadActionString,
                error,
                operationId_,
                storeRelativePath_);

            // Could not send message and hence cancel the operation on FileSender
            AsyncOperationSPtr operation;
            {
                AcquireExclusiveLock lock(this->lock_);
                operation = move(senderAsyncOperation_);
            }

            if (operation) { operation->Cancel(); }

            ConvertErrorAndTryComplete(thisSPtr, move(error));
            return;
        }

        return;
    }

    void CancelFileCommitUploadTimer()
    {
        TimerSPtr timer;
        {
            AcquireExclusiveLock lock(lock_);
            if (timer_)
            {
                timer = move(timer_);
            }
        }

        if (timer)
        {
            timer->Cancel();
        }
    }

    void ConvertErrorAndTryComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const &error)
    {
        wstring msg;

        if (error.IsSuccess())
        {
            owner_.IncrementChunkUpload();
        }

        switch (error.ReadValue())
        {
            case ErrorCodeValue::SendFailed:
            {
                msg = StringResource::Get(IDS_FSS_CLIENT_SendChunkFailure);
                break;
            }
            case ErrorCodeValue::TransportSendQueueFull:
            {
                msg = wformatString(StringResource::Get(IDS_FSS_CLIENT_UnableToUploadFile), this->storeRelativePath_);
                break;
            }
            case ErrorCodeValue::OperationCanceled:
            {
                msg = StringResource::Get(IDS_FSS_CLIENT_OperationCanceled);
                break;
            }
            case ErrorCodeValue::ConnectionConfirmWaitExpired:
            {
                msg = StringResource::Get(IDS_FSS_CLIENT_ConnectionConfirmWaitExpired);
                break;
            }
            case ErrorCodeValue::OperationFailed:
            {
                msg = wformatString(StringResource::Get(IDS_FSS_CLIENT_OperationFailed), this->storeRelativePath_);
                break;
            }
            default:
            {
                TryComplete(thisSPtr, move(error));
                return;
            }
        }

        ErrorCode convertedError(ErrorCodeValue::OperationFailed, move(msg));
        TryComplete(thisSPtr, move(convertedError));
        return;
    }

private:
    wstring const serviceName_;
    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;
    TimerSPtr timer_;
    AsyncOperationSPtr senderAsyncOperation_;
    Common::atomic_bool gotFileUploadCommitResponse_;
    Common::atomic_long commitResponseSendCount_;
    Common::atomic_long retryResendFileCount_;
    Common::atomic_bool useChunkBasedUpload_;
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
            if (!completedOrCanceled_)
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
    bool useChunkBasedUpload,
    ComponentRoot const & root)
    : RootedObject(root)
    , connection_(connection)
    , transport_(connection->Transport)
    , fileSender_()
    , fileReceiver_()
    , pendingOperations_()
{
    useChunkBasedUpload_ = useChunkBasedUpload;
    fileSender_ = FileSender::CreateOnClient(
        connection_,
        true,
        this->Root.TraceId,
        this->Root,
        useChunkBasedUpload_);

    fileReceiver_ = make_unique<FileReceiver>(
        transport_,
        true,
        L"" /*working dir*/,
        this->Root.TraceId,
        this->Root);
}

ErrorCode FileTransferClient::OnOpen()
{
    totalChunkBasedUploads_.store(0);
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
