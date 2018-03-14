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
    ComGetDeployedServiceManifestListResult::ComGetDeployedServiceManifestListResult(
        vector<DeployedServiceManifestQueryResult> && deployedServiceManifestList)
        : deployedServiceManifestList_(),
        heap_()
    {
        deployedServiceManifestList_ = heap_.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedServiceManifestQueryResult,
            FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedServiceManifestList), 
                *deployedServiceManifestList_);
    }

    const FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedServiceManifestListResult::get_DeployedServicePackageList(void)
    {
        return deployedServiceManifestList_.GetRawPointer();
    }
}
