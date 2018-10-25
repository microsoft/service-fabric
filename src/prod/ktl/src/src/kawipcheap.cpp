/*++

Module Name:

    KAWIpcHeap.cpp

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

ULONG KAWIpcKIoBufferElement::GetSharedMemoryOffset()
{
    PVOID myPointer;
    ULONG offset;

    myPointer = (PVOID)GetBuffer();
    offset = _Heap->GetSharedMemory()->PtrToOffset(myPointer);
    return(offset);
}

KAWIpcKIoBufferElement::KAWIpcKIoBufferElement()
{
    // Not used
    KInvariant(FALSE);
}

KAWIpcKIoBufferElement::~KAWIpcKIoBufferElement()
{
    LONG allocations;
    
    _Heap->Free(_Size, _Buffer);
    _Buffer = nullptr;
    
    allocations = InterlockedDecrement(&_Heap->_NumberAllocations);

    //
    // Release service activity taken when this KIoBufferElement was
    // created.
    //
    if (_ReleaseActivity)
    {
        _Heap->ReleaseServiceActivity();
    }
    
    _Heap = nullptr;
    
    _Size = 0;
}

VOID KAWIpcKIoBufferElement::OnDelete()
{
    KAWIpcPageAlignedHeap::SPtr heap = _Heap;
    
    this->~KAWIpcKIoBufferElement();

    //
    // Return element back to the free list
    //
    K_LOCK_BLOCK(heap->_Lock)
    {
        heap->_FreeIoBufferElements.InsertHead(this);      
    }    
}

KAWIpcKIoBufferElement::KAWIpcKIoBufferElement(
    __in KAWIpcPageAlignedHeap& Heap,
    __in PVOID Buffer,
    __in ULONG Size
    )
{    
    NTSTATUS status = STATUS_SUCCESS;
    
    _Heap = &Heap;

    InterlockedIncrement(&_Heap->_NumberAllocations);
    
    //
    // KIoBufferElement fields
    //
    _FreeBuffer = FALSE;
    _IsReference = FALSE;
    _Buffer = Buffer;
    _Size = Size;
#if defined(PLATFORM_UNIX)      
    _PageAccessChanged = FALSE;
#endif
    
    //
    // Acquire a service activity on the parent heap which will be
    // released when this KIoBufferElement is destructed. This will
    // ensure that the heap will not close before all KIoBufferElements
    // are freed.
    //
    _ReleaseActivity = _Heap->TryAcquireServiceActivity();
    if (! _ReleaseActivity)
    {
        status = K_STATUS_API_CLOSED;
    }

    // NOTE: This routine should never return STATUS_NO_MORE_ENTRIES

    SetConstructorStatus(status); 
}

NTSTATUS KAWIpcKIoBufferElement::Create(
    __in KAWIpcPageAlignedHeap& Heap,
    __in PVOID Buffer,
    __in ULONG Size,
    __out KIoBufferElement::SPtr& Context
)
{
    NTSTATUS status;
    KAWIpcKIoBufferElement* context = nullptr;
    KAWIpcPageAlignedHeap::SPtr heap = &Heap;

    K_LOCK_BLOCK(heap->_Lock)
    {
        context = heap->_FreeIoBufferElements.RemoveHead();
    }
    
    if (! context)
    {
        status = STATUS_NO_MORE_ENTRIES;
        KTraceFailedAsyncRequest(status, nullptr, Size, 0);

        //
        // There should always be enough preallocated for this to never
        // fail
        //
        KInvariant(FALSE);
        return(status);
    }

    context = new((PUCHAR)(context)) KAWIpcKIoBufferElement(Heap, Buffer, Size);
    status = context->Status();

    if (! NT_SUCCESS(status))
    {
        context->~KAWIpcKIoBufferElement();
        K_LOCK_BLOCK(heap->_Lock)
        {
            heap->_FreeIoBufferElements.InsertHead(context);      
        }
        
        KTraceFailedAsyncRequest(status, nullptr, Size, 0);
        return(status);
    }
    
    Context = context;
    return(status); 
}


KAWIpcPageAlignedHeap::KAWIpcPageAlignedHeap()
    : _FreeIoBufferElements(FIELD_OFFSET(KAWIpcKIoBufferElement, _FreeListEntry))
{
    // Not used
    KInvariant(FALSE);
}

KAWIpcPageAlignedHeap::~KAWIpcPageAlignedHeap()
{
    KAWIpcKIoBufferElement* element;
    PVOID ptr;

    KInvariant(_FreeIoBufferElements.Count() == _NumberFreeElements);

    //
    // Empty and free the elements from the list
    //
    do
    {
        element = _FreeIoBufferElements.RemoveHead();
        if (element != nullptr)
        {
            ptr = (PVOID)element;
            _delete(ptr);
        }       
    } while (element != nullptr);
}

KAWIpcPageAlignedHeap::KAWIpcPageAlignedHeap(
    __in KAWIpcSharedMemory& SharedMemory
    ) : _FreeIoBufferElements(FIELD_OFFSET(KAWIpcKIoBufferElement, _FreeListEntry))
{
    NTSTATUS status;

    _SharedMemory = &SharedMemory;

    //
    // Create the quota gate
    //
    status = KCoQuotaGate::Create(GetThisAllocationTag(), GetThisAllocator(), _QuotaGate);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    //
    // Create an allocation bitmap for the allocator to use
    //
    ULONG number4KBlocks = _SharedMemory->GetSize() / KAWIpcPageAlignedHeap::FourKB;
    status = KAllocationBitmap::Create(number4KBlocks, _Bitmap, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _Bitmap->SetAsFree(0, number4KBlocks);

    _MaximumAllocationPossible = _SharedMemory->GetSize();

    //
    // Create the list of preallocated KAWIpcKIoBufferElements
    //
    _NumberFreeElements = number4KBlocks;
    
    PVOID ptr;
    KAWIpcKIoBufferElement* element;
    for (ULONG i = 0; i < number4KBlocks; i++)
    {
        ptr = (PVOID)_newArray<UCHAR>(GetThisAllocationTag(), GetThisAllocator(), sizeof(KAWIpcKIoBufferElement));
        if (ptr == nullptr)
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        
        RtlZeroMemory(ptr, sizeof(KAWIpcKIoBufferElement));
        element =  (KAWIpcKIoBufferElement*)ptr;
        _FreeIoBufferElements.InsertHead(element);
    }
}

NTSTATUS KAWIpcPageAlignedHeap::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in KAWIpcSharedMemory& SharedMemory,
    __out KAWIpcPageAlignedHeap::SPtr& Context
)
{
    KAWIpcPageAlignedHeap::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcPageAlignedHeap(SharedMemory);
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

ktl::Awaitable<NTSTATUS> KAWIpcPageAlignedHeap::OpenAsync(
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

    co_return co_await *awaiter;
}

VOID KAWIpcPageAlignedHeap::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task KAWIpcPageAlignedHeap::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcPageAlignedHeap::SPtr thisPtr = this;

    _NumberAllocations = 0;
    
    status = co_await _QuotaGate->ActivateAsync(_MaximumAllocationPossible);
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcPageAlignedHeap::OpenTask", status,
                        (ULONGLONG)this,
                        0,
                        (ULONGLONG)0,
                        0); 

    CompleteOpen(status);
    
}

Task KAWIpcPageAlignedHeap::ShutdownQuotaGate()
{
    co_await _QuotaGate->DeactivateAsync();
}

ktl::Awaitable<NTSTATUS> KAWIpcPageAlignedHeap::CloseAsync()
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

VOID KAWIpcPageAlignedHeap::OnDeferredClosing()
{
    ShutdownQuotaGate();
}

VOID KAWIpcPageAlignedHeap::OnServiceClose()
{
    CloseTask();
}

Task KAWIpcPageAlignedHeap::CloseTask()
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // For safety, keep a ref on this for the lifetime of the whole
    // routine.
    //
    KAWIpcPageAlignedHeap::SPtr thisPtr = this;
    
    KDbgCheckpointWDataInformational(0,
                        "KAWIpcPageAlignedHeap::CloseTask", status,
                        (ULONGLONG)this,
                        0,
                        0,
                        0);


    CompleteClose(status);
    co_return;
}


ktl::Awaitable<NTSTATUS> KAWIpcPageAlignedHeap::AllocateAsync(
    __in ULONG Size,
    __out KIoBuffer::SPtr& IoBuffer
)
{
    KCoService$ApiEntry(TRUE);  
    
    NTSTATUS status;
    ULONG remainingQuanta;

    if (((Size % KAWIpcPageAlignedHeap::FourKB) != 0) ||
        ((Size > GetMaximumAllocationPossible()) ||
         (Size == 0)))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
        co_return status; 
    }           

    status = co_await _QuotaGate->StartAcquireAsync(Size, nullptr);

    if (NT_SUCCESS(status))
    {
        status = Allocate(Size, IoBuffer, remainingQuanta);
        if (! NT_SUCCESS(status))
        {
            //
            // QuotaGate thinks we have enough space but Allocate
            // failed and this is not expected.
            //
            KInvariant(status == K_STATUS_API_CLOSED);
            KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
            _QuotaGate->ReleaseQuanta(remainingQuanta);
        }
    } else {
        KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
    }
    
    co_return status;       
}

NTSTATUS KAWIpcPageAlignedHeap::Allocate(
    __in ULONG Size,
    __out KIoBuffer::SPtr& IoBuffer,
    __out ULONG& RemainingQuanta
)
{
    //
    // To keep things simple and the hopefully the heap from being too
    // fragmented, the allocation strategy relies upon the fact that a
    // KIoBuffer can be broken up into multiple discontiguous chunks.
    // Each allocation will be broken up into multiple 64K or 4K chunks
    //

    NTSTATUS status;
    BOOLEAN b = TRUE;
    PVOID startAddress;
    PVOID baseAddress;
    KIoBuffer::SPtr ioBuffer;
    KIoBufferElement::SPtr ioBufferElement;
    ULONG number64K, number4K;
    ULONG startBit = 0;

    KInvariant((Size % KAWIpcPageAlignedHeap::FourKB) == 0);
    
    RemainingQuanta = Size;
    
    if (Size > GetFreeAllocation())
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);        
        return(status);
    }
    
    number64K = Size / KAWIpcPageAlignedHeap::SixtyFourKB;
    number4K = (Size - (number64K * KAWIpcPageAlignedHeap::SixtyFourKB)) / KAWIpcPageAlignedHeap::FourKB;
    KInvariant( ((number64K * KAWIpcPageAlignedHeap::SixtyFourKB) + (number4K * KAWIpcPageAlignedHeap::FourKB)) == Size );

    status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, 0, 0);        
        return(status);
    }

    baseAddress = _SharedMemory->GetBaseAddress();
    for (ULONG i = 0; i < number64K; i++)
    {
        K_LOCK_BLOCK(_Lock)
        {
            b = _Bitmap->Allocate(KAWIpcPageAlignedHeap::SixtyFourKBInBits, startBit);
        }
        if (! b)
        {
            //
            // There are not enough contiguous bits to satisfy the allocation, we
            // need to retry with 4K allocs. Any memory or bits
            // previously allocated will be freed as the
            // KAWIpcKIoBufferElement destructors run as a result of
            // freeing the KIoBuffer here.
            //
            ioBuffer = nullptr;
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceFailedAsyncRequest(status, this, Size, KAWIpcPageAlignedHeap::SixtyFourKBInBits);

            //
            // Next Try to allocate all in 4K elements in case of fragmentation.
            //
            status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(0, status, nullptr, 0, 0);        
                return(status);
            }

            number4K =  Size / KAWIpcPageAlignedHeap::FourKB;
            break;
        }

        startAddress = (PUCHAR)baseAddress + (startBit * KAWIpcPageAlignedHeap::FourKB);
        status = KAWIpcKIoBufferElement::Create(*this,
                                                startAddress,
                                                KAWIpcPageAlignedHeap::SixtyFourKB,
                                                ioBufferElement);
        if (! NT_SUCCESS(status))
        {
            if (status == STATUS_NO_MORE_ENTRIES)
            {
                //
                // Return bits back to bitmap only if memory allocation
                // failed
                //
                K_LOCK_BLOCK(_Lock)
                {
                    _Bitmap->Free(startBit, KAWIpcPageAlignedHeap::SixtyFourKBInBits);
                }
            } else {
                //
                // Otherwise the KAWIpcKIoBufferElement destructor has
                // been called and so the bits were freed and the
                // quanta returned
                //
                RemainingQuanta -= KAWIpcPageAlignedHeap::SixtyFourKB;
            }
            
            KTraceFailedAsyncRequest(status, this, Size, KAWIpcPageAlignedHeap::SixtyFourKBInBits);
            return(status);         
        }

        ioBuffer->AddIoBufferElement(*ioBufferElement);
    }

    for (ULONG i = 0; i < number4K; i++)
    {
        K_LOCK_BLOCK(_Lock)
        {
            b = _Bitmap->Allocate(KAWIpcPageAlignedHeap::FourKBInBits, startBit);
        }
        if (! b)
        {
            //
            // There are not enough bits to satisfy the allocation, we
            // need to return out of memory. Any memory or bits
            // previously allocated will be freed as the
            // KAWIpcKIoBufferElement destructors run
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceFailedAsyncRequest(status, this, Size, KAWIpcPageAlignedHeap::FourKBInBits);
            return(status);
        }

        startAddress = (PUCHAR)baseAddress + (startBit * KAWIpcPageAlignedHeap::FourKB);
        status = KAWIpcKIoBufferElement::Create(*this,
                                                startAddress,
                                                KAWIpcPageAlignedHeap::FourKB,
                                                ioBufferElement);
        if (! NT_SUCCESS(status))
        {
            if (status == STATUS_NO_MORE_ENTRIES)
            {
                //
                // Return bits back to bitmap only if memory allocation
                // failed
                //
                K_LOCK_BLOCK(_Lock)
                {
                    _Bitmap->Free(startBit, KAWIpcPageAlignedHeap::FourKBInBits);
                }
            } else {
                //
                // Otherwise the KAWIpcKIoBufferElement destructor has
                // been called and so the bits were freed and the
                // quanta returned
                //
                RemainingQuanta -= KAWIpcPageAlignedHeap::FourKB;
            }
            
            KTraceFailedAsyncRequest(status, this, Size, KAWIpcPageAlignedHeap::SixtyFourKBInBits);
            return(status);         
        }

        ioBuffer->AddIoBufferElement(*ioBufferElement);
    }
    
    IoBuffer = Ktl::Move(ioBuffer);
    return(STATUS_SUCCESS);
}


ktl::Awaitable<NTSTATUS> KAWIpcPageAlignedHeap::AllocateAsync(
    __in ULONG Size,
    __out KIoBufferElement::SPtr& IoBufferElement
)
{
    KCoService$ApiEntry(TRUE);  
    
    NTSTATUS status;
    ULONG remainingQuanta;

    if ( ((Size % KAWIpcPageAlignedHeap::FourKB) != 0) ||
         (Size > GetMaximumAllocationPossible()) ||
         (Size == 0))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
        co_return status; 
    }           

    status = co_await _QuotaGate->StartAcquireAsync(Size, nullptr);

    if (NT_SUCCESS(status))
    {
        status = Allocate(Size, IoBufferElement, remainingQuanta);
        if (! NT_SUCCESS(status))
        {
            //
            // QuotaGate thinks we have enough space but Allocate
            // failed and this is not expected.
            //
            KInvariant(status == K_STATUS_API_CLOSED);
            KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
            _QuotaGate->ReleaseQuanta(remainingQuanta);
        }
    } else {
        KTraceFailedAsyncRequest(status, this, Size, GetMaximumAllocationPossible());
    }
    
    co_return status;       
}

NTSTATUS KAWIpcPageAlignedHeap::Allocate(
    __in ULONG Size,
    __out KIoBufferElement::SPtr& IoBufferElement,
    __out ULONG& RemainingQuanta
)
{
    NTSTATUS status;
    BOOLEAN b = TRUE;
    PVOID startAddress;
    PVOID baseAddress;
    KIoBufferElement::SPtr ioBufferElement;
    ULONG startBit = 0;
    ULONG number4K = Size / KAWIpcPageAlignedHeap::FourKB;

    KInvariant((Size % KAWIpcPageAlignedHeap::FourKB) == 0);
    
    RemainingQuanta = Size;
    
    if (Size > GetFreeAllocation())
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);        
        return(status);
    }
    
    baseAddress = _SharedMemory->GetBaseAddress();

    K_LOCK_BLOCK(_Lock)
    {
        b = _Bitmap->Allocate(number4K, startBit);
    }
    if (! b)
    {
        //
        // There are not enough contiguous bits to satisfy the
        // allocation.
        //
        // TODO: Can this occur when the heap is too fragmented ?
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, this, Size, number4K);
        return(status);
    }

    startAddress = (PUCHAR)baseAddress + (startBit * KAWIpcPageAlignedHeap::FourKB);
    status = KAWIpcKIoBufferElement::Create(*this,
                                            startAddress,
                                            Size,
                                            ioBufferElement);
    if (! NT_SUCCESS(status))
    {
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            //
            // Return bits back to bitmap only if memory allocation
            // failed
            //
            K_LOCK_BLOCK(_Lock)
            {
                _Bitmap->Free(startBit, Size);
            }
        } else {
            //
            // Otherwise the KAWIpcKIoBufferElement destructor has
            // been called and so the bits were freed and the
            // quanta returned
            //
            RemainingQuanta -= Size;
        }

        KTraceFailedAsyncRequest(status, this, Size, Size);
        return(status);         
    }
    
    IoBufferElement = Ktl::Move(ioBufferElement);
    return(STATUS_SUCCESS);
}


VOID KAWIpcPageAlignedHeap::Free(
    __in ULONG Size,
    __in PVOID AllocationPtr
  )
{
    ULONG offset;
    PVOID baseAddress = _SharedMemory->GetBaseAddress();

    KInvariant((ULONG_PTR)(AllocationPtr) >= (ULONG_PTR)(baseAddress));

    //
    // Return bits back to the bitmap to indicate free memory
    //
    offset = (ULONG)((ULONG_PTR)AllocationPtr - (ULONG_PTR)baseAddress);
    K_LOCK_BLOCK(_Lock)
    {
        _Bitmap->Free(offset / KAWIpcPageAlignedHeap::FourKB,
                      Size / KAWIpcPageAlignedHeap::FourKB);
    }

    _QuotaGate->ReleaseQuanta(Size);
}           

ULONG KAWIpcPageAlignedHeap::GetFreeAllocation()
{
    ULONG freeAllocation = 0;
    
    K_LOCK_BLOCK(_Lock)
    {
        freeAllocation = _Bitmap->QueryNumFreeBits() * FourKB;
    }
    return(freeAllocation);
};

