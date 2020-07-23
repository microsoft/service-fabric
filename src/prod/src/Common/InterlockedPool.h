// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // InterlockedPool is a thread-safe pool of objects that uses the interlocked SLIST APIs.
    // NOTE: The type T represents the pool object header. The InterlockedPool allocates space for the pool 
    // object header + the size of the pool object. The size of the pool object is configured by the higher layers 
    // and should be tuned based on perf measurements.
    //
    template<typename TPoolObjectHeader>
    class InterlockedPool
    {
    public:
        InterlockedPool();
        ~InterlockedPool();

        //
        // Pre-allocate initialCount objects and put them in the pool.
        //
        HRESULT Initialize(
            _In_ DWORD initialCount = 0, 
            _In_ DWORD allowedPercentageOfExtraObjects = 0,
            _In_ size_t objectBufferSize = 0);
        
        //
        // Get an object from the pool.
        //
        TPoolObjectHeader* GetPoolObject();

        // 
        // Return an object to the pool.
        //
        void ReturnPoolObject(_In_ TPoolObjectHeader* pObject);

        //
        // Used only for diagnostics.
        //
        LONG GetPoolObjectCount() const;

        __declspec(property(get = get_ObjectBufferSize)) size_t PoolObjectBufferSize;
        size_t get_ObjectBufferSize() const { return objectBufferSize_; }

    private:
        TPoolObjectHeader* AllocateNewObject();

        // Pop object from top of object stack. 
        TPoolObjectHeader* PopObject();

        // Push object to top of object stack.
        void PushObject(_In_ TPoolObjectHeader* pObject);
     
        PSLIST_HEADER listHeader_;
        volatile LONG poolObjectCount_;
        volatile LONG inUseObjectCount_;
        LONG initialObjectCount_;
        double allowedPercentageOfExtraObjects_;
        size_t objectBufferSize_;

        IndividualHeapAllocator allocator_;

        struct PoolItem
        {
            SLIST_ENTRY listEntry_;
            TPoolObjectHeader m_object;
        };
        
    private: // Not implemented.
        InterlockedPool(const InterlockedPool&);
        InterlockedPool& operator=(const InterlockedPool&);
    };

    template<typename TPoolObjectHeader>
    InterlockedPool<TPoolObjectHeader>::InterlockedPool()
        : listHeader_(nullptr)
        , poolObjectCount_(0)
        , inUseObjectCount_(0)
        , initialObjectCount_(0)
        , allowedPercentageOfExtraObjects_(0.0)
    {
    }

    template<typename TPoolObjectHeader>
    InterlockedPool<TPoolObjectHeader>::~InterlockedPool()
    {
        if (listHeader_ != nullptr)
        {
            // Remove all objects.
            PSLIST_ENTRY pSListEntry;
            while ((pSListEntry = ::InterlockedPopEntrySList(listHeader_)) != nullptr)
            {
                // Get the container PoolItem.
                PoolItem* pPoolItem = CONTAINING_RECORD(pSListEntry, PoolItem, listEntry_);

                // Destructor was already called when this was returned to the pool, so just free memory.
                allocator_.Free(pPoolItem);
            }

            // Free header.
            allocator_.Free(listHeader_);
            listHeader_ = nullptr;
        }
    }
 
    template<typename TPoolObjectHeader>
    HRESULT InterlockedPool<TPoolObjectHeader>::Initialize(
        _In_ DWORD initialCount, 
        _In_ DWORD allowedPercentageOfExtraObjects,
        _In_ size_t objectBufferSize)
    {
        if (listHeader_ != nullptr)
        {
            // Initialize has already been called.
            return E_FAIL;
        }

        listHeader_ = static_cast<PSLIST_HEADER>(allocator_.Alloc(sizeof(SLIST_HEADER)));

        if (listHeader_ == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        initialObjectCount_ = initialCount;
        allowedPercentageOfExtraObjects_ = allowedPercentageOfExtraObjects / 100.0;
        objectBufferSize_ = objectBufferSize;

        // All SList items must be aligned on a MEMORY_ALLOCATION_ALIGNMENT boundary.
        ::InitializeSListHead(listHeader_);

        for (DWORD i = 0; i < initialCount; i++)
        {
            TPoolObjectHeader* pObject = AllocateNewObject();
            if (pObject == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            PushObject(pObject);
        }

        return S_OK;
    }

    template<typename TPoolObjectHeader>
    TPoolObjectHeader* InterlockedPool<TPoolObjectHeader>::GetPoolObject()
    {
        TPoolObjectHeader* pObject = PopObject();

        if (pObject == nullptr)
        {
            // Pool is empty.
            pObject = AllocateNewObject();
        }

        if (pObject != nullptr)
        {
            // Call their constructor.
            (void)new (pObject) TPoolObjectHeader;
        }
        
        ::InterlockedIncrement(&inUseObjectCount_);

        return pObject;
    }
 
    template<typename TPoolObjectHeader>
    void InterlockedPool<TPoolObjectHeader>::ReturnPoolObject(_In_ TPoolObjectHeader* pObject)
    {
        // Call their destructor, but keep the memory around.
        pObject->~TPoolObjectHeader();

        // Get the information we need to decide whether or not to free the object's memory.
        LONG approximateObjectsInUse = ::InterlockedDecrement(&inUseObjectCount_);
        LONG approximateTotalPoolSize = poolObjectCount_;
        LONG approximateOpenPoolSize = (approximateTotalPoolSize > approximateObjectsInUse) ? approximateTotalPoolSize - approximateObjectsInUse : 0;
        
        // After a spike in used objects occurs (such as during a traffic spike), we may be left with large amounts of objects that will never be used again.
        // To prevent this, we need a way to shirnk the pool as objects are returned.
        // We do this by examining the number of objects we have free in the pool (approximateOpenPoolSize) as a percentage of our current buffer usage (approximateObjectsInUse).
        // As a spike subsides, the number of objects free in the pool will increase while the number of objects in use will decrease.  Once this ratio passes a
        // threshold (configured via allowedPercentageOfExtraObjects_), we will begin freeing objects returned to the pool instead of keeping them around.
        // In this way, we recover from usage spikes without an ever-increasing pool.
        // We will not decrease beyond the initial configured size of the pool.  We also will not free if the configured percentage is set to 0 - this is a special
        // value designated as a kill switch, as it has no effective meaning in a pool (would also free the returned object).
        // Some rounding error is expected but shouldn't affect the overall operation.
        double thresholdForFreeingObject = approximateObjectsInUse * allowedPercentageOfExtraObjects_;
        if (approximateOpenPoolSize > thresholdForFreeingObject && approximateTotalPoolSize > initialObjectCount_ && allowedPercentageOfExtraObjects_ != 0)
        {
            PoolItem* pPoolItem = CONTAINING_RECORD(pObject, PoolItem, m_object);
            allocator_.Free(pPoolItem);
            ::InterlockedDecrement(&poolObjectCount_);
            
            return;
        }
        
        PushObject(pObject);
    }

    template<typename TPoolObjectHeader>
    void InterlockedPool<TPoolObjectHeader>::PushObject(_In_ TPoolObjectHeader* pObject)
    {        
        // Get the container PoolItem.
        PoolItem* pPoolItem = CONTAINING_RECORD(pObject, PoolItem, m_object);

        // Return to the pool.
        ::InterlockedPushEntrySList(listHeader_, &pPoolItem->listEntry_);
    }

    template<typename TPoolObjectHeader>
    inline LONG InterlockedPool<TPoolObjectHeader>::GetPoolObjectCount() const
    {
        return poolObjectCount_;
    }
 
    template<typename TPoolObjectHeader>
    TPoolObjectHeader* InterlockedPool<TPoolObjectHeader>::AllocateNewObject()
    {
        // Allocate a new PoolItem.
        // All SList items must be aligned on a MEMORY_ALLOCATION_ALIGNMENT boundary.
        PoolItem* pPoolItem = static_cast<PoolItem*>(allocator_.Alloc(sizeof(PoolItem) + objectBufferSize_));
        if (pPoolItem == nullptr)
        {
            return nullptr;
        }

        (void)::InterlockedIncrement(&poolObjectCount_);

        return &pPoolItem->m_object;
    }
 
    template<typename TPoolObjectHeader>
    TPoolObjectHeader* InterlockedPool<TPoolObjectHeader>::PopObject()
    {
        // Get the next object (if there is one).
        PSLIST_ENTRY pSListEntry = ::InterlockedPopEntrySList(listHeader_);

        // Pool is empty.
        if (pSListEntry == nullptr)
        {
            return nullptr;
        }

        // Turn it into a PoolItem.
        PoolItem* pPoolItem = CONTAINING_RECORD(pSListEntry, PoolItem, listEntry_);

        // Return pointer to the object.
        return &pPoolItem->m_object;
    }
}
