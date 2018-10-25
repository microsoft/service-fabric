/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KAsyncLock.h

    Description:
        Kernel Template Library (KTL): Async Lock Class Definitions

    Notes:
        KAsyncLock can only be used from within the context of a KAsyncContext. See 
        KAsyncContext::AcquireLock() and KAsyncContext::ReleaseLock().

    History:
      richhas          23-Sep-2010         Initial version.
      richhas          19-Oct-2010         Removed KShared<> base.

--*/

#pragma once

//** KAsyncLock Definitions
class KAsyncLock 
{
    K_DENY_COPY(KAsyncLock);

public:
    NOFAIL KAsyncLock();
    ~KAsyncLock();

private:
    friend class KAsyncContextBase;
    friend class KAsyncContextTestDerivation;
    friend class KAsyncLockHandleImp;

    BOOLEAN 
    AcquireLock(__in KAsyncContextBase* RequesterPtr);

    VOID 
    ReleaseLock(__in KAsyncContextBase* RequesterPtr);

    BOOLEAN 
    IsLockOwner(__in KAsyncContextBase* RequesterPtr);    

    BOOLEAN 
    CancelAcquire(__in KAsyncContextBase* RequesterPtr);

    VOID
    HandleWasCreated();

    VOID
    HandleWasDestroyed();

private:
    KSpinLock                       _ThisLock;
    KSharedPtr<KAsyncContextBase>   _OwnerSPtr;
    LinkListHeader                  _Waiters;
};

//* KSharedAsyncLock
class KSharedAsyncLock sealed 
    :   public KAsyncLock, 
        public KObject<KSharedAsyncLock>, 
        public KShared<KSharedAsyncLock>
{
    K_FORCE_SHARED(KSharedAsyncLock);

public:
    //
    // Create an instance of a KSharedAsyncLock
    //
    static NTSTATUS
    Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out KSharedAsyncLock::SPtr& Result);

    // KAsyncContext class that acts as a handle for the producing KSharedAsyncLock
    class Handle : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(Handle);

    public:
        //
        // Start the async acquire of the KSharedAsyncLock that produced a Handle instance
        //
        // Parameters:
        //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
        //      Callback    - Delagate to call when the lock is acquired successfully or cancelled. 
        //                    The completion status (Status()) will be:
        //                          STATUS_SUCCESS      - Lock was acquired - ReleaseLock() must be called
        //                          STATUS_CANCELLED    - The lock was not acquired because Cancel() was called
        virtual VOID 
        StartAcquireLock(
            __in_opt KAsyncContextBase* const Parent, 
            __in KAsyncContextBase::CompletionCallback Callback,
            __in_opt KAsyncGlobalContext* const GlobalContext = nullptr) = 0;

        #if defined(K_UseResumable)
        // Start an awaitable KSharedAsyncLock operation
        //
        // Parameters:
        //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
        //
        //	Usage: 
        //			NTSTATUS status = co_await myHandle->StartAcquireLockAsync(nullptr);		// wait - no parent
        //
        ktl::Awaitable<NTSTATUS> StartAcquireLockAsync(__in_opt KAsyncContextBase* const Parent) noexcept
        {
            ktl::kasync::StartAwaiter::SPtr awaiter;

            NTSTATUS status = ktl::kasync::StartAwaiter::Create(
                GetThisAllocator(),
                GetThisAllocationTag(),
                *this,
                awaiter,
                Parent,
                nullptr);

            status = co_await *awaiter;
            co_return status;
        }
        #endif

        //
        // Release the lock currently held by this instance.
        //
        // NOTE: Must be called if StartAcquire() is completed with a STATUS_SUCCESS before dtor, Reuse(), or 
        //       the next StartAcquire() or a FAILFAST will occur.
        //
        virtual VOID 
        ReleaseLock() = 0;
    };

    // Create an instance of an KAsyncLock::Handle
    //
    NTSTATUS
    CreateHandle(__in ULONG AllocationTag, __out Handle::SPtr& Result);

    #if defined(K_UseResumable)
    // Start an awaitable KSharedAsyncLock operation
    //
    // Parameters:
    //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
    //
    //	Usage: 
    //			NTSTATUS status = co_await myHandle->StartAcquireLockAsync(tag, nullptr);		// wait - no parent
    //
    //

    ktl::Awaitable<NTSTATUS> StartAcquireLockAsync(
        __in ULONG Tag,
        __out Handle::SPtr& Result,
        __in_opt KAsyncContextBase* const Parent) noexcept
    {
        NTSTATUS status = CreateHandle(Tag, Result);
        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        co_return co_await Result->StartAcquireLockAsync(Parent);
    }
    #endif
};

