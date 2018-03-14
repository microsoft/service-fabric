// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {A9B2FFB2-236D-4223-AF0B-84AB2C902788}
    static const GUID CLSID_ComUpgradeOrchestrationServiceAgent =
    { 0xa9b2ffb2, 0x236d, 0x4223, { 0xaf, 0xb, 0x84, 0xab, 0x2c, 0x90, 0x27, 0x88 } };

    class ComUpgradeOrchestrationServiceAgent :
        public IFabricUpgradeOrchestrationServiceAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComUpgradeOrchestrationServiceAgent)

            BEGIN_COM_INTERFACE_LIST(ComUpgradeOrchestrationServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricUpgradeOrchestrationServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricUpgradeOrchestrationServiceAgent, IFabricUpgradeOrchestrationServiceAgent)
            COM_INTERFACE_ITEM(CLSID_ComUpgradeOrchestrationServiceAgent, ComUpgradeOrchestrationServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComUpgradeOrchestrationServiceAgent(IUpgradeOrchestrationServiceAgentPtr const & impl);
        virtual ~ComUpgradeOrchestrationServiceAgent();

        IUpgradeOrchestrationServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricFaultAnalysisServiceAgent methods
        //

        HRESULT STDMETHODCALLTYPE RegisterUpgradeOrchestrationService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricUpgradeOrchestrationService *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterUpgradeOrchestrationService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

    private:
        IUpgradeOrchestrationServiceAgentPtr impl_;
    };
}

