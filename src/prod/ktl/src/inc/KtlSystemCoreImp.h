/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    KtlSystemCoreImp.h

    Description:
        Types and definition published by the KtlSystemCoreImp class

--*/

#pragma once

#include <ktl.h>

//* KtlSystemCoreImp
class KtlSystemCoreImp : public KtlSystemCore
{
public:
    static KtlSystemCore&
    GetDefaultKtlSystemCore();

    static KtlSystemCore*
    TryGetDefaultKtlSystemCore();

    KAllocator&
    NonPagedAllocator() override;

    KAllocator&
    PagedAllocator() override;

    KAllocator&
    PageAlignedRawAllocator() override;

    #if KTL_USER_MODE
        void SetEnableAllocationCounters(bool ToEnabled) override;
    #endif

protected:
    typedef KtlCoreNonPageAlignedAllocator<NonPagedPool> NonPagedKAllocator;
    typedef KtlCoreNonPageAlignedAllocator<PagedPool>    PagedKAllocator;

    // Heap used only for the first order contents of this instance
    NonPagedKAllocator*          _SystemHeap;

    NonPagedKAllocator*          _NonPagedAllocator;
    PagedKAllocator*             _PagedAllocator;
    KtlCorePageAlignedAllocator* _RawPageAlignedAllocator;

    #if KTL_USER_MODE
    static KSpinLock             gs_SingletonLock;
    HANDLE                       _HeapHandle;
    #endif

    static NTSTATUS
    Create(__out KtlSystemCoreImp*& ResultSystem);

    KtlSystemCoreImp();
    virtual ~KtlSystemCoreImp();
};

