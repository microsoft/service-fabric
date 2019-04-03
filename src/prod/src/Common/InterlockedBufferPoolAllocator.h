// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // Provides an implementation of the ktl's KAllocator interface. It returns the
    // global KTL system for the GetKtlSystem() method. The alloc and free are done over
    // a buffer managed by this allocator. The buffer is got from InterlockedBufferPool
    //
    class InterlockedBufferPoolAllocator : public KAllocator
    {
    public:
        InterlockedBufferPoolAllocator();
        ~InterlockedBufferPoolAllocator();

        typedef std::function<void(PVOID, size_t)> AllocCallback;
        typedef std::function<void(PVOID)> FreeCallback;

        //
        // Initialize must be called before any memory allocation operations.
        //
        HRESULT Initialize(
            __in InterlockedBufferPool* pPool,
            __in_opt AllocCallback allocCallback = nullptr,
            __in_opt FreeCallback freeCallback = nullptr,
            __in_opt InterlockedPoolPerfCountersSPtr perfCounters = nullptr);
        
        //
        // This hands out portions of an InterlockedBufferPoolItem piece-by-piece in sequence. 
        // If the current InterlockedBufferPoolItem cannot accommodate the current request, 
        // then this gets another InterlockedBufferPoolItem from the backing InterlockedBufferPool.
        //
        PVOID Alloc(size_t Size) override;

        //
        // Free does nothing. This allows allocations to be very quick.
        // The InterlockedBufferPoolItem objects are returned to the InterlockedBufferPool in the destructor.
        //
        void Free(PVOID Mem) override;

        PVOID AllocWithTag(size_t Size, ULONG Tag) override;

        KtlSystem& GetKtlSystem() override;

        ULONGLONG GetAllocsRemaining() override;

        #if KTL_USER_MODE
            #if DBG
                ULONGLONG GetTotalAllocations() override { return 0;  }     // not implemented
            #endif
        #endif

    private:
        // After initialization, GetNewPoolItem must always be called under write-lock (m_allocationLock).
        bool GetNewPoolItem() const;
        
        mutable InterlockedBufferPool* pPool_;
        mutable InterlockedBufferPoolItem* pHead_;
        InterlockedPoolPerfCountersSPtr perfCounters_;
        mutable BYTE* pbFree_;
        mutable size_t cbFree_;
        mutable Common::RwLock allocationLock_;

        mutable LONG bufferCount_;
        mutable size_t totalBytesAllocated_;
        AllocCallback allocCallback_;
        FreeCallback freeCallback_;
        
    private: // not implemented
        InterlockedBufferPoolAllocator(const InterlockedBufferPoolAllocator&);
        InterlockedBufferPoolAllocator& operator=(const InterlockedBufferPoolAllocator&);
    };
}
