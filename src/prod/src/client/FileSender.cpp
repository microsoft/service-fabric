// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <numeric>

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("FileSender");
StringLiteral const RetryChunkSendTimerTag("FileSender.RetryChunkTimer");
StringLiteral const FileChunkBatchSendTimerTag("FileSender.FileChunkBatchSendTimer");

template <class R>
class FileChunkJobQueue : public Common::CommonJobQueue<R>
{
    DENY_COPY(FileChunkJobQueue)
public:
    FileChunkJobQueue(std::wstring const & name, R & root, bool forceEnqueue, int maxThreads = 0)
        : Common::CommonJobQueue<R>(
            name,
            root,
            forceEnqueue,
            maxThreads),
        onFinishEvent_()
    {
    }

    __declspec(property(get = get_OperationsFinishedEvent)) Common::ManualResetEvent & OperationsFinishedAsyncEvent;
    Common::ManualResetEvent & get_OperationsFinishedEvent() { return onFinishEvent_; }

protected:
    void OnFinishItems() override { onFinishEvent_.Set(); }

private:
    Common::ManualResetEvent onFinishEvent_;
};

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

class FileSender::FileChunkJobItem : public FileSender::FileJobItem
{
    DENY_COPY(FileChunkJobItem)

public:
    FileChunkJobItem(ProcessingCallback const & callback)
        : FileJobItem(callback)
    {
    }
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
            AddRequiredHeadersForSingleFileUploadProtocol(*fileContentMessage, sequenceNumber);

            WriteNoise(
                TraceComponent,
                owner_.traceId_,
                "{0}: Sending {1} bytes with SequenceNumber {2}.",
                operationId_,
                buffer->size(),
                sequenceNumber);

            error = owner_.SendMessage(sendTarget_, currentGateway_, fileContentMessage, RemainingTime);
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

