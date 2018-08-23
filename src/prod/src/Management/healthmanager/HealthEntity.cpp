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
using namespace Query;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("HealthEntity");

HealthEntity::HealthEntity(
    AttributesStoreDataSPtr && attributes,
    __in HealthManagerReplica & healthManagerReplica,
    bool considerSystemErrorUnhealthy,
    bool expectSystemReports,
    HealthEntityState::Enum entityState)
    : entityIdString_(attributes->Key)
    , entityState_(entityState)
    , healthManagerReplica_(healthManagerReplica)
    , attributes_(move(attributes))
    , pendingAttributes_()
    , events_()
    , pendingEvents_()
    , lock_()
    , considerSystemErrorUnhealthy_(considerSystemErrorUnhealthy)
    , hasPendingCleanupTask_(false)
    , checkConsistencyBetweenMemoryAndStoreTime_(DateTime::Now())
    , hasLeakedEvents_(false)
{
    if (!attributes_->IsMarkedForDeletion)
    {
        this->HealthManagerCounters->NumberOfEntities.Increment();
    }

    attributes_->ExpectSystemReports = expectSystemReports;
}

HealthEntity::~HealthEntity()
{
}

bool HealthEntity::IsCleanedUp(
    ActivityId const & activityId,
    __out bool & hasPendingWork)
{
    bool noChildren = CleanupChildren();

    bool shouldMarkAsStale = false;
    bool isCleanedUp = false;
    hasPendingWork = false;

    { // lock
        AcquireReadLock lock(lock_);
        // The entity can be PendingFirstReport if:
        // - the entity was cleaned up. If it has no children, it can be deleted.
        // - the entity was created because of a child. Needs to be kept alive so child can see parent state.
        // - the entity was leaked, so it has pending events. Needs to be kept alive to delete the events.
        // - the entity was invalidated, in which case it's immediately removed from store. The entity must
        // be kept in memory for a while so any depending entities (Eg. replicas when a node is invalidated) can clean up based on parent state.
        if (entityState_.IsPendingFirstReport)
        {
            if (noChildren && events_.empty() && !attributes_->IsRecentlyInvalidated())
            {
                isCleanedUp = true;
                shouldMarkAsStale = (!attributes_->IsCleanedUp);
            }
            else
            {
                isCleanedUp = false;
            }
        }
        else
        {
            isCleanedUp = attributes_->IsCleanedUp;
        }

        if (isCleanedUp)
        {
            // Check whether there is still pending work on this entity.
            // The work may change state, so the entity should not be removed from cache yet.
            // Need to check under the entity lock, since usually work needs the lock to change state and complete
            hasPendingWork = this->JobQueueManager.HasWork(entityIdString_);
            healthManagerReplica_.WriteInfo(
                TraceComponent,
                entityIdString_,
                "{0}: Entity is cleaned up, hasPendingWork={1}: {2}, state {3}, noChildren {4}",
                activityId,
                hasPendingWork,
                *attributes_,
                entityState_.State,
                noChildren);
        }
    } // endlock

    if (shouldMarkAsStale)
    {
        // Since the entity is going to be removed from cache, mark the old attributes as stale and the new attributes as cleaned up
        // to let any children know that they need to update their pointer.
        AcquireWriteLock lock(lock_);
        auto cleanedUpAttributes = attributes_->CreateNewAttributesFromCurrent();
        cleanedUpAttributes->MarkAsCleanedUp();
        attributes_->MarkAsStale();
        attributes_ = move(cleanedUpAttributes);
    }

    return isCleanedUp;
}

bool HealthEntity::ShouldBeCleanedUp() const
{
    AcquireReadLock lock(lock_);

    // The entity is not in store but has events. Need to remove the events.
    // This covers the case when the entity was deleted or cleaned up and it had more than MaxEntityHealthReportsAllowedPerTransaction events.
    if (entityState_.IsPendingFirstReport && !events_.empty() && hasLeakedEvents_)
    {
        return true;
    }

    if (!entityState_.IsReady)
    {
        return false;
    }

    return attributes_->ShouldBeCleanedUp();
}

bool HealthEntity::NoAttributeChangeInGracePeriodCallerHoldsLock() const
{
    if (attributes_->HasNoChanges(ManagementConfig::GetConfig().HealthStoreEntityWithoutSystemReportKeptInterval))
    {
        healthManagerReplica_.WriteNoise(
            TraceComponent,
            "{0}: {1}: no system event in grace period ({2})",
            this->PartitionedReplicaId.TraceId,
            *attributes_,
            ManagementConfig::GetConfig().HealthStoreEntityWithoutSystemReportKeptInterval);
        return true;
    }

    return false;
}

bool HealthEntity::IsDeletedOrCleanedUp() const
{
    AcquireReadLock lock(lock_);
    return IsDeletedOrCleanedUpCallerHoldsLock();
}

bool HealthEntity::IsDeletedOrCleanedUpCallerHoldsLock() const
{
    return attributes_->IsMarkedForDeletion || attributes_->IsCleanedUp;
}

Common::ErrorCode HealthEntity::TryReserveCleanupJob(Common::ActivityId const & activityId)
{
    // If there is no cleanup task on the entity, mark tentatively that cleanup job is allowed,
    // to optimize for the path where there are not many cleanup task in the cache manager.
    bool expected = false;
    if (!hasPendingCleanupTask_.compare_exchange_weak(expected, true))
    {
        HMEvents::Trace->ReserveCleanupJobFailed(entityIdString_, activityId);
        return ErrorCode(ErrorCodeValue::AlreadyExists);
    }

    auto error = healthManagerReplica_.EntityManager->TryCreateCleanupJob(activityId, entityIdString_);
    if (!error.IsSuccess())
    {
        // Reject the cleanup task as there are too many cleanup task in all caches
        expected = true;
        bool flagChanged = hasPendingCleanupTask_.compare_exchange_weak(expected, false);
        ASSERT_IFNOT(flagChanged, "{0}: revert reserved cleanup task: task not marked as started in the entity", entityIdString_);
    }

    return error;
}

void HealthEntity::OnCleanupJobCompleted()
{
    bool expected = true;
    bool flagChanged = hasPendingCleanupTask_.compare_exchange_weak(expected, false);
    ASSERT_IFNOT(flagChanged, "{0}: cleanup task completed but not marked as started in the entity", entityIdString_);
    this->EntityManager->OnCleanupJobCompleted();
}

bool HealthEntity::ShouldBeDeletedDueToParentState(Common::ActivityId const & activityId)
{
    bool deleteDueToNoChildren = ShouldBeDeletedDueToChildrenState();

    // Update parents outside of the lock
    UpdateParents(activityId);

    { // lock
        AcquireReadLock lock(lock_);
        if (!entityState_.IsReady)
        {
            return false;
        }

        if (this->IsDeletedOrCleanedUpCallerHoldsLock())
        {
            return false;
        }

        if (deleteDueToNoChildren)
        {
            // If the entity doesn't expect system reports, it can't be deleted through reports.
            // Delete if it has no children.
            healthManagerReplica_.WriteInfo(
                TraceComponent,
                entityIdString_,
                "{0}: Entity has no system reports and no children, delete: {1}",
                activityId,
                *attributes_);
            return true;
        }

        // Ignore the returned value of AreParentsHealthy, use the set value of deleteEntityDueToParentState
        bool deleteEntityDueToParentState = this->DeleteDueToParentsCallerHoldsLock();

        if (deleteEntityDueToParentState ||
            (!attributes_->HasSystemReport && NoAttributeChangeInGracePeriodCallerHoldsLock()))
        {
            // It has been too long since system report has been received
            // or the parents are in the wrong state
            return true;
        }

        return false;
    } // endlock
}

bool HealthEntity::ShouldAutoCleanupEvents() const
{
    { // lock
        AcquireReadLock lock(lock_);
        if (!entityState_.IsReady)
        {
            return false;
        }

        if (events_.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction))
        {
            // There are more than allowed number of reports per entity.
            // Try to automatically cleanup the events that do not impact health.
            return true;
        }

        return false;
    } // endlock
}

bool HealthEntity::HasExpiredEventChanges()
{
     { // lock
        AcquireWriteLock lock(lock_);
        if (!entityState_.IsReady)
        {
            return false;
        }

        for (auto const & entry : events_)
        {
            bool isExpired = entry->UpdateExpired();
            if (entry->RemoveWhenExpired && isExpired)
            {
                // The event needs to be deleted from store
                return true;
            }

            if (entry->NeedsToBePersistedToStore())
            {
                // Change in expiration info between in memory and persisted data, need to update store entry
                return true;
            }
        }

        return false;
    } // endlock
}

bool HealthEntity::ShouldInvalidate(
    wstring const & sourceId,
    FABRIC_SEQUENCE_NUMBER invalidateLsn,
    Common::ActivityId const & activityId) const
{
    AcquireReadLock lock(lock_);
    if (!entityState_.IsReady)
    {
        return false;
    }

    bool found = false;
    for (auto const & event : events_)
    {
        if (event->SourceId == sourceId)
        {
            found = true;
            if (event->ReportSequenceNumber >= invalidateLsn)
            {
                // A newer report from the source was received, so do not invalidate the entity
                return false;
            }
        }
    }

    if (!found)
    {
        HealthManagerReplica::WriteInfo(
            TraceComponent,
            this->EntityIdString,
            "{0}: no '{1}' report in {2}, skip invalidation",
            activityId,
            sourceId,
            *attributes_);
        return false;
    }

    HealthManagerReplica::WriteInfo(
        TraceComponent,
        this->EntityIdString,
        "{0}: need to cleanup invalidated entity {1} because of {2}:{3}",
        activityId,
        *attributes_,
        sourceId,
        invalidateLsn);
    return true;
}

Common::ErrorCode HealthEntity::HasAttributeMatchCallerHoldsLock(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & hasMatch) const
{
    // By default, only check this entity;
    // if the entity has parents, the derived class should check parent entities as well
    return attributes_->HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
}

bool HealthEntity::Match(
    AttributesMap const & matchAttributes) const
{
    AcquireReadLock lock(lock_);

    if (IsDeletedOrCleanedUpCallerHoldsLock())
    {
        return false;
    }

    for (auto it = matchAttributes.begin(); it != matchAttributes.end(); ++it)
    {
        bool isMatch;
        auto error = this->HasAttributeMatchCallerHoldsLock(it->first, it->second, /*out*/isMatch);
        if (error.IsSuccess())
        {
            if (!isMatch)
            {
                HealthManagerReplica::WriteNoise(
                    TraceComponent,
                    "{0}: {1}: no match for {2}={3}",
                    this->PartitionedReplicaId.TraceId,
                    *attributes_,
                    it->first,
                    it->second);

                return isMatch;
            }
        }
        else
        {
            // Attribute invalid or not set
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: no match for {2}={3}, attribute invalid or not set",
                this->PartitionedReplicaId.TraceId,
                *attributes_,
                it->first,
                it->second);
            return false;
        }
    }

    return true;
}

bool HealthEntity::HasSystemReportCallerHoldsLock()
{
    if (attributes_->HasSystemReport)
    {
        return true;
    }
    else
    {
        HMEvents::Trace->NoSystemReport(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            *attributes_);

        return false;
    }
}

