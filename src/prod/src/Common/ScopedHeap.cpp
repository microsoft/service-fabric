// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    ScopedHeap::ScopedHeap()
        : scopedHeapList_(), lock_()
    {
    }

    ScopedHeap::~ScopedHeap()
    {
        for (auto & entry : scopedHeapList_)
        {
            delete[] entry.heapHead;
            entry.heapHead = nullptr;
            entry.heapCurrent = nullptr;
            entry.sizeRemain = 0;
        }
    }

    LPVOID ScopedHeap::Allocate(size_t size)
    {
        byte * result = nullptr;
        Common::AcquireExclusiveLock acquire(lock_);

        for (auto & entry : scopedHeapList_)
        {
            if (entry.sizeRemain >= size)
            {
                result = entry.heapCurrent;
                entry.heapCurrent += size;
                entry.sizeRemain -= size;

                return result;
            }
        }

        size_t allocateSize = (size > allocateSizeDefault) ? allocateSizeDefault + size : allocateSizeDefault;

        byte * buffer = new byte[allocateSize]();

        ScopedHeapEntry entry;
        entry.heapHead = buffer;
        entry.heapCurrent = buffer + size;
        entry.sizeRemain = allocateSize - size;
        scopedHeapList_.push_back(entry);

        result = buffer;
        
        return result;
    }

    LPCWSTR ScopedHeap::AddString(std::wstring const & s)
    {
        return AddString(s, false);
    }

    LPCWSTR ScopedHeap::AddString(std::wstring const & s, bool nullIfEmpty)
    {
        wchar_t * result = nullptr;
        if (nullIfEmpty && s.empty())
        {
            return result;
        }
        
        size_t byteCount = (s.size() + 1) * sizeof(wchar_t);
        result = reinterpret_cast<wchar_t*>(Allocate(byteCount));
        if (result != NULL)
        {
            memcpy_s(result, byteCount, s.c_str(), byteCount);
        }
        
        return result;
    }
}
