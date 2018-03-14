// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetPartitionListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetPartitionListResult :
        public IFabricGetPartitionListResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetPartitionListResult)

        BEGIN_COM_INTERFACE_LIST(ComGetPartitionListResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetPartitionListResult2)
            COM_INTERFACE_ITEM(IID_IFabricGetPartitionListResult, IFabricGetPartitionListResult)
            COM_INTERFACE_ITEM(IID_IFabricGetPartitionListResult2, IFabricGetPartitionListResult2)
            COM_INTERFACE_ITEM(CLSID_ComGetPartitionListResult, ComGetPartitionListResult)
        END_COM_INTERFACE_LIST()

    public:
        ComGetPartitionListResult(
            std::vector<ServiceModel::ServicePartitionQueryResult> && partitionList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetPartitionListResult methods
        // 
        const FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_PartitionList( void);

        // 
        // IFabricGetPartitionListResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST> partitionList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetPartitionListResult> ComGetPartitionListResultCPtr;
}
