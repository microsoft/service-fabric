// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceHealthStateChunkList)

StringLiteral const TraceSource("ServiceHealthStateChunkList");

ServiceHealthStateChunkList::ServiceHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

ServiceHealthStateChunkList::ServiceHealthStateChunkList(
    ULONG totalCount,
    std::vector<ServiceHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

ServiceHealthStateChunkList::~ServiceHealthStateChunkList()
{
}

ErrorCode ServiceHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SERVICE_HEALTH_STATE_CHUNK_LIST & publicServiceHealthStateChunkList) const
{
    publicServiceHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<ServiceHealthStateChunk, FABRIC_SERVICE_HEALTH_STATE_CHUNK, FABRIC_SERVICE_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicServiceHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode ServiceHealthStateChunkList::FromPublicApi(FABRIC_SERVICE_HEALTH_STATE_CHUNK_LIST const * publicServiceHealthStateChunkList)
{
    if (publicServiceHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicServiceHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<ServiceHealthStateChunk, FABRIC_SERVICE_HEALTH_STATE_CHUNK_LIST>(publicServiceHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
