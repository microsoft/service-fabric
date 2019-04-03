// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class DistrictKeySerializer
        : public KObject<DistrictKeySerializer>
        , public KShared<DistrictKeySerializer>
        , public Data::StateManager::IStateSerializer<DistrictKey::SPtr>
    {
        K_FORCE_SHARED(DistrictKeySerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in DistrictKey::SPtr districtKey,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        DistrictKey::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
