// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ScopedHeap
    {
        DENY_COPY(ScopedHeap);

        struct ScopedHeapEntry
        {
            byte * heapHead;
            byte * heapCurrent;
            size_t sizeRemain;
        };

    public:
        ScopedHeap();
        ~ScopedHeap();

        template<class TItem>
        ReferencePointer<TItem> AddItem()
        {
            ReferencePointer<TItem> p(reinterpret_cast<TItem*>(Allocate(sizeof(TItem))));
            return p;
        }   

        template<class TItem>
        ReferenceArray<TItem> AddArray(size_t count)
        {
            ReferenceArray<TItem> a(count, reinterpret_cast<TItem*>(Allocate(count * sizeof(TItem))));
            return a;
        }

        LPCWSTR AddString(std::wstring const & s);
        LPCWSTR AddString(std::wstring const & s, bool nullIfEmpty);

        LPVOID Allocate(size_t size);

    private:
        Common::RwLock lock_;
        static const size_t allocateSizeDefault = 8000;// A size based on our analysis, sync with jefchen, xunlu to change the value
        std::vector<ScopedHeapEntry> scopedHeapList_;
    };
}
