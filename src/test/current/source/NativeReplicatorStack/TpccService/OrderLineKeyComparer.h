// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderLineKeyComparer
        : public KObject<OrderLineKeyComparer>
        , public KShared<OrderLineKeyComparer>
        , public Data::IComparer<OrderLineKey::SPtr>
    {
        K_FORCE_SHARED(OrderLineKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const OrderLineKey::SPtr& lhs,
            __in const OrderLineKey::SPtr& rhs) const override;
    };
}
