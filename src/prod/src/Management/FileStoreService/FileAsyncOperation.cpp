// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("FileAsyncOperation");
StringLiteral const RetryTimerTag("FileAsyncOperation.RetryTimer");

FileAsyncOperation::FileAsyncOperation(
    __in RequestManager & requestManager,
    bool useSimpleTx,
    wstring const & storeRelativePath,    
    bool useTwoPhaseCommit,
    Common::ActivityId const & activityId,    
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    ReplicaActivityTraceComponent(requestManager.ReplicaObj.PartitionedReplicaId, activityId),
    requestManager_(requestManager),
    useSimpleTx_(useSimpleTx),
    storeRelativePath_(storeRelativePath),    
    useTwoPhaseCommit_(useTwoPhaseCommit),
    timeoutHelper_(timeout)
{
}

FileAsyncOperation::~FileAsyncOperation()
{
}

ErrorCode FileAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<FileAsyncOperation>(operation)->Error;
}

void FileAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr) 
{   
    this->StartTransitionToIntermediateState(thisSPtr);
}

void FileAsyncOperation::OnCompleted()
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

    AsyncOperation::OnCompleted();
}

void FileAsyncOperation::StartTransitionToIntermediateState(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent,
        TraceId,
        "Queueing StoreTransaction for TransitionToIntermediateState. StoreRelativePath:{0}",
        storeRelativePath_);

    auto intermediateStateOperation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
        requestManager_,
        useSimpleTx_,
        [this](StoreTransaction const & storeTx) 
        { 
            WriteInfo(
                TraceComponent,
                TraceId,
                "Starting StoreTransaction for TransitionToIntermediateState. StoreRelativePath:{0}",
                storeRelativePath_);

            return this->TransitionToIntermediateState(storeTx); 
        },
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation) { this->OnCommitToIntermediateStateCompleted(operation, false); },
        thisSPtr);
    this->OnCommitToIntermediateStateCompleted(intermediateStateOperation, true);
}

void FileAsyncOperation::OnCommitToIntermediateStateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = StoreTransactionAsyncOperation::End(operation);
    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "Ending StoreTransaction for TransitionToIntermediateState. StoreRelativePath:{0}, Error:{1}",
        storeRelativePath_,
        error);
    
    if (error.IsSuccess())
    {
        auto fileOperation = this->OnBeginFileOperation(
            [this](AsyncOperationSPtr const & operation) { this->OnFileOperationCompleted(operation, false); }, 
            operation->Parent);
        this->OnFileOperationCompleted(fileOperation, true);
    }
    else
    {
        this->TryComplete(operation->Parent, move(error));
    }
}

void FileAsyncOperation::OnFileOperationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->OnEndFileOperation(operation);

    if (error.IsSuccess())
    {
        this->StartTransitionToReplicatingState(operation->Parent);
    }
    else
    {
        this->Rollback(operation->Parent, error);
    }
}

void FileAsyncOperation::StartTransitionToReplicatingState(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent,
        TraceId,
        "Queueing StoreTransaction for TransitionToReplicatingState. StoreRelativePath:{0}",
        storeRelativePath_);

    auto finalStateOperation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
        requestManager_,
        useSimpleTx_,
        [this](StoreTransaction const & storeTx) 
        { 
            WriteInfo(
                TraceComponent,
                TraceId,
                "Starting StoreTransaction for TransitionToReplicatingState. StoreRelativePath:{0}",
                storeRelativePath_);

            return this->TransitionToReplicatingState(storeTx); 
        },
        this->ActivityId,
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation) { this->OnCommitToReplicatingStateCompleted(operation, false); },
        thisSPtr);
    this->OnCommitToReplicatingStateCompleted(finalStateOperation, true);
}

void FileAsyncOperation::OnCommitToReplicatingStateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto const & thisSPtr = operation->Parent;

    auto error = StoreTransactionAsyncOperation::End(operation);
    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "Ending StoreTransaction for TransitionToReplicatingState. StoreRelativePath:{0}, Error:{1}",
        storeRelativePath_,
        error);

    if (error.IsSuccess())
    {
        this->StartTransitionToCommittedState(thisSPtr);
    }
    else
    {
        this->Rollback(thisSPtr, error);
    }
}

void FileAsyncOperation::StartTransitionToCommittedState(AsyncOperationSPtr const & thisSPtr)
{
    if (!useTwoPhaseCommit_)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "skipping two-phase commit: StoreRelativePath:{0}",
            storeRelativePath_);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Queueing StoreTransaction for TransitionToCommittedState. StoreRelativePath:{0}",
            storeRelativePath_);

        auto operation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
            requestManager_,
            useSimpleTx_,
            [this](StoreTransaction const & storeTx) 
            { 
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "Starting StoreTransaction for TransitionToCommittedState. StoreRelativePath:{0}",
                    storeRelativePath_);

                return this->TransitionToCommittedState(storeTx); 
            },
            this->ActivityId,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCommitToCommittedStateCompleted(operation, false); },
            thisSPtr);
        this->OnCommitToCommittedStateCompleted(operation, true);
    }
}

void FileAsyncOperation::OnCommitToCommittedStateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = StoreTransactionAsyncOperation::End(operation);

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "Ending StoreTransaction for TransitionToCommittedState. StoreRelativePath:{0}, Error:{1}",
        storeRelativePath_,
        error);

    if (error.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(error));
    }
    else
    {
        this->Rollback(thisSPtr, move(error));
    }
}

void FileAsyncOperation::Rollback(AsyncOperationSPtr const & thisSPtr, ErrorCode const & originalError)
{    
    WriteWarning(
        TraceComponent,
        TraceId,
        "Performing rollback due to Error:{0}, StoreRelativePath:{1}",
        originalError,
        storeRelativePath_);

    WriteInfo(
        TraceComponent,
        TraceId,
        "Starting StoreTransaction for TransitionToRolledbackState. StoreRelativePath:{0}",
        storeRelativePath_);
    auto finalStateOperation = AsyncOperation::CreateAndStart<StoreTransactionAsyncOperation>(
        requestManager_,
        useSimpleTx_,
        [this] (StoreTransaction const & storeTx) { return this->TransitionToRolledbackState(storeTx); },
        this->ActivityId,
        TimeSpan::MaxValue,
        [this, originalError] (AsyncOperationSPtr const & operation) { this->OnCommitToRolledbackStateCompleted(operation, false, originalError); },
        thisSPtr);
    this->OnCommitToRolledbackStateCompleted(finalStateOperation, true, originalError);
}

void FileAsyncOperation::OnCommitToRolledbackStateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously, ErrorCode const & originalError)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = StoreTransactionAsyncOperation::End(operation);

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        TraceId,
        "Ending StoreTransaction for TransitionToRolledbackState. StoreRelativePath:{0}, Error:{1}",
        storeRelativePath_,
        error);

    if(error.IsSuccess())
    {
        // Undo file operation only if rollback is successful
        this->UndoFileOperation();

        // If the operation rolled back, from client's perspective
        // the operations has failed
        error.Overwrite(originalError);
    }

    TryComplete(operation->Parent, move(error));
}    

ErrorCode FileAsyncOperation::TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, function<void(AsyncOperationSPtr const &)> const & callback)
{
    TimeSpan delay = FileStoreServiceConfig::GetConfig().FileOperationBackoffInterval;
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

bool FileAsyncOperation::IsSharingViolationError(Common::ErrorCode const& error)
{
    if (error.IsWin32Error(ERROR_SHARING_VIOLATION) || error.IsError(ErrorCodeValue::SharingAccessLockViolation))
    {
        return true;
    }

    return false;
}
