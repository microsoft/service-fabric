// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const TraceFTCache("FTCache");

class FailoverUnitHealthReportContext : public BackgroundThreadContext
{
    DENY_COPY(FailoverUnitHealthReportContext);

public:
    FailoverUnitHealthReportContext(FailoverManager & failoverManager, FABRIC_SEQUENCE_NUMBER ackedSequence, FABRIC_SEQUENCE_NUMBER initialSequence)
        :   BackgroundThreadContext(L"FailoverUnitHealthReportContext"),
            failoverManager_(failoverManager), 
            ackedSequence_(ackedSequence),
            initialSequence_(initialSequence)
    {
    }

    virtual BackgroundThreadContextUPtr CreateNewContext() const
    {
        return make_unique<FailoverUnitHealthReportContext>(failoverManager_, ackedSequence_, initialSequence_);
    }

    virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
    {
        UNREFERENCED_PARAMETER(fm);

        if (failoverUnit.HealthSequence >= ackedSequence_ &&
            failoverUnit.HealthSequence < initialSequence_)
        {
            failoverManager_.HealthClient->AddHealthReport(
                failoverManager_.HealthReportFactoryObj.GenerateFailoverUnitHealthReport(failoverUnit));
        }
    }

    virtual void Merge(BackgroundThreadContext const & context) { UNREFERENCED_PARAMETER(context); }

    virtual bool ReadyToComplete() { return true; }

    virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
    {
        UNREFERENCED_PARAMETER(isEnumerationAborted);

        if (isContextCompleted)
        {
            fm.FailoverUnitCacheObj.CompleteHealthInitialization();
        }
    }

private:
    FailoverManager & failoverManager_;
    FABRIC_SEQUENCE_NUMBER ackedSequence_;
    FABRIC_SEQUENCE_NUMBER initialSequence_;
};

    
FailoverUnitCache::Visitor::Visitor(FailoverUnitCache const& cache, 
                                    bool randomAccess,
                                    TimeSpan timeout,
                                    bool executeStateMachine)
    : cache_(cache), index_(-1), timeout_(timeout), executeStateMachine_(executeStateMachine)
{
    for (auto it = cache.failoverUnits_.begin(); it != cache.failoverUnits_.end(); it++)
    {
        shuffleTable_.push_back(it->first);
    }

    if (randomAccess)
    {
        random_shuffle(shuffleTable_.begin(), shuffleTable_.end());
    }
}

LockedFailoverUnitPtr FailoverUnitCache::Visitor::MoveNext()
{
    LockedFailoverUnitPtr failoverUnit;

    bool result = false;
    do
    {
        FailoverUnitId failoverUnitId;
        failoverUnit = MoveNext(result, failoverUnitId);
    } while (!result);

    return failoverUnit;
}

LockedFailoverUnitPtr FailoverUnitCache::Visitor::MoveNext(__out bool & result, FailoverUnitId & failoverUnitId)
{
    size_t index;
    while ((index = static_cast<size_t>(InterlockedIncrement(&index_))) < shuffleTable_.size())
    {
        failoverUnitId = shuffleTable_[index];

        LockedFailoverUnitPtr failoverUnit;
        if (cache_.TryGetLockedFailoverUnit(failoverUnitId, failoverUnit, timeout_, executeStateMachine_))
        {
            if (failoverUnit)
            {
                result = true;
                return failoverUnit;
            }
        }
        else
        {
            result = false;
            return LockedFailoverUnitPtr();
        }
    }

    // return an empty one
    result = true;
    return LockedFailoverUnitPtr();
}

/// <summary>
/// Reset the visitor to start from the beginning.
/// </summary>
void FailoverUnitCache::Visitor::Reset()
{
    InterlockedExchange(&index_, -1);
}

