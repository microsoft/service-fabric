// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class StockValue
        : public KObject<StockValue>
        , public KShared<StockValue>
    {
        K_FORCE_SHARED(StockValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        PROPERTY(LONG32, quantity_, Quantity)

        KSTRING_PROPERTY(district1_, District1)
        KSTRING_PROPERTY(district2_, District2)
        KSTRING_PROPERTY(district3_, District3)
        KSTRING_PROPERTY(district4_, District4)
        KSTRING_PROPERTY(district5_, District5)
        KSTRING_PROPERTY(district6_, District6)
        KSTRING_PROPERTY(district7_, District7)
        KSTRING_PROPERTY(district8_, District8)
        KSTRING_PROPERTY(district9_, District9)
        KSTRING_PROPERTY(district10_, District10)

        PROPERTY(double, ytd_, Ytd)
        PROPERTY(LONG32, orderCount_, OrderCount)
        PROPERTY(LONG32, remoteCount_, RemoteCount)

        KSTRING_PROPERTY(data_, Data)

    private:
        LONG32 quantity_;
        KString::SPtr district1_;
        KString::SPtr district2_;
        KString::SPtr district3_;
        KString::SPtr district4_;
        KString::SPtr district5_;
        KString::SPtr district6_;
        KString::SPtr district7_;
        KString::SPtr district8_;
        KString::SPtr district9_;
        KString::SPtr district10_;
        double ytd_;
        LONG32 orderCount_;
        LONG32 remoteCount_;
        KString::SPtr data_;
    };
}
