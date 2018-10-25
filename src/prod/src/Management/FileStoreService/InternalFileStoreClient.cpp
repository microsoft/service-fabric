// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("InternalFileStoreClient");
StringLiteral const RetryTimerTag("InternalFileStoreClient.RetryTimer");

class InternalFileStoreClient::CopyToStagingLocationAsyncOperation
    : public Common::AsyncOperation
    , protected Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
{
    DENY_COPY(CopyToStagingLocationAsyncOperation)

public:
    CopyToStagingLocationAsyncOperation(
        InternalFileStoreClient & owner,
        PartitionContextSPtr const & partition,
        wstring const & sourceFullPath,
        wstring const & destination,
        StagingLocationInfo const & previousStagingLocationInfo,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , partition_(partition)
        , sourceFullPath_(sourceFullPath)
        , destination_(destination)
        , stagingRelativePath_(Guid::NewGuid().ToString())
        , timeoutHelper_(timeout)
        , retryTimer_()
        , timerLock_()
        , stagingLocationInfo_(previousStagingLocationInfo)
        , accessDeniedErrorRetryCount_(0)
        , stagingCopyRetryCount_(0)
    {
    }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & operation,
        __out std::wstring & stagingRelativePath,
        __out StagingLocationInfo & stagingLocationInfo)
    {
        auto casted = AsyncOperation::End<CopyToStagingLocationAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            stagingRelativePath = casted->stagingRelativePath_;
            stagingLocationInfo = casted->stagingLocationInfo_;
        }

        return casted->Error;
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        this->GetStagingLocation(thisSPtr);
    }

    void GetStagingLocation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            owner_.TraceId,
            "Begin(GetStagingLocation): CurrentStagingLocationInfo:{0}, Destination:{1}, Source:{2}",
            stagingLocationInfo_,
            destination_,
            sourceFullPath_);

        auto operation = partition_->BeginGetStagingLocation(
            stagingLocationInfo_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnGetStagingLocationComplete(operation, false); },
            thisSPtr);

        this->OnGetStagingLocationComplete(operation, true);
    }

    void OnGetStagingLocationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        StagingLocationInfo newStagingLocation;
        auto error = partition_->EndGetStagingLocation(operation, newStagingLocation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(GetStagingLocation): PrevStagingLocationInfo:{0}, NewStagingLocationInfo:{1}, Error:{2}",
            stagingLocationInfo_,
            newStagingLocation,
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, move(error));
            return;
        }

        stagingLocationInfo_ = newStagingLocation;

        auto thisSPtr = operation->Parent;
        auto jobItem = make_unique<DefaultTimedJobItem<InternalFileStoreClient>>(
            this->timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](InternalFileStoreClient &) { this->CopyToStaging(thisSPtr); },
            [thisSPtr, this](InternalFileStoreClient &)
        {
            WriteWarning(
                TraceComponent,
                this->owner_.TraceId,
                "JobItem for CopyToStaging timed out before executing. Destination: {0}",
                this->destination_);

            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        });

        bool success = owner_.fileStoreJobQueue_->Enqueue(move(jobItem));
        if (!success)
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "Failed to enqueue CopyToStaging to JobQueue.");
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }
    }

    void CopyToStaging(AsyncOperationSPtr const & thisSPtr)
    {
        wstring destinationFullPath = Path::Combine(stagingLocationInfo_.ShareLocation, this->stagingRelativePath_);

        ASSERT_IFNOT(owner_.smbCopyContext_, "SMB copy context should be set");

        auto error = owner_.smbCopyContext_->CopyFileW(sourceFullPath_, destinationFullPath);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "CopyToStaging: Source:{0}, Destination:{1}, Error:{2}",
            sourceFullPath_,
            destinationFullPath,
            error);

        if (!error.IsSuccess())
        {
            // Retry getting the staging location to handle the case where primary fails over
            // after we get the staging location
            if (ImpersonatedSMBCopyContext::IsRetryableError(error))
            {
                if (stagingCopyRetryCount_ == FileStoreServiceConfig::GetConfig().MaxStagingCopyFailureRetryCount)
                {
                    wstring msg;
                    if (ImpersonatedSMBCopyContext::IsRetryableNetworkError(error))
                    {
                        msg = wformatString(StringResource::Get(IDS_FSS_Staging_SMBCopy_NetworkFailure), error);
                    }
                    else
                    {
                        msg = wformatString(StringResource::Get(IDS_FSS_Staging_SMBCopyFailure), error);
                    }

                    ErrorCode convertedError(error.ReadValue(), move(msg));
                    WriteWarning(
                        TraceComponent,
                        owner_.TraceId,
                        "CopyToStaging Failed. source:{0}, destination:{1}, error:{2} errorMessage:{3} retryCount:{4}",
                        sourceFullPath_,
                        destinationFullPath,
                        convertedError,
                        convertedError.Message,
                        stagingCopyRetryCount_);

                    this->TryComplete(thisSPtr, move(convertedError));
                    return;
                }

                ++stagingCopyRetryCount_;
                auto innerError = this->TryScheduleRetry(
                    thisSPtr,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->GetStagingLocation(thisSPtr); });

                if (innerError.IsSuccess())
                {
                    return;
                }
                else
                {
                    TryComplete(thisSPtr, move(innerError));
                    return;
                }
            }
            else if (error.IsError(ErrorCodeValue::AccessDenied))
            {
                bool doesStagingLocationExists = false;
                auto dirError = Directory::Exists(stagingLocationInfo_.ShareLocation, doesStagingLocationExists);
                WriteInfo(
                    TraceComponent,
                    owner_.TraceId,
                    "Staging Share location:{0}, Destination:{1}, OriginalError:{2} stagingExistsError:{3} DoesStagingLocationExists:{4} stagingRelativePath:{5}",
                    stagingLocationInfo_.ShareLocation,
                    destinationFullPath,
                    error,
                    dirError,
                    doesStagingLocationExists,
                    this->stagingRelativePath_);

                auto destPathExists = Directory::Exists(destinationFullPath);
                if (!destPathExists && doesStagingLocationExists && accessDeniedErrorRetryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
                {
                    WriteWarning(
                        TraceComponent,
                        owner_.TraceId,
                        "Destination dir doesn't exist: Retry CopyToStaging : Source:{0}, Destination:{1}, Error:{2}, retryCount={3}",
                        sourceFullPath_,
                        destinationFullPath,
                        error,
                        accessDeniedErrorRetryCount_);

                        ++accessDeniedErrorRetryCount_;
                        auto innerError = this->TryScheduleRetry(
                        thisSPtr,
                        [this](AsyncOperationSPtr const & thisSPtr) { this->GetStagingLocation(thisSPtr); });

                    if (innerError.IsSuccess())
                    {
                        return;
                    }
                    else
                    {
                        TryComplete(thisSPtr, move(innerError));
                        return;
                    }
                }
            }
        }

        if (!error.IsError(ErrorCodeValue::ObjectClosed))
        {
            this->TryComplete(thisSPtr, move(error));
        }
        else
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "CopyToStaging failed with errorCode ObjectClosed. Converting to errorCode OperationCanceled for source:{0} destination:{1}.",
                sourceFullPath_,
                destinationFullPath);

            // ScpCopy could return ObjectClosed error when it is unable to establish ScpSession for transfering files.
            // Returning ObjectClosed would cause CM to assume FSS have failed over and has the potential to block the upgrade.
            // So returning a retriable error code.
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        }
    }

