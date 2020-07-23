// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // This GUID should be the same for all list results
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetDeployedNetworkListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetDeployedNetworkListResult :
        public IFabricGetDeployedNetworkListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedNetworkListResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedNetworkListResult,
            IID_IFabricGetDeployedNetworkListResult,
            IFabricGetDeployedNetworkListResult,
            CLSID_ComGetDeployedNetworkListResult,
            ComGetDeployedNetworkListResult)

    public:
        ComGetDeployedNetworkListResult(
            std::vector<ServiceModel::DeployedNetworkQueryResult> && deployedNetworkList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetDeployedNetworkListResult methods
        // 
        const FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_DeployedNetworkList( void );
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void );

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_LIST> deployedNetworkList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedNetworkListResult> ComGetDeployedNetworkListResultCPtr;
}