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
        class UnsignedLongComparer 
        : public KObject<UnsignedLongComparer>
        , public KShared<UnsignedLongComparer>
        , public IComparer<ULONG64>
        {
            K_FORCE_SHARED(UnsignedLongComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out UnsignedLongComparer::SPtr & result);

            int Compare(__in const ULONG64& x, __in const ULONG64& y) const override;
        };
    }
}
