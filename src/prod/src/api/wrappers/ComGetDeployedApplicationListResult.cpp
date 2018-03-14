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
    ComGetDeployedApplicationListResult::ComGetDeployedApplicationListResult(
        vector<DeployedApplicationQueryResult> && deployedApplicationList)
        : deployedApplicationList_(),
        heap_()
    {
        deployedApplicationList_ = heap_.AddItem<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedApplicationQueryResult,
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedApplicationList), 
                *deployedApplicationList_);
    }

    const FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedApplicationListResult::get_DeployedApplicationList(void)
    {
        return deployedApplicationList_.GetRawPointer();
    }
}
