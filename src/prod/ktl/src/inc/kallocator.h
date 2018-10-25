/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kallocator.h

    Description:
        Interfaces and routines related to memory allocation.

    History:
      PengLi            30-September-2010         Initial version.

--*/

#pragma once

#include <ktlcommon.h>

//
// Types and routines not available in user mode.
//

#if KTL_USER_MODE
typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool
} POOL_TYPE;

PVOID ExAllocatePoolWithTag(__in HANDLE HeapHandle, __in POOL_TYPE Type, __in size_t Size, __in ULONG Tag);
VOID ExFreePool(__in HANDLE HeapHandle, __in PVOID Mem);
#endif

//
// An abstract memory allocator interface.
//
class KAllocatorSupport;
class KtlSystem;

class KAllocator
{
public:
    virtual PVOID Alloc(__in size_t Size) = 0;
    virtual PVOID AllocWithTag(__in size_t Size, __in ULONG Tag) = 0;
    virtual VOID Free(__in PVOID Mem) = 0;
    virtual KtlSystem& GetKtlSystem() = 0;
    virtual ULONGLONG GetAllocsRemaining() = 0;
	
	#if KTL_USER_MODE
		#if DBG
			virtual ULONGLONG GetTotalAllocations() = 0;	// Test support only: Returns the total bytes allocated by a KAllocator 
		#endif
	#endif

protected:
    friend class KAllocatorSupport;
};

//  All non-KShared types that require use of a KAllocator SHOULD be decorated with the
//  KAllocator_Required() macro call.
//
//  The use of this macro indicates that a KAllocator is needed by the implementation to
//  allocate memory. Further it indicates a pattern where the ctor form T(KAllocator&)
//  must be implemented.
//
//  Example:
//      struct MyStruct
//      {
//          ...
//          KWString        Str1;
//          ...
//          MyStruct(KAllocator& Allocator)
//              :   Str1(Allocator)
//          {}
//
//          ...
//          KAllocator_Required();
//      };
//
//  This would allow:
//      KArray<MyStruct>    myStructArray(Allocator);
//
//
#define KAllocator_Required() \
    public: \
        const static BOOLEAN KAllocatorRequired = TRUE; \
    private:

//
// Traits that can be used to check if type T is decorated with the KAllocator_Required() or not
//
template <class T, class = int>
struct KAllocatorRequired
{
	static constexpr bool value = false;
};

template <class T>
struct KAllocatorRequired<T, decltype((void)T::KAllocatorRequired, 0)>
{
	static constexpr bool value = true;
};

//
// Custom allocation support. The allocator information is embedded in the memory
// so that the memory can be released using the correct allocator.
//
class KAllocatorSupport
{
public:
    static PVOID Allocate(__in_opt KAllocator* Allocator, __in size_t Size, __in ULONG Tag);
    static VOID Free(__in PVOID Mem);
    static size_t GetAllocatedSize(__in PVOID Mem);
    static void* GetAllocationCaller(__in PVOID Mem);
    static KAllocator& GetAllocator(__in  void const * ApplicationObject);
    static ULONG GetAllocationTag(__in void const * ApplicationObject);

private:
    friend void* operator new(__in size_t Size, __in ULONG Tag, __in KAllocator& Allocator);
#if defined(PLATFORM_UNIX)
    friend void* operator new[](__in size_t Size, __in ULONG Tag, __in KAllocator& Allocator);
#endif

    friend void operator delete(__in void*, __in ULONG Tag, __in KAllocator& Allocator);
    template<typename T> friend VOID _deleteArray(T* HeapArrayObj);
    template<typename T> friend T* _newArray(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit);

	// BUG: This method s/b be forced inline for perf reasons in retail build BUT there is a bug in the VS c++ where
	//      bogus indexing code is gen'ed when maintaining the gs_AllocCounts[] values - until then it is in kallocator.cpp
	//
	static PVOID Allocate(
        __in KAllocator& Allocator,
        __in size_t Size,
        __in ULONG Tag,
        __in void* Caller,
        __in ULONG NumberOfElements = 0);

    static VOID AssertValidLength(__in PVOID Obj, __in size_t Size);
    static ULONG GetArrayElementCount(__in PVOID Obj);

    struct Preamble
    {
        KAllocator*     Allocator;
        void*           AllocationCaller;
        size_t          AllocatedSize;

        union
        {
            #pragma warning(push)
            #pragma warning(disable:4201)   // C4201: nonstandard extension used : nameless struct/union

            struct
            {
            #if defined(_WIN64)
                ULONG   Tag;
            #endif
                ULONG   ArrayElementCount;  // 0 == not treated as an array
            };

