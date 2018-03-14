// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetDeployedServiceManifestListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetDeployedServiceManifestListResult :
        public IFabricGetDeployedServicePackageListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedServiceManifestListResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedServiceManifestListResult,
            IID_IFabricGetDeployedServicePackageListResult,
            IFabricGetDeployedServicePackageListResult,
            CLSID_ComGetDeployedServiceManifestListResult,
            ComGetDeployedServiceManifestListResult)

    public:
        explicit ComGetDeployedServiceManifestListResult(std::vector<ServiceModel::DeployedServiceManifestQueryResult> && deployedServiceManifestList);

        // 
        // IFabricGetDeployedServicePackageListResult methods
        // 
        const FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_DeployedServicePackageList( void);

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST> deployedServiceManifestList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedServiceManifestListResult> ComGetDeployedServiceManifestListResultCPtr;
}