Common::ErrorCode HealthEntity::Close()
{
    { // lock
        AcquireWriteLock lock(lock_);
        entityState_.TransitionClosed();
    } //endlock

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode HealthEntity::GetEventsHealthState(
    Common::ActivityId const & activityId,
    bool considerWarningAsError,
    __out FABRIC_HEALTH_STATE & eventsHealthState)
{
    std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations;
    return GetEventsHealthState(activityId, considerWarningAsError, false, eventsHealthState, unhealthyEvaluations);
}

Common::ErrorCode HealthEntity::GetEventsHealthState(
    Common::ActivityId const & activityId,
    bool considerWarningAsError,
    __out FABRIC_HEALTH_STATE & eventsHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    return GetEventsHealthState(activityId, considerWarningAsError, true, eventsHealthState, unhealthyEvaluations);
}

Common::ErrorCode HealthEntity::GetEventsHealthState(
    Common::ActivityId const & activityId,
    bool considerWarningAsError,
    bool setUnhealthyEvaluations,
    __out FABRIC_HEALTH_STATE & eventsHealthState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    eventsHealthState = FABRIC_HEALTH_STATE_UNKNOWN;

    // Update parents outside of the lock
    UpdateParents(activityId);

    // Check whether we have hierarchy system reports
    bool hasHierarchySystemReport = this->HasHierarchySystemReport;

    { // lock
        AcquireReadLock lock(lock_);
        if (entityState_.IsClosed)
        {
            HMEvents::Trace->GetEventsHealthStateFailedDueToState(
                entityIdString_,
                activityId,
                entityState_.State);
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!entityState_.CanAcceptQuery())
        {
            HMEvents::Trace->GetEventsHealthStateFailedDueToState(
                entityIdString_,
                activityId,
                entityState_.State);
            return ErrorCode(ErrorCodeValue::InvalidState);
        }
        else if (IsDeletedOrCleanedUpCallerHoldsLock())
        {
            healthManagerReplica_.WriteNoise(
                TraceComponent,
                entityIdString_,
                "{0}: GetEventsHealthState failed because entity is deleted or cleaned up",
                activityId);
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().EntityDeletedOrCleanedUp));
        }
        else if (!HasSystemReportCallerHoldsLock())
        {
            // Ignore nodes that are not reported by System.
            // These nodes are cleaned up after the grace period.
            // All system components that report attributes on the node (including node instance)
            // should report with a source that starts with "System".
            // Otherwise, without a System report, these reports are not visible.
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().NoSystemReport));
        }
        else if (!hasHierarchySystemReport)
        {
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().NoHierarchySystemReport));
        }
        else if (this->DeleteDueToParentsCallerHoldsLock())
        {
            // Check that parents are still healthy (or unknown) and this entity doesn't need to be deleted
            healthManagerReplica_.WriteNoise(
                TraceComponent,
                "{0}: {1}: {2}: GetEventsHealthState failed because of parents check",
                this->PartitionedReplicaId.TraceId,
                activityId,
                entityIdString_);
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().DeleteEntityDueToParent, entityIdString_));
        }
        else
        {
            vector<HealthEvent> nonPersistedEvents;
            eventsHealthState = GetEventsHealthStateCallerHoldsLock(
                activityId,
                considerWarningAsError,
                setUnhealthyEvaluations,
                unhealthyEvaluations,
                nonPersistedEvents);
            return ErrorCode(ErrorCodeValue::Success);
        }
    } // endlock
}

//
// Query processing
//
Common::ErrorCode HealthEntity::ProcessQuery(
    __in QueryRequestContext & context)
{
    std::vector<ServiceModel::HealthEvent> queryEvents;
    FABRIC_HEALTH_STATE entityEventsState = FABRIC_HEALTH_STATE_INVALID;
    std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations;

    // Set the events filter if needed
    std::unique_ptr<HealthEventsFilter> eventsFilter;
    if (context.ContextKind == RequestContextKind::QueryEntityDetail)
    {
        auto error = context.GetEventsFilter(eventsFilter);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // Update parents outside of the lock
    UpdateParents(context.ActivityId);

    // Check whether we have hierarchy system reports
    bool hasHierarchySystemReport = this->HasHierarchySystemReport;

    // Query can only be processed once data has been loaded in memory for this entity
    { // lock
        AcquireReadLock lock(lock_);
        if (entityState_.IsClosed)
        {
            // Nothing to do, just notify the job item
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (IsDeletedOrCleanedUpCallerHoldsLock())
        {
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().EntityDeletedOrCleanedUp));
        }
        else if (!entityState_.CanAcceptQuery())
        {
            // When the event is accepted, the entity should be already loaded, so fail request
            healthManagerReplica_.WriteNoise(
                TraceComponent,
                "{0}: {1}: {2}: Can't accept query in state {3}",
                this->PartitionedReplicaId.TraceId,
                context.ActivityId,
                entityIdString_,
                entityState_.State);
            return ErrorCode(ErrorCodeValue::InvalidState);
        }
        else if (!HasSystemReportCallerHoldsLock())
        {
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().NoSystemReport));
        }
        else if (!hasHierarchySystemReport)
        {
            return ErrorCode(ErrorCodeValue::HealthEntityNotFound, wformatString("{0}", Resources::GetResources().NoHierarchySystemReport));
        }
        else if (this->DeleteDueToParentsCallerHoldsLock())
        {
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().DeleteEntityDueToParent, entityIdString_));
        }
        else
        {
            // Parents are still healthy (or unknown) and this entity doesn't need to be deleted
            auto error = UpdateContextHealthPoliciesCallerHoldsLock(context);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (context.ContextKind == RequestContextKind::QueryEntityDetail ||
                context.ContextKind == RequestContextKind::QueryEntityHealthStateChunk ||
                context.ContextKind == RequestContextKind::QueryEntityHealthState)
            {
                // Compute the aggregated health based on the entity events only
                // This will also update the expired state of the event at the moment the health evaluation was done
                vector<HealthEvent> nonPersistedEvents;
                entityEventsState = GetEventsHealthStateCallerHoldsLock(
                    context.ActivityId,
                    context.HealthPolicyConsiderWarningAsError,
                    true, // setUnhealthyEvaluations
                    unhealthyEvaluations,
                    nonPersistedEvents);

                if (context.ContextKind == RequestContextKind::QueryEntityDetail)
                {
                    for (auto const & entry : events_)
                    {
                        if (entry->IsTransientExpiredEvent())
                        {
                            // Ignore transient expired events
                            continue;
                        }

                        bool add = true;
                        if (eventsFilter)
                        {
                            auto healthState = entry->GetEvaluatedHealthState(context.HealthPolicyConsiderWarningAsError);
                            if (!eventsFilter->IsRespected(healthState))
                            {
                                add = false;
                                healthManagerReplica_.WriteNoise(
                                    TraceComponent,
                                    entityIdString_,
                                    "{0}: Skip event {1} with evaluated health state {2} as it doesn't match filter {3}",
                                    context.ActivityId,
                                    *entry,
                                    healthState,
                                    eventsFilter->HealthStateFilter);
                            }
                        }

                        if (add)
                        {
                            queryEvents.push_back(entry->GenerateEvent());
                        }
                    }

                    for (auto && entry : nonPersistedEvents)
                    {
                        ASSERT_IF(entry.State == FABRIC_HEALTH_STATE_OK, "{0}: generated non-persisted event has state Ok: {1}", context.ActivityId, entry);
                        FABRIC_HEALTH_STATE eventState = context.HealthPolicyConsiderWarningAsError ? FABRIC_HEALTH_STATE_ERROR : entry.State;
                        if (!eventsFilter || eventsFilter->IsRespected(eventState))
                        {
                            queryEvents.push_back(move(entry));
                        }
                    }
                }
            }
        }
    } // endlock

    // Compute children aggregated health outside the lock
    switch (context.ContextKind)
    {
    case RequestContextKind::QueryEntityChildren:
    case RequestContextKind::QueryEntityUnhealthyChildren:
        return this->SetChildrenAggregatedHealthQueryResult(context);

    case RequestContextKind::QueryEntityDetail:
    case RequestContextKind::QueryEntityHealthStateChunk:
        return this->SetDetailQueryResult(context, entityEventsState, move(queryEvents), move(unhealthyEvaluations));

    case RequestContextKind::QueryEntityHealthState:
        return this->SetEntityHealthState(context, entityEventsState);

    default:
        Assert::CodingError("{0}: invalid request context", context);
    }
}

//
// Report processing
//
void HealthEntity::StartProcessReport(
    IHealthJobItemSPtr const & jobItem,
    __in vector<ReportRequestContext> & contexts,
    bool isDeleteRequest)
{
    // Contexts preprocessing:
    // - Check if any context is expired
    // - Check if any context is stale compared to the other queued contexts
    // Set error so these reports are not checked under the lock
    ASSERT_IF(contexts.empty(), "{0}: at least one report should be processed", *jobItem);
    vector<size_t> acceptedIndexes;
    for (size_t i = 0; i < contexts.size(); ++i)
    {
        if (isDeleteRequest)
        {
            // For delete requests, do not persist the event;
            // just update the entity state and remove any pending items
            ASSERT_IFNOT(contexts.size() == 1, "{0}: delete reports shouldn't be batched in job item: {1} from {2}", this->EntityIdString, contexts[i], jobItem);
        }

        if (contexts[i].CheckAndSetExpired())
        {
            continue;
        }

        bool isNew = true;
        for (size_t j = 0; j < acceptedIndexes.size(); ++j)
        {
            // Same source and property?
            auto const & previous = contexts[acceptedIndexes[j]];
            if (contexts[i].Report.SourceId == previous.Report.SourceId && contexts[i].Report.Property == previous.Report.Property)
            {
                isNew = false;
                if (contexts[i].Report.SequenceNumber <= previous.Report.SequenceNumber)
                {
                    // New context is stale, reject
                    contexts[i].SetError(ErrorCode(ErrorCodeValue::HealthCheckPending));
                    healthManagerReplica_.WriteNoise(
                        TraceComponent,
                        entityIdString_,
                        "Mark {0} with HealthCheckPending",
                        contexts[i]);
                }
                else
                {
                    // Previous context is stale, reject
                    healthManagerReplica_.WriteNoise(
                        TraceComponent,
                        entityIdString_,
                        "Mark {0} with HealthCheckPending",
                        previous);
                    previous.SetError(ErrorCode(ErrorCodeValue::HealthCheckPending));
                    acceptedIndexes[j] = i;
                }

                break;
            }
        }

        if (isNew)
        {
            acceptedIndexes.push_back(i);
        }
    }

    ErrorCode error(ErrorCodeValue::Success);
    if (acceptedIndexes.empty())
    {
        // No reports are accepted, no need to take the lock
        OnProcessReportFailed(*jobItem, error);
        return;
    }

    bool needsToPersist = false;
    TimeSpan timeout = TimeSpan::MinValue;

    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            // Nothing to do, just notify the job item
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!entityState_.CanAcceptReport())
        {
            // When the event is accepted, the entity should be already loaded, so fail request
            error = ErrorCode(ErrorCodeValue::InvalidState, wformatString("reject report in entity state {0}", entityState_.State));
        }
        else
        {
            // Clear the pending events and attributes, as they may have been left behind by a unsuccessful transaction
            pendingEvents_.clear();
            pendingAttributes_.reset();

            for (auto & context : contexts)
            {
                if (context.IsErrorSet())
                {
                    // Already processed
                    continue;
                }

                if (!TryCreatePendingReportInformationCallerHoldsLock(context, isDeleteRequest))
                {
                    continue;
                }

                auto remainingTime = context.OwnerRemainingTime;
                if (timeout < remainingTime)
                {
                    timeout = remainingTime;
                }

                UpdateEntityInstanceInHealthReport(context);
            }

            needsToPersist = pendingAttributes_ || (!pendingEvents_.empty());

            if (contexts.size() > 1)
            {
                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    entityIdString_,
                    "{0}: Process batched reports: pending events {1}, total contexts {2}",
                    jobItem->ReplicaActivityId,
                    pendingEvents_.size(),
                    contexts.size());
            }
        }
    } // endlock

    if (!error.IsSuccess() || !needsToPersist)
    {
        // Complete work item outside the lock
        // None of the reports have been accepted. Complete the batch with current error.
        // If reports have been completed with an individual error, that error is persisted.
        OnProcessReportFailed(*jobItem, error);
    }
    else if (isDeleteRequest)
    {
        StartPersistDeleteEntity(jobItem, timeout);
    }
    else
    {
        StartPersistCurrentData(jobItem, timeout);
    }
}

void HealthEntity::EndProcessReport(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation,
    bool isDeleteRequest)
{
    if (!isDeleteRequest)
    {
        OnPersistCurrentDataCompleted(jobItem, operation);
    }
    else
    {
        OnPersistDeleteEntityCompleted(jobItem, operation);
    }
}

