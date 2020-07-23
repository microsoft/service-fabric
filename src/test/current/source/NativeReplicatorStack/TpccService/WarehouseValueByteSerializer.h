// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class WarehouseValueByteSerializer
        : public KObject<WarehouseValueByteSerializer>
        , public KShared<WarehouseValueByteSerializer>
        , public Data::StateManager::IStateSerializer<WarehouseValue::SPtr>
    {
        K_FORCE_SHARED(WarehouseValueByteSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in WarehouseValue::SPtr warehouseValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);
        
        WarehouseValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
