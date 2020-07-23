// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

const DWORD     c_defaultDwFlags = 0;
const size_t    c_initialHeapSize = 1024 * 1024;    // Initial size    
const size_t    c_maxHeapSize = 0;    // unbound

IndividualHeapAllocator::IndividualHeapAllocator()
: heap_(::HeapCreate(c_defaultDwFlags, c_initialHeapSize, c_maxHeapSize))
{
}

IndividualHeapAllocator::~IndividualHeapAllocator()
{
    if (heap_!= nullptr)
    {
        (void)::HeapDestroy(heap_);
        heap_= nullptr;
    }
}

PVOID IndividualHeapAllocator::Alloc(size_t bytesToAllocate)
{
    if (heap_ == nullptr)
    {
        return nullptr;
    }

    return ::HeapAlloc(heap_, c_defaultDwFlags, bytesToAllocate);
}

PVOID IndividualHeapAllocator::AllocWithTag(size_t bytesToAllocate, ULONG tag)
{
    UNREFERENCED_PARAMETER(tag);
    return Alloc(bytesToAllocate);
}

void IndividualHeapAllocator::Free(PVOID pv)
{
    if ((heap_!= nullptr) && (pv != nullptr))
    {
        (void)::HeapFree(heap_, c_defaultDwFlags, pv);
    }
}

KtlSystem& IndividualHeapAllocator::GetKtlSystem()
{
    return GetSFDefaultKtlSystem();
}

ULONGLONG IndividualHeapAllocator::GetAllocsRemaining()
{
    Assert::TestAssert("Calling GetAllocsRemaining");
    return 0xFFFF;
}
