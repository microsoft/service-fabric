// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

InterlockedBufferPoolAllocator::InterlockedBufferPoolAllocator()
    : pPool_(nullptr)
    , pHead_(nullptr)
    , pbFree_(nullptr)
    , cbFree_(0)
    , bufferCount_(0)
    , totalBytesAllocated_(0)
    , allocationLock_()
    , allocCallback_(nullptr)
    , freeCallback_(nullptr)
    , perfCounters_(nullptr)
{
}

InterlockedBufferPoolAllocator::~InterlockedBufferPoolAllocator()
{
    if (perfCounters_)
    {
        perfCounters_->UpdatePerfCounters(bufferCount_, totalBytesAllocated_);
    }

    InterlockedBufferPoolItem* pCurrent = pHead_;
    while (pCurrent != nullptr)
    {
        InterlockedBufferPoolItem* pNext = pCurrent->pNext_;
        pPool_->ReturnPoolObject(pCurrent);
        pCurrent = pNext;
    }
}

HRESULT InterlockedBufferPoolAllocator::Initialize(
    InterlockedBufferPool* pPool,
    AllocCallback allocCallback,
    FreeCallback freeCallback,
    InterlockedPoolPerfCountersSPtr perfCounters)
{
    if (pPool_ != nullptr)
    {
        return E_UNEXPECTED;
    }

    pPool_ = pPool;
    if (!GetNewPoolItem())
    {
        return E_OUTOFMEMORY;
    }

    allocCallback_ = allocCallback;
    freeCallback_ = freeCallback;
    perfCounters_ = perfCounters;

    return S_OK;
}

void InterlockedBufferPoolAllocator::Free(void* pv)
{
    // Free does nothing. This allows allocations to be very quick.
    // The InterlockedBufferPoolItem objects are returned to the InterlockedBufferPool in the destructor.

    if (freeCallback_ != nullptr)
    {
        freeCallback_(pv);
    }
}

PVOID InterlockedBufferPoolAllocator::Alloc(size_t bytesToAllocate)
{
    static const int c_alignmentDelta = sizeof(void*)-1; // Add in an extra alignment - 1 bytes
    static const int c_alignmentMask = ~c_alignmentDelta; // Mask off the alignment size (to ensure proper alignment)

    // Now, adding the alignment delta and bitwise anding the alignment mask should get us cbBytes rounded up
    // to the nearest alignment boundary.  Use that as the allocation size, which will leave up to c_alignmentDelta
    // extra bytes at the end of the allocation as unused space.
    size_t cbBytes = (bytesToAllocate + c_alignmentDelta) & c_alignmentMask;
    if (cbBytes > pPool_->PoolObjectBufferSize)
    {
        return nullptr;
    }

    {
        AcquireWriteLock writeLock(allocationLock_);

        if ((cbBytes > cbFree_) && !GetNewPoolItem())
        {
            return nullptr;
        }

        void* pv = pbFree_;
        pbFree_ += cbBytes;
        cbFree_ -= cbBytes;
        totalBytesAllocated_ += cbBytes;

        if (allocCallback_ != nullptr)
        {
            allocCallback_(pv, cbBytes);
        }

        return pv;
    }
}

PVOID InterlockedBufferPoolAllocator::AllocWithTag(size_t bytesToAllocate, ULONG tag)
{
    UNREFERENCED_PARAMETER(tag);
    return Alloc(bytesToAllocate);
}

KtlSystem& InterlockedBufferPoolAllocator::GetKtlSystem()
{
    return GetSFDefaultKtlSystem();
}

ULONGLONG InterlockedBufferPoolAllocator::GetAllocsRemaining()
{
    // KTL for some reason needs this. We will not run out of buffer
    // space as long as the process has virtual memory.
    //
    return 0xFFFF;
}

// After initialization, GetNewPoolItem must always be called under write-lock (m_allocationLock).
bool InterlockedBufferPoolAllocator::GetNewPoolItem() const
{
    InterlockedBufferPoolItem* pItem = pPool_->GetPoolObject();
    if (pItem == nullptr)
    {
        return false;
    }

    pItem->pNext_ = pHead_;
    pHead_ = pItem;
    pbFree_ = pHead_->buffer_;
    cbFree_ = pPool_->PoolObjectBufferSize ;
    bufferCount_++;

    return true;
}