FailoverUnitCache::FailoverUnitCache(
    FailoverManager& fm,
    vector<FailoverUnitUPtr> & failoverUnits,
    int64 savedLookupVersion,
    Common::ComponentRoot const & root)
    : fm_(fm),
    fmStore_(fm.Store),
    plb_(fm.PLB),
    nodeCache_(fm.NodeCacheObj),
    serviceCache_(fm.ServiceCacheObj),
    loadCache_(fm.LoadCacheObj),
    serviceLookupTable_(fm, failoverUnits, savedLookupVersion, root),
    savedLookupVersion_(savedLookupVersion),
    healthSequence_(0),
    initialSequence_(0),
    ackedSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
    invalidateSequence_(0),
    healthInitialized_(false)
{
    int64 plbElapsedMilliseconds;
    for (size_t i = 0; i < failoverUnits.size(); i++)
    {
        if (failoverUnits[i]->HealthSequence >= healthSequence_)
        {
            healthSequence_ = failoverUnits[i]->HealthSequence + 1;
        }

        if (!failoverUnits[i]->IsOrphaned)
        {
            plb_.UpdateFailoverUnit(failoverUnits[i]->GetPLBFailoverUnitDescription(Stopwatch::Now()), plbElapsedMilliseconds);
        }

        FailoverUnitId failoverUnitId = failoverUnits[i]->Id;
        failoverUnits_.insert(make_pair(failoverUnitId, make_shared<FailoverUnitCacheEntry>(fm_, move(failoverUnits[i]))));
    }

    fm_.InBuildFailoverUnitCacheObj.InitializeHealthSequence(healthSequence_);

    // Currently we only need health report from FM, not FMM.
    if (!fm.IsMaster)
    {
        initialSequence_ = healthSequence_;

        fm.HealthClient->HealthPreInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_PARTITION, fm_.PreOpenFMServiceEpoch.DataLossVersion);

        fm.WriteInfo(TraceFTCache, "Loaded health sequence: {0}", healthSequence_);
    }
}

size_t FailoverUnitCache::get_Count() const
{
    AcquireReadLock grab(lock_);
    return failoverUnits_.size(); 
}

void FailoverUnitCache::InsertFailoverUnitInCache(FailoverUnitUPtr && failoverUnit)
{
    AcquireWriteLock grab(lock_);

    FailoverUnitId failoverUnitId = failoverUnit->Id;

    if (failoverUnits_.find(failoverUnitId) != failoverUnits_.end())
    {
        fm_.WriteError(TraceFTCache, failoverUnit->IdString,
            "Cannot insert FailoverUnit. A FailoverUnit with the same ID already exists: {0}", failoverUnitId);
        return;
    }

    auto failoverUnitCacheEntry = make_shared<FailoverUnitCacheEntry>(fm_, move(failoverUnit));
    auto result = failoverUnits_.insert(make_pair(failoverUnitId, failoverUnitCacheEntry));

    FailoverUnit & insertedFailoverUnit = *(result.first->second->FailoverUnit);

    // Update ServiceLookupTable
    serviceLookupTable_.Update(insertedFailoverUnit);

    // Update PLB FailoverUnit information
    if (insertedFailoverUnit.ReplicaCount == 0 && fm_.NodeCacheObj.IsClusterStable)
    {
        if (insertedFailoverUnit.IsStateful)
        {
            insertedFailoverUnit.ReplicaDifference = insertedFailoverUnit.GetInitialReplicaPlacementCount();
        }
        else if (insertedFailoverUnit.TargetReplicaSetSize != -1)
        {
            insertedFailoverUnit.ReplicaDifference = insertedFailoverUnit.TargetReplicaSetSize;
        }
    }
    else
    {
        insertedFailoverUnit.ReplicaDifference = 0;
    }

    int64 plbElapsedMilliseconds;
    plb_.UpdateFailoverUnit(insertedFailoverUnit.GetPLBFailoverUnitDescription(Stopwatch::Now()), plbElapsedMilliseconds);

    vector<StateMachineActionUPtr> noActions;
    fm_.FTEvents.FTUpdateBackground(failoverUnitId.Guid, *(failoverUnitCacheEntry->FailoverUnit), noActions, insertedFailoverUnit.ReplicaDifference, 0, plbElapsedMilliseconds);

    insertedFailoverUnit.ReplicaDifference = 0;   
}

FABRIC_SEQUENCE_NUMBER FailoverUnitCache::GetHealthSequence()
{
    return InterlockedIncrement64(&healthSequence_) - 1;
}

