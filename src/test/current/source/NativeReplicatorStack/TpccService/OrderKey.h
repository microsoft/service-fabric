// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderKey
        : public KObject<OrderKey>
        , public KShared<OrderKey>
    {
        K_FORCE_SHARED(OrderKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of DistrictKey::SPtr
        static ULONG Hash(__in const OrderKey::SPtr& key);

        PROPERTY(int, warehouse_, Warehouse)
        PROPERTY(int, district_, District)
        PROPERTY(int, customer_, Customer)
        PROPERTY(LONG64, order_, Order)

    private:
        int warehouse_;
        int district_;
        int customer_;
        LONG64 order_;
    };
}
