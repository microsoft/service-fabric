// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderKeySerializer
        : public KObject<OrderKeySerializer>
        , public KShared<OrderKeySerializer>
        , public Data::StateManager::IStateSerializer<OrderKey::SPtr>
    {
        K_FORCE_SHARED(OrderKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in OrderKey::SPtr orderKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        OrderKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
