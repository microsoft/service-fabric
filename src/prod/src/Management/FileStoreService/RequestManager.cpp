// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management::FileStoreService;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("RequestManager");

class RequestManager::RecoverAsyncOperation : public AsyncOperation
{
public:
    RecoverAsyncOperation(RequestManager & requestManager, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(requestManager)
        , filesToCopy_()
        , filesToDelete_()
        , initializedEvent_(false)
        , pendingRecoveryCount_(0)
        , completionError_(ErrorCodeValue::Success)
        , completionLock_()
        , expectDataLoss_(false)
    {
    }

    ~RecoverAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisSPtr = AsyncOperation::End<RecoverAsyncOperation>(operation);
        return thisSPtr->Error;
    }

    bool ExpectDataLoss()
    {
        WriteInfo(
            TraceComponent,
            owner_.TraceId,
            "Setting expect data loss");

        initializedEvent_.WaitOne();

        expectDataLoss_ = true;

        return !filesToCopy_.empty();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->ScheduleStart(TimeSpan::Zero, thisSPtr);
    }

    void ScheduleStart(TimeSpan const delay, AsyncOperationSPtr const & thisSPtr)
    {
        if (this->InternalIsCompleted) { return; }

        {
            AcquireWriteLock lock(timerLock_);

            startRetryTimer_ = Timer::Create(
                TimerTagDefault,
                [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->OnScheduledStart(thisSPtr);
                },
                false);
        }

        auto error = owner_.TransitionToRecoveryScheduled();
        if (error.IsSuccess())
        {
            startRetryTimer_->Change(delay);
        }
        else
        {
            this->CleanupTimers();

            this->TryComplete(thisSPtr, error);
        }
    }

    void OnScheduledStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToRecoveryStarting();
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
                
