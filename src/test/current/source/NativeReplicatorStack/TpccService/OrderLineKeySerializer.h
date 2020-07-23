// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderLineKeySerializer
        : public KObject<OrderLineKeySerializer>
        , public KShared<OrderLineKeySerializer>
        , public Data::StateManager::IStateSerializer<OrderLineKey::SPtr>
    {
        K_FORCE_SHARED(OrderLineKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in OrderLineKey::SPtr orderLineKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        OrderLineKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
