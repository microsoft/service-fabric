/*++

Copyright (c) 2011,2017  Microsoft Corporation

Module Name:

    klookaside.h

Abstract:

    This file defines a lookaside allocator for KShared objects that have Reuse method, such as a KAsyncBaseContext.

Author:
    Norbert P. Kusters (norbertk) 15-Nov-2011
    Richhas 2/2/17 - updated docs and style

Environment:
    Kernel mode and User mode

Notes:

--*/

#pragma once


template <class T>
class KLookaside : public KObject<KLookaside<T>>,
                   public KShared<KLookaside<T>> 
{
    K_FORCE_SHARED(KLookaside);

    public:

        // Caller supplied allocation and deallocation methods for this KLookaside's T
        typedef KDelegate<T* ()> AllocateRoutine;
        typedef KDelegate<void (__in T*)> FreeRoutine;

        /* KLookaside instance factory
            This routine creates a lookaside allocator for the given type.  The base allocate and free routine are provided as delegates.
            This lookaside allocator will keep an array of the given 'LookasideSize' to store pre-allocated objects.

            Arguments:
                Allocate            - Supplies the base allocate routine.
                LookasideAllocator  - Returns the newly created lookaside allocator.
                Allocator           - Supplies the allocator.
                AllocationTag       - Supplies the allocation tag.
                MinimumDepth        - Minimum number of items kept in the cache - default 4
                MaximumDepth        - Maximum number of items kept in the cache - default 256
                BalanceIntervalInMs - Balance interval in ms (MAXULONG == forever) - default 3 secs
				      This interval control the maximum rate at which the pool size of cached T's will be adjusted - this 
				      adjusting only happens when Allocate() or Free() are called. 

            Return Value:
                STATUS_INSUFFICIENT_RESOURCES
                STATUS_SUCCESS
        */
        static NTSTATUS
        Create(
            __in AllocateRoutine Allocate,
            __in FreeRoutine Free,
            __out KSharedPtr<KLookaside<T>>& LookasideAllocator,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __in ULONG MinimumDepth = 4,
            __in ULONG MaximumDepth = 256,
            __in ULONG BalanceIntervalInMs = 3000
            );

        /*  Allocate an instanse of T from the pool
            
            This routine allocates a new item of type T.  The allocation is made by checking the lookaside list first and otherwise
            defaults to the base allocator.

            Arguments:
                None.

            Return Value:
                A pointer to the newly allocate object or nullptr if out of resource.
        */
        T* Allocate();


        /*++ Release an instance of T back into the current KLookaside pool

            This routine frees the given element, adding to the lookaside list if the lookaside list is not full, otherwise
            sending the free to the base allocator.

            Arguments:
                ToFree  - Supplies the item to free.
                          NOTE: The caller is responsible for making sure that the T instance being freed is in a re-usable state. There 
                                are number of ways this is typically done: explicitly clear it field-by-field, call the dtor that cleans up, 
				and/or call the in_place ctor "new (&T) T(....)

            Return Value:
                None.
        */
        void Free(__in T* ToFree);

        //* Reset the cache
        void Clear();

        //* Get current cache size
        ULONG Count();

    private:
        KLookaside(
            __in AllocateRoutine AllocateCallback,
            __in FreeRoutine FreeCallback,
            __in ULONG MinimumDepth,
            __in ULONG MaximumDepth,
            __in ULONG BalanceIntervalInMs);

        void AdjustDepth();

private:
    KSpinLock _SpinLock;
        KArray<T*> _LookasideList;
        AllocateRoutine _AllocateCallback;
        FreeRoutine _FreeCallback;
        ULONGLONG _NextBalanceTickCount;
        ULONG _TargetDepth;
        ULONGLONG _TotalAllocates;
        ULONGLONG _TotalAllocateMisses;
        ULONG _MinimumDepth;
        ULONG _MaximumDepth;
        ULONG _BalanceIntervalInMs;
};

template <class T> inline
KLookaside<T>::KLookaside(
    __in AllocateRoutine AllocateCallback,
    __in FreeRoutine FreeCallback,
    __in ULONG MinimumDepth,
    __in ULONG MaximumDepth,
    __in ULONG BalanceIntervalInMs
    ) :
    _LookasideList(GetThisAllocator(), MaximumDepth)
{
    _AllocateCallback = AllocateCallback;
    _FreeCallback = FreeCallback;
    _NextBalanceTickCount = 0;
    _TargetDepth = MinimumDepth;
    _TotalAllocates = 0;
    _TotalAllocateMisses = 0;
    _MinimumDepth = MinimumDepth;
    _MaximumDepth = MaximumDepth;
    _BalanceIntervalInMs = BalanceIntervalInMs;
    SetConstructorStatus(_LookasideList.Status());
}

template <class T> inline
KLookaside<T>::~KLookaside()
{
    Clear();
}

