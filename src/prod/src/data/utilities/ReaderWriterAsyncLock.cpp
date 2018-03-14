// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using ::_delete;
using namespace ktl;
using namespace Data::Utilities;


ReaderWriterAsyncLock::ReaderWriterAsyncLock()
    : waitingWritersList_(GetThisAllocator())
{
    SetConstructorStatus(waitingWritersList_.Status());
}

ReaderWriterAsyncLock::ReaderWriterAsyncLock(__in SignalOwnersTestFunctionType TestSignalOwnersAction)
    : waitingWritersList_(GetThisAllocator())
    , signalOwnersTestAction_(TestSignalOwnersAction)
{
    SetConstructorStatus(waitingWritersList_.Status());
}

ReaderWriterAsyncLock::~ReaderWriterAsyncLock()
{
}

NTSTATUS
ReaderWriterAsyncLock::Create(
    __in KAllocator& Allocator, 
    __in ULONG Tag, 
    __out ReaderWriterAsyncLock::SPtr& result)
{
    NTSTATUS status;
    ReaderWriterAsyncLock::SPtr output = _new(Tag, Allocator) ReaderWriterAsyncLock();

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

NTSTATUS
ReaderWriterAsyncLock::Create(
    __in SignalOwnersTestFunctionType TestSignalOwnersAction,
    __in KAllocator& Allocator, 
    __in ULONG Tag, 
    __out ReaderWriterAsyncLock::SPtr& result)
{
    NTSTATUS status;
    ReaderWriterAsyncLock::SPtr output = _new(Tag, Allocator) ReaderWriterAsyncLock(TestSignalOwnersAction);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

void ReaderWriterAsyncLock::Close()
{
    KArray<KSharedPtr<AwaitableCompletionSource<bool>>> waiters(GetThisAllocator());
    K_LOCK_BLOCK(readerWriterLock_)
    {
        isClosed_ = true;

        // Set exception for all waiting writers.
        for (ULONG i = 0; i < waitingWritersList_.Count(); i++)
        {
            NTSTATUS status = waiters.Append(waitingWritersList_[i]);

            if (NT_SUCCESS(status) == false)
            {
                throw ktl::Exception(status);
            }
        }

        // Set exception for the waiting reader task.
        auto cachedWaitingReadersAcs = waitingReadersAcs_.Get();

        if (cachedWaitingReadersAcs != nullptr)
        {
            NTSTATUS status = waiters.Append(cachedWaitingReadersAcs);

            if (NT_SUCCESS(status) == false)
            {
                throw ktl::Exception(status);
            }
        }

        // Clear local state.
        waitingWritersList_.Clear();
        waitingReadersAcs_.Put(nullptr);
    }

    // Set exception for all waiters outside the lock.
    for (ULONG index = 0; index < waiters.Count(); index++)
    {
        // It is okay to synchronously complete here.
        auto completedWaiter = waiters[index];

        completedWaiter->SetException(ktl::Exception(SF_STATUS_OBJECT_CLOSED));
    }

    // Clear the collection.
    waiters.Clear();
}

ktl::Awaitable<bool> ReaderWriterAsyncLock::WaitForLock(
    __in ktl::AwaitableCompletionSource<bool> & waitingCompletionSource, 
    __in bool writer, 
    __in int timeoutInMilliSeconds)
{
    KSharedPtr<ktl::AwaitableCompletionSource<bool>> waitingAcs(&waitingCompletionSource);
    bool isLockAcquired = false;

    SharedException::CSPtr exceptionCSPtr = nullptr;

    KTimer::SPtr localTimer;
    NTSTATUS status = KTimer::Create(localTimer, GetThisAllocator(), READER_WRITER_LOCK_TAG);
    THROW_ON_FAILURE(status);

    auto timeoutTask = localTimer->StartTimerAsync(timeoutInMilliSeconds, nullptr);
    auto localWaiterTask = waitingAcs->GetAwaitable();

    co_await ktl::EitherReady(this->GetThisAllocator().GetKtlSystem(), timeoutTask, localWaiterTask);

    try
    {
        K_LOCK_BLOCK(readerWriterLock_)
        {
            if (isClosed_)
            {
                exceptionCSPtr = SharedException::Create(Ktl::Move(ktl::Exception(SF_STATUS_OBJECT_CLOSED)), GetThisAllocator());

                // This will be caught below
                auto exec = exceptionCSPtr->Info;
                throw exec;
            }

            if (waitingAcs->IsCompleted())
            {
                if (waitingAcs->IsExceptionSet())
                {
                    isLockAcquired = false;
                    // todo: use NT status codes here.
                    exceptionCSPtr = SharedException::Create(Ktl::Move(ktl::Exception(SF_STATUS_OBJECT_CLOSED)), GetThisAllocator());
                }
                else
                {
                    isLockAcquired = true;
                }
            }
            else
            {
                if (writer)
                {
                    // Timeout raced with signaling the waiting tcs (setresult).
                    // Shared ptr implements comparator that compares the raw ptr.
                    if (waitingAcs == activeWriterAcs_.Get())
                    {
                        isLockAcquired = true;
                    }
                }
                else
                {
                    // Timeout raced with signaling the waiting tcs (setresult).
                    if (waitingAcs == lastSignaledReaderAcs_)
                    {
                        isLockAcquired = true;
                    }
                }
            }

            if (isLockAcquired == false)
            {
                if (writer)
                {
                    // Dequeue waiting writer and immediately complete it w/ exception
                    waitingAcs->SetException(SF_STATUS_TIMEOUT);
                    int index = GetIndex(waitingAcs);
                    BOOLEAN wasRemoved = waitingWritersList_.Remove(index);

                    // Assert will fire if there was a delay in signaling the tcs on release writer or release reader.
                    ASSERT_IFNOT(wasRemoved == TRUE, "Delay in signaling TCS on release writer or reader");
                }
                else
                {
                    // Clean up the state that was set now that lock-acquisition has failed.
                    waitingReaderCount_--;
                    ASSERT_IFNOT(waitingReaderCount_ >= 0, "No waiting readers");
                }
            }

            if (exceptionCSPtr != nullptr)
            {
                ASSERT_IFNOT(isLockAcquired == false, "Lock should not be held processing exception during initialization");

                // This will be caught below
                auto exec = exceptionCSPtr->Info;
                throw exec;
            }
        }
    }
    catch (ktl::Exception const & e)
    {
        ASSERT_IFNOT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED, "Unexpected exception. status={0}", e.GetStatus());
    }

    // Ensure the timeout task has run to completion, cancelling if necessary
    localTimer->Cancel();

    try
    {
        co_await timeoutTask;
    }
    catch (Exception const & e)
    {
        ASSERT_IFNOT(e.GetStatus() == STATUS_CANCELLED, "Unexpected exception. status={0}", e.GetStatus());
    }

    // Move the awaitable for the timed out lock to the background to be completed and cleaned up eventually
    // If the lock didn't timeout or has already run to completion, this will be a no-op
    Task timedOutLockTask = ToTask(localWaiterTask);
    ASSERT_IFNOT(timedOutLockTask.IsTaskStarted(), "Long running TCS should be awaited on in background");

    // Clean up has finished, so throw to higher layer if necessary
    if (exceptionCSPtr != nullptr)
    {
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }

    co_return isLockAcquired;
}

ktl::Awaitable<bool> ReaderWriterAsyncLock::AcquireReadLockAsync(__in int timeoutInMilliSeconds)
{
    bool waitForReadLock = false;
    bool isLockAcquired = false;

    KSharedPtr<AwaitableCompletionSource<bool>> tempWaitingReadersAcs = nullptr;
    K_LOCK_BLOCK(readerWriterLock_)
    {
        if (isClosed_)
        {
            try
            {
                throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
            }
            catch (...)
            {
                throw;
            }
        }

        if (activeWriterAcs_.Get() == nullptr && waitingWritersList_.Count() == 0)
        {
            activeReaderCount_++;
            isLockAcquired = true;
        }
        else
        {
            waitingReaderCount_++;

            auto cachedWaitingReadersAcs = waitingReadersAcs_.Get();
            if (cachedWaitingReadersAcs == nullptr)
            {
                NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetThisAllocator(), 0, cachedWaitingReadersAcs);

                if (NT_SUCCESS(status) == false)
                {
                    throw ktl::Exception(status);
                }

                waitingReadersAcs_.Put(Ktl::Move(cachedWaitingReadersAcs));
                cachedWaitingReadersAcs = waitingReadersAcs_.Get();
            }

            // Make a copy because await is done outside the lock.
            tempWaitingReadersAcs = cachedWaitingReadersAcs;

            // Respect timeout only if the reader needs to wait.
            waitForReadLock = true;
        }
    }

    if (waitForReadLock == false)
    {
        co_await suspend_never{};
        co_return isLockAcquired;
    }
    else
    {
        isLockAcquired = co_await WaitForLock(*tempWaitingReadersAcs, false, timeoutInMilliSeconds);
    }

    co_return isLockAcquired;
}

