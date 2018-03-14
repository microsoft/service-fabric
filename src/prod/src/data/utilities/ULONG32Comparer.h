// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LONG_COMPARER_TAG 'moCL'

namespace Data
{
    namespace Utilities
    {
        class ULONG32Comparer 
        : public IComparer<ULONG32>
        , public KObject<ULONG32Comparer>
        , public KShared<ULONG32Comparer>
        {
            K_FORCE_SHARED(ULONG32Comparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out ULONG32Comparer::SPtr & result);

            int Compare(__in const ULONG32& x, __in const ULONG32& y) const override;

            static ULONG HashFunction(__in const ULONG32& key);
        };
    }
}
