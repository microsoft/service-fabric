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
    ComGetApplicationTypePagedListResult::ComGetApplicationTypePagedListResult(
        vector<ApplicationTypeQueryResult> && applicationTypeList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : applicationTypeList_()
        , pagingStatus_()
        , heap_()
    {
        applicationTypeList_ = heap_.AddItem<FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ApplicationTypeQueryResult,
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM,
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST>(
                heap_, 
                move(applicationTypeList), 
                *applicationTypeList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetApplicationTypePagedListResult::get_ApplicationTypePagedList(void)
    {
        return applicationTypeList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetApplicationTypePagedListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
