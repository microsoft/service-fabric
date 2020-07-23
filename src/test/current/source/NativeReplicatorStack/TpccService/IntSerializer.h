// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class IntSerializer
        : public KObject<IntSerializer>
        , public KShared<IntSerializer>
        , public Data::StateManager::IStateSerializer<int>
    {
        K_FORCE_SHARED(IntSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in int value,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        int Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
