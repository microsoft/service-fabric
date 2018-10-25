/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KtlCoreAllocator.cpp

    Description:
        Types and definition published by the KtlSystemBase class

    History:

--*/

#include <ktl.h>

//** KtlCoreAllocator implementation
KtlCoreAllocator::KtlCoreAllocator(KtlSystemBase* const System)
    :   _System(reinterpret_cast<KtlSystem * const>(System)),
        _HeapEmptyEvent(nullptr),
        _IsLocked(FALSE)
{
	#if KTL_USER_MODE
		#if defined(TRACK_ALLOCATIONS)
			_TotalAllocations = 0;
		#endif
	#endif
}

KtlCoreAllocator::~KtlCoreAllocator()
{
    // Hold dtor until HeapWentEmptyNotification() is not in interlocked section
    _ThisLock.Acquire();
    {
        KAssert(_HeapEmptyEvent == nullptr);
    }
    _ThisLock.Release();
}

PVOID
KtlCoreAllocator::Alloc(__in size_t Size)
{
    return AllocWithTag(Size, KTL_TAG_BASE);
}

KtlSystem&
KtlCoreAllocator::GetKtlSystem()
{
    KInvariant(_System != nullptr);
    return *_System;
}

VOID
KtlCoreAllocator::SignalWhenEmpty(KEvent& HeapEmptyEvent)
{
    _ThisLock.Acquire();
    {
        KAssert(_HeapEmptyEvent == nullptr);
        _HeapEmptyEvent = &HeapEmptyEvent;
    }
    _ThisLock.Release();

    HeapWentEmptyNotification();
}

VOID
KtlCoreAllocator::HeapWentEmptyNotification()
{
    _ThisLock.Acquire();
    {
        if ((GetAllocsRemaining() == 0) && (_HeapEmptyEvent != nullptr))
        {
            // Heap is empty and notification and locking is being requested - signal empty
            // and lock the allocator
            _IsLocked = TRUE;
            _HeapEmptyEvent->SetEvent();
            _HeapEmptyEvent = nullptr;
        }
    }
    _ThisLock.Release();
}

VOID
KtlCoreAllocator::ClearHeapEmptyEvent()
{
    _ThisLock.Acquire();
    {
        _HeapEmptyEvent = nullptr;
        // NOTE: Leaves _IsLocked set
    }
    _ThisLock.Release();
}

BOOLEAN
KtlCoreAllocator::GetIsLocked()
{
    return _IsLocked;
}



//* page-aligned allocator implementation
KtlCorePageAlignedAllocator::KtlCorePageAlignedAllocator(KtlSystemBase* const System)
    :   KtlCoreAllocator(System),
        _AllocationCount(0)
{
}

KtlCorePageAlignedAllocator::~KtlCorePageAlignedAllocator()
{
}

PVOID
KtlCorePageAlignedAllocator::AllocWithTag(__in size_t Size, __in ULONG Tag)
{
    PVOID       result;

    #if KTL_USER_MODE
        UNREFERENCED_PARAMETER(Tag);
        result = VirtualAlloc(NULL, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #else
        result = ExAllocatePoolWithTag(NonPagedPool, Size < PAGE_SIZE ? PAGE_SIZE : Size, Tag);
    #endif

    if (result != nullptr)
    {
        InterlockedIncrement64(&_AllocationCount);
		#if KTL_USER_MODE
			#if defined(TRACK_ALLOCATIONS)
				InterlockedAdd64(&_TotalAllocations, Size);;
			#endif
		#endif
    }

    return result;
}

VOID
KtlCorePageAlignedAllocator::Free(__in PVOID Mem)
{
    if (Mem != nullptr)
    {
        BOOLEAN     heapWentEmpty;

        KInvariant(_AllocationCount > 0);
		heapWentEmpty = (InterlockedDecrement64(&_AllocationCount) == 0);

        #if KTL_USER_MODE
            VirtualFree(Mem, 0, MEM_RELEASE);
        #else
            ExFreePool(Mem);
        #endif

        if (heapWentEmpty)
        {
            // Report possible empty heap
            HeapWentEmptyNotification();
        }
    }
}

ULONGLONG
KtlCorePageAlignedAllocator::GetAllocsRemaining()
{
    return (ULONGLONG)_AllocationCount;
}

VOID
KtlCorePageAlignedAllocator::DebugDump()
{
}

//
// KInvariant hook support
//
BOOLEAN DefaultKInvariantCallout(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    )
{
    UNREFERENCED_PARAMETER(Condition);
    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Line);

    //
    // Default behavior is to assert or crash system
    //
    return(TRUE);
}

KInvariantCalloutType KInvariantCallout = DefaultKInvariantCallout;

KInvariantCalloutType SetKInvariantCallout(
    __in KInvariantCalloutType Callout
)
{
    KInvariantCalloutType previousCallout = (KInvariantCalloutType)InterlockedExchangePointer(
        (volatile PVOID*)&KInvariantCallout, Callout);

    return(previousCallout);
}
