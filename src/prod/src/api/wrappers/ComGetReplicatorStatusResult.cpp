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
    ComGetReplicatorStatusResult::ComGetReplicatorStatusResult(
        ReplicatorStatusQueryResultSPtr && queryResult)
        : result_(),
        heap_()
    {
        result_ = heap_.AddItem<FABRIC_REPLICATOR_STATUS_QUERY_RESULT>();
        queryResult->ToPublicApi(heap_, *result_.GetRawPointer());
    }

    const FABRIC_REPLICATOR_STATUS_QUERY_RESULT *STDMETHODCALLTYPE ComGetReplicatorStatusResult::get_ReplicatorStatus(void)
    {
        return result_.GetRawPointer();
    }
}
