// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class StockKey
        : public KObject<StockKey>
        , public KShared<StockKey>
    {
        K_FORCE_SHARED(StockKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of DistrictKey::SPtr
        static ULONG Hash(__in const StockKey::SPtr& key);

        PROPERTY(LONG32, warehouse_, Warehouse)
        PROPERTY(LONG64, stock_, Stock)

    private:
        LONG32 warehouse_;
        LONG64 stock_;
    };
}