    void AddRequiredHeadersForSingleFileUploadProtocol(ClientServerRequestMessage & message, uint64 const sequenceNumber)
    {
        // Add Timeout and FileUploadRequest header only for the first message
        if (sequenceNumber == 0)
        {
            message.Headers.Replace(TimeoutHeader(this->RemainingTime));
            message.Headers.Replace(FileUploadRequestHeader(serviceName_, storeRelativePath_, shouldOverwrite_));
        }

        message.Headers.Add(MessageIdHeader());

        if (owner_.includeClientVersionHeaderInMessage_)
        {
            message.Headers.Replace(*ClientProtocolVersionHeader::SingleFileUploadVersionHeader);
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

class FileSender::SendChunkBasedAsyncOperation :
    public TimedAsyncOperation,
    private TextTraceComponent < TraceTaskCodes::FileTransfer >
{
    DENY_COPY(SendChunkBasedAsyncOperation)

public:
    class SendFileChunkAsyncOperation :
        public TimedAsyncOperation,
        private TextTraceComponent < TraceTaskCodes::FileTransfer >
    {
        DENY_COPY(SendFileChunkAsyncOperation)

    public:

        SendFileChunkAsyncOperation(
            FileSender & owner,
            Guid const & operationId,
            uint64 const sequenceNumber,
            uint64 const startPosition,
            uint64 const endPosition,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout, callback, parent),
            owner_(owner),
            operationId_(operationId),
            sequenceNumber_(sequenceNumber),
            startPosition_(startPosition),
            endPosition_(endPosition),
            file_(),
            operationCompletedOrCanceled_(false),
            initialEnqueueSucceeded_(false),
            resendChunkTimer_(),
            resendChunkTimerlock_(),
            fileChunkBuffer_(),
            retryCount_(0),
            random_(),
            cleanupDone_(false),
            lock_()
        {
        }

        virtual ~SendFileChunkAsyncOperation()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<SendFileChunkAsyncOperation>(operation);
            return thisPtr->Error;
        }

        void OnCancel() override
        {
            operationCompletedOrCanceled_.store(true);
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

            // The lifetime of sendChunkBasedAsyncOperation_ is valid till the operation is completed
            sendChunkBasedAsyncOperation_ = AsyncOperation::Get<SendChunkBasedAsyncOperation>(thisSPtr->Parent);
            auto error = file_.TryOpen(
                sendChunkBasedAsyncOperation_->sourceFullPath_,
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
                    sendChunkBasedAsyncOperation_->sourceFullPath_,
                    error);
                this->TryComplete(thisSPtr, move(error));
                return;
            }

            error = sendChunkBasedAsyncOperation_->TryAddToPendingSendChunkOperation(sequenceNumber_, thisSPtr);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "Unable to add sequence number to ActiveFileChunks operationId {0} seqId : {1}. SourceFullPath: {2}, StoreRelativePath: {3}, error : {4}",
                    operationId_,
                    sequenceNumber_,
                    sendChunkBasedAsyncOperation_->sourceFullPath_,
                    sendChunkBasedAsyncOperation_->storeRelativePath_,
                    error
                );

                this->TryComplete(thisSPtr, move(error));
                return;
            }

            bool success = sendChunkBasedAsyncOperation_->fileChunkJobQueue_->Enqueue(
                make_unique<FileChunkJobItem>([thisSPtr, this]() { 
                        initialEnqueueSucceeded_ = true;
                        this->StartFileChunkProcessing(thisSPtr); 
                    }));
            if (!success)
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: Failed to enqueue the operation to JobQueue.",
                    operationId_);
                TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
                return;
            }
        }

        void OnCompleted()
        {
            Cleanup();
        }

    private:

        void Cleanup()
        {
            // Minimal contention on this lock so shouldn't affect performance
            AcquireExclusiveLock cleanupLock(lock_);
            bool completeAsyncJob = false;
            {
                if (cleanupDone_)
                {
                    WriteNoise(
                        TraceComponent,
                        owner_.traceId_,
                        "{0}: Cleanup already done for {1} sequenceNumber:{2}",
                        operationId_,
                        sendChunkBasedAsyncOperation_->storeRelativePath_,
                        sequenceNumber_);
                    return;
                }
                else
                {
                    if (initialEnqueueSucceeded_)
                    {
                        completeAsyncJob = true;
                        cleanupDone_ = true;
                    }
                    else
                    {
                        // There is a possible race scenario where initialEnqueueSucceeded_ is not set since it is scheduled but not started executing callback, 
                        // but cleanup is called by OnCompleted() on cancellation.
                        // In that case, though rest of the cleanup is done, completeAsyncJob() is not called.
                        // This can leave active thread behind without completion. So set the cleanupDone_ to false, so the scheduled thread can call completion later.
                        cleanupDone_ = false;
                        completeAsyncJob = false;
                    }
                }
            }

            if (resendChunkTimer_)
            {
                TimerSPtr timer;
                {
                    AcquireExclusiveLock lock(resendChunkTimerlock_);
                    timer = move(resendChunkTimer_);
                }

                if (timer)
                {
                    timer->Cancel();
                }
            }

            if (file_.IsValid())
            {
                file_.Close2();
                TimedAsyncOperation::OnCompleted();
                operationCompletedOrCanceled_.store(true);
            }

            if (completeAsyncJob)
            {
                WriteNoise(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: Async job completing for {1} activeThreads:{2} sequenceNumber:{3}",
                    operationId_,
                    sendChunkBasedAsyncOperation_->storeRelativePath_,
                    sendChunkBasedAsyncOperation_->fileChunkJobQueue_->GetActiveThreads(),
                    sequenceNumber_);

                sendChunkBasedAsyncOperation_->fileChunkJobQueue_->CompleteAsyncJob();
                initialEnqueueSucceeded_ = false;
            }
        }

        ErrorCode ReadFileChunk(int bufferSize, shared_ptr<vector<BYTE>> buffer)
        {
            DWORD bytesRead = 0;

            auto filesize = static_cast<uint64>(sendChunkBasedAsyncOperation_->fileSize_);
            if (filesize != 0 && startPosition_ >= filesize)
            {
                return ErrorCodeValue::OperationFailed;
            }

            if (operationCompletedOrCanceled_.load() || !file_.IsValid())
            {
                return ErrorCodeValue::OperationCanceled;
            }

            file_.Seek(startPosition_, Common::SeekOrigin::Begin);
            auto error = file_.TryRead2(buffer->data(), bufferSize, bytesRead);
            if (!error.IsSuccess() || static_cast<int>(bytesRead) != bufferSize)
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: TryRead of '{1}' failed with {2}. BytesRead {3} Expected {4}",
                    operationId_,
                    sendChunkBasedAsyncOperation_->sourceFullPath_,
                    error,
                    bytesRead,
                    bufferSize);

                if (static_cast<int>(bytesRead) != bufferSize)
                {
                    error.Overwrite(ErrorCodeValue::OperationFailed);
                }
            }
            return error;
        }

        // This job is not completed until either chunk is successfully sent or got non-retyable error during send.
        // Threads in queue not scheduled any other work until this operation is completed.
        void StartFileChunkProcessing(AsyncOperationSPtr const & thisSPtr)
        {
            if (operationCompletedOrCanceled_.load())
            {
                WriteNoise(
                    TraceComponent,
                    owner_.traceId_,
                    "StartFileChunkProcessing: {0}: Operation already canceled for '{1}' sequenceId:{2}.",
                    operationId_,
                    sendChunkBasedAsyncOperation_->storeRelativePath_,
                    sequenceNumber_);

                Cleanup();
                return;
            }

            int bufferSize = sendChunkBasedAsyncOperation_->GetChunkSize(sequenceNumber_);

            fileChunkBuffer_ = make_shared<vector<BYTE>>(bufferSize);
            auto error = ReadFileChunk(bufferSize, fileChunkBuffer_);
            if (!error.IsSuccess() && !operationCompletedOrCanceled_.load())
            {
                WriteInfo(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: Unable to read chunk {1} sequenceNumber:{2}.",
                    operationId_,
                    error,
                    sequenceNumber_);
                this->TryComplete(thisSPtr, move(error));
                return;
            }

            UploadFileChunk(thisSPtr);
        }

        void UploadFileChunk(AsyncOperationSPtr const & thisSPtr)
        {
            if (operationCompletedOrCanceled_.load())
            {
                WriteNoise(
                    TraceComponent,
                    owner_.traceId_,
                    "UploadFileChunk: {0}: Operation already canceled for '{1}' sequenceId:{2}.",
                    operationId_,
                    sendChunkBasedAsyncOperation_->storeRelativePath_,
                    sequenceNumber_);

                Cleanup();
                return;
            }

            // Optimization: Do not send chunk for which the ack has already been received
            if (!sendChunkBasedAsyncOperation_->IsFileChunkNeedsToBeSent(sequenceNumber_))
            {
                if (!operationCompletedOrCanceled_.load())
                {
                    TryComplete(thisSPtr, ErrorCodeValue::Success);
                }
                else
                {
                    Cleanup();
                }

                return;
            }

            ClientServerRequestMessageUPtr fileContentMessage = FileTransferTcpMessage::GetFileContentMessage(
                fileChunkBuffer_,
                sequenceNumber_,
                false,
                operationId_,
                Actor::FileReceiver);

            AddRequiredHeaders(*fileContentMessage, fileChunkBuffer_->size());

            auto error = owner_.SendMessage(sendChunkBasedAsyncOperation_->sendTarget_, currentGateway_, fileContentMessage, RemainingTime);
            if (!error.IsSuccess() && IsRetryable(error))
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: SendOneWay during UploadFileChunk failed with retryable error:{1} retryCount:{2} storeRelativePath:{3} sequenceId:{4}.",
                    operationId_,
                    error,
                    retryCount_.load(),
                    sendChunkBasedAsyncOperation_->storeRelativePath_,
                    sequenceNumber_);

                error = TryScheduleRetry(
                    thisSPtr,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->UploadFileChunk(thisSPtr); });

                if (error.IsSuccess())
                {
                    return;
                }
            }

            WriteTrace(
                error.ToLogLevel(),
                TraceComponent,
                owner_.traceId_,
                "{0}: SendOneWay during UploadFileChunk error:{1} retryCount:{2} storeRelativePath:{3} sequenceId:{4}",
                operationId_,
                error,
                retryCount_.load(),
                sendChunkBasedAsyncOperation_->storeRelativePath_,
                sequenceNumber_);

            TryComplete(thisSPtr, move(error));
        }

        void AddRequiredHeaders(ClientServerRequestMessage & message, uint64 const bufferSize)
        {
            message.Headers.Replace(TimeoutHeader(this->RemainingTime));
            message.Headers.Replace(FileSequenceHeader(sequenceNumber_, false, bufferSize));

            if (owner_.includeClientVersionHeaderInMessage_)
            {
                message.Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);
            }

            message.Headers.Add(MessageIdHeader());
        }

        bool IsRetryable(ErrorCode const & error)
        {
            switch (error.ReadValue())
            {
            case ErrorCodeValue::TransportSendQueueFull:
            case ErrorCodeValue::NotReady:
            case ErrorCodeValue::OperationsPending:
                return true;
            default:
                break;
            }

            return false;
        }

        Common::TimeSpan GetDelayInterval()
        {
            auto delayInterval = random_.Next((int)ClientConfig::GetConfig().FileChunkRetryInterval.TotalMilliseconds() - 500, (int)ClientConfig::GetConfig().FileChunkRetryInterval.TotalMilliseconds() + 500);
            return Common::TimeSpan::FromMilliseconds(static_cast<double>(min<int64>(delayInterval, this->RemainingTime.TotalMilliseconds())));
        }

        typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

        ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
        {
            if (this->RemainingTime.TotalMilliseconds() == 0)
            {
                return ErrorCodeValue::Timeout;
            }

            auto delay = GetDelayInterval();
            {
                if (retryCount_.load() >= ClientConfig::GetConfig().FileChunkRetryAttempt)
                {
                    WriteWarning(
                        TraceComponent,
                        owner_.traceId_,
                        "{0}: SendOneWay during UploadFileChunk failed after retrying {1} number of times. Returning SendFailed errorcode for storeRelativePath:{2} sequenceId:{3}",
                        operationId_,
                        retryCount_.load(),
                        sendChunkBasedAsyncOperation_->storeRelativePath_,
                        sequenceNumber_);

                    return ErrorCodeValue::SendFailed;
                }

                AcquireExclusiveLock lock(resendChunkTimerlock_);

                if (!operationCompletedOrCanceled_.load())
                {
                    ++retryCount_;
                    resendChunkTimer_ = Timer::Create(
                        RetryChunkSendTimerTag,
                        [this, thisSPtr, callback](TimerSPtr const & timer)
                    {
                        timer->Cancel();
                        callback(thisSPtr);
                    });

                    resendChunkTimer_->Change(delay);
                }
                else
                {
                    Cleanup();
                }
            }

            return ErrorCodeValue::Success;
        }

    private:
        Guid const operationId_;
        unique_ptr<GatewayDescription> currentGateway_;
        FileSender & owner_;
        uint64 sequenceNumber_;
        uint64 startPosition_;
        uint64 endPosition_;
        File file_;
        Common::atomic_bool operationCompletedOrCanceled_;
        bool initialEnqueueSucceeded_;
        SendChunkBasedAsyncOperation *sendChunkBasedAsyncOperation_; // This doesn't allocate/own memory
        TimerSPtr resendChunkTimer_; // Resend timer for chunk that was unable to send due to Transport queue full
        ExclusiveLock resendChunkTimerlock_; // lock for the resend timer
        shared_ptr<vector<BYTE>> fileChunkBuffer_; // file content
        Common::atomic_long retryCount_;
        Common::Random random_;
        bool cleanupDone_; // protected by lock_
        ExclusiveLock lock_;
    };

    SendChunkBasedAsyncOperation(
        FileSender & owner,
        Guid const & operationId,
        ISendTarget::SPtr const & sendTarget,
        wstring const & serviceName,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        bool retryAttempt,
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
        retryAttempt_(retryAttempt),
        progressHandler_(move(progressHandler)),
        completedOrCanceled_(false),
        fileSize_(0),
        fileChunkJobQueue_(),
        file_(),
        pendingFileChunkSendMap_(),
        pendingFileChunkSendLock_(),
        fileChunkSendCompletedSet_(),
        resendRetryCount_(0),
        expectedFileChunksSendCount_(0),
        fileChunkAcksReceivedSet_(),
        timer_(),
        timerlock_(),
        allChunksSent_(false),
        fileCreateOperationError_(),
        fileCreateOperationCompleted_(false),
        fileSendOperationCompleted_(false),
        gotfileCreateOperationResponse_(false),
        gatewayUnreachableErrorCount_(0),
        nextFileChunkIndexToBeSent_(0),
        fileChunkBatchTimer_(),
        fileChunkBatchTimerLock_(),
        fileChunkBatchLock_(),
        expectedFileChunksToBeSent_()
    {
        maxAllowedChunkSize_ = Utility::GetMessageContentThreshold();
    }

    virtual ~SendChunkBasedAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SendChunkBasedAsyncOperation>(operation);
        return thisPtr->Error;
    }

    void ProcessFileChunkAckMessage(AsyncOperationSPtr const & operation, ErrorCode const & errorCode, uint64 sequenceNumber, uint64 maxAllowedSize)
    {
        if (completedOrCanceled_.load())
        {
            return;
        }

        if (errorCode.IsError(ErrorCodeValue::OperationsPending) || errorCode.IsError(ErrorCodeValue::TransportSendQueueFull))
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Ignoring chunk response for storeRelativePath:{1} sequenceNumber:{2} error:{3}",
                operationId_,
                storeRelativePath_,
                sequenceNumber,
                errorCode);

            return;
        }

        // Check if the reply is for create file upload message
        if (sequenceNumber == numeric_limits<uint64>::max())
        {
            // Update Chunk size based on cluster's allowed size
            maxAllowedChunkSize_ = maxAllowedSize == 0 ? Utility::GetMessageContentThreshold() : maxAllowedSize;

            bool expected = false;
            if (gotfileCreateOperationResponse_.compare_exchange_strong(expected, true))
            {
                fileCreateOperationCompleted_.Set();
                fileCreateOperationError_.Overwrite(move(errorCode));
                return;
            }
            else
            {
                // It is possible that we receive multiple responses for file create message since we send multiple times
                // We just need to act upon only the first message and just ignore the others.
                return;
            }
        }

        // It is possible that gateway may drop chunks before sending to FSS(p) with returning GatewayUnreachable error.
        // This error is usually transient and retrying next time should succeed.
        // If we are getting GatewayUnreachable error consecutively for a threshold times, we would consider that as permanent error.
        if (errorCode.IsErrno(ErrorCodeValue::GatewayUnreachable))
        {
            AcquireExclusiveLock lock(lock_);
            ++gatewayUnreachableErrorCount_;
            if (gatewayUnreachableErrorCount_ < ClientConfig::GetConfig().GatewayNotReachableThresholdLimit)
            {
                WriteInfo(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: Gateway not reachable error for storeRelativePath:{1} sequenceNumber:{2} error:{3} under the threshold count:{4}",
                    operationId_,
                    storeRelativePath_,
                    sequenceNumber,
                    errorCode,
                    gatewayUnreachableErrorCount_);
                return;
            }
        }

        if (!errorCode.IsSuccess())
        {
            uint gatewayUnreachableErrorCount = 0;
            {
                AcquireExclusiveLock lock(lock_);
                gatewayUnreachableErrorCount = gatewayUnreachableErrorCount_;
            }

            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Chunk ack received but with failure for storeRelativePath:{1} sequenceNumber:{2} error:{3} gatewayUnreachableErrorCount:{4}.",
                operationId_,
                storeRelativePath_,
                sequenceNumber,
                errorCode,
                gatewayUnreachableErrorCount);

            // cancel existing send operation
            CancelFileChunkSendOperations();
            TryComplete(operation, move(errorCode));
            return;
        }


        bool allAcksReceived = false;
        {
            AcquireExclusiveLock lock(pendingFileChunkSendLock_);
            fileChunkAcksReceivedSet_.insert(sequenceNumber);
            if (pendingFileChunkSendMap_.erase(sequenceNumber) == 0)
            {
                return; // already received ack for this chunk, so do not process this
            }

            WriteNoise(
                TraceComponent,
                owner_.traceId_,
                "{0}: Chunk ack received for storeRelativePath:{1} sequenceNumber:{2}",
                operationId_,
                storeRelativePath_,
                sequenceNumber);

            allAcksReceived = (fileChunkAcksReceivedSet_.size() == totalFileChunks_);
            if (allAcksReceived)
            {
                completedOrCanceled_.store(true);
            }
        }

        // Update progress handler for chunks sent and ack received
        IFileStoreServiceClientProgressEventHandlerPtr progressHandler;
        {
            AcquireExclusiveLock lock(lock_);
            progressHandler = progressHandler_;
            gatewayUnreachableErrorCount_ = 0;
        }

        if (progressHandler.get() != nullptr && fileSize_ != 0)
        {
            progressHandler->IncrementTransferCompletedItems(GetChunkSize(sequenceNumber));
        }

        // Complete the FileSend request once all acks are received
        if (allAcksReceived)
        {
            WriteInfo(
                TraceComponent,
                owner_.traceId_,
                "{0}: All chunk acks received for storeRelativePath:{1}",
                operationId_,
                storeRelativePath_);

            TryComplete(operation, move(errorCode));
            return;
        }
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

        std::wstring queueName = wformatString("{0}-{1}", L"FileChunksSenderJobQueue", operationId_);
        fileChunkJobQueue_ = make_unique<FileChunkJobQueue<FileSender>>(
            queueName,
            owner_,
            false /* forceEnqueue */,
            ClientConfig::GetConfig().MaxFileChunkSenderThreads);
        fileChunkJobQueue_->SetAsyncJobs(true);

        if (ClientConfig::GetConfig().IsEnableFileChunkQueueTracing)
        {
            fileChunkJobQueue_->SetExtraTracing(true);
        }

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
            file_.Close2();
        }

        TimerSPtr timer;
        {
            AcquireExclusiveLock lock(timerlock_);
            timer = move(timer_);
        }
        if (timer)
        {
            timer->Cancel();
        }

        fileChunkJobQueue_->Close();
        CancelFileChunkBatchTimer();

        {
            AcquireExclusiveLock lock(pendingFileChunkSendLock_);
            pendingFileChunkSendMap_.clear();
            fileChunkSendCompletedSet_.clear();
            fileChunkAcksReceivedSet_.clear();
        }

        {
            AcquireExclusiveLock lock(fileChunkBatchLock_);
            expectedFileChunksToBeSent_.clear();
        }

        WriteInfo(
            TraceComponent,
            owner_.traceId_,
            "{0}: Removed operationId from ActiveSends. storeRelativePath:{1}",
            operationId_,
            storeRelativePath_);

        fileSendOperationCompleted_.Set();

        auto maxCloseWaitDuration = ClientConfig::GetConfig().MaxCloseJobQueueWaitDuration;

        // Wait for all items in the query job queue to be processed
        bool closed = fileChunkJobQueue_->OperationsFinishedAsyncEvent.WaitOne(maxCloseWaitDuration);
        if (!closed)
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Queue not drained. storeRelativePath:{1}. ActiveThreads:{2}",
                operationId_,
                storeRelativePath_,
                fileChunkJobQueue_->GetActiveThreads());

            // If this is fired check if the async job item fires CompleteAsyncJob() on all exit paths.
            Assert::CodingError("File chunk job queue didn't close in the allotted time {0}. operationId:{1} storeRelativePath:{2}", maxCloseWaitDuration, operationId_, storeRelativePath_);
        }
    }

    ErrorCode TryAddToPendingSendChunkOperation(uint64 const sequenceId, AsyncOperationSPtr const & operation)
    {
        ASSERT_IF(sequenceId >= totalFileChunks_, "Chunk sequence id cannot be greater than or equal to total chunks.");

        AcquireExclusiveLock lock(pendingFileChunkSendLock_);
        pendingFileChunkSendMap_[sequenceId] = operation;
        return ErrorCodeValue::Success;
    }

    // Check if ack already received. If not return true, else false
    bool IsFileChunkNeedsToBeSent(uint64 sequenceId)
    {
        if (!completedOrCanceled_.load())
        {
            AcquireExclusiveLock lock(pendingFileChunkSendLock_);
            return (fileChunkAcksReceivedSet_.find(sequenceId) == fileChunkAcksReceivedSet_.end());
        }
        else
        {
            return false;
        }
    }

