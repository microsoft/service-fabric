// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(PartitionHealthStateChunk)

StringLiteral const TraceSource("PartitionHealthStateChunk");

PartitionHealthStateChunk::PartitionHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , partitionId_(Guid::Empty())
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
    , replicaHealthStateChunks_()
{
}

PartitionHealthStateChunk::PartitionHealthStateChunk(
    Common::Guid partitionId,
    FABRIC_HEALTH_STATE healthState,
    ReplicaHealthStateChunkList && replicaHealthStateChunks)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , partitionId_(partitionId)
    , healthState_(healthState)
    , replicaHealthStateChunks_(move(replicaHealthStateChunks))
{
}

PartitionHealthStateChunk::~PartitionHealthStateChunk()
{
}

ErrorCode PartitionHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_PARTITION_HEALTH_STATE_CHUNK & publicPartitionHealthStateChunk) const
{
    publicPartitionHealthStateChunk.PartitionId = partitionId_.AsGUID();

    publicPartitionHealthStateChunk.HealthState = healthState_;

    // ReplicaHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST>();
        auto error = replicaHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ReplicaHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicPartitionHealthStateChunk.ReplicaHealthStateChunks = publicInner.GetRawPointer();
    }

    return ErrorCode::Success();
}

ErrorCode PartitionHealthStateChunk::FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_CHUNK const & publicPartitionHealthStateChunk)
{
    partitionId_ = Guid(publicPartitionHealthStateChunk.PartitionId);

    healthState_ = publicPartitionHealthStateChunk.HealthState;

    auto error = replicaHealthStateChunks_.FromPublicApi(publicPartitionHealthStateChunk.ReplicaHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}

void PartitionHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("PartitionHealthStateChunk({0}: {1}, {2} replicas)", partitionId_, healthState_, replicaHealthStateChunks_.Count);
}
