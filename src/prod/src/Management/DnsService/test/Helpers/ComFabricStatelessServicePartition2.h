// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    using ::_delete;

    class ComFabricStatelessServicePartition2 :
        public KShared<ComFabricStatelessServicePartition2>,
        public IFabricStatelessServicePartition2
    {
        K_FORCE_SHARED(ComFabricStatelessServicePartition2);

        K_BEGIN_COM_INTERFACE_LIST(ComFabricStatelessServicePartition2)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricStatelessServicePartition2)
            K_COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition2, IFabricStatelessServicePartition2)
            K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComFabricStatelessServicePartition2::SPtr& spPartition,
            __in KAllocator& allocator
        );

    public:
        virtual HRESULT STDMETHODCALLTYPE GetPartitionInfo(
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **
        ) override
        {
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE ReportLoad(
            /* [in] */ ULONG,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC*
        ) override
        {
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE ReportFault(
            /* [in] */ FABRIC_FAULT_TYPE
        ) override
        {
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE ReportMoveCost(
            /* [in] */ FABRIC_MOVE_COST
        ) override
        {
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE ReportInstanceHealth(
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) override;

        virtual HRESULT STDMETHODCALLTYPE ReportPartitionHealth(
            /* [in] */ const FABRIC_HEALTH_INFORMATION *
        ) override
        {
            return E_NOTIMPL;
        }
    };
}}