        ActivityId activityId;
        StoreTransactionUPtr storeTx;
        error = owner_.ReplicaStoreWrapperUPtr->CreateTransaction(activityId, storeTx);
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        vector<FileMetadata> metadataList;
        error = owner_.ReplicaStoreWrapperUPtr->ReadPrefix(*storeTx, L"", metadataList);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "Unable to read Metadata list during recovery. Error:{0}. ActivityId: {1}.",
                error,
                activityId);
            ScheduleStart(FileStoreServiceConfig::GetConfig().RecoveryRetryInterval, thisSPtr);
            return;
        }

        VersionNumber version;
        error = owner_.ReplicaStoreWrapperUPtr->ReadExact(*storeTx, version);
        if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "Unable to read VersionNumber during recovery. Error:{0}. ActivityId: {1}.",
                error,
                activityId);
            ScheduleStart(FileStoreServiceConfig::GetConfig().RecoveryRetryInterval, thisSPtr);
            return;
        }

        WriteInfo(
            TraceComponent,
            owner_.TraceId,
            "VersionNumber read from store: {0}, Found:{1}",
            version,
            error.IsSuccess());
       
        auto existingFiles = Utility::GetAllFiles(owner_.ReplicaObj.StoreRoot);

        // When creating/updating a new file, we replicate a transient "Updating" state before
        // actually making it available in the filesystem. This avoids having to
        // look for orphaned files in the filesystem during recovery - we only have
        // to scan for transient metadata and revert the filesystem change accordingly.
        //
        for(auto const & metadata : metadataList)
        {        
            auto currentFileName = Utility::GetVersionedFileFullPath(
                owner_.ReplicaObj.StoreRoot, 
                metadata.StoreRelativeLocation, 
                metadata.CurrentVersion);

            if (FileState::IsStable(metadata.State))
            {
                if (existingFiles.find(currentFileName) == existingFiles.end())
                {
                    WriteNoise(
                        TraceComponent,
                        owner_.TraceId,
                        "Metadata of file to copy: {0}",
                        metadata);

                    filesToCopy_.push_back(metadata);
                }
            }
            else
            {
                WriteNoise(
                    TraceComponent,
                    owner_.TraceId,
                    "Metadata of file to delete: {0}",
                    metadata);

                if (existingFiles.find(currentFileName) != existingFiles.end())
                {
                    File::Delete2(currentFileName, true /*deleteReadOnlyFiles*/).ReadValue();
                }

                auto previousVersion = Utility::GetVersionedFileFullPath(
                    owner_.ReplicaObj.StoreRoot, 
                    metadata.StoreRelativeLocation, 
                    metadata.PreviousVersion);

                // As per Vaishnav: This only works because FileStoreService relies on the ClusterManager
                // to gate provision requests and not attempt to re-provision an application type that
                // already exists. Otherwise, this can result in data loss if failover occurs when
                // and existing stable file is put into the updating state.
                //
                // As a general file store service, we should instead be preserving the previous version
                // and rolling back the metadata.
                //
                if (existingFiles.find(previousVersion) != existingFiles.end())
                {
                    WriteInfo(
                        TraceComponent,
                        owner_.TraceId,
                        "Deleting previous version of {0}: version={1}",
                        metadata,
                        previousVersion);

                    File::Delete2(previousVersion, true /*deleteReadOnlyFiles*/).ReadValue();
                }

                filesToDelete_.push_back(metadata);
            }
        } // for each file metadata

        initializedEvent_.Set();

        error = owner_.TransitionToRecoveryStarted();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "TransitionToRecoveryStarted failed. Error:{0} ActivityId:{1}",
                error,
                activityId);

            ScheduleStart(FileStoreServiceConfig::GetConfig().RecoveryRetryInterval, thisSPtr);
            return;
        }

        // 1) Delete orphaned files
        // 2) Copy missing files
        //
        pendingRecoveryCount_ = 2;

        if (filesToCopy_.empty())
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "No missing files to : ActivityId:{0}",
                activityId);

            this->OnRecoveryOperationComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            this->ScheduleCopyMissingFiles(activityId, TimeSpan::Zero, thisSPtr);
        }

        if (filesToDelete_.empty())
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "No transient metadata to delete: ActivityId:{0}",
                activityId);

            this->OnRecoveryOperationComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            this->DeleteOrphanedFiles(activityId, filesToDelete_, thisSPtr);
        }
    }

    void DeleteOrphanedFiles(ActivityId const & activityId, vector<FileMetadata> const & filesToDelete, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
            owner_,
            false, // useSimpleTx
            [this, activityId, &filesToDelete] (StoreTransaction const & storeTx) { return this->DeleteMetadata(storeTx, activityId, filesToDelete); },
            activityId,
            TimeSpan::MaxValue,
            [this, activityId] (AsyncOperationSPtr const & operation) { this->OnStoreTransactionCompleted(activityId, operation, false); },
            thisSPtr);
        this->OnStoreTransactionCompleted(activityId, operation, true);
    }

    ErrorCode DeleteMetadata(StoreTransaction const & storeTx, ActivityId const & activityId, vector<FileMetadata> const & filesToDelete)
    {
        for (auto const & metadata : filesToDelete)
        {         
            auto error = storeTx.Delete(metadata);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "Unable to delete transient Metadata: {0}, Error:{1}, ActivityId:{2}",
                    metadata,
                    error,
                    activityId);
                return error;
            }
        }        

        return ErrorCodeValue::Success;
    }
    
    void OnStoreTransactionCompleted(ActivityId const & activityId, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = StoreTransactionAsyncOperation::End(operation);

        WriteInfo(
            TraceComponent,
            owner_.TraceId,
            "Transient metadata deletion complete: Error:{0} ActivityId:{1}",
            error,
            activityId);

        this->OnRecoveryOperationComplete(operation->Parent, error);
    }

    void ScheduleCopyMissingFiles(ActivityId const & activityId, TimeSpan const delay, AsyncOperationSPtr const & thisSPtr)
    {
        if (this->InternalIsCompleted) { return; }

        {
            AcquireWriteLock lock(timerLock_);

            copyRetrytimer_ = Timer::Create(
                TimerTagDefault,
                [this, activityId, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->StartCopyMissingFiles(activityId, thisSPtr);
                },
                false);
        }

        copyRetrytimer_->Change(delay);
    }

    void StartCopyMissingFiles(ActivityId const & activityId, AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.ReplicaObj.StoreClient->BeginInternalGetStoreLocations(
            owner_.ReplicaObj.PartitionId,
            FileStoreServiceConfig::GetConfig().InternalServiceCallTimeout,
            [this, activityId] (AsyncOperationSPtr const & operation) { this->OnListFilesComplete(activityId, operation, false); },
            thisSPtr);
        this->OnListFilesComplete(activityId, operation, true);
    }

    void OnListFilesComplete(ActivityId const & activityId, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        vector<wstring> secondaryShares;
        auto error = owner_.ReplicaObj.StoreClient->EndInternalGetStoreLocations(operation, secondaryShares);

        if (!error.IsSuccess())
        {
            auto delay = FileStoreServiceConfig::GetConfig().RecoveryRetryInterval;

            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "ListFiles failed - retrying: Error:{0} ActivityId:{1} Delay:{2}",
                error,
                activityId,
                delay);

            this->ScheduleCopyMissingFiles(activityId, delay, thisSPtr);
        }
        else
        {
            int failures = 0;
            for (auto const & metadata : filesToCopy_)
            {
                auto currentDestinationFilePath = Utility::GetVersionedFileFullPath(
                    owner_.ReplicaObj.StoreRoot, 
                    metadata.StoreRelativeLocation, 
                    metadata.CurrentVersion);

                if (File::Exists(currentDestinationFilePath))
                {
                    WriteInfo(
                        TraceComponent,
                        owner_.TraceId,
                        "File already copied: {0}",
                        currentDestinationFilePath);

                    continue;
                }

                for (auto const & secondaryShare : secondaryShares)
                {
                    auto currentSourceFilePath = Utility::GetVersionedFileFullPath(
                        secondaryShare, 
                        metadata.StoreRelativeLocation, 
                        metadata.CurrentVersion);

                    error = owner_.ReplicaObj.SmbContext->CopyFileW(currentSourceFilePath, currentDestinationFilePath);
                    if (error.IsSuccess()) 
                    { 
                        WriteInfo(
                            TraceComponent,
                            owner_.TraceId,
                            "Copied missing file: {0} -> {1}",
                            currentSourceFilePath,
                            currentDestinationFilePath);

                        break;
                    }
                    else
                    {
                        WriteInfo(
                            TraceComponent,
                            owner_.TraceId,
                            "Failed to copy missing file: {0} -> {1}: Error:{2}",
                            currentSourceFilePath,
                            currentDestinationFilePath,
                            error);

                        ++failures;
                    }
                } // secondaryShares
            } // filesToCopy_

            // A stable file (Available_V1, Committed) must exist on at least one replica since it was 
            // quorum acked unless there was dataloss.
            //
            if (failures > 0)
            {
                if (expectDataLoss_)
                {
                    WriteInfo(
                        TraceComponent,
                        owner_.TraceId,
                        "Delete missing files on dataloss: ActivityId:{0}",
                        activityId);

                    this->DeleteOrphanedFiles(activityId, filesToCopy_, thisSPtr);
                }
                else
                {
                    auto delay = FileStoreServiceConfig::GetConfig().RecoveryRetryInterval;

                    WriteInfo(
                        TraceComponent,
                        owner_.TraceId,
                        "Retrying {0} file copy failures: ActivityId:{1} Delay:{2}",
                        error,
                        activityId,
                        delay);

                    this->ScheduleCopyMissingFiles(activityId, delay, thisSPtr);
                }
            }
            else
            {
                this->OnRecoveryOperationComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }
    }

    void OnRecoveryOperationComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        bool shouldComplete = false;
        {
            AcquireExclusiveLock lock(completionLock_);

            completionError_ = ErrorCode::FirstError(completionError_, error);

            shouldComplete = (--pendingRecoveryCount_ == 0);
        }

        if (shouldComplete)
        {
            this->TryComplete(thisSPtr, completionError_);
        }
    }

    void CleanupTimers()
    {
        initializedEvent_.Set();

        AcquireWriteLock lock(timerLock_);

        if (startRetryTimer_)
        {
            startRetryTimer_->Cancel();
        }

        if (copyRetrytimer_)
        {
            copyRetrytimer_->Cancel();
        }
    }

    virtual void OnCancel()
    {
        this->CleanupTimers();

        AsyncOperation::OnCancel();
    }

