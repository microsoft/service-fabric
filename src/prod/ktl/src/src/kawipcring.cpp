/*++

Module Name:

    KAWIpcRing.cpp

Abstract:

    This file contains the implementation for the KAWIpc mechanism.
    This is a cross process IPC mechanism.

Author:

    Alan Warwick 05/2/2017

Environment:

    User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>
#include <kawipc.h>

#if defined(PLATFORM_UNIX)
#include <palio.h>
#endif

KAWIpcRing::KAWIpcRing()
{
    KInvariant(FALSE);
}

KAWIpcRing::~KAWIpcRing()
{
    if (_IsCreator)
    {
        KInvariant((_Ring->Lock == 0) || (_Ring->Lock == _ClosedOwnerId));
        KInvariant(IsEmpty());
    }
}

KAWIpcRing::KAWIpcRing(
    __in LONG OwnerId,
    __in KAWIpcSharedMemory& RingSharedMemory,
    __in KAWIpcSharedMemory& MessageSharedMemory,
    __in BOOLEAN Initialize
    )
{
    _OwnerId = OwnerId;
    _RingSharedMemory = &RingSharedMemory;
    _MessageSharedMemory = &MessageSharedMemory;
    _IsCreator = Initialize;

    ULONG ringOffset = 0;   // Ring header is always at the front
    _Ring = (RingHead*)_RingSharedMemory->OffsetToPtr(ringOffset);
    if (Initialize)
    {
        RtlZeroMemory(_RingSharedMemory->GetBaseAddress(), _RingSharedMemory->GetSize());
        ULONG ringSize = ((_RingSharedMemory->GetSize() - FIELD_OFFSET(RingHead, Data)) / sizeof(ULONG))-1;
        _Ring->PC.a.P = 0;
        _Ring->PC.a.C = ringSize;
        _Ring->Lock = 0;
        _Ring->NumberEntries = ringSize;
    }
    
    _NumberEntries = _Ring->NumberEntries;
}

NTSTATUS KAWIpcRing::CreateAndInitialize(
    __in LONG OwnerId,
    __in KAWIpcSharedMemory& RingSharedMemory,
    __in KAWIpcSharedMemory& MessageSharedMemory,
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcRing::SPtr& Context
    )
{
    KAWIpcRing::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcRing(OwnerId, RingSharedMemory, MessageSharedMemory, TRUE);
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

NTSTATUS KAWIpcRing::Create(
    __in LONG OwnerId,
    __in KAWIpcSharedMemory& RingSharedMemory,
    __in KAWIpcSharedMemory& MessageSharedMemory,
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcRing::SPtr& Context
    )
{
    KAWIpcRing::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcRing(OwnerId, RingSharedMemory, MessageSharedMemory, FALSE);
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

// STATUS_ARRAY_BOUNDS_EXCEEDED
// STATUS_IMPLEMENTATION_LIMIT
// STATUS_RING_NEWLY_EMPTY

NTSTATUS KAWIpcRing::Enqueue(
    __in KAWIpcMessage* Message,
    __out BOOLEAN& WasEmpty
    )
{
    NTSTATUS status;
    ULONG messageOffset;
    PANDC oldPC;
    ULONG oldP;
    ULONG newP;

    KInvariant(Message->Next == 0);

    if (IsLockedForClose())
    {
        return(STATUS_FILE_FORCED_CLOSED);
    }
        
    messageOffset = _MessageSharedMemory->PtrToOffset(Message);
    
    status = Lock();
    if (! NT_SUCCESS(status))
    {
        // NOTE: Should only ever return here in unit test
        return(status);
    }
    KFinally([&](){ Unlock(); });
    
    oldPC.PAndC = _Ring->PC.PAndC;
    if (oldPC.a.P == oldPC.a.C)
    {
        return(STATUS_ARRAY_BOUNDS_EXCEEDED);   // Full
    }

    // TODO: Consider usefulness of these error codes
    // STATUS_RING_PREVIOUSLY_EMPTY
    if ((IsEmpty(oldPC)) && (_Ring->WaitingForEvent == 1))
    {
        WasEmpty = TRUE;
        _Ring->WaitingForEvent = 0;
    } else {
        WasEmpty = FALSE;
    }

    Message->Next = 1;
    Message->OwnerId = _OwnerId;
    
    oldP = oldPC.a.P;
    newP = Advance(oldP);
	KInvariant(messageOffset != 1);
    _Ring->Data[oldP] = messageOffset;
    _Ring->PC.a.P = newP;

	KDbgCheckpointWDataInformational(0,
						"Enqueue", STATUS_SUCCESS,
						(ULONGLONG)this,
						(ULONGLONG)Message,
						(ULONGLONG)oldP,
						(ULONGLONG)_Ring->PC.PAndC);	
	
    return(STATUS_SUCCESS); 
}

NTSTATUS KAWIpcRing::Dequeue(
    __out KAWIpcMessage*& Message
    )
{
    NTSTATUS status;
    ULONG messageOffset;
    PANDC oldPC;
    ULONG oldC;
    ULONG newC;

    Message = nullptr;
    {        
        status = Lock();
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
        KFinally([&](){ Unlock(); });

        oldPC.PAndC = _Ring->PC.PAndC;

        if (IsEmpty(oldPC))
        {
            _Ring->WaitingForEvent = 1;
            return(STATUS_NOT_FOUND);
        }

        oldC = oldPC.a.C;
        newC = Advance(oldC);
        messageOffset = _Ring->Data[newC];
        _Ring->PC.a.C = newC;
    }
 
    Message = (KAWIpcMessage*)_MessageSharedMemory->OffsetToPtr(messageOffset);
    KInvariant(Message->Next == 1);
    Message->Next = 0;
    Message->OwnerId = 0;

	KDbgCheckpointWDataInformational(0,
						"Dequeue", STATUS_SUCCESS,
						(ULONGLONG)this,
						(ULONGLONG)Message,
						(ULONGLONG)newC,
						(ULONGLONG)_Ring->PC.PAndC);	
	
	
    return(STATUS_SUCCESS);
}

NTSTATUS KAWIpcRing::LockForClose(
    )
{
    NTSTATUS status;
    LONG result;

    result = InterlockedCompareExchange(&_Ring->LockedForClose, _OwnerId, 0);
    status = (result != 0)  ? STATUS_FILE_FORCED_CLOSED : STATUS_SUCCESS;
    return(status);
}

NTSTATUS KAWIpcRing::LockInternal(
    __in LONG OwnerId
    )
{
    NTSTATUS status;
    static const ULONGLONG lockAbortThreshold = (60 * 1000);   // one minute
    ULONGLONG startTicks = GetTickCount64();
    LONG result;

    do
    {
        result = InterlockedCompareExchange(&_Ring->Lock, OwnerId, 0);
        if (result != 0) 
        {
            if ((GetTickCount64() - startTicks) > lockAbortThreshold)
            {
                //
                // If a lock cannot be acquired within a minute then
                // something is wrong and so we need to just failfast
                //
                status = STATUS_INVALID_LOCK_SEQUENCE;
                KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)_Ring);
                KInvariant(FALSE);
                return(status);
            }

            //
            // See if we can give the owner a chance to let go of the
            // lock
            //
            KNt::Sleep(0);
        }
    } while (result != 0);
    
    return(STATUS_SUCCESS);
}


VOID KAWIpcRing::BreakOwner(
    __in LONG OwnerId
)
{
    if (_Ring->Lock == OwnerId)
    {
        KTraceFailedAsyncRequest(STATUS_UNSUCCESSFUL, nullptr, (ULONGLONG)this, (ULONGLONG)OwnerId);      
        _Ring->Lock = 0;
    }
}
