// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {CC4AD587-CDB3-4287-861C-71EE09FC8F39}
    static const GUID CLSID_ComTokenValidationServiceAgent = 
    { 0xcc4ad587, 0xcdb3, 0x4287, { 0x86, 0x1c, 0x71, 0xee, 0x9, 0xfc, 0x8f, 0x39 } };

    class ComTokenValidationServiceAgent :
        public IFabricTokenValidationServiceAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComTokenValidationServiceAgent)

        BEGIN_COM_INTERFACE_LIST(ComTokenValidationServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricTokenValidationServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricTokenValidationServiceAgent, IFabricTokenValidationServiceAgent)
            COM_INTERFACE_ITEM(CLSID_ComTokenValidationServiceAgent, ComTokenValidationServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComTokenValidationServiceAgent(ITokenValidationServiceAgentPtr const & impl);
        virtual ~ComTokenValidationServiceAgent();

        ITokenValidationServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricTokenValidationServiceAgent methods
        // 

        HRESULT STDMETHODCALLTYPE RegisterTokenValidationService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricTokenValidationService *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterTokenValidationService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

    private:
        ITokenValidationServiceAgentPtr impl_;
    };
}

