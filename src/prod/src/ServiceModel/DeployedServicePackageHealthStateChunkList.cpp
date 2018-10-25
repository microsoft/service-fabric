// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageHealthStateChunkList)

StringLiteral const TraceSource("DeployedServicePackageHealthStateChunkList");

DeployedServicePackageHealthStateChunkList::DeployedServicePackageHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

DeployedServicePackageHealthStateChunkList::DeployedServicePackageHealthStateChunkList(
    ULONG totalCount,
    std::vector<DeployedServicePackageHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

DeployedServicePackageHealthStateChunkList::~DeployedServicePackageHealthStateChunkList()
{
}

ErrorCode DeployedServicePackageHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_LIST & publicDeployedServicePackageHealthStateChunkList) const
{
    publicDeployedServicePackageHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<DeployedServicePackageHealthStateChunk, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicDeployedServicePackageHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackageHealthStateChunkList::FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_LIST const * publicDeployedServicePackageHealthStateChunkList)
{
    if (publicDeployedServicePackageHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicDeployedServicePackageHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<DeployedServicePackageHealthStateChunk, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_LIST>(publicDeployedServicePackageHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
