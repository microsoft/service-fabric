// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TestCommon;
using namespace FabricTest;

void ComTestReplicaMap::ClearReplicaMap()
{
    AcquireWriteLock grab(lock_);
    ASSERT_IF(!asyncContextMap_.empty(), "asyncContextMap_ is not empty");
    replicas_.clear();
}

void ComTestReplicaMap::UpdateReplicaMap(FABRIC_REPLICA_ID replicaId)
{
    AcquireWriteLock grab(lock_);
    ComTestReplica newReplica;
    replicas_.insert(make_pair(replicaId, newReplica));
}

void ComTestReplicaMap::UpdateCatchupReplicaSet(std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & ccReplicas, std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & pcReplicas, std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & pcAndccReplicas)
{
    AcquireWriteLock grab(lock_);

    // Verify replica state transition
    for (auto replicaEntity : pcAndccReplicas)
    {
        auto replicaId = replicaEntity.first;
        auto replicaLSN = replicaEntity.second;
        replicas_[replicaId].UpdateCatchup(true, true, replicaId, replicaLSN);
    }

    for (auto replicaEntity : ccReplicas)
    {
        auto replicaId = replicaEntity.first;
        auto replicaLSN = replicaEntity.second;
        if (pcAndccReplicas.find(replicaId) == pcAndccReplicas.end())
        {
            replicas_[replicaId].UpdateCatchup(true, false, replicaId, replicaLSN);
        }
    }

    for (auto replicaEntity : pcReplicas)
    {
        auto replicaId = replicaEntity.first;
        auto replicaLSN = replicaEntity.second;
        if (pcAndccReplicas.find(replicaId) == pcAndccReplicas.end())
        {
            replicas_[replicaId].UpdateCatchup(false, true, replicaId, replicaLSN);
        }
    }

    // Update previous remaining PC, CC and PCandCC replicas state to NA
    for (auto replicaEntity : replicas_)
    {
        auto replicaId = replicaEntity.first;
        auto replica = replicaEntity.second;

        if (replica.State == ReplicaState::PCOnly && pcReplicas.find(replicaId) == pcReplicas.end())
        {
            replicas_[replicaId].UpdateCatchup(false, false, replicaId);
        }
        else if (replica.State == ReplicaState::CCOnly && ccReplicas.find(replicaId) == ccReplicas.end())
        {
            replicas_[replicaId].UpdateCatchup(false, false, replicaId);
        }
        else if (replica.State == ReplicaState::PCandCC && pcAndccReplicas.find(replicaId) == pcAndccReplicas.end())
        {
            replicas_[replicaId].UpdateCatchup(false, false, replicaId);
        }
    }
}

void ComTestReplicaMap::UpdateCurrentReplicaSet(std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & ccReplicas)
{
    AcquireWriteLock grab(lock_);
    
    // Verify replica state transition
    for (auto replicaEntity : ccReplicas)
    {
        auto replicaId = replicaEntity.first;
        replicas_[replicaId].UpdateCurrent(true, replicaId);
    }

    // Update previous PC, PCandCC and previous remaining CC replicas state to NA
    // Remaining CC replicas are those previously in CC but not in CC in this UpdateCurrent call. Those replicas will transit to NA state
    for (auto replicaEntity : replicas_)
    {
        auto replicaId = replicaEntity.first;
        auto replica = replicaEntity.second;

        if (replica.State == ReplicaState::PCOnly || replica.State == ReplicaState::PCandCC)
        {
            replicas_[replicaId].UpdateCurrent(false, replicaId); // PC, PCandCC replica transit to NA
        }
        else if (replica.State == ReplicaState::CCOnly && ccReplicas.find(replicaId) == ccReplicas.end())
        {
            replicas_[replicaId].UpdateCurrent(false, replicaId); // Remaining CC replicas transit to NA
        }
    }
}

void ComTestReplicaMap::BeginBuildReplica(FABRIC_REPLICA_ID replicaId)
{
    UpdateReplicaMap(replicaId);

    AcquireWriteLock grab(lock_);
    replicas_[replicaId].BuildReplica();
}

void ComTestReplicaMap::BuildReplicaOnComplete(FABRIC_REPLICA_ID replicaId, HRESULT hr)
{
    AcquireWriteLock grab(lock_);
    replicas_[replicaId].BuildReplicaOnComplete(hr);
}

void ComTestReplicaMap::EndBuildReplica(IFabricAsyncOperationContext * context, HRESULT hr)
{
    AcquireWriteLock grab(lock_);
    auto replicaId = asyncContextMap_[context];
    replicas_[replicaId].BuildReplicaOnComplete(hr);
    asyncContextMap_.erase(context);
}

void ComTestReplicaMap::RemoveReplica(FABRIC_REPLICA_ID replicaId)
{
    AcquireWriteLock grab(lock_);
    replicas_[replicaId].RemoveReplica();
    replicas_.erase(replicaId);
}

void ComTestReplicaMap::AsyncMapAdd(IFabricAsyncOperationContext * context, FABRIC_REPLICA_ID replicaId)
{
    AcquireWriteLock grab(lock_);
    asyncContextMap_[context] = replicaId;
}
