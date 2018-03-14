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
    ComGetClusterLoadInformationResult::ComGetClusterLoadInformationResult(
        ClusterLoadInformationQueryResult && clusterLoadInformation)
        : clusterLoadInformation_()
        , heap_()
    {
        clusterLoadInformation_ = heap_.AddItem<FABRIC_CLUSTER_LOAD_INFORMATION>();
        clusterLoadInformation.ToPublicApi(heap_, *clusterLoadInformation_.GetRawPointer());
    }

    const FABRIC_CLUSTER_LOAD_INFORMATION *STDMETHODCALLTYPE ComGetClusterLoadInformationResult::get_ClusterLoadInformation (void)
    {
        return clusterLoadInformation_.GetRawPointer();
    }
}