void FailoverUnitCache::PreUpdateFailoverUnit(
    LockedFailoverUnitPtr & failoverUnit,
    __out PersistenceState::Enum & persistenceState,
    __out bool & shouldUpdateLookupVersion,
    __out bool & shouldReportHealth)
{
    persistenceState = failoverUnit->PersistenceState;

    ASSERT_IF(
        !failoverUnit.IsUpdating && persistenceState != PersistenceState::NoChange,
        "FailoverUnit persistence state {0} but not updating: {1}", persistenceState, *failoverUnit);

    ASSERT_IF(
        persistenceState == PersistenceState::ToBeInserted, 
        "FailoverUnit insert should use InsertFailoverUnit API");

    //
    // Update LookupVersion
    //

    if (failoverUnit->IsChangingConfiguration &&
        failoverUnit.Old->CurrentConfigurationVersion < failoverUnit->CurrentConfigurationVersion)
    {
        for (auto it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; ++it)
        {
            if (it->IsInCurrentConfiguration)
            {
                if (it->IsAvailable && it->PreviousConfigurationRole == it->CurrentConfigurationRole)
                {
                    it->IsEndpointAvailable = true;
                }
            }
        }
    }

    shouldUpdateLookupVersion = failoverUnit.Old->IsPersistencePending;
    if (!shouldUpdateLookupVersion)
    {
        if (failoverUnit->IsStateful)
        {
            shouldUpdateLookupVersion =
                (failoverUnit->CurrentConfigurationEpoch == failoverUnit.Old->CurrentConfigurationEpoch &&
                    failoverUnit.Old->IsChangingConfiguration &&
                    !failoverUnit->IsChangingConfiguration) ||
                failoverUnit->IsToBeDeleted ||
                failoverUnit->IsQuorumLost();

            if (!shouldUpdateLookupVersion)
            {
                for (auto const& replica : failoverUnit->CurrentConfiguration.Replicas)
                {
                    auto old = failoverUnit.Old->GetReplica(replica->FederationNodeId);

                    if (old &&
                        ((old->IsInCurrentConfiguration && !old->IsAvailable && replica->IsAvailable) ||
                        (failoverUnit.Old->IsChangingConfiguration && !old->IsEndpointAvailable && replica->IsEndpointAvailable)))
                    {
                        shouldUpdateLookupVersion = true;
                        break;
                    }
                }
            }
        }
        else
        {
            for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; replica++)
            {
                auto old = failoverUnit.Old->GetReplica(replica->FederationNodeId);

                if ((!old && replica->IsAvailable) ||
                    (old && replica->IsAvailable != old->IsAvailable))
                {
                    shouldUpdateLookupVersion = true;
                    break;
                }
            }
        }
    }

    if (shouldUpdateLookupVersion)
    {
        serviceLookupTable_.UpdateLookupVersion(*failoverUnit);
    }

    //
    // Update PlacementStartTime
    //

    if (!failoverUnit.Old->IsPlacementNeeded && failoverUnit->IsPlacementNeeded)
    {
        failoverUnit->PlacementStartTime = DateTime::Now();
    }

    //
    // Update HealthSequence
    //

    shouldReportHealth = (failoverUnit->IsOrphaned && !failoverUnit.Old->IsOrphaned) || failoverUnit->UpdateHealthState();
    if (shouldReportHealth)
    {
        failoverUnit->HealthSequence = GetHealthSequence();
    }
}

