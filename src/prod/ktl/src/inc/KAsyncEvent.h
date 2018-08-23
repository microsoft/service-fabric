/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KAsyncEvent.h

    Description:
      Kernel Template Library (KTL): Async Event Support Definitions

    History:
      richhas          15-Apr-2011         Initial version.

--*/

#pragma once

//** KAsyncEvent class
//
// Description:
//      This class is a simple async for of KEvent with only a couple of minor extensions. Specifically
//      it provides the ability to change its mode of operation between Manual and Auto reset. The user
//      allocates one or more instances of KAsyncEvent::WaitContext via the CreateWaitContext() factory
//      method. Async waits are then requested via KAsyncEvent::WaitContext::StartWaitUntilSet()
//      which follows the KAsyncContext patterns for callback, completion, reuse, and cancelation.
//
//      NOTE: The user of this class is responsible for making sure that there are no pending
//            WaitContext operations at time of destruction. Not doing so will result in a fail-fast.
//            Calling Cancel() on outstanding WaitContexts is the means for doing this sort of clean up.
//            Typically the container for KAsyncEvents will be a KAsyncContextBase derivation supporting
//            it's own notion of shutdown. As such the WaitContexts that are active will have that
//            container as their parent KAsyncContext and will thus keep a ref count of that parent
//            while active. This boils down to being very careful when trying to use KAsyncEvent instances
//            in cases where there is not a shutdown notion for KAsyncEvent containers.
//
class KAsyncEvent sealed
{
    K_DENY_COPY(KAsyncEvent);

public:
    NOFAIL KAsyncEvent(__in BOOLEAN IsManualReset = FALSE, __in BOOLEAN InitialState = FALSE);
    ~KAsyncEvent();

    // Signal an event.
    //
    // In auto-reset mode if there is a pending waiter, that waiter will be released (completed) and
    // the event's state automatically reset to not signaled. If there is not a pending waiter, the
    // event is left in a signaled state. In manual-reset mode any pending and future waiters will
    // be released and the event left in a signaled state until ResetEvent() is called.
    //
    VOID
    SetEvent();

    // Clear the state of the event to not-signaled.
    VOID
    ResetEvent();

    VOID
    ChangeMode(BOOLEAN IsManualReset);

    BOOLEAN
    IsSignaled() const;

    BOOLEAN
    IsManualResetMode();

    ULONG
    CountOfWaiters();

    // Class representing an (one) async wait operation on a KAsyncEvent instance
    class WaitContext : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(WaitContext);

    public:
        // Schedule an async wait callback.
        //
        // This method will result in *CallbackPtr being invoked when either the
        // related KAsyncEvent is in a signaled state or Cancel() is called. If
        // Cancel() is called, a value of STATUS_CANCELLED will be returned from
        // Status() to indicate the completion is a result of that cancel. Otherwise
        // STATUS_SUCCESS is always returned.
        VOID
        StartWaitUntilSet(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

#if defined(K_UseResumable)

        ktl::Awaitable<NTSTATUS> StartWaitUntilSetAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) noexcept
        {
            // DEBUG: Trap promise corruption
            auto& myPromise = *co_await ktl::CorHelper::GetCurrentPromise<ktl::Awaitable<NTSTATUS>::promise_type>();
            auto myState = myPromise.GetState();
            KInvariant(myState.IsUnusedBitsValid());
            KInvariant(!myState.IsReady());
            KInvariant(!myState.IsSuspended());
            KInvariant(!myState.IsFinalHeld());
            KInvariant(!myState.IsCalloutOnFinalHeld());

            ktl::kasync::StartAwaiter::SPtr awaiter;

            NTSTATUS status = ktl::kasync::StartAwaiter::Create(
                GetThisAllocator(),
                GetThisAllocationTag(),
                *this,
                awaiter,
                ParentAsyncContext,
                nullptr);

            if (!NT_SUCCESS(status))
            {
                co_return status;
            }

            // DEBUG: Trap promise corruption
            myState = myPromise.GetState();
            KInvariant(myState.IsUnusedBitsValid());
            KInvariant(!myState.IsReady());
            KInvariant(!myState.IsFinalHeld());

            status = co_await *awaiter;

            // DEBUG: Trap promise corruption
            myState = myPromise.GetState();
            KInvariant(myState.IsUnusedBitsValid());
            KInvariant(!myState.IsReady());
            KInvariant(!myState.IsFinalHeld());
            co_return status;
        }
#endif
        __inline BOOLEAN
        IsOwner(KAsyncEvent& WouldBeOwner)	{ return &WouldBeOwner == _OwningAsyncEvent; }

    protected:
        WaitContext(KAsyncEvent* const Owner);

    private:
        friend class KAsyncEvent;

        VOID
        OnStart() override;

        VOID
        OnCancel() override;

        VOID
        OnReuse() override;

    private:
        KListEntry          _Links;
        KAsyncEvent* const  _OwningAsyncEvent;
    };

    NTSTATUS
    CreateWaitContext(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out WaitContext::SPtr& Context);

private:
    VOID
    Wait(WaitContext* const NewWaiter);

    VOID
    CancelWait(WaitContext* const Waiter);

private:
    KSpinLock                   _ThisLock;
    BOOLEAN                     _IsSignaled;
    BOOLEAN                     _IsManualReset;
    KNodeList<WaitContext>      _Waiters;
};


