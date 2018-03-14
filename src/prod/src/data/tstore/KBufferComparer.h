// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KBUFFER_COMPARER_TAG 'moCB'

namespace Data
{
    namespace TStore
    {
        class KBufferComparer
            : public IComparer<KBuffer::SPtr>
            , public KObject<KBufferComparer>
            , public KShared<KBufferComparer>
        {
            K_FORCE_SHARED(KBufferComparer)
                K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result);

            static bool Equals(__in KBuffer::SPtr & one, __in KBuffer::SPtr & two);
            static ULONG Hash(__in KBuffer::SPtr const & key);
            static int CompareBuffers(__in const KBuffer::SPtr& x, __in const KBuffer::SPtr& y);

            int Compare(__in const KBuffer::SPtr& x, __in const KBuffer::SPtr& y) const override;
        };
    }
}
