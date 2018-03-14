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
    ComGetPartitionListResult::ComGetPartitionListResult(
        vector<ServicePartitionQueryResult> && partitionList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : partitionList_()
        , pagingStatus_()
        , heap_()
    {
        partitionList_ = heap_.AddItem<FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServicePartitionQueryResult,
            FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST>(
                heap_, 
                move(partitionList), 
                *partitionList_);
        
        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetPartitionListResult::get_PartitionList(void)
    {
        return partitionList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetPartitionListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
