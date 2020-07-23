// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class CustomerValueSerializer
        : public KObject<CustomerValueSerializer>
        , public KShared<CustomerValueSerializer>
        , public Data::StateManager::IStateSerializer<CustomerValue::SPtr>
    {
        K_FORCE_SHARED(CustomerValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in CustomerValue::SPtr customerValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        CustomerValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
