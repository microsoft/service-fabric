// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace ServiceModel;

ComInternalQueryResult::ComInternalQueryResult(QueryResult && nativeResult)
    : heap_()
    , queryResult_()
    , pagingStatus_()
{
    queryResult_ = heap_.AddItem<FABRIC_QUERY_RESULT>();  
    nativeResult.ToPublicApi(heap_, *queryResult_);

    auto pagingStatus = nativeResult.MovePagingStatus();
    if (pagingStatus)
    {
        pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
        pagingStatus->ToPublicApi(heap_, *pagingStatus_);
    }
}

const FABRIC_QUERY_RESULT * STDMETHODCALLTYPE ComInternalQueryResult::get_Result(void)
{
    return queryResult_.GetRawPointer();        
}

const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComInternalQueryResult::get_PagingStatus()
{
    return pagingStatus_.GetRawPointer();
}


