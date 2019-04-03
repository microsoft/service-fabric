// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class GuidSerializer
        : public KObject<GuidSerializer>
        , public KShared<GuidSerializer>
        , public Data::StateManager::IStateSerializer<KGuid>
    {
        K_FORCE_SHARED(GuidSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in KGuid guid,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        KGuid Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