            #pragma warning(pop)

            // Padding to make sure the user-visible buffer is aligned properly
            // for interlocked singly linked lists. See static_assert below.
            void* Reserved;
        };
    };
    static_assert((sizeof(Preamble) % (sizeof(void*) * 2)) == 0, "Preamble size not correct");

public:
    static ULONG const          MaxAllocationBlockOverhead = sizeof(Preamble) + (2 * sizeof(void*));
    static ULONG const          AllocationBlockOverhead = sizeof(Preamble);

    static bool                 gs_AllocCountsDisabled;
    static const ULONG          gs_AllocCountsSize = 64;
    static LONG volatile        gs_AllocCounts[gs_AllocCountsSize];

    static __inline LONGLONG AllocsRemaining(void)
    {
        LONGLONG    result = 0;

        for (int ix = 0; ix < gs_AllocCountsSize; ix++)
        {
            result += gs_AllocCounts[ix];
        }
        return result;
    }
};

#define gs_AllocsRemaining AllocsRemaining()             // for backcompat


__forceinline PVOID
KAllocatorSupport::Allocate(
    __in_opt KAllocator*    Allocator,
    __in size_t             Size,
    __in ULONG              Tag
)
{
    KInvariant(Allocator != nullptr);
    return Allocate(*Allocator, Size, Tag, _ReturnAddress());
}

__forceinline VOID
KAllocatorSupport::Free(__in PVOID Mem)
{
    void* base = (void*)((ULONG_PTR)(Mem)-sizeof(Preamble));
    Preamble* preamble = reinterpret_cast<Preamble*>(base);

    if (!gs_AllocCountsDisabled)
    {
        KAssert(gs_AllocCounts[(((ULONGLONG)base) >> 4) & (KAllocatorSupport::gs_AllocCountsSize - 1)] > 0);

        // NOTE: The low order 4 bits of heap allocated objects are on 16 bytes boundaries (hence the >> 4) and so the most random
        //       bits to be usd as a counter index start at the 5th bit.

		InterlockedDecrement(&gs_AllocCounts[(((ULONGLONG)base) >> 4) & (KAllocatorSupport::gs_AllocCountsSize - 1)]);
	}

    KInvariant(preamble->Allocator != nullptr);
    preamble->Allocator->Free(base);
}


// KTL specific global new operator - _new/_newArray() should be used by all application code
inline void* operator new(__in size_t Size, __in ULONG Tag, __in KAllocator& Allocator)
{
    return KAllocatorSupport::Allocate(Allocator, Size, Tag, _ReturnAddress());
}

#if defined(PLATFORM_UNIX)
inline void* operator new[](__in size_t Size, __in ULONG Tag, __in KAllocator& Allocator)
{
    return KAllocatorSupport::Allocate(Allocator, Size, Tag, _ReturnAddress());
}
#endif

// Simple operator placement delete for matching operator placement new
void operator delete(__in void *, __in ULONG, __in KAllocator&);


#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
    // Either use KTL or STL/ATL defined placement new - but not both
    inline void* operator new(__in size_t Size, __in void* Placement)
    {
        UNREFERENCED_PARAMETER(Size);
        return Placement;
    }

	// Simple operator placement delete for matching operator placement new
	inline void operator delete(_Pre_maybenull_ void *, __in void*) {}
#endif


//* Allocate simple block or simple contained array
//
//  Usage: _new(Tag, Allocator) Type();
//         _new(Tag, Allocator) Type()[x];      -- only valid if Type does not have a dtor
//
//  Note:   Objects/blocks allocated via _new MUST be deallocated via _delete(). If the
//          type of object being allocated has a default ctor (implied or explicit), it
//          will be called via _new;
//
#define _new(Tag, Allocator) new((Tag), (Allocator))

//* Deallocate block allocated via _new
//
//  Usage: _delete(allocatedObj*);
//
//  Note:   Dtor will be called (if implemented or implied) - just like the
//          delete keyword.
//
template<typename T>
VOID
_delete(T* HeapObj)
{
    if (HeapObj != nullptr)
    {
        // Dtor in place - will invoke virtual dtor chain correctly
        HeapObj->~T();
        KAllocatorSupport::Free(HeapObj);
    }
}

VOID __inline
_delete(void* HeapObj)
{
    if (HeapObj != nullptr)
    {
        KAllocatorSupport::Free(HeapObj);
    }
}


//* Replacement implementation for new[](Tag, Allocator) type()[x] keyword
//

