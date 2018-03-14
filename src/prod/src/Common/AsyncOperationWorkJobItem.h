// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Used by job item to execute work
    using BeginAsyncWorkCallback = std::function<AsyncOperationSPtr(AsyncCallback const & callback)>;
    using EndAsyncWorkCallback = std::function<void(AsyncOperationSPtr const & operation)>;
    using OnStartWorkTimedOutCallback = std::function<void()>;

    // Job item used to execute async work done through async operations.
    // The job items are enqueued in AsyncWorkJobItem.
    // When a thread is available, it starts begin callback.
    // If the callback completes on the same thread, the work is done.
    // If it completes asynchronously, on a different thread,
    // the job item lets the job queue know so a new thread can be scheduled to complete work.
    class AsyncOperationWorkJobItem : public AsyncWorkJobItem
    {
        DENY_COPY(AsyncOperationWorkJobItem)
    public:
        AsyncOperationWorkJobItem();
        AsyncOperationWorkJobItem(
            BeginAsyncWorkCallback const & beginCallback,
            EndAsyncWorkCallback const & endCallback);

        AsyncOperationWorkJobItem(
            BeginAsyncWorkCallback const & beginCallback,
            EndAsyncWorkCallback const & endCallback,
            OnStartWorkTimedOutCallback const & startTimeoutCallback);

        virtual ~AsyncOperationWorkJobItem();

        AsyncOperationWorkJobItem(AsyncOperationWorkJobItem && other);
        AsyncOperationWorkJobItem & operator=(AsyncOperationWorkJobItem && other);

    protected:
        virtual void StartWork(
            AsyncWorkReadyToCompleteCallback const & completeCallback,
            __out bool & completedSync) override;
        virtual void EndWork() override;
        virtual void OnStartWorkTimedOut() override;

    private:
        BeginAsyncWorkCallback beginCallback_;
        EndAsyncWorkCallback endCallback_;
        OnStartWorkTimedOutCallback startTimeoutCallback_;
        AsyncOperationSPtr operation_;
    };
}