private:
    /// Cancels the existing file chunk operation in progress
    void CancelFileChunkSendOperations()
    {
        fileChunkJobQueue_->Close();

        CancelFileChunkBatchTimer();

        std::vector<AsyncOperationSPtr> cancelChunksOperations;
        {
            AcquireExclusiveLock lock(pendingFileChunkSendLock_);
            completedOrCanceled_.store(true);
            for (auto const &entry : pendingFileChunkSendMap_)
            {
                cancelChunksOperations.emplace_back(move(entry.second));
            }
        }
        for (auto const &op : cancelChunksOperations)
        {
            if (op)
            {
                op->Cancel();
            }
        }
    }

    void ResendUnacknowledgedFileChunks(AsyncOperationSPtr const &thisSPtr)
    {
        {
            AcquireExclusiveLock lock(lock_);
            if (!allChunksSent_ || completedOrCanceled_.load())
            {
                return;
            }
        }

        if (resendRetryCount_.load() >= ClientConfig::GetConfig().FileChunkResendRetryAttempt)
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "{0}: Resend file chunks attempted {1} of tries so failing the send operation. Number of chunks without ack {2}, storeRelativePath:{3}",
                operationId_,
                ClientConfig::GetConfig().FileChunkResendRetryAttempt,
                (totalFileChunks_ - fileChunkAcksReceivedSet_.size()),
                storeRelativePath_);

            CancelFileChunkSendOperations();
            TryComplete(thisSPtr, ErrorCodeValue::SendFailed);

            return;
        }

        ++resendRetryCount_;

        std::vector<uint64> sequenceIdsToBeResent;
        {
            // All SendFileChunks completed, create new sendChunk async operations
            AcquireExclusiveLock lock(pendingFileChunkSendLock_);

            if (completedOrCanceled_.load())
                return;

            fileChunkSendCompletedSet_.clear();

            for (uint64 i = 0; i < totalFileChunks_; ++i)
            {
                if (fileChunkAcksReceivedSet_.find(i) == fileChunkAcksReceivedSet_.end())
                {
                    sequenceIdsToBeResent.emplace_back(i);
                }
            }

            expectedFileChunksSendCount_ = sequenceIdsToBeResent.size();
        }

        WriteWarning(
            TraceComponent,
            owner_.traceId_,
            "{0}: Resending file chunks. Total number of chunks to be sent: {1} storeRelativePath:{2} retryAttempt:{3}",
            operationId_,
            expectedFileChunksSendCount_,
            storeRelativePath_,
            resendRetryCount_.load());

        {
            AcquireExclusiveLock lock(lock_);
            allChunksSent_ = false;
        }

        {
            AcquireExclusiveLock lock(fileChunkBatchLock_);
            nextFileChunkIndexToBeSent_ = 0;
            expectedFileChunksToBeSent_.clear();
            expectedFileChunksToBeSent_.swap(sequenceIdsToBeResent);
        }

        CheckAndSendNextFileChunkBatch(thisSPtr);
        return;
    }

    // For each sent chunk, this callback is called.
    // Determine if all chunks are sent successfully, if so, start a resend timer which will send chunks periodically for unacknowledged chunks.
    void OnFileChunkTransferred(
        uint64 sequenceNumber,
        uint64 bytesSent,
        AsyncOperationSPtr const & operation)
    {
        if (completedOrCanceled_.load())
        {
            return;
        }

        auto error = SendFileChunkAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                owner_.traceId_,
                "{0}: FileChunkTransferred error storeRelativePath:{1} error:{2} sequenceId:{3} bytesSent:{4}",
                operationId_,
                storeRelativePath_,
                error,
                sequenceNumber,
                bytesSent);

            CancelFileChunkSendOperations();
            this->TryComplete(operation->Parent, move(error));
            return;
        }
        else
        {
            bool allChunksSent = false;
            {
                AcquireExclusiveLock lock(pendingFileChunkSendLock_);
                fileChunkSendCompletedSet_.insert(sequenceNumber);
                if (fileChunkSendCompletedSet_.size() == expectedFileChunksSendCount_) // check if all chunks sent for this iteration.
                {
                    allChunksSent = true;
                }
            }

            if (allChunksSent)
            {
                CancelFileChunkBatchTimer();
                AcquireExclusiveLock lock(lock_);
                {
                    allChunksSent_ = true;
                }

                TimerSPtr timer;
                AcquireExclusiveLock timerlock(timerlock_);
                {
                    if (timer_) 
                    { 
                        timer = move(timer_);
                    }

                    auto thisSPtr = operation->Parent;
                    timer_ = Timer::Create(
                        "FileChunkResendTimer",
                        [this, thisSPtr](TimerSPtr const &timer)
                    {
                        // Resend the chunks for whom the ack from FSS are not received.
                        ResendUnacknowledgedFileChunks(thisSPtr);
                        timer->Cancel();
                    },
                        false);
                    timer_->Change(TimeSpan::FromMilliseconds((double)ClientConfig::GetConfig().FileChunkResendWaitInterval.TotalMilliseconds()));
                }

                if (timer)
                {
                    timer->Cancel();
                }

                WriteNoise(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: All chunks sent for storeRelativePath:{1}. Number of chunks sent {2}. Now going to resend for missing acks.",
                    operationId_,
                    storeRelativePath_,
                    expectedFileChunksSendCount_);
            }
        }
    }

    bool IsRetryable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
        case ErrorCodeValue::TransportSendQueueFull:
        case ErrorCodeValue::NotReady:
        case ErrorCodeValue::OperationsPending:
            return true;
        default:
            break;
        }

        return false;
    }

    /// Sends file create action message and waits for the reply synchronously.
    /// In case of no reply ErrorCodeValue::ConnectionConfirmWaitExpired is returned,
    /// which the above layer understands and updates the protocol (chunk vs single file) if needed.
    ErrorCode SendFileCreateMessage()
    {
        ErrorCode error;
        int sendCount = 0;
        int sentSuccessfullyCount = 0;
        do
        {
            if (!owner_.UseChunkBasedUpload)
            {
                return ErrorCodeValue::ConnectionConfirmWaitExpired;
            }

            ClientServerRequestMessageUPtr fileCreateMessage = FileTransferTcpMessage::GetFileCreateMessage(
                operationId_,
                Actor::FileReceiver);

            fileCreateMessage->Headers.Replace(TimeoutHeader(this->RemainingTime));
            fileCreateMessage->Headers.Replace(FileUploadCreateRequestHeader(serviceName_, storeRelativePath_, shouldOverwrite_, fileSize_));

            if (owner_.includeClientVersionHeaderInMessage_)
            {
                fileCreateMessage->Headers.Replace(*ClientProtocolVersionHeader::CurrentVersionHeader);
            }

            error = owner_.SendMessage(sendTarget_, currentGateway_, fileCreateMessage, RemainingTime);
            if (!error.IsSuccess() && !IsRetryable(error))
            {
                // failure to send the chunk
                break;
            }
            else if (IsRetryable(error))
            {
                auto interval = min<int64>(ClientConfig::GetConfig().FileChunkRetryInterval.TotalMilliseconds(), this->RemainingTime.TotalMilliseconds());
                if (interval <= 0) return ErrorCodeValue::Timeout;

                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "{0}: SendOneWay during file create message failed with {1}. Retrying after {2} milliseconds.",
                    operationId_,
                    error,
                    interval);
                ::Sleep(static_cast<DWORD>(interval));
                ++sendCount;
                continue;
            }
            else
            {
                ++sentSuccessfullyCount;

                // Once we are successfully able to send it from the client side, we don't have to be aggressive in sending file create messages 
                // This can affect number of chunks we can enqueue to send it to other side
                if (sentSuccessfullyCount >= ClientConfig::GetConfig().NumberOfFileCreateRequestsSent)
                {
                    break;
                }

                // Wait for the response.
                error = fileCreateOperationCompleted_.Wait(static_cast<uint>(ClientConfig::GetConfig().FileCreateSendRetryInterval.TotalMilliseconds()));
                if (error.IsSuccess())
                {
                    owner_.AtleastOneCreateChunkResponse = true;
                    // Successful response
                    WriteInfo(
                        TraceComponent,
                        owner_.Root.TraceId,
                        "FileCreate got a success response for operationId:{0} storeRelativePath:{1} error:{2} chunkSize:{3}",
                        operationId_,
                        storeRelativePath_,
                        error,
                        maxAllowedChunkSize_);

                    return ErrorCodeValue::Success;
                }
                else if (error.IsWin32Error(WAIT_ABANDONED))
                {
                    WriteWarning(
                        TraceComponent,
                        owner_.Root.TraceId,
                        "FileCreate didn't get any response and wait was abandoned.operationId :{0} storeFilePath {1} Error: {2}",
                        operationId_,
                        storeRelativePath_,
                        error);

                    break;
                }
                if (error.IsError(ErrorCodeValue::Timeout))
                {
                    WriteNoise(
                        TraceComponent,
                        owner_.Root.TraceId,
                        "FileCreate didn't get any response. operationId:{0} storeRelativePath:{1} error:{2} sendAttemptCount:{3}",
                        operationId_,
                        storeRelativePath_,
                        error,
                        sendCount);
                    ++sendCount;
                    continue;
                }
            }
        } while (!completedOrCanceled_.load() && sendCount < ClientConfig::GetConfig().FileCreateSendAttempt);

        // The case where there is no response for the create message yet
        if (sentSuccessfullyCount > 0)
        {
            uint waitTimeInMilliSeconds;
            // Provide more time to get the resoponse since previous chunks based uploads have gotten response.
            // This is to avoid issue of prematurely trying to switch to old protocol since there could have been no response due to congestion.
            if (owner_.AtleastOneCreateChunkResponse)
            {
                // Wait for longer time since we know cluster can accept chunked version
                waitTimeInMilliSeconds = static_cast<uint>(ClientConfig::GetConfig().FileCreateMessageResponseWaitInterval.TotalMilliseconds());
            }
            else
            {
                // Initial wait time should be less as older version of cluster would never send response. So we wait for shorter time.
                waitTimeInMilliSeconds = static_cast<uint>(ClientConfig::GetConfig().FileCreateMessageInitialResponseWaitInterval.TotalMilliseconds());
            }

            error = fileCreateOperationCompleted_.Wait(waitTimeInMilliSeconds);
            if (error.IsSuccess())
            {
                owner_.AtleastOneCreateChunkResponse = true;

                // Successful response
                WriteInfo(
                    TraceComponent,
                    owner_.Root.TraceId,
                    "Additional wait time resulted in successful fileCreate response for operationId:{0} storeRelativePath:{1} error:{2} WaitTime:{3} chunkSize:{4}",
                    operationId_,
                    storeRelativePath_,
                    error,
                    waitTimeInMilliSeconds,
                    maxAllowedChunkSize_);

                return ErrorCodeValue::Success;
            }

            WriteWarning(
                TraceComponent,
                owner_.Root.TraceId,
                "File Create upload session message didn't get any response for operationId:{0} storeRelativePath:{1} error:{2} sendAttemptCount:{3} WaitTime:{4} atleastOneCreateChunkResponse:{5}",
                operationId_,
                storeRelativePath_,
                error,
                sendCount,
                waitTimeInMilliSeconds,
                owner_.AtleastOneCreateChunkResponse);

            return ErrorCodeValue::ConnectionConfirmWaitExpired;
        }
        else if (completedOrCanceled_.load())
        {
            return ErrorCodeValue::OperationCanceled;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                owner_.Root.TraceId,
                "File Create upload session message was unable to be sent for operationId:{0} storeRelativePath:{1} error:{2} sendAttemptCount:{3} atleastOneCreateChunkResponse:{5}",
                operationId_,
                storeRelativePath_,
                error,
                sendCount,
                owner_.AtleastOneCreateChunkResponse);

            return error;
        }
    }

    // For each file this processing is done.
    // Open the file, calculate the number of chunks to be sent
    // Start async operation to send each file chunks
    // Complete processing once the whole file is sent or if there is any error
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
                "{0}: Added operationId to ActiveSends: error:{1} sourceFullPath:{2}, storeRelativePath:{3}",
                operationId_,
                error,
                sourceFullPath_,
                storeRelativePath_);

            progressHandler = progressHandler_;
        }

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, move(error));
            return;
        }

        error = file_.TryOpen(
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
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        fileSize_ = file_.size();
        error = SendFileCreateMessage();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                owner_.traceId_,
                "Unable to send file create message for operationId:{0} storeRelativePath:{1} error:{2}",
                operationId_,
                storeRelativePath_,
                error);

            this->TryComplete(thisSPtr, move(error));
            return;
        }
        else
        {
            if (!fileCreateOperationError_.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.traceId_,
                    "File create message returned error:{0} for operationId:{1} storeRelativePath:{2}",
                    error,
                    operationId_,
                    storeRelativePath_);

                this->TryComplete(thisSPtr, move(fileCreateOperationError_));
                return;
            }
        }

        if (progressHandler.get() != nullptr)
        {
            progressHandler->IncrementTotalTransferItems(fileSize_);
            if (!retryAttempt_)
            {
                progressHandler->IncrementTotalFiles(1);
            }
        }

        if (fileSize_ == 0)
        {
            totalFileChunks_ = 1;
            expectedFileChunksSendCount_ = totalFileChunks_;
            auto const asyncOperation = AsyncOperation::CreateAndStart<SendFileChunkAsyncOperation>(
                owner_,
                operationId_,
                0,
                0,
                0,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnFileChunkTransferred(0, 0, operation);
            },
                thisSPtr);
        }
        else
        {
            totalFileChunks_ = (fileSize_ / maxAllowedChunkSize_) + ((fileSize_ % maxAllowedChunkSize_ == 0) ? 0 : 1);
            expectedFileChunksSendCount_ = totalFileChunks_;

            {
                AcquireExclusiveLock lock(fileChunkBatchLock_);
                expectedFileChunksToBeSent_.resize(totalFileChunks_);
                std::iota(std::begin(expectedFileChunksToBeSent_), std::end(expectedFileChunksToBeSent_), 0);
            }

            CheckAndSendNextFileChunkBatch(thisSPtr);
        }

        // block thread from processing other files
        auto uploadDone = fileSendOperationCompleted_.WaitOne(ClientConfig::GetConfig().PerFileUploadWaitTime);
        if (!uploadDone)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
    }

    /// For larger files when chunks are sent all at once, it causes lot of congestion while sending (TransportSendQueueFull) or
    /// at the gateway resulting in FabricGatewayUnreachable. To reduce the congestion on the sender side, we use flow control technique
    /// where file chunks are sent in batches. We would send the next batch only if previous send operations are completed for a threshold (ex:80%) of file chunks.
    /// When the first batch is sent, it checked whether next batch has to be sent. If so, a timer is created to periodically check and send the next batch.
    /// Once all the chunks are sent, timer is canceled. This operation is repeated if file chunks are to be resent(chunks which hasn't received ack from cluster) again.
    bool CheckAndSendNextFileChunkBatch(AsyncOperationSPtr const & thisSPtr)
    {
        vector<uint64> sequenceIdsToBeResent;
        uint64 startIndex, endIndex;
        bool createFileChunkBatchTimer = false;
        {
            // Atomically check if the next batch needs to be sent and if yes, update the nextFileChunkIndex
            AcquireExclusiveLock lock(fileChunkBatchLock_);

            bool firstBatch = (nextFileChunkIndexToBeSent_ == 0);
            uint64 lastFileChunkIndex = expectedFileChunksToBeSent_.size() - 1;
            if (nextFileChunkIndexToBeSent_ >= expectedFileChunksToBeSent_.size())
            {
                return false;
            }

            else if (nextFileChunkIndexToBeSent_ != 0 &&
                (fileChunkSendCompletedSet_.size() + ClientConfig::GetConfig().MaxAllowedPendingFileChunkSendBeforeNextBatch < nextFileChunkIndexToBeSent_))
            {
                // Check if the sent chunks has reached a threshold so as to send the next batch. If it has not reached the threshold return here
                return true;
            }

            startIndex = nextFileChunkIndexToBeSent_;
            auto expectedEndIndex = startIndex + ClientConfig::GetConfig().FileChunkBatchCount - 1;
            endIndex = lastFileChunkIndex >= expectedEndIndex ? expectedEndIndex : lastFileChunkIndex;

            std::copy_n(expectedFileChunksToBeSent_.begin(), expectedFileChunksToBeSent_.size(), std::back_inserter(sequenceIdsToBeResent));

            WriteInfo(
                TraceComponent,
                owner_.traceId_,
                "Sending next file chunk batch for operationId:{0} storeRelativePath:{1} startIndex:{2} endIndex:{3} totalFileChunks:{4}",
                operationId_,
                storeRelativePath_,
                startIndex,
                endIndex,
                totalFileChunks_);

            nextFileChunkIndexToBeSent_ = endIndex + 1;

            if (firstBatch && nextFileChunkIndexToBeSent_ <= expectedFileChunksToBeSent_.size() - 1)
            {
                createFileChunkBatchTimer = true;
            }
        }

        for (uint64 i = startIndex; i <= endIndex; i++)
        {
            uint64 startPosition, endPosition;
            uint64 sequenceId = sequenceIdsToBeResent[i];
            auto currentChunkSize = GetStartAndEndPosition(maxAllowedChunkSize_, sequenceId, startPosition, endPosition);

            auto const asyncOperation = AsyncOperation::CreateAndStart<SendFileChunkAsyncOperation>(
                owner_,
                operationId_,
                sequenceId,
                startPosition,
                endPosition,
                this->RemainingTime,
                [this, sequenceId, currentChunkSize](AsyncOperationSPtr const & operation)
            {
                this->OnFileChunkTransferred(sequenceId, currentChunkSize, operation);
            },
                thisSPtr);
        }

        if (createFileChunkBatchTimer)
        {
            CreateFileChunkBatchTimer(thisSPtr);
        }

        return true;
    }

    void CreateFileChunkBatchTimer(AsyncOperationSPtr const & thisSPtr)
    {
        TimerSPtr timer;
        AcquireExclusiveLock timerlock(fileChunkBatchTimerLock_);
        {
            if (fileChunkBatchTimer_)
            {
                timer = move(fileChunkBatchTimer_);
            }

            fileChunkBatchTimer_ = Timer::Create(
                FileChunkBatchSendTimerTag,
                [this, thisSPtr](TimerSPtr const &timer)
            {
                // check and resend next chunk batch if conditions met
                if (!CheckAndSendNextFileChunkBatch(thisSPtr))
                {
                    timer->Cancel();
                }
            },
                false);
            fileChunkBatchTimer_->Change(TimeSpan::FromMilliseconds((double)ClientConfig::GetConfig().FileChunkBatchUploadInterval.TotalMilliseconds()),
                TimeSpan::FromMilliseconds((double)ClientConfig::GetConfig().FileChunkBatchUploadInterval.TotalMilliseconds()));
        }

        if (timer)
        {
            timer->Cancel();
        }
    }

    void CancelFileChunkBatchTimer()
    {
        TimerSPtr timer;
        AcquireExclusiveLock timerlock(fileChunkBatchTimerLock_);
        {
            if (fileChunkBatchTimer_)
            {
                timer = move(fileChunkBatchTimer_);
            }
        }

        if (timer)
        {
            timer->Cancel();
        }
    }

    uint64 GetStartAndEndPosition(uint64 chunkSize, uint64 sequenceId, __out uint64 &startPosition, __out uint64 & endPosition)
    {
        if (fileSize_ != 0)
        {
            startPosition = sequenceId * chunkSize;
            endPosition = (startPosition + chunkSize - 1) < (uint64)(fileSize_ - 1) ? (startPosition + chunkSize - 1) : (fileSize_ - 1);
            return endPosition - startPosition + 1;
        }
        else
        {
            startPosition = endPosition = 0;
            return 0;
        }
    }

    // Returns the chunk size for the given sequence/chunk id
    int GetChunkSize(uint64 sequenceId)
    {
        if (sequenceId >= totalFileChunks_)
        {
            Assert::CodingError("Request chunk size for chunk id which doesn't exists. chunkId:{0}. operationId:{1} storeRelativePath:{2} totalChunks:{3}", sequenceId, operationId_, storeRelativePath_, totalFileChunks_);
        }

        // If the file size is 0, chunk size would be zero.
        // For chunks expect last one, it would be the max allowed chunk size.
        // The last chunk would be the remaining file content to be sent which can be less than max allowed chunk size.
        if (fileSize_ == 0)
        {
            return 0;
        }
        else if (sequenceId != totalFileChunks_ - 1)
        {
            return static_cast<int>(maxAllowedChunkSize_);
        }
        else
        {
            auto remainingSize = fileSize_ % maxAllowedChunkSize_;
            return remainingSize == 0 ? static_cast<int>(maxAllowedChunkSize_) : static_cast<int>(remainingSize);
        }
    }