template <class T> inline
VOID
KLookaside<T>::Clear()
{
    _SpinLock.Acquire();

    for (ULONG i = 0; i < _LookasideList.Count(); i++)
    {
        T* Item = _LookasideList[i];
        _FreeCallback(Item);
    }

    _LookasideList.Clear();
    _SpinLock.Release();
}

template <class T> inline
ULONG
KLookaside<T>::Count()
{
    ULONG n;

    _SpinLock.Acquire();
        n = _LookasideList.Count();
    _SpinLock.Release();
    return n;
}

template <class T> inline
NTSTATUS KLookaside<T>::Create(
    __in AllocateRoutine Allocate,
    __in FreeRoutine Free,
    __out KSharedPtr<KLookaside<T>>& LookasideAllocator,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG MinimumDepth,
    __in ULONG MaximumDepth,
    __in ULONG BalanceIntervalInMs
    )
{
    NTSTATUS status;

    LookasideAllocator = _new(AllocationTag, Allocator) KLookaside(Allocate, Free, MinimumDepth, MaximumDepth, BalanceIntervalInMs);

    if (!LookasideAllocator) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = LookasideAllocator->Status();
    if (!NT_SUCCESS(status)) 
    {
        LookasideAllocator = nullptr;
        return status;
    }

    return STATUS_SUCCESS;
}

template <class T> inline
T* KLookaside<T>::Allocate()
{
    ULONGLONG tickCount;
    ULONG n;
    T* r;

    _SpinLock.Acquire();

    tickCount = KNt::GetTickCount64();

    if (tickCount >= _NextBalanceTickCount) 
    {
        // Re-adjust the depth of the lookaside list based on the allocation statistics.
        _NextBalanceTickCount = tickCount + _BalanceIntervalInMs;
        AdjustDepth();
    }

    _TotalAllocates++;

    n = _LookasideList.Count();

    if (n) 
    {
        n--;
        r = _LookasideList[n];
        _LookasideList.SetCount(n);
        _SpinLock.Release();
        return r;
    }

    _TotalAllocateMisses++;
    _SpinLock.Release();
    return _AllocateCallback();
}

template <class T> inline
void KLookaside<T>::Free(__in T* ToFree)
{
    ULONGLONG tickCount;
    NTSTATUS status;

    _SpinLock.Acquire();

    tickCount = KNt::GetTickCount64();

    if (tickCount >= _NextBalanceTickCount) 
    {
        // Readjust the depth of the lookaside list based on the allocation statistics.
        _NextBalanceTickCount = tickCount + _BalanceIntervalInMs;
        AdjustDepth();
    }


    if (_LookasideList.Count() < _TargetDepth) 
    {
        status = _LookasideList.Append(ToFree);
        _SpinLock.Release();
        KInvariant(NT_SUCCESS(status));
        return;
    }

    _SpinLock.Release();
    _FreeCallback(ToFree);
}

template <class T> inline
void KLookaside<T>::AdjustDepth()
{
    const ULONGLONG SmallDeltaAllocationsThreshold = 75;
    const ULONGLONG SmallDeltaAllocationsAdjustment = 10;	
    const ULONGLONG MissedVsCachedAllocationThresholdPercentage = 5;	// 0.5%	
    const ULONGLONG MinLargeMissedCachedThreshold = 5;	
    const ULONGLONG MaxLargeMissedCachedThreshold = 30;	


    ULONG r;
    ULONG d;

    if (_TotalAllocates < SmallDeltaAllocationsThreshold)
    {
        // There were not very many allocations.  Reduce the depth by SmallDeltaAllocationsAdjustment.
        if (_TargetDepth > SmallDeltaAllocationsAdjustment + _MinimumDepth) 
        {
            _TargetDepth -= SmallDeltaAllocationsAdjustment;
        } else {
            _TargetDepth = _MinimumDepth;
        }
    } else {
        // Figure out the proportion of misses compared to allocates.
        r = (ULONG) ((_TotalAllocateMisses*1000) / _TotalAllocates);

        if (r < MissedVsCachedAllocationThresholdPercentage) 
        {
            // The proportion of misses is less then MissedVsCachedAllocationThresholdPercentage.  Reduce the target depth by 1.
            _TargetDepth--;
            if (_TargetDepth < _MinimumDepth) 
            {
                _TargetDepth = _MinimumDepth;
            }
        } else {
            // Increase the depth according to 1/2 the proportion times the distance from the max-depth, and choosing
            // a minimum increase of MinLargeMissedCachedThreshold, and a maximum increase of MaxLargeMissedCachedThreshold.
            d = r * (_MaximumDepth - _TargetDepth) / (1000 * 2) + MinLargeMissedCachedThreshold;
            if (d > MaxLargeMissedCachedThreshold) 
            {
                d = MaxLargeMissedCachedThreshold;
            }

            if (_TargetDepth + d < _MaximumDepth) 
            {
                _TargetDepth = _TargetDepth + d;
            } else {
                _TargetDepth = _MaximumDepth;
            }
        }
    }

    // Reset the counters for the next rebalancing.
    _TotalAllocates = 0;
    _TotalAllocateMisses = 0;
}
