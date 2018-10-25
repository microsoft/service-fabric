/*++

Module Name:

    kresourcelock.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KResourceLock object.

Author:

    Norbert P. Kusters (norbertk) 18-Nov-2011

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>


KResourceLock::~KResourceLock(
    )
{
    KFatal(_SharedRefCount == 1);
    KFatal(!_ExclusiveAcquireInProgress);
    KFatal(!_ExclusiveWaiter);
    KFatal(!_Waiters.Count());
}

KResourceLock::KResourceLock(
    ) :
    _Waiters(FIELD_OFFSET(AcquireContext, ListEntry))
{
    _SharedRefCount = 1;
    _ExclusiveAcquireInProgress = FALSE;
    _ExclusiveWaiter = NULL;
}

NTSTATUS
KResourceLock::Create(
    __out KResourceLock::SPtr& ResourceLock,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    ResourceLock = _new(AllocationTag, Allocator) KResourceLock;

    if (!ResourceLock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ResourceLock->Status();

    if (!NT_SUCCESS(status)) {
        ResourceLock = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KResourceLock::AllocateAcquire(
    __out AcquireContext::SPtr& Async,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    Async = _new(AllocationTag, GetThisAllocator()) AcquireContext;

    if (!Async) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Async->Status();

    if (!NT_SUCCESS(status)) {
        Async = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
KResourceLock::AcquireShared(
    __in AcquireContext& Async,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine does a shared acquire of this resource and completes once the resource is acquired.

Arguments:

    Async       - Supplies the context for this request.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, a parent async to synchronize the completion with.

Return Value:

    None.

--*/

{
    if (Async.Status() != K_STATUS_NOT_STARTED) {
        Async.Reuse();
    }

    Async.InitializeForAcquire(*this, FALSE);

    Async.Start(ParentAsync, Completion);
}

BOOLEAN
KResourceLock::TryAcquireShared(
    )

/*++

Routine Description:

    This routine attempts a shared acquire of this resource and fails immediately if is not possible.

Arguments:

    None.

Return Value:

    FALSE   - The resource was not acquired.

    TRUE    - The resource was acquired.

--*/

{
    InterlockedIncrement(&_SharedRefCount);

    if (_ExclusiveAcquireInProgress) {
        ReleaseShared();
        return FALSE;
    }

    return TRUE;
}

VOID
KResourceLock::AcquireExclusive(
    __in AcquireContext& Async,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    if (Async.Status() != K_STATUS_NOT_STARTED) {
        Async.Reuse();
    }

    Async.InitializeForAcquire(*this, TRUE);

    Async.Start(ParentAsync, Completion);
}

VOID
KResourceLock::ReleaseShared(
    )
{
    LONG r;

    r = InterlockedDecrement(&_SharedRefCount);
    KFatal(r >= 0);

    if (r) {
        return;
    }

    //
    // Check to see if this is the first touch to zero and complete the exclusive waiter in that case.
    //

    K_LOCK_BLOCK(_SpinLock)
    {
        if (_ExclusiveWaiter) {
            _ExclusiveWaiter->DoComplete(STATUS_SUCCESS);
            _ExclusiveWaiter = NULL;
        }
    }
}

VOID
KResourceLock::ReleaseExclusive(
    )
{
    BOOLEAN exclusiveWaiterSet = FALSE;
    AcquireContext* context;

    //
    // It must be the case that there isn't an exclusive waiter.  That is, the exclusive waiter has already received
    // notification that it has the lock.
    //

    K_LOCK_BLOCK(_SpinLock)
    {

        KFatal(!_ExclusiveWaiter);
        KFatal(_ExclusiveAcquireInProgress);

        context = _Waiters.RemoveHead();

        if (!context) {

            //
            // If there aren't any waiters, then just put this lock back into its initial state.
            //


            InterlockedIncrement(&_SharedRefCount);
            InterlockedExchange(&_ExclusiveAcquireInProgress, FALSE);

            return;
        }

        if (context->_IsExclusiveAcquire) {

            //
            // If the next waiter is exclusive, then keep the lock in the exclusive wait state and complete the next waiter.
            //

            context->DoComplete(STATUS_SUCCESS);

            return;
        }

        //
        // The next waiter is shared.  Grab all of the shared waiters and complete them.
        //

        InterlockedIncrement(&_SharedRefCount);

        for (;;) {

            InterlockedIncrement(&_SharedRefCount);

            context->DoComplete(STATUS_SUCCESS);

            context = _Waiters.RemoveHead();

            if (!context) {

                //
                // All of the waiters we're shared.  Allow new shared acquires to proceed from this point.
                //

                InterlockedExchange(&_ExclusiveAcquireInProgress, FALSE);

                break;
            }

            if (context->_IsExclusiveAcquire) {

                //
                // Set up this exclusive waiter for the shared run down.
                //

                _ExclusiveWaiter = context;
                exclusiveWaiterSet = TRUE;

                break;
            }
        }
    }

    //
    // Set up the exclusive waiter, if any, to be completed after the shared ref count goes to zero.
    //

    if (exclusiveWaiterSet) {
        ReleaseShared();
    }
}

