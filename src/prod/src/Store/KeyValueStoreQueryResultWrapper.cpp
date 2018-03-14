// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Store;

KeyValueStoreQueryResultWrapper::KeyValueStoreQueryResultWrapper(
    shared_ptr<KeyValueStoreQueryResult> && queryResult)
    : queryResult_(queryResult)
{
}

void KeyValueStoreQueryResultWrapper::GetResult(
    __inout ScopedHeap & heap, 
    __out FABRIC_REPLICA_STATUS_QUERY_RESULT & result)
{
    queryResult_->ToPublicApi(heap, result);
}
