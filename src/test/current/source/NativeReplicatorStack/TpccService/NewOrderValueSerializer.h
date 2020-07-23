// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class NewOrderValueSerializer
        : public KObject<NewOrderValueSerializer>
        , public KShared<NewOrderValueSerializer>
        , public Data::StateManager::IStateSerializer<NewOrderValue::SPtr>
    {
        K_FORCE_SHARED(NewOrderValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in NewOrderValue::SPtr newOrderValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        NewOrderValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
