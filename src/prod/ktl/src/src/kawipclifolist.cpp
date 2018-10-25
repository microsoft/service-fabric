/*++

Module Name:

    KAWIpcLIFOList.cpp

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

KAWIpcLIFOList::KAWIpcLIFOList()
{
}

KAWIpcLIFOList::~KAWIpcLIFOList()
{
    if (_IsCreator)
    {
        KInvariant(*_OnListCountPtr == (LONG)((_Count64K + _Count4096 + _Count1024 + _Count64)));
    }
}

KAWIpcLIFOList::KAWIpcLIFOList(
    __in KAWIpcSharedMemory& SharedMemory,
    __in BOOLEAN Initialize
    )
{   
    _SharedMemory = &SharedMemory;
    _IsCreator = Initialize;

    ListHeads* listHeads;
    listHeads = (ListHeads*)_SharedMemory->OffsetToPtr(0);
    
    if (Initialize)
    {
        //
        // Compute the number of messages for the different sizes
        //
        ULONG totalSize;
        ULONG sizeLeft;
        ULONG count64K;
        ULONG count4096;
        ULONG count1024;
        ULONG count64;
        ULONG offset;
        KAWIpcMessage* message;
        RtlZeroMemory(_SharedMemory->GetBaseAddress(), _SharedMemory->GetSize());

        totalSize = _SharedMemory->GetSize();
        sizeLeft = totalSize - sizeof(ListHeads);
        
        if (sizeLeft > OneMb)
        {
            count64K = 4;
        } else {
            count64K = 2;
        }
        KAssert(sizeLeft >= (count64K * SixtyFourKb));
        sizeLeft -= count64K * SixtyFourKb;

        count4096 = (sizeLeft / 4) / FourKb;
        KAssert(sizeLeft >= (count4096 * FourKb));
        sizeLeft -= count4096 * FourKb;
                   
        count1024 = (sizeLeft / 2) / OneKb;
        KAssert(sizeLeft >= (count1024 * OneKb));
        sizeLeft -= count1024 * OneKb;
        
        KAssert(sizeLeft >= SixtyFour);
        count64 = sizeLeft / SixtyFour;

        //
        // Build lists of messages
        //
        offset = sizeof(ListHeads);

        _OnListCountPtr = (volatile LONG*)(&listHeads->OnListCount);
        *_OnListCountPtr = 0;
        
        listHeads->Offset64K = offset;
        _ListHead64K = (volatile ULONG*)(&listHeads->Head64K.Head);
        _Count64K = count64K;
        for (ULONG i = 0; i < count64K; i++)
        {
            message = (KAWIpcMessage*)_SharedMemory->OffsetToPtr(offset);
            message->SizeIncludingHeader = SixtyFourKb;
            Add(*message);
            offset += SixtyFourKb;
        }
        
        listHeads->Offset4096 = offset;
        _ListHead4096 = (volatile ULONG*)(&listHeads->Head4096.Head);
        _Count4096 = count4096;
        for (ULONG i = 0; i < count4096; i++)
        {
            message = (KAWIpcMessage*)_SharedMemory->OffsetToPtr(offset);
            message->SizeIncludingHeader = FourKb;
            Add(*message);
            offset += FourKb;
        }
                
        listHeads->Offset1024 = offset;
        _ListHead1024 = (volatile ULONG*)(&listHeads->Head1024.Head);
        _Count1024 = count1024;
        for (ULONG i = 0; i < count1024; i++)
        {
            message = (KAWIpcMessage*)_SharedMemory->OffsetToPtr(offset);
            message->SizeIncludingHeader = OneKb;
            Add(*message);
            offset += OneKb;
        }

        listHeads->Offset64 = offset;
        _ListHead64 = (volatile ULONG*)(&listHeads->Head64.Head);
        _Count64 = count64;        
        for (ULONG i = 0; i < count64; i++)
        {
            message = (KAWIpcMessage*)_SharedMemory->OffsetToPtr(offset);
            message->SizeIncludingHeader = SixtyFour;
            Add(*message);
            offset += SixtyFour;
        }               
    }
    else 
    {
        _ListHead64 = (volatile ULONG*)(&listHeads->Head64);
        _ListHead1024 = (volatile ULONG*)(&listHeads->Head1024);
        _ListHead4096 = (volatile ULONG*)(&listHeads->Head4096);
        _ListHead64K = (volatile ULONG*)(&listHeads->Head64K);
        _OnListCountPtr = (volatile LONG*)(&listHeads->OnListCount);
    }        
}

   
NTSTATUS KAWIpcLIFOList::CreateAndInitialize(
    __in KAWIpcSharedMemory& SharedMemory,
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcLIFOList::SPtr& Context
)
{
    KAWIpcLIFOList::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcLIFOList(SharedMemory, TRUE);
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

NTSTATUS KAWIpcLIFOList::Create(
    __in KAWIpcSharedMemory& SharedMemory,
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcLIFOList::SPtr& Context
)
{
    KAWIpcLIFOList::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcLIFOList(SharedMemory, FALSE);
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

KAWIpcMessage* KAWIpcLIFOList::Remove(
    __in ULONG Size
    )
{
    volatile ULONG* listHead = nullptr;
    ULONG oldHead;
    ULONG newHead;
    ULONG result;
    ULONG entry;
    KAWIpcMessage* entryPtr;

    if (Size <= SixtyFour)
    {
        listHead = _ListHead64;
    } else if (Size <= OneKb) {
        listHead = _ListHead1024;       
    } else if (Size <= FourKb) {
        listHead = _ListHead4096;
    } else if (Size <= SixtyFourKb) {
        listHead = _ListHead64K;
    } else {
        KInvariant(FALSE);    // Allocation greater than max
    }
    
    static const ULONGLONG lockAbortThreshold = (60 * 1000);   // one minute
    ULONGLONG startTicks = GetTickCount64();

retry:  
    oldHead = *listHead;
    if (oldHead == 0)
    {
        return nullptr;   // List empty
    }
    
    entry = oldHead;
    entryPtr = (KAWIpcMessage*)_SharedMemory->OffsetToPtr(entry);
    newHead = entryPtr->Next;
    
    result = (ULONG)InterlockedCompareExchange((volatile LONG*)listHead, (LONG)newHead, (LONG)oldHead);
    if (result != oldHead)
    {
        if ((GetTickCount64() - startTicks) > lockAbortThreshold)
        {
            //
            // If a lock cannot be acquired within a minute then
            // something is wrong and so we need to just failfast
            //
            KTraceFailedAsyncRequest(STATUS_UNSUCCESSFUL, nullptr, (ULONGLONG)this, (ULONGLONG)listHead);
            KInvariant(FALSE);
        }
        
        KNt::Sleep(0);
        goto retry;
    }
    entryPtr->Next = 0;
    InterlockedDecrement(_OnListCountPtr);
    
    return(entryPtr);
}

VOID KAWIpcLIFOList::Add(
    __in KAWIpcMessage& Message
    )
{
    volatile ULONG* listHead = nullptr;
    ULONG oldHead;
    ULONG newHead;
    ULONG result;
    ULONG size;
    ULONG entryOffset;
    KAWIpcMessage* message = &Message;

    KInvariant(message->Next == 0);
    
    size = message->SizeIncludingHeader;

    switch(size)
    {
        case SixtyFour:
        {
            listHead = _ListHead64;
            break;
        }
        
        case OneKb:
        {
            listHead = _ListHead1024;
            break;
        }
        
        case FourKb:
        {
            listHead = _ListHead4096;
            break;
        }
        
        case SixtyFourKb:
        {
            listHead = _ListHead64K;
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }
        
    static const ULONGLONG lockAbortThreshold = (60 * 1000);   // one minute
    ULONGLONG startTicks = GetTickCount64();

retry:
    entryOffset =_SharedMemory->PtrToOffset(message);
    oldHead = *listHead;
    message->Next = oldHead;
    newHead = entryOffset;
    result = (ULONG)InterlockedCompareExchange((volatile LONG*)listHead, (LONG)newHead, (LONG)oldHead);
    
    if (result != oldHead)
    {
        if ((GetTickCount64() - startTicks) > lockAbortThreshold)
        {
            //
            // If a lock cannot be acquired within a minute then
            // something is wrong and so we need to just failfast
            //
            KTraceFailedAsyncRequest(STATUS_UNSUCCESSFUL, nullptr, (ULONGLONG)this, (ULONGLONG)listHead);
            KInvariant(FALSE);
        }
        
        KNt::Sleep(0);
        goto retry;
    }
    InterlockedIncrement(_OnListCountPtr);
}


KAWIpcMessageAllocator::KAWIpcMessageAllocator()
{
}

KAWIpcMessageAllocator::~KAWIpcMessageAllocator()
{
}

NTSTATUS KAWIpcMessageAllocator::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out KAWIpcMessageAllocator::SPtr& Context
    )
{
    KAWIpcMessageAllocator::SPtr context;
    
    context = _new(AllocatorTag, Allocator) KAWIpcMessageAllocator();
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

ktl::Awaitable<NTSTATUS> KAWIpcMessageAllocator::OpenAsync(
    __in KAWIpcLIFOList& Lifo,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    NTSTATUS status;
    
    OpenAwaiter::SPtr awaiter;
    status = OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        ParentAsync,
        GlobalContextOverride);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "OpenAwaiter::Create", status);
        co_return status;
    }

    _Lifo = &Lifo;

    co_return co_await *awaiter;
}

VOID KAWIpcMessageAllocator::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcMessageAllocator::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcMessageAllocator::SPtr thisPtr = this;

    _IsCancelled = FALSE;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcMessageAllocator::OpenTask", status,
                        (ULONGLONG)this,
                        (ULONGLONG)_Lifo.RawPtr(),
                        (ULONGLONG)0,
                        (ULONGLONG)0);

    CompleteOpen(status);   
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcMessageAllocator::CloseAsync()
{
    NTSTATUS status;
    
    CloseAwaiter::SPtr awaiter;
    status = CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "CloseAwaiter::Create", status);
        co_return status;
    }

    co_return co_await *awaiter;
}

VOID KAWIpcMessageAllocator::OnDeferredClosing()
{
    //
    // Cancel any waiting operations
    //
    _IsCancelled = TRUE;
}

VOID KAWIpcMessageAllocator::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcMessageAllocator::CloseTask()
{
    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcMessageAllocator::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcMessageAllocator::CloseTask", STATUS_SUCCESS,
                        (ULONGLONG)this,
                        (ULONGLONG)_Lifo.RawPtr(),
                        (ULONGLONG)0,
                        (ULONGLONG)0);
    
    CompleteClose(STATUS_SUCCESS);
    co_return;
}

ktl::Awaitable<NTSTATUS> KAWIpcMessageAllocator::AllocateAsync(
    __in ULONG Size,
    __out KAWIpcMessage*& Message
    )
{
    KCoService$ApiEntry(TRUE);

    KAWIpcMessageAllocator::SPtr thisPtr = this;
    static const ULONG startThrottleDelayInMs = 1;
    static const ULONG maxThrottleDelayInMs = 256;
    ULONG throttleDelayInMs = startThrottleDelayInMs;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS statusDontCare;
        
    Message = nullptr;
    do
    {
        Message = _Lifo->Remove(Size);
        if (Message == nullptr)
        {
            //
            // Free list is empty so we need to first check if the LIFO is
            // shutting down and wait for messages to be
            // freed. We do not have a notification for this; Consider adding
            // one. For now, just wait a ms and try again. Note that
            // this is a natural throttle point
            //
            if (_IsCancelled)
            {
                status = STATUS_CANCELLED;
                break;
            }

            statusDontCare = co_await KTimer::StartTimerAsync(GetThisAllocator(), KTL_TAG_TEST, throttleDelayInMs, nullptr, nullptr);
            throttleDelayInMs = throttleDelayInMs * 2;
            if (throttleDelayInMs > maxThrottleDelayInMs)
            {
                throttleDelayInMs = maxThrottleDelayInMs;
            }
        }       
    } while (Message == nullptr);
    
    co_return(status);
}

VOID KAWIpcMessageAllocator::Free(
    __in KAWIpcMessage* Message
    )
{
    _Lifo->Add(*Message);
}