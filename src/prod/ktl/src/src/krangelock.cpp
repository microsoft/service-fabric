/*++

Module Name:

    krangelock.cpp

Abstract:

    This file contains the implementation of the KRangeLock code.

Author:

    Norbert P. Kusters (norbertk) 26-Dec-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>


KRangeLock::~KRangeLock(
    )
{
    KFatal(!_LocksTable.Count());
}

KRangeLock::KRangeLock(
    __in ULONG AllocationTag
    ) : _Compare(AcquireContext::Compare), _LocksTable(FIELD_OFFSET(AcquireContext, TableEntry), _Compare)
{
    SetConstructorStatus(Initialize(AllocationTag));
}

NTSTATUS
KRangeLock::Create(
    __out KRangeLock::SPtr& RangeLock,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine will create a new range lock.

Arguments:

    RangeLock       - Returns the newly created range lock.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies the allocator.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    KRangeLock* rangeLock;
    NTSTATUS status;

    rangeLock = _new(AllocationTag, Allocator) KRangeLock(AllocationTag);

    if (!rangeLock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = rangeLock->Status();

    if (!NT_SUCCESS(status)) {
        _delete(rangeLock);
        return status;
    }

    RangeLock = rangeLock;

    return STATUS_SUCCESS;
}

NTSTATUS
KRangeLock::AllocateAcquire(
    __out AcquireContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a context that can be used for acquiring a range.

Arguments:

    Async   - Returns the newly allocated context.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    return AllocateAcquireStatic(Async, GetThisAllocator(), AllocationTag);
}

NTSTATUS
KRangeLock::AllocateAcquireStatic(
    __out AcquireContext::SPtr& Async,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a context that can be used for acquiring a range.

Arguments:

    Async           - Returns the newly allocated context.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies the allocator.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    AcquireContext* context;
    NTSTATUS status;

    context = _new(AllocationTag, Allocator) AcquireContext;

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    Async = context;

    return STATUS_SUCCESS;
}

VOID
KRangeLock::AcquireRange(
    __inout AcquireContext& Async,
    __in ULONGLONG Offset,
    __in ULONGLONG Length,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt const GUID* Id
    )

/*++

Routine Description:

    This routine acquires the given range.

Arguments:

    Async       - Supplies the context to use for acquiring the lock.

    Offset      - Supplies the offset of the range to be acquired.

    Length      - Supplies the length of the range to be acquired.

    Completion  - Supplies the completion that will be called once the lock is acquired.

    ParentAsync - Supplies, optionally, the parent async to synchronize the completion with.

    Id          - Supplies, optionally, an id that this range belongs to.

Return Value:

    None.

--*/

{
    if (Async.Status() != K_STATUS_NOT_STARTED) {
        Async.Reuse();
    }

    Async.InitializeForAcquire(*this, Id, Offset, Length);

    Async.Start(ParentAsync, Completion);
}

VOID
KRangeLock::ReleaseRange(
    __inout AcquireContext& Async
    )

/*++

Routine Description:

    This routine releases a previously acquired lock.

Arguments:

    Async   - Supplies the context that was used in the acquire.

Return Value:

    None.

--*/

{
    KFatal(Async._RangeLock == this);

    Async.ReleaseRange();
}

BOOLEAN
KRangeLock::AreLocksOutstanding(
    )

/*++

Routine Description:

    This routine returns whether or not any locks are currently being held and monitored by this object.

Arguments:

    None.

Return Value:

    FALSE   - No locks are being held.

    TRUE    - At least one lock is being held.

--*/

{
    BOOLEAN r;

    _SpinLock.Acquire();
    r = _LocksTable.Count() ? TRUE : FALSE;
    _SpinLock.Release();

    return r;
}

NTSTATUS
KRangeLock::Initialize(
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    status = AllocateAcquire(_SearchKey, AllocationTag);

    return status;
}

KRangeLock::AcquireContext::~AcquireContext(
    )
{
    KFatal(!_Waiters.Count());
    KFatal(!_InWaitList);
}

KRangeLock::AcquireContext::AcquireContext(
    ) : _Waiters(FIELD_OFFSET(AcquireContext, ListEntry))
{
    Initialize();
}

VOID
KRangeLock::AcquireContext::Initialize(
    )
{
    _RangeLock = NULL;
    RtlZeroMemory(&_Id, sizeof(GUID));
    _Offset = 0;
    _LengthDesired = 0;
    _LengthAcquired = 0;
}

VOID
KRangeLock::AcquireContext::InitializeForAcquire(
    __inout KRangeLock& RangeLock,
    __in_opt const GUID* Id,
    __in ULONGLONG Offset,
    __in ULONGLONG Length
    )
{
    _RangeLock = &RangeLock;
    if (Id) {
        _Id = *Id;
    } else {
        RtlZeroMemory(&_Id, sizeof(GUID));
    }
    _Offset = Offset;
    _LengthDesired = Length;
    _LengthAcquired = 0;
    KFatal(!_Waiters.Count());
    KFatal(!_InWaitList);
}