bool HealthEntity::TryCreatePendingReportInformationCallerHoldsLock(
    __in ReportRequestContext & context,
    bool isDeleteRequest)
{
    auto error = context.Report.ValidateReport();
    if (!error.IsSuccess())
    {
        context.SetError(error);
        return false;
    }

    // If the new event reports attributes for an old instance, do not accept it
    bool skipLsnCheck;
    bool replaceAttributesMetadata;
    if (entityState_.IsPendingFirstReport && (!events_.empty() || attributes_->IsInvalidated))
    {
        // The entity is not persisted, but has events (leaked) or was invalidated, in which case we accept reports for lower instances
        skipLsnCheck = true;
        replaceAttributesMetadata = true;
    }
    else
    {
        error = attributes_->TryAcceptHealthReport(context.Report, /*out*/ skipLsnCheck, /*out*/replaceAttributesMetadata);
        if (!error.IsSuccess())
        {
            healthManagerReplica_.WriteNoise(
                TraceComponent,
                entityIdString_,
                "Can't accept report {0}: error {1} {2}",
                context,
                error,
                error.Message);
            context.SetError(move(error));
            return false;
        }
    }

    // For delete request, do not compare with existing events, there will be no entry for this property
    auto itEvent = events_.end();
    if (!isDeleteRequest)
    {
        // Check source and property staleness
        for (auto it = events_.begin(); it != events_.end(); ++it)
        {
            if ((*it)->Property == context.Report.Property && (*it)->SourceId == context.Report.SourceId)
            {
                // For higher instance, we accept events with lower sequence number, so no check is needed
                if (!skipLsnCheck && !(*it)->CanUpdateEvent(context.Report.SequenceNumber))
                {
                    context.SetError(ErrorCode(ErrorCodeValue::HealthStaleReport));
                    return false;
                }

                // We want to consider the previous state as error if it's expired and removeWhenExpired=False
                // If removeWhenExpired=True, then it doesn't matter since it will be removed anyway.
                if ((*it)->IsExpired && !(*it)->RemoveWhenExpired)
                {
                    context.PreviousState = FABRIC_HEALTH_STATE_ERROR;
                }
                else
                {
                    context.PreviousState = (*it)->State;
                }

                itEvent = it;
                break;
            }
        }
    }

    // If the instance is same, merge the new attributes with the current ones.
    // If the instance is higher, the attributes will be replaced
    // and the existing events cleaned.
    // If the instance is required and not provided, pendingAttributes is set to null.
    // Note that for delete request, the entity state is changed to MarkForDeletion.
    bool hasAttributeChanges;
    this->CreatePendingAttributesCallerHoldsLock(context, replaceAttributesMetadata, isDeleteRequest, hasAttributeChanges);

    // Check whether we can skip persisting the event - no attribute changes
    // and new report has same fields as the previous event.
    // In this case, simply update in memory event data.
    // NOTE: we do not optimize critical reports (system reports with special attributes, see ServiceModel::HealthReport)
    if (!hasAttributeChanges &&
        (itEvent != events_.end()) &&
        (context.Priority != Priority::Critical))
    {
        auto const & previousEvent = *itEvent;
        if (previousEvent->HasSameFields(context.Report))
        {
            if (previousEvent->TryUpdateDiff(context.Report))
            {
                this->HealthManagerCounters->NumberOfOptimizedHealthReports.Increment();

                // No more work needed, complete the request with success regardless of other requests in the same job item
                context.SetError(ErrorCode(ErrorCodeValue::Success, wformatString("Optimized, persisted sn {0}", previousEvent->PersistedReportSequenceNumber)));
                return false;
            }
            else if (!previousEvent->RemoveWhenExpired)
            {
                // Events that have RemoveWhenExpired true must always be persisted, because the expiration is sliding
                // based on the time they are persisted.
                // Only trace for non RemoveWhenExpired events, since that is the event of interest.
                HMEvents::Trace->PersistEventPreviousExpired(
                    entityIdString_,
                    context.ReplicaActivityId.ActivityId,
                    previousEvent->PersistedReportSequenceNumber);
            }
        }
    }

    if (!isDeleteRequest)
    {
        auto pendingEvent = GetStoreData(context);
        pendingEvent->IsInStore = (itEvent != events_.end());
        pendingEvents_.push_back(move(pendingEvent));
    }

    ASSERT_IF(
        pendingAttributes_ && pendingAttributes_->IsCleanedUp,
        "ProcessReport {0}: cleaned up attributes should never be persisted: current {1}, new {2}, state {3}",
        context,
        *attributes_,
        *pendingAttributes_,
        entityState_.State);

    return true;
}

//
// Load store data
//
void HealthEntity::PrepareLoadFromStore(
    AttributesStoreDataSPtr && newAttributes)
{
    { // lock
        AcquireWriteLock lock(lock_);
        ASSERT_IFNOT(
            entityState_.IsPendingFirstReport || entityState_.IsPendingLoadFromStore,
            "{0}: {1}: PrepareLoadFromStore: entity is in wrong state {2}",
            this->PartitionedReplicaId.TraceId,
            *attributes_,
            entityState_.State);
        entityState_.TransitionPendingLoadFromStore();

        // Preserve expect system reports flag that is not persisted to store from previous attributes
        newAttributes->ExpectSystemReports = attributes_->ExpectSystemReports;

        attributes_->MarkAsStale();
        attributes_ = move(newAttributes);
    } // endlock
}

bool HealthEntity::ShouldCheckConsistencyBetweenMemoryAndStore(Common::ActivityId const & activityId)
{
    if (!ManagementConfig::GetConfig().EnableHealthCacheConsistencyCheck)
    {
        return false;
    }

    DateTime lastRun;
    { // lock
        AcquireReadLock lock(lock_);
        if (!entityState_.IsReady && !entityState_.IsPendingFirstReport)
        {
            return false;
        }

        lastRun = checkConsistencyBetweenMemoryAndStoreTime_;
    } // endlock

    auto now = DateTime::Now();
    TimeSpan elapsed = now - lastRun;
    TimeSpan minTime = ManagementConfig::GetConfig().MinHealthCacheConsistencyCheckInterval;

    if (elapsed < minTime)
    {
        // Not yet min delay since last run, no need to run it again
        return false;
    }

    TimeSpan maxTime = ManagementConfig::GetConfig().MaxHealthCacheConsistencyCheckInterval;
    if (elapsed >= maxTime)
    {
        // More than max delay since last run, run it
        return true;
    }

    // Between min and max, use randomization to determine whether we run
    double min = minTime.TotalMillisecondsAsDouble();
    double max = maxTime.TotalMillisecondsAsDouble();
    double diff = max - min;
    if (diff <= 0)
    {
        TRACE_AND_TESTASSERT(
            HealthManagerReplica::WriteWarning,
            TraceComponent,
            entityIdString_,
            "{0}: Invalid configuration: MinHealthCacheConsistencyCheckInterval {1} should be less than MaxHealthCacheConsistencyCheckInterval {2}",
            activityId,
            minTime,
            maxTime);

        return true;
    }

    auto randomizedTime = TimeSpan::FromMilliseconds(min + diff * (static_cast<double>(rand()) / RAND_MAX));
    if (elapsed >= randomizedTime)
    {
        HealthManagerReplica::WriteNoise(
            TraceComponent,
            entityIdString_,
            "{0}: Should check consistency: elapsed {1}, randomized time {2}.",
            activityId,
            elapsed,
            randomizedTime);
        return true;
    }

    return false;
}

// NOTE: the job item is scheduled by EntityJobQueueManager, which makes sure it is not executed in parallel
// with other entity work.
// Otherwise, we would have races because of timing of commit reports:
// we could load old data from store and check it against new in-memory data (saved after reports are persisted),
// or we could load new data from store and check it against old in-memory data (not yet saved because it's waiting for lock).
void HealthEntity::CheckInMemoryAgainstStoreData(
    IHealthJobItemSPtr const & jobItem)
{
    Stopwatch stopwatch;
    stopwatch.Start();

    auto const & activityId = jobItem->ReplicaActivityId.ActivityId;

    // Load the attributes from store outside the lock
    auto storeAttributes = GetAttributesCopy();
    auto attributesLoadError = healthManagerReplica_.StoreWrapper.ReadExact(activityId, *storeAttributes);

    // Load the events from store outside the lock
    vector<HealthEventStoreDataUPtr> storeEvents;
    auto eventsLoadError = this->LoadEventsFromStore(activityId, storeEvents);

    // Check events and attributes from store with in-memory data
    ErrorCode error(ErrorCodeValue::Success);
    bool isInSync = true;
    { // lock
        AcquireReadLock lock(lock_);
        if (entityState_.IsClosed)
        {
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (pendingAttributes_)
        {
            // See note above: the memory checkup should only happen when reports are not processed
            TRACE_AND_TESTASSERT(
                healthManagerReplica_.WriteWarning,
                TraceComponent,
                entityIdString_,
                "{0}: Skip CheckInMemoryAgainstStoreData: there are pending attributes, which means there is a report processed: {1}",
                activityId,
                pendingAttributes_);
        }
        else if (!pendingEvents_.empty())
        {
            // See note above: the memory checkup should only happen when reports are not processed
            TRACE_AND_TESTASSERT(
                healthManagerReplica_.WriteWarning,
                TraceComponent,
                entityIdString_,
                "{0}: Skip CheckInMemoryAgainstStoreData: there are pending events, which means there is a report processed. First pending event: {1}",
                activityId,
                *pendingEvents_[0]);
        }
        else if (attributesLoadError.IsSuccess())
        {
            // The attributes are persisted in store.
            // Expected: entity IsInStore=true
            if (!entityState_.IsInStore)
            {
                healthManagerReplica_.WriteWarning(
                    TraceComponent,
                    entityIdString_,
                    "{0}: CheckInMemoryAgainstStoreData: In memory entity IsInStore=false, state={1}. In Store: persisted attributes {2}",
                    activityId,
                    entityState_.State,
                    *storeAttributes);
                isInSync = false;
            }
            else
            {
                if (attributes_->InstanceId != storeAttributes->InstanceId)
                {
                    healthManagerReplica_.WriteWarning(
                        TraceComponent,
                        entityIdString_,
                        "{0}: CheckInMemoryAgainstStoreData: instance mismatch: In memory: {1}. In Store: {2}",
                        activityId,
                        *attributes_,
                        *storeAttributes);
                    isInSync = false;
                }
                else
                {
                    isInSync = VerifyInMemoryAgainstStoreEventsCallerHoldsLock(activityId, eventsLoadError, storeEvents);
                }
            }
        }
        else if (attributesLoadError.IsError(ErrorCodeValue::NotFound))
        {
            // Attributes do not exist in store.
            // Expected: entity IsInStore=false, and no events should be persisted
            if (entityState_.IsInStore)
            {
                healthManagerReplica_.WriteWarning(
                    TraceComponent,
                    entityIdString_,
                    "{0}: CheckInMemoryAgainstStoreData: In memory: entity IsInStore=true, state {1}, attributes {2}. In Store: no attributes persisted",
                    activityId,
                    entityState_.State,
                    *attributes_);
                isInSync = false;
            }
            else
            {
                isInSync = VerifyInMemoryAgainstStoreEventsCallerHoldsLock(activityId, eventsLoadError, storeEvents);
            }
        }
        else
        {
            healthManagerReplica_.WriteInfo(
                TraceComponent,
                entityIdString_,
                "{0}: CheckInMemoryAgainstStoreData: error loading attributes from store: {1}. Skip check",
                activityId,
                attributesLoadError);
            isInSync = VerifyInMemoryAgainstStoreEventsCallerHoldsLock(activityId, eventsLoadError, storeEvents);
        }
    } //endlock

    stopwatch.Stop();

    if (!isInSync)
    {
        healthManagerReplica_.ReportTransientFault(ErrorCode(ErrorCodeValue::InconsistentInMemoryState));
    }
    else
    {
        { // lock
            AcquireWriteLock lock(lock_);
            HMEvents::Trace->CheckInMemoryAgainstStoreData(entityIdString_, activityId, error, stopwatch.Elapsed, *attributes_);
            checkConsistencyBetweenMemoryAndStoreTime_ = DateTime::Now();
        } // endlock
    }

    this->JobQueueManager.OnWorkComplete(*jobItem, error);
}

bool HealthEntity::VerifyInMemoryAgainstStoreEventsCallerHoldsLock(
    Common::ActivityId const & activityId,
    Common::ErrorCode const & eventsLoadError,
    vector<HealthEventStoreDataUPtr> const & storeEvents)
{
    if (attributes_->ExpectSystemReports && attributes_->HasSystemReport)
    {
        bool expectZeroEvents = (!entityState_.IsInStore || attributes_->IsMarkedForDeletion || attributes_->IsCleanedUp);
        if ((expectZeroEvents != events_.empty()) || (eventsLoadError.IsSuccess() && (expectZeroEvents != storeEvents.empty())))
        {
            // This shouldn't happen, unless there are either leaked events (when entity doesn't exist) or missing events (when entity exists).
            // We shouldn't bring the process down in production, but we need to stop the tests to understand what is wrong.
            TRACE_AND_TESTASSERT(
                healthManagerReplica_.WriteWarning,
                TraceComponent,
                entityIdString_,
                "{0}: VerifyInMemoryAgainstStore: expectedZeroEvents={1}, instead event count: in-store={2}, in-memory={3}, entityState={4}, {5}",
                activityId,
                expectZeroEvents,
                storeEvents.size(),
                events_.size(),
                entityState_.State,
                *attributes_);
        }
    }

    if (!eventsLoadError.IsSuccess())
    {
        healthManagerReplica_.WriteInfo(
            TraceComponent,
            entityIdString_,
            "{0}: CheckInMemoryAgainstStoreData: error reading the events: {1}. Skip check",
            activityId,
            eventsLoadError);
        return true;
    }

    // Map that tracks in store events per key
    unordered_map<wstring, HealthEventStoreData*> storeEventKeys;
    for (auto const & storeEvent : storeEvents)
    {
        storeEventKeys.insert(make_pair(storeEvent->Key, storeEvent.get()));
    }

    bool hasSystemReport = false;
    int systemErrorCount = 0;
    for (auto const & event : events_)
    {
        UpdateSystemDataCallerHoldsLock(*event, hasSystemReport, systemErrorCount);

        auto it = storeEventKeys.find(event->Key);
        if (it == storeEventKeys.end())
        {
            // The event is in memory, but not in store
            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: VerifyInMemoryAgainstStore: event {1} is in memory but not in store. {2}",
                activityId,
                *event,
                *attributes_);
            return false;
        }
        else if (it->second->State != event->State ||
                 it->second->TimeToLive != event->TimeToLive ||
                 it->second->RemoveWhenExpired != event->RemoveWhenExpired)
        {
            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: VerifyInMemoryAgainstStore: in-memory event {1} doesn't match in-store {2}. {3}",
                activityId,
                *event,
                *it->second,
                *attributes_);
            return false;
        }

        storeEventKeys.erase(it);
    }

    if (!storeEventKeys.empty())
    {
        // The event is in store, but not in memory.
        // It's possible that the events were deleted from memory in a wrongly fashion or
        // the events were leaked in store because of another bug.
        // The purpose of reporting fault is to fix the in-memory state based on the in-store state.
        // If the entity is in store, reloading will help; otherwise, the events will not be loaded,
        // since the events are only loaded for existing entities.
        healthManagerReplica_.WriteWarning(
            TraceComponent,
            entityIdString_,
            "{0}: VerifyInMemoryAgainstStore: {1} events are in store but not in memory. First event: {2}. IsInStore: {3}. {4}",
            activityId,
            storeEventKeys.size(),
            *storeEventKeys.cbegin(),
            entityState_.IsInStore,
            *attributes_);
        if (entityState_.IsInStore)
        {
            return false;
        }
    }

    // Check consistency of the system report stats
    if (attributes_->ExpectSystemReports)
    {
        if (hasSystemReport != attributes_->HasSystemReport)
        {
            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: attributes hasSystemReport is incorrectly computed. {1}",
                activityId,
                *attributes_);
            return false;
        }

        if (systemErrorCount != attributes_->SystemErrorCount)
        {
            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: systemErrorCount mismatch, should be {1}. {2}",
                activityId,
                systemErrorCount,
                *attributes_);
            return false;
        }
    }

    return true;
}

