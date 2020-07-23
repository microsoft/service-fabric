// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class NewOrderKeyComparer
        : public KObject<NewOrderKeyComparer>
        , public KShared<NewOrderKeyComparer>
        , public Data::IComparer<NewOrderKey::SPtr>
    {
        K_FORCE_SHARED(NewOrderKeyComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        int Compare(
            __in const NewOrderKey::SPtr& lhs,
            __in const NewOrderKey::SPtr& rhs) const override;
    };
}
