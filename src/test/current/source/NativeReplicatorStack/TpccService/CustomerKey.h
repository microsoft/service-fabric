// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class CustomerKey
        : public KObject<CustomerKey>
        , public KShared<CustomerKey>
    {
        K_FORCE_SHARED(CustomerKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of CustomerKey::SPtr
        static ULONG Hash(__in const CustomerKey::SPtr& key);

        PROPERTY(LONG32, warehouse_, Warehouse)
        PROPERTY(LONG32, district_, District)
        PROPERTY(LONG32, customer_, Customer)

    private:
        LONG32 warehouse_;
        LONG32 district_;
        LONG32 customer_;
    };
}