Common::ErrorCode HealthEntity::AddStoreLeakedEvents(
    Common::ActivityId const & activityId,
    std::vector<HealthEventStoreDataUPtr> && storeEvents)
{
    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        if (!entityState_.IsPendingFirstReport)
        {
            TRACE_LEVEL_AND_TESTASSERT(
                healthManagerReplica_.WriteInfo,
                TraceComponent,
                "{0}: {1}: {2}: AddStoreLeakedEvents: entity is not in desired state {3}",
                this->PartitionedReplicaId.TraceId,
                activityId,
                *attributes_,
                entityState_.State);

            return ErrorCodeValue::InvalidState;
        }

        auto healthInfoKind = HealthEntityKind::GetHealthInformationKind(this->EntityKind, attributes_->UseInstance);

        // Put the events on the entity, but do not update any system statistics.
        for (auto && event : storeEvents)
        {
            event->UpdateOnLoadFromStore(healthInfoKind);
            events_.push_back(move(event));
        }

        hasLeakedEvents_ = true;
    } //endlock

    return ErrorCode::Success();
}

Common::ErrorCode HealthEntity::LoadStoreData(
    Common::ActivityId const & activityId,
    std::vector<HealthEventStoreDataUPtr> && storeEvents)
{
    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        if (!entityState_.CanAcceptLoadFromStore())
        {
            TRACE_LEVEL_AND_TESTASSERT(
                healthManagerReplica_.WriteInfo,
                TraceComponent,
                "{0}: {1}: {2}: LoadStoreData: entity is not in desired state {3}",
                this->PartitionedReplicaId.TraceId,
                activityId,
                *attributes_,
                entityState_.State);

            return ErrorCodeValue::InvalidState;
        }

        auto healthInfoKind = HealthEntityKind::GetHealthInformationKind(this->EntityKind, attributes_->UseInstance);

        bool hasSystemReport = false;
        int systemErrorCount = 0;
        for (auto && event : storeEvents)
        {
            UpdateSystemDataCallerHoldsLock(*event, hasSystemReport, systemErrorCount);

            event->UpdateOnLoadFromStore(healthInfoKind);
            events_.push_back(move(event));
        }

        if (attributes_->ExpectSystemReports)
        {
            attributes_->HasSystemReport = hasSystemReport;
            attributes_->UpdateSystemErrorCount(systemErrorCount);
        }

        // If the entity is marked for delete, there should be no events.
        // However, if there are any events left behind, we need to clean them up.
        if (attributes_->IsCleanedUp)
        {
            // Cleaned up state should never be persisted to store.
            // If it is, it's because of a previous bug.
            // Remove the cleaned up flag and mark the entity for delete, without changing the last modified time
            // The next time the cleanup timer fires and the grace period is elapsed, this entity will be cleaned up from store.
            // During this time, reports for the same instance will be rejected with entity deleted.
            auto newAttributes = attributes_->CreateNewAttributesFromCurrent();
            newAttributes->PrepareCleanup();

            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: {1}: Load from memory returned cleaned up attributes {2} and {3} events. Replace with {4}",
                this->PartitionedReplicaId.TraceId,
                activityId,
                *attributes_,
                events_.size(),
                *newAttributes);

            attributes_->MarkAsStale();
            attributes_ = move(newAttributes);
        }

        entityState_.TransitionReady();
    } // endlock

    this->OnEntityReadyToAcceptRequests(activityId);
    this->CreateOrUpdateNonPersistentParents(activityId);

    return ErrorCode::Success();
}

void HealthEntity::UpdateInMemoryDataCallerHoldsLock(Store::ReplicaActivityId const & replicaActivityId)
{
    // If the instance is same, update the map;
    // if the instance is higher, clean old events and add new one
    bool removeOldEvents = false;
    bool updateAttributesInMemoryOnly = false;

    // Cache system error reports to avoid some string comparison
    vector<bool> systemReports(pendingEvents_.size(), false);
    bool hasSystemReport = 0;
    int systemErrorCountDiff = 0;
    if (attributes_->ExpectSystemReports)
    {
        for (size_t i = 0; i < pendingEvents_.size(); ++i)
        {
            if (pendingEvents_[i]->IsSystemReport)
            {
                hasSystemReport = true;
                systemReports[i] = true;
                if (considerSystemErrorUnhealthy_ && pendingEvents_[i]->IsStatePropertyError)
                {
                    ++systemErrorCountDiff;
                }
            }
        }
    }

    if (pendingAttributes_)
    {
        ASSERT_IF(
            pendingAttributes_->IsCleanedUp,
            "{0}: ProcessReportComplete: invalid pending attributes: current {1}, new {2}, state {3}",
            replicaActivityId.TraceId,
            *attributes_,
            *pendingAttributes_,
            entityState_.State);
        if (!entityState_.IsInStore)
        {
            entityState_.TransitionReady();
        }

        removeOldEvents = attributes_->ShouldReplaceAttributes(*pendingAttributes_);
        if (removeOldEvents)
        {
            // Remove all previous reports and add only the new events
            events_.clear();
            pendingAttributes_->ResetSystemInfo();
            for (auto && pendingEvent : pendingEvents_)
            {
                events_.push_back(move(pendingEvent));
            }
        }

        hasLeakedEvents_ = false;

        if (!pendingAttributes_->IsMarkedForDeletion && IsDeletedOrCleanedUpCallerHoldsLock())
        {
            // Entity was marked for deletion and now it is not; increment number of entities
            this->HealthManagerCounters->NumberOfEntities.Increment();
        }
    }

    if (!removeOldEvents)
    {
        // Assumption: the new events are all for different source + property
        int pendingEventIndex = 0;
        for (auto && pendingEvent : pendingEvents_)
        {
            if (pendingEvent->IsInStore)
            {
                // Find the event that needs to be replaced and update it
                for (auto & oldEvent : events_)
                {
                    if (pendingEvent->Property == oldEvent->Property && pendingEvent->SourceId == oldEvent->SourceId)
                    {
                        // Replace existing event for same event id
                        // If old event is system error, remember previous system error to adjust stats
                        if (considerSystemErrorUnhealthy_ && systemReports[pendingEventIndex] && oldEvent->IsStatePropertyError)
                        {
                            --systemErrorCountDiff;
                        }

                        oldEvent->Update(move(*pendingEvent));
                        break;
                    }
                }
            }
            else
            {
                // Add the event as new entry
                events_.push_back(move(pendingEvent));
            }

            ++pendingEventIndex;
        }

        // If we need to update attributes system stats and pending attributes are not set, create them
        // They will be changed in memory only.
        // Since we are only adding events, we need to change if the system error count changed
        // or if the attributes have no system report and the new events are system
        if (attributes_->ExpectSystemReports &&
            !pendingAttributes_ &&
            (systemErrorCountDiff != 0 || (!attributes_->HasSystemReport && hasSystemReport)))
        {
            // Create new attributes for any in memory stats changes
            updateAttributesInMemoryOnly = true;
            pendingAttributes_ = attributes_->CreateNewAttributesFromCurrent();
        }
    }

    if (pendingAttributes_)
    {
        if (attributes_->ExpectSystemReports)
        {
            pendingAttributes_->HasSystemReport |= hasSystemReport;
            pendingAttributes_->UpdateSystemErrorCount(systemErrorCountDiff);
        }

        // Mark the old attributes as stale and replace the shared pointer with the new one
        attributes_->MarkAsStale();
        attributes_ = move(pendingAttributes_);

        HMEvents::Trace->ProcessReportUpdateAttributes(
            entityIdString_,
            replicaActivityId.ActivityId,
            *attributes_,
            updateAttributesInMemoryOnly,
            entityState_.State);
    }
}

//
// Delete entity due to parent state (called on the cache cleanup timer)
//
void HealthEntity::StartDeleteEntity(
    IHealthJobItemSPtr const & jobItem)
{
    ErrorCode error(ErrorCodeValue::Success);
    bool deleteDueToNoChildren = ShouldBeDeletedDueToChildrenState();
    UpdateParents(jobItem->ReplicaActivityId.ActivityId);

    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            // Nothing to do, just notify the job item
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (IsDeletedOrCleanedUpCallerHoldsLock())
        {
            // Already deleted, no need to delete again
            // Do not set error message here, as it is not sent back to the client
            error = ErrorCode(ErrorCodeValue::HealthEntityNotFound);
        }
        else if (!entityState_.CanAcceptReport())
        {
            // When the event is accepted, the entity should be already loaded, so fail request
            error = ErrorCode(ErrorCodeValue::InvalidState);
        }
        else
        {
            bool shouldDelete = false;

            // Check that system event was received lately
            if (!attributes_->HasSystemReport && NoAttributeChangeInGracePeriodCallerHoldsLock())
            {
                // It has been too long since system report has been received
                shouldDelete = true;
            }
            else if (!attributes_->ExpectSystemReports)
            {
                shouldDelete = deleteDueToNoChildren;
            }
            else
            {
                // Update parents; if they became healthy, do not delete.
                shouldDelete |= this->DeleteDueToParentsCallerHoldsLock();
            }

            if (shouldDelete)
            {
                HMEvents::Trace->DeleteEntityFromStore(
                    entityIdString_,
                    jobItem->ReplicaActivityId.ActivityId,
                    *attributes_,
                    entityState_.State);

                pendingEvents_.clear();
                pendingAttributes_ = attributes_->CreateNewAttributesFromCurrent();
                pendingAttributes_->MarkForDeletion();
            }
            else
            {
                // Use StaleRequest to let the job item know that the delete was not executed, as it was not needed at this time.
                error = ErrorCode(ErrorCodeValue::StaleRequest);
            }
        }
    } // endlock

    if (error.IsSuccess())
    {
        StartPersistDeleteEntity(jobItem, ManagementConfig::GetConfig().MaxOperationTimeout);
    }
    else
    {
        // Complete work item outside the lock
        this->JobQueueManager.OnWorkComplete(*jobItem, error);
    }
}

void HealthEntity::EndDeleteEntity(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    OnPersistDeleteEntityCompleted(jobItem, operation);
}

//
// Cleanup entity
//
void HealthEntity::StartCleanupEntity(
    IHealthJobItemSPtr const & jobItem,
    std::wstring const & invalidateSource,
    FABRIC_SEQUENCE_NUMBER invalidateLsn)
{
    // Cleanup entity only if the state indicates that it's marked for deletion and
    // more than grace period has passed since expiration time
    // The check is needed because from the time cleanup was enqueued in the job queue,
    // a new report with a higher instance may have came in and been processed.
    // In that case, we don't want to remove the new entity.
    if (ShouldBeCleanedUp() ||
        ((invalidateLsn != FABRIC_INVALID_SEQUENCE_NUMBER) && ShouldInvalidate(invalidateSource, invalidateLsn, jobItem->ReplicaActivityId.ActivityId)))
    {
        StartPersistCleanupEntity(jobItem);
    }
    else
    {
        // Complete work item outside the lock
        // Use StaleRequest to let the job item know that the delete was not executed, as it was not needed at this time.
        this->JobQueueManager.OnWorkComplete(*jobItem, ErrorCode(ErrorCodeValue::StaleRequest));
    }
}

