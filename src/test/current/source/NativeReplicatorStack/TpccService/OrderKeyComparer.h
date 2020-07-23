// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderKeyComparer
        : public KObject<OrderKeyComparer>
        , public KShared<OrderKeyComparer>
        , public Data::IComparer<OrderKey::SPtr>
    {
        K_FORCE_SHARED(OrderKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const OrderKey::SPtr& lhs,
            __in const OrderKey::SPtr& rhs) const override;
    };
}
