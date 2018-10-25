// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ApplicationHealthStateChunkList)

StringLiteral const TraceSource("ApplicationHealthStateChunkList");

ApplicationHealthStateChunkList::ApplicationHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

ApplicationHealthStateChunkList::ApplicationHealthStateChunkList(
    ULONG totalCount,
    std::vector<ApplicationHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

ApplicationHealthStateChunkList::~ApplicationHealthStateChunkList()
{
}

ErrorCode ApplicationHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST & publicApplicationHealthStateChunkList) const
{
    publicApplicationHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<ApplicationHealthStateChunk, FABRIC_APPLICATION_HEALTH_STATE_CHUNK, FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicApplicationHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode ApplicationHealthStateChunkList::FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST const * publicApplicationHealthStateChunkList)
{
    if (publicApplicationHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicApplicationHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<ApplicationHealthStateChunk, FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST>(publicApplicationHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