private:
    RequestManager & owner_;
    vector<FileMetadata> filesToCopy_;
    vector<FileMetadata> filesToDelete_;
    ManualResetEvent initializedEvent_;

    int pendingRecoveryCount_;
    ErrorCode completionError_;
    ExclusiveLock completionLock_;

    TimerSPtr startRetryTimer_;
    TimerSPtr copyRetrytimer_;
    ExclusiveLock timerLock_;

    bool expectDataLoss_;
};

RequestManager::RequestManager(
    ImageStoreServiceReplicaHolder const & serviceReplicaHolder,
    std::wstring const & stagingShareLocation,
    std::wstring const & storeShareLocation,
    int64 const epochDataLossNumber,
    int64 const epochConfigurationNumber,
    ServiceRoutingAgentProxy & routingAgentProxy)
    : StateMachine(Created),
    localStagingLocation_(),
    stagingShareLocation_(),
    storeShareLocation_(storeShareLocation),
    serviceReplicaHolder_(serviceReplicaHolder),
    epochDataLossNumber_(epochDataLossNumber),
    epochConfigurationNumber_(epochConfigurationNumber),
    routingAgentProxy_(routingAgentProxy),
    pendingWriteOperations_(),
    versionNumber_(0),
    recoveryOperation_(),
    recoveryOperationLock_(),
    requestProcessingJobQueue_(),
    fileOperationProcessingJobQueue_()
{
    this->SetTraceId(serviceReplicaHolder.Value.PartitionedReplicaId.TraceId);

    this->uploadSessionMap_ = make_unique<Management::FileStoreService::UploadSessionMap>(*this,  serviceReplicaHolder.Root);
    wstring tempStagingFolderName = Guid::NewGuid().ToString();
    localStagingLocation_ = Path::Combine(serviceReplicaHolder.Value.StagingRoot, tempStagingFolderName);
    stagingShareLocation_ = Path::Combine(stagingShareLocation, tempStagingFolderName);
}

