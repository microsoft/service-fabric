// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class Long64Serializer
        : public KObject<Long64Serializer>
        , public KShared<Long64Serializer>
        , public Data::StateManager::IStateSerializer<LONG64>
    {
        K_FORCE_SHARED(Long64Serializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in LONG64 value,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        LONG64 Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
