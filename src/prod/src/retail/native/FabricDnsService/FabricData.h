// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricData :
        public KShared<FabricData>,
        public IFabricData
    {
        K_FORCE_SHARED(FabricData);
        K_SHARED_INTERFACE_IMP(IFabricData);

    public:
        static void Create(
            __out FabricData::SPtr& spData,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer
        );

    private:
        FabricData(
            __in IDnsTracer& tracer
        );

    public:
        // IFabricData Impl
        virtual bool DeserializeServiceEndpoints(
            __in IFabricResolvedServicePartitionResult&,
            __out KArray<KString::SPtr>&
        ) const override;

        virtual bool DeserializePropertyValue(
            __in IFabricPropertyValueResult&,
            __out KString::SPtr&
        ) const override;

    public:
        static bool SerializeServiceEndpoints(
            __in LPCWSTR wszEndpoint,
            __out KString& result
        );

    private:
        IDnsTracer& _tracer;
    };
}
