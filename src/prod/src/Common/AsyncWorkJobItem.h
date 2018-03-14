// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Called by job item to let the caller know that the async work is ready to complete and EndWork must be scheduled
    using AsyncWorkReadyToCompleteCallback = std::function<void(uint64 sequenceNumber)>;

    // Job item used to execute async work done through the asyn job queue, AsyncWorkJobItem.
    // When a thread is available, it starts work.
    // If work completes on the same thread, the job item is done sync.
    // If it completes asynchronously, on a different thread,
    // the job item lets the job queue know so a new thread can be scheduled to complete work.
    class AsyncWorkJobItem
    {
        DENY_COPY(AsyncWorkJobItem)
    
    public:
        AsyncWorkJobItem();
        explicit AsyncWorkJobItem(TimeSpan const workTimeout);
        virtual ~AsyncWorkJobItem();

        AsyncWorkJobItem(AsyncWorkJobItem && other);
        AsyncWorkJobItem & operator=(AsyncWorkJobItem && other);

        __declspec(property(get = get_SequenceNumber)) uint64 SequenceNumber;
        uint64 get_SequenceNumber() const { return sequenceNumber_; }

        __declspec(property(get = get_State)) AsyncWorkJobItemState::Enum State;
        AsyncWorkJobItemState::Enum get_State() const { return state_; }

        __declspec(property(get = get_EnqueuedTime)) StopwatchTime const & EnqueuedTime;
        StopwatchTime const & get_EnqueuedTime() const { return enqueuedTime_; }

        __declspec(property(get = get_RemainingTime)) TimeSpan RemainingTime;
        TimeSpan get_RemainingTime() const;

        __declspec(property(get = get_WorkTimeout)) TimeSpan const & WorkTimeout;
        TimeSpan const & get_WorkTimeout() const { return workTimeout_; }

        // Called by job queue to execute work per state
        AsyncWorkJobItemState::Enum ProcessJob(
            AsyncWorkReadyToCompleteCallback const & completeCallback);

        // Called by job queue when enqueue failed
        virtual void OnEnqueueFailed(ErrorCode &&) {}

    protected:
        virtual void StartWork(
            AsyncWorkReadyToCompleteCallback const & completeCallback,
            __out bool & completedSync) = 0;
        virtual void EndWork() = 0;
        virtual void OnStartWorkTimedOut() = 0;
                
        // Called by async work callback when ready to complete.
        // It will call into the job queue to let it know the item completed.
        void OnInnerAsyncWorkReadyToComplete(uint64 sequenceNumber);

    private:
        void OnBeginWaitCompleted(
            AsyncOperationSPtr const & operation,
            uint64 sequenceNumber,
            bool completedSynchronously);

        static uint64 GlobalSequenceNumber;

        AsyncWorkJobItemState::Enum state_;
        uint64 sequenceNumber_;
        StopwatchTime enqueuedTime_;
        TimeSpan workTimeout_;
        AsyncWorkReadyToCompleteCallback jobQueueCallback_;
        std::unique_ptr<AsyncManualResetEvent> beginWorkDone_;
    };

    using AsyncWorkJobItemUPtr = std::unique_ptr<AsyncWorkJobItem>;
}