// Allocate a one, two, or three dim array of objects
//
//  Usage: _newArray<type>(Tag, Allocator, dim0Size[, dim1Size [, dim2Size]]);
//
//  Note:   Default Ctor will be called (if implemented or implied) for each element.
//          Arrays allocated with _newArray MUST be deallocated via _deleteArray only.
//
template<typename T>
T*
_newArray(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit)
{
	ULONG bytes;
	HRESULT hr;

	hr = ULongMult(sizeof(T), Dim0Limit, &bytes);
	KInvariant(SUCCEEDED(hr));
	T*      result = (T*)(KAllocatorSupport::Allocate(Allocator, bytes, Tag, _ReturnAddress(), Dim0Limit));
	if (!result)
    {
        return nullptr;
    }

    T*      resultLimit = result + Dim0Limit;

    for (T* nextElement = result; nextElement < resultLimit; nextElement++)
    {
        // Construct array element in place
        new(nextElement) T;
    }

    return result;
}

template<typename T>
T*
_newArray(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit, __in ULONG Dim1Limit)
{
	HRESULT hr;
	ULONG result;
	hr = ULongMult(Dim0Limit, Dim1Limit, &result);
	KInvariant(SUCCEEDED(hr));
	return _newArray<T>(Tag, Allocator, result);
}

template<typename T>
T*
_newArray(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit, __in ULONG Dim1Limit, __in ULONG Dim2Limit)
{
	HRESULT hr;
	ULONG result;
	hr = ULongMult(Dim0Limit, Dim1Limit, &result);
	KInvariant(SUCCEEDED(hr));
	hr = ULongMult(result, Dim2Limit, &result);
	KInvariant(SUCCEEDED(hr));
    return _newArray<T>(Tag, Allocator, result);
}

//* Deallocate an array of objects allocated via _newArray()
//
//  Usage: _deleteArray(array*);
//
//  Note:   Dtor will be called (if implemented or implied) for each element - just like delete[].
//
template<typename T>
VOID
_deleteArray(T* HeapArrayObj)
{
    if (HeapArrayObj != nullptr)
    {
        ULONG   numberOfElements = KAllocatorSupport::GetArrayElementCount(HeapArrayObj);
        KAllocatorSupport::AssertValidLength(HeapArrayObj, sizeof(T) * numberOfElements);

        T*  endOfArray = HeapArrayObj + numberOfElements;
        for (T* nextPtr = HeapArrayObj; nextPtr < endOfArray; nextPtr++)
        {
            // Dtor in place - will invoke virtual dtor chain correctly
            nextPtr->~T();
        }

        KAllocatorSupport::Free(HeapArrayObj);
    }
}


// Special POD type-specific optimizations for single dim _new/_deleteArray
template<> __inline
UCHAR*
_newArray<UCHAR>(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit)
{
    UCHAR* ret = _new(Tag, Allocator) UCHAR[Dim0Limit];
    if (ret != nullptr)
    {
        RtlZeroMemory(static_cast<PVOID>(ret), Dim0Limit * sizeof(UCHAR));
    }
    return ret;
}

template<> __inline
ULONG*
_newArray<ULONG>(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit)
{
    ULONG* ret =  _new(Tag, Allocator) ULONG[Dim0Limit];
    if (ret != nullptr)
    {
        RtlZeroMemory(static_cast<PVOID>(ret), Dim0Limit * sizeof(ULONG));
    }
    return ret;
}

template<> __inline
ULONGLONG*
_newArray<ULONGLONG>(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit)
{
    ULONGLONG* ret = _new(Tag, Allocator) ULONGLONG[Dim0Limit];
    if (ret != nullptr)
    {
        RtlZeroMemory(static_cast<PVOID>(ret), Dim0Limit * sizeof(ULONGLONG));
    }
    return ret;
}

template<> __inline
WCHAR*
_newArray<WCHAR>(__in ULONG Tag, __in KAllocator& Allocator, __in ULONG Dim0Limit)
{
    WCHAR* ret = _new(Tag, Allocator) WCHAR[Dim0Limit];
    if (ret != nullptr)
    {
        RtlZeroMemory(static_cast<PVOID>(ret), Dim0Limit * sizeof(WCHAR));
    }
    return ret;
}

template<> __inline
VOID
_deleteArray<UCHAR>(UCHAR* CharArray)
{
    _delete(CharArray);
}

template<> __inline
VOID
_deleteArray<ULONG>(ULONG* Array)
{
    _delete(Array);
}

template<> __inline
VOID
_deleteArray<ULONGLONG>(ULONGLONG* Array)
{
    _delete(Array);
}

template<> __inline
VOID
_deleteArray<WCHAR>(WCHAR* CharArray)
{
    _delete(CharArray);
}