private:

    ErrorCode TryScheduleRetry(
        AsyncOperationSPtr const & thisSPtr,
        function<void(AsyncOperationSPtr const &)> const & callback)
    {
        TimeSpan delay = FileStoreServiceConfig::GetConfig().ClientRetryInterval;
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::Timeout;
        }

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!this->InternalIsCompleted)
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

    InternalFileStoreClient & owner_;
    PartitionContextSPtr partition_;
    wstring sourceFullPath_;
    wstring destination_;
    TimeoutHelper timeoutHelper_;
    StagingLocationInfo stagingLocationInfo_;
    std::wstring stagingRelativePath_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
    uint32 accessDeniedErrorRetryCount_;
    uint32 stagingCopyRetryCount_;
};

class InternalFileStoreClient::OpenAsyncOperation : public LinkableAsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        InternalFileStoreClient & owner,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , queryClient_()
        , fileStoreServiceClient_()
        , partitions_()
        , retryTimer_()
        , timerLock_()
    {
    }

    OpenAsyncOperation(
        InternalFileStoreClient & owner,
        AsyncOperationSPtr const & primary,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(primary, callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , queryClient_()
        , fileStoreServiceClient_()
        , partitions_()
        , retryTimer_()
        , timerLock_()
    {
    }

    ~OpenAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<OpenAsyncOperation>(operation)->Error;
    }

protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
    {
        OpenFileStoreClient(thisSPtr);
    }

    void OnCompleted()
    {
        if (!this->IsSecondary)
        {
            {
                AcquireWriteLock lock(owner_.lock_);
                ASSERT_IF(owner_.pendingOpenOperation_.get() != this, "Invalid pendingOpenOperation operation.");
                ASSERT_IF(owner_.opened_, "FabricStoreClient should not be open.");

                owner_.fileStoreServiceClient_ = this->fileStoreServiceClient_;

                if (this->Error.IsSuccess())
                {
                    for (auto const& patition : this->partitions_)
                    {
                        owner_.partitions_.push_back(
                            make_shared<PartitionContext>(
                            owner_,
                            patition,
                            this->fileStoreServiceClient_));
                    }

                    owner_.smbCopyContext_ = move(smbCopyContext_);
                    owner_.opened_ = true;
                }

                owner_.pendingOpenOperation_.reset();
            }
        }

        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(timerLock_);
            snap = retryTimer_;
        }

        if (snap)
        {
            snap->Cancel();
        }

        LinkableAsyncOperation::OnCompleted();
    }

private:
    void OpenFileStoreClient(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.clientFactory_->CreateQueryClient(this->queryClient_);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, move(error));
            return;
        }

        error = owner_.clientFactory_->CreateInternalFileStoreServiceClient(this->fileStoreServiceClient_);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, move(error));
            return;
        }

        this->GetPartition(thisSPtr);
    }

    void GetPartition(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(GetPartitionList): ServiceName:{0}",
            owner_.serviceName_);

        NamingUri serviceName;
        bool success = NamingUri::TryParse(owner_.serviceName_, serviceName);
        TESTASSERT_IF(!success, "Could not parse service name: {0} to NamingUri", owner_.serviceName_);

        if(!success)
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidNameUri);
            return;
        }

        int64 timeoutTicks = min<int64>(
            timeoutHelper_.GetRemainingTime().Ticks, 
            FileStoreServiceConfig::GetConfig().QueryOperationTimeout.Ticks);
        auto operation = queryClient_->BeginGetPartitionList(
            serviceName,
            Guid::Empty() /*all partitions*/,
            wstring(), /*continuationToken*/
            TimeSpan::FromTicks(timeoutTicks),
            [this](AsyncOperationSPtr const & operation) { this->OnGetPartitionListComplete(operation, false); },
            thisSPtr);
        this->OnGetPartitionListComplete(operation, true);
    }

    void OnGetPartitionListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<ServicePartitionQueryResult> partitionList;
        PagingStatusUPtr pagingStatus;
        auto error = queryClient_->EndGetPartitionList(operation, partitionList, pagingStatus);
        // TODO: handle continuation token. if received, continue to get pages.
        ASSERT_IF(pagingStatus && !pagingStatus->ContinuationToken.empty(), "Received unexpected continuation token data: {0}", pagingStatus->ContinuationToken);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(GetPartitionList): ServiceName:{0}, PartitionCount:{1}, Error:{2}",
            owner_.serviceName_,
            partitionList.size(),
            error);

        if(!error.IsSuccess() && error.IsError(ErrorCodeValue::Timeout))
        {
            // Retry on Timeout if more time is available
            error = TryScheduleRetry(
                operation->Parent, 
                [this](AsyncOperationSPtr const & thisSPtr) { this->GetPartition(thisSPtr); });
            if(error.IsSuccess())
            {
                // Scheduled for retry
                return;
            }
        }

        if(error.IsSuccess())
        {
            for (ServicePartitionQueryResult const & partition : partitionList)
            {
                partitions_.push_back(partition.PartitionId);
            }            

            owner_.fileStoreJobQueue_ = make_unique<DefaultTimedJobQueue<InternalFileStoreClient>>(
                owner_.TraceId,
                owner_,
                false /*forceEnqueue*/,
                FileStoreServiceConfig::GetConfig().MaxClientOperationThreads /* maxThreads */);

            if(owner_.shouldImpersonate_)
            {
                error = ImpersonatedSMBCopyContext::Create(smbCopyContext_);
            }
        }

        TryComplete(operation->Parent, move(error));
    }

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
    {
        TimeSpan delay = FileStoreServiceConfig::GetConfig().ClientRetryInterval;
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::OperationCanceled;
        }

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!this->InternalIsCompleted)
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
    InternalFileStoreClient & owner_;
    TimeoutHelper timeoutHelper_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
    IQueryClientPtr queryClient_;
    IInternalFileStoreServiceClientPtr fileStoreServiceClient_;
    vector<Guid> partitions_;
    ImpersonatedSMBCopyContextSPtr smbCopyContext_;
};

