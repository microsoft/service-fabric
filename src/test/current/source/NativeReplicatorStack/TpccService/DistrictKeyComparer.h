// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class DistrictKeyComparer
        : public KObject<DistrictKeyComparer>
        , public KShared<DistrictKeyComparer>
        , public Data::IComparer<DistrictKey::SPtr>
    {
        K_FORCE_SHARED(DistrictKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const DistrictKey::SPtr& lhs,
            __in const DistrictKey::SPtr& rhs) const override;
    };
}
