// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(PartitionHealthStateChunkList)

StringLiteral const TraceSource("PartitionHealthStateChunkList");

PartitionHealthStateChunkList::PartitionHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

PartitionHealthStateChunkList::PartitionHealthStateChunkList(
    ULONG totalCount,
    std::vector<PartitionHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

PartitionHealthStateChunkList::~PartitionHealthStateChunkList()
{
}

ErrorCode PartitionHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST & publicPartitionHealthStateChunkList) const
{
    publicPartitionHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<PartitionHealthStateChunk, FABRIC_PARTITION_HEALTH_STATE_CHUNK, FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicPartitionHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode PartitionHealthStateChunkList::FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST const * publicPartitionHealthStateChunkList)
{
    if (publicPartitionHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicPartitionHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<PartitionHealthStateChunk, FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST>(publicPartitionHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
