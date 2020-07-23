// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderLineValueSerializer
        : public KObject<OrderLineValueSerializer>
        , public KShared<OrderLineValueSerializer>
        , public Data::StateManager::IStateSerializer<OrderLineValue::SPtr>
    {
        K_FORCE_SHARED(OrderLineValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in OrderLineValue::SPtr orderLineValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        OrderLineValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
