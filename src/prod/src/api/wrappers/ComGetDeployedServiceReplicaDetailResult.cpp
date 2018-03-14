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
    ComGetDeployedServiceReplicaDetailResult::ComGetDeployedServiceReplicaDetailResult(
        DeployedServiceReplicaDetailQueryResult && queryResult)
        : result_(),
        heap_()
    {
        result_ = heap_.AddItem<FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM>();
        queryResult.ToPublicApi(heap_, *result_.GetRawPointer());
    }

    const FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM *STDMETHODCALLTYPE ComGetDeployedServiceReplicaDetailResult::get_ReplicaDetail(void)
    {
        return result_.GetRawPointer();
    }
}
