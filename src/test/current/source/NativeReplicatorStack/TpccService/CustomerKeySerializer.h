// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class CustomerKeySerializer
        : public KObject<CustomerKeySerializer>
        , public KShared<CustomerKeySerializer>
        , public Data::StateManager::IStateSerializer<CustomerKey::SPtr>
    {
        K_FORCE_SHARED(CustomerKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in CustomerKey::SPtr customerKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        CustomerKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
