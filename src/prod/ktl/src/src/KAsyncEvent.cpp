/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KAsyncEvent.cpp

    Description:
      Kernel Template Library (KTL): Async Event Implementation

    History:
      richhas          15-Apr-2010         Initial version.

--*/

#include "ktl.h"

//** KAsyncEvent implementation
KAsyncEvent::KAsyncEvent(__in BOOLEAN IsManualReset, __in BOOLEAN InitialState)
    :   _IsManualReset(IsManualReset),
        _IsSignaled(InitialState),
        _Waiters(FIELD_OFFSET(WaitContext, _Links))
{
}

KAsyncEvent::~KAsyncEvent()
{
    KFatal(_Waiters.Count() == 0);
}

VOID
KAsyncEvent::SetEvent()
{
    WaitContext* waiter = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (!_IsSignaled)
        {
            if (_IsManualReset)
            {
                // BUG, richhas, xxxxx, Consider adding a primitive to KNodeList to allow moving the entire contents of
                //                      one list to another VERY QUICKLY
                _IsSignaled = TRUE;
                while (_Waiters.Count() > 0)
                {
                    WaitContext* nextWaiter = _Waiters.RemoveHead();
                    nextWaiter->Complete(STATUS_SUCCESS);
                }
            }
            else    // Auto Reset mode
            {
                waiter = _Waiters.RemoveHead();
                _IsSignaled = (waiter == nullptr);  // Leave in the signaled state if no pending waiters
            }
        }
    }

    if (waiter != nullptr)
    {
        // Let first waiter go
        waiter->Complete(STATUS_SUCCESS);
    }
}

VOID
KAsyncEvent::ResetEvent()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _IsSignaled = FALSE;
    }
}

BOOLEAN
KAsyncEvent::IsSignaled() const
{
    return _IsSignaled;
}

BOOLEAN
KAsyncEvent::IsManualResetMode()
{
    return _IsManualReset;
}

ULONG
KAsyncEvent::CountOfWaiters()
{
    ULONG       result = 0;
    K_LOCK_BLOCK(_ThisLock)
    {
        result = _Waiters.Count();
    }
    return result;
}

VOID
KAsyncEvent::Wait(WaitContext* const NewWaiter)
{
    KAssert((NewWaiter->_Links.Blink == nullptr) && (NewWaiter->_Links.Flink == nullptr));  // Not on a list
    BOOLEAN completeNewWaiter = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        if ((completeNewWaiter = _IsSignaled) == TRUE)
        {
            if (!_IsManualReset)
            {
                // Auto-reset
                _IsSignaled = FALSE;
            }
        }
        else
        {
            // Must suspend NewWaiter
            _Waiters.AppendTail(NewWaiter);
        }
    }

    if (completeNewWaiter)
    {
        NewWaiter->Complete(STATUS_SUCCESS);
    }
}

VOID
KAsyncEvent::CancelWait(WaitContext* const Waiter)
{
    WaitContext* waiterRemoved = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        waiterRemoved = _Waiters.Remove(Waiter);
    }

    if (waiterRemoved != nullptr)
    {
        KAssert(Waiter == waiterRemoved);
        BOOLEAN completeAccepted =  Waiter->Complete(STATUS_CANCELLED);
        KFatal(completeAccepted);
    }
}

VOID
KAsyncEvent::ChangeMode(BOOLEAN IsManualReset)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _IsManualReset = IsManualReset;
    }
}

NTSTATUS
KAsyncEvent::CreateWaitContext(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out WaitContext::SPtr& Context)
{
    Context = _new(AllocationTag, Allocator) WaitContext(this);
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

KAsyncEvent::WaitContext::WaitContext(KAsyncEvent* const Owner)
    :   _OwningAsyncEvent(Owner)
{
}

KAsyncEvent::WaitContext::~WaitContext()
{
}

VOID
KAsyncEvent::WaitContext::StartWaitUntilSet(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KAsyncEvent::WaitContext::OnStart()
{
    _OwningAsyncEvent->Wait(this);
}

VOID
KAsyncEvent::WaitContext::OnCancel()
{
    _OwningAsyncEvent->CancelWait(this);
}

VOID
KAsyncEvent::WaitContext::OnReuse()
{
    KAssert((_Links.Blink == nullptr) && (_Links.Flink == nullptr));  // Not on a list
}

