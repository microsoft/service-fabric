// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ClusterHealthChunk)

StringLiteral const TraceSource("ClusterHealthChunk");

ClusterHealthChunk::ClusterHealthChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
    , nodeHealthStateChunks_()
    , applicationHealthStateChunks_()
{
}

ClusterHealthChunk::ClusterHealthChunk(
    FABRIC_HEALTH_STATE healthState,
    NodeHealthStateChunkList && nodeHealthStateChunks,
    ApplicationHealthStateChunkList && applicationHealthStateChunks)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , healthState_(healthState)
    , nodeHealthStateChunks_(move(nodeHealthStateChunks))
    , applicationHealthStateChunks_(move(applicationHealthStateChunks))
{
}

ClusterHealthChunk::ClusterHealthChunk(ClusterHealthChunk && other)
    : Common::IFabricJsonSerializable(move(other))
    , Serialization::FabricSerializable(move(other))
    , Common::ISizeEstimator(move(other))
    , healthState_(move(other.healthState_))
    , nodeHealthStateChunks_(move(other.nodeHealthStateChunks_))
    , applicationHealthStateChunks_(move(other.applicationHealthStateChunks_))
{
}

ClusterHealthChunk & ClusterHealthChunk::operator =(ClusterHealthChunk && other)
{
    if (this != &other)
    {
        healthState_ = move(other.healthState_);
        nodeHealthStateChunks_ = move(other.nodeHealthStateChunks_);
        applicationHealthStateChunks_ = move(other.applicationHealthStateChunks_);
    }

    Common::IFabricJsonSerializable::operator=(move(other));
    Serialization::FabricSerializable::operator=(move(other));
    Common::ISizeEstimator::operator=(move(other));
    return *this;
}

ClusterHealthChunk::~ClusterHealthChunk()
{
}

ErrorCode ClusterHealthChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_CLUSTER_HEALTH_CHUNK & publicClusterStateChunk) const
{
    publicClusterStateChunk.HealthState = healthState_;

    // NodeHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_NODE_HEALTH_STATE_CHUNK_LIST>();
        auto error = nodeHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "NodeHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicClusterStateChunk.NodeHealthStateChunks = publicInner.GetRawPointer();
    }

    // ApplicationHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST>();
        auto error = applicationHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ApplicationHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicClusterStateChunk.ApplicationHealthStateChunks = publicInner.GetRawPointer();
    }

    return ErrorCode::Success();
}

ErrorCode ClusterHealthChunk::FromPublicApi(FABRIC_CLUSTER_HEALTH_CHUNK const & publicClusterStateChunk)
{
    healthState_ = publicClusterStateChunk.HealthState;

    auto error = nodeHealthStateChunks_.FromPublicApi(publicClusterStateChunk.NodeHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = applicationHealthStateChunks_.FromPublicApi(publicClusterStateChunk.ApplicationHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void ClusterHealthChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ClusterHealthChunk({0}, {1} apps, {2} nodes)", healthState_, applicationHealthStateChunks_.Count, nodeHealthStateChunks_.Count);
}
