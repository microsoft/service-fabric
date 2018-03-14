// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // An abstraction for ktl::AwaitableCompletionSource(ACS) and is contained in a LogRecord.
    // Avoids creating multiple ACS objects if multiple callers end up invoking "await" on a single log record's "AwaitReplication" or "AwaitApply" etc.
    //
    class CompletionTask 
        : public KObject<CompletionTask>
        , public KShared<CompletionTask>
    {
        K_FORCE_SHARED(CompletionTask)

    public:

        static CompletionTask::SPtr Create(__in KAllocator & allocator);
        
        template<typename T>
        static KSharedPtr<ktl::AwaitableCompletionSource<T>> CreateAwaitableCompletionSource(ULONG tag, __in KAllocator & allocator)
        {
            KSharedPtr<ktl::AwaitableCompletionSource<T>> result;
            NTSTATUS status = ktl::AwaitableCompletionSource<T>::Create(
                allocator, 
                tag, 
                result);
            
            if (!NT_SUCCESS(status))
            {
                throw ktl::Exception(status);
            }

            return result;
        }

        //
        // Returns TRUE if the underlying awaitable is completed
        //
        __declspec(property(get = get_IsCompleted)) bool IsCompleted;
        bool get_IsCompleted() const
        {
            return isCompleted_.load();
        }

        __declspec(property(get = get_CompletionCode)) NTSTATUS CompletionCode;
        NTSTATUS get_CompletionCode() const
        {
            ASSERT_IFNOT(
                isCompleted_.load() == true, 
                "CompletionTask must be completed before checking its result code");

            return errorCode_.load();
        }

        ktl::Awaitable<NTSTATUS> AwaitCompletion();

        //
        // Completes the underlying awaitable with the optional exception.
        // exception can be nullptr
        //
        void CompleteAwaiters(__in NTSTATUS errorCode);

    private:

        mutable KSpinLock thisLock_;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr completionTcs_;
        Common::atomic_bool isCompleted_;
        Common::atomic_long errorCode_;
    };
}
