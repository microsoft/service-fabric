// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComClusterHealthResult::ComClusterHealthResult(
    ClusterHealth const & clusterHealth)
    : IFabricClusterHealthResult()
    , ComUnknownBase()
    , heap_()
    , clusterHealth_()
{
    clusterHealth_ = heap_.AddItem<FABRIC_CLUSTER_HEALTH>();
    clusterHealth.ToPublicApi(heap_, *clusterHealth_);
}

ComClusterHealthResult::~ComClusterHealthResult()
{
}

const FABRIC_CLUSTER_HEALTH * STDMETHODCALLTYPE ComClusterHealthResult::get_ClusterHealth()
{
    return clusterHealth_.GetRawPointer();
}
