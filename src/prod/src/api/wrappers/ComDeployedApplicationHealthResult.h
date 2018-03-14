// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // d0bc8c96-d531-4152-a51b-d9dda3a42eff 
    static const GUID CLSID_ComDeployedApplicationHealthResult = 
        { 0xd0bc8c96, 0xd531, 0x4152, {0xa5, 0x1b, 0xd9, 0xdd, 0xa3, 0xa4, 0x2e, 0xff} };

    class ComDeployedApplicationHealthResult :
        public IFabricDeployedApplicationHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComDeployedApplicationHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComDeployedApplicationHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricDeployedApplicationHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricDeployedApplicationHealthResult, IFabricDeployedApplicationHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComDeployedApplicationHealthResult, ComDeployedApplicationHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComDeployedApplicationHealthResult(ServiceModel::DeployedApplicationHealth const & deployedApplicationHealth);
        virtual ~ComDeployedApplicationHealthResult();

        // 
        // IFabricDeployedApplicationHealthResult methods
        // 
        const FABRIC_DEPLOYED_APPLICATION_HEALTH * STDMETHODCALLTYPE get_DeployedApplicationHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_DEPLOYED_APPLICATION_HEALTH> deployedApplicationHealth_;
    };
}