RequestManager::~RequestManager()
{
}

ErrorCode RequestManager::Open()
{
    replicatedStoreWrapper_ = make_unique<ReplicatedStoreWrapper>(*this);

    requestProcessingJobQueue_ = make_unique<DefaultJobQueue<RequestManager>>(
        wformatString("{0}+{1}", TraceId, L"requestProcessing"),
        *this,
        true /*forceEnqueue*/,
        FileStoreServiceConfig::GetConfig().MaxRequestProcessingThreads);

    fileOperationProcessingJobQueue_ = make_unique<DefaultTimedJobQueue<RequestManager>>(
        wformatString("{0}+{1}", TraceId, L"fileOperationProcessing"),
        *this,
        false /*forceEnqueue*/,
        FileStoreServiceConfig::GetConfig().MaxFileOperationThreads);

    storeTransactionProcessingJobQueue_ = make_unique<DefaultTimedJobQueue<RequestManager>>(
        wformatString("{0}+{1}", TraceId, L"storeTransactionProcessing"),
        *this,
        false /*forceEnqueue*/,
        FileStoreServiceConfig::GetConfig().MaxStoreOperations);
    storeTransactionProcessingJobQueue_->SetAsyncJobs(true);
    storeTransactionProcessingJobQueue_->SetExtraTracing(FileStoreServiceConfig::GetConfig().EnableTxQueueTracing);

    auto root = this->CreateAsyncOperationRoot();
    auto operation = make_shared<RecoverAsyncOperation>(
        *this,
        [this] (AsyncOperationSPtr const & operation) { this->OnRecoveryCompleted(operation, false); },
        root);

    auto error = this->TransitionToRecoveryScheduling();
    if(!error.IsSuccess())
    {
        return error;
    }

    {
        AcquireWriteLock lock(recoveryOperationLock_);

        recoveryOperation_ = operation;
    }

    operation->Start(operation);

    this->OnRecoveryCompleted(operation, true);    

    return ErrorCodeValue::Success;
}

