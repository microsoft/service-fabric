// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        ktl::Awaitable<NTSTATUS>
        AcquireLock(__in ULONG allocationTag, __in KAllocator& allocator, __in KAsyncEvent& lock);
    }
}

#define AsyncLockBlock(lock) \
    NTSTATUS __lockStatus = co_await Data::Utilities::AcquireLock(GetThisAllocationTag(), GetThisAllocator(), lock); \
    if (!NT_SUCCESS(__lockStatus)) { co_return __lockStatus; } \
    KFinally([&] { lock.SetEvent(); });
