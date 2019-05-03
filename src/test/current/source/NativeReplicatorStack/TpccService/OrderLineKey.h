// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderLineKey
        : public KObject<OrderLineKey>
        , public KShared<OrderLineKey>
    {
        K_FORCE_SHARED(OrderLineKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of DistrictKey::SPtr
        static ULONG Hash(__in const OrderLineKey::SPtr& key);

        PROPERTY(int, warehouse_, Warehouse)
        PROPERTY(int, district_, District)
        PROPERTY(LONG64, order_, Order)
        PROPERTY(LONG64, item_, Item)
        PROPERTY(int, supplyWarehouse_, SupplyWarehouse)
        PROPERTY(int, number_, Number)

    private:
        int warehouse_;
        int district_;
        LONG64 order_;
        LONG64 item_;
        int supplyWarehouse_;
        int number_;
    };
}
