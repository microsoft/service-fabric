// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComGetClusterHealthChunkResult::ComGetClusterHealthChunkResult(
    ClusterHealthChunk const & clusterHealthChunk)
    : IFabricGetClusterHealthChunkResult()
    , ComUnknownBase()
    , heap_()
    , clusterHealthChunk_()
{
    clusterHealthChunk_ = heap_.AddItem<FABRIC_CLUSTER_HEALTH_CHUNK>();
    clusterHealthChunk.ToPublicApi(heap_, *clusterHealthChunk_);
}

ComGetClusterHealthChunkResult::~ComGetClusterHealthChunkResult()
{
}

const FABRIC_CLUSTER_HEALTH_CHUNK * STDMETHODCALLTYPE ComGetClusterHealthChunkResult::get_ClusterHealthChunk()
{
    return clusterHealthChunk_.GetRawPointer();
}
