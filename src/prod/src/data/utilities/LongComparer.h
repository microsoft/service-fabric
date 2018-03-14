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
        class LongComparer 
        : public KObject<LongComparer>
        , public KShared<LongComparer>
        , public IComparer<LONG64>
        {
            K_FORCE_SHARED(LongComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out LongComparer::SPtr & result);

            int Compare(__in const LONG64& x, __in const LONG64& y) const override;
        };
    }
}
