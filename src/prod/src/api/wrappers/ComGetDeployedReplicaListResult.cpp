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
    ComGetDeployedReplicaListResult::ComGetDeployedReplicaListResult(
        vector<DeployedServiceReplicaQueryResult> && deployedReplicaList)
        : deployedReplicaList_(),
        heap_()
    {
        deployedReplicaList_ = heap_.AddItem<FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            DeployedServiceReplicaQueryResult,
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM,
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST>(
                heap_, 
                move(deployedReplicaList), 
                *deployedReplicaList_);
    }

    const FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetDeployedReplicaListResult::get_DeployedReplicaList(void)
    {
        return deployedReplicaList_.GetRawPointer();
    }
}