ktl::Awaitable<bool> ReaderWriterAsyncLock::AcquireWriteLockAsync(__in int timeoutInMilliSeconds)
{
    KSharedPtr<ktl::AwaitableCompletionSource<bool>> waitingWriterAcs = nullptr;
    NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetThisAllocator(), 0, waitingWriterAcs);
    if (NT_SUCCESS(status) == false)
    {
        throw ktl::Exception(status);
    }

    bool waitForWriteLock = false;
    bool isLockAcquired = false;

    K_LOCK_BLOCK(readerWriterLock_)
    {
        if (isClosed_)
        {
            waitingWriterAcs = nullptr;
            try
            {
                throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
            }
            catch (...)
            {
                throw;
            }
        }

        if (activeWriterAcs_.Get() == nullptr && activeReaderCount_ == 0)
        {
            activeWriterAcs_.Put(Ktl::Move(waitingWriterAcs));
            waitingWriterAcs = activeWriterAcs_.Get();
            isLockAcquired = true;
        }
        else
        {
            status = waitingWritersList_.Append(waitingWriterAcs);
            if (NT_SUCCESS(status) == false)
            {
                throw ktl::Exception(status);
            }
            waitForWriteLock = true;
        }
    }

    if (waitForWriteLock == false)
    {
        co_await suspend_never{};
        co_return isLockAcquired;
    }
    else
    {
        isLockAcquired = co_await WaitForLock(*waitingWriterAcs, true, timeoutInMilliSeconds);
    }

    co_return isLockAcquired;
}