private:
    wstring const serviceName_;
    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    bool const retryAttempt_;
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;
    Guid const operationId_;

    ISendTarget::SPtr sendTarget_;
    unique_ptr<GatewayDescription> currentGateway_;
    FileSender & owner_;
    File file_;
    int64 fileSize_;

    ManualResetEvent fileCreateOperationCompleted_; // When set it indicates a received response for file create operation
    Common::atomic_bool gotfileCreateOperationResponse_; // Indicates whether a response is received for file create operation (used to avoid handling multiple responses).
    ErrorCode fileCreateOperationError_; // Error captured for file create operation
    ManualResetEvent fileSendOperationCompleted_; // When set indicates this send operation can be removed from the queue
    Common::atomic_bool completedOrCanceled_; // Indicates if this async operation is in canceled or completed state
    std::unique_ptr<FileChunkJobQueue<FileSender>> fileChunkJobQueue_; // Chunk queue to send file chunks to the cluster
    uint64 totalFileChunks_; // total number of files chunks to be sent
    uint64 expectedFileChunksSendCount_; // Count of expected number of file chunks to be sent for a given iteration (only chunks without acks are resent).
    ExclusiveLock pendingFileChunkSendLock_; // lock protecting the map of sequence number and send file chunk async operation
    std::map<uint64, AsyncOperationSPtr> pendingFileChunkSendMap_; // Map between sequence number and SendFileChunk async operation
    std::set<uint64> fileChunkSendCompletedSet_; // Sequence number of file chunks whose send operation is completed
    std::set<uint64> fileChunkAcksReceivedSet_; // Sequence number of file chunks whose ack is received
    bool allChunksSent_; // Indicates completion of sending of all file chunks.  This is used for resending file chunk with no acks.
    ExclusiveLock lock_; // lock protection for activeSends_ map and allChunksSent_
    Common::atomic_long resendRetryCount_; // Number of times retrying chunks has been attempted
    TimerSPtr timer_; // Resend timer for chunks started after completion of first batch of sent chunks
    ExclusiveLock timerlock_; // lock for the resend timer
    uint gatewayUnreachableErrorCount_; // To determine whether gateway not reachable error is transient or not. (Protected by lock_).
    uint64 maxAllowedChunkSize_; // Maximum allowed chunk size by the cluster

    // Flow control to reduce congestion on the sender side by sending file chunks in batches.
    ExclusiveLock fileChunkBatchLock_;
    uint64 nextFileChunkIndexToBeSent_; // File chunks are sent in batches to avoid causing contention at the gateway. This tracks of next chunk to be sent.
    std::vector<uint64> expectedFileChunksToBeSent_; // List of chunks to be sent/resent. Protected by fileChunkBatchLock_
    TimerSPtr fileChunkBatchTimer_; // Timer for sending next file chunk batch.
    ExclusiveLock fileChunkBatchTimerLock_; // lock for the file chunk batch
};

