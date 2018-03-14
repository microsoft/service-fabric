// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetReplicaListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetReplicaListResult :
        public IFabricGetReplicaListResult2,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetReplicaListResult)

        BEGIN_COM_INTERFACE_LIST(ComGetReplicaListResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetReplicaListResult2)
            COM_INTERFACE_ITEM(IID_IFabricGetReplicaListResult, IFabricGetReplicaListResult)
            COM_INTERFACE_ITEM(IID_IFabricGetReplicaListResult2, IFabricGetReplicaListResult2)
            COM_INTERFACE_ITEM(CLSID_ComGetReplicaListResult, ComGetReplicaListResult)
        END_COM_INTERFACE_LIST()

    public:
        ComGetReplicaListResult(
            std::vector<ServiceModel::ServiceReplicaQueryResult> && replicaList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        // 
        // IFabricGetReplicaListResult methods
        // 
        const FABRIC_SERVICE_REPLICA_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ReplicaList( void);

        // 
        // IFabricGetReplicaListResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_REPLICA_QUERY_RESULT_LIST> replicaList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetReplicaListResult> ComGetReplicaListResultCPtr;
}
