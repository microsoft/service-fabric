// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ReplicaHealthStateChunk)

StringLiteral const TraceSource("ReplicaHealthStateChunk");

ReplicaHealthStateChunk::ReplicaHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , replicaOrInstanceId_(FABRIC_INVALID_REPLICA_ID)
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

ReplicaHealthStateChunk::ReplicaHealthStateChunk(
    FABRIC_REPLICA_ID replicaOrInstanceId,
    FABRIC_HEALTH_STATE healthState)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , replicaOrInstanceId_(replicaOrInstanceId)
    , healthState_(healthState)
{
}

ReplicaHealthStateChunk::~ReplicaHealthStateChunk()
{
}

ErrorCode ReplicaHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_REPLICA_HEALTH_STATE_CHUNK & publicReplicaHealthStateChunk) const
{
    UNREFERENCED_PARAMETER(heap);

    publicReplicaHealthStateChunk.HealthState = healthState_;
    publicReplicaHealthStateChunk.ReplicaOrInstanceId = replicaOrInstanceId_;
   
    return ErrorCode::Success();
}


ErrorCode ReplicaHealthStateChunk::FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_CHUNK const & publicReplicaHealthStateChunk)
{
    replicaOrInstanceId_ = publicReplicaHealthStateChunk.ReplicaOrInstanceId;
    healthState_ = publicReplicaHealthStateChunk.HealthState;
    
    return ErrorCode::Success();
}

void ReplicaHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ReplicaHealthStateChunk({0}: {1})", replicaOrInstanceId_, healthState_);
}



