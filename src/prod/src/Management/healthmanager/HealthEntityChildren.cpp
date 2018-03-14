// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("HealthEntityChildren");

template <class TEntity>
HealthEntityChildren<TEntity>::HealthEntityChildren(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    std::wstring const & parentEntityId)
    : parentEntityId_(parentEntityId)
    , partitionedReplicaId_(partitionedReplicaId)
    , entities_()
    , lock_()
{
}

template <class TEntity>
HealthEntityChildren<TEntity>::~HealthEntityChildren()
{
}

template <class TEntity>
void HealthEntityChildren<TEntity>::AddChild(std::shared_ptr<TEntity> const & child)
{
    HMEvents::Trace->AddChild(
        parentEntityId_,
        partitionedReplicaId_.TraceId,
        child->EntityIdString);

    AcquireWriteLock lock(lock_);
    entities_.push_back(child);
}

template <class TEntity>
std::set<std::shared_ptr<TEntity>> HealthEntityChildren<TEntity>::GetChildren()
{
    AcquireWriteLock lock(lock_);
    size_t initialCount = entities_.size();
    std::vector<weak_ptr<TEntity>> upToDateChildren = move(entities_);
    std::set<shared_ptr<TEntity>> children;

    for (auto it = upToDateChildren.begin(); it != upToDateChildren.end(); ++it)
    {
        auto entity = it->lock();
        // If health doesn't exist anymore, just ignore it
        // Same if the entity was duplicated
        if (entity)
        {
            if (children.insert(entity).second)
            {
                entities_.push_back(entity);
            }
            else
            {
                HealthManagerReplica::WriteNoise(
                    TraceComponent, 
                    "{0}: GetChildren: Entity {1} duplicated - removed",
                    parentEntityId_,
                    entity->EntityIdString);
            }
        }
    }

    if (entities_.size() != initialCount)
    {
        HealthManagerReplica::WriteNoise(
            TraceComponent, 
            "{0}: GetChildren: Return {1} children (count: {2}/initial {3})",
            parentEntityId_,
            children.size(),
            entities_.size(),
            initialCount);
    }

    return children;
}

template <class TEntity>
bool HealthEntityChildren<TEntity>::CleanupChildren()
{
    AcquireWriteLock lock(lock_);
    size_t initialCount = entities_.size();
    
    auto it = remove_if(entities_.begin(), entities_.end(), [](weak_ptr<TEntity> const & child) { return (child.use_count() == 0); });
    entities_.erase(it, entities_.end());

    if (entities_.size() != initialCount)
    {
        HealthManagerReplica::WriteNoise(
            TraceComponent, 
            "{0}: CleanupChildren: initial {1}, after cleanup {2}",
            parentEntityId_,
            initialCount,
            entities_.size());
    }

    return entities_.empty();
}

TEMPLATE_SPECIALIZATION_ENTITY(Management::HealthManager::HealthEntityChildren)
