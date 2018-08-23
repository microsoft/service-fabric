/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KtlCoreAllocator.h

    Description:
        Types and definition published by the KtlSystemBase class

    History:

--*/

#pragma once

#if DBG
#ifndef TRACK_ALLOCATIONS
#define TRACK_ALLOCATIONS
#endif
#endif

#include <ktl.h>
#if !defined(KTL_CORE_LIB)
#include <ktrace.h>
#endif

class KtlSystemBase;
class KtlCoreAllocator : public KAllocator
{
public:
    KtlCoreAllocator(KtlSystemBase* const System);
    virtual ~KtlCoreAllocator();

    PVOID
    Alloc(__in size_t Size);

    KtlSystem&
    GetKtlSystem();

	#if KTL_USER_MODE
		#if DBG
			ULONGLONG 
			GetTotalAllocations() override { return (ULONGLONG)_TotalAllocations; }
		#endif
	#endif

protected:
    VOID
    HeapWentEmptyNotification();

protected:
    KSpinLock           _ThisLock;
    BOOLEAN             _IsLocked;      // TRUE - means fail any further allocations
	#if KTL_USER_MODE
		#if defined(TRACK_ALLOCATIONS)
			volatile LONGLONG	_TotalAllocations;
		#endif
	#endif

private:
    // API for KtlSystemBase
    friend class KtlSystemBase;

    VOID
    SignalWhenEmpty(KEvent& HeapEmptyEvent);

    VOID
    ClearHeapEmptyEvent();

    BOOLEAN
    GetIsLocked();

    virtual VOID
    DebugDump() = 0;

private:
    KtlSystem* const    _System;
    KEvent*             _HeapEmptyEvent;
};

//* Non-page aligned allocator (paged and non-paged)

template <POOL_TYPE PoolType>
class KtlCoreNonPageAlignedAllocator : public KtlCoreAllocator
{
public:
    KtlCoreNonPageAlignedAllocator(
		#if KTL_USER_MODE
        __in HANDLE HeapHandle,
		#endif
        __in KtlSystemBase* const System);

    virtual ~KtlCoreNonPageAlignedAllocator();

    PVOID
    AllocWithTag(__in size_t Size, __in ULONG Tag);

    VOID
    Free(__in PVOID Mem);

    ULONGLONG
    GetAllocsRemaining();

    #if KTL_USER_MODE
        void SetEnableAllocationCounters(bool ToEnabled)
        {
            _AllocationCountsEnabled = ToEnabled;
        }
    #endif


protected:
    VOID
    DebugDump();

protected:
    #pragma warning(push)
    #pragma warning(disable : 4200)
    struct Preamble
    {
#if KTL_USER_MODE
#if defined(TRACK_ALLOCATIONS)
        KListEntry          ListEntry;
#else
        UCHAR               Unused[sizeof(KListEntry)];
#endif
#else
        KListEntry          ListEntry;
#endif
        UCHAR               AppMemBlk[];
    };
    static_assert((sizeof(Preamble) % (sizeof(void*) * 2)) == 0, "Preamble size not correct");
    #pragma warning(pop)

#if KTL_USER_MODE
    HANDLE                  _HeapHandle;
    bool                    _AllocationCountsEnabled = true;

#if defined(TRACK_ALLOCATIONS)
    KNodeList<Preamble>     _AllocatedEntries;
#else
    static const ULONG      _AllocationCountsSize = 64;
    __declspec(align(16)) LONG volatile _AllocationCounts[_AllocationCountsSize];
#endif

#else
    KNodeList<Preamble>     _AllocatedEntries;
#endif

};

//* Non-page aligned allocator implementation
#if KTL_USER_MODE
template <POOL_TYPE PoolType>
KtlCoreNonPageAlignedAllocator<PoolType>::KtlCoreNonPageAlignedAllocator(HANDLE HeapHandle, KtlSystemBase* const System)
    :   KtlCoreAllocator(System),
#if defined(TRACK_ALLOCATIONS)
        _HeapHandle(HeapHandle),
        _AllocatedEntries(FIELD_OFFSET(Preamble, ListEntry))
{
}
#else
        _HeapHandle(HeapHandle)
{
    RtlZeroMemory((void*)(&_AllocationCounts[0]), sizeof(ULONG) * _AllocationCountsSize);
}
#endif