class InternalFileStoreClient::ClientBaseAsyncOperation : public AsyncOperation
{
public:
    ClientBaseAsyncOperation(
        InternalFileStoreClient & owner,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , partition_()
        , retryTimer_()
        , timerLock_()
        , getPartitionRetryCount_(0)
    {
    }

    virtual ~ClientBaseAsyncOperation() {};

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool shouldOpen = false;
        {
            AcquireReadLock lock(owner_.lock_);
            if (!owner_.opened_)
            {
                shouldOpen = true;
            }
        }

        AsyncOperationSPtr linkedOpenOperation;
        if (shouldOpen)
        {
            AcquireWriteLock lock(owner_.lock_);
            if (!owner_.opened_)
            {
                if (owner_.pendingOpenOperation_)
                {
                    linkedOpenOperation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(
                        owner_,
                        owner_.pendingOpenOperation_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation) { this->OnLinkedOpenCompleted(operation, false); },
                        thisSPtr);
                }
                else
                {
                    linkedOpenOperation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(
                        owner_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation) { this->OnLinkedOpenCompleted(operation, false); },
                        thisSPtr);

                    owner_.pendingOpenOperation_ = linkedOpenOperation;
                }
            }
        }

        if (linkedOpenOperation)
        {
            auto casted = AsyncOperation::Get<OpenAsyncOperation>(linkedOpenOperation);
            casted->ResumeOutsideLock(linkedOpenOperation);
            this->OnLinkedOpenCompleted(linkedOpenOperation, true);
        }
        else
        {
            this->GetPartitionContextAndStartOperation(thisSPtr);
        }
    }

    virtual void OnStartOperation(AsyncOperationSPtr const & thisSPtr) = 0;

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    virtual bool IsRetryable(ErrorCode const & error)
    {
        return error.ReadValue() == ErrorCodeValue::Timeout
            || error.ReadValue() == ErrorCodeValue::OperationCanceled;
    }

    TimeSpan GetShortRequestTimeout(TimeSpan remainingTime)
    {
        int64 timeoutTicks = min<int64>(
            remainingTime.Ticks,
            FileStoreServiceConfig::GetConfig().ShortRequestTimeout.Ticks);
        return TimeSpan::FromTicks(timeoutTicks);
    }

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
    {
        TimeSpan delay = FileStoreServiceConfig::GetConfig().ClientRetryInterval;
        if (delay > timeoutHelper_.GetRemainingTime())
        {
            // Not enough timeout left - just fail early
            return ErrorCodeValue::Timeout;
        }

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!this->InternalIsCompleted)
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

    void OnCompleted()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(timerLock_);
            snap = retryTimer_;
        }

        if (snap)
        {
            snap->Cancel();
        }
    }

private:
    void OnLinkedOpenCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = OpenAsyncOperation::End(operation);
        if(error.IsSuccess())
        {
            this->GetPartitionContextAndStartOperation(operation->Parent);
            return;
        }

        TryComplete(operation->Parent, move(error));
    }

    void GetPartitionContextAndStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto const & partition : owner_.partitions_)
        {
            if(partition->DoesMatchPartition(0 /*currently building non-partitioned service*/))
            {
                partition_ = partition;
                break;
            }
        }

        if (!partition_)
        {
            if (getPartitionRetryCount_ < FileStoreServiceConfig::GetConfig().MaxFileOperationFailureRetryCount)
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "GetPartitionContext: Unable to get partition context. Count:{0} retryCount:{1}",
                    owner_.partitions_.size(),
                    getPartitionRetryCount_);

                ++getPartitionRetryCount_;
                auto innerError = TryScheduleRetry(thisSPtr,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->GetPartitionContextAndStartOperation(thisSPtr); });

                if (innerError.IsSuccess())
                {
                    return;
                }
                else
                {
                    TryComplete(thisSPtr, move(innerError));
                    return;
                }
            }
        }

        ASSERT_IFNOT(partition_, "The key should match atleast one partition. Owner partition count:{0}", owner_.partitions_.size());

        this->OnStartOperation(thisSPtr);
    }

protected:
    InternalFileStoreClient & owner_;
    PartitionContextSPtr partition_;
    TimeoutHelper timeoutHelper_;
    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
    uint getPartitionRetryCount_;
};

class InternalFileStoreClient::UploadAsyncOperation : public ClientBaseAsyncOperation
{
public:
    UploadAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , sourceFullPath_(sourceFullPath)
        , storeRelativePath_(storeRelativePath)
        , shouldOverwrite_(shouldOverwrite)
        , stagingLocationInfo_()
    {
    }

    ~UploadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<UploadAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if (sourceFullPath_.empty())
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        if (!File::Exists(sourceFullPath_) && !Directory::Exists(sourceFullPath_))
        {
            TryComplete(thisSPtr, ErrorCodeValue::FileNotFound);
            return;
        }

