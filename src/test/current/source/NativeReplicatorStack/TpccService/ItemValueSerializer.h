// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class ItemValueSerializer
        : public KObject<ItemValueSerializer>
        , public KShared<ItemValueSerializer>
        , public Data::StateManager::IStateSerializer<ItemValue::SPtr>
    {
        K_FORCE_SHARED(ItemValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in ItemValue::SPtr itemValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        ItemValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
