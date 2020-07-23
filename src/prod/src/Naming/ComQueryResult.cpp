// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;

    ComQueryResult::ComQueryResult(ServiceModel::QueryResult && nativeResult)
        : heap_(),
        queryResult_()
    {
        queryResult_ = heap_.AddItem<FABRIC_QUERY_RESULT>();  
        nativeResult.ToPublicApi(heap_, *queryResult_);
     }

    const FABRIC_QUERY_RESULT * STDMETHODCALLTYPE ComQueryResult::get_Result(void)
    {
        return queryResult_.GetRawPointer();        
    }
}
