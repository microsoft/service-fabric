// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;

EntitySet::EntitySet(Parameters const & parameters)
: displayName_(parameters.DisplayName),
  perfData_(parameters.PerfData),
  id_(parameters.Id)
{
}

EntitySet::~EntitySet()
{
}

uint64 EntitySet::AddEntity(Infrastructure::EntityEntryBaseSPtr const & entity)
{
    uint64 rv = 0;

    {
        AcquireWriteLock grab(lock_);
        auto inserted = entities_.insert(entity).second;
        TESTASSERT_IF(!inserted, "attempted to add a duplicate entity into a set {0} {1}", displayName_, entity->GetEntityIdForTrace());
        rv = static_cast<uint64>(entities_.size());
    }

    if (perfData_ != nullptr)
    {
        perfData_->Increment();
    }

    return rv;
}

uint64 EntitySet::RemoveEntity(Infrastructure::EntityEntryBaseSPtr const & entity)
{
    uint64 rv = 0;

    {
        AcquireWriteLock grab(lock_);
        auto countErased = entities_.erase(entity);
        TESTASSERT_IF(countErased != 1, "attempted to remove an entity not in the set {0} {1}", displayName_, entity->GetEntityIdForTrace());
        rv = static_cast<uint64>(entities_.size());
    }

    if (perfData_ != nullptr)
    {
        perfData_->Decrement();
    }

    return rv;
}

EntityEntryBaseList EntitySet::GetEntities() const
{
    AcquireReadLock grab(lock_);
    return EntityEntryBaseList(entities_.begin(), entities_.end());
}

bool EntitySet::IsSubsetOf(Infrastructure::EntityEntryBaseSet const & s) const
{
    AcquireReadLock grab(lock_);
    return includes(
        s.begin(),
        s.end(),
        entities_.begin(),
        entities_.end());
}

EntitySet::ChangeSetMembershipAction::ChangeSetMembershipAction(
    EntitySetIdentifier const & id,
    bool addToSet)
: addToSet_(addToSet),
  id_(id)
{    
}

void EntitySet::ChangeSetMembershipAction::OnPerformAction(
    std::wstring const & activityId, 
    Infrastructure::EntityEntryBaseSPtr const & entity,
    ReconfigurationAgent & ra)
{
    OnPerformAction(activityId, entity, ra.EntitySetCollectionObj);
}

string EntitySet::GetRandomEntityTraceId() const
{
    AcquireReadLock grab(lock_);
    if (entities_.empty())
    {
        return string();
    }
    else
    {
        return (*entities_.begin())->GetEntityIdForTrace();
    }
}

void EntitySet::ChangeSetMembershipAction::OnPerformAction(
    std::wstring const & activityId,
    Infrastructure::EntityEntryBaseSPtr const & entity,
    EntitySetCollection const & collection)
{
    auto data = collection.Get(id_);
    auto & set = data.Set;
    auto timer = data.Timer;

    uint64 size = 0;

    if (addToSet_)
    {
        size = set.AddEntity(entity);
        
        /*
            It is not required that every set has a timer associcated with it
            In addition, the timer should only be armed when the first item is added to the set
            Otherwise, the timer would already be armed and there is no need to arm it again

            If there are multiple threads that add at the same time then the thread to add 
            the first item arms the timer.

            Arming the timer acquires a write lock on the timer which is expensive if multiple fts
            are concurrently being added to the set - example when node up ack is processed a large
            number of fts are scheduled for open or when upgrade starts a large number start to be closed

            Removing items from the set does not disarm the timer - that is a less important optimization
        */
        if (timer != nullptr && size == 1)
        {
            timer->Set();
        }
    }
    else
    {
        size = set.RemoveEntity(entity);
    }

    // Safe to access display name without lock as it is const
    RAEventSource::Events->FTSetOperation(set.displayName_, activityId, entity->GetEntityIdForTrace(), addToSet_, size);
}

void EntitySet::ChangeSetMembershipAction::OnCancelAction(ReconfigurationAgent&)
{
}
