// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // Provides an implementation of the ktl's KAllocator interface. It returns the
    // global KTL system for the GetKtlSystem() method. The alloc and free are just
    // wrappers over heapalloc and heapfree
    //
    class IndividualHeapAllocator : public KAllocator
    {
    public:
        IndividualHeapAllocator();
        ~IndividualHeapAllocator();
        
        PVOID Alloc(size_t Size) override;
        PVOID AllocWithTag(size_t Size, ULONG Tag) override;
        void Free(PVOID Mem) override;
        KtlSystem& GetKtlSystem() override;
        ULONGLONG GetAllocsRemaining() override;
	#if KTL_USER_MODE
		#if DBG
			ULONGLONG GetTotalAllocations() override { return 0; }	// not implemented
		#endif
	#endif

    private:
        HANDLE heap_;
        
    private: // not implemented
        IndividualHeapAllocator(const IndividualHeapAllocator&);
        IndividualHeapAllocator& operator=(const IndividualHeapAllocator&);

    };
}
