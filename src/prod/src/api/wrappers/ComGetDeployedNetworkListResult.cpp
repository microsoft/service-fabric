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
    ComGetDeployedNetworkListResult::ComGetDeployedNetworkListResult(
        vector<DeployedNetworkQueryResult> && deployedNetworkList,
        PagingStatusUPtr && pagingStatus)
        : deployedNetworkList_()
        , pagingStatus_()
        , heap_()
    {
        deployedNetworkList_ = heap_.AddItem<FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedNetworkQueryResult,
            FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedNetworkList),
                *deployedNetworkList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedNetworkListResult::get_DeployedNetworkList()
    {
        return deployedNetworkList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetDeployedNetworkListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