void HealthEntity::EndCleanupEntity(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    OnPersistCleanupEntityCompleted(jobItem, operation);
}

//
// Auto-Cleanup of events when there are too many events
//
void HealthEntity::StartAutoCleanupEvents(
    IHealthJobItemSPtr const & jobItem)
{
    ErrorCode error(ErrorCodeValue::Success);

    { // lock
        AcquireReadLock lock(lock_);
        if (entityState_.IsClosed)
        {
            // Nothing to do, just notify the job item
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!entityState_.IsReady)
        {
            error = ErrorCode(ErrorCodeValue::InvalidState);
        }
    } // endlock

    if (!error.IsSuccess())
    {
        // Complete work item outside the lock
        this->JobQueueManager.OnWorkComplete(*jobItem, error);
        return;
    }

    StartPersistAutoCleanupEvents(jobItem);
}

void HealthEntity::EndAutoCleanupEvents(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    OnPersistAutoCleanupEventsCompleted(jobItem, operation);
}

//
// Cleanup expired transient events
//
void HealthEntity::StartCleanupExpiredTransientEvents(
    IHealthJobItemSPtr const & jobItem)
{
    ErrorCode error(ErrorCodeValue::Success);

    { // lock
        AcquireReadLock lock(lock_);
        if (entityState_.IsClosed)
        {
            // Nothing to do, just notify the job item
            error = ErrorCode(ErrorCodeValue::ObjectClosed);
        }
        else if (!entityState_.IsReady)
        {
            error = ErrorCode(ErrorCodeValue::InvalidState);
        }
    } // endlock

    if (!error.IsSuccess())
    {
        // Complete work item outside the lock
        this->JobQueueManager.OnWorkComplete(*jobItem, error);
        return;
    }

    StartPersistExpiredEventChanges(jobItem);
}

void HealthEntity::EndCleanupExpiredTransientEvents(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    OnPersistExpiredEventChangesCompleted(jobItem, operation);
}

void HealthEntity::ReplaceInMemoryAttributes(AttributesStoreDataSPtr && attributes)
{
    AcquireWriteLock lock(lock_);
    ASSERT_IF(attributes_->ExpectSystemReports, "{0}: ReplaceInMemoryAttributes called with {1}", attributes_, attributes);

    if (!entityState_.IsInStore)
    {
        entityState_.TransitionReady();
    }

    attributes_->MarkAsStale();
    swap(attributes_, attributes);
    healthManagerReplica_.WriteInfo(
        TraceComponent,
        "{0}: {1}: Replaced in memory attributes",
        this->PartitionedReplicaId.TraceId,
        attributes_);
}

