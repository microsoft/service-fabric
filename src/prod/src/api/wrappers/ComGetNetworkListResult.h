// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
	// This GUID should be the same for all list results
	// {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetNetworkListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetNetworkListResult :
        public IFabricGetNetworkListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetNetworkListResult)

        COM_INTERFACE_LIST2(
            ComGetNetworkListResult,
            IID_IFabricGetNetworkListResult,
            IFabricGetNetworkListResult,
            CLSID_ComGetNetworkListResult,
            ComGetNetworkListResult)

    public:
        ComGetNetworkListResult(
            std::vector<ServiceModel::NetworkInformation> && networkList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetNetworkListResult methods
        // 
        const FABRIC_NETWORK_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_NetworkList( void );
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void );

    private:
        Common::ReferencePointer<FABRIC_NETWORK_QUERY_RESULT_LIST> networkList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetNetworkListResult> ComGetNetworkListResultCPtr;
}