// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {3D8D24E3-C9D3-4F61-AA5E-9DFC298AAD61}
    static const GUID CLSID_ComFabricOrchestrationUpgradeStatusResult =
    { 0x3d8d24e3, 0xc9d3, 0x4f61, { 0xaa, 0x5e, 0x9d, 0xfc, 0x29, 0x8a, 0xad, 0x61 } };

    class ComFabricOrchestrationUpgradeStatusResult
        : public IFabricOrchestrationUpgradeStatusResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricOrchestrationUpgradeStatusResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricOrchestrationUpgradeStatusResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOrchestrationUpgradeStatusResult)
            COM_INTERFACE_ITEM(IID_IFabricOrchestrationUpgradeStatusResult, IFabricOrchestrationUpgradeStatusResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricOrchestrationUpgradeStatusResult, ComFabricOrchestrationUpgradeStatusResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricOrchestrationUpgradeStatusResult(
            IFabricOrchestrationUpgradeStatusResultPtr const &impl);

        IFabricOrchestrationUpgradeStatusResultPtr const & get_Impl() const { return impl_; }

        FABRIC_ORCHESTRATION_UPGRADE_PROGRESS * STDMETHODCALLTYPE get_Progress();

    private:
        IFabricOrchestrationUpgradeStatusResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
