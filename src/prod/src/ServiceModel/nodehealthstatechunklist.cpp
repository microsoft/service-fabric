// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(NodeHealthStateChunkList)

StringLiteral const TraceSource("NodeHealthStateChunkList");

NodeHealthStateChunkList::NodeHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

NodeHealthStateChunkList::NodeHealthStateChunkList(
    ULONG totalCount,
    std::vector<NodeHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

NodeHealthStateChunkList::~NodeHealthStateChunkList()
{
}

ErrorCode NodeHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __inout FABRIC_NODE_HEALTH_STATE_CHUNK_LIST & publicNodeHealthStateChunkList) const
{
    publicNodeHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<NodeHealthStateChunk, FABRIC_NODE_HEALTH_STATE_CHUNK, FABRIC_NODE_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicNodeHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode NodeHealthStateChunkList::FromPublicApi(FABRIC_NODE_HEALTH_STATE_CHUNK_LIST const * publicNodeHealthStateChunkList)
{
    if (publicNodeHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicNodeHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<NodeHealthStateChunk, FABRIC_NODE_HEALTH_STATE_CHUNK_LIST>(publicNodeHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
