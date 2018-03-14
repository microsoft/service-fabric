// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

ktl::Awaitable<NTSTATUS>
Data::Utilities::AcquireLock(__in ULONG allocationTag, __in KAllocator& allocator, __in KAsyncEvent& lock)
{
    NTSTATUS status;
    KAsyncEvent::WaitContext::SPtr lockWaitContext;

    status = lock.CreateWaitContext(allocationTag, allocator, lockWaitContext);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await lockWaitContext->StartWaitUntilSetAsync(nullptr);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    co_return STATUS_SUCCESS;
}
