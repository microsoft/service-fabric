/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KQuotaGate.cpp

    Description:
      Kernel Template Library (KTL): Async QuotaGate/Semaphore Implementation

    History:
      richhas          27-May-2010         Initial version.

--*/

#include "ktl.h"

//** KQuotaGate implementation
KQuotaGate::KQuotaGate()
    :   _Waiters(FIELD_OFFSET(AcquireContext, _Links)),
        _FreeQuanta(0),
        _IsActive(FALSE),
        _ThisVersion(1),
        _ActivityCount(0)
{
}

KQuotaGate::~KQuotaGate()
{
    KAssert(!_IsActive);
    KAssert(_Waiters.IsEmpty());
}

NTSTATUS
KQuotaGate::Create(
    __in ULONG AllocationTag, 
    __in KAllocator& Allocator, 
    __out KSharedPtr<KQuotaGate>& Context)
{
    Context = _new(AllocationTag, Allocator) KQuotaGate();
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;    
}


VOID 
KQuotaGate::UnsafeAcquireActivity()
//
// Acquire the right to use this instance. Once _ActivityCount reaches zero, no
// such right will be granted as the instance is in a deactivating state.
//
{
    KAssert(_ThisLock.IsOwned());
    KAssert(_IsActive && (_ActivityCount > 0));
    _ActivityCount++;
}

VOID
KQuotaGate::UnsafeReleaseActivity(ULONG ByCountOf)
//
// Release the right of usage acquired via UnsafeTryAcquireActivity().
//
// Completes operation cycle when _ActivityCount goes to zero.
//
{
    KAssert(_ThisLock.IsOwned());
    KAssert(_ActivityCount >= ByCountOf);
    _ActivityCount -= ByCountOf;

    if (_ActivityCount == 0)
    {
        BOOLEAN completing = Complete(STATUS_SUCCESS);
        KFatal(completing);

        _ThisVersion++;             // Invalidate the current version of this instance as viewed by 
                                    // existing DequeueOperation instances produced by this Queue.
    }
}

NTSTATUS
KQuotaGate::Activate(
    __in ULONGLONG InitialFreeQuanta,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(!_IsActive);
        KAssert(_ActivityCount == 0);
        _IsActive = TRUE;
        _FreeQuanta = InitialFreeQuanta;
        _ActivityCount++;                       // #2
    }

    Start(ParentAsyncContext, CallbackPtr);
    return STATUS_PENDING;
}

VOID
KQuotaGate::OnStart()
{
}

VOID
KQuotaGate::Deactivate()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(_IsActive);
        _IsActive = FALSE;              // Lock the API
    }

    Cancel();
}

VOID
KQuotaGate::OnCancel()
{
    // trap Cancel calls not from Deactivate()
    KFatal(!_IsActive);             

    // Wake up any suspended waiters with a status of K_STATUS_SHUTDOWN_PENDING
    ULONG                           totalActivityToRelease = 1;      // Allow eventual Complete(); reverse #2 in Activate()

    _ThisLock.Acquire();
        KNodeList<AcquireContext>   waiters(Ktl::Move(_Waiters));
    _ThisLock.Release();

    AcquireContext* currentWaiter = waiters.PeekHead();

    while (currentWaiter != nullptr)
    {
        AcquireContext* nextWaiter = waiters.Successor(currentWaiter);
        waiters.Remove(currentWaiter);

        totalActivityToRelease++;
        currentWaiter->Complete(K_STATUS_SHUTDOWN_PENDING);

        currentWaiter = nextWaiter;
    }

    K_LOCK_BLOCK(_ThisLock)
    {
        UnsafeReleaseActivity(totalActivityToRelease);      // allow eventual Complete(); 
                                                            // reverse #1 in StartWait() 
    }
}

VOID
KQuotaGate::OnReuse()
{
    KAssert(!_IsActive);
    KAssert(_Waiters.IsEmpty());
    KAssert(_ActivityCount == 0);
}