ErrorCode RequestManager::Close()
{
    auto error = this->TransitionToDeactivating();
    if(!error.IsSuccess())
    {
        return error;
    }

    error = Directory::Delete(this->localStagingLocation_, true, true);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Unable to delete StagingLocation during Close. Path:{0}, Error:{1}",
            this->localStagingLocation_,
            error);
    }

    if (fileOperationProcessingJobQueue_)
    {
        fileOperationProcessingJobQueue_->Close();
    }

    if(storeTransactionProcessingJobQueue_)
    {
        storeTransactionProcessingJobQueue_->Close();
    }

    return this->TransitionToDeactivated();
}

AsyncOperationSPtr RequestManager::BeginOnDataLoss(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode RequestManager::EndOnDataLoss(
    __in AsyncOperationSPtr const & operation,
    __out bool & isStateChanged)
{
    {
        AcquireReadLock lock(recoveryOperationLock_);

        if (recoveryOperation_)
        {
            recoveryOperation_->ExpectDataLoss();

            isStateChanged = true;
        }
        else
        {
            isStateChanged = false;
        }
    }

    return CompletedAsyncOperation::End(operation);
}

void RequestManager::OnAbort()
{
    AsyncOperationSPtr snap;
    {
        AcquireWriteLock lock(recoveryOperationLock_);

        snap = move(recoveryOperation_);
    }

    if (snap.get() != nullptr)
    {
        snap->Cancel();
    }

    if(Directory::Exists(this->localStagingLocation_))
    {
        auto error = Directory::Delete(this->localStagingLocation_, true, true);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Unable to delete StagingLocation during Abort. Path:{0}, Error:{1}",
                this->localStagingLocation_,
                error);
        }
    }    

    if (fileOperationProcessingJobQueue_)
    {
        fileOperationProcessingJobQueue_->Close();
    }

    if(storeTransactionProcessingJobQueue_)
    {
        storeTransactionProcessingJobQueue_->Close();
    }
}

void RequestManager::OnRecoveryCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    {
        AcquireWriteLock lock(recoveryOperationLock_);

        recoveryOperation_.reset();
    }
    
    auto error = RecoverAsyncOperation::End(operation);

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "End(RecoveryOperation): Error:{0}",
        error);

    if (error.IsSuccess())
    {
        error = this->TransitionToRecoveryCompleting();
        if(!error.IsSuccess())
        {
            return;
        }
        
        if(!Directory::Exists(localStagingLocation_))
        {
            error = Directory::Create2(this->localStagingLocation_);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "Unable to create StagingLocation directory after recovery. Path:{0}, Error:{1}",
                    this->localStagingLocation_,
                    error);
                this->TransitionToFailed();
                this->ReplicaObj.TransientFault(L"Failed to create staging location");
                return;
            }
        }

        error = this->TransitionToActive();
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            TraceId,
            "RequestManage transitioning to Active. Error:{0}",
            error);
    }
    else
    {
        error = this->TransitionToFailed();
        if(error.IsSuccess())
        {
            if(this->ReplicaObj.IsActivePrimary)
            {
                this->ReplicaObj.TransientFault(L"The RequestedManager has transitioned to failed state on an ActivePrimary");
            }
        }
    }
}

void RequestManager::ProcessRequest(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext)
{
    MoveUPtr<Message> messageMover(move(request));
    MoveUPtr<IpcReceiverContext> contextMover(move(receiverContext));

    auto jobItem = DefaultJobItem<RequestManager>(
        [messageMover, contextMover, this](RequestManager & ) mutable
        { 
            MessageUPtr message = messageMover.TakeUPtr();
            IpcReceiverContextUPtr context = contextMover.TakeUPtr();
            this->OnProcessRequest(move(message), move(context));
        });

    bool success = this->requestProcessingJobQueue_->Enqueue(move(jobItem));
    TESTASSERT_IFNOT(success, "JobItem should always enqueue since queue is never closed");    
}

