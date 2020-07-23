// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderLineValue
        : public KObject<OrderLineValue>
        , public KShared<OrderLineValue>
    {
        K_FORCE_SHARED(OrderLineValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        PROPERTY(int, quantity_, Quantity)
        PROPERTY(double, amount_, Amount)
        KSTRING_PROPERTY(info_, Info)

    private:
        // DateTime dilevery_;
        int quantity_;
        double amount_;
        KString::SPtr info_;
    };
}
