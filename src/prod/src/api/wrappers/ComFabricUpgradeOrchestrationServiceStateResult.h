// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5AAF9EC4-F461-41D0-975F-5F6CAB3CB258}
    static const GUID CLSID_ComFabricUpgradeOrchestrationServiceStateResult =
    { 0x5aaf9ec4, 0xf461, 0x41d0, { 0x97, 0x5f, 0x5f, 0x6c, 0xab, 0x3c, 0xb2, 0x58 } };

    class ComFabricUpgradeOrchestrationServiceStateResult
        : public IFabricUpgradeOrchestrationServiceStateResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricUpgradeOrchestrationServiceStateResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricUpgradeOrchestrationServiceStateResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricUpgradeOrchestrationServiceStateResult)
            COM_INTERFACE_ITEM(IID_IFabricUpgradeOrchestrationServiceStateResult, IFabricUpgradeOrchestrationServiceStateResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricUpgradeOrchestrationServiceStateResult, ComFabricUpgradeOrchestrationServiceStateResult)
        END_COM_INTERFACE_LIST()

    public:
		ComFabricUpgradeOrchestrationServiceStateResult(
			IFabricUpgradeOrchestrationServiceStateResultPtr const &impl);

		IFabricUpgradeOrchestrationServiceStateResultPtr const & get_Impl() const { return impl_; }

		FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE * STDMETHODCALLTYPE get_State();

    private:
		IFabricUpgradeOrchestrationServiceStateResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
