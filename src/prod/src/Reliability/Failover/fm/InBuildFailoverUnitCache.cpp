// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;
using namespace std;

StringLiteral const TraceRebuild("Rebuild");

InBuildFailoverUnitCache::InBuildFailoverUnitCache(
    FailoverManager& fm,
    std::vector<InBuildFailoverUnitUPtr> && inBuildFailoverUnits,
    ComponentRoot const& root)
    : RootedObject(root),
      fm_(fm),
      fmStore_(fm.Store),
      inBuildFailoverUnits_(),
      anyReplicaFound_(false),
      isRebuildComplete_(true),
      fullRebuildTimer_(),
      lock_()
{
    for (size_t i = 0; i < inBuildFailoverUnits.size(); ++i)
    {
        UpdateServiceInstance(inBuildFailoverUnits[i]->Description.Name, inBuildFailoverUnits[i]->Description.Instance);
        fm_.WriteInfo(TraceRebuild, wformatString(inBuildFailoverUnits[i]->Id), "Loaded InBuildFT {0}", *inBuildFailoverUnits[i]);
        inBuildFailoverUnits_.insert(make_pair(inBuildFailoverUnits[i]->Id, move(inBuildFailoverUnits[i])));
    }

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        it->second->IsToBeDeleted = (it->second->Description.Instance < serviceInstances_[it->second->Description.Name]);
    }
}

void InBuildFailoverUnitCache::InitializeHealthSequence(__inout FABRIC_SEQUENCE_NUMBER & healthSequence) const
{
    AcquireReadLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        if (it->second->HealthSequence >= healthSequence)
        {
            healthSequence = it->second->HealthSequence + 1;
        }
    }
}

size_t InBuildFailoverUnitCache::get_Count() const
{
    AcquireReadLock grab(lock_);
    return inBuildFailoverUnits_.size(); 
}

void InBuildFailoverUnitCache::GetInBuildFailoverUnits(std::vector<InBuildFailoverUnitUPtr> & result) const
{
    AcquireReadLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        result.push_back(make_unique<InBuildFailoverUnit>(*(it->second)));
    }
}

bool InBuildFailoverUnitCache::UpdateServiceInstance(std::wstring const & name, uint64 instance)
{
    auto it = serviceInstances_.find(name);
    if (it == serviceInstances_.end() || it->second < instance)
    {
        serviceInstances_[name] = instance;
        return true;
    }

    return false;
}

void InBuildFailoverUnitCache::OnStartRebuild()
{
    AcquireExclusiveLock grab(lock_);

    isRebuildComplete_ = false;
}

ErrorCode InBuildFailoverUnitCache::CreateServiceIfNotPresent(
    FailoverUnitId const & failoverUnitId,
    InBuildFailoverUnitUPtr const& inBuildFailoverUnitUPtr,
    ServiceDescription const& serviceDescription)
{
    ServiceInfoSPtr serviceInfo = fm_.ServiceCacheObj.GetService(serviceDescription.Name);
    if (serviceInfo)
    {
        if (serviceInfo->Instance != serviceDescription.Instance)
        {
            fm_.WriteError(TraceRebuild, wformatString(failoverUnitId),
                "InBuild Failoverunit of {0} does not match existing service {1}",
                serviceDescription, *serviceInfo);
            if (inBuildFailoverUnitUPtr)
            {
                inBuildFailoverUnitUPtr->IsToBeDeleted = true;
            }
            return ErrorCodeValue::StaleRequest;
        }

        if (serviceInfo->ServiceDescription.UpdateVersion < serviceDescription.UpdateVersion)
        {
            ServiceDescription targetServiceDescription = serviceDescription;
            ErrorCode error = fm_.ServiceCacheObj.UpdateService(move(targetServiceDescription));
            if (!error.IsSuccess())
            {
                fm_.WriteWarning(
                    TraceRebuild, wformatString(failoverUnitId),
                    "UpdateService for service failed with error {0}: {1}", 
                    error, serviceDescription);

                return error;
            }
        }

		ManualResetEvent completedEvent(false);
		AsyncOperationSPtr operation = fm_.ServiceCacheObj.BeginAddFailoverUnitId(
			serviceDescription.Name,
			failoverUnitId,
			[&completedEvent](AsyncOperationSPtr const&) { completedEvent.Set(); },
			fm_.CreateAsyncOperationRoot());
		completedEvent.WaitOne();
		ErrorCode error = fm_.ServiceCacheObj.EndAddFailoverUnitId(operation);
        if (!error.IsSuccess())
        {
            fm_.WriteWarning(
                TraceRebuild, wformatString(failoverUnitId),
                "AddFailoverUnitId for service failed with error {0}: {1}",
                error, serviceDescription);

            return error;
        }
    }
    else
    {
        vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
        consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(failoverUnitId.Guid));

        bool isServiceUpdateNeeded = (inBuildFailoverUnitUPtr ? inBuildFailoverUnitUPtr->IsServiceUpdateNeeded() : false);

        auto description = serviceDescription;
        ErrorCode error = fm_.ServiceCacheObj.CreateServiceDuringRebuild(move(description), move(consistencyUnitDescriptions), isServiceUpdateNeeded);
        if (!error.IsSuccess())
        {
            fm_.WriteWarning(
                TraceRebuild, wformatString(failoverUnitId),
                "CreateServiceDuringRebuild for service {0} failed with error {1}", 
                serviceDescription, 
                error);

            return error;
        }
        else
        {
            fm_.WriteInfo(
                TraceRebuild, wformatString(failoverUnitId),
                "Service {0} created during rebuild",
                serviceDescription);
        }
    }

    return ErrorCode::Success();
}

