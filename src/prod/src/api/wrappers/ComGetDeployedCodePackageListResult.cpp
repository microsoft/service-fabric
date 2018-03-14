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
    ComGetDeployedCodePackageListResult::ComGetDeployedCodePackageListResult(
        vector<DeployedCodePackageQueryResult> && deployedCodePackageList)
        : deployedCodePackageList_(),
        heap_()
    {
        deployedCodePackageList_ = heap_.AddItem<FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedCodePackageQueryResult,
            FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedCodePackageList), 
                *deployedCodePackageList_);
    }

    const FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedCodePackageListResult::get_DeployedCodePackageList(void)
    {
        return deployedCodePackageList_.GetRawPointer();
    }
}
