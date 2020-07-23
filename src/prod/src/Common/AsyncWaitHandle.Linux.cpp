// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/eventfd.h>

using namespace std;
using namespace Common;

namespace
{
    StringLiteral const TraceType = "AsyncWaitHandle";
}

template <bool ManualReset> 
AsyncWaitHandle<ManualReset>::WaitAsyncOperation::WaitAsyncOperation(
    AsyncWaitHandle<ManualReset> & owner,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , owner_(owner)
    , waiterQueueWPtr_(owner_.waiters_)
{
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::WaitAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr) 
{
    auto err = owner_.TryAddWaitOperation(thisSPtr);
    if (!err.IsSuccess())
    {
        TryComplete(thisSPtr);
        return;
    }

    TimedAsyncOperation::OnStart(thisSPtr);
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::WaitAsyncOperation::OnTimeout(AsyncOperationSPtr const & thisSPtr) 
{
    LeaveWaiterQueue(thisSPtr);
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::WaitAsyncOperation::OnCancel()
{
    LeaveWaiterQueue(shared_from_this());
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::WaitAsyncOperation::LeaveWaiterQueue(AsyncOperationSPtr const & thisSPtr)
{
    if (auto waiterQueue = waiterQueueWPtr_.lock())
    {
        waiterQueue->Erase(thisSPtr);
    }
}

template <bool ManualReset> 
void AsyncWaitHandle<ManualReset>::WaiterQueue::Enqueue(AsyncOperationSPtr const & wop)
{
    AcquireWriteLock grab(lock_);

    queue_.emplace_back(wop);
}

template <bool ManualReset> 
void AsyncWaitHandle<ManualReset>::WaiterQueue::Erase(AsyncOperationSPtr const & wop)
{
    AcquireWriteLock grab(lock_);

    for (auto iter = queue_.cbegin(); iter != queue_.cend(); ++iter)
    {
        if (*iter == wop)
        {
            queue_.erase(iter);
            return;
        }
    }
}

template <bool ManualReset> 
AsyncOperationSPtr AsyncWaitHandle<ManualReset>::WaiterQueue::PopFront()
{
    AcquireWriteLock grab(lock_);

    AsyncOperationSPtr front;
    if (queue_.empty()) return front;

    front = move(queue_.front());
    queue_.pop_front();
    return front;
}
 
template <bool ManualReset> 
deque<AsyncOperationSPtr> AsyncWaitHandle<ManualReset>::WaiterQueue::PopAll()
{
    AcquireWriteLock grab(lock_);

    auto queue = move(queue_);
    return queue;
}

template <bool ManualReset> 
AsyncWaitHandle<ManualReset>::AsyncWaitHandle(bool initialState, wstring const &eventName)
    : WaitHandle<ManualReset>(initialState, eventName)
    , evtLoop_(&(EventLoopPool::GetDefault()->Assign()))
    , waiters_(make_shared<WaiterQueue>())
{
    eventFd_ = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
    Invariant2(eventFd_ > 0, "eventfd failed: errno = {0}", errno);
    __super::WriteNoise(TraceType, "{0}: ctor: eventFd_ = {1:x}", TextTracePtrAs(this, WaitHandle<ManualReset>), eventFd_);
}

template <bool ManualReset> 
AsyncWaitHandle<ManualReset>::~AsyncWaitHandle()
{
    __super::Close();
    __super::WriteNoise(TraceType, "{0}: leaving dtor", TextTracePtrAs(this, WaitHandle<ManualReset>));
}
 
template <bool ManualReset> 
ErrorCode AsyncWaitHandle<ManualReset>::TryAddWaitOperation(AsyncOperationSPtr const & wop)
{
    deque<AsyncOperationSPtr> opsToComplete;
    ErrorCode err;
    {
        auto* mutex = &(__super::mutex_);
        pthread_mutex_lock(mutex);
        KFinally([mutex] { pthread_mutex_unlock(mutex); });

        waiters_->Enqueue(wop);

        opsToComplete = TryStartComplete_MutexHeld();

        if (!__super::closed_)
        {
            if (!fdCtx_)
            {
                fdCtx_ = evtLoop_->RegisterFd(
                            eventFd_,
                            EPOLLIN,
                            true, 
                            [this] (int fd, uint evts) { EventFdReadCallback(fd, evts); });
            }

            err = evtLoop_->Activate(fdCtx_);
        }
    }

    FinishComplete(opsToComplete);
    
    return err;
}
 
template <bool ManualReset> 
void AsyncWaitHandle<ManualReset>::EventFdReadCallback(int fd, uint evts)
{
    __super::WriteNoise(TraceType, "{0}: enter EventFdReadCallback, fd = {1:x}, evts = {2:x}", TextTracePtrAs(this, WaitHandle<ManualReset>), fd, evts);

    deque<AsyncOperationSPtr> opsToComplete;
    {
        auto* mutex = &(__super::mutex_);
        pthread_mutex_lock(mutex);
        KFinally([mutex] { pthread_mutex_unlock(mutex); });

        if (!__super::closed_)
        {
            Invariant2(fd == eventFd_, "{0}: eventFd_ = {1:x}, fd = {2:x}, should be the same", TextTracePtrAs(this, WaitHandle<ManualReset>), eventFd_, fd);

            if ((evts & EPOLLIN) == 0)
            {
                __super::WriteInfo(
                    TraceType,
                    "events {0:x} reported on {1:x}, EPOLLHUP={2},EPOLLERR={3}, EPOLLIN is not set",
                    evts,
                    eventFd_,
                    bool(evts & EPOLLHUP),
                    bool(evts & EPOLLERR));
            }

            uint64 v = 0;
            auto retval = read(eventFd_, &v, sizeof(v));
            if (retval < 0)
            {
                auto err = errno;
                __super::WriteInfo(TraceType, "WaitAsyncOperation: read failed: {0}", err);
            }

        }

        opsToComplete = TryStartComplete_MutexHeld();

        if (!__super::closed_)
        {
            auto err = evtLoop_->Activate(fdCtx_);
            if (!err.IsSuccess())
            {
                __super::WriteInfo(TraceType, "WaitAsyncOperation: Activate failed: {0}", err);
            }
        }
    }

    Threadpool::Post([opsToComplete = move(opsToComplete), this] { FinishComplete(opsToComplete); } );

    __super::WriteNoise(TraceType, "{0}: leave EventFdReadCallback, eventFd_ = {1:x}", TextTracePtrAs(this, WaitHandle<ManualReset>), eventFd_);
}

template <bool ManualReset> 
deque<AsyncOperationSPtr> AsyncWaitHandle<ManualReset>::TryStartComplete_MutexHeld()
{
    deque<AsyncOperationSPtr> ops;

    if (!__super::signaled_) return ops;

    if (ManualReset)
    {
        ops = waiters_->PopAll();
        for(auto const & op : ops)
        {
            op->TryStartComplete();
        }
        return ops;
    }

    while (auto op = waiters_->PopFront())
    {
        if (op->TryStartComplete())
        {
            ops.emplace_back(move(op));
            __super::signaled_ = false;
            break;
        }
    }

    return ops;
}

template <bool ManualReset> 
void AsyncWaitHandle<ManualReset>::FinishComplete(deque<AsyncOperationSPtr> const & ops)
{
    for(auto const & op : ops)
    {
        op->FinishComplete(op);
    }
}

template <bool ManualReset> 
AsyncOperationSPtr AsyncWaitHandle<ManualReset>::BeginWaitOne(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    
    return AsyncOperation::CreateAndStart<WaitAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

template <bool ManualReset> 
ErrorCode AsyncWaitHandle<ManualReset>::EndWaitOne(
    AsyncOperationSPtr const & operation)
{
    return WaitAsyncOperation::End(operation);
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::OnSetEvent()
{
    __super::WriteNoise(TraceType, "{0}: OnSetEvent, eventFd_ = {1:x}", TextTracePtrAs(this, WaitHandle<ManualReset>), eventFd_);

    //closed_ is checked in base class Set
    const uint64 v = 1;
    auto retval = write(eventFd_, &v, sizeof(v)); 
    if (retval < 0)
    {
        auto err = errno;
        __super::WriteError(TraceType, "OnSetEvent: write failed: {0}", err);
    }
}

template <bool ManualReset>
void AsyncWaitHandle<ManualReset>::OnClose()
{
    __super::WriteNoise(TraceType, "{0}: OnClose, eventFd_ = {1:x}", TextTracePtrAs(this, WaitHandle<ManualReset>), eventFd_);

    if (fdCtx_)
    {
        // OnClose cannot be called while holding __super::mutex_ because UnregisterFd waits for callback completion and
        // callback also acquires __super::mutex_. fdCtx_ reset below without locking is safe as OnClose is called after
        // closed_ is set under mutex_ in __super::Close(), other accesses to fdCtx_ are only done if closed_ is false. 
        evtLoop_->UnregisterFd(fdCtx_, true);
        fdCtx_ = nullptr;
    }
    close(eventFd_);
    eventFd_ = -1;

    Threadpool::Post([ops = waiters_->PopAll()]
    {
       for(auto const & op : ops)
       {
           op->TryComplete(op);
       }
    });
}

namespace Common
{
    template class AsyncWaitHandle<false>;
    template class AsyncWaitHandle<true>;
}