//
// Persist report to store
//
Common::ErrorCode HealthEntity::PreparePersistCurrentDataTx(
    Store::ReplicaActivityId const & replicaActivityId,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    // 1. Persist the attributes, if any values have been changed

    // Note that if the instance is newer, all old entries must be first deleted.
    // If the instance is same, just add or replace the new event.
    bool removeOldEvents = false;
    if (pendingAttributes_)
    {
        ASSERT_IF(
            pendingAttributes_->IsCleanedUp,
            "ProcessReport: cleaned up attributes should never be persisted: current {0}, new {1}, state {2}",
            *attributes_,
            *pendingAttributes_,
            entityState_.State);

        removeOldEvents = attributes_->ShouldReplaceAttributes(*pendingAttributes_);

        if (entityState_.IsInStore)
        {
            error = healthManagerReplica_.StoreWrapper.Update(tx, *pendingAttributes_);
        }
        else
        {
            error = healthManagerReplica_.StoreWrapper.Insert(tx, *pendingAttributes_);
        }

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    if (removeOldEvents)
    {
        int deletedEvents = 0;
        int maxEventsToDelete = ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction;
        for (auto it = events_.begin(); it != events_.end(); ++it)
        {
            // Delete all events except the new one (to avoid StoreConflict due to same record
            // being updated twice in the same transaction)
            for (auto const & pendingEvent : pendingEvents_)
            {
                if (pendingEvent->IsInStore &&
                    ((*it)->Property == pendingEvent->Property && (*it)->SourceId == pendingEvent->SourceId))
                {
                    continue;
                }

                error = healthManagerReplica_.StoreWrapper.Delete(tx, **it);
                if (!error.IsSuccess())
                {
                    return error;
                }

                if (++deletedEvents > maxEventsToDelete)
                {
                    // Not all events can be deleted. Reject the new report.
                    // This can only happen when the health reporting is faulty and there are too many reports on the entity.
                    // The mitigation is for users to delete some of the events manually.
                    // The error sent back to the health client is a retryable one, since the next time the client sends
                    // this report, the number of reports may be smaller and the report can succeed.
                    healthManagerReplica_.WriteWarning(
                        TraceComponent,
                        entityIdString_,
                        "{0}: ProcessReport can't remove old events. There are {1} events, max allowed {2}. Reject report",
                        replicaActivityId.ActivityId,
                        events_.size(),
                        maxEventsToDelete);

                    return ErrorCode(ErrorCodeValue::MaxResultsReached);
                }
            }
        }
    }

    // 2. Persist the events - either overwrite existing entry or just insert it
    for (auto const & pendingEvent : pendingEvents_)
    {
        if (pendingEvent->IsInStore)
        {
            error = healthManagerReplica_.StoreWrapper.Update(tx, *pendingEvent);
        }
        else
        {
            error = healthManagerReplica_.StoreWrapper.Insert(tx, *pendingEvent);
        }

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

void HealthEntity::StartPersistCurrentData(
    IHealthJobItemSPtr const & jobItem,
    Common::TimeSpan const & timeout)
{
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [this](ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistCurrentDataTx(replicaActivityId, tx);
        },
        [this] (__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistCurrentDataCompleted(jobItem, operation);
        },
        timeout);

    if (!error.IsSuccess())
    {
        OnProcessReportFailed(*jobItem, error);
    }
}

void HealthEntity::OnProcessReportFailed(
    __in IHealthJobItem & jobItem,
    Common::ErrorCode const & error)
{
    { //lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsPendingWriteToStore)
        {
            // Change the state to pending first report, since this report could not be applied
            entityState_.TransitionPendingFirstReport();
        }

        pendingEvents_.clear();
        pendingAttributes_.reset();
    } // endlock

    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

void HealthEntity::OnPersistCurrentDataCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    auto error = ReplicatedStoreWrapper::EndCommit(operation);
    bool wasReadyToAcceptRequests = true;

    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            error.Overwrite(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
        else
        {
            wasReadyToAcceptRequests = entityState_.IsInStore;

            if (error.IsSuccess())
            {
                // Update in memory entries:
                // add or replace an older version of the event
                // and update attributes is needed
                UpdateInMemoryDataCallerHoldsLock(jobItem.ReplicaActivityId);
            }
            else if (ReplicatedStoreWrapper::ShowsInconsistencyBetweenMemoryAndStore(error))
            {
                // Write info about the entity state and the operation that caused the error
                wstring traceDetails;
                StringWriter writer(traceDetails);
                writer.Write("Commit report failed: {0} {1}: entity state = {2}", error, error.Message, entityState_.State);
                if (pendingAttributes_)
                {
                    writer.Write(", pendingAttributes = {0}", *pendingAttributes_);
                }

                for (auto const & pendingEvent : pendingEvents_)
                {
                    writer.Write(", [{0} sn={1} IsInStore={2}]", pendingEvent->Key, pendingEvent->ReportSequenceNumber, pendingEvent->IsInStore);
                }

                healthManagerReplica_.WriteWarning(TraceComponent, entityIdString_, "{0}", traceDetails);

                // Check consistency with store should be done asap.
                // We can't report fault directly, as the error may come from store tombstone logic,
                // and not be a real HM memory inconsistency.
                checkConsistencyBetweenMemoryAndStoreTime_ = DateTime::Zero;
            }
            else if (entityState_.IsPendingWriteToStore)
            {
                // Change the state to pending first report, since this report could not be applied
                entityState_.TransitionPendingFirstReport();
            }
        }

        pendingEvents_.clear();
        pendingAttributes_.reset();
    } // endlock

    if (error.IsSuccess())
    {
        if (!wasReadyToAcceptRequests)
        {
            this->OnEntityReadyToAcceptRequests(jobItem.ReplicaActivityId.ActivityId);
        }

        // Update or create the non-persisted parent
        // if not done so previously and if the attributes needed are set.
        CreateOrUpdateNonPersistentParents(jobItem.ReplicaActivityId.ActivityId);
    }

    // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

//
// Persist delete entity request to store:
// mark this instance of the entity for tombstoning
//
Common::ErrorCode HealthEntity::PreparePersistDeleteEntityTx(
    Store::ReplicaActivityId const & replicaActivityId,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    ASSERT_IFNOT(pendingEvents_.empty(), "{0}: StartPersistDeleteEntity: pending events should not exist: count = {1}", replicaActivityId.TraceId, pendingEvents_.size());
    ASSERT_IFNOT(pendingAttributes_, "{0}: {1}: StartPersistDeleteEntity: pending attributes should exist", replicaActivityId.TraceId, *attributes_);
    ASSERT_IF(pendingAttributes_->IsCleanedUp, "{0}: pendingAttributes are in invalid state: current {1}, new {2}, state {3}", replicaActivityId.TraceId, *attributes_, *pendingAttributes_, entityState_.State);

    ErrorCode error(ErrorCodeValue::Success);

    error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    // 1. Persist attributes with state marked for deletion
    if (entityState_.IsInStore)
    {
        error = healthManagerReplica_.StoreWrapper.Update(tx, *pendingAttributes_);
    }
    else
    {
        error = healthManagerReplica_.StoreWrapper.Insert(tx, *pendingAttributes_);
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    // 2. Delete the events that are in memory
    error = AddDeleteEventsToTx(replicaActivityId.ActivityId, tx);

    return error;
}

void HealthEntity::StartPersistDeleteEntity(
    IHealthJobItemSPtr const & jobItem,
    TimeSpan const & timeout)
{
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [this](ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistDeleteEntityTx(replicaActivityId, tx);
        },
        [this] (__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistDeleteEntityCompleted(jobItem, operation);
        },
        timeout);

    if (!error.IsSuccess())
    {
        { // lock
            AcquireWriteLock lock(lock_);
            if (entityState_.IsPendingWriteToStore)
            {
                // Change the state to pending first report, since this report could not be applied
                entityState_.TransitionPendingFirstReport();
            }

            pendingEvents_.clear();
            pendingAttributes_.reset();
        } // endlock

        this->JobQueueManager.OnWorkComplete(*jobItem, error);
    }
}

void HealthEntity::OnPersistDeleteEntityCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    auto error = ReplicatedStoreWrapper::EndCommit(operation);

    ASSERT_IFNOT(pendingAttributes_, "{0}: {1}: OnPersistDeleteEntityCompleted: pending attributes should exist", this->PartitionedReplicaId, *attributes_);

    bool wasReadyToAcceptRequests = true;

    { // lock
        AcquireWriteLock lock(lock_);

        if (error.IsSuccess())
        {
            // Update in memory entries:
            // remove all events and replace the attributes

            attributes_->MarkAsStale();
            attributes_ = move(pendingAttributes_);

            attributes_->ResetSystemInfo();

            // Clear the events that were deleted from store.
            UpdateDeletedEventsCallerHoldsLock();
            if (!entityState_.IsInStore)
            {
                // The entry for the attributes was added to store, marked for delete
                entityState_.TransitionReady();
                wasReadyToAcceptRequests = false;
            }
        }
        else if (entityState_.IsPendingWriteToStore)
        {
            // Change the state to pending first report, since this report could not be applied
            entityState_.TransitionPendingFirstReport();
        }

        pendingEvents_.clear();
        pendingAttributes_.reset();
    } // endlock

    if (error.IsSuccess())
    {
        this->HealthManagerCounters->NumberOfEntities.Decrement();
        if (!wasReadyToAcceptRequests)
        {
            this->OnEntityReadyToAcceptRequests(jobItem.ReplicaActivityId.ActivityId);
        }

        // Do not create non-persisted parents, since attributes are not set
    }

    // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

Common::ErrorCode HealthEntity::AddDeleteEventsToTx(
    Common::ActivityId const & activityId,
    __in Store::IStoreBase::TransactionSPtr & tx)
{
    if (events_.empty())
    {
        // Nothing to do.
        return ErrorCode::Success();
    }

    int deletedEvents = 0;
    wstring traceDetails;
    StringWriter writer(traceDetails);
    int maxEventsToDelete = ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction;
    ErrorCode error(ErrorCodeValue::Success);
    int traceCount = 0;
    for (auto const & entry : events_)
    {
        // Trace the first few events that are deleted
        AppendEventDataForTrace(writer, entry, traceCount);

        error = healthManagerReplica_.StoreWrapper.Delete(tx, *entry);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (++deletedEvents >= maxEventsToDelete)
        {
            // Not all events can be deleted. Delete only up to max allowed size, the rest will be leaked.
            healthManagerReplica_.WriteWarning(
                TraceComponent,
                entityIdString_,
                "{0}: tx.Delete {1} events, the rest up to {2} events are leaked and will be cleaned up later. First deleted events: {3}",
                activityId,
                maxEventsToDelete,
                events_.size(),
                traceDetails);
            return error;
        }
    }

    healthManagerReplica_.WriteInfo(
        TraceComponent,
        entityIdString_,
        "{0}: tx.Delete {1} events. First deleted events: {2}",
        activityId,
        events_.size(),
        traceDetails);
    return error;
}

void HealthEntity::UpdateDeletedEventsCallerHoldsLock()
{
    size_t maxEventsToDelete = static_cast<size_t>(ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction);
    if (events_.size() <= maxEventsToDelete)
    {
        events_.clear();
        hasLeakedEvents_ = false;
    }
    else
    {
        // Only some events are deleted, remove them from memory. The rest should be kept to be cleaned up later.
        auto itBegin = events_.begin();
        auto itEnd = itBegin;
        advance(itEnd, maxEventsToDelete);
        events_.erase(itBegin, itEnd);
        healthManagerReplica_.WriteInfo(
            TraceComponent,
            entityIdString_,
            "tx.Delete removed {0} events, left {1} leaked events.",
            maxEventsToDelete,
            events_.size());
        hasLeakedEvents_ = true;
    }
}

void HealthEntity::AppendEventDataForTrace(
    __in StringWriter & writer,
    HealthEventStoreDataUPtr const & entry,
    __inout int & traceCount)
{
    static int maxTraceCount = 50;
    if (++traceCount <= maxTraceCount)
    {
        // Write few events per line
        if (traceCount % 4 == 0)
        {
            writer.WriteLine();
        }

        writer.Write("({0}+{1} sn={2}); ", entry->SourceId, entry->Property, entry->ReportSequenceNumber);
    }
}

//
// Persist cleanup entity:
// Remove the entity from the store
//
Common::ErrorCode HealthEntity::PreparePersistCleanupEntityTx(
    Store::ReplicaActivityId const & replicaActivityId,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    // If the entity is not in store and has no events, there's nothing to be cleaned up.
    if (entityState_.IsPendingFirstReport)
    {
        if (events_.empty() || !hasLeakedEvents_)
        {
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        HMEvents::Trace->CleanupLeakedEvents(
            this->EntityIdString,
            replicaActivityId.TraceId,
            events_.size(),
            *attributes_);
    }
    else
    {
        // Start cleaning up an existing entity in the store
        HMEvents::Trace->Cleanup(
            this->EntityIdString,
            replicaActivityId.TraceId,
            *attributes_);
    }

    ErrorCode error(ErrorCodeValue::Success);

    error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Delete the entity.
    // Since no report is accepted while the check whether cleanup should be performed,
    // there's no need to check whether the instance of the entity match.
    if (!entityState_.IsPendingFirstReport)
    {
        error = healthManagerReplica_.StoreWrapper.Delete(tx, *attributes_);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // Delete any events
    error = AddDeleteEventsToTx(replicaActivityId.ActivityId, tx);
    return error;
}

void HealthEntity::StartPersistCleanupEntity(
    IHealthJobItemSPtr const & jobItem)
{
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [this](Store::ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistCleanupEntityTx(replicaActivityId, tx);
        },
        [this] (__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistCleanupEntityCompleted(jobItem, operation);
        },
        ManagementConfig::GetConfig().MaxOperationTimeout);

    TESTASSERT_IFNOT(error.IsSuccess(), "{0}: Cleanup entity: start commit job item failed", jobItem);
}

void HealthEntity::OnPersistCleanupEntityCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    auto error = ReplicatedStoreWrapper::EndCommit(operation);

    if (error.IsSuccess())
    {
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            error.Overwrite(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
        else
        {
            if (!attributes_->IsMarkedForDeletion)
            {
                this->HealthManagerCounters->NumberOfEntities.Decrement();
            }

            auto cleanedUpAttributes = attributes_->CreateNewAttributesFromCurrent();
            cleanedUpAttributes->MarkAsCleanedUp();

            attributes_->MarkAsStale();
            attributes_ = move(cleanedUpAttributes);

            // Remove the entity events from memory.
            UpdateDeletedEventsCallerHoldsLock();

            // The entity is no longer in store; if a new instance is reported, use Insert rather than Update
            // when writing attributes
            entityState_.TransitionPendingFirstReport();
        }
    }

    // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

//
// Auto cleanup of events when there are too many events.
//
Common::ErrorCode HealthEntity::PreparePersistAutoCleanupEventsTx(
    Store::ReplicaActivityId const & replicaActivityId,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    auto error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<std::list<HealthEventStoreDataUPtr>::const_iterator> eventsToBeDeletedIts;
    size_t maxAllowed = static_cast<size_t>(ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction);
    size_t initialSize;
    wstring traceDetails;
    StringWriter writer(traceDetails);

    // 2 cleanup job items should not be executed in parallel.
    // If the second one comes here, it's because the first one is executing the commit async.
    // Check IsPendingUpdateToStore and if any event has it set, do not run, wait until next cleanup task is scheduled.
    // The executing task will update the flag when it's done.
    { // lock
        AcquireReadLock lock(lock_);
        initialSize = events_.size();
        if (initialSize > maxAllowed)
        {
            size_t count = 0;
            int traceCount = 0;
            for (auto it = events_.cbegin(); it != events_.cend(); ++it)
            {
                auto & entry = *it;

                if (entry->IsPendingUpdateToStore)
                {
                    healthManagerReplica_.WriteInfo(
                        TraceComponent,
                        entityIdString_,
                        "{0}: The event {1} is already marked IsPendingUpdateToStore. {2}",
                        replicaActivityId.TraceId,
                        *entry,
                        *attributes_);
                    return ErrorCode(ErrorCodeValue::AlreadyExists);
                }

                // Only delete events that do not impact health.
                if (entry->DoesNotImpactHealth())
                {
                    AppendEventDataForTrace(writer, entry, traceCount);

                    eventsToBeDeletedIts.push_back(it);
                    entry->IsPendingUpdateToStore = true;
                    if (++count > maxAllowed)
                    {
                        // Added upto max number of reports per tx.
                        break;
                    }

                    if (initialSize - count <= maxAllowed)
                    {
                        // The remaining reports are below the critical level.
                        break;
                    }
                }
            }
        }
    } // endlock

    if (eventsToBeDeletedIts.empty())
    {
        // No work needed
        healthManagerReplica_.WriteInfo(
            TraceComponent,
            entityIdString_,
            "{0}: Entity has {1} events, max allowed = {2}. Auto-cleanup found no events to remove.",
            replicaActivityId.ActivityId,
            initialSize,
            maxAllowed,
            eventsToBeDeletedIts.size());
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    healthManagerReplica_.WriteInfo(
        TraceComponent,
        entityIdString_,
        "{0}: Entity has {1} events, max allowed = {2}. Auto-cleanup {3} events that do not impact health. First events: {4}",
        replicaActivityId.ActivityId,
        initialSize,
        maxAllowed,
        eventsToBeDeletedIts.size(),
        traceDetails);

    // It's ok to access the iterators outside the lock, since the job queue manager ensures only one job queue is active at one time.
    for (auto const & it : eventsToBeDeletedIts)
    {
        error = healthManagerReplica_.StoreWrapper.Delete(tx, *(*it));
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

void HealthEntity::StartPersistAutoCleanupEvents(
    IHealthJobItemSPtr const & jobItem)
{
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [this](Store::ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistAutoCleanupEventsTx(replicaActivityId, tx);
        },
            [this](__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistAutoCleanupEventsCompleted(jobItem, operation);
        },
        ManagementConfig::GetConfig().MaxOperationTimeout);

    TESTASSERT_IFNOT(error.IsSuccess(), "{0}: Auto-Cleanup events: start commit job item failed with {1}", *jobItem, error);
}

void HealthEntity::OnPersistAutoCleanupEventsCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    auto error = ReplicatedStoreWrapper::EndCommit(operation);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        // Nothing to do, no events were found, no need to take lock
        this->JobQueueManager.OnWorkComplete(jobItem, error);
        return;
    }

    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            error.Overwrite(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
        else if (error.IsSuccess())
        {
            // Auto-cleanup only removes events that can't impact system count, so no changes to System error/report count are needed.
            for (auto it = events_.begin(); it != events_.end();)
            {
                auto & entry = *it;
                if (entry->IsPendingUpdateToStore)
                {
                    events_.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            ResetPendingEventsCallerHoldsLock();
        }
    } //endlock

      // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

//
// Persist cleanup expired transient events: Remove the events from the store
// Persist events where recent expired status doesn't match persisted one: update event in store
//
Common::ErrorCode HealthEntity::PreparePersistExpiredEventChangesTx(
    Store::ReplicaActivityId const & replicaActivityId,
    __out Store::IStoreBase::TransactionSPtr & tx)
{
    auto error = healthManagerReplica_.StoreWrapper.CreateSimpleTransaction(replicaActivityId.ActivityId, tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Create new events to be persisted to disk
    vector<HealthEventStoreDataUPtr> eventsToBeUpdatedToStore;

    // Indexes for events that need to be changed. Set under lock
    vector<std::list<HealthEventStoreDataUPtr>::const_iterator> eventsToBeDeletedIts;

    // 2 cleanup job items should not be executed in parallel.
    // If the second one comes here, it's because the first one is executing the commit async.
    // Check IsPendingUpdateToStore and if any event has it set, do not run, wait until next cleanup task is scheduled.
    // The executing task will update the flag when it's done.
    { // lock
        AcquireReadLock lock(lock_);
        for (auto const & entry: events_)
        {
            if (entry->IsPendingUpdateToStore)
            {
                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    entityIdString_,
                    "{0}: The event {1} is already marked IsPendingUpdateToStore. {2}",
                    replicaActivityId.TraceId,
                    *entry,
                    *attributes_);
                return ErrorCode(ErrorCodeValue::AlreadyExists);
            }
        }
    } // endlock

    size_t maxAllowed = static_cast<size_t>(ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction);

    { // lock
        // Take write lock because UpdateExpired can change the cached expired state
        AcquireWriteLock lock(lock_);
        for (auto it = events_.cbegin(); it != events_.cend(); ++it)
        {
            if (eventsToBeDeletedIts.size() + eventsToBeUpdatedToStore.size() > maxAllowed)
            {
                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    entityIdString_,
                    "{0}: Events to be deleted / updated are more than MaxEntityHealthReportsAllowedPerTransaction {1}, processing accumulated events.",
                    replicaActivityId.ActivityId,
                    maxAllowed);
                break;
            }

            auto & entry = *it;

            // Update expired information and get events that need to be modified
            entry->UpdateExpired();

            if (entry->IsTransientExpiredEvent())
            {
                eventsToBeDeletedIts.push_back(it);

                // The pending operation is marked on the event and
                // if the store delete fails, the flag is cleared.
                // If the store operation succeeds, the event will be removed from memory.
                entry->IsPendingUpdateToStore = true;
                continue;
            }

            if (entry->NeedsToBePersistedToStore())
            {
                HMEvents::Trace->PersistReportExpiredMismatch(
                    entityIdString_,
                    replicaActivityId.TraceId,
                    entry->SourceId,
                    entry->Property,
                    entry->ReportSequenceNumber,
                    entry->IsExpired);

                // Do not modify existing event, create a new copy with values from diff.
                // If persist to store succeeds, the event will be updated, otherwise, the current event is not changed.
                HealthEventStoreDataUPtr newEvent;
                entry->GetDiffEvent(replicaActivityId, newEvent);
                eventsToBeUpdatedToStore.push_back(move(newEvent));
                entry->IsPendingUpdateToStore = true;
            }
        }
    } // endlock

    if (eventsToBeDeletedIts.empty() && eventsToBeUpdatedToStore.empty())
    {
        // No work needed
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    HMEvents::Trace->CleanupExpiredTransientEventsStart(
        replicaActivityId.TraceId,
        *attributes_,
        static_cast<uint64>(eventsToBeDeletedIts.size()),
        static_cast<uint64>(eventsToBeUpdatedToStore.size()));

    // It's ok to access the iterators outside the lock, since the job queue manager ensures only one job queue is active at one time.
    // Only queries are not executed on the job queue manager, so changing the events state must be protected by lock.
    for (auto const & entry : eventsToBeUpdatedToStore)
    {
        error = healthManagerReplica_.StoreWrapper.Update(tx, *entry);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    for (auto const & it : eventsToBeDeletedIts)
    {
        // As an optimization, do not create another event, simply delete the existing one, as it is not used by queries anymore.
        error = healthManagerReplica_.StoreWrapper.Delete(tx, *(*it));
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

void HealthEntity::StartPersistExpiredEventChanges(
    IHealthJobItemSPtr const & jobItem)
{
    auto error = healthManagerReplica_.StartCommitJobItem(
        jobItem,
        [this](Store::ReplicaActivityId const & replicaActivityId, __out Store::IStoreBase::TransactionSPtr & tx)->ErrorCode
        {
            return this->PreparePersistExpiredEventChangesTx(replicaActivityId, tx);
        },
        [this] (__in IHealthJobItem & jobItem, AsyncOperationSPtr const & operation)
        {
            this->OnPersistExpiredEventChangesCompleted(jobItem, operation);
        },
        ManagementConfig::GetConfig().MaxOperationTimeout);

    TESTASSERT_IFNOT(error.IsSuccess(), "{0}: Cleanup expired events: start commit job item failed", *jobItem);
}

void HealthEntity::OnPersistExpiredEventChangesCompleted(
    __in IHealthJobItem & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    auto error = ReplicatedStoreWrapper::EndCommit(operation);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        // Nothing to do, no events were found, no need to take lock
        this->JobQueueManager.OnWorkComplete(jobItem, error);
        return;
    }

    { // lock
        AcquireWriteLock lock(lock_);
        if (entityState_.IsClosed)
        {
            error.Overwrite(ErrorCode(ErrorCodeValue::ObjectClosed));
        }
        else if (error.IsSuccess())
        {
            // Some of the expired events may belong to System.
            // Rebuild system report and system error metrics if they changed
            bool hasSystemReport = false;
            int systemErrorCount = 0;
            for (auto it = events_.begin(); it != events_.end();)
            {
                auto & entry = *it;
                if (entry->IsPendingUpdateToStore)
                {
                    // If it's cleaned up transient entry, remove it from memory
                    if (entry->RemoveWhenExpired)
                    {
                        ASSERT_IFNOT(entry->IsExpired, "{0}: {1}: Unexpected pending update entry: event should be transient expired: {2}", this->EntityIdString, jobItem.ReplicaActivityId.ActivityId, *it);

                        // Trace that an event has expired. Only trace if the expiry has an impact on health state.
                        // So, do not trace events with OK health status, in order to cut down on clutter in the traces.
                        if (entry->State != FABRIC_HEALTH_STATE_OK)
                        {
                            HMEvents::Trace->TraceEventExpiredOperational(
                                jobItem.EntityKind,
                                entry,
                                attributes_);
                        }

                        events_.erase(it++);
                        this->HealthManagerCounters->NumberOfExpiredTransientHealthReports.Increment();
                    }
                    else
                    {
                        ASSERT_IFNOT(entry->NeedsToBePersistedToStore(), "{0}: {1}: Unexpected pending update entry {2}", this->EntityIdString, jobItem.ReplicaActivityId.ActivityId, *it);

                        // Because of optimizations, NeedsToBePersisted checks if in memory and persisted is different (for expired tracker), in cases where the
                        // source, property, TTL, and description are the same. This means that it's possible that the in the store, we have an expired event
                        // that is becoming un-expired. We don't want to trace expired in that case.
                        if (entry->IsExpired)
                        {
                            // Trace that an event has expired
                            HMEvents::Trace->TraceEventExpiredOperational(
                                jobItem.EntityKind,
                                entry,
                                attributes_);
                        }


                        // Update in place - move diff into current persisted
                        entry->MoveDiffToCurrent(jobItem.ReplicaActivityId);
                        entry->IsPendingUpdateToStore = false;
                        this->HealthManagerCounters->NumberOfDelayedPersistedHealthReports.Increment();

                        // Since the report is still persisted, update the system error count
                        UpdateSystemDataCallerHoldsLock(*entry, hasSystemReport, systemErrorCount);

                        ++it;
                    }
                }
                else
                {
                    UpdateSystemDataCallerHoldsLock(*entry, hasSystemReport, systemErrorCount);
                    ++it;
                }
            }

            if (attributes_->ExpectSystemReports &&
                (attributes_->HasSystemReport != hasSystemReport || attributes_->SystemErrorCount != systemErrorCount))
            {
                auto newAttributes = attributes_->CreateNewAttributesFromCurrent();
                newAttributes->ResetSystemInfo();
                newAttributes->HasSystemReport = hasSystemReport;
                newAttributes->UpdateSystemErrorCount(systemErrorCount);

                attributes_->MarkAsStale();
                attributes_ = move(newAttributes);
            }
        }
        else
        {
            ResetPendingEventsCallerHoldsLock();
        }
    } //endlock

    // Notify current context of the result
    this->JobQueueManager.OnWorkComplete(jobItem, error);
}

void HealthEntity::ResetPendingEventsCallerHoldsLock()
{
    for (auto & entry : events_)
    {
        if (entry->IsPendingUpdateToStore)
        {
            entry->IsPendingUpdateToStore = false;
        }
    }
}

void HealthEntity::UpdateSystemDataCallerHoldsLock(
    HealthEventStoreData const & event,
    __out bool & hasSystemReport,
    __out int & systemErrorCount) const
{
    if (attributes_->ExpectSystemReports && event.IsSystemReport)
    {
        hasSystemReport = true;
        if (considerSystemErrorUnhealthy_ && event.IsStatePropertyError)
        {
            ++systemErrorCount;
        }
    }
}

void HealthEntity::UpdateEntityInstanceInHealthReport(__in ReportRequestContext & context)
{
    // In EntityHealthInformation, check that HasUnknownInstance is true before updating the EntityInstance value
    auto & entity = context.Report.EntityInformation;

    if (entity->HasUnknownInstance() && attributes_->UseInstance)
    {
        entity->EntityInstance = attributes_->InstanceId;
    }
}

void HealthEntity::CreatePendingAttributesCallerHoldsLock(
    ReportRequestContext const & context,
    bool replaceAttributesMetadata,
    bool isDeleteRequest,
    __out bool & hasAttributeChanges)
{
    auto const & healthReport = context.Report;

    if (pendingAttributes_)
    {
        // Processing multiple reports in a job item, they should all have the same instance
        // Simply merge the attributes
        pendingAttributes_->SetAttributes(healthReport.AttributeInfo);
        hasAttributeChanges = !healthReport.AttributeInfo.Attributes.empty();
        return;
    }

    if (replaceAttributesMetadata ||
        (attributes_->UseInstance && healthReport.EntityInformation->EntityInstance > attributes_->InstanceId))
    {
        // When the instance is higher or attributes changed some metadata, create new attributes and ignore previous values
        // Eg. for replica, current attributes specify stateless kind and new attributes are for stateful kind.
        pendingAttributes_ = attributes_->CreateNewAttributes(context);
    }
    else
    {
        // Same instance or the entity has no instance
        if (entityState_.IsInStore && !isDeleteRequest && !attributes_->ShouldUpdateAttributeInfo(healthReport.AttributeInfo))
        {
            hasAttributeChanges = false;
            return;
        }

        // Copy all existing attributes from previous attributes
        pendingAttributes_ = attributes_->CreateNewAttributesFromCurrent();
        pendingAttributes_->ReInitializeTracing(context.ReplicaActivityId);

        if (!entityState_.IsInStore)
        {
            // If the entity is not in store, write attributes so the entity is persisted, even if no attributes were passed in the report
            // InstanceId is set to Unknown if the entity has instance.
            // Retain the old attributes, normally they should be empty but for ad-hoc
            // and system application entity, there is a default policy.
            // Reset any previous entity state flags.
            pendingAttributes_->ResetState();
        }
    }

    hasAttributeChanges = true;
    if (isDeleteRequest)
    {
        // Change state and ignore any attributes, either old or passed in
        pendingAttributes_->MarkForDeletion();
    }
    else
    {
        pendingAttributes_->UpdateStateModifiedTime();
        pendingAttributes_->SetAttributes(healthReport.AttributeInfo);
    }
}

// Evaluates the aggregated health state of the events only, based on the considerWarningAsError part of the policy.
// It sets unhealthy evaluations if required by the caller. The unhealthy evaluations are only needed for
// health queries (eg. Get<Entity>Health - GetClusterHealth, GetApplicationHealth etc) and upgrade queries (eg. IsClusterHealthy)
// and not for the general queries, like Get<Entity>List. The general queries only need the aggregated health state.
// It sets the non persisted events that HM generates to flag conditions like no authority reports or too many health reports per entity.
FABRIC_HEALTH_STATE HealthEntity::GetEventsHealthStateCallerHoldsLock(
    Common::ActivityId const & activityId,
    bool considerWarningAsError,
    bool setUnhealthyEvaluations,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<ServiceModel::HealthEvent> & nonPersistedEvents)
{
    // Compute the latest count of different health states

    // NOTE: Some optimizations were tried here:
    // 1. Keep the first expiration time and compute the count of ok, warning and error
    // at that time. If a query comes before that time, no work is done. Otherwise, the counts
    // are updated and the new expiration time is saved.
    // This approach outperformed the simple one when the event count was above 200.

    // 2. Keep a multimap with keys expiration times and entries raw pointers to the events.
    // Keep accurate counts of ok, warning and error state. Update them accordingly when an event is added/replaced.
    // When Query comes, remove all events where expiration time is less than the query time.
    // This approach outperformed a lot the simple one, but only when event count was above 400.

    // Since the expected average number of events is small, the simple approach gives best code simplicity-performance trade off.

    // Check authority source report for all entities that expect system reports
    bool hasAuthoritySourceReport = !attributes_->ExpectSystemReports;

    if (!entityState_.IsReady)
    {
        return FABRIC_HEALTH_STATE_OK;
    }

    int okCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int expiredEventsCount = 0;

    // Check system data is set correctly
#ifdef DBG
    bool hasSystemReport = !attributes_->ExpectSystemReports;
    int systemErrorCount = 0;
#endif

    auto firstExpired = events_.end();
    auto firstWarning = events_.end();
    auto firstError = events_.end();

    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        auto & event = *(*it);

        bool ignoreReport = false;
        if (event.UpdateExpired())
        {
            // Ignore expired transient events
            if (event.RemoveWhenExpired)
            {
                ignoreReport = true;
            }
            else
            {
                if (firstExpired == events_.end())
                {
                    firstExpired = it;
                }

                ++expiredEventsCount;
            }
        }
        else
        {
            switch(event.State)
            {
            case FABRIC_HEALTH_STATE_OK:
                ++okCount;
                break;
            case FABRIC_HEALTH_STATE_WARNING:
                if (firstWarning == events_.end())
                {
                    firstWarning = it;
                }

                ++warningCount;
                break;
            case FABRIC_HEALTH_STATE_ERROR:
                if (firstError == events_.end())
                {
                    firstError = it;
                }

                ++errorCount;
                break;
            default:
                Assert::CodingError("{0}: Invalid event state", *it);
            }
        }

        if (!ignoreReport)
        {
            if (!hasAuthoritySourceReport)
            {
                // Update authority report information if applicable
                hasAuthoritySourceReport = (event.Priority == Priority::Higher || event.Priority == Priority::Critical);
            }
        }

#ifdef DBG
            UpdateSystemDataCallerHoldsLock(event, hasSystemReport, systemErrorCount);
#endif
    }

#ifdef DBG
    if (attributes_->ExpectSystemReports)
    {
        ASSERT_IFNOT(
            hasSystemReport == attributes_->HasSystemReport,
            "{0}: {1}: hasSystemReport should be {2}",
            this->PartitionedReplicaId.TraceId,
            *attributes_,
            hasSystemReport);
        ASSERT_IFNOT(
            systemErrorCount == attributes_->SystemErrorCount,
            "{0}: {1}: systemErrorCount should be {2}",
            this->PartitionedReplicaId.TraceId,
            *attributes_,
            systemErrorCount);
    }
#endif

    int totalUnhealthy = warningCount + errorCount + expiredEventsCount;

    FABRIC_HEALTH_STATE aggregatedState = FABRIC_HEALTH_STATE_OK;
    if (errorCount > 0)
    {
        aggregatedState = FABRIC_HEALTH_STATE_ERROR;
        if (setUnhealthyEvaluations)
        {
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<EventHealthEvaluation>(aggregatedState, (*firstError)->GenerateEvent(), considerWarningAsError)));
        }
    }
    else if (expiredEventsCount > 0)
    {
        aggregatedState = FABRIC_HEALTH_STATE_ERROR;
        if (setUnhealthyEvaluations)
        {
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<EventHealthEvaluation>(aggregatedState, (*firstExpired)->GenerateEvent(), considerWarningAsError)));
        }
    }
    else if (warningCount > 0)
    {
        aggregatedState = considerWarningAsError ? FABRIC_HEALTH_STATE_ERROR : FABRIC_HEALTH_STATE_WARNING;
        if (setUnhealthyEvaluations)
        {
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<EventHealthEvaluation>(aggregatedState, (*firstWarning)->GenerateEvent(), considerWarningAsError)));
        }
    }

    if (!hasAuthoritySourceReport)
    {
        // If the entity doesn't have an instance, a System component reported without instance.
        // For example, Infrastructure Service. This can happen in scale-up and down scenarios, where the node is removed from FM
        // but Infrastructure service reports on the Azure view. Since FM may never report on this node,
        // do not let it impact cluster upgrades.
        FABRIC_HEALTH_STATE authorityReportState = (attributes_->HasInvalidInstance && !considerWarningAsError) ? FABRIC_HEALTH_STATE_WARNING : FABRIC_HEALTH_STATE_ERROR;
        wstring reportDescription;
        if (setUnhealthyEvaluations)
        {
            reportDescription = GetMissingAuthorityReportHealthDescription();
        }

        AddGeneratedEventsCallerHoldsLock(
            activityId,
            *ServiceModel::Constants::AuthorityReportProperty,
            authorityReportState,
            move(reportDescription),
            setUnhealthyEvaluations,
            considerWarningAsError,
            aggregatedState,
            unhealthyEvaluations,
            nonPersistedEvents);
    }

    // Ignore the events that are expired and pending remove from store
    int totalEvents = totalUnhealthy + okCount;
    int maxSuggested = ManagementConfig::GetConfig().MaxSuggestedNumberOfEntityHealthReports;
    if (totalEvents > maxSuggested)
    {
        // Too many health reports
        FABRIC_HEALTH_STATE tooManyEventsHealthState = considerWarningAsError ? FABRIC_HEALTH_STATE_ERROR : FABRIC_HEALTH_STATE_WARNING;
        wstring reportDescription;
        if (setUnhealthyEvaluations)
        {
            reportDescription = GetTooManyReportsHealthDescription(totalEvents, maxSuggested);
        }

        AddGeneratedEventsCallerHoldsLock(
            activityId,
            *ServiceModel::Constants::HealthReportCountProperty,
            tooManyEventsHealthState,
            move(reportDescription),
            setUnhealthyEvaluations,
            considerWarningAsError,
            aggregatedState,
            unhealthyEvaluations,
            nonPersistedEvents);
    }

    if (aggregatedState != FABRIC_HEALTH_STATE_OK)
    {
        HMEvents::Trace->GetHealth(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            activityId,
            *attributes_,
            okCount,
            warningCount,
            errorCount,
            expiredEventsCount,
            considerWarningAsError,
            wformatString(aggregatedState),
            hasAuthoritySourceReport);
    }

    return aggregatedState;
}

void HealthEntity::AddGeneratedEventsCallerHoldsLock(
    Common::ActivityId const & activityId,
    std::wstring const & reportProperty,
    FABRIC_HEALTH_STATE reportState,
    std::wstring && reportDescription,
    bool setUnhealthyEvaluations,
    bool considerWarningAsError,
    __inout FABRIC_HEALTH_STATE & aggregatedState,
    __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
    __inout std::vector<ServiceModel::HealthEvent> & nonPersistedEvents)
{
    bool evaluationsNotSet = false;
    if (aggregatedState < reportState)
    {
        aggregatedState = reportState;
        evaluationsNotSet = true;
    }

    if (setUnhealthyEvaluations)
    {
        // Always set the health report to be returned, but only set the unhealthy evaluation if the aggregated health state was impacted.
        auto report = GenerateNonPersistedEvent(
            activityId,
            reportProperty,
            move(reportDescription),
            reportState);
        nonPersistedEvents.push_back(move(report));
        if (evaluationsNotSet)
        {
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<EventHealthEvaluation>(aggregatedState, nonPersistedEvents.back(), considerWarningAsError)));
        }
    }
}

std::wstring HealthEntity::GetMissingAuthorityReportHealthDescription() const
{
    auto authoritySources = HealthReport::GetAuthoritySources(HealthEntityKind::GetHealthInformationKind(this->EntityKind, attributes_->UseInstance));
    ASSERT_IF(authoritySources.empty(), "{0}: GetMissingAuthorityReportHealthDescription: there are no authority sources for this entity kind", *attributes_);
    return wformatString(
        HMResource::GetResources().EntityMissingRequiredReport,
        authoritySources);
}

std::wstring HealthEntity::GetTooManyReportsHealthDescription(
    int totalEvents,
    int maxSuggested) const
{
    return wformatString(
        HMResource::GetResources().TooManyHealthReports,
        totalEvents,
        maxSuggested);
}

ServiceModel::HealthEvent HealthEntity::GenerateNonPersistedEvent(
    Common::ActivityId const & activityId,
    std::wstring const & property,
    std::wstring && description,
    FABRIC_HEALTH_STATE state) const
{
    auto now = DateTime::Now();
    auto lastOkTransition = DateTime::Zero;
    auto lastWarningTransition = DateTime::Zero;
    auto lastErrorTransition = DateTime::Zero;

    switch (state)
    {
    case FABRIC_HEALTH_STATE_OK:
        lastOkTransition = now;
        break;
    case FABRIC_HEALTH_STATE_WARNING:
        lastWarningTransition = now;
        break;
    case FABRIC_HEALTH_STATE_ERROR:
        lastErrorTransition = now;
        break;
    default:
        Assert::CodingError("{0}: {1}: GenerateNonPersistedEvent called with {2}, not supported", this->EntityIdString, activityId, state);
    }

    healthManagerReplica_.WriteInfo(
        TraceComponent,
        entityIdString_,
        "{0}: Generate HM report {1} - '{2}'",
        activityId,
        property,
        description);

    return HealthEvent(
        *ServiceModel::Constants::HealthReportHMSource,
        property,
        TimeSpan::MaxValue, // TTL
        state,
        move(description),
        0, // report sequence number
        now, // source timestamp
        now, // last modified timestamp
        false, // is expired
        false, // remove when expired
        lastOkTransition,
        lastWarningTransition,
        lastErrorTransition);
}

bool HealthEntity::UpdateHealthState(
    Common::ActivityId activityId,
    HealthEntityKind::Enum childrenType,
    HealthCount const & healthCount,
    BYTE maxUnhealthy,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState)
{
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        FABRIC_HEALTH_STATE healthState = healthCount.GetHealthState(maxUnhealthy);
        HMEvents::Trace->ChildrenState(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            activityId,
            childrenType,
            healthCount,
            wformatString(healthState),
            maxUnhealthy);

        if (aggregatedHealthState < healthState)
        {
            aggregatedHealthState = healthState;
            return true;
        }
    }

    return false;
}