ErrorCode InBuildFailoverUnitCache::OnRebuildComplete()
{
    vector<StateMachineActionUPtr> actions;
    return ProcessFailoverUnits(false, actions);
}

ErrorCode InBuildFailoverUnitCache::ProcessFailoverUnits(bool shouldDropOfflineReplicas, vector<StateMachineActionUPtr> & actions)
{
    FailoverConfig const & config = FailoverConfig::GetConfig();

    AcquireExclusiveLock grab(lock_);

    isRebuildComplete_ = true;

    serviceInstances_.clear();
    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        UpdateServiceInstance(it->second->Description.Name, it->second->Description.Instance);
    }

    ErrorCode result(ErrorCodeValue::Success);
    vector<FailoverUnitId> inBuildFailoverUnitsToDelete;

    for (auto iterator = inBuildFailoverUnits_.begin(); iterator != inBuildFailoverUnits_.end(); iterator++)
    {
        InBuildFailoverUnitUPtr const& inBuildFailoverUnit = iterator->second;

        bool dropOfflineReplicas = false;
        if (!fm_.IsMaster &&
            shouldDropOfflineReplicas &&
            DateTime::Now() - max(inBuildFailoverUnit->LastUpdated, fm_.ReadyTime) >= inBuildFailoverUnit->Description.QuorumLossWaitDuration)
        {
            dropOfflineReplicas = true;
        }

        FailoverUnitUPtr failoverUnit = inBuildFailoverUnit->Generate(fm_.NodeCacheObj, dropOfflineReplicas);
        if (failoverUnit)
        {
            // Update the lookup version.
            fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

            // Generation is complete so we need to persist the new FailoverUnit and delete the InBuildFT in one transaction
            ErrorCode e = PersistGeneratedFailoverUnit(move(failoverUnit), inBuildFailoverUnit, inBuildFailoverUnit->Description);
            if (e.IsSuccess())
            {
                inBuildFailoverUnitsToDelete.push_back(inBuildFailoverUnit->Id);
            }
            else
            {
                result = ErrorCode::FirstError(result, e);
            }
        }
        else
        {
            fm_.FTEvents.FTInBuildQuorumLoss(wformatString(inBuildFailoverUnit->Id));

            if (fm_.IsMaster && !fullRebuildTimer_)
            {
                SetFullRebuildTimer();
            }

            if (!inBuildFailoverUnit->IsHealthReported &&
                DateTime::Now() - inBuildFailoverUnit->CreatedTime > config.RebuildPartitionTimeLimit &&
                (fm_.IsMaster || fm_.FailoverUnitCacheObj.IsHealthInitialized) &&
                (!fm_.IsMaster || fm_.BackgroundManagerObj.IsStateTraceEnabled))
            {
                auto newInBuildFailoverUnit = make_unique<InBuildFailoverUnit>(*inBuildFailoverUnit);
                newInBuildFailoverUnit->PersistenceState = PersistenceState::ToBeUpdated;

                if (!fm_.IsMaster)
                {
                    newInBuildFailoverUnit->HealthSequence = fm_.FailoverUnitCacheObj.GetHealthSequence();
                    newInBuildFailoverUnit->IsHealthReported = true;
                }

                int64 commitDuration;
                ErrorCode error = fmStore_.UpdateData(*newInBuildFailoverUnit, commitDuration);

                ReportHealth(*newInBuildFailoverUnit, false, error);

                if (error.IsSuccess())
                {
                    fm_.WriteInfo(
                        TraceRebuild, wformatString(newInBuildFailoverUnit->Id),
                        "InBuildFailoverUnit updated: {0}\r\nCommit Duration = {1} ms", *newInBuildFailoverUnit, commitDuration);

                    inBuildFailoverUnits_[newInBuildFailoverUnit->Id] = move(newInBuildFailoverUnit);
                }
                else
                {
                    fm_.WriteInfo(
                        TraceRebuild, wformatString(newInBuildFailoverUnit->Id),
                        "InBuildFailoverUnit update failed with error {0}: {1}\r\nCommit Duration = {2} ms",
                        error, *newInBuildFailoverUnit, commitDuration);
                }
            }

            if (!fm_.IsMaster)
            {
                if (inBuildFailoverUnit->IsToBeDeleted ||
                    inBuildFailoverUnit->CreatedTime + config.InBuildFailoverUnitKeepDuration < DateTime::Now())
                {
                    bool canDelete = (inBuildFailoverUnit->CreatedTime + config.InBuildFailoverUnitKeepDuration + config.OfflineReplicaKeepDuration < DateTime::Now());
                    if (!canDelete)
                    {
                        canDelete = inBuildFailoverUnit->GenerateDeleteReplicaActions(fm_.NodeCacheObj, actions);
                    }

                    if (canDelete)
                    {
                        auto newInBuildFailoverUnit = make_unique<InBuildFailoverUnit>(*inBuildFailoverUnit);
                        newInBuildFailoverUnit->PersistenceState = PersistenceState::ToBeDeleted;

                        newInBuildFailoverUnit->HealthSequence = fm_.FailoverUnitCacheObj.GetHealthSequence();

                        int64 commitDuration;
                        ErrorCode e = fmStore_.UpdateData(*newInBuildFailoverUnit, commitDuration);
                        
                        ReportHealth(*newInBuildFailoverUnit, true, e);

                        if (e.IsSuccess())
                        {
                            inBuildFailoverUnitsToDelete.push_back(inBuildFailoverUnit->Id);
                            fm_.WriteInfo(TraceRebuild, wformatString(inBuildFailoverUnit->Id),
                                "Deleting in-build failover unit {0}\r\nCommit Duration = {1} ms",
                                *inBuildFailoverUnit, commitDuration);
                        }
                        else
                        {
                            result = ErrorCode::FirstError(result, e);
                        }
                    }
                }
            }
        }
    }

    if (fm_.IsMaster && !fullRebuildTimer_ && inBuildFailoverUnits_.empty() && anyReplicaFound_ &&
        !fm_.FailoverUnitCacheObj.FailoverUnitExists(FailoverUnitId(Constants::FMServiceGuid)))
    {
        SetFullRebuildTimer();
    }

    for (FailoverUnitId const& failoverUnitId : inBuildFailoverUnitsToDelete)
    {
        inBuildFailoverUnits_.erase(failoverUnitId);
    }

    // If we failed to persist even one InBuildFT this will be false and the higher layer will retry 
    return result;
}