void ReaderWriterAsyncLock::ReleaseReadLock()
{
    KSharedPtr<ktl::AwaitableCompletionSource<bool>> tcsToBeSignalled = nullptr;

    K_LOCK_BLOCK(readerWriterLock_)
    {
        activeReaderCount_--;
        ASSERT_IFNOT(activeReaderCount_ >= 0, "No active readers during release lock");

        // If this is the last reader, then signal a waiting writer if one is present.
        if (activeReaderCount_ == 0 && waitingWritersList_.Count() > 0)
        {
            ASSERT_IFNOT(activeWriterAcs_.Get() == nullptr, "Null active writer TCS");
            tcsToBeSignalled = waitingWritersList_[0];
            ASSERT_IFNOT(tcsToBeSignalled != nullptr, "Null to be signalled TCS");

            activeWriterAcs_.Put(Ktl::Move(tcsToBeSignalled));
            tcsToBeSignalled = activeWriterAcs_.Get();
            BOOLEAN removed = waitingWritersList_.Remove(0);
            ASSERT_IFNOT(removed == TRUE, "Failed to remove from waitingWritersList in ReaderWriterAsyncLock::ReleaseReadLock");
        }
    }

    if (tcsToBeSignalled != nullptr)
    {
        //tcsToBeSignalled->SetResult(true);
        SignalCompletionSource(*tcsToBeSignalled);
    }
}

void ReaderWriterAsyncLock::ReleaseWriteLock()
{
    ASSERT_IFNOT(IsActiveWriter == true, "No active writers");

    KSharedPtr<ktl::AwaitableCompletionSource<bool>> tcsToBeSignalled = nullptr;

    K_LOCK_BLOCK(readerWriterLock_)
    {
        // Check for waiting writers first so that writer gets preference.
        if (waitingWritersList_.Count() > 0)
        {
            // Active writer is already true so do not have to set it, just signal the waiting writer.
            tcsToBeSignalled = waitingWritersList_[0];
            activeWriterAcs_.Put(Ktl::Move(tcsToBeSignalled));
            tcsToBeSignalled = activeWriterAcs_.Get();
            BOOLEAN removed = waitingWritersList_.Remove(0);
            ASSERT_IFNOT(removed == TRUE, "Failed to remove from waitingWritersList in ReaderWriterAsyncLock::ReleaseWriteLock");
        }
        else
        {
            auto cachedActiveWriterAcs = activeWriterAcs_.Get();
            ASSERT_IFNOT(cachedActiveWriterAcs != nullptr, "Null active writer TCS");

            activeWriterAcs_.Put(nullptr);
            if (waitingReaderCount_ > 0)
            {
                // Allow waiting readers
                tcsToBeSignalled = waitingReadersAcs_.Get();

                lastSignaledReaderAcs_ = tcsToBeSignalled;
                activeReaderCount_ = waitingReaderCount_;
                waitingReaderCount_ = 0;

                waitingReadersAcs_.Put(nullptr);
            }
        }
    }

    if (tcsToBeSignalled != nullptr)
    {
        //tcsToBeSignalled->SetResult(true);
        SignalCompletionSource(*tcsToBeSignalled);
    }
}

int ReaderWriterAsyncLock::GetIndex(__in KSharedPtr<ktl::AwaitableCompletionSource<bool>> waitingAcs)
{
    for (int i = 0; i < static_cast<int>(waitingWritersList_.Count()); i++)
    {
        if (waitingWritersList_[i] == waitingAcs)
        {
            return i;
        }
    }

    ASSERT_IF(true, "ReaderWriterAsyncLock GetIndex did not find waitingtcs");
    return -1;
}

void ReaderWriterAsyncLock::SignalCompletionSource(__in ktl::AwaitableCompletionSource<bool> & tcs)
{
    if (signalOwnersTestAction_)
    {
        signalOwnersTestAction_();
    }

    tcs.SetResult(true);
}
