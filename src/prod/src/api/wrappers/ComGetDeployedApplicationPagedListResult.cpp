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
    ComGetDeployedApplicationPagedListResult::ComGetDeployedApplicationPagedListResult(
        vector<DeployedApplicationQueryResult> && deployedApplicationList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : deployedApplicationList_()
        , heap_()
        , pagingStatus_()
    {
        deployedApplicationList_ = heap_.AddItem<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedApplicationQueryResult,
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST>(
                heap_,
                move(deployedApplicationList),
                *deployedApplicationList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedApplicationPagedListResult::get_DeployedApplicationPagedList(void)
    {
        return deployedApplicationList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetDeployedApplicationPagedListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
