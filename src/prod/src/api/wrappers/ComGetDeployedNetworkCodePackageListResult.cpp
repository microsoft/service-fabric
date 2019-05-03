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
    ComGetDeployedNetworkCodePackageListResult::ComGetDeployedNetworkCodePackageListResult(
        vector<DeployedNetworkCodePackageQueryResult> && deployedNetworkCodePackageList,
        PagingStatusUPtr && pagingStatus)
        : deployedNetworkCodePackageList_()
        , pagingStatus_()
        , heap_()
    {
        deployedNetworkCodePackageList_ = heap_.AddItem<FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedNetworkCodePackageQueryResult,
            FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedNetworkCodePackageList),
                *deployedNetworkCodePackageList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedNetworkCodePackageListResult::get_DeployedNetworkCodePackageList()
    {
        return deployedNetworkCodePackageList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetDeployedNetworkCodePackageListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