        this->CopyToStagingLocation(thisSPtr);
    }

    void CopyToStagingLocation(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<CopyToStagingLocationAsyncOperation>(
            this->owner_,
            this->partition_,
            this->sourceFullPath_,
            this->storeRelativePath_,
            this->stagingLocationInfo_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCopyToStagingLocationComplete(operation, false); },
            thisSPtr);

        this->OnCopyToStagingLocationComplete(operation, true);
    }

    void Upload(
        wstring stagingRelativePath,
        AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            owner_.TraceId,
            "Begin(Upload): Staging:{0}, Store:{1}, ShouldOverwrite:{2}, Partition:{3}",
            stagingRelativePath,
            storeRelativePath_,
            shouldOverwrite_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginUpload(            
            partition_->PartitionId,
            stagingRelativePath,
            storeRelativePath_,
            shouldOverwrite_,
            timeoutHelper_.GetRemainingTime(),
            [this, stagingRelativePath](AsyncOperationSPtr const & operation) { this->OnUploadComplete(operation, false, stagingRelativePath); },
            thisSPtr);
        this->OnUploadComplete(operation, true, stagingRelativePath);
    }

    void OnUploadComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, wstring const & stagingRelativePath)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndUpload(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(Upload): Store:{0}, Error:{1}",
            storeRelativePath_,
            error);

        bool refreshPrimary;
        if(!error.IsSuccess() && this->IsRetryable(error, refreshPrimary))
        {
            ErrorCode innerError;
            if(refreshPrimary)
            {
                innerError = this->TryScheduleRetry(
                    operation->Parent,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->CopyToStagingLocation(thisSPtr); });
            }
            else
            {
                innerError = this->TryScheduleRetry(
                    operation->Parent,
                    [this, stagingRelativePath](AsyncOperationSPtr const & thisSPtr) { this->Upload(stagingRelativePath, thisSPtr); });
            }

            if (innerError.IsSuccess())
            {
                return;
            }
            else
            {
                error = innerError;
            }
        }

        TryComplete(operation->Parent, move(error));
    }

    bool IsRetryable(ErrorCode const & error, __out bool & refreshPrimary)
    {
        
        switch(error.ReadValue())
        {
            // NotPrimary is converted to OperationCanceled at gateway
        case ErrorCodeValue::OperationCanceled:
        case ErrorCodeValue::StagingFileNotFound:
            refreshPrimary = true;
            return true;

        default:
            return false;
        }
    }

private:

    void OnCopyToStagingLocationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        std::wstring stagingRelativePath;

        auto error = CopyToStagingLocationAsyncOperation::End(operation, stagingRelativePath, this->stagingLocationInfo_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        Upload(stagingRelativePath, thisSPtr);
    }

    wstring const sourceFullPath_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    StagingLocationInfo stagingLocationInfo_;
};

class InternalFileStoreClient::CopyAsyncOperation : public ClientBaseAsyncOperation
{
public:
    CopyAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & sourceStoreRelativePath,
        StoreFileVersion const sourceVersion,
        wstring const & destinationStoreRelativePath,
        bool shouldOverwrite,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , sourceStoreRelativePath_(sourceStoreRelativePath)      
        , sourceVersion_(sourceVersion)
        , destinationStoreRelativePath_(destinationStoreRelativePath)
        , shouldOverwrite_(shouldOverwrite)
    {
    }

    ~CopyAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<CopyAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(Copy): Source:{0}, SourceVersion:{1}, Dest:{2}, ShouldOverwrite:{3}, Partition:{4}",
            sourceStoreRelativePath_,
            sourceVersion_,
            destinationStoreRelativePath_,
            shouldOverwrite_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginCopy(
            partition_->PartitionId,
            sourceStoreRelativePath_,
            sourceVersion_,
            destinationStoreRelativePath_,
            shouldOverwrite_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCopyComplete(operation, false); },
            thisSPtr);
        this->OnCopyComplete(operation, true);
    }

    void OnCopyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndCopy(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(Copy): Source:{0}, SourceVersion:{1}, Dest:{2}, ShouldOverwrite:{3}, Error:{4}",
            sourceStoreRelativePath_,
            sourceVersion_,
            destinationStoreRelativePath_,
            shouldOverwrite_,
            error);

        TryComplete(operation->Parent, move(error));
    }

private:
    wstring const sourceStoreRelativePath_;
    StoreFileVersion const sourceVersion_;
    wstring const destinationStoreRelativePath_;
    bool const shouldOverwrite_;
};

class InternalFileStoreClient::DownloadAsyncOperation : public ClientBaseAsyncOperation
{
public:
    DownloadAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & storeRelativePath,
        wstring const & destinationFullPath,
        StoreFileVersion const version,
        vector<std::wstring> const & availableShares,
        IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)        
        , storeRelativePath_(storeRelativePath)
        , destinationFullPath_(destinationFullPath)
        , version_(version)
        , availableShares_(availableShares)        
        , progressHandler_(move(progressHandler))
    {
    }

    ~DownloadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<DownloadAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnCompleted()
    {
        progressHandler_ = IFileStoreServiceClientProgressEventHandlerPtr();

        ClientBaseAsyncOperation::OnCompleted();
    }

    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {                 
        if(availableShares_.empty() 
            || storeRelativePath_.empty() 
            || destinationFullPath_.empty()
            || version_ == StoreFileVersion::Default)
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        auto jobItem = make_unique<DefaultTimedJobItem<InternalFileStoreClient>>(
            this->timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](InternalFileStoreClient &) { this->CopyFromStore(thisSPtr); },
            [thisSPtr, this](InternalFileStoreClient &)
        {
            WriteWarning(
                TraceComponent,
                this->owner_.TraceId,
                "JobItem for CopyFromStore timed out before executing. StoreRelativePath: {0}",
                this->storeRelativePath_);
            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        });

        bool success = owner_.fileStoreJobQueue_->Enqueue(move(jobItem));
        if(!success)
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "{0}: Failed to enqueue CopyFromStore to JobQueue.");
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }       
    }

