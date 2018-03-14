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
    ComGetComposeDeploymentStatusListResult::ComGetComposeDeploymentStatusListResult(
        vector<ComposeDeploymentStatusQueryResult> && queryResultList,
        PagingStatusUPtr && pagingStatus)
        : dockerComposeDeploymentStatusList_()
        , pagingStatus_()
        , heap_()
    {
        dockerComposeDeploymentStatusList_ = heap_.AddItem<FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ComposeDeploymentStatusQueryResult,
            FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM,
            FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST>(
                heap_,
                move(queryResultList),
                *dockerComposeDeploymentStatusList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetComposeDeploymentStatusListResult::get_ComposeDeploymentStatusQueryList(void)
    {
        return dockerComposeDeploymentStatusList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetComposeDeploymentStatusListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
