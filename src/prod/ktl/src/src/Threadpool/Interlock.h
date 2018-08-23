#pragma once

#include "MinPal.h"

namespace KtlThreadpool {

    inline LONG InterlockedIncrement(LONG volatile *lpAddend)
    {
        return __sync_add_and_fetch(lpAddend, (LONG)1);
    }

    inline LONGLONG InterlockedIncrement64(LONGLONG volatile *lpAddend)
    {
        return __sync_add_and_fetch(lpAddend, (LONGLONG)1);
    }

    inline LONG InterlockedDecrement(LONG volatile *lpAddend)
    {
        return __sync_sub_and_fetch(lpAddend, (LONG)1);
    }

    inline LONGLONG InterlockedDecrement64(LONGLONG volatile *lpAddend)
    {
        return __sync_sub_and_fetch(lpAddend, (LONGLONG)1);
    }

    inline LONG InterlockedExchange(LONG volatile *Target, LONG Value)
    {
        return __sync_swap(Target, Value);
    }

    inline LONGLONG InterlockedExchange64(LONGLONG volatile *Target, LONGLONG Value)
    {
        return __sync_swap(Target, Value);
    }

    inline LONG InterlockedCompareExchange(LONG volatile *Destination, LONG Exchange, LONG Comperand)
    {
        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
    }

    inline LONG InterlockedCompareExchangeAcquire(LONG volatile *Destination, LONG Exchange, LONG Comperand)
    {
        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
    }

    inline LONG InterlockedCompareExchangeRelease(LONG volatile *Destination, LONG Exchange, LONG Comperand)
    {
        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
    }

    inline LONGLONG InterlockedCompareExchange64(LONGLONG volatile *Destination, LONGLONG Exchange, LONGLONG Comperand)
    {
        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
    }

    inline LONG InterlockedExchangeAdd(LONG volatile *Addend, LONG Value)
    {
        return __sync_fetch_and_add(Addend, Value);
    }

    inline LONGLONG InterlockedExchangeAdd64(LONGLONG volatile *Addend, LONGLONG Value)
    {
        return __sync_fetch_and_add(Addend, Value);
    }

    inline LONG InterlockedAnd(LONG volatile *Destination, LONG Value)
    {
        return __sync_fetch_and_and(Destination, Value);
    }

    inline LONG InterlockedOr(LONG volatile *Destination, LONG Value)
    {
        return __sync_fetch_and_or(Destination, Value);
    }

    inline UCHAR InterlockedBitTestAndReset(LONG volatile *Base, LONG Bit)
    {
        return (InterlockedAnd(Base, ~(1 << Bit)) & (1 << Bit)) != 0;
    }

    inline UCHAR InterlockedBitTestAndSet(LONG volatile *Base, LONG Bit)
    {
        return (InterlockedOr(Base, (1 << Bit)) & (1 << Bit)) != 0;
    }

    inline void MemoryBarrier(VOID)
    {
        __sync_synchronize();
    }
}