void InBuildFailoverUnitCache::SetFullRebuildTimer()
{
    if (FailoverConfig::GetConfig().FullRebuildWaitDuration != TimeSpan::MaxValue)
    {
        auto root = Root.CreateComponentRoot();
        fullRebuildTimer_ = Timer::Create(TimerTagDefault, [this, root] (TimerSPtr const&) { RecoverFMServicePartition(); });
        fullRebuildTimer_->Change(FailoverConfig::GetConfig().FullRebuildWaitDuration);
    }
}

bool InBuildFailoverUnitCache::RecoverFMServicePartition()
{
    ASSERT_IFNOT(fm_.IsMaster, "Only FMM should create the FMService FailoverUnit.");

    bool result = false;

    AcquireExclusiveLock grab(lock_);

    if ((!inBuildFailoverUnits_.empty() || anyReplicaFound_) &&
        !fm_.FailoverUnitCacheObj.FailoverUnitExists(FailoverUnitId(Constants::FMServiceGuid)))
    {
        // Get the FM ServiceDescription
        ServiceDescription serviceDescription;
        if (!inBuildFailoverUnits_.empty())
        {
            serviceDescription = inBuildFailoverUnits_[FailoverUnitId(Constants::FMServiceGuid)]->Description;
        }
        else
        {
            serviceDescription = fm_.ServiceCacheObj.GetFMServiceDescription();
        }

        // Check if ServiceDescription update is needed
        bool isServiceUpdateNeeded = false;
        if (!inBuildFailoverUnits_.empty())
        {
            isServiceUpdateNeeded = inBuildFailoverUnits_[FailoverUnitId(Constants::FMServiceGuid)]->IsServiceUpdateNeeded();
        }

        vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
        consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(Constants::FMServiceGuid));

        // Create the FMService
        ErrorCode error = fm_.ServiceCacheObj.CreateServiceDuringRebuild(move(serviceDescription), move(consistencyUnitDescriptions), isServiceUpdateNeeded);

        TESTASSERT_IF(!error.IsSuccess() && fm_.IsActive, "Failed to create FM service during full rebuild: {0}", error);

        // Get the newly created ServiceInfo for the FMService
        auto serviceInfo = fm_.ServiceCacheObj.GetService(ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName);

        // Create an empty FailoverUnit for the FMService.
        FailoverUnitUPtr failoverUnit = make_unique<FailoverUnit>(
            FailoverUnitId(Constants::FMServiceGuid),
            ConsistencyUnitDescription(ConsistencyUnitId(Constants::FMServiceGuid)),
            serviceInfo->ServiceDescription.IsStateful,
            serviceInfo->ServiceDescription.HasPersistedState,
            serviceInfo->Name,
            serviceInfo);

        if (!inBuildFailoverUnits_.empty())
        {
            // Update epoch
            failoverUnit->CurrentConfigurationEpoch = inBuildFailoverUnits_[FailoverUnitId(Constants::FMServiceGuid)]->MaxEpoch;
            failoverUnit->UpdateEpochForDataLoss();
        }

        // Set the PersistenceState
        failoverUnit->PersistenceState = PersistenceState::NoChange;

        // Update the lookup version
        fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

        // Clear the InBuildFailoverUnitCache
        inBuildFailoverUnits_.clear();

        fm_.FTEvents.FTUpdateInBuildQuorumLossRebuild(failoverUnit->Id.Guid);

        // Update pointers
        failoverUnit->UpdatePointers(fm_, fm_.NodeCacheObj, fm_.ServiceCacheObj);

        // Insert the FailoverUnit into the FailoverUnitCache
        fm_.FailoverUnitCacheObj.InsertFailoverUnitInCache(move(failoverUnit));

        result = true;
    }

    return result;
}