FileSender::FileSender(
    ClientConnectionManagerSPtr const & connection,
    bool const includeClientVersionHeaderInMessage,
    wstring const & traceId,
    ComponentRoot const & root,
    bool const useChunkBasedUpload)
    : RootedObject(root)
    , connection_(connection)
    , transport_()
    , includeClientVersionHeaderInMessage_(includeClientVersionHeaderInMessage)
    , traceId_(traceId)
    , jobQueue_()
    , useChunkBasedUpload_(useChunkBasedUpload)
    , connectionToClusterFailureCount_(0)
    , atleastOneCreateChunkResponse_(false)
{
}

FileSender::FileSender(
    ClientServerTransportSPtr const & transport,
    bool const includeClientVersionHeaderInMessage,
    wstring const & traceId,
    ComponentRoot const & root,
    bool const useChunkBasedUpload)
    : RootedObject(root)
    , connection_()
    , transport_(transport)
    , includeClientVersionHeaderInMessage_(includeClientVersionHeaderInMessage)
    , traceId_(traceId)
    , jobQueue_()
    , useChunkBasedUpload_(useChunkBasedUpload)
    , connectionToClusterFailureCount_(0)
    , atleastOneCreateChunkResponse_(false)
{
}

unique_ptr<FileSender> FileSender::CreateOnClient(
    ClientConnectionManagerSPtr const & connection, 
    bool const includeClientVersionHeaderInMessage,
    std::wstring const & traceId,
    Common::ComponentRoot const & root,
    bool const useChunkBasedUpload)
{
    return unique_ptr<FileSender>(new FileSender(connection, includeClientVersionHeaderInMessage, traceId, root, useChunkBasedUpload));
}