VOID
KRangeLock::AcquireContext::OnStart(
    )

/*++

Routine Description:

    This routine performs an acquire of the given lock, or queuing for eventually acquiring the lock.

Arguments:

    None.

Return Value:

    None.

--*/

{
    AcquireRange();
}

VOID
KRangeLock::AcquireContext::OnCancel(
    )

/*++

Routine Description:

    This routine handles a cancel.

Arguments:

    None.

Return Value:

    None.

--*/

{
    AcquireContext::SPtr context;

    _RangeLock->_SpinLock.Acquire();

    if (!_InWaitList) {
        _RangeLock->_SpinLock.Release();
        return;
    }

    _InWaitList->_Waiters.Remove(this);
    _InWaitList = NULL;

    if (_LengthAcquired) {
        _RangeLock->_LocksTable.Remove(*this);
    }

    _RangeLock->_SpinLock.Release();

    while (_Waiters.Count()) {
        context = _Waiters.RemoveHead();
        context->_InWaitList = NULL;
        context->AcquireRange();
    }

    Complete(STATUS_CANCELLED);
}

VOID
KRangeLock::AcquireContext::OnReuse(
    )
{
    KFatal(!_Waiters.Count());
    _RangeLock = NULL;
    _InWaitList = NULL;
}

LONG
KRangeLock::AcquireContext::Compare(
    __in AcquireContext& First,
    __in AcquireContext& Second
    )

/*++

Routine Description:

    This is a standard range compare routine where overlapping ranges are 'equal'.

Arguments:

    First   - Supplies the first element of the comparison.

    Second  - Supplies the second element of the comparison.

Return Value:

    <0  - The first element is less than the second element.

    0   - The elements are equal.

    >0  - The first element is greater than the second element.

--*/

{
    LONG r;

    r = memcmp(&First._Id, &Second._Id, sizeof(GUID));

    if (r) {
        return r;
    }

    if (First._Offset + First._LengthAcquired <= Second._Offset) {
        return -1;
    }

    if (Second._Offset + Second._LengthAcquired <= First._Offset) {
        return 1;
    }

    return 0;
}

VOID
KRangeLock::AcquireContext::AcquireRange(
    )

/*++

Routine Description:

    This routine will kick off the acquire the given range, leaving this async in a pending state until it is acquired.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONGLONG end = _Offset + _LengthDesired;
    AcquireContext::SPtr context;
    BOOLEAN b;

    //
    // Do an 'AddRef' here to keep the context alive until 'Release'.
    //

    _RangeLock->_SpinLock.Acquire();

    _RangeLock->_SearchKey->_Id = _Id;
    _RangeLock->_SearchKey->_Offset = _Offset + _LengthAcquired;
    _RangeLock->_SearchKey->_LengthAcquired = 1;

    //
    // Check to see if the range is available.
    //

    context = _RangeLock->_LocksTable.LookupEqualOrNext(*(_RangeLock->_SearchKey));

    if (!context || context->_Id != _Id || context->_Offset >= end) {

        //
        // In this case the entire range is free.
        //

        if (_LengthAcquired) {

            //
            // In this case the item is already in the table.  Just extend the range.
            //

            _LengthAcquired = _LengthDesired;

        } else {

            //
            // Insert into the table to claim the range.
            //

            _LengthAcquired = _LengthDesired;

            b = _RangeLock->_LocksTable.Insert(*this);
            KFatal(b);
        }

        _RangeLock->_SpinLock.Release();

        Complete(STATUS_SUCCESS);

        return;
    }

    if (context->_Offset > _Offset) {

        //
        // We have some initial part of the range, just not all of it.  Take what we can.
        //

        if (_LengthAcquired) {

            //
            // We get to lengthen the range that we have acquired even though the acquire is not yet complete.
            //

            _LengthAcquired = context->_Offset - _Offset;

        } else {

            //
            // Insert into the table to claim the range.
            //

            _LengthAcquired = context->_Offset - _Offset;

            b = _RangeLock->_LocksTable.Insert(*this);
            KFatal(b);
        }
    }

    //
    // Queue up on the range that we don't have.
    //

    KFatal(!_InWaitList);
    _InWaitList = context;
    context->_Waiters.AppendTail(this);

    _RangeLock->_SpinLock.Release();
}

VOID
KRangeLock::AcquireContext::ReleaseRange(
    )

/*++

Routine Description:

    This routine will release this previously acquired range.

Arguments:

    None.

Return Value:

    None.

--*/

{
    AcquireContext::SPtr context;

    _RangeLock->_SpinLock.Acquire();

    KFatal(_LengthAcquired == _LengthDesired);
    KFatal(!_InWaitList);

    //
    // Take this item out of the table.
    //

    _RangeLock->_LocksTable.Remove(*this);

    _RangeLock->_SpinLock.Release();

    //
    // Anything in the 'waiters' list can be retried.
    //

    while (_Waiters.Count()) {
        context = _Waiters.RemoveHead();
        context->_InWaitList = NULL;
        context->AcquireRange();
    }
}
