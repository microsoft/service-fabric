// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class NewOrderKeySerializer
        : public KObject<NewOrderKeySerializer>
        , public KShared<NewOrderKeySerializer>
        , public Data::StateManager::IStateSerializer<NewOrderKey::SPtr>
    {
        K_FORCE_SHARED(NewOrderKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in NewOrderKey::SPtr newOrderKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        NewOrderKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