ErrorCode FailoverUnitCache::PostUpdateFailoverUnit(
    LockedFailoverUnitPtr & failoverUnit,
    PersistenceState::Enum persistenceState,
    bool shouldUpdateLookupVersion,
    bool shouldReportHealth,
    ErrorCode error,
    __out int64 & plbDuration)
{
    if (shouldReportHealth)
    {
        ReportHealthAfterFailoverUnitUpdate(*failoverUnit.Current, error.IsSuccess());
    }

    if (error.IsSuccess())
    {
        if (persistenceState == PersistenceState::ToBeDeleted)
        {
            AcquireWriteLock grab(lock_);

            auto it = failoverUnits_.find(failoverUnit->Id);

            if (it != failoverUnits_.end())
            {
                it->second->IsDeleted = true;
                failoverUnits_.erase(it);
                serviceLookupTable_.RemoveEntry(*failoverUnit);
            }
            else
            {
                fm_.FTEvents.FTUpdateFailureBecauseAlreadyDeleted(failoverUnit->Id.Guid);
            }

            fm_.FTEvents.PartitionDeleted(failoverUnit->IdString, failoverUnit->CurrentConfigurationVersion);
        }
        else
        {
            if (shouldUpdateLookupVersion)
            {
                serviceLookupTable_.Update(*failoverUnit);
            }
        }

        UpdatePlacementAndLoadBalancer(failoverUnit, persistenceState, plbDuration);

        if (failoverUnit.Old->CurrentConfigurationVersion < failoverUnit->CurrentConfigurationVersion ||
            failoverUnit.Old->IsChangingConfiguration && !failoverUnit->IsChangingConfiguration)
        {
            wstring secondaryReplicas;
            for (auto replica : failoverUnit->CurrentConfiguration.Replicas)
            {
                if (replica->CurrentConfigurationRole == ReplicaRole::Secondary)
                {
                    secondaryReplicas.append(replica->FederationNodeId.ToString());
                    secondaryReplicas.append(L" ");
                }
            }

            if (failoverUnit->IsChangingConfiguration)
            {
                fm_.FTEvents.ReconfigurationStarted(
                    failoverUnit->IdString,
                    failoverUnit->CurrentConfigurationVersion,
                    failoverUnit->CurrentConfiguration.Primary->FederationNodeId.ToString(),
                    secondaryReplicas);
            }
            else
            {
                TimeSpan reconfigurationDuration = DateTime::Now() - failoverUnit->ReconfigurationStartTime;
                fm_.FailoverUnitCounters->FailoverUnitReconfigurationDurationBase.Increment();
                fm_.FailoverUnitCounters->FailoverUnitReconfigurationDuration.IncrementBy(static_cast<PerformanceCounterValue>(reconfigurationDuration.TotalMilliseconds()));

                fm_.FTEvents.ReconfigurationCompleted(
                    failoverUnit->IdString,
                    failoverUnit->CurrentConfigurationVersion,
                    failoverUnit->CurrentConfiguration.IsEmpty ? L"" : failoverUnit->CurrentConfiguration.Primary->FederationNodeId.ToString(),
                    secondaryReplicas);
            }
        }
        
        if (!failoverUnit->IsStateful && failoverUnit.Old->LookupVersion < failoverUnit->LookupVersion)
        {
            wstring replicas;
            for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; replica++)
            {
                replicas.append(replica->FederationNodeId.ToString());
                replicas.append(L" ");
            }

            fm_.FTEvents.ServiceLocationUpdated(
                failoverUnit->IdString,
                failoverUnit->LookupVersion,
                replicas);
        }

        if (failoverUnit.Old->IsPlacementNeeded && !failoverUnit->IsPlacementNeeded)
        {
            TimeSpan placementDuration = DateTime::Now() - failoverUnit->PlacementStartTime;
            fm_.FailoverUnitCounters->FailoverUnitPlacementDurationBase.Increment();
            fm_.FailoverUnitCounters->FailoverUnitPlacementDuration.IncrementBy(static_cast<PerformanceCounterValue>(placementDuration.TotalMilliseconds()));
        }

        failoverUnit.Submit();
    }
    else
    {
        fm_.FTEvents.FTUpdateFailureError(failoverUnit->Id.Guid, *failoverUnit, error);

        if (fm_.IsMaster || FailoverConfig::GetConfig().StoreConnectionString.size() == 0)
        {
            return error;
        }

        error = fmStore_.GetFailoverUnit(failoverUnit->Id, failoverUnit.Current);
        if (error.IsSuccess())
        {
            if (failoverUnit.Current->LookupVersion > failoverUnit.Old->LookupVersion)
            {
                serviceLookupTable_.Update(*failoverUnit);
            }

            failoverUnit->UpdatePointers(fm_, nodeCache_, serviceCache_);

            failoverUnit.Submit();

            error = ErrorCodeValue::FMStoreUpdateFailed;
        }
        else
        {
            if (persistenceState == PersistenceState::ToBeDeleted)
            {
                if (error.ReadValue() == ErrorCodeValue::FMFailoverUnitNotFound)
                {
                    AcquireWriteLock grab(lock_);

                    auto it = failoverUnits_.find(failoverUnit->Id);

                    if (it != failoverUnits_.end())
                    {
                        it->second->IsDeleted = true;
                        failoverUnits_.erase(it);
                        serviceLookupTable_.RemoveEntry(*failoverUnit);
                    }
                    else
                    {
                        fm_.FTEvents.FTUpdateFailureBecauseAlreadyDeleted(failoverUnit->Id.Guid);
                    }

                    fm_.FTEvents.PartitionDeleted(failoverUnit->IdString, failoverUnit->CurrentConfigurationVersion);
                }
            }
        }
    }

    return error;
}

