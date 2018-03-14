// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Store;
using namespace Api;
using namespace Management::FileStoreService;

StringLiteral const TraceComponent("StoreTransactionAsyncOperation");

StoreTransactionAsyncOperation::StoreTransactionAsyncOperation(    
    RequestManager & requestManager,
    bool const useSimpleTx,
    StoreOperation const & storeOperation,
    Common::ActivityId const & activityId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    ReplicaActivityTraceComponent(requestManager.ReplicaObj.PartitionedReplicaId, activityId),    
    requestManager_(requestManager),
    useSimpleTx_(useSimpleTx),
    storeOperation_(storeOperation),
    timeoutHelper_(timeout),
    failureCount_(0),    
    retryTimer_(),
    lock_(),
    canceled_(false),
    initialEnqueueSucceeded_(false)
{
}

ErrorCode StoreTransactionAsyncOperation::End(Common::AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<StoreTransactionAsyncOperation>(operation)->Error;
}

void StoreTransactionAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{    
    auto jobItem = make_unique<DefaultTimedJobItem<RequestManager>>(
        timeoutHelper_.GetRemainingTime(),
        [thisSPtr, this](RequestManager &)
    {
        initialEnqueueSucceeded_ = true;
        this->CreateStoreTransaction(thisSPtr);
    },
        [thisSPtr, this](RequestManager &)
    {
        initialEnqueueSucceeded_ = true;
        WriteWarning(
            TraceComponent,
            TraceId,
            "Operation timed out before job queue item was processed.");

        this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
    });

    if(!this->requestManager_.storeTransactionProcessingJobQueue_->Enqueue(move(jobItem)))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Could not enqueue operation to JobQueue.");
        this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
    }
}

void StoreTransactionAsyncOperation::OnCompleted()
{
    if(initialEnqueueSucceeded_)
    {
        requestManager_.storeTransactionProcessingJobQueue_->CompleteAsyncJob();
    }
}

void StoreTransactionAsyncOperation::CreateStoreTransaction(Common::AsyncOperationSPtr const & thisSPtr)
{
    StoreTransactionUPtr storeTx;
    ErrorCode error;
    if(useSimpleTx_)
    {
        error = requestManager_.ReplicaStoreWrapperUPtr->CreateSimpleTransaction(this->ActivityId, storeTx);
    }
    else
    {
        error = requestManager_.ReplicaStoreWrapperUPtr->CreateTransaction(this->ActivityId, storeTx);
    }

    if(!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    this->StartStoreTransaction(move(storeTx), thisSPtr);
}

void StoreTransactionAsyncOperation::StartStoreTransaction(StoreTransactionUPtr && storeTx, AsyncOperationSPtr const & thisSPtr)
{                
    auto error = storeOperation_(*storeTx);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "StoreOperation: Error:{0}",
        error);

    if(!error.IsSuccess())
    {
        storeTx->Rollback();
        
        if(IsRetryableError(error))
        {
            OnRetriableError(thisSPtr);
            return;
        }        

        TryComplete(thisSPtr, error);
        return;
    }

    // During ReplicatedStore transaction commit use MaxTimeout
    // This is done because it is possible that Commit timesout even though
    // the operation is successfully replicated. This will cause issue when
    // the operation is transition into Intermediate state since on Timeout
    // we will fail the operation and leave the file in intermediate state.
    auto operation = StoreTransaction::BeginCommit(
        move(*storeTx),
        TimeSpan::MaxValue, 
        [this](AsyncOperationSPtr const & operation) { this->OnCommitCompleted(operation, false); },
        thisSPtr);
    this->OnCommitCompleted(operation, true);
}

void StoreTransactionAsyncOperation::OnCommitCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = StoreTransaction::EndCommit(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceComponent,
        TraceId,
        "EndCommit: Error:{0}",
        error);    

    AsyncOperationSPtr thisSPtr = operation->Parent;
    if(!error.IsSuccess() && IsRetryableError(error))
    {
        OnRetriableError(thisSPtr);
        return;
    }

    TryComplete(thisSPtr, error);    
}

void StoreTransactionAsyncOperation::OnRetriableError(AsyncOperationSPtr const & thisSPtr)
{
    if(timeoutHelper_.IsExpired)
    {
        TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        return;
    }

    {
        AcquireExclusiveLock lock(this->lock_);
        if (canceled_) { return; }

        ++failureCount_;
        retryTimer_ = Timer::Create(
            TimerTagDefault,
            [this, thisSPtr](TimerSPtr const & timer)
        {
            timer->Cancel();

            // On failure, retry is scheduled through JobQueue
            auto jobItem = make_unique<DefaultTimedJobItem<RequestManager>>(
                timeoutHelper_.GetRemainingTime(),
                [thisSPtr, this](RequestManager &)
            {
                this->CreateStoreTransaction(thisSPtr);
            },
                [thisSPtr, this](RequestManager &)
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "Operation timedout before job queue item was processed.");
                this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
            });

            if(!this->requestManager_.storeTransactionProcessingJobQueue_->Enqueue(move(jobItem)))
            {
                WriteWarning(
                    TraceComponent,
                    TraceId,
                    "Could not enqueue retry operation to JobQueue.");
                this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
            }
            else
            {
                // If retry has been successfuly added to the queue, complete the async job for
                // previous store transaction
                requestManager_.storeTransactionProcessingJobQueue_->CompleteAsyncJob();
            }
        },
            false);
    }

    retryTimer_->Change(this->GetDueTime());
}

void StoreTransactionAsyncOperation::OnCancel()
{
    TimerSPtr timer;
    {
        AcquireExclusiveLock lock(lock_); 
        timer = retryTimer_;
        canceled_ = true;
    }

    if (timer)
    {
        timer->Cancel();
    }

    AsyncOperation::OnCancel();
}

bool StoreTransactionAsyncOperation::IsRetryableError(ErrorCode const & error)
{
    switch(error.ReadValue())
    {            
    case ErrorCodeValue::NoWriteQuorum:    
    case ErrorCodeValue::StoreWriteConflict:
    case ErrorCodeValue::StoreSequenceNumberCheckFailed:
    case ErrorCodeValue::REQueueFull:
    case ErrorCodeValue::ReconfigurationPending:
    case ErrorCodeValue::StoreUnexpectedError:
    case ErrorCodeValue::StoreOperationCanceled:

        // Retrying since store returns NotPrimary even when Reconfiguration is pending
        // and replica is still primary. There are cases, when the replica gets NotPrimary
        // but gets write status soon after without any change role call. On retry, we will
        // call TryGetTransaction on RequestManager which will return false if we ChangeRole
        // out of primary. Bug# 3377036 is tracking this issue on RA.
    case ErrorCodeValue::NotPrimary:
        return true;

    default:
        return false;
    }    
}

TimeSpan StoreTransactionAsyncOperation::GetDueTime()
{
    TimeSpan dueTime = TimeSpan::Zero;

    if(failureCount_ != 0)
    {
        int64 retryTicks = (int64) (FileStoreServiceConfig::GetConfig().StoreTransactionRetryInterval.Ticks * failureCount_);
        if(retryTicks > FileStoreServiceConfig::GetConfig().StoreTransactionMaxRetryInterval.Ticks)
        {
            retryTicks = FileStoreServiceConfig::GetConfig().StoreTransactionMaxRetryInterval.Ticks;
        }

        if(TimeSpan::FromTicks(retryTicks) > timeoutHelper_.GetRemainingTime())
        {
            retryTicks = timeoutHelper_.GetRemainingTime().Ticks;
        }

        dueTime = TimeSpan::FromTicks(retryTicks);
    }
        
    return dueTime;
}
