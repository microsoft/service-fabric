// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ReplicaHealthStateChunkList)

StringLiteral const TraceSource("ReplicaHealthStateChunkList");

ReplicaHealthStateChunkList::ReplicaHealthStateChunkList()
    : HealthStateChunkList()
    , items_()
{
}

ReplicaHealthStateChunkList::ReplicaHealthStateChunkList(
    ULONG totalCount,
    std::vector<ReplicaHealthStateChunk> && items)
    : HealthStateChunkList(totalCount)
    , items_(move(items))
{
}

ReplicaHealthStateChunkList::~ReplicaHealthStateChunkList()
{
}

ErrorCode ReplicaHealthStateChunkList::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST & publicReplicaHealthStateChunkList) const
{
    publicReplicaHealthStateChunkList.TotalCount = this->TotalCount;
    auto error = PublicApiHelper::ToPublicApiList<ReplicaHealthStateChunk, FABRIC_REPLICA_HEALTH_STATE_CHUNK, FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST>(
        heap,
        items_,
        publicReplicaHealthStateChunkList);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

Common::ErrorCode ReplicaHealthStateChunkList::FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST const * publicReplicaHealthStateChunkList)
{
    if (publicReplicaHealthStateChunkList == NULL)
    {
        return ErrorCode::Success();
    }

    this->TotalCount = publicReplicaHealthStateChunkList->TotalCount;
    auto error = PublicApiHelper::FromPublicApiList<ReplicaHealthStateChunk, FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST>(publicReplicaHealthStateChunkList, items_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}