ErrorCode FailoverUnitCache::UpdateFailoverUnit(LockedFailoverUnitPtr & failoverUnit)
{
    int64 plbDuration(0);
    if (failoverUnit->PersistenceState == PersistenceState::NoChange)
    {
        if (failoverUnit->ReplicaDifference != 0)
        {
            UpdatePlacementAndLoadBalancer(failoverUnit, failoverUnit->PersistenceState, plbDuration);
        }

        return ErrorCodeValue::Success;
    }
    
    PersistenceState::Enum persistenceState = PersistenceState::NoChange;
    bool shouldUpdateLooupVersion = false;
    bool shouldReportHealth = false;
    
    PreUpdateFailoverUnit(failoverUnit, persistenceState, shouldUpdateLooupVersion, shouldReportHealth);

    int64 commitDuration;
    ErrorCode error = fmStore_.UpdateData(*failoverUnit.Current, commitDuration);

    return PostUpdateFailoverUnit(failoverUnit, persistenceState, shouldUpdateLooupVersion, shouldReportHealth, error, plbDuration);
}

void FailoverUnitCache::UpdateFailoverUnitAsync(
    LockedFailoverUnitPtr && failoverUnit,
    vector<StateMachineActionUPtr> && actions,
    bool isBackground)
{
    TESTASSERT_IF(
        failoverUnit->PersistenceState == PersistenceState::NoChange,
        "PersistenceState is NoChange during FailoverUnit update: {0}", *failoverUnit);

    PersistenceState::Enum persistenceState = PersistenceState::NoChange;
    bool shouldUpdateLooupVersion = false;
    bool shouldReportHealth = false;

    PreUpdateFailoverUnit(failoverUnit, persistenceState, shouldUpdateLooupVersion, shouldReportHealth);

    FailoverUnitCommitJobItem * commitJobItem = new FailoverUnitCommitJobItem(move(failoverUnit), persistenceState, shouldUpdateLooupVersion, shouldReportHealth, move(actions), isBackground);

    fmStore_.BeginUpdateData(
        *(commitJobItem->GetFailoverUnit()),
        [this, commitJobItem, persistenceState, shouldUpdateLooupVersion, shouldReportHealth](AsyncOperationSPtr const& updateOperation) mutable
        {
            int64 commitDuration;
            ErrorCode error = fmStore_.EndUpdateData(*(commitJobItem->GetFailoverUnit().Current), updateOperation, commitDuration);

            FailoverUnitCommitJobItemUPtr commitJobItemUPtr(commitJobItem);
            commitJobItemUPtr->SetCommitError(error);
            commitJobItemUPtr->SetCommitDuration(commitDuration);
            fm_.CommitQueue.Enqueue(move(commitJobItemUPtr));
        },
        fm_.CreateAsyncOperationRoot());
}

void FailoverUnitCache::UpdatePlacementAndLoadBalancer(LockedFailoverUnitPtr & failoverUnit, PersistenceState::Enum persistenceState, int64 & plbDuration) const
{
    if (failoverUnit.Current->IsOrphaned && !failoverUnit.Old->IsOrphaned)
    {
        plb_.DeleteFailoverUnit(wstring(failoverUnit->ServiceName), failoverUnit->FailoverUnitDescription.FailoverUnitId.Guid, plbDuration);
        loadCache_.DeleteLoads(failoverUnit->FailoverUnitDescription.FailoverUnitId);
    }
    else if (persistenceState != PersistenceState::ToBeDeleted && !failoverUnit.Current->IsOrphaned)
    {
        if (!failoverUnit->IsPersistencePending)
        {            
            plb_.UpdateFailoverUnit(failoverUnit->GetPLBFailoverUnitDescription(failoverUnit->ProcessingStartTime), plbDuration);
        }

        failoverUnit->ReplicaDifference = 0;
    }
}

