// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("ClusterCache");

ClusterCache::ClusterCache(
    Common::ComponentRoot const & root,
    __in HealthManagerReplica & healthManagerReplica)
    : HealthEntityCache(root, healthManagerReplica)
{
}

ClusterCache::~ClusterCache()
{
}

HEALTH_ENTITY_CACHE_ENTITY_TEMPLATED_METHODS_DEFINITIONS(ClusterCache, ClusterHealthId, ClusterEntity, ClusterAttributesStoreData)

Common::ErrorCode ClusterCache::GetClusterHealthPolicy(
    __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy) const
{
    auto cluster = cluster_.lock();
    if (cluster)
    {
        return cluster->GetClusterHealthPolicy(healthPolicy);
    }

    return ErrorCode(ErrorCodeValue::InvalidState);
}

void ClusterCache::CreateDefaultEntities()
{
    auto entity = make_shared<ClusterEntity>(
        make_shared<ClusterAttributesStoreData>(),
        this->HealthManagerReplicaObj,
        HealthEntityState::PendingFirstReport);
    
    cluster_ = entity;

    this->HealthManagerReplicaObj.WriteInfo(TraceComponent, "{0}: Add default: {1}", this->PartitionedReplicaId.TraceId, entity->EntityIdString);
    this->AddEntityCallerHoldsLock(entity->EntityId, entity);
}