private:
    void CopyFromStore(AsyncOperationSPtr const & thisSPtr)
    {
        auto destinationDirectory = Path::GetDirectoryName(destinationFullPath_);
        if(!Directory::Exists(destinationDirectory))
        {
            auto error = Directory::Create2(destinationDirectory);
            if(!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, move(error));
                return;
            }
        }

        ErrorCode lastError = ErrorCodeValue::FileNotFound;
        uint64 startIndex = GetRandomShareIndex(availableShares_.size());
        uint64 index = startIndex;
        do
        {
            if(timeoutHelper_.IsExpired)
            {
                lastError = ErrorCodeValue::Timeout;
                break;
            }

            wstring sourceFullPath = Utility::GetVersionedFileFullPath(availableShares_[index], storeRelativePath_, version_);
            lastError = owner_.smbCopyContext_->CopyFileW(sourceFullPath, destinationFullPath_);
            WriteTrace(
                lastError.ToLogLevel(),
                TraceComponent,
                owner_.TraceId,
                "Download: Source:{0}, Destination:{1}, Error:{2}",
                sourceFullPath,
                destinationFullPath_,
                lastError);

            if(lastError.IsSuccess())
            {
                break;
            }

            index = (++index % availableShares_.size());
        } while(index != startIndex);

        if (lastError.IsSuccess() && progressHandler_.get() != nullptr)
        {
            progressHandler_->IncrementCompletedItems(1);
        }

        TryComplete(thisSPtr, move(lastError));
    }

    uint64 GetRandomShareIndex(uint64 size)
    {
        Random rand;
        return rand.Next() % size;
    }

private:
    wstring const destinationFullPath_;
    wstring const storeRelativePath_;
    StoreFileVersion const version_;
    vector<wstring> const availableShares_;    
    IFileStoreServiceClientProgressEventHandlerPtr progressHandler_;
};

class InternalFileStoreClient::DeleteAsyncOperation : public ClientBaseAsyncOperation
{
public:
    DeleteAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & storeRelativePath,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
    {
    }

    ~DeleteAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<DeleteAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(Delete): Store:{0}, Partition:{1}",
            storeRelativePath_,            
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginDelete(            
            partition_->PartitionId,            
            storeRelativePath_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteComplete(operation, false); },
            thisSPtr);
        this->OnDeleteComplete(operation, true);
    }

    void OnDeleteComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndDelete(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(Delete): Store:{0}, Error:{1}",
            storeRelativePath_,            
            error);

        TryComplete(operation->Parent, move(error));
    }

private:
    wstring const storeRelativePath_;
};

class InternalFileStoreClient::CheckExistenceAsyncOperation : public ClientBaseAsyncOperation
{
public:
    CheckExistenceAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & storeRelativePath,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , result_(false)
    {
    }

    ~CheckExistenceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & result)
    {
        auto thisSPtr = AsyncOperation::End<CheckExistenceAsyncOperation>(operation);
        result = thisSPtr->result_;
        return thisSPtr->Error;
    }
        
protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(CheckExistence): store:{0}, Partition:{1}",
            storeRelativePath_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginCheckExistence(
            partition_->PartitionId,
            storeRelativePath_,
            GetShortRequestTimeout(timeoutHelper_.GetRemainingTime()),
            [this](AsyncOperationSPtr const & operation) { this->OnCheckExistenceComplete(operation, false); },
            thisSPtr);

        this->OnCheckExistenceComplete(operation, true);
    }

    void OnCheckExistenceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndCheckExistence(operation, result_);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(CheckExistence): Store:{0}, Existed:{1}, Error:{2}",
            storeRelativePath_,
            result_,
            error);

        if (!error.IsSuccess() && IsRetryable(error))
        {
            ErrorCode innerError = this->TryScheduleRetry(
                operation->Parent,
                [this](AsyncOperationSPtr const & thisSPtr) { this->OnStartOperation(thisSPtr); });

            if (!innerError.IsSuccess())
            {
                TryComplete(operation->Parent, move(innerError));
            }

            return;
        }

        TryComplete(operation->Parent, move(error));
    }

private:

    wstring const storeRelativePath_;
    bool result_;
};


class InternalFileStoreClient::ListAsyncOperation : public ClientBaseAsyncOperation
{
public:
    ListAsyncOperation(
        InternalFileStoreClient & owner,
        wstring const & storeRelativePath,
        wstring const & continuationToken,
        BOOLEAN const & shouldIncludeDetails,
        BOOLEAN const & isRecursive,
        bool isPaging,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , continuationToken_(continuationToken)
        , shouldIncludeDetails_(shouldIncludeDetails)
        , isRecursive_(isRecursive)
        , isPaging_(isPaging)
        , availableFiles_()
        , availableFolders_()
        , availableShares_()
    {
    }

    ~ListAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out StoreFileInfoMap & availableFiles, 
        __out vector<StoreFolderInfo> & availableFolders, 
        __out vector<wstring> & availableShares,
        __out wstring & continuationToken)
    {
        auto thisSPtr = AsyncOperation::End<ListAsyncOperation>(operation);

        if (thisSPtr->Error.IsSuccess())
        {
            if (!thisSPtr->availableFiles_.empty())
            {
                availableFiles = move(thisSPtr->availableFiles_);
            }

            if (!thisSPtr->availableFolders_.empty())
            {
                availableFolders = move(thisSPtr->availableFolders_);
            }

            if (!thisSPtr->availableShares_.empty())
            {
                availableShares = move(thisSPtr->availableShares_);
            }

            continuationToken = move(thisSPtr->continuationToken_);
        }
        
        return thisSPtr->Error;
    }


protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(List): Store:{0}, ShouldIncludeDetails:{1}, IsRecursive:{2}, Partition:{3}",
            storeRelativePath_,
            shouldIncludeDetails_,
            isRecursive_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginListFiles(
            partition_->PartitionId,
            storeRelativePath_,
            continuationToken_,
            shouldIncludeDetails_,
            isRecursive_,
            isPaging_,
            GetShortRequestTimeout(timeoutHelper_.GetRemainingTime()),
            [this](AsyncOperationSPtr const & operation) { this->OnListComplete(operation, false); },
            thisSPtr);
        this->OnListComplete(operation, true);
    }

    void OnListComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<StoreFileInfo> files;
        vector<StoreFolderInfo> folders;
        auto error = owner_.fileStoreServiceClient_->EndListFiles(operation, files, folders, availableShares_, continuationToken_);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(List): Store:{0}, FilesCount:{1}, FoldersCount:{2}, ContinuationToken:{3}, Error:{4}",
            storeRelativePath_,
            files.size(),
            folders.size(),
            continuationToken_,
            error);

        if (error.IsSuccess())
        {
            for (std::vector<StoreFileInfo>::size_type ix = 0; ix != files.size(); ++ix)
            {
                availableFiles_.insert(std::pair<std::wstring, StoreFileInfo>(files[ix].StoreRelativePath, files[ix]));
            }

            if (!folders.empty())
            {
                availableFolders_ = std::move(folders);
            }
        }
        else if (IsRetryable(error))
        {
            ErrorCode innerError = this->TryScheduleRetry(
                operation->Parent,
                [this](AsyncOperationSPtr const & thisSPtr) { this->OnStartOperation(thisSPtr); });

            if (!innerError.IsSuccess())
            {
                TryComplete(operation->Parent, move(innerError));
            }

            return;
        }
       
        TryComplete(operation->Parent, move(error));
    }