/// <summary>
/// Create an enumerator for visiting the all failoverUnits.
/// Note that we are not using standard iterator because
/// the FailoverUnit to be visited may be erased in another thread.
/// </summary>
/// <param name="randomAccess">Whether the failoverUnits be visited randomly.</param>
/// <returns>The visitor for enumerating all failoverUnits.</returns>
FailoverUnitCache::VisitorSPtr FailoverUnitCache::CreateVisitor(bool randomAccess) const
{
    return CreateVisitor(randomAccess, FailoverConfig::GetConfig().LockAcquireTimeout);
}

FailoverUnitCache::VisitorSPtr FailoverUnitCache::CreateVisitor(bool randomAccess, TimeSpan timeout, bool executeStateMachine) const
{
    AcquireReadLock grab(lock_);
    return make_shared<Visitor>(*this, randomAccess, timeout, executeStateMachine);
}

bool FailoverUnitCache::TryProcessTaskAsync(FailoverUnitId failoverUnitId, DynamicStateMachineTaskUPtr & task, Federation::NodeInstance const & from, bool const isFromPLB) const
{
    FailoverUnitCacheEntrySPtr entry;

    {
        AcquireReadLock grab(lock_);

        auto it = failoverUnits_.find(failoverUnitId);
        if (it == failoverUnits_.end())
        {
            return false;
        }

        entry = it->second;
    }

    entry->ProcessTaskAsync(move(task), from, isFromPLB);

    return true;
}

bool FailoverUnitCache::TryGetLockedFailoverUnit(
    FailoverUnitId const& failoverUnitId,
    __out LockedFailoverUnitPtr & failoverUnit) const
{
    return TryGetLockedFailoverUnit(failoverUnitId, failoverUnit, FailoverConfig::GetConfig().LockAcquireTimeout, false);
}

bool FailoverUnitCache::TryGetLockedFailoverUnit(
    FailoverUnitId const& failoverUnitId,
    __out LockedFailoverUnitPtr & failoverUnit,
    TimeSpan timeout,
    bool executeStateMachine) const
{
    FailoverUnitCacheEntrySPtr entry;

    {
        AcquireReadLock grab(lock_);

        auto it = failoverUnits_.find(failoverUnitId);
        if (it == failoverUnits_.end())
        {
            return true;
        }

        entry = it->second;
    }

    bool isDeleted;
    if (entry->Lock(timeout, executeStateMachine, isDeleted))
    {
        if (!isDeleted)
        {
            failoverUnit = LockedFailoverUnitPtr(entry);
        }

        return true;
    }

    fm_.FTEvents.FTLockFailure(failoverUnitId.Guid);
    return false;
}

bool FailoverUnitCache::IsFailoverUnitValid(FailoverUnitId const& failoverUnitId) const
{
    AcquireReadLock grab(lock_);

    auto it = failoverUnits_.find(failoverUnitId);
    return (it != failoverUnits_.end() && !it->second->FailoverUnit->IsToBeDeleted);
}

bool FailoverUnitCache::FailoverUnitExists(FailoverUnitId const& failoverUnitId) const
{
    AcquireReadLock grab(lock_);
    auto it = failoverUnits_.find(failoverUnitId);
    return (it != failoverUnits_.end());
}

bool FailoverUnitCache::IsSafeToRemove(FailoverUnit const& failoverUnit) const
{
    bool result = false;

    int64 highestLookupVersion = serviceLookupTable_.EndVersion - 1;

    if (failoverUnit.LookupVersion < highestLookupVersion)
    {
        result = true;
    }
    else
    {
        ASSERT_IFNOT(
            failoverUnit.LookupVersion == highestLookupVersion,
            "LookupVersion = '{0}', EndLookupVersion = '{1}'.",
            failoverUnit.LookupVersion, serviceLookupTable_.EndVersion);

        ASSERT_IFNOT(
            highestLookupVersion >= savedLookupVersion_,
            "EndLookupVersion = '{0}', SavedLookupVersion = '{1}'.",
            serviceLookupTable_.EndVersion, savedLookupVersion_);

        result = (highestLookupVersion == savedLookupVersion_);
    }

    return result;
}

void FailoverUnitCache::SaveLookupVersion(int64 lookupVersion)
{
    AcquireWriteLock grab(lock_);

    if (savedLookupVersion_ < lookupVersion)
    {
        ErrorCode error = this->fmStore_.UpdateKeyValue(Constants::LookupVersionKey, StringUtility::ToWString(lookupVersion));

        if (error.IsSuccess())
        {
            savedLookupVersion_ = lookupVersion;

            fm_.WriteInfo(
                TraceFTCache,
                "Saved lookup version updated to {0}", savedLookupVersion_);
        }
        else
        {
            fm_.WriteError(
                TraceFTCache,
                "Unable to write service lookup version to store. Error = '{0}'.",
                error.ReadValue());
        }
    }
}

