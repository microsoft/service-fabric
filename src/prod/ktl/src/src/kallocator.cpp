/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kallocator.cpp

    Description:
        Interfaces and routines related to memory allocation.

    History:
      Peng Li (pengli)  30-September-2010         Initial version.

--*/

#include <ktl.h>

#pragma intrinsic(_ReturnAddress)

#if KTL_USER_MODE

PVOID ExAllocatePoolWithTag(__in HANDLE HeapHandle, __in POOL_TYPE Type, __in size_t Size, __in ULONG Tag)
{
    UNREFERENCED_PARAMETER(Type);
    UNREFERENCED_PARAMETER(Tag);

    return HeapAlloc(HeapHandle, 0, Size);
}

VOID ExFreePool(__in HANDLE HeapHandle, __in PVOID Mem)
{
    HeapFree(HeapHandle, 0, Mem);
}

#endif

//** KAllocatorSupport implementation
__declspec(align(16)) LONG volatile KAllocatorSupport::gs_AllocCounts[KAllocatorSupport::gs_AllocCountsSize] =
{ 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0,
};
static_assert(sizeof(KAllocatorSupport::gs_AllocCounts) / sizeof(ULONG) == KAllocatorSupport::gs_AllocCountsSize, "gs_AllocCounts size is incorrect");

bool    KAllocatorSupport::gs_AllocCountsDisabled = false;

size_t
KAllocatorSupport::GetAllocatedSize(__in PVOID Mem)
{
    void* base = (void*)((ULONG_PTR)(Mem) - sizeof(Preamble));
    Preamble* preamble = reinterpret_cast<Preamble*>(base);

    return preamble->AllocatedSize;
}

void*
KAllocatorSupport::GetAllocationCaller(__in PVOID Mem)
{
    void* base = (void*)((ULONG_PTR)(Mem) - sizeof(Preamble));
    Preamble* preamble = reinterpret_cast<Preamble*>(base);

    return preamble->AllocationCaller;
}

KAllocator&
KAllocatorSupport::GetAllocator(__in void const * ApplicationObject)
{
    KAllocator*     result = reinterpret_cast<Preamble const *>((reinterpret_cast<UCHAR const *>(ApplicationObject)) - sizeof(Preamble))->Allocator;
    KInvariant(result != nullptr);
    return *result;
}

ULONG
KAllocatorSupport::GetAllocationTag(__in void const * ApplicationObject)
{
    Preamble* preamble = reinterpret_cast<Preamble*>((void*)((ULONG_PTR)(ApplicationObject) - sizeof(Preamble)));

#if defined(_WIN64)
    return preamble->Tag;
#else
    preamble;
    return 0;
#endif
}

VOID
KAllocatorSupport::AssertValidLength(__in PVOID Obj, __in size_t Size)
{
    KInvariant(((reinterpret_cast<Preamble*>((reinterpret_cast<UCHAR*>(Obj)) - sizeof(Preamble)))->AllocatedSize) == Size);
}

ULONG
KAllocatorSupport::GetArrayElementCount(__in PVOID Obj)
{
    Preamble*   preamble = reinterpret_cast<Preamble*>((reinterpret_cast<UCHAR*>(Obj)) - sizeof(Preamble));
    ULONG result = preamble->ArrayElementCount;
    KInvariant(result > 0);

    return result;
}

void
operator delete(__in void *, __in ULONG, __in KAllocator&)
{
    // Must use _delete
    KInvariant(TRUE);
}

#if !KTL_USER_MODE
    // When targeting kernel-mode, global delete must be defined to avoid undef link errors
    void __cdecl operator delete(__in void* )
    {
        KInvariant(FALSE);     // Should never be called
    }
#endif

// BUG: This method s/b be forced inline for perf reasons in retail build BUT there is a bug in the VS c++ where
//      bogus indexing code is gen'ed when maintaining the gs_AllocCounts[] values - until then it is here
//
PVOID
KAllocatorSupport::Allocate(
	__in KAllocator& Allocator,
	__in size_t Size,
	__in ULONG Tag,
	__in void* Caller,
	__in ULONG NumberOfElements)
{
	HRESULT hr;
	size_t result;

	hr = SizeTAdd(sizeof(Preamble), Size, &result);
	KInvariant(SUCCEEDED(hr));

	void* base = Allocator.AllocWithTag(result, Tag);

	if (base == nullptr)
	{
		return nullptr;
	}

	if (!gs_AllocCountsDisabled)
	{
		// NOTE: The low order 4 bits of heap allocated objects are on 16 bytes boundaries (hence the >> 4) and so the most random
		//       bits to be usd as a counter index start at the 5th bit.

		InterlockedIncrement(&gs_AllocCounts[(((ULONGLONG)base) >> 4) & (KAllocatorSupport::gs_AllocCountsSize - 1)]);
	}

	Preamble* preamble = reinterpret_cast<Preamble*>(base);
	preamble->Allocator = &Allocator;

#if defined(_WIN64)
	preamble->Tag = Tag;
#endif

	preamble->ArrayElementCount = NumberOfElements;
	preamble->AllocationCaller = Caller;
	preamble->AllocatedSize = Size;

	return (void*)(preamble + 1);
}
// #pragma optimize( "", on )

