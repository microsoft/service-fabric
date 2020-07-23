// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class StockValueSerializer
        : public KObject<StockValueSerializer>
        , public KShared<StockValueSerializer>
        , public Data::StateManager::IStateSerializer<StockValue::SPtr>
    {
        K_FORCE_SHARED(StockValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in StockValue::SPtr stockValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        StockValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
