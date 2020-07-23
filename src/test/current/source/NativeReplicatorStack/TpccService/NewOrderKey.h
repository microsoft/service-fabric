// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class NewOrderKey
        : public KObject<NewOrderKey>
        , public KShared<NewOrderKey>
    {
        K_FORCE_SHARED(NewOrderKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of DistrictKey::SPtr
        static ULONG Hash(__in const NewOrderKey::SPtr& key);

        PROPERTY(LONG32, warehouse_, Warehouse)
        PROPERTY(LONG32, district_, District)
        PROPERTY(LONG64, order_, Order)

    private:
        LONG32 warehouse_;
        LONG32 district_;
        LONG64 order_;
    };
}