unique_ptr<FileSender> FileSender::CreateOnGateway(
    ClientServerTransportSPtr const & transport,
    bool const includeClientVersionHeaderInMessage,
    std::wstring const & traceId,
    Common::ComponentRoot const & root,
    bool const useChunkBasedUpload)
{
    return unique_ptr<FileSender>(new FileSender(transport, includeClientVersionHeaderInMessage, traceId, root, useChunkBasedUpload));
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

    usingFileChunkUploads_.Close();

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

    auto isUsingChunkBasedUpload = UseChunkBasedUpload;
    if (!usingFileChunkUploads_.TryGet(operationId, isUsingChunkBasedUpload))
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "EndSendFileToGateway: Unable to find the operationId in the usingFileChunkUploads_ map for OperationId:{0}",
            operationId);

        // If there is no client version header or version less than 1.2, then old protocol reply is sent
        ClientProtocolVersionHeader versionHeader;
        if (!message->Headers.TryReadFirst(versionHeader))
        {
            isUsingChunkBasedUpload = false;
        }
        else if (versionHeader.Major <= 1 && versionHeader.Minor < 2)
        {
            isUsingChunkBasedUpload = false;
        }
    }

    if (isUsingChunkBasedUpload)
    {
        if (!UseChunkBasedUpload)
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "Dropping message for OperationId:{0}. Received response for chunk based but we are now in single file upload protocol. Getting response for stale operation.",
                operationId);
            return;
        }

        FileSequenceHeader sequenceHeader;
        if (!message->Headers.TryReadFirst(sequenceHeader))
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "Dropping message for OperationId:{0}. Failed to get FileSequenceHeader.",
                operationId);
            return;
        }

        auto sendChunkBasedOperation = AsyncOperation::Get<SendChunkBasedAsyncOperation>(operation);
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

            sendChunkBasedOperation->ProcessFileChunkAckMessage(operation, error, sequenceHeader.SequenceNumber, sequenceHeader.BufferSize);
        }
        else if (message->Action == FileTransferTcpMessage::ClientOperationSuccessAction)
        {
            sendChunkBasedOperation->ProcessFileChunkAckMessage(operation, ErrorCodeValue::Success, sequenceHeader.SequenceNumber, sequenceHeader.BufferSize);
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
    else
    {
        if (UseChunkBasedUpload)
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "Dropping message for OperationId:{0}. Received response for single file upload but we are now in chunk based protocol.",
                operationId);
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
}

