// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // a6d57e96-4857-4a8d-b686-f8e1cffb61a3 
    static const GUID CLSID_ComDeployedServicePackageHealthResult = 
        { 0xa6d57e96, 0x4857, 0x4a8d, {0xb6, 0x86, 0xf8, 0xe1, 0xcf, 0xfb, 0x61, 0xa3} };

    class ComDeployedServicePackageHealthResult :
        public IFabricDeployedServicePackageHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComDeployedServicePackageHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComDeployedServicePackageHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricDeployedServicePackageHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricDeployedServicePackageHealthResult, IFabricDeployedServicePackageHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComDeployedServicePackageHealthResult, ComDeployedServicePackageHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComDeployedServicePackageHealthResult(ServiceModel::DeployedServicePackageHealth const & deployedServicePackageHealth);
        virtual ~ComDeployedServicePackageHealthResult();

        // 
        // IFabricDeployedServicePackageHealthResult methods
        // 
        const FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH * STDMETHODCALLTYPE get_DeployedServicePackageHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH> deployedServicePackageHealth_;
    };
}
