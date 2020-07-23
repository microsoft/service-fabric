// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class StockKeyComparer
        : public KObject<StockKeyComparer>
        , public KShared<StockKeyComparer>
        , public Data::IComparer<StockKey::SPtr>
    {
        K_FORCE_SHARED(StockKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const StockKey::SPtr& lhs,
            __in const StockKey::SPtr& rhs) const override;
    };
}
