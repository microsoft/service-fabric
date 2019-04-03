// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class CustomerKeyComparer
        : public KObject<CustomerKeyComparer>
        , public KShared<CustomerKeyComparer>
        , public Data::IComparer<CustomerKey::SPtr>
    {
        K_FORCE_SHARED(CustomerKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const CustomerKey::SPtr& lhs,
            __in const CustomerKey::SPtr& rhs) const override;
    };
}
