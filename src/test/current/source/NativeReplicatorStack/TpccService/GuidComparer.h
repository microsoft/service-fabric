// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class GuidComparer
        : public KObject<GuidComparer>
        , public KShared<GuidComparer>
        , public Data::IComparer<KGuid>
    {
        K_FORCE_SHARED(GuidComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const KGuid& lhs,
            __in const KGuid& rhs) const override;
    };
}
