// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetNodeListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetNodeListResult :
        public IFabricGetNodeListResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetNodeListResult)

        BEGIN_COM_INTERFACE_LIST(ComGetNodeListResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetNodeListResult2)
            COM_INTERFACE_ITEM(IID_IFabricGetNodeListResult, IFabricGetNodeListResult)
            COM_INTERFACE_ITEM(IID_IFabricGetNodeListResult2, IFabricGetNodeListResult2)
            COM_INTERFACE_ITEM(CLSID_ComGetNodeListResult, ComGetNodeListResult)
        END_COM_INTERFACE_LIST()

    public:
        ComGetNodeListResult(
            std::vector<ServiceModel::NodeQueryResult> && nodeList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetNodeListResult methods
        // 
        const FABRIC_NODE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_NodeList( void);

        // 
        // IFabricGetNodeListResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void );

    private:
        Common::ReferencePointer<FABRIC_NODE_QUERY_RESULT_LIST> nodeList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetNodeListResult> ComGetNodeListResultCPtr;
}
