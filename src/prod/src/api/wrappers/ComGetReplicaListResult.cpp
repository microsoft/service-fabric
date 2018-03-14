// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Api
{
    ComGetReplicaListResult::ComGetReplicaListResult(
        vector<ServiceReplicaQueryResult> && replicaList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : replicaList_()
        , pagingStatus_()
        , heap_()
    {
        replicaList_ = heap_.AddItem<FABRIC_SERVICE_REPLICA_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceReplicaQueryResult,
            FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_REPLICA_QUERY_RESULT_LIST>(
                heap_, 
                move(replicaList), 
                *replicaList_);
        
        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_SERVICE_REPLICA_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetReplicaListResult::get_ReplicaList(void)
    {
        return replicaList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetReplicaListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