private:

    wstring const storeRelativePath_;
    wstring continuationToken_;
    BOOLEAN isRecursive_;
    bool isPaging_;
    BOOLEAN shouldIncludeDetails_;
    StoreFileInfoMap availableFiles_;
    vector<StoreFolderInfo> availableFolders_;
    vector<wstring> availableShares_;
};

class InternalFileStoreClient::ListUploadSessionAsyncOperation 
    : public ClientBaseAsyncOperation
{
public:
    ListUploadSessionAsyncOperation(
        InternalFileStoreClient & owner,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , sessionId_(sessionId)
    {
    }

    ~ListUploadSessionAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation, __out UploadSession & uploadSession)
    {
        auto thisSPtr = AsyncOperation::End<ListUploadSessionAsyncOperation>(operation);
        uploadSession = std::move(thisSPtr->uploadSession_);
        return thisSPtr->Error;
    }

protected:

    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(ListUploadSession): Store:{0}, SessionId:{1}, Partition:{2}",
            storeRelativePath_,
            sessionId_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginListUploadSession(
            partition_->PartitionId,
            storeRelativePath_,
            sessionId_,
            GetShortRequestTimeout(timeoutHelper_.GetRemainingTime()),
            [this](AsyncOperationSPtr const & operation) { this->OnListUploadSessionComplete(operation, false); },
            thisSPtr);

        this->OnListUploadSessionComplete(operation, true);
    }

    void OnListUploadSessionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        UploadSession uploadSession;
        auto error = owner_.fileStoreServiceClient_->EndListUploadSession(operation, uploadSession);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(ListUploadSession):Store:{0},SessionId:{1},Error:{2}",
            storeRelativePath_,
            sessionId_,
            error);

        if (error.IsSuccess())
        {
            this->uploadSession_ = std::move(uploadSession);
        }
        else if(IsRetryable(error))
        {
            ErrorCode innerError = this->TryScheduleRetry(
                operation->Parent,
                [this](AsyncOperationSPtr const & thisSPtr) { this->OnStartOperation(thisSPtr); });

            if (!innerError.IsSuccess())
            {
                TryComplete(operation->Parent, move(innerError));
            }
            
            return;
        }
       
        TryComplete(operation->Parent, move(error));
    }

private:

    wstring storeRelativePath_;
    Common::Guid sessionId_;
    UploadSession uploadSession_;
};

class InternalFileStoreClient::CreateUploadSessionAsyncOperation
    : public ClientBaseAsyncOperation
{
public:
    CreateUploadSessionAsyncOperation(
        InternalFileStoreClient & owner,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        uint64 fileSize,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , sessionId_(sessionId)
        , fileSize_(fileSize)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CreateUploadSessionAsyncOperation>(operation)->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->sessionId_ == Common::Guid::Empty())
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "Empty upload session Id");

            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(CreateUploadSession):Id:{0},Partition:{1}",
            this->sessionId_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginCreateUploadSession(
            partition_->PartitionId,
            this->storeRelativePath_,
            this->sessionId_,
            this->fileSize_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCreateComplete(operation, false); },
            thisSPtr);

        this->OnCreateComplete(operation, true);
    }

    void OnCreateComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        ErrorCode error = owner_.fileStoreServiceClient_->EndCreateUploadSession(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(CreateUploadSession):Session:{0},StoreRelativePath:{1},FileSize:{2},Error:{3}",
            this->sessionId_.ToString(),
            this->storeRelativePath_,
            this->fileSize_,
            error);

        TryComplete(operation->Parent, move(error));
    }

private:
    wstring storeRelativePath_;
    Common::Guid sessionId_;
    uint64 fileSize_;
};

class InternalFileStoreClient::DeleteUploadSessionAsyncOperation
    : public ClientBaseAsyncOperation
{
public:
    DeleteUploadSessionAsyncOperation(
        InternalFileStoreClient & owner,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , sessionId_(sessionId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DeleteUploadSessionAsyncOperation>(operation)->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(DeleteUploadSession):Id:{0},Partition:{1}",
            this->sessionId_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginDeleteUploadSession(
            partition_->PartitionId,
            this->sessionId_,
            GetShortRequestTimeout(timeoutHelper_.GetRemainingTime()),
            [this](AsyncOperationSPtr const & operation) { this->OnDeleteComplete(operation, false); },
            thisSPtr);

        this->OnDeleteComplete(operation, true);
    }

    void OnDeleteComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = owner_.fileStoreServiceClient_->EndDeleteUploadSession(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(DeleteUploadSession):Session:{0},Error:{1}",
            this->sessionId_.ToString(),
            error);

        if (!error.IsSuccess() && IsRetryable(error))
        {
            ErrorCode innerError = this->TryScheduleRetry(
                operation->Parent,
                [this](AsyncOperationSPtr const & thisSPtr) { this->OnStartOperation(thisSPtr); });

            if (!innerError.IsSuccess())
            {
                TryComplete(operation->Parent, move(innerError));
            }
           
            return;
        }

        TryComplete(operation->Parent, move(error));
    }

private:

    Common::Guid sessionId_;
};