ErrorCode HealthEntity::SetChildrenAggregatedHealthQueryResult(
    __in QueryRequestContext & context)
{
    Assert::CodingError("{0}: {1} is not supported", this->EntityIdString, context);
}

ErrorCode HealthEntity::SetEntityHealthState(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE)
{
    Assert::CodingError("{0}: {1} is not supported", this->EntityIdString, context);
}

void HealthEntity::CheckListPagerErrorAndTrace(
    Common::ErrorCode const & error,
    std::wstring const & entityString,
    Common::ActivityId const & activityId,
    std::wstring const & listPagerItemId,
    __out bool & hasError,
    __out bool & errorBenign)
{
    errorBenign = true;
    hasError = false;
    if (error.IsError(ErrorCodeValue::EntryTooLarge))
    {
        HMEvents::Trace->Query_MaxMessageSizeReached(
            entityString,
            activityId,
            listPagerItemId,
            error,
            error.Message);
        hasError = true;
    }
    else if (error.IsError(ErrorCodeValue::MaxResultsReached))
    {
        // This just prints where the error came from and the error message returned by list pager. Through structured tracing.
        HMEvents::Trace->Query_MaxResultsReached(
            entityString,
            activityId,
            listPagerItemId,
            error,
            error.Message);
        hasError = true;
    }
    else if (!error.IsSuccess())
    {
        HMEvents::Trace->InternalMethodFailed(
            entityString,
            activityId,
            L"TryAdd",
            error,
            error.Message);
        errorBenign = false;
        hasError = true;
    }
}

