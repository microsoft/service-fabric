// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

CompletionTask::CompletionTask()
    : KObject()
    , KShared()
    , completionTcs_()
    , isCompleted_(false)
    , thisLock_()
    , errorCode_(STATUS_SUCCESS)
{
}

CompletionTask::~CompletionTask()
{
}

CompletionTask::SPtr CompletionTask::Create(__in KAllocator & allocator)
{
    CompletionTask * pointer = _new(COMPLETIONTASK_TAG, allocator)CompletionTask();
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return CompletionTask::SPtr(pointer);
}

Awaitable<NTSTATUS> CompletionTask::AwaitCompletion()
{
    AwaitableCompletionSource<NTSTATUS>::SPtr completionTcs;
    bool completeNow = false;
    NTSTATUS returnCode = STATUS_SUCCESS;

    K_LOCK_BLOCK(thisLock_)
    {
        if (isCompleted_.load() == false)
        {
            if (completionTcs_ == nullptr)
            {
                completionTcs_ = CreateAwaitableCompletionSource<NTSTATUS>(COMPLETIONTASK_TAG, GetThisAllocator());
            }

            completionTcs = completionTcs_;
        }
        else
        {
            completeNow = true;
            returnCode = errorCode_.load();
        }
    }

    if (completeNow == false)
    {
        returnCode = co_await completionTcs->GetAwaitable();
    }

    co_return returnCode;
}
 
void CompletionTask::CompleteAwaiters(__in NTSTATUS errorCode)
{
    AwaitableCompletionSource<NTSTATUS>::SPtr awaiters = nullptr;

    K_LOCK_BLOCK(thisLock_)
    {
        ASSERT_IF(
            isCompleted_.load(),
            "Awaitable already completed. Cannot complete again");

        isCompleted_.store(true);
        awaiters = completionTcs_;
        errorCode_.store(errorCode);
    }

    if (awaiters == nullptr)
    {
        // There are no awaiters. Just return
        return;
    }

    // There are awaiters who need to be signalled
    bool isSet = awaiters->TrySetResult(errorCode);
    ASSERT_IFNOT(
        isSet == true,
        "Completion Task completed without isSet = true");
}