class InternalFileStoreClient::CommitUploadSessionAsyncOperation
    : public ClientBaseAsyncOperation
{
public:
    CommitUploadSessionAsyncOperation(
        InternalFileStoreClient & owner,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , storeRelativePath_(storeRelativePath)
        , sessionId_(sessionId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CommitUploadSessionAsyncOperation>(operation)->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(CommitUploadSession):Id:{0},Partition:{1}",
            this->sessionId_,
            partition_->PartitionId);

        auto operation = owner_.fileStoreServiceClient_->BeginCommitUploadSession(
            partition_->PartitionId,
            this->storeRelativePath_,
            this->sessionId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnMoveComplete(operation, false); },
            thisSPtr);

        this->OnMoveComplete(operation, true);
    }

    void OnMoveComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        ErrorCode error = owner_.fileStoreServiceClient_->EndCommitUploadSession(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(CommitUploadSession):Session:{0},Error:{1}",
            this->sessionId_.ToString(),
            error);

        TryComplete(operation->Parent, move(error));
    }

private:
    std::wstring storeRelativePath_;
    Common::Guid sessionId_;
};

class InternalFileStoreClient::UploadChunkAsyncOperation 
    : public ClientBaseAsyncOperation
{
public:   
    UploadChunkAsyncOperation(
        InternalFileStoreClient & owner,
        std::wstring const & localSource,
        Common::Guid const & sessionId,
        uint64 startPosition,
        uint64 endPosition,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , localSource_(localSource)
        , sessionId_(sessionId)
        , startPosition_(startPosition)
        , endPosition_(endPosition)
        , stagingLocationInfo_()
    {
    }
        
    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UploadChunkAsyncOperation>(operation)->Error;
    }

protected:

    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->localSource_.empty())
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "Empty local source");

            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        if (!File::Exists(this->localSource_))
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "File doesn't exist {0}",
                this->localSource_);

            TryComplete(thisSPtr, ErrorCodeValue::FileNotFound);
            return;
        }

        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "Begin(UploadChunk):Id:{0},Range:'{1}'~'{2}',Partition:{3}",
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            partition_->PartitionId);

        this->CopyToStagingLocation(thisSPtr);
    }

private:

    void CopyToStagingLocation(AsyncOperationSPtr const & thisSPtr)
    {
        wstring chunkRange = Common::wformatString("{0}~{1}", this->startPosition_, this->endPosition_);
        auto operation = AsyncOperation::CreateAndStart<CopyToStagingLocationAsyncOperation>(
            this->owner_,
            this->partition_,
            this->localSource_,
            chunkRange,
            stagingLocationInfo_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCopyToStagingLocationComplete(operation, false); },
            thisSPtr);

        this->OnCopyToStagingLocationComplete(operation, true);
    }

    void OnCopyToStagingLocationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        AsyncOperationSPtr const & thisSPtr = operation->Parent;
        std::wstring stagingRelativePath;

        auto error = CopyToStagingLocationAsyncOperation::End(operation, stagingRelativePath, this->stagingLocationInfo_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        auto uploadChunkOperation = owner_.fileStoreServiceClient_->BeginUploadChunk(
            partition_->PartitionId,
            stagingRelativePath,
            this->sessionId_,
            this->startPosition_,
            this->endPosition_,
            timeoutHelper_.GetRemainingTime(),
            [this, stagingRelativePath](AsyncOperationSPtr const & operation) { this->OnUploadChunkComplete(operation, false, stagingRelativePath); },
            thisSPtr);

        this->OnUploadChunkComplete(uploadChunkOperation, true, stagingRelativePath);
    }

    void OnUploadChunkComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, wstring const & stagingRelativePath)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = owner_.fileStoreServiceClient_->EndUploadChunk(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(UploadChunk):Id:{0},Range:'{1}'~'{2}',Error:{3}",
            this->sessionId_.ToString(),
            this->startPosition_,
            this->endPosition_,
            error);

        if (error.IsError(ErrorCodeValue::OperationCanceled)
            || error.IsError(ErrorCodeValue::StagingFileNotFound))
        {
            wstring destinationStagingFullPath = Path::Combine(stagingLocationInfo_.ShareLocation, stagingRelativePath);

            // Delete the chunk file
            if (File::Exists(destinationStagingFullPath))
            {
                auto deleteError = File::Delete2(destinationStagingFullPath);
                if (!deleteError.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        owner_.TraceId,
                        "Deletion of staging chunk file failed with error {0} sessionId:{1} UploadChunkError:{2} stagingDestPath:{3}",
                        deleteError,
                        this->sessionId_.ToString(),
                        error,
                        destinationStagingFullPath);
                }
            }

            ErrorCode innerError = this->TryScheduleRetry(
                operation->Parent,
                [this](AsyncOperationSPtr const & thisSPtr) { this->CopyToStagingLocation(thisSPtr); });

            if (innerError.IsSuccess())
            {
                return;
            }
            else
            {
                TryComplete(operation->Parent, move(innerError));
                return;
            }
        }

        TryComplete(operation->Parent, move(error));
    }

    std::wstring localSource_;
    Common::Guid sessionId_;
    uint64 startPosition_;
    uint64 endPosition_;
    StagingLocationInfo stagingLocationInfo_;
};

class InternalFileStoreClient::UploadChunkContentAsyncOperation : public ClientBaseAsyncOperation
{
public:
    UploadChunkContentAsyncOperation(
        InternalFileStoreClient & owner,
        Transport::MessageUPtr && chunkContentMessage,
        Management::FileStoreService::UploadChunkContentDescription && uploadChunkContentDescription,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ClientBaseAsyncOperation(owner, timeout, callback, parent)
        , chunkContentMessage_(move(chunkContentMessage))
        , uploadChunkContentDescription_(move(uploadChunkContentDescription))
    {
    }

    ~UploadChunkContentAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<UploadChunkContentAsyncOperation>(operation);
        return thisSPtr->Error;
    }

protected:
    void OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            owner_.TraceId,
            "InternalFileStoreClient::UploadChunkContentAsyncOperation SessionId {0}",
            uploadChunkContentDescription_.SessionId);

        auto operation = owner_.fileStoreServiceClient_->BeginUploadChunkContent(
            partition_->PartitionId,
            chunkContentMessage_,
            uploadChunkContentDescription_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnUploadChunkContentComplete(operation, false); },
            thisSPtr);
        this->OnUploadChunkContentComplete(operation, true);
    }

    void OnUploadChunkContentComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreServiceClient_->EndUploadChunkContent(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            owner_.TraceId,
            "End(InternalFileStoreClient::UploadChunkContent): operationId:{0} Error:{1}",
            uploadChunkContentDescription_.SessionId,
            error);

        TryComplete(operation->Parent, error);
    }

