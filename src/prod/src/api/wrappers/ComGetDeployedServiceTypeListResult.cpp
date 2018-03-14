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
    ComGetDeployedServiceTypeListResult::ComGetDeployedServiceTypeListResult(
        vector<DeployedServiceTypeQueryResult> && deployedServiceTypeList)
        : deployedServiceTypeList_(),
        heap_()
    {
        deployedServiceTypeList_ = heap_.AddItem<FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedServiceTypeQueryResult,
            FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedServiceTypeList), 
                *deployedServiceTypeList_);
    }

    const FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedServiceTypeListResult::get_DeployedServiceTypeList(void)
    {
        return deployedServiceTypeList_.GetRawPointer();
    }
}
