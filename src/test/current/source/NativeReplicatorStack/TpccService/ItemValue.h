// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class ItemValue
        : public KObject<ItemValue>
        , public KShared<ItemValue>
    {
        K_FORCE_SHARED(ItemValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        PROPERTY(LONG64, image_, Image)
        KSTRING_PROPERTY(name_, Name)
        PROPERTY(double, price_, Price)
        KSTRING_PROPERTY(data_, Data)

    private:
        // DateTime entry_;
        LONG64 image_;
        KString::SPtr name_;
        double price_;
        KString::SPtr data_;
    };
}