VOID
KQuotaGate::StartWait(AcquireContext& Waiter)
{
    BOOLEAN         waitSatisfied = FALSE;
    NTSTATUS        statusToCompleteWith = STATUS_SUCCESS;
    ULONGLONG       quantaAllocated = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsActive || (Waiter._OwnerVersion != _ThisVersion))
        {
            statusToCompleteWith = K_STATUS_SHUTDOWN_PENDING;
            waitSatisfied = TRUE;
        }
        else
        {
            if (_Waiters.IsEmpty() && (_FreeQuanta >= Waiter._DesiredQuanta))
            {
                _FreeQuanta -= (quantaAllocated = Waiter._DesiredQuanta);
                waitSatisfied = TRUE;
            }
            else
            {
                // Not enough remaining quota or others waiting - put item on the end of the wait list
                _Waiters.AppendTail(&Waiter);
                UnsafeAcquireActivity();
            }
        }
    }

    if (waitSatisfied)
    {
        BOOLEAN didComplete = Waiter.Complete(statusToCompleteWith);
        if (!didComplete && (statusToCompleteWith == STATUS_SUCCESS))
        {
            // For some reason another Complete() call raced ahead of this call...
            // put back the allocated quanta
            ReleaseQuanta(quantaAllocated);
        }
    }
}

VOID
KQuotaGate::CancelWait(AcquireContext& Waiter)
{
    BOOLEAN             cancelSatisfied = FALSE;
    AcquireContext*     topWaiter = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        topWaiter = _Waiters.PeekHead();
        cancelSatisfied = (_Waiters.Remove(&Waiter) != nullptr);
    }

    if (cancelSatisfied)
    {
        BOOLEAN didComplete = Waiter.Complete(STATUS_CANCELLED);
        KFatal(didComplete);

        _ThisLock.Acquire();
            UnsafeReleaseActivity();
        _ThisLock.Release();

        if (topWaiter == &Waiter)
        {
            // The top of list waiter was canceled, cause re-eval of new top of list waiters 
            // in case they can make progress now that the current top wait is removed.
            ReleaseQuanta(0);           
        }
    }
}

VOID
KQuotaGate::ReleaseQuanta(ULONGLONG QuantaToRelease)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        ULONG           releasedActivity = 0;

        _FreeQuanta += QuantaToRelease;

        AcquireContext*     topWaiter;
        while (((topWaiter = _Waiters.PeekHead()) != nullptr) && (topWaiter->_DesiredQuanta <= _FreeQuanta))
        {
            _FreeQuanta -= topWaiter->_DesiredQuanta;
            void* t = _Waiters.RemoveHead();
            KFatal(t != nullptr);

            releasedActivity++;
            topWaiter->Complete(STATUS_SUCCESS);
        }
        
        if (releasedActivity > 0)
        {
            UnsafeReleaseActivity(releasedActivity);
        }
    }
}

NTSTATUS 
KQuotaGate::CreateAcquireContext(AcquireContext::SPtr& Context)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsActive)
        {
            Context.Reset();
            return K_STATUS_SHUTDOWN_PENDING;
        }
    }

    Context = _new(GetThisAllocationTag(), GetThisAllocator()) AcquireContext(*this, _ThisVersion);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }

    return status;
}

KQuotaGate::AcquireContext::AcquireContext(KQuotaGate& Owner, ULONG OwnerVersion)
    :   _OwningQuotaGate(&Owner),
        _OwnerVersion(OwnerVersion)
{
}

KQuotaGate::AcquireContext::~AcquireContext()
{
}

VOID
KQuotaGate::AcquireContext::StartAcquire(
    __in ULONGLONG DesiredQuanta,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _DesiredQuanta = DesiredQuanta;
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KQuotaGate::AcquireContext::OnStart()
{
    _OwningQuotaGate->StartWait(*this);
}

VOID
KQuotaGate::AcquireContext::OnCancel()
{
    _OwningQuotaGate->CancelWait(*this);
}

#if defined(K_UseResumable)

NTSTATUS
KCoQuotaGate::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KSharedPtr<KCoQuotaGate>& Context)
{
    Context = _new(AllocationTag, Allocator) KCoQuotaGate();
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

NOFAIL KCoQuotaGate::KCoQuotaGate()
{
}

KCoQuotaGate::~KCoQuotaGate()
{
}
#endif