void InBuildFailoverUnitCache::CheckQuorumLoss()
{
    if (isRebuildComplete_)
    {
        vector<StateMachineActionUPtr> actions;
        ErrorCode error = ProcessFailoverUnits(true, actions);
        if (!error.IsSuccess())
        {
            fm_.WriteInfo(
                TraceRebuild,
                "CheckQuorumLoss failed with '{0}'.", error);
        }

        for (StateMachineActionUPtr const & action : actions)
        {
            action->PerformAction(fm_);
        }
    }
}

ErrorCode InBuildFailoverUnitCache::AddFailoverUnitReport(FailoverUnitInfo const & report, Federation::NodeInstance const & nodeInstance)
{
    AcquireExclusiveLock grab(lock_);

    if (UpdateServiceInstance(report.ServiceDescription.Name, report.ServiceDescription.Instance))
    {
        for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
        {
            if (it->second->Description.Name == report.ServiceDescription.Name)
            {
                it->second->IsToBeDeleted = true;
            }
        }
    }

    auto it = inBuildFailoverUnits_.find(report.FailoverUnitDescription.FailoverUnitId);
    bool inBuildFailoverUnitExists = (it != inBuildFailoverUnits_.end());
    bool failoverUnitExists = fm_.FailoverUnitCacheObj.FailoverUnitExists(report.FailoverUnitDescription.FailoverUnitId);

    fm_.WriteInfo(TraceRebuild, wformatString(report.FailoverUnitDescription.FailoverUnitId),
        "Processing report from {0}: {1}, inBuildFailoverUnitExists={2}, failoverUnitExists={3}",
        nodeInstance, report, inBuildFailoverUnitExists, failoverUnitExists);

    InBuildFailoverUnitUPtr inBuildFailoverUnit = nullptr;

    if (failoverUnitExists)
    {
        // Both FailoverUnit and InBuildFT cannot exist at the same time
        ASSERT_IF(inBuildFailoverUnitExists, 
            "Both InBuildFT and FailoverUnit exist {0}",
            report.FailoverUnitDescription);

        // FailoverUnit has already been generated as part of rebuild. Ignore message
        return ErrorCodeValue::FMFailoverUnitAlreadyExists;
    }
    
    if (inBuildFailoverUnitExists)
    {
        inBuildFailoverUnit = make_unique<InBuildFailoverUnit>(*it->second);
    }
    else
    {
        ServiceInfoSPtr service = fm_.ServiceCacheObj.GetService(report.ServiceDescription.Name);
        if (service && (service->IsToBeDeleted || service->IsDeleted))
        {
            return ErrorCodeValue::FMServiceDeleteInProgress;
        }

        // InbuildFailoverUnit and FailoverUnit does not exist so we need to create a new one
        inBuildFailoverUnit = make_unique<InBuildFailoverUnit>(
            report.FailoverUnitDescription.FailoverUnitId, 
            report.FailoverUnitDescription.ConsistencyUnitDescription,
            report.ServiceDescription);
        inBuildFailoverUnit->IsToBeDeleted = (inBuildFailoverUnit->Description.Instance < serviceInstances_[inBuildFailoverUnit->Description.Name]);
    }

    bool result = inBuildFailoverUnit->Add(report, nodeInstance, fm_);

    if (!result)
    {
        fm_.WriteInfo(TraceRebuild, wformatString(report.FailoverUnitDescription.FailoverUnitId),
            "Ignoring report from {0}.", nodeInstance);

        return ErrorCodeValue::Success;
    }

    if (isRebuildComplete_)
    {
        FailoverUnitUPtr failoverUnit = inBuildFailoverUnit->Generate(fm_.NodeCacheObj, false);
        if (failoverUnit)
        {
            // Update the lookup version.
            fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

            ErrorCode e;
            if (inBuildFailoverUnitExists)
            {
                // Generation is complete so we need to persist the new FailoverUnit and delete the InBuildFT in one transaction
                e = PersistGeneratedFailoverUnit(move(failoverUnit), inBuildFailoverUnit, report.ServiceDescription);
            }
            else
            {
                // If InBuildFailoverUnit did not exist before this report so we don't need to delete it
                e = PersistGeneratedFailoverUnit(move(failoverUnit), nullptr, report.ServiceDescription);
            }

            // If persistence is successful, remove the InBuildFailoverUnit from the cache.
            if (e.IsSuccess() && inBuildFailoverUnitExists)
            {
                inBuildFailoverUnits_.erase(inBuildFailoverUnit->Id);
            }

            return e;
        }
        else
        {
            fm_.FTEvents.FTInBuildQuorumLoss(wformatString(inBuildFailoverUnit->Id));
        }
    }

    inBuildFailoverUnit->PersistenceState = (inBuildFailoverUnitExists ? PersistenceState::ToBeUpdated : PersistenceState::ToBeInserted);

    // FailoverUnit generation or Rebuild is not complete so we just need to update the InBuildFailoverUnit
    int64 commitDuration;
    ErrorCode error = fmStore_.UpdateData(*inBuildFailoverUnit, commitDuration);
    if (error.IsSuccess())
    {
        fm_.WriteInfo(
            TraceRebuild, wformatString(report.FailoverUnitDescription.FailoverUnitId),
            "InBuildFailoverUnit updated: {0}\r\nCommit Duration = {1} ms", *inBuildFailoverUnit, commitDuration);

        inBuildFailoverUnits_[inBuildFailoverUnit->Id] = move(inBuildFailoverUnit);
    }
    else
    {
        fm_.WriteInfo(
            TraceRebuild, wformatString(inBuildFailoverUnit->Id),
            "InBuildFailoverUnit update failed with error {0}: {1}\r\nCommit Duration = {2} ms", 
            error, *inBuildFailoverUnit, commitDuration);
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::PersistGeneratedFailoverUnit(
    FailoverUnitUPtr && failoverUnit,
    InBuildFailoverUnitUPtr const& inBuildFailoverUnitUPtr,
    ServiceDescription const& serviceDescription)
{
    ErrorCode error = CreateServiceIfNotPresent(failoverUnit->Id, inBuildFailoverUnitUPtr, serviceDescription);
    if (!error.IsSuccess())
    {
        return error;
    }

    ErrorCode result(ErrorCodeValue::Success);
    for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; ++replica)
    {
        NodeInfoSPtr node = fm_.NodeCacheObj.GetNode(replica->FederationNodeId);
        if (!node || node->NodeInstance.InstanceId < replica->FederationNodeInstance.InstanceId)
        {
            ErrorCode e = fm_.NodeCacheObj.NodeDown(replica->FederationNodeInstance);
            result = ErrorCode::FirstError(result, e);
        }
    }

    if (!result.IsSuccess())
    {
        return result;
    }

    // If the service is marked as ToBeDeleted, mark the generated FailoverUnit as ToBeDeleted.
    ServiceInfoSPtr serviceInfo = fm_.ServiceCacheObj.GetService(failoverUnit->ServiceName);
    if (serviceInfo->IsToBeDeleted)
    {
        failoverUnit->SetToBeDeleted();
    }

    result = fmStore_.DeleteInBuildFailoverUnitAndUpdateFailoverUnit(inBuildFailoverUnitUPtr, failoverUnit);
    if (!result.IsSuccess())
    {
        fm_.WriteWarning(
            Constants::FTUpdateFailureSource, failoverUnit->IdString,
            "Failed to delete InBuildFT {0} with error {1}",
            failoverUnit->IdString, result);
    }
    else
    {
        failoverUnit->UpdatePointers(fm_, fm_.NodeCacheObj, fm_.ServiceCacheObj);

        fm_.FTEvents.FTUpdateRebuildComplete(failoverUnit->Id.Guid, *failoverUnit);

        if (inBuildFailoverUnitUPtr)
        {
            fm_.WriteInfo(
                TraceRebuild, failoverUnit->IdString,
                "Rebuild from InBuildFT {0}",
                *inBuildFailoverUnitUPtr);
        }

        fm_.FailoverUnitCacheObj.InsertFailoverUnitInCache(move(failoverUnit));
    }

    return result;
}

bool InBuildFailoverUnitCache::InBuildFailoverUnitExists(FailoverUnitId const& failoverUnitId) const
{
    AcquireReadLock grab(lock_);

    return inBuildFailoverUnits_.find(failoverUnitId) != inBuildFailoverUnits_.end();
}

bool InBuildFailoverUnitCache::InBuildFailoverUnitExistsForService(std::wstring const & serviceName) const
{
    AcquireReadLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        if (it->second->Description.Name == serviceName)
        {
            return true;
        }
    }

    return false;
}

