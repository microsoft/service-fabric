// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetApplicationListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetApplicationListResult :
        public IFabricGetApplicationListResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetApplicationListResult)

        BEGIN_COM_INTERFACE_LIST(ComGetApplicationListResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetApplicationListResult2)
            COM_INTERFACE_ITEM(IID_IFabricGetApplicationListResult, IFabricGetApplicationListResult)
            COM_INTERFACE_ITEM(IID_IFabricGetApplicationListResult2, IFabricGetApplicationListResult2)
            COM_INTERFACE_ITEM(CLSID_ComGetApplicationListResult, ComGetApplicationListResult)
        END_COM_INTERFACE_LIST()
       
    public:
        ComGetApplicationListResult(
            std::vector<ServiceModel::ApplicationQueryResult> && applicationList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetApplicationListResult methods
        // 
        const FABRIC_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ApplicationList( void);

        // 
        // IFabricGetApplicationListResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_APPLICATION_QUERY_RESULT_LIST> applicationList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetApplicationListResult> ComGetApplicationListResultCPtr;
}
