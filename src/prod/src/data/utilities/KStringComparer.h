// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SHARED_STRING_COMPARER_TAG 'moCS'

namespace Data
{
    namespace Utilities
    {
        class KStringComparer 
        : public IComparer<KString::SPtr>
        , public KObject<KStringComparer>
        , public KShared<KStringComparer>
        {
            K_FORCE_SHARED(KStringComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out KStringComparer::SPtr & result);

            int Compare(__in const KString::SPtr& x, __in const KString::SPtr& y) const override;
        };
    }
}
