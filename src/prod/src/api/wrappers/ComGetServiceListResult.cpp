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
    ComGetServiceListResult::ComGetServiceListResult(
        vector<ServiceQueryResult> && serviceList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : serviceList_()
        , pagingStatus_()
        , heap_()
    {
        serviceList_ = heap_.AddItem<FABRIC_SERVICE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceQueryResult,
            FABRIC_SERVICE_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_QUERY_RESULT_LIST>(
                heap_, 
                move(serviceList), 
                *serviceList_);
        
        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_SERVICE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetServiceListResult::get_ServiceList(void)
    {
        return serviceList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetServiceListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