#else
template <POOL_TYPE PoolType>
KtlCoreNonPageAlignedAllocator<PoolType>::KtlCoreNonPageAlignedAllocator(KtlSystemBase* const System)
    :   KtlCoreAllocator(System),
        _AllocatedEntries(FIELD_OFFSET(Preamble, ListEntry))
{
}
#endif

template <POOL_TYPE PoolType>
KtlCoreNonPageAlignedAllocator<PoolType>::~KtlCoreNonPageAlignedAllocator()
{
}

#if KTL_USER_MODE
template <POOL_TYPE PoolType>
__forceinline PVOID
KtlCoreNonPageAlignedAllocator<PoolType>::AllocWithTag(__in size_t Size, __in ULONG Tag)
{
    PVOID result;
#if defined(TRACK_ALLOCATIONS)
    size_t actualSize;

    HRESULT hr = SizeTAdd(Size, sizeof(Preamble), &actualSize);
    KInvariant(SUCCEEDED(hr));

    result = ExAllocatePoolWithTag(_HeapHandle, PoolType, actualSize, Tag);
#else
    result = ExAllocatePoolWithTag(_HeapHandle, PoolType, Size, Tag);
#endif

    if (result != nullptr)
    {
#if defined(TRACK_ALLOCATIONS)
        Preamble* preamble = reinterpret_cast<Preamble*>(result);
        #if !defined(PLATFORM_UNIX)
        preamble->Preamble::Preamble();
        #else
        *preamble = {};
        #endif
#endif
        
        if (_IsLocked)
        {
            // Allocator is locked - release the memory and return allocation failure
            ExFreePool(_HeapHandle, result);
            return nullptr;
        }
        else
        {
#if defined(TRACK_ALLOCATIONS)
            _ThisLock.Acquire();
            {
                _AllocatedEntries.AppendTail(preamble);
                _TotalAllocations += Size;
            }
            _ThisLock.Release();
            return &preamble->AppMemBlk[0];
#else
            if (_AllocationCountsEnabled)
            {
                // NOTE: The low order 4 bits of heap allocated objects are on 16 bytes boundaries (hence the >> 4) and so the most random
                //       bits to be usd as a counter index start at the 5th bit.

				InterlockedIncrement(&_AllocationCounts[(((ULONGLONG)result) >> 4) & (_AllocationCountsSize - 1)]);
            }
#endif
        }
    }
    return result;
}

template <POOL_TYPE PoolType>
VOID
__forceinline KtlCoreNonPageAlignedAllocator<PoolType>::Free(__in PVOID Mem)
{
    if (Mem != nullptr)
    {
        bool     heapMaybeWentEmpty = false;

        #if defined(TRACK_ALLOCATIONS)
            Preamble*   preamble = reinterpret_cast<Preamble*>(((UCHAR*)Mem) - sizeof(Preamble));

            _ThisLock.Acquire();
            {
                _AllocatedEntries.Remove(preamble);
                heapMaybeWentEmpty = (_AllocatedEntries.Count() == 0);
		    }
            _ThisLock.Release();

            preamble->~Preamble();
            ExFreePool(_HeapHandle, preamble);

        #else
            if (_AllocationCountsEnabled)
            {
                // NOTE: The low order 4 bits of heap allocated objects are on 16 bytes boundaries (hence the >> 4) and so the most random
                //       bits to be usd as a counter index start at the 5th bit.
				
				heapMaybeWentEmpty = (InterlockedDecrement(&_AllocationCounts[(((ULONGLONG)Mem) >> 4) & (_AllocationCountsSize - 1)]) == 0);
            }

            ExFreePool(_HeapHandle, Mem);
        #endif

        if (heapMaybeWentEmpty)
        {
            // Report possible empty heap
            HeapWentEmptyNotification();
        }
    }
}

template <POOL_TYPE PoolType>
ULONGLONG
KtlCoreNonPageAlignedAllocator<PoolType>::GetAllocsRemaining()
{
    #if defined(TRACK_ALLOCATIONS)
        return _AllocatedEntries.Count();
    #else
        ULONGLONG   result = 0;
        for (int ix = 0; ix < _AllocationCountsSize; ix++)
        {
            result += _AllocationCounts[ix];
        }
        return result;
    #endif
}

