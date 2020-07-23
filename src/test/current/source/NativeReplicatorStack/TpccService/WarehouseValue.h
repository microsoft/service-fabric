// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class WarehouseValue
        : public KObject<WarehouseValue>
        , public KShared<WarehouseValue>
    {
        K_FORCE_SHARED(WarehouseValue)

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

    private:
        KString::SPtr name_;
        KString::SPtr street1_;
        KString::SPtr street2_;
        KString::SPtr city_;
        KString::SPtr state_;
        KString::SPtr zip_;
        double tax_;
        double ytd_;
    };

    inline
    bool operator== (const WarehouseValue& lhs, const WarehouseValue& rhs)
    {
        return lhs.Name->Compare(*rhs.Name) == 0 &&
            lhs.Street_1->Compare(*rhs.Street_1) == 0 &&
            lhs.Street_2->Compare(*rhs.Street_2) == 0 &&
            lhs.City->Compare(*rhs.City) == 0 &&
            lhs.State->Compare(*rhs.State) == 0 &&
            lhs.Zip->Compare(*rhs.Zip) == 0 &&
            lhs.Tax == rhs.Tax &&
            lhs.Ytd == rhs.Ytd;
    }

    inline
    bool operator!= (const WarehouseValue& lhs, const WarehouseValue& rhs)
    {
        return !(lhs == rhs);
    }
}
