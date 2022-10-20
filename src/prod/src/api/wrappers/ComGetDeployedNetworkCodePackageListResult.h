// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // This GUID should be the same for all list results
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetDeployedNetworkCodePackageListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetDeployedNetworkCodePackageListResult :
        public IFabricGetDeployedNetworkCodePackageListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedNetworkCodePackageListResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedNetworkCodePackageListResult,
            IID_IFabricGetDeployedNetworkCodePackageListResult,
            IFabricGetDeployedNetworkCodePackageListResult,
            CLSID_ComGetDeployedNetworkCodePackageListResult,
            ComGetDeployedNetworkCodePackageListResult)

    public:
        ComGetDeployedNetworkCodePackageListResult(
            std::vector<ServiceModel::DeployedNetworkCodePackageQueryResult> && deployedNetworkCodePackageList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetDeployedNetworkCodePackageListResult methods
        // 
        const FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_DeployedNetworkCodePackageList( void );
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void );

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST> deployedNetworkCodePackageList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedNetworkCodePackageListResult> ComGetDeployedNetworkCodePackageListResultCPtr;
}