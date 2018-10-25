// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationHealthStateChunkList)

StringLiteral const TraceSource("DeployedApplicationHealthStateChunkList");

DeployedApplicationHealthStateChunkList::DeployedApplicationHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

DeployedApplicationHealthStateChunkList::DeployedApplicationHealthStateChunkList(
    ULONG totalCount,
    std::vector<DeployedApplicationHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

DeployedApplicationHealthStateChunkList::~DeployedApplicationHealthStateChunkList()
{
}

ErrorCode DeployedApplicationHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST & publicDeployedApplicationHealthStateChunkList) const
{
    publicDeployedApplicationHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<DeployedApplicationHealthStateChunk, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicDeployedApplicationHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationHealthStateChunkList::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST const * publicDeployedApplicationHealthStateChunkList)
{
    if (publicDeployedApplicationHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicDeployedApplicationHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<DeployedApplicationHealthStateChunk, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST>(publicDeployedApplicationHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
