// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetServiceListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetServiceListResult :
        public IFabricGetServiceListResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetServiceListResult)

        BEGIN_COM_INTERFACE_LIST(ComGetServiceListResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetServiceListResult2)
            COM_INTERFACE_ITEM(IID_IFabricGetServiceListResult, IFabricGetServiceListResult)
            COM_INTERFACE_ITEM(IID_IFabricGetServiceListResult2, IFabricGetServiceListResult2)
            COM_INTERFACE_ITEM(CLSID_ComGetServiceListResult, ComGetServiceListResult)
        END_COM_INTERFACE_LIST()

    public:
        ComGetServiceListResult(
            std::vector<ServiceModel::ServiceQueryResult> && serviceList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetServiceListResult methods
        // 
        const FABRIC_SERVICE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ServiceList( void);
        
        // 
        // IFabricGetServiceListResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_QUERY_RESULT_LIST> serviceList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetServiceListResult> ComGetServiceListResultCPtr;
}
