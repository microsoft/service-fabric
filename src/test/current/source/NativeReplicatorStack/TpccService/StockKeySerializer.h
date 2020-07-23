// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class StockKeySerializer
        : public KObject<StockKeySerializer>
        , public KShared<StockKeySerializer>
        , public Data::StateManager::IStateSerializer<StockKey::SPtr>
    {
        K_FORCE_SHARED(StockKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in StockKey::SPtr stockKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        StockKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
