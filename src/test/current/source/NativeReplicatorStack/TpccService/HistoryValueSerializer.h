// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class HistoryValueSerializer
        : public KObject<HistoryValueSerializer>
        , public KShared<HistoryValueSerializer>
        , public Data::StateManager::IStateSerializer<HistoryValue::SPtr>
    {
        K_FORCE_SHARED(HistoryValueSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        void Write(
            __in HistoryValue::SPtr historyValue,
            __in Data::Utilities::BinaryWriter& binaryWriter);

        HistoryValue::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
    };
}
