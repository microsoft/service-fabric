// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComInternalQueryResult
        : public IInternalFabricQueryResult2
        , private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComInternalQueryResult)

        BEGIN_COM_INTERFACE_LIST(ComInternalQueryResult)  
            COM_INTERFACE_ITEM(IID_IUnknown,IInternalFabricQueryResult)                         
            COM_INTERFACE_ITEM(IID_IInternalFabricQueryResult,IInternalFabricQueryResult)
            COM_INTERFACE_ITEM(IID_IInternalFabricQueryResult2,IInternalFabricQueryResult2)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComInternalQueryResult(ServiceModel::QueryResult && nativeResult);

        // 
        // IInternalFabricQueryResult2 methods
        // 
        const FABRIC_QUERY_RESULT * STDMETHODCALLTYPE get_Result(void);

        // 
        // IInternalFabricQueryResult2 methods
        // 
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_QUERY_RESULT> queryResult_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
    };
}
