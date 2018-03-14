// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define INT_COMPARER_TAG 'moCI'

namespace Data
{
    namespace Utilities
    {
        class IntComparer 
        : public IComparer<int>
        , public KObject<IntComparer>
        , public KShared<IntComparer>
        {
            K_FORCE_SHARED(IntComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out IntComparer::SPtr & result);

            int Compare(__in const int& x, __in const int& y) const override;
        };
    }
}