ErrorCode InBuildFailoverUnitCache::OnReplicaDropped(FailoverUnitId const& failoverUnitId, NodeId const& nodeId, bool isDeleted)
{
    ErrorCode error = ErrorCode::Success();

    AcquireWriteLock grab(lock_);

    auto it = inBuildFailoverUnits_.find(failoverUnitId);

    if (it == inBuildFailoverUnits_.end())
    {
        return ErrorCodeValue::FMFailoverUnitNotFound;
    }

    InBuildFailoverUnitUPtr inBuildFailoverUnit = make_unique<InBuildFailoverUnit>(*it->second);
    
    bool isUpdated = inBuildFailoverUnit->OnReplicaDropped(nodeId, isDeleted);

    if (isUpdated)
    {
        inBuildFailoverUnit->PersistenceState = PersistenceState::ToBeUpdated;

        int64 commitDuration;
        error = fmStore_.UpdateData(*inBuildFailoverUnit, commitDuration);

        if (error.IsSuccess())
        {
            fm_.WriteInfo(
                TraceRebuild, wformatString(failoverUnitId),
                "InBuildFailoverUnit updated: {0}\r\nCommit Duration = {1} ms", *inBuildFailoverUnit, commitDuration);

            inBuildFailoverUnits_[failoverUnitId] = move(inBuildFailoverUnit);
        }
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::RecoverPartitions()
{
    ErrorCode error = ErrorCodeValue::Success;

    if (fm_.IsMaster)
    {
        RecoverFMServicePartition();

        return error;
    }

    vector<FailoverUnitId> inBuildFailoverUnitsToDelete;

    AcquireWriteLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        ErrorCode e = RecoverPartition(it->second);

        if (e.IsSuccess())
        {
            inBuildFailoverUnitsToDelete.push_back(it->first);
        }
        else
        {
            error = ErrorCode::FirstError(error, e);
        }
    }

    for (FailoverUnitId const& failoverUnitId : inBuildFailoverUnitsToDelete)
    {
        inBuildFailoverUnits_.erase(failoverUnitId);
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::RecoverPartition(FailoverUnitId const& failoverUnitId)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (fm_.IsMaster)
    {
        bool isFound = RecoverFMServicePartition();
        
        if (!isFound)
        {
            error = ErrorCodeValue::PartitionNotFound;
        }

        return error;
    }

    AcquireWriteLock grab(lock_);

    auto it = inBuildFailoverUnits_.find(failoverUnitId);
    if (it != inBuildFailoverUnits_.end())
    {
        error = RecoverPartition(it->second);

        if (error.IsSuccess())
        {
            inBuildFailoverUnits_.erase(failoverUnitId);
        }
    }
    else
    {
        error = ErrorCodeValue::PartitionNotFound;
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::RecoverServicePartitions(wstring const& serviceName)
{
    ErrorCode error = ErrorCodeValue::Success;
    bool isFound = false;

    if (fm_.IsMaster)
    {
        isFound = RecoverFMServicePartition();

        if (!isFound)
        {
            error = ErrorCodeValue::ServiceNotFound;
        }

        return error;
    }

    vector<FailoverUnitId> inBuildFailoverUnitsToDelete;

    AcquireWriteLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        if (it->second->Description.Name == serviceName)
        {
            isFound = true;

            ErrorCode e = RecoverPartition(it->second);

            if (e.IsSuccess())
            {
                inBuildFailoverUnitsToDelete.push_back(it->first);
            }
            else
            {
                error = ErrorCode::FirstError(error, e);
            }
        }
    }

    for (FailoverUnitId const& failoverUnitId : inBuildFailoverUnitsToDelete)
    {
        inBuildFailoverUnits_.erase(failoverUnitId);
    }

    if (!isFound)
    {
        error = ErrorCodeValue::ServiceNotFound;
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::RecoverSystemPartitions()
{
    ErrorCode error = ErrorCodeValue::Success;

    if (fm_.IsMaster)
    {
        RecoverFMServicePartition();

        return error;
    }

    vector<FailoverUnitId> inBuildFailoverUnitsToDelete;

    AcquireWriteLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        if (ConsistencyUnitId::IsReserved(it->first.Guid))
        {
            ErrorCode e = RecoverPartition(it->second);

            if (e.IsSuccess())
            {
                inBuildFailoverUnitsToDelete.push_back(it->first);
            }
            else
            {
                error = ErrorCode::FirstError(error, e);
            }
        }
    }

    for (FailoverUnitId const& failoverUnitId : inBuildFailoverUnitsToDelete)
    {
        inBuildFailoverUnits_.erase(failoverUnitId);
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::RecoverPartition(InBuildFailoverUnitUPtr & inBuildFailoverUnit)
{
    ErrorCode error(ErrorCodeValue::Success);

    FailoverUnitUPtr failoverUnit = inBuildFailoverUnit->Generate(fm_.NodeCacheObj, true);

    ASSERT_IFNOT(
        failoverUnit,
        "FailoverUnit not generated during RecoverPartition: {0}", *inBuildFailoverUnit);

    // Update the lookup version.
    fm_.FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);

    // Persiste the generated FailoverUnit and delete the InBuildFailoverUnit in a single transaction.
    error = PersistGeneratedFailoverUnit(move(failoverUnit), inBuildFailoverUnit, inBuildFailoverUnit->Description);

    if (!error.IsSuccess())
    {
        fm_.WriteInfo(
            TraceRebuild, wformatString(inBuildFailoverUnit->Id),
            "Failed to persist generated FailoverUnit: {0}", error);
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::DeleteInBuildFailoverUnitsForService(wstring const& serviceName)
{
    AcquireWriteLock grab(lock_);

    ErrorCode error = ErrorCodeValue::Success;
    vector<FailoverUnitId> inBuildFailoverUnitsToDelete;

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        if (it->second->Description.Name == serviceName)
        {
            it->second->PersistenceState = PersistenceState::ToBeDeleted;

            int64 commitDuration;
            ErrorCode e = fmStore_.UpdateData(*it->second, commitDuration);

            if (e.IsSuccess())
            {
                inBuildFailoverUnitsToDelete.push_back(it->second->Id);
            }
            else
            {
                Trace.WriteWarning(TraceRebuild, wformatString(it->first),
                    "Failed to delete InBuild FailoverUnit: {0}\r\nCommit Duration = {1} ms",
                    e, commitDuration);
                error = ErrorCode::FirstError(error, e);
            }
        }
    }

    for (FailoverUnitId const& failoverUnitId : inBuildFailoverUnitsToDelete)
    {
        inBuildFailoverUnits_.erase(failoverUnitId);

        fm_.WriteInfo(
            TraceRebuild, wformatString(failoverUnitId),
            "InBuildFailoverUnit deleted.");
    }

    return error;
}

ErrorCode InBuildFailoverUnitCache::MarkInBuildFailoverUnitsToBeDeletedForService(wstring const& serviceName)
{
    AcquireWriteLock grab(lock_);

    ErrorCode error = ErrorCodeValue::Success;

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        InBuildFailoverUnit & inBuildFailoverUnit = *it->second;

        if (inBuildFailoverUnit.Description.Name == serviceName)
        {
            inBuildFailoverUnit.IsToBeDeleted = true;
            inBuildFailoverUnit.PersistenceState = PersistenceState::ToBeUpdated;

            int64 commitDuration;
            ErrorCode e = fmStore_.UpdateData(inBuildFailoverUnit, commitDuration);

            if (!e.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceRebuild, wformatString(it->first),
                    "Failed to mark InBuildFailoverUnit as ToBeDeleted: {0}\r\nCommit Duration = {1} ms", e, commitDuration);
                error = ErrorCode::FirstError(error, e);
            }
        }
    }

    return error;
}

bool InBuildFailoverUnitCache::InBuildReplicaExistsForNode(NodeId const& nodeId) const
{
    AcquireWriteLock grab(lock_);

    for (auto it = inBuildFailoverUnits_.begin(); it != inBuildFailoverUnits_.end(); ++it)
    {
        InBuildFailoverUnitUPtr const& inBuildFailoverUnit = it->second;

        if (inBuildFailoverUnit->InBuildReplicaExistsForNode(nodeId))
        {
            return true;
        }
    }

    return false;
}

void InBuildFailoverUnitCache::ReportHealth(InBuildFailoverUnit const& inBuildFailoverUnit, bool isDeleted, ErrorCode error)
{
    if (error.IsSuccess())
    {
        auto e = fm_.HealthClient->AddHealthReport(
            fm_.HealthReportFactoryObj.GenerateInBuildFailoverUnitHealthReport(
                inBuildFailoverUnit,
                isDeleted));

        fm_.WriteInfo(
            TraceRebuild, wformatString(inBuildFailoverUnit.Id),
            "AddHealthReport for InBuildFailoverUnit {0} with health sequence {1} completed with {2}",
            inBuildFailoverUnit.Id, inBuildFailoverUnit.HealthSequence, e);
    }
    else
    {
        fm_.HealthClient->HealthSkipSequence(
            *ServiceModel::Constants::HealthReportFMSource,
            FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_PARTITION,
            inBuildFailoverUnit.HealthSequence);

        fm_.WriteInfo(
            TraceRebuild, wformatString(inBuildFailoverUnit.Id),
            "Skipping health sequence {0} for InBuildFailoverUnit {1}",
            inBuildFailoverUnit.HealthSequence, inBuildFailoverUnit.Id);
    }
}

void InBuildFailoverUnitCache::Dispose()
{
    if (fullRebuildTimer_)
    {
        fullRebuildTimer_->Cancel();
    }
}