KResourceLock::AcquireContext::~AcquireContext(
    )
{
    // Nothing.
}

KResourceLock::AcquireContext::AcquireContext(
    )
{
    // Nothing.
}

VOID
KResourceLock::AcquireContext::InitializeForAcquire(
    __inout KResourceLock& ResourceLock,
    __in BOOLEAN IsExclusiveAcquire
    )
{
    _ResourceLock = &ResourceLock;
    _IsExclusiveAcquire = IsExclusiveAcquire;
}

VOID
KResourceLock::AcquireContext::OnStart(
    )
{
    if (_IsExclusiveAcquire) {

        //
        // Exclusive acquire.
        //

        K_LOCK_BLOCK(_ResourceLock->_SpinLock)
        {
            if (_ResourceLock->_ExclusiveAcquireInProgress) {

                //
                // In this case there is already an exclusive owner of this lock.  Queue up to wait in line.
                //

                _ResourceLock->_Waiters.AppendTail(this);
                return;
            }

            //
            // It must be the case that there isn't an exclusive waiter since the above flag is not set.  Make this
            // context the exclusive waiter and initiate the shared rundown.
            //

            KFatal(_ResourceLock->_SharedRefCount);
            KFatal(!_ResourceLock->_ExclusiveWaiter);

            InterlockedExchange(&_ResourceLock->_ExclusiveAcquireInProgress, TRUE);

            _ResourceLock->_ExclusiveWaiter = this;
        }

        //
        // Initiate the rundown for the exclusive acquire.
        //

        _ResourceLock->ReleaseShared();

        return;
    }

    //
    // Shared acquire.
    //

    K_LOCK_BLOCK(_ResourceLock->_SpinLock)
    {
        if (_ResourceLock->_ExclusiveAcquireInProgress) {

            //
            // The lock is being acquired exclusively.  Queue up to wait in line.
            //

            _ResourceLock->_Waiters.AppendTail(this);
            return;
        }

        //
        // All is clear.  Take a shared ref.
        //

        KFatal(_ResourceLock->_SharedRefCount);
        KFatal(!_ResourceLock->_ExclusiveWaiter);
        KFatal(!_ResourceLock->_Waiters.Count());

        InterlockedIncrement(&_ResourceLock->_SharedRefCount);

        DoComplete(STATUS_SUCCESS);
    }
}

VOID
KResourceLock::AcquireContext::OnCancel(
    )
{
    BOOLEAN b;

    K_LOCK_BLOCK(_ResourceLock->_SpinLock)
    {
        b = DoComplete(STATUS_CANCELLED);

        if (!b) {

            //
            // The request has already been completed.
            //

            KTraceCancelCalled(this, FALSE, FALSE, 0);
            return;
        }

        KTraceCancelCalled(this, TRUE, TRUE, 0);

        //
        // Figure out what was just queued to complete, and dequeue it from the internal data structures.
        //

        if (this != _ResourceLock->_ExclusiveWaiter) {

            //
            // This is a queued request, not actively doing anything.  Just dequeue it.
            //

            _ResourceLock->_Waiters.Remove(this);

            return;
        }

        //
        // This is a cancel of an active exclusive acquire.  Execute the code as though there was a release.
        //

        _ResourceLock->_ExclusiveWaiter = NULL;
    }

    //
    // Do the release for the cancelled exclusive acquire.
    //

    _ResourceLock->ReleaseExclusive();
}

VOID
KResourceLock::AcquireContext::OnReuse(
    )
{
    _ResourceLock = NULL;
}

BOOLEAN
KResourceLock::AcquireContext::DoComplete(
    __in NTSTATUS Status
    )
{
    return Complete(Status);
}