ErrorCode FailoverUnitCache::GetConsistencyUnitDescriptions(
    wstring const& serviceName,
    vector<ConsistencyUnitDescription> & consistencyUnitDescriptions) const
{
    return fm_.ServiceCacheObj.IterateOverFailoverUnits(
        serviceName,
        [&consistencyUnitDescriptions](LockedFailoverUnitPtr & failoverUnit) -> ErrorCode
        {
            if (!failoverUnit->IsOrphaned)
            {
                consistencyUnitDescriptions.push_back(failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription);
            }

            return ErrorCodeValue::Success;
        });
}

void FailoverUnitCache::AddThreadContexts()
{
    AcquireWriteLock grab(lock_);

    if (fm_.IsMaster)
    {
        return;
    }

    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::AddThreadContexts);
    auto startTime = Stopwatch::Now();
    bool addedHealthReportContext = false;

    bool isInitialProgress = (ackedSequence_ == FABRIC_INVALID_SEQUENCE_NUMBER);
    FABRIC_SEQUENCE_NUMBER progress;
    ErrorCode error = fm_.HealthClient->HealthGetProgress(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_PARTITION, progress);
    if (error.IsSuccess() && ackedSequence_ != progress)
    {
        ackedSequence_ = progress;
        fm_.WriteInfo(TraceFTCache, "HealthGetProgress: {0}", progress);
    }

    if (isInitialProgress && error.IsSuccess() && progress > initialSequence_)
    {
        fm_.WriteWarning(TraceFTCache, "Progress retrieved {0} is larger than FM knows {1}",
            progress, initialSequence_);

        invalidateSequence_ = initialSequence_ = progress;

        FABRIC_SEQUENCE_NUMBER currentSequence = healthSequence_;
        while (currentSequence < progress)
        {
            FABRIC_SEQUENCE_NUMBER last = InterlockedCompareExchange64(&healthSequence_, progress, currentSequence);
            if (last == currentSequence)
            {
                fm_.WriteWarning(TraceFTCache, "Updating sequence from {0} to {1}",
                    currentSequence, progress);
                break;
            }
            else
            {
                currentSequence = last;
            }
        }
    }

    if (!healthInitialized_ && ackedSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        if (ackedSequence_ == initialSequence_)
        {
            InternalCompleteHealthInitialization();
        }
        else
        {
            fm_.BackgroundManagerObj.AddThreadContext(make_unique<FailoverUnitHealthReportContext>(fm_, ackedSequence_, initialSequence_));
            addedHealthReportContext = true;
        }
    }

    auto duration = Stopwatch::Now() - startTime;
    fm_.Events.FailoverUnitContextsPeriodicTaskEnd(fm_.Id, duration, addedHealthReportContext);
}

void FailoverUnitCache::ReportHealthAfterFailoverUnitUpdate(FailoverUnit const & failoverUnit, bool updated)
{
    if (updated)
    {
        fm_.HealthClient->AddHealthReport(
            fm_.HealthReportFactoryObj.GenerateFailoverUnitHealthReport(
                failoverUnit));
    }
    else if (!fm_.IsMaster)
    {
        fm_.HealthClient->HealthSkipSequence(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_PARTITION, failoverUnit.HealthSequence);
    }
}

void FailoverUnitCache::CompleteHealthInitialization()
{
    AcquireWriteLock grab(lock_);
    InternalCompleteHealthInitialization();
}

void FailoverUnitCache::InternalCompleteHealthInitialization()
{
    FABRIC_SEQUENCE_NUMBER invalidateSequence = (invalidateSequence_ == 0 ? FABRIC_INVALID_SEQUENCE_NUMBER : invalidateSequence_);
    fm_.HealthClient->HealthPostInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_PARTITION, initialSequence_, invalidateSequence);
    healthInitialized_ = true;

    fm_.WriteInfo(TraceFTCache, "HealthPostInitialize {0}/{1}", initialSequence_, invalidateSequence);
}