private:
    Management::FileStoreService::UploadChunkContentDescription uploadChunkContentDescription_;
    Transport::MessageUPtr chunkContentMessage_;
};

InternalFileStoreClient::InternalFileStoreClient(
    wstring const & serviceName,
    IClientFactoryPtr const & clientFactory,
    bool const shouldImpersonate) 
    : serviceName_(serviceName)
    , clientFactory_(clientFactory)
    , fileStoreServiceClient_() /* initialized during open */
    , partitions_()
    , lock_()
    , opened_(false)
    , pendingOpenOperation_()
    , shouldImpersonate_(shouldImpersonate)
    , fileStoreJobQueue_()
{
}

InternalFileStoreClient::~InternalFileStoreClient()
{
}

ErrorCode InternalFileStoreClient::SetSecurity(Transport::SecuritySettings && securitySettings)
{
    IClientSettingsPtr clientSettings;
    auto error = clientFactory_->CreateSettingsClient(clientSettings);

    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "SetSecurity: Error:{0}",
        error);

    if (error.IsSuccess())
    {
        clientSettings->SetSecurity(move(securitySettings));
    }

    return error;
}

AsyncOperationSPtr InternalFileStoreClient::BeginUpload(
    wstring const & sourceFullPath,
    wstring const & storeRelativePath,
    bool shouldOverwrite,
    IFileStoreServiceClientProgressEventHandlerPtr &&,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<UploadAsyncOperation>(
        *this,
        sourceFullPath,
        storeRelativePath,
        shouldOverwrite,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndUpload(AsyncOperationSPtr const & operation)
{
    return UploadAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginCopy(
    wstring const & sourceStoreRelativePath,
    StoreFileVersion const sourceVersion,
    wstring const & destinationStoreRelativePath,
    bool shouldOverwrite,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<CopyAsyncOperation>(
        *this,
        sourceStoreRelativePath,
        sourceVersion,
        destinationStoreRelativePath,
        shouldOverwrite,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndCopy(
    AsyncOperationSPtr const &operation)
{
    return CopyAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginDownload(
    wstring const & storeRelativePath,
    wstring const & destinationFullPath,
    StoreFileVersion const version,
    vector<std::wstring> const & availableShares,
    IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<DownloadAsyncOperation>(
        *this,
        storeRelativePath,
        destinationFullPath,
        version,
        availableShares,
        move(progressHandler),
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndDownload(AsyncOperationSPtr const &operation)
{
    return DownloadAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginCheckExistence(
    std::wstring const & storeRelativePath,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<CheckExistenceAsyncOperation>(
        *this,
        storeRelativePath,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndCheckExistence(
    Common::AsyncOperationSPtr const &operation,
    __out bool & result)
{
    return CheckExistenceAsyncOperation::End(operation, result);
}

AsyncOperationSPtr InternalFileStoreClient::BeginList(
    wstring const & storeRelativePath,
    wstring const & continuationToken,
    BOOLEAN	const & shouldIncludeDetails,
    BOOLEAN const & isRecursive,
    bool isPaging,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ListAsyncOperation>(
        *this,
        storeRelativePath,
        continuationToken,
        shouldIncludeDetails,
        isRecursive,
        isPaging,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndList(
    AsyncOperationSPtr const &operation,
    __out StoreFileInfoMap & availableFiles,
    __out std::vector<StoreFolderInfo> & availableFolders,
    __out std::vector<std::wstring> & availableShares,
    __out wstring & continuationToken)
{
    return ListAsyncOperation::End(operation, availableFiles, availableFolders, availableShares, continuationToken);
}

AsyncOperationSPtr InternalFileStoreClient::BeginDelete(
    wstring const & storeRelativePath,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<DeleteAsyncOperation>(
        *this,
        storeRelativePath,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndDelete(AsyncOperationSPtr const &operation)
{
    return DeleteAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginListUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ListUploadSessionAsyncOperation>(
        *this,
        storeRelativePath,
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndListUploadSession(
    Common::AsyncOperationSPtr const & operation,
    __out UploadSession & uploadSession)
{
    return ListUploadSessionAsyncOperation::End(operation, uploadSession);
}

AsyncOperationSPtr InternalFileStoreClient::BeginCreateUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    uint64 fileSize,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CreateUploadSessionAsyncOperation>(
        *this,
        storeRelativePath,
        sessionId,
        fileSize,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndCreateUploadSession(
    Common::AsyncOperationSPtr const & operation)
{
    return CreateUploadSessionAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginUploadChunk(
    std::wstring const & localSource,
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UploadChunkAsyncOperation>(
        *this,
        localSource,
        sessionId,
        startPosition,
        endPosition,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndUploadChunk(
    Common::AsyncOperationSPtr const & operation)
{
    return UploadChunkAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginUploadChunkContent(
    Transport::MessageUPtr & chunkContentMessage,
    Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UploadChunkContentAsyncOperation>(
        *this,
        move(chunkContentMessage),
        move(uploadChunkContentDescription),
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndUploadChunkContent(
    Common::AsyncOperationSPtr const & operation)
{
    return UploadChunkContentAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginDeleteUploadSession(
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteUploadSessionAsyncOperation>(
        *this,
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndDeleteUploadSession(AsyncOperationSPtr const &operation)
{
    return DeleteUploadSessionAsyncOperation::End(operation);
}

AsyncOperationSPtr InternalFileStoreClient::BeginCommitUploadSession(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CommitUploadSessionAsyncOperation>(
        *this,
        storeRelativePath,
        sessionId,
        timeout,
        callback,
        parent);
}

ErrorCode InternalFileStoreClient::EndCommitUploadSession(
    Common::AsyncOperationSPtr const & operation)
{
    return CommitUploadSessionAsyncOperation::End(operation);
}
    
    