void RequestManager::OnProcessRequest(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext)
{    
    wstring rejectReason;
    if(!ValidateClientMessage(request, rejectReason))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "ValidateClientMessage failed. Reason:{0}, MessageId:{1}",
            rejectReason,
            request->MessageId);

        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        return;
    }

    auto timeout = TimeoutHeader::FromMessage(*request).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;
    auto messageId = request->MessageId;
    auto action = request->Action;

    WriteInfo(
        TraceComponent,
        TraceId,
        "ProcessRequest: Action:{0}, ActivityId:{1}, MessageId:{2}, Timeout:{3}, RequestManagerState:{4}",
        action,
        activityId,
        messageId,
        timeout,
        this->GetState());

    if(this->GetState() != RequestManager::Active)
    {
        // Allow read operations from secondary. Reject the rest
        if (action != FileStoreServiceTcpMessage::InternalListAction &&
            action != FileStoreServiceTcpMessage::GetStoreLocationAction &&
            action != FileStoreServiceTcpMessage::GetStoreLocationsAction)
        {
            routingAgentProxy_.OnIpcFailure(ErrorCodeValue::FileStoreServiceNotReady, *receiverContext, activityId);
            return;
        }
    }

    if(action == FileStoreServiceTcpMessage::GetStagingLocationAction)
    {
        ShareLocationReply shareLocationReply(stagingShareLocation_);
        auto replyMessage = move(FileStoreServiceMessage::GetClientOperationSuccess(shareLocationReply, activityId));
        routingAgentProxy_.SendIpcReply(move(replyMessage), *receiverContext);
        return;
    }
    else if(action == FileStoreServiceTcpMessage::GetStoreLocationAction || action == FileStoreServiceTcpMessage::GetStoreLocationsAction)
    {
        ShareLocationReply shareLocationReply(storeShareLocation_);
        auto replyMessage = move(FileStoreServiceMessage::GetClientOperationSuccess(shareLocationReply, activityId));
        routingAgentProxy_.SendIpcReply(move(replyMessage), *receiverContext);
        return;
    }
    else if(action == FileStoreServiceTcpMessage::UploadAction)
    {
        UploadRequest uploadRequest;
        if(!request->GetBody<UploadRequest>(uploadRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            return;
        }

#if defined(PLATFORM_UNIX)
        wstring storeRelativePath = uploadRequest.StoreRelativePath;
        std::replace(storeRelativePath.begin(),storeRelativePath.end(), '\\', '/');
        uploadRequest.StoreRelativePath = storeRelativePath;
#endif

        AsyncOperation::CreateAndStart<ProcessUploadRequestAsyncOperation>(
            *this,
            move(uploadRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if (action == FileStoreServiceTcpMessage::CopyAction)
    {
        CopyRequest copyRequest;
        if (!request->GetBody<CopyRequest>(copyRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            return;
        }

        AsyncOperation::CreateAndStart<ProcessCopyRequestAsyncOperation>(
            *this,
            move(copyRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if (action == FileStoreServiceTcpMessage::CheckExistenceAction)
    {
        ImageStoreBaseRequest checkExistenceRequest;
        if (!request->GetBody<ImageStoreBaseRequest>(checkExistenceRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing CheckExistenceRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessCheckExistenceRequestAsyncOperation>(
            *this,
            move(checkExistenceRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::ListAction)
    {
        ListRequest listRequest;
        if(!request->GetBody<ListRequest>(listRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            return;
        }

        AsyncOperation::CreateAndStart<ProcessListRequestAsyncOperation>(
            *this,
            move(listRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::InternalListAction)
    {
        ListRequest listRequest;
        if(!request->GetBody<ListRequest>(listRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            return;
        }

        AsyncOperation::CreateAndStart<ProcessInternalListRequestAsyncOperation>(
            *this,
            move(listRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::DeleteAction)
    {
        ImageStoreBaseRequest deleteRequest;
        if(!request->GetBody<ImageStoreBaseRequest>(deleteRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            return;
        }

        AsyncOperation::CreateAndStart<ProcessDeleteRequestAsyncOperation>(
            *this,
            move(deleteRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if (action == FileStoreServiceTcpMessage::ListUploadSessionAction)
    {
        UploadSessionRequest uploadSessionRequest;
        if (!request->GetBody<UploadSessionRequest>(uploadSessionRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing UploadSessionRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessListUploadSessionRequestAsyncOperation>(
            *this,
            move(uploadSessionRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::DeleteUploadSessionAction)
    {
        DeleteUploadSessionRequest deleteUploadSessionRequest;
        if (!request->GetBody<DeleteUploadSessionRequest>(deleteUploadSessionRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing DeleteUploadSessionRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessDeleteUploadSessionRequestAsyncOperation>(
            *this,
            move(deleteUploadSessionRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::CreateUploadSessionAction)
    {
        CreateUploadSessionRequest createUploadSessionRequest;
        if (!request->GetBody<CreateUploadSessionRequest>(createUploadSessionRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing CreateUploadSessionRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessCreateUploadSessionRequestAsyncOperation>(
            *this,
            move(createUploadSessionRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::UploadChunkAction)
    {
        UploadChunkRequest uploadChunkRequest;
        if (!request->GetBody<UploadChunkRequest>(uploadChunkRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing UploadChunkRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessUploadChunkRequestAsyncOperation>(
            *this,
            move(uploadChunkRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else if(action == FileStoreServiceTcpMessage::CommitUploadSessionAction)
    {
        UploadSessionRequest uploadSessionRequest;
        if (!request->GetBody<UploadSessionRequest>(uploadSessionRequest))
        {
            auto error = ErrorCode::FromNtStatus(request->Status);
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceComponent,
                    TraceId,
                    "Parsing UploadSessionRequest failed. Error:{0}, MessageId:{1}",
                    error,
                    request->MessageId);

                return;
            }
        }

        AsyncOperation::CreateAndStart<ProcessCommitUploadSessionRequestAsyncOperation>(
            *this,
            move(uploadSessionRequest),
            move(receiverContext),
            activityId,
            timeout,
            [this](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(asyncOperation);
        },
            this->CreateAsyncOperationRoot());
    }
    else
    {
        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidOperation, *receiverContext, activityId);
    }
}

void RequestManager::OnProcessRequestComplete(AsyncOperationSPtr const & operation)
{
    IpcReceiverContextUPtr receiverContext;
    MessageUPtr reply;
    ActivityId activityId;
    auto error = ProcessRequestAsyncOperation::End(operation, receiverContext, reply, activityId);
    if(error.IsSuccess())
    {
        ASSERT_IFNOT(reply, "Reply should be set if the request is processed successfully");
        routingAgentProxy_.SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxy_.OnIpcFailure(error, *receiverContext, activityId);
    }
}

bool RequestManager::ValidateClientMessage(__in Transport::MessageUPtr & message, __out wstring & rejectReason)
{
    bool success = true;

    if (success)
    {
        FabricActivityHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *Constants::ErrorReason::MissingActivityHeader;
        }
    }

    if (success)
    {
        TimeoutHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *Constants::ErrorReason::MissingTimeoutHeader;
        }
    }

    if (success)
    {
        success = (message->Actor == Transport::Actor::FileStoreService);

        if (!success)
        {
            StringWriter(rejectReason).Write("{0}: {1}", Constants::ErrorReason::IncorrectActor, message->Actor);
        }
    }

    return success;
}

StoreFileVersion RequestManager::GetNextFileVersion()
{
    return StoreFileVersion(epochDataLossNumber_, epochConfigurationNumber_, ++versionNumber_);
}
