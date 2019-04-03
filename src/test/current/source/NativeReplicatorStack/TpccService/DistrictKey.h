// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class DistrictKey
        : public KObject<DistrictKey>
        , public KShared<DistrictKey>
    {
        K_FORCE_SHARED(DistrictKey)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        // Hash function. Return the hash code of DistrictKey::SPtr
        static ULONG Hash(__in const DistrictKey::SPtr& key);

        PROPERTY(LONG32, warehouse_, Warehouse)
        PROPERTY(int, district_, District)

    private:
        LONG32 warehouse_;
        LONG32 district_;
    };
}
