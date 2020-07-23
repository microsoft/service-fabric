// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class DistrictValue
        : public KObject<DistrictValue>
        , public KShared<DistrictValue>
    {
        K_FORCE_SHARED(DistrictValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        KSTRING_PROPERTY(name_, Name)
        KSTRING_PROPERTY(street1_, Street_1)
        KSTRING_PROPERTY(street2_, Street_2)
        KSTRING_PROPERTY(city_, City)
        KSTRING_PROPERTY(state_, State)
        KSTRING_PROPERTY(zip_, Zip)
        PROPERTY(double, tax_, Tax)
        PROPERTY(double, ytd_, Ytd)
        PROPERTY(LONG64, nextOrderId_, NextOrderId)

    private:

        KString::SPtr name_;
        KString::SPtr street1_;
        KString::SPtr street2_;
        KString::SPtr city_;
        KString::SPtr state_;
        KString::SPtr zip_;
        double tax_;
        double ytd_;
        LONG64 nextOrderId_;
    };
}