Common::AsyncOperationSPtr FileSender::BeginSendFileToGateway(
    Common::Guid const & operationId,
    std::wstring const & serviceName,
    std::wstring const & sourceFullPath,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    bool const useChunkBasedUpload,
    bool const retryAttempt,
    IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
{
    // It is possible that upload operation would have started with chunk based protocol but in the meantime filesender would have switched to old protocol for all files.
    // It is also possbile thatupload operation would have switched to old protocol for this file but it is not switched to old protocol for all files in filesender yet.
    // In both cases above we should use old protocol (single file upload) instead of new chunk protocol.
    if (useChunkBasedUpload && this->UseChunkBasedUpload)
    {
        auto error = usingFileChunkUploads_.TryAdd(operationId, true);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "BeginSendFileToGateway: failed to add true to usingFileChunkUploads_ for OperationId:{0} with storeRelativePath:{1}",
                operationId,
                storeRelativePath);
        }

        return AsyncOperation::CreateAndStart<SendChunkBasedAsyncOperation>(
            *this,
            operationId,
            nullptr,
            serviceName,
            sourceFullPath,
            storeRelativePath,
            shouldOverwrite,
            retryAttempt,
            move(progressHandler),
            timeout,
            callback,
            parent);
    }
    else
    {
        auto error = usingFileChunkUploads_.TryAdd(operationId, false);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "BeginSendFileToGateway: failed to add to false usingFileChunkUploads_ for OperationId:{0} with storeRelativePath:{1}",
                operationId,
                storeRelativePath);
        }

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
}

