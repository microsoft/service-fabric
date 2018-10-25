// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("EntityCache");
StringLiteral const HealthEntityCacheLoadStoreDataTimerTag("HealthLoadCacheData");
StringLiteral const HealthEntityCacheCleanupTimerTag("HealthCleanupCacheTimer");

template <class TEntityId>
HealthEntityCache<TEntityId>::HealthEntityCache(
    Common::ComponentRoot const & root,
    __in HealthManagerReplica & healthManagerReplica)
    : RootedObject(root)
    , healthManagerReplica_(healthManagerReplica)
    , entities_()
    , sequenceStreamMap_()
    , pendingSequenceStream_()
    , acceptRequests_(false)
    , lock_()
    , allDataLoadedInMemory_(false)
    , nonInitializedEntities_()
    , cleanupTimer_()
    , loadStoreDataTimer_()
{
}

template <class TEntityId>
HealthEntityCache<TEntityId>::~HealthEntityCache()
{
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::Open()
{
    { // lock
        AcquireWriteLock lock(lock_);

        if (acceptRequests_)
        {
            TRACE_LEVEL_AND_TESTASSERT(healthManagerReplica_.WriteInfo, TraceComponent, "{0}: Open: Cache is already opened", this->PartitionedReplicaId.TraceId);

            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        acceptRequests_ = true;

        // Create default entities. They will be loaded from store on the timer
        this->CreateDefaultEntities();

        auto root = this->Root.CreateComponentRoot();

        // Schedule loading the entities from store.
        // This operation is going to be retried on errors.
        // The replica will be processing requests only after this step succeeds.
        loadStoreDataTimer_ = Timer::Create(
            HealthEntityCacheLoadStoreDataTimerTag,
            [this, root](TimerSPtr const &) { this->LoadStoreDataTimerCallback(); },
            true);
        loadStoreDataTimer_->Change(TimeSpan::FromSeconds(Constants::HealthEntityCacheLoadStoreDataDelayInSeconds));
    } //endlock

    return ErrorCode(ErrorCodeValue::Success);
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::Close()
{
    ErrorCode error(ErrorCodeValue::Success);
    HealthEntityMap entities;

    { // lock
        AcquireWriteLock lock(lock_);
        acceptRequests_ = false;

        if (loadStoreDataTimer_)
        {
            loadStoreDataTimer_->Cancel();
            loadStoreDataTimer_ = nullptr;
        }

        if (cleanupTimer_)
        {
            cleanupTimer_->Cancel();
            cleanupTimer_ = nullptr;
        }

        allDataLoadedInMemory_ = false;

        // Close all entities and remove them from cache
        entities = move(entities_);

        sequenceStreamMap_.clear();
        nonInitializedEntities_.clear();
    } // endlock

    // Unregister all handlers and close entities
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        auto innerError = it->second->Close();
        if (!innerError.IsSuccess())
        {
            error.Overwrite(innerError);
        }
    }

    return error;
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::LoadStoreDataTimerCallback()
{
    vector<HealthEntitySPtr> nonInitializedEntities;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_ || allDataLoadedInMemory_)
        {
            healthManagerReplica_.WriteNoise(TraceComponent, "{0}: LoadStoreDataTimerCallback.Cancel: acceptRequests={1}", this->PartitionedReplicaId.TraceId, acceptRequests_);
            return;
        }

        nonInitializedEntities.swap(nonInitializedEntities_);
    } // endlock

    ActivityId activityId;
    if (nonInitializedEntities.empty())
    {
        LoadEntitiesAttributes(activityId);
    }
    else
    {
        LoadStoreEntitiesEvents(activityId, move(nonInitializedEntities));
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::AddEntityCallerHoldsLock(
    TEntityId const & entityId,
    HealthEntitySPtr const & entity)
{
    entities_.insert(pair<TEntityId, HealthEntitySPtr>(entityId, entity));
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::LoadEntitiesAttributes(
    __in Common::ActivityId & activityId)
{
    // Read all attributes for this entity type and build the entities for them.
    // Note that this operation is read only, so it performs no cleanup for expired entries
    vector<AttributesStoreDataSPtr> storeEntities;
    vector<SequenceStreamStoreDataUPtr> sequenceStreams;
    auto error = this->LoadEntities(
        activityId,
        storeEntities,
        sequenceStreams);

    vector<HealthEntitySPtr> nonInitializedEntities;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            healthManagerReplica_.WriteNoise(TraceComponent, "{0}: LoadEntitiesAttributes.Cancel: acceptRequests={1}", this->PartitionedReplicaId.TraceId, acceptRequests_);
            return;
        }

        if (error.IsSuccess())
        {
            // Create the entities
            map<wstring, HealthEntitySPtr> existingEntities;
            for (auto const & entityPair : entities_)
            {
                existingEntities.insert(pair<wstring, HealthEntitySPtr>(entityPair.second->EntityIdString, entityPair.second));
            }

            for (auto && storeEntity : storeEntities)
            {
                HealthEntitySPtr entity;
                auto itFind = existingEntities.find(storeEntity->Key);
                if (itFind == existingEntities.end())
                {
                    // For each entity, create an object to represent it, passing in the read attributes
                    entity = this->AddNewEntityCallerHoldsLock(move(storeEntity), HealthEntityState::PendingLoadFromStore);
                }
                else
                {
                    // The entity has been created as non-persisted by a child; pass it the new attributes
                    // then load it from store.
                    entity = itFind->second;
                    entity->PrepareLoadFromStore(move(storeEntity));
                }

                nonInitializedEntities.push_back(move(entity));
            }

            for (auto it = sequenceStreams.begin(); it != sequenceStreams.end(); ++it)
            {
                healthManagerReplica_.WriteInfo("SS", "{0}: Loaded {1}", this->PartitionedReplicaId.TraceId, *it);
                sequenceStreamMap_.insert(make_pair((*it)->SourceId, move(*it)));
            }
        }
        else
        {
            HMEvents::Trace->ReadFailed(
                this->PartitionedReplicaId.TraceId,
                activityId,
                this->EntitiesType,
                error);

            // Schedule retry
            loadStoreDataTimer_->Change(ManagementConfig::GetConfig().HealthStoreReadRetryInterval);

            return;
        }
    } // endlock

    activityId.IncrementIndex();
    LoadStoreEntitiesEvents(activityId, move(nonInitializedEntities));
}

template <class TEntityId>
std::vector<HealthEventStoreDataUPtr> HealthEntityCache<TEntityId>::GetEntityEvents(
    std::vector<EntityHealthEventStoreData<TEntityId>> && events)
{
    vector<HealthEventStoreDataUPtr> result;
    for (auto && entry : events)
    {
        result.push_back(make_unique<EntityHealthEventStoreData<TEntityId>>(move(entry)));
    }

    return result;
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::LoadStoreEntitiesEvents(
    Common::ActivityId const & activityId,
    vector<HealthEntitySPtr> && entities)
{
    auto tx = healthManagerReplica_.StoreWrapper.CreateTransaction(activityId);
    std::vector<EntityHealthEventStoreData<TEntityId>> storeEvents;
    // Read all events for all entities with this cache type
    auto error = healthManagerReplica_.StoreWrapper.ReadPrefix<EntityHealthEventStoreData<TEntityId>>(
        tx,
        L"",
        /*out*/storeEvents);
    tx.CommitReadOnly();

    if (!error.IsSuccess())
    {
        HMEvents::Trace->LoadEntityDataFailed(
            this->EntitiesType,
            this->PartitionedReplicaId.TraceId,
            activityId,
            error);

        if (error.IsError(ErrorCodeValue::NotPrimary))
        {
            // Non retryable error, do not schedule retry
            HealthManagerReplica::WriteInfo(TraceComponent, "{0}: {1}: LoadStoreEntitiesEvents.Cancel: nonRetryableError={2}", this->PartitionedReplicaId.TraceId, activityId, error);
            return;
        }
        else
        { // lock
            AcquireWriteLock lock(lock_);
            if (!acceptRequests_)
            {
                HealthManagerReplica::WriteInfo(TraceComponent, "{0}: {1}: LoadStoreEntitiesEvents.Cancel: acceptRequests={2}", this->PartitionedReplicaId.TraceId, activityId, acceptRequests_);
                return;
            }

            // Schedule retry load events
            nonInitializedEntities_.swap(entities);
            loadStoreDataTimer_->Change(ManagementConfig::GetConfig().HealthStoreReadRetryInterval);
            return;
        } // endlock
    }

    // Events are loaded. Organize events per entity id and pass them to associated entity.
    uint64 eventCount = static_cast<uint64>(storeEvents.size());

    using EntityEvents = std::vector<EntityHealthEventStoreData<TEntityId>>;
    map<TEntityId, EntityEvents> eventsPerTEntityId;
    for (auto && entry : storeEvents)
    {
        // If the key doesn't exist, a new entry is inserted; otherwise, get the iterator to the existing entry.
        auto it = eventsPerTEntityId.insert(make_pair(entry.EntityId, EntityEvents())).first;
        it->second.push_back(move(entry));
    }

    // The health entity doesn't have the strongly typed entity id, so build a map with entity id string as key.
    map<wstring, EntityEvents> eventsPerEntityId;
    for (auto it = eventsPerTEntityId.begin(); it != eventsPerTEntityId.end(); ++it)
    {
        eventsPerEntityId.insert(make_pair(wformatString(it->first), move(it->second)));
    }
    
    // For each entity, find the events and load them.
    for (auto const & entity : entities)
    {
        EntityEvents entityEvents;
        auto itEvents = eventsPerEntityId.find(entity->EntityIdString);
        // There may be entities without events (for example, for entities that had a transient system report
        // that expired and was removed from store before the previous primary went down.
        // Transition the state to ready to show that load from store is done.
        if (itEvents != eventsPerEntityId.end())
        {
            entityEvents = move(itEvents->second);
            eventsPerEntityId.erase(itEvents);
        }

        error = entity->LoadStoreData(activityId, GetEntityEvents(move(entityEvents)));
        if (!error.IsSuccess())
        {
            // Load events should not fail at this point; it can only happen if the replica is closed or in invalid state.
            break;
        }
    }

    // If there are remaining events that are not associated with an entity,
    // they are leaked.
    if (error.IsSuccess())
    {
        ReplicaActivityId replicaActivityId(this->PartitionedReplicaId, activityId);
        for (auto && entry : eventsPerEntityId)
        {
            // The events are leaked.
            ASSERT_IF(entry.second.empty(), "{0}: {1} has leaked events, but eventsPerEntityId has 0 entries", activityId, entry.first);
            HMEvents::Trace->CreateNonPersistedForLeakedEvents(
                activityId,
                this->EntitiesType,
                entry.second.size(),
                entry.second[0].Key);

            TEntityId const & entityId = entry.second[0].EntityId;
            HealthEntitySPtr leakedEntity;
            { // lock
                AcquireWriteLock lock(lock_);
                // Create a non-persisted entity and put the events on it, so they can be removed from store
                // on cleanup timer.
                leakedEntity = this->AddNewEntityCallerHoldsLock(
                    this->CreateAttributes(entityId, replicaActivityId),
                    HealthEntityState::PendingFirstReport);
            } //endlock

            error = leakedEntity->AddStoreLeakedEvents(activityId, GetEntityEvents(move(entry.second)));
            if (!error.IsSuccess())
            {
                // Load events should not fail at this point; it can only happen if the replica is closed or in invalid state.
                break;
            }
        }
    }

    bool reportFault = false;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            healthManagerReplica_.WriteInfo(TraceComponent, "{0}: {1}: LoadStoreEntitiesEvents.Cancel: acceptRequests={2}", this->PartitionedReplicaId.TraceId, activityId, acceptRequests_);
            return;
        }

        if (!error.IsSuccess())
        {
            // Invalid state. Since we don't know how we got into this state, bring replica down.
            reportFault = true;
        }
        else
        {
            // Entity cache is now ready to accept requests
            allDataLoadedInMemory_ = true;

            HMEvents::Trace->ReadComplete(
                this->PartitionedReplicaId.TraceId,
                activityId,
                this->EntitiesType,
                static_cast<uint64>(entities_.size()),
                eventCount);

            loadStoreDataTimer_->Cancel();
            loadStoreDataTimer_ = nullptr;

            auto root = this->Root.CreateComponentRoot();
            // Schedule cleanup timer
            cleanupTimer_ = Timer::Create(
                HealthEntityCacheCleanupTimerTag,
                [this, root](TimerSPtr const &) { this->CleanupEntitiesTimerCallback(); },
                true); // allow concurrency
            cleanupTimer_->Change(ManagementConfig::GetConfig().HealthStoreCleanupInterval);
        } // end success
    } // endlock

    if (reportFault)
    {
        healthManagerReplica_.ReportTransientFault(error);
    }
    else
    {
        // Notify cache manager that load is done outside the lock
        healthManagerReplica_.EntityManager->OnCacheLoadCompleted();
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::CleanupEntitiesTimerCallback()
{
    vector<IHealthJobItemSPtr> jobItems;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            // Closed, nothing to do
            return;
        }

        if (!allDataLoadedInMemory_)
        {
            TRACE_LEVEL_AND_TESTASSERT(
                healthManagerReplica_.WriteWarning,
                TraceComponent,
                "{0}: Cleanup timer should happen after all data is loaded in memory",
                this->PartitionedReplicaId.TraceId);
            return;
        }

        ActivityId activityId;
        size_t startEntityCount = entities_.size();
        FABRIC_SEQUENCE_NUMBER invalidateLsn = FABRIC_INVALID_SEQUENCE_NUMBER;
        wstring invalidateSource;
        bool hasInvalidation = false;

        DateTime now = DateTime::Now();
        for (auto it = sequenceStreamMap_.begin(); it != sequenceStreamMap_.end(); ++it)
        {
            if (now > it->second->InvalidateTime)
            {
                ASSERT_IF(
                    it->second->InvalidateLsn == FABRIC_INVALID_SEQUENCE_NUMBER,
                    "{0}: inconsistent invalidation state {1}",
                    activityId,
                    *(it->second));
                invalidateLsn = it->second->InvalidateLsn;
                invalidateSource = it->second->SourceId;

                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    "{0}: Processing invalidation for {1} {2} seq {3}",
                    activityId,
                    it->second->Type,
                    invalidateSource,
                    invalidateLsn);
                break;
            }
        }

        HealthEntityMap remainingEntities;

        // Go through all entities and remove the ones that have been previously cleaned up.
        // For each entity, check whether there is any cleanup work to be done.
        // If there are not too many pending cleanup job items, create new ones to execute the cleanup.
        // Otherwise, the job items will be created on next timer callback.
        int skippedJobItems = 0;
        int scheduledJobItems = 0;
        int previouslyScheduledJobItems = 0;
        for (auto it = entities_.begin(); it != entities_.end(); ++it)
        {
            // If the entity was already cleaned up and there's no work pending for it in job queue, remove it from cache
            bool hasPendingWork;
            if (it->second->IsCleanedUp(activityId, hasPendingWork))
            {
                if (!hasPendingWork)
                {
                    // Do not add this entity to remaining ones, so it will be removed from cache
                    continue;
                }
            }
            else
            {
                // Check whether entity should be cleaned up. This also cleans up children where appropriate.
                bool cleanup = it->second->ShouldBeCleanedUp();
                if (!cleanup && (invalidateLsn != FABRIC_INVALID_SEQUENCE_NUMBER))
                {
                    cleanup = it->second->ShouldInvalidate(invalidateSource, invalidateLsn, activityId);
                }

                IHealthJobItemSPtr cleanupJobItem;
                ErrorCode error(ErrorCodeValue::Success);
                if (cleanup)
                {
                    hasInvalidation = true;

                    error = this->TryReserveCleanupJobIfNotThrottled(activityId, it->second, skippedJobItems);
                    if (error.IsSuccess())
                    {
                        // Schedule tasks to delete the entity from store.
                        // Note that entities will remain in memory until next cleanup timer fires.
                        cleanupJobItem = make_shared<CleanupEntityJobItem>(
                            it->second,
                            invalidateSource,
                            invalidateLsn,
                            activityId);
                    }
                }
                // Always check delete because of parent state, even if skippedJobItems shows that we reached throttle values.
                // This sets the parent to correct entity (if it changed).
                else if (it->second->ShouldBeDeletedDueToParentState(activityId))
                {
                    error = this->TryReserveCleanupJobIfNotThrottled(activityId, it->second, skippedJobItems);
                    if (error.IsSuccess())
                    {
                        cleanupJobItem = make_shared<DeleteEntityJobItem>(
                            it->second,
                            activityId);
                    }
                }
                else if (skippedJobItems > 0)
                {
                    // Only check expired events if the cleanup tasks haven't reached max value
                    // Executing events cleanup on the next timer doesn't hurt.
                    error = ErrorCode(ErrorCodeValue::ServiceTooBusy);
                }
                else if (it->second->HasExpiredEventChanges())
                {
                    error = this->TryReserveCleanupJobIfNotThrottled(activityId, it->second, skippedJobItems);
                    if (error.IsSuccess())
                    {
                        cleanupJobItem = make_shared<CleanupEntityExpiredTransientEventsJobItem>(
                            it->second,
                            activityId);
                    }
                }
                else if (it->second->ShouldAutoCleanupEvents())
                {
                    error = this->TryReserveCleanupJobIfNotThrottled(activityId, it->second, skippedJobItems);
                    if (error.IsSuccess())
                    {
                        cleanupJobItem = make_shared<AutoCleanupEventsJobItem>(
                            it->second,
                            activityId);
                    }
                }
                else if (it->second->ShouldCheckConsistencyBetweenMemoryAndStore(activityId))
                {
                    error = this->TryReserveCleanupJobIfNotThrottled(activityId, it->second, skippedJobItems);
                    if (error.IsSuccess())
                    {
                        cleanupJobItem = make_shared<CheckInMemoryEntityDataJobItem>(
                            it->second,
                            activityId);
                    }
                }

                if (error.IsSuccess())
                {
                    if (cleanupJobItem)
                    {
                        ++scheduledJobItems;
                        jobItems.push_back(move(cleanupJobItem));
                        activityId.IncrementIndex();
                    }
                }
                else if (error.IsError(ErrorCodeValue::AlreadyExists))
                {
                    ++previouslyScheduledJobItems;
                }
                else
                {
                    ++skippedJobItems;
                }
            }

            remainingEntities.insert(move(*it));
        }

        entities_.swap(remainingEntities);

        if (!hasInvalidation && (invalidateLsn != FABRIC_INVALID_SEQUENCE_NUMBER))
        {
            // Close invalidation as it is already completed.
            sequenceStreamMap_[invalidateSource]->InvalidateTime = DateTime::MaxValue;
        }

        HMEvents::Trace->CleanupTimer(
            this->PartitionedReplicaId.TraceId,
            activityId,
            this->EntitiesType,
            static_cast<int32>(startEntityCount),
            static_cast<int32>(entities_.size()),
            scheduledJobItems,
            skippedJobItems,
            previouslyScheduledJobItems);

        // Schedule next timer callback
        cleanupTimer_->Change(ManagementConfig::GetConfig().HealthStoreCleanupInterval);
    } // endlock

    // Enqueue the cleanup job items outside the cache lock
    for (auto && jobItem : jobItems)
    {
        this->JobQueueManager.AddWork(move(jobItem));
    }
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::TryReserveCleanupJobIfNotThrottled(
    Common::ActivityId const & activityId,
    __in HealthEntitySPtr & entity,
    int rejectedCount) const
{
    if (rejectedCount > 0)
    {
        return ErrorCode(ErrorCodeValue::ServiceTooBusy);
    }

    return entity->TryReserveCleanupJob(activityId);
}

template <class TEntityId>
HealthEntitySPtr HealthEntityCache<TEntityId>::GetEntity(
    TEntityId const & entityId) const
{
    HealthEntitySPtr entity;
    AcquireReadLock lock(lock_);
    TryGetEntityCallerHoldsLock(entityId, entity);
    return entity;
}

template <class TEntityId>
bool HealthEntityCache<TEntityId>::TryGetEntityCallerHoldsLock(
    TEntityId const & entityId,
    __out HealthEntitySPtr & entity) const
{
    auto it = entities_.find(entityId);
    if (it == entities_.end())
    {
        entity = nullptr;
        return false;
    }
    else
    {
        entity = it->second;
        return true;
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::ExecuteEntityWorkUnderCacheLock(
    Common::ActivityId const & activityId,
    TEntityId const & entityId,
    EntityWorkCallback const & callback)
{
    AcquireWriteLock lock(lock_);
    if (!acceptRequests_)
    {
        // Do not create the entity when the cache is disabled
        return;
    }

    HealthEntitySPtr entity;
    auto it = entities_.find(entityId);
    if (it == entities_.end())
    {
        ReplicaActivityId replicaActivityId(this->PartitionedReplicaId, activityId);
        entity = this->AddNewEntityCallerHoldsLock(
            this->CreateAttributes(entityId, replicaActivityId),
            HealthEntityState::PendingFirstReport);
        HMEvents::Trace->CreateNonPersisted(
            entity->EntityIdString,
            replicaActivityId.TraceId,
            this->EntitiesType);
    }
    else
    {
        entity = it->second;
    }

    // Execute callback under the cache lock
    callback(entity);
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::GetExistingOrCreateEntityCallerHoldsLock(
    TEntityId const & entityId,
    ReportRequestContext const & context,
    __out HealthEntitySPtr & entity)
{
    auto it = entities_.find(entityId);
    if (it == entities_.end())
    {
        // All store entities have been loaded in memory.
        // Since this entityId doesn't exist in memory, it must not exist in store either.
        // Therefore, it's sufficient to create an empty entity, which has no attributes set other than the id.
        // There's also no need to read events, since there can't be any, so skip loading entity data.
        // The constructor will mark that the entity is ready for processing immediately, without checking the database.
        entity = this->AddNewEntityCallerHoldsLock(
            this->CreateAttributes(entityId, context),
            HealthEntityState::PendingWriteToStore);
    }
    else
    {
        entity = it->second;
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::AddReport(
    ReportRequestContext && context)
{
    Common::ErrorCode error(ErrorCodeValue::Success);
    HealthEntitySPtr entity;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!allDataLoadedInMemory_)
        {
            error = ErrorCode(ErrorCodeValue::NotReady);
        }
        else
        {
            // Check whether a sequence stream was passed in
            if (context.SequenceStreamFromLsn != FABRIC_INVALID_SEQUENCE_NUMBER)
            {
                // Search for existing sequence streams, check whether there is gap.
                // If the sequence stream is new, the new context should specify starting value (0)
                FABRIC_SEQUENCE_NUMBER savedUpToLsn = 0;
                auto it = sequenceStreamMap_.find(context.Report.SourceId);
                if (it != sequenceStreamMap_.end())
                {
                    savedUpToLsn = it->second->UpToLsn;
                }

                // The provided start Lsn must be equal to the last upToLsn (since the interval is inclusive left, exclusive right)
                // or less (if the previous reports were not received by ReportingComponent and this is a retry)
                if (context.SequenceStreamFromLsn > savedUpToLsn)
                {
                    // Persisted [start, to), received [to + x, newTo), there's a gap
                    healthManagerReplica_.WriteNoise(
                        TraceComponent,
                        "{0}: AddReport {1}, sn {2}: reject, received SequenceStreamFromLsn {3} > savedUpToLsn {4}, SequenceStreamRangeGapError",
                        context.ActivityId,
                        context.Report.EntityPropertyId,
                        context.Report.SequenceNumber,
                        context.SequenceStreamFromLsn,
                        savedUpToLsn);
                    error = ErrorCode(ErrorCodeValue::SequenceStreamRangeGapError);
                }
                else if (context.SequenceStreamFromLsn < savedUpToLsn &&
                    context.SequenceStreamUpToLsn <= savedUpToLsn)
                {
                    // Persisted [start,to), received [start+x,to-y), reject
                    healthManagerReplica_.WriteNoise(
                        TraceComponent,
                        "{0}: AddReport {1}, sn {2}: reject, current uptoLsn {3}, received [{4}, {5}): StaleRequest",
                        context.ActivityId,
                        context.Report.EntityPropertyId,
                        context.Report.SequenceNumber,
                        savedUpToLsn,
                        context.SequenceStreamFromLsn,
                        context.SequenceStreamUpToLsn);
                    error = ErrorCode(ErrorCodeValue::StaleRequest);
                }
            }

            if (error.IsSuccess())
            {
                TEntityId entityId = context.GetEntityId<TEntityId>();
                GetExistingOrCreateEntityCallerHoldsLock(entityId, context, /*out*/entity);
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        // Notify the context outside the lock
        healthManagerReplica_.QueueReportForCompletion(move(context), move(error));
        return;
    }

    // Add the report job item outside the cache lock
    ASSERT_IFNOT(entity, "{0}: AddReport to cache: entity should exist", context);
    this->JobQueueManager.AddWork(make_shared<ProcessReportEntityJobItem>(entity, move(context)));
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::ProcessQuery(
    TEntityId const & entityId,
    __in QueryRequestContext & context) const
{
    HMEvents::Trace->QueryReceived(context);
    HealthEntitySPtr entity;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!allDataLoadedInMemory_)
        {
            return ErrorCode(ErrorCodeValue::NotReady);
        }
        else
        {
            TryGetEntityCallerHoldsLock(entityId, /*out*/entity);
        }
    } // endlock

    if (entity)
    {
        // Process the request on the current thread from QueryJobQueue and outside the cache lock
        return entity->ProcessQuery(context);
    }
    else
    {
        healthManagerReplica_.WriteNoise(
            TraceComponent,
            "{0}: Entity not found in cache",
            context);

        // Since all entities have been loaded, if the entity doesn't exist,
        // it can't have any reports associated with it.
        // Complete the request immediately.
        return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().EntityNotFound));
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::AddSequenceStream(
    SequenceStreamRequestContext && context)
{
    Common::ErrorCode error(ErrorCodeValue::Success);

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!allDataLoadedInMemory_)
        {
            error = ErrorCode(ErrorCodeValue::NotReady);
        }
    } // endlock

    if (!error.IsSuccess())
    {
        // Notify the context outside the lock
        healthManagerReplica_.QueueSequenceStreamForCompletion(move(context), move(error));
        return;
    }

    // Create the ss job item outside the cache lock
    this->JobQueueManager.AddWork(make_shared<ProcessSequenceStreamJobItem<TEntityId>>(*this, move(context)));
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::StartProcessSequenceStream(
    IHealthJobItemSPtr const & jobItem,
    SequenceStreamRequestContext const & context)
{
    Common::ErrorCode error(ErrorCodeValue::Success);
    bool updateExisting = false;

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else
        {
            ASSERT_IFNOT(
                allDataLoadedInMemory_,
                "Load not completed when StartProcessSequenceStream for {0}",
                context);

            auto it = sequenceStreamMap_.find(context.SequenceStream.SourceId);
            FABRIC_INSTANCE_ID savedInstance = 0;
            FABRIC_SEQUENCE_NUMBER savedUpToLsn = 0;
            if (it != sequenceStreamMap_.end())
            {
                savedInstance = it->second->Instance;
                savedUpToLsn = it->second->UpToLsn;
                updateExisting = true;
            }

            if (context.SequenceStream.IsForInstanceUpdate())
            {
                if (updateExisting && context.SequenceStream.Instance <= savedInstance)
                {
                    error = ErrorCode(ErrorCodeValue::AlreadyExists);
                }
                else
                {
                    healthManagerReplica_.WriteInfo(
                        TraceComponent,
                        "{0}: updating instance for {1} {2} from {3} to {4}",
                        context.ReplicaActivityId.TraceId,
                        context.SequenceStream.Kind,
                        context.SequenceStream.SourceId,
                        savedInstance,
                        context.SequenceStream.Instance);

                    pendingSequenceStream_ = make_unique<EntitySequenceStreamStoreData<TEntityId>>(
                        context.SequenceStream.SourceId,
                        context.SequenceStream.Instance,
                        updateExisting ? it->second->HighestLsn : 0,
                        updateExisting ? it->second->InvalidateLsn : FABRIC_INVALID_SEQUENCE_NUMBER,
                        updateExisting ? it->second->InvalidateTime : DateTime::MaxValue,
                        updateExisting ? it->second->HighestLsn : 0,
                        context.ReplicaActivityId);
                }
            }
            else if (context.SequenceStream.Instance != savedInstance)
            {
                ASSERT_IF(
                    context.SequenceStream.Instance > savedInstance,
                    "Incoming instance {0} later than existing {1} for {2} {3}",
                    context.SequenceStream.Instance,
                    savedInstance,
                    context.SequenceStream.Kind,
                    context.SequenceStream.SourceId);
                error = ErrorCode(ErrorCodeValue::StaleRequest);
            }
            // The provided start Lsn must be equal to the last upToLsn (since the interval is inclusive left, exclusive right)
            // or less (when reporting component retries)
            else if (context.SequenceStream.FromLsn > savedUpToLsn)
            {
                error = ErrorCode(ErrorCodeValue::SequenceStreamRangeGapError);
            }
            else if (context.SequenceStream.FromLsn < savedUpToLsn &&
                     context.SequenceStream.UpToLsn < savedUpToLsn)
            {
                // Can't accept lower value than what's persisted
                error = ErrorCode(ErrorCodeValue::StaleRequest);
            }
            else if (context.SequenceStream.FromLsn < savedUpToLsn &&
                     context.SequenceStream.UpToLsn == savedUpToLsn)
            {
                // Send ACK back to reporting component, to know that the sequence stream is already persisted
                error = ErrorCode(ErrorCodeValue::AlreadyExists);
            }
            else
            {
                FABRIC_SEQUENCE_NUMBER invalidateLsn = context.SequenceStream.InvalidateLsn;
                DateTime invalidateTime = DateTime::MaxValue;
                if (updateExisting && invalidateLsn <= it->second->InvalidateLsn)
                {
                    invalidateLsn = it->second->InvalidateLsn;
                    invalidateTime = it->second->InvalidateTime;
                }
                else if (invalidateLsn != FABRIC_INVALID_SEQUENCE_NUMBER)
                {
                    invalidateTime = (context.EntityKind == HealthEntityKind::Node ?
                        DateTime::Now() + ManagementConfig::GetConfig().HealthStoreNodeEntityInvalidationDuration :
                        DateTime::Now() + ManagementConfig::GetConfig().HealthStoreEntityWithoutSystemReportKeptInterval);

                    healthManagerReplica_.WriteInfo(
                        TraceComponent,
                        "{0}: invalidating {1} {2} up to {3} effective at {4}",
                        context.ReplicaActivityId.TraceId,
                        context.SequenceStream.Kind,
                        context.SequenceStream.SourceId,
                        invalidateLsn,
                        invalidateTime);
                }

                pendingSequenceStream_ = make_unique<EntitySequenceStreamStoreData<TEntityId>>(
                    context.SequenceStream.SourceId,
                    context.SequenceStream.Instance,
                    context.SequenceStream.UpToLsn,
                    invalidateLsn,
                    invalidateTime,
                    max(updateExisting ? it->second->HighestLsn : 0, context.SequenceStream.UpToLsn),
                    context.ReplicaActivityId);
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        // Complete work item outside the lock
        if (error.IsError(ErrorCodeValue::AlreadyExists))
        {
            // Replace with success, so the sequence stream result is added to reply
            error = ErrorCode(ErrorCodeValue::Success);
        }

        this->JobQueueManager.OnWorkComplete(*jobItem, error);
        return;
    }

    StartPersistSequenceStream(jobItem, context, updateExisting);
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::EndProcessSequenceStream(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    OnPersistSequenceStreamCompleted(jobItem, operation);
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::GetSequenceStreamProgress(
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    bool getHighestLsn,
    __inout SequenceStreamResult & sequenceStreamResult)
{
    ErrorCode error(ErrorCodeValue::Success);

    { // lock
        AcquireReadLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!allDataLoadedInMemory_)
        {
            error = ErrorCode(ErrorCodeValue::NotReady);
        }
        else
        {
            auto itMap = sequenceStreamMap_.find(sourceId);
            if (itMap == sequenceStreamMap_.end() || instance > itMap->second->Instance)
            {
                error = ErrorCode(ErrorCodeValue::UpdatePending);
            }
            else
            {
                ASSERT_IF(itMap->second->HighestLsn < itMap->second->UpToLsn,
                    "HighestLsn is lower than UpToLsn: {0}", *(itMap->second));
                sequenceStreamResult.UpToLsn = (getHighestLsn ? itMap->second->HighestLsn : itMap->second->UpToLsn);
                sequenceStreamResult.Instance = itMap->second->Instance;
                healthManagerReplica_.WriteInfo("GetSSProgress", "Result: {0}", sequenceStreamResult);
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        HMEvents::Trace->GetSSProgressFailed(
            this->PartitionedReplicaId.TraceId,
            this->EntitiesType,
            error.ReadValue());
    }

    return error;
}

template <class TEntityId>
ErrorCode HealthEntityCache<TEntityId>::AcceptSequenceStream(
    Common::ActivityId const & activityId,
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn)
{
    ErrorCode error(ErrorCodeValue::Success);

    { // lock
        AcquireWriteLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!allDataLoadedInMemory_)
        {
            error = ErrorCode(ErrorCodeValue::NotReady);
        }
        else
        {
            auto itMap = sequenceStreamMap_.find(sourceId);
            ASSERT_IF(
                itMap == sequenceStreamMap_.end(),
                "{0}: Report received but can't find sequence information for {1}",
                activityId,
                sourceId);

            if (instance < itMap->second->Instance)
            {
                error = ErrorCode(ErrorCodeValue::StaleRequest);
            }
            else
            {
                ASSERT_IF(
                    instance > itMap->second->Instance,
                    "{0}: Report sequence instance {1} higher than {2} for {3}",
                    activityId,
                    instance,
                    itMap->second->Instance,
                    sourceId);

                if (pendingSequenceStream_ &&
                    pendingSequenceStream_->SourceId == sourceId &&
                    pendingSequenceStream_->Instance > instance)
                {
                    // There is a pending update with a higher instance, reject this request with retryable error
                    healthManagerReplica_.WriteInfo(
                        TraceComponent,
                        "{0}: {1}: Reject SS({2}/{3}): there is a pending update to {4}",
                        activityId,
                        this->EntitiesType,
                        sourceId,
                        instance,
                        *pendingSequenceStream_);
                    error = ErrorCode(ErrorCodeValue::UpdatePending);
                }
                else if (upToLsn > itMap->second->HighestLsn)
                {
                    healthManagerReplica_.WriteInfo(
                        TraceComponent,
                        "{0}: {1}: Accept SS({2}/{3}): highest old={4}, new={5}",
                        activityId,
                        this->EntitiesType,
                        sourceId,
                        instance,
                        itMap->second->HighestLsn,
                        upToLsn);
                    itMap->second->HighestLsn = upToLsn;
                }
                else
                {
                    healthManagerReplica_.WriteNoise(
                        TraceComponent,
                        "{0}: {1}: Accept SS({2}/{3}): highest old={4}, new={5}. No change.",
                        activityId,
                        this->EntitiesType,
                        sourceId,
                        instance,
                        itMap->second->HighestLsn,
                        upToLsn);
                }
            }
        }
    } // endlock

    return error;
}

template <class TEntityId>
ErrorCode HealthEntityCache<TEntityId>::PreparePersistSequenceStreamTx(
    Store::ReplicaActivityId const & replicaActivityId,
    bool updateExisting,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    ASSERT_IFNOT(pendingSequenceStream_, "{0}: pending sequence stream should exist", replicaActivityId.TraceId);

    HMEvents::Trace->CachePersistSS(
        replicaActivityId.TraceId,
        *pendingSequenceStream_);

    Common::ErrorCode error(ErrorCodeValue::Success);

    error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (updateExisting)
    {
        error = healthManagerReplica_.StoreWrapper.Update(tx, *pendingSequenceStream_);
    }
    else
    {
        error = healthManagerReplica_.StoreWrapper.Insert(tx, *pendingSequenceStream_);
    }

    return error;
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::StartPersistSequenceStream(
    IHealthJobItemSPtr const & jobItem,
    SequenceStreamRequestContext const & context,
    bool updateExisting)
{
    ASSERT_IFNOT(pendingSequenceStream_, "{0}: pending sequence stream should exist", context);
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [updateExisting, this](ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistSequenceStreamTx(replicaActivityId, updateExisting, tx);
        },
            [this](__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistSequenceStreamCompleted(jobItem, operation);
        },
        context.OwnerRemainingTime);

    if (!error.IsSuccess())
    {
        { // lock
            AcquireWriteLock lock(lock_);
            pendingSequenceStream_.reset();
        } // endlock

        this->JobQueueManager.OnWorkComplete(*jobItem, error);
    }
}

template <class TEntityId>
void HealthEntityCache<TEntityId>::OnPersistSequenceStreamCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    ASSERT_IFNOT(pendingSequenceStream_, "{0}: pending sequence stream should exist", jobItem);

    auto error = ReplicatedStoreWrapper::EndCommit(operation);

    { // lock
        AcquireWriteLock lock(lock_);
        // Update in memory sequence stream
        if (error.IsSuccess())
        {
            auto it = sequenceStreamMap_.find(pendingSequenceStream_->SourceId);
            if (it != sequenceStreamMap_.end() && it->second->HighestLsn > pendingSequenceStream_->HighestLsn)
            {
                pendingSequenceStream_->HighestLsn = it->second->HighestLsn;
            }

            sequenceStreamMap_[pendingSequenceStream_->SourceId] = move(pendingSequenceStream_);
        }

        pendingSequenceStream_.reset();
    } // endlock

    // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::GetEntities(
    Common::ActivityId const & activityId,
    __inout std::vector<HealthEntitySPtr> & entities) const
{
    return HealthEntityCache<TEntityId>::GetEntitiesByName(activityId, nullptr, entities);
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::GetEntitiesByName(
    Common::ActivityId const & activityId,
    TEntityId const * name,
    __inout std::vector<HealthEntitySPtr> & entities) const
{
    ErrorCode error(ErrorCodeValue::Success);
    entities.clear();

    { // lock
        AcquireReadLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else
        {
            // Search by name
            if (name != nullptr)
            {
                auto it = entities_.find(*name);
                if (it != entities_.end())
                {
                    entities.push_back(it->second);
                }
            }
            else
            {
                for (auto it = entities_.begin(); it != entities_.end(); ++it)
                {
                    entities.push_back(it->second);
                }
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        wstring byNameMessage = L"list";

        if (name != nullptr)
        {
            byNameMessage = L"by name";
        }

        HealthManagerReplica::WriteWarning(
            TraceComponent,
            "{0}: {1}: failed to get {2} {3}: {4}",
            this->PartitionedReplicaId.TraceId,
            activityId,
            this->EntitiesType,
            move(byNameMessage),
            error);
    }

    return error;
}

template <class TEntityId>
Common::ErrorCode HealthEntityCache<TEntityId>::GetEntities(
    Common::ActivityId const & activityId,
    TEntityId const & continuationToken,
    __inout std::vector<HealthEntitySPtr> & entities) const
{
    ErrorCode error(ErrorCodeValue::Success);
    entities.clear();

    { // lock
        AcquireReadLock lock(lock_);
        if (!acceptRequests_)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else
        {
            auto contTokenIt = entities_.upper_bound(continuationToken);

            if (contTokenIt == entities_.end())
            {
                HealthManagerReplica::WriteInfo(
                    TraceComponent,
                    "{0}: {1}: get {2} with continuation token {3} returned no results",
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    this->EntitiesType,
                    continuationToken);
            }
            else
            {
                HealthManagerReplica::WriteInfo(
                    TraceComponent,
                    "{0}: {1}: get {2} with continuation token {3} start at {4}",
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    this->EntitiesType,
                    continuationToken,
                    contTokenIt->second->EntityIdString);

                for (auto it = contTokenIt; it != entities_.end(); ++it)
                {
                    entities.push_back(it->second);
                }
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        HealthManagerReplica::WriteWarning(
            TraceComponent,
            "{0}: {1}: failed to get {2} list with continuation token {3}: {4}",
            this->PartitionedReplicaId.TraceId,
            activityId,
            this->EntitiesType,
            continuationToken,
            error);
    }

    return error;
}

template <class TEntityId>
FABRIC_SEQUENCE_NUMBER HealthEntityCache<TEntityId>::Test_GetInvalidatedLsn(wstring const & sourceId) const
{
    AcquireReadLock lock(lock_);
    auto it = sequenceStreamMap_.find(sourceId);
    if (it == sequenceStreamMap_.end())
    {
        return FABRIC_INVALID_SEQUENCE_NUMBER;
    }

    return it->second->InvalidateLsn;
}

// Template specializations
TEMPLATE_SPECIALIZATION_ENTITY_ID(Management::HealthManager::HealthEntityCache)