template <POOL_TYPE PoolType>
VOID
KtlCoreNonPageAlignedAllocator<PoolType>::DebugDump()
{
#ifndef PLATFORM_UNIX
#if defined(TRACK_ALLOCATIONS)
    K_LOCK_BLOCK(_ThisLock)
    {
        Preamble*   next = _AllocatedEntries.PeekHead();
        while (next != nullptr)
        {
            KDbgPrintf("\t\t@:  0x%016llx: Allocation Caller: 0x%016llx; Size: %u\n",
                next,
                KAllocatorSupport::GetAllocationCaller(&next->AppMemBlk[KAllocatorSupport::AllocationBlockOverhead]),
                KAllocatorSupport::GetAllocatedSize(&next->AppMemBlk[KAllocatorSupport::AllocationBlockOverhead]));

            next = _AllocatedEntries.Successor(next);
        }
    }
#else
    KDbgPrintf("Debug dump not available for retail builds + user mode");
#endif
#endif
}

#else			// !KTL_USER_MODE
template <POOL_TYPE PoolType>
PVOID
KtlCoreNonPageAlignedAllocator<PoolType>::AllocWithTag(__in size_t Size, __in ULONG Tag)
{
    PVOID       result;
    size_t      actualSize;

    HRESULT hr;
    hr = SizeTAdd(Size, sizeof(Preamble), &actualSize);
    KInvariant(SUCCEEDED(hr));

    result = ExAllocatePoolWithTag(PoolType, actualSize, Tag);
    if (result != nullptr)
    {
        Preamble* preamble = reinterpret_cast<Preamble*>(result);
        preamble->Preamble::Preamble();

        BOOLEAN         backoutAllocation = FALSE;

        _ThisLock.Acquire();
        {
            if (_IsLocked)
            {
                // Allocator is locked - release the memory and return allocation failure
                backoutAllocation = TRUE;
            }
            else
            {
                _AllocatedEntries.AppendTail(preamble);
            }
        }
        _ThisLock.Release();

        if (backoutAllocation)
        {
            ExFreePool(preamble);
            return nullptr;
        }

        return &preamble->AppMemBlk[0];
    }
    return result;
}

template <POOL_TYPE PoolType>
VOID
KtlCoreNonPageAlignedAllocator<PoolType>::Free(__in PVOID Mem)
{
    if (Mem != nullptr)
    {
        Preamble*   preamble = reinterpret_cast<Preamble*>(((UCHAR*)Mem) - sizeof(Preamble));
        BOOLEAN     heapWentEmpty;

        _ThisLock.Acquire();
        {
            _AllocatedEntries.Remove(preamble);
            heapWentEmpty = (_AllocatedEntries.Count() == 0);
        }
        _ThisLock.Release();

        preamble->Preamble::~Preamble();
        ExFreePool(preamble);

        if (heapWentEmpty)
        {
            // Report possible empty heap
            HeapWentEmptyNotification();
        }
    }
}

template <POOL_TYPE PoolType>
ULONGLONG
KtlCoreNonPageAlignedAllocator<PoolType>::GetAllocsRemaining()
{
    return _AllocatedEntries.Count();
}

template <POOL_TYPE PoolType>
VOID
KtlCoreNonPageAlignedAllocator<PoolType>::DebugDump()
{
#ifndef PLATFORM_UNIX
    K_LOCK_BLOCK(_ThisLock)
    {
        Preamble*   next = _AllocatedEntries.PeekHead();
        while (next != nullptr)
        {
            KDbgPrintf("\t\t@:  0x%016I64X: Allocation Caller: 0x%016I64X; Size: %u\n",
                next,
                KAllocatorSupport::GetAllocationCaller(&next->AppMemBlk[KAllocatorSupport::AllocationBlockOverhead]),
                KAllocatorSupport::GetAllocatedSize(&next->AppMemBlk[KAllocatorSupport::AllocationBlockOverhead]));

            next = _AllocatedEntries.Successor(next);
        }
    }
#endif
}
#endif // KTL_USER_MODE


//* Page-aligned allocator
class KtlCorePageAlignedAllocator : public KtlCoreAllocator
{
public:
    KtlCorePageAlignedAllocator(KtlSystemBase* const system);
    virtual ~KtlCorePageAlignedAllocator();

    PVOID
    AllocWithTag(__in size_t Size, __in ULONG Tag);

    VOID
    Free(__in PVOID Mem);

    ULONGLONG
    GetAllocsRemaining();

protected:
    VOID
    DebugDump();

protected:
    volatile LONGLONG  _AllocationCount;
};

