// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderValueSerializer
        : public KObject<OrderValueSerializer>
        , public KShared<OrderValueSerializer>
        , public Data::StateManager::IStateSerializer<OrderValue::SPtr>
    {
        K_FORCE_SHARED(OrderValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in OrderValue::SPtr orderValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        OrderValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
