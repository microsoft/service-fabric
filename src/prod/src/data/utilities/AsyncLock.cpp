// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

AsyncLock::AsyncLock()
    : KObject()
    , KShared()
    , lock_(GetThisKtlSystem())
{
}

AsyncLock::~AsyncLock()
{
}

NTSTATUS AsyncLock::Create(
    __in KAllocator& allocator,
    __in ULONG tag,
    __out AsyncLock::SPtr & result) noexcept
{
    AsyncLock::SPtr pointer = _new(tag, allocator) AsyncLock();

    if (!pointer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(pointer->Status()))
    {
        return pointer->Status();
    }

    result = Ktl::Move(pointer);
    return STATUS_SUCCESS;
}

Awaitable<bool> AsyncLock::AcquireAsync(__in Common::TimeSpan const & timeout) noexcept
{
    Awaitable<void> locker = lock_.AcquireAsync();

    // When no contention, this is the fast case where lock is already acquired
    if (locker.IsReady())
    {
        co_await locker;
        co_return true;
    }

    // Slow case where there is contention
    KTimer::SPtr timer;
    NTSTATUS status = KTimer::Create(timer, GetThisAllocator(), 'LYSA');
    UINT uintTimeout = 0;

    if (!NT_SUCCESS(status))
    {
        ToTaskAndRelease(locker);
        co_return false;
    }

    // Timeout is too big
    if (timeout.TotalPositiveMilliseconds() > MAXUINT)
    {
        uintTimeout = MAXUINT;
    }
    else
    {
        uintTimeout = static_cast<uint>(timeout.TotalPositiveMilliseconds());
    }

    Awaitable<NTSTATUS> timeoutTask = timer->StartTimerAsync(uintTimeout, nullptr);
    
    co_await EitherReady(GetThisKtlSystem(), locker, timeoutTask);

    // If lock was acquired successfully, cancel the timer and return true
    if (locker.IsReady())
    {
        timer->Cancel();
        ToTask(timeoutTask);
        co_await locker;
        co_return true;
    }

    // If timeout happened, await the lock and release it immediately
    ToTaskAndRelease(locker);

    ASSERT_IFNOT(timeoutTask.IsReady(), "Timeout task must be ready");
    co_await timeoutTask;
    co_return false;
}

void AsyncLock::ReleaseLock() noexcept
{
    lock_.Release();
}
