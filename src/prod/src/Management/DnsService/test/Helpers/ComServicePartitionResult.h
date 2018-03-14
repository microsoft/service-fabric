// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    using ::_delete;

    class ComServicePartitionResult :
        public KShared<ComServicePartitionResult>,
        public IFabricResolvedServicePartitionResult
    {
        K_FORCE_SHARED(ComServicePartitionResult);

        K_BEGIN_COM_INTERFACE_LIST(ComServicePartitionResult)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricResolvedServicePartitionResult)
            K_COM_INTERFACE_ITEM(IID_IFabricResolvedServicePartitionResult, IFabricResolvedServicePartitionResult)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComServicePartitionResult::SPtr& spResult,
            __in KAllocator& allocator,
            __in KString& result
            );

    private:
        ComServicePartitionResult(
            __in KString& result
        );

    public:
        virtual const FABRIC_RESOLVED_SERVICE_PARTITION *get_Partition(void);

        virtual HRESULT GetEndpoint(
            /* [retval][out] */ const FABRIC_RESOLVED_SERVICE_ENDPOINT **endpoint);

        virtual HRESULT CompareVersion(
            /* [in] */ IFabricResolvedServicePartitionResult *other,
            /* [retval][out] */ LONG *compareResult);

    private:
        FABRIC_RESOLVED_SERVICE_ENDPOINT _endpoint;
        FABRIC_RESOLVED_SERVICE_PARTITION _partition;
        KString::SPtr _spResult;
    };
}}
