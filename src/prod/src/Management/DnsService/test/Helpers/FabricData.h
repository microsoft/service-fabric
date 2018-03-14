// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
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
            __in KAllocator& allocator
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
        bool SerializeServiceEndpoints(
            __out ComPointer<IFabricResolvedServicePartitionResult>&,
            __in KArray<KString::SPtr>&
        );

        bool SerializePropertyValue(
            __out ComPointer<IFabricPropertyValueResult>&,
            __in LPCWSTR wszValue
        );
    };
}}