ErrorCode FileSender::EndSendFileToGateway(Common::AsyncOperationSPtr const &operation, Guid const & operationId, __out bool &isUsingChunkBasedUpload)
{
    isUsingChunkBasedUpload = UseChunkBasedUpload;
    if (!usingFileChunkUploads_.TryGetAndRemove(operationId, isUsingChunkBasedUpload))
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "EndSendFileToGateway: Unable to find or delete the operationId in the usingFileChunkUploads_ map for OperationId:{0}",
            operationId);
    }

    if (isUsingChunkBasedUpload)
    {
        return SendChunkBasedAsyncOperation::End(operation);
    }
    else
    {
        return SendAsyncOperation::End(operation);
    }
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

bool FileSender::SwitchToSingleFileUploadProtocol(bool value)
{
    // If we have already switched then return here
    if (!UseChunkBasedUpload)
    {
        return false;
    }

    if (value)
    {
        ++connectionToClusterFailureCount_;
    }
    else
    {
        connectionToClusterFailureCount_.store(0);
    }

    // Update it only if consecutive failure count reaches the threshold
    if (connectionToClusterFailureCount_.load() >= static_cast<long>(ClientConfig::GetConfig().SwitchUploadProtocolThreshold))
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "Switching to single file upload protocol for all of filesender since consecutive failures of {0} have occurred.", 
            connectionToClusterFailureCount_.load());

        UseChunkBasedUpload = false;
        return true;
    }

    return false;
}


ErrorCode FileSender::SendMessage(ISendTarget::SPtr const &sendTarget, unique_ptr<GatewayDescription> &currentGateway, ClientServerRequestMessageUPtr & message, TimeSpan timeout)
{
    ErrorCode error(ErrorCodeValue::OperationFailed);
    if (sendTarget)
    {
        if (transport_)
        {
            error = transport_->SendOneWay(sendTarget, move(message), timeout);
        }
        else
        {
            auto const traceMessage = wformatString("BeginSendFileToClient cannot be called from client");
            WriteError(TraceComponent, traceId_, "{0}", traceMessage);
            Assert::TestAssert("{0}", traceMessage);
            return ErrorCodeValue::OperationFailed;
        }
    }
    else
    {
        if (connection_)
        {
            error = connection_->SendOneWayToGateway(move(message), currentGateway, timeout);
        }
        else
        {
            wstring traceMessage = wformatString("BeginSendFileToGateway cannot be called from gateway");
            WriteError(TraceComponent, traceId_, "{0}", traceMessage);
            Assert::TestAssert("{0}", traceMessage);
            return ErrorCodeValue::OperationFailed;
        }
    }

    return error;
}
