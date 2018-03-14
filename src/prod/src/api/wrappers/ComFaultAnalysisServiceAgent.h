// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {14aaebfc-62fb-4469-82d3-f79e7181416a}
    static const GUID CLSID_ComFaultAnalysisServiceAgent = 
        {0x14aaebfc,0x62fb,0x4469,{0x82,0xd3,0xf7,0x9e,0x71,0x81,0x41,0x6a}};

    class ComFaultAnalysisServiceAgent :
        public IFabricFaultAnalysisServiceAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFaultAnalysisServiceAgent)

        BEGIN_COM_INTERFACE_LIST(ComFaultAnalysisServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricFaultAnalysisServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricFaultAnalysisServiceAgent, IFabricFaultAnalysisServiceAgent)
            COM_INTERFACE_ITEM(CLSID_ComFaultAnalysisServiceAgent, ComFaultAnalysisServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComFaultAnalysisServiceAgent(IFaultAnalysisServiceAgentPtr const & impl);
        virtual ~ComFaultAnalysisServiceAgent();

        IFaultAnalysisServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricFaultAnalysisServiceAgent methods
        //

        HRESULT STDMETHODCALLTYPE RegisterFaultAnalysisService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricFaultAnalysisService *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterFaultAnalysisService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

    private:

        IFaultAnalysisServiceAgentPtr impl_;
    };
}

