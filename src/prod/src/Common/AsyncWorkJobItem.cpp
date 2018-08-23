// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

uint64 AsyncWorkJobItem::GlobalSequenceNumber = 0;

AsyncWorkJobItem::AsyncWorkJobItem()
    : state_(AsyncWorkJobItemState::NotStarted)
    , sequenceNumber_(InterlockedIncrement64((volatile LONGLONG*) &GlobalSequenceNumber))
    , enqueuedTime_(Stopwatch::Now())
    , workTimeout_(TimeSpan::MaxValue)
    , jobQueueCallback_()
    , beginWorkDone_(make_unique<AsyncManualResetEvent>())
{
}

AsyncWorkJobItem::AsyncWorkJobItem(TimeSpan const workTimeout)
    : state_(AsyncWorkJobItemState::NotStarted)
    , sequenceNumber_(InterlockedIncrement64((volatile LONGLONG*) &GlobalSequenceNumber))
    , enqueuedTime_(Stopwatch::Now())
    , workTimeout_(workTimeout)
    , jobQueueCallback_()
    , beginWorkDone_(make_unique<AsyncManualResetEvent>())
{
}


AsyncWorkJobItem::~AsyncWorkJobItem()
{
}

AsyncWorkJobItem::AsyncWorkJobItem(AsyncWorkJobItem && other)
    : state_(move(other.state_))
    , sequenceNumber_(move(other.sequenceNumber_))
    , enqueuedTime_(move(other.enqueuedTime_))
    , workTimeout_(move(other.workTimeout_))
    , jobQueueCallback_(move(other.jobQueueCallback_))
    , beginWorkDone_(move(other.beginWorkDone_))
{
}

AsyncWorkJobItem & AsyncWorkJobItem::operator=(AsyncWorkJobItem && other)
{
    if (this != &other)
    {
        state_ = move(other.state_);
        sequenceNumber_ = move(other.sequenceNumber_);
        enqueuedTime_ = move(other.enqueuedTime_);
        workTimeout_ = move(other.workTimeout_);
        jobQueueCallback_ = move(other.jobQueueCallback_);
        beginWorkDone_ = move(other.beginWorkDone_);
    }

    return *this;
}

TimeSpan AsyncWorkJobItem::get_RemainingTime() const
{
    if (workTimeout_ == TimeSpan::MaxValue)
    {
        return TimeSpan::MaxValue;
    }

    auto remaining = enqueuedTime_ + workTimeout_ - Stopwatch::Now();
    if (remaining <= TimeSpan::Zero)
    {
        return TimeSpan::Zero;
    }
    else
    {
        return remaining;
    }
}

AsyncWorkJobItemState::Enum AsyncWorkJobItem::ProcessJob(
    AsyncWorkReadyToCompleteCallback const & completeCallback)
{
    switch (state_)
    {
    case AsyncWorkJobItemState::NotStarted:
    {
        ASSERT_IFNOT(completeCallback, "completeCallback not set in ProcessJob->StartWork, sequence number {0}", sequenceNumber_);
        if (this->RemainingTime <= TimeSpan::Zero)
        {
            // The time allowed for executing work item has passed, so drop the work
            this->OnStartWorkTimedOut();
            state_ = AsyncWorkJobItemState::CompletedSync;
            return state_;
        }
        
        // To prevent races, the callback must ensure that the begin work is called.
        // The state may complete async (eg CompletedSynchronously is called at a time when the operation didn't yet complete),
        // but the callback is quickly executed on the same thread.
        // In this case, if we don't wait for BeginWork to finish, the async operation callback
        // into the job queue may race with changing the state.
        // The effect would be that the job queue tries to complete work when Start hasn't yet completed;
        // Race can be that state is not correctly set in EndWork
        // or EndWork completes and the job item is destructed before BeginWork is completed,
        // which would lead to races.
        //

        jobQueueCallback_ = completeCallback;
        bool completedSync;

        this->StartWork(
            [this](uint64 sequenceNumber) { this->OnInnerAsyncWorkReadyToComplete(sequenceNumber); },
            /*out*/completedSync);
        if (completedSync)
        {
            // Call end work and the job item is completed. No more work is needed, job item will be removed from the job queue.
            this->EndWork();
            state_ = AsyncWorkJobItemState::CompletedSync;
            return state_;
        }
        else
        {
            // Work will complete async.
            // Work is already started, it may complete on a different thread very quickly.
            // As soon as beginWorkDone is set, the job item can be processed by the job queue in parallel
            // with the thread that called this method.
            // When done, the job item is destructed, which means it can be destructed before this method completes.
            // We can't return a private variable, since the job item can be destructed.
            state_ = AsyncWorkJobItemState::AsyncPending;
            auto cachedState = state_;
            beginWorkDone_->Set();
            return cachedState;
        }
    
        break;
    }
    case AsyncWorkJobItemState::AsyncPending:
        ASSERT_IF(completeCallback, "completeCallback should not set in ProcessJob->StartWork with AsyncPending state, sequence number {0}", sequenceNumber_);
        // Call end work, which always completes sync
        this->EndWork();
        state_ = AsyncWorkJobItemState::CompletedAsync;
        return state_;

    default:
        Assert::CodingError("AsyncWorkJobItem::Process: invalid state {0}, sequence number {1}", state_, sequenceNumber_);
    }
}

void AsyncWorkJobItem::OnInnerAsyncWorkReadyToComplete(
    uint64 sequenceNumber)
{
    // First wait for BeginWork to complete to prevent races.
    // The job queue callback holds the job queue alive, which holds the job item
    // So there is no need to pass extra root to the async operation.
    auto operation = beginWorkDone_->BeginWaitOne(
        TimeSpan::MaxValue,
        [sequenceNumber, this](AsyncOperationSPtr const & operation)
        {
            this->OnBeginWaitCompleted(operation, sequenceNumber, false);
        },
        AsyncOperationSPtr());
    this->OnBeginWaitCompleted(operation, sequenceNumber, true);
}

void AsyncWorkJobItem::OnBeginWaitCompleted(
    AsyncOperationSPtr const & operation,
    uint64 sequenceNumber,
    bool completedSynchronously)
{
    if (operation->CompletedSynchronously != completedSynchronously) { return; }

    auto error = beginWorkDone_->EndWaitOne(operation);
    ASSERT_IFNOT(error.IsSuccess(), "OnBeginWaitCompleted: beginWorkDone is not signaled in time, job queue sequence number {0}, state {1}", this->SequenceNumber, this->State);

    // Let the job queue know that the async work is ready to complete
    ASSERT_IFNOT(jobQueueCallback_, "OnBeginWaitCompleted: job queue callback not set, job queue sequence number {0}, state {1}", this->SequenceNumber, this->State);
    auto callback = move(jobQueueCallback_);
    // After callback is executed, no access to local variable should be done, as the job item can be destructed
    // The job queue guarantees to keep it alive until the callback completes.
    callback(sequenceNumber);
}
