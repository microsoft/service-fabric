// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class DistrictValueByteSerializer
        : public KObject<DistrictValueByteSerializer>
        , public KShared<DistrictValueByteSerializer>
        , public Data::StateManager::IStateSerializer<DistrictValue::SPtr>
    {
        K_FORCE_SHARED(DistrictValueByteSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in DistrictValue::SPtr districtValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        DistrictValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
