// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    using ::_delete;

    class ComFabricServiceDescriptionResult :
        public KShared<ComFabricServiceDescriptionResult>,
        public IFabricServiceDescriptionResult
    {
        K_FORCE_SHARED(ComFabricServiceDescriptionResult);

        K_BEGIN_COM_INTERFACE_LIST(ComFabricServiceDescriptionResult)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricServiceDescriptionResult)
            K_COM_INTERFACE_ITEM(IID_IFabricServiceDescriptionResult, IFabricServiceDescriptionResult)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComFabricServiceDescriptionResult::SPtr& spResult,
            __in KAllocator& allocator,
            __in FABRIC_PARTITION_SCHEME scheme
            );

    private:
        ComFabricServiceDescriptionResult(
            __in FABRIC_PARTITION_SCHEME scheme
        );

    public:
        virtual const FABRIC_SERVICE_DESCRIPTION* get_Description(void);

    private:
        FABRIC_SERVICE_DESCRIPTION _desc;
        FABRIC_STATELESS_SERVICE_DESCRIPTION _statelessDesc;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION _statefulDesc;
        FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION _int64Desc;
        FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION _namedDesc;
    };
}}