// *******************************************
// Test hooks
// *******************************************
bool HealthEntity::Test_HasHierarchySystemReport() const
{
    if (!HasHierarchySystemReport)
    {
        return false;
    }

    return GetAttributesCopy()->HasSystemReport;
}

bool HealthEntity::Test_GetData(
    __out HealthEntityState::Enum & entityState,
    __inout shared_ptr<AttributesStoreData> & attributes,
    __inout vector<HealthEvent> & events) const
{
    AcquireReadLock lock(lock_);
    entityState = entityState_.State;
    if (entityState_.IsReady || entityState_.IsPendingFirstReport)
    {
        attributes = attributes_;
        for (auto it = events_.begin(); it != events_.end(); ++it)
        {
            if (!(*it)->IsExpired || !(*it)->RemoveWhenExpired)
            {
                // Create copy of event
                events.push_back((*it)->GenerateEvent());
            }
        }

        return true;
    }

    return false;
}

bool HealthEntity::Test_CorruptEntity(
    Common::ActivityId const & activityId,
    bool changeEntityState,
    bool changeHasSystemReport,
    bool changeSystemErrorCount,
    std::vector<ServiceModel::HealthInformation> && addReports,
    std::vector<std::pair<std::wstring, std::wstring>> const & deleteEventKeys,
    std::vector<std::tuple<std::wstring, std::wstring, FABRIC_HEALTH_STATE>> const & scrambleEventKeys,
    __inout HealthEntityState::Enum & entityState,
    __inout size_t & eventCount)
{
    AcquireWriteLock lock(lock_);
    if (entityState_.IsClosed)
    {
        healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: state is closed, do nothing", activityId);
        return false;
    }

    if (changeEntityState)
    {
        if (entityState_.IsReady)
        {
            entityState_.TransitionPendingFirstReport();
        }
        else
        {
            entityState_.TransitionReady();
        }

        healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: changed state to {1}", activityId, entityState_.State);
    }

    if (changeHasSystemReport)
    {
        bool prevValue = attributes_->HasSystemReport;
        attributes_->HasSystemReport = !prevValue;
        healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: changed has system report to {1}", activityId, attributes_->HasSystemReport);
    }

    if (changeSystemErrorCount)
    {
        int prevValue = attributes_->SystemErrorCount;
        if (prevValue == 0)
        {
            // Set to a positive value instead
            attributes_->UpdateSystemErrorCount(3);
        }
        else
        {
            // Set to 0.
            attributes_->UpdateSystemErrorCount(-prevValue);
        }

        healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: changed system error count to {1}, previous {2}", activityId, attributes_->SystemErrorCount, prevValue);
    }

    // Delete events if present, fail otherwise
    if (!deleteEventKeys.empty())
    {
        // Do no modify the state before ensuring all keys to delete are present
        for (auto const & key : deleteEventKeys)
        {
            bool found = false;
            for (auto it = events_.begin(); it != events_.end(); ++it)
            {
                if (key.first == (*it)->SourceId && key.second == (*it)->Property)
                {
                    healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: deleting event {1}+{2}", activityId, key.first, key.second);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    entityIdString_,
                    "{0}: Test_CorruptEntity: there is no event with key {1} + {2}",
                    activityId,
                    key.first,
                    key.second);
                return false;
            }
        }

        // Delete the events
        for (auto const & key : deleteEventKeys)
        {
            events_.remove_if([&key](HealthEventStoreDataUPtr const & event)->bool
            {
                return event->SourceId == key.first && event->Property == key.second;
            });
        }
    }

    // Scramble events if present, fail otherwise
    if (!scrambleEventKeys.empty())
    {
        for (auto const & key : scrambleEventKeys)
        {
            auto const & keySource = get<0>(key);
            auto const & keyProperty = get<1>(key);
            FABRIC_HEALTH_STATE keyState = get<2>(key);

            bool found = false;
            for (auto it = events_.begin(); it != events_.end(); ++it)
            {
                if (keySource == (*it)->SourceId && keyProperty == (*it)->Property)
                {
                    healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: corrupt event {1}+{2}: previous health state {3}, new {4}", activityId, keySource, keyProperty, (*it)->State, keyState);
                    (*it)->State = keyState;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                healthManagerReplica_.WriteInfo(
                    TraceComponent,
                    entityIdString_,
                    "{0}: Test_CorruptEntity: there is no event with key {1} + {2}",
                    activityId,
                    keySource,
                    keyProperty);
                return false;
            }
        }
    }

    ReplicaActivityId replicaActivityId(this->PartitionedReplicaId, activityId);
    for (auto const & healthInfo : addReports)
    {
        // Create reports and add them in memory
        healthManagerReplica_.WriteInfo(TraceComponent, entityIdString_, "{0}: Test_CorruptEntity: adding event {1}+{2}", activityId, healthInfo.SourceId, healthInfo.Property);
        events_.push_back(this->GetStoreData(healthInfo, Priority::NotAssigned, replicaActivityId));
    }

    entityState = entityState_.State;
    eventCount = events_.size();
    return true;
}
