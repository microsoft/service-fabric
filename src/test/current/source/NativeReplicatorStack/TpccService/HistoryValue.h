// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class HistoryValue
        : public KObject<HistoryValue>
        , public KShared<HistoryValue>
    {
        K_FORCE_SHARED(HistoryValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        PROPERTY(LONG32, warehouse_, Warehouse)
        PROPERTY(LONG32, district_, District)
        PROPERTY(LONG32, customerWarehouse_, CustomerWarehouse)
        PROPERTY(LONG32, customerDistrict_, CustomerDistrict)
        PROPERTY(LONG32, customer_, Customer)
        PROPERTY(double, amount_, Amount)
        KSTRING_PROPERTY(data_, Data)

    private:
		LONG32 warehouse_;
		LONG32 district_;
		LONG32 customerWarehouse_;
		LONG32 customerDistrict_;
		LONG32 customer_;
        //Datetime date_;
        double amount_;
        KString::SPtr data_;
    };
}
