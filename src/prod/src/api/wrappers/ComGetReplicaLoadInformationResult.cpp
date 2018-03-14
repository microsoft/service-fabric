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
    ComGetReplicaLoadInformationResult::ComGetReplicaLoadInformationResult(
        ReplicaLoadInformationQueryResult const & replicaLoadInformation)
        : replicaLoadInformation_()
        , heap_()
    {
        replicaLoadInformation_ = heap_.AddItem<FABRIC_REPLICA_LOAD_INFORMATION>();

        replicaLoadInformation.ToPublicApi(heap_, *replicaLoadInformation_);
    }

    const FABRIC_REPLICA_LOAD_INFORMATION *STDMETHODCALLTYPE ComGetReplicaLoadInformationResult::get_ReplicaLoadInformation(void)
    {
        return replicaLoadInformation_.GetRawPointer();
    }
}
