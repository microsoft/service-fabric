// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {B051C95A-EEAC-47B9-8801-48486E96F976}
    static const GUID CLSID_ComGatewayResourceManagerAgent = 
    {0xb051c95a,0xeeac,0x47b9,{0x88,0x1,0x48,0x48,0x6e,0x96,0xf9,0x76}};

    class ComGatewayResourceManagerAgent :
        public IFabricGatewayResourceManagerAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComGatewayResourceManagerAgent)

        BEGIN_COM_INTERFACE_LIST(ComGatewayResourceManagerAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGatewayResourceManagerAgent)
            COM_INTERFACE_ITEM(IID_IFabricGatewayResourceManagerAgent, IFabricGatewayResourceManagerAgent)
            COM_INTERFACE_ITEM(CLSID_ComGatewayResourceManagerAgent, ComGatewayResourceManagerAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComGatewayResourceManagerAgent(IGatewayResourceManagerAgentPtr const & impl);
        virtual ~ComGatewayResourceManagerAgent();

        IGatewayResourceManagerAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricGatewayResourceManagerAgent methods
        //

        HRESULT STDMETHODCALLTYPE RegisterGatewayResourceManager(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricGatewayResourceManager *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterGatewayResourceManager(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

    private:

        IGatewayResourceManagerAgentPtr impl_;
    };
}

