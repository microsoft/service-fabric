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
    ComGetApplicationListResult::ComGetApplicationListResult(
        vector<ApplicationQueryResult> && applicationList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : applicationList_()
        , pagingStatus_()
        , heap_()
    {
        applicationList_ = heap_.AddItem<FABRIC_APPLICATION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ApplicationQueryResult,
            FABRIC_APPLICATION_QUERY_RESULT_ITEM,
            FABRIC_APPLICATION_QUERY_RESULT_LIST>(
                heap_, 
                move(applicationList), 
                *applicationList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetApplicationListResult::get_ApplicationList(void)
    {
        return applicationList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetApplicationListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
