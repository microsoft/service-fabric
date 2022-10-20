// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ComQueryResult
        : public IQueryResult
        , private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComQueryResult)

        BEGIN_COM_INTERFACE_LIST(ComQueryResult)  
            COM_INTERFACE_ITEM(IID_IUnknown,IQueryResult)                         
            COM_INTERFACE_ITEM(IID_IQueryResult,IQueryResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComQueryResult(ServiceModel::QueryResult && nativeResult);

        const FABRIC_QUERY_RESULT * STDMETHODCALLTYPE get_Result(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_QUERY_RESULT> queryResult_;
    };
}
