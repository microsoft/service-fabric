// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

FailoverUnit::FailoverUnit()
    : StoreData(),
      previousConfiguration_(false, EndIterator),
      currentConfiguration_(true, EndIterator),
      isConfigurationValid_(false),
      updateVersion_(0),
      extraDescription_(),
      currentHealthState_(FailoverUnitHealthState::Invalid),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      updateForHealthReport_(false),
      idString_(),
      fm_(nullptr),
      replicaDifference_(0),
      lastQuorumLossTime_(0),
      lastActionTime_(StopwatchTime::Zero),
      processingStartTime_(StopwatchTime::Zero),
      reconfigurationStartTime_(DateTime::Zero),
      placementStartTime_(DateTime::Now()),
      isPersistencePending_(false)
{
}

FailoverUnit::FailoverUnit(
    FailoverUnitId failoverUnitId,
    ConsistencyUnitDescription const& consistencyUnitDescription,
    bool isStateful,
    bool hasPersistedState,
    wstring const& serviceName,
    ServiceInfoSPtr const& serviceInfo)
    : StoreData(PersistenceState::ToBeInserted),
      failoverUnitDesc_(
        failoverUnitId,
        consistencyUnitDescription,
        Epoch(FailoverConfig::GetConfig().IsTestMode ? 1 : DateTime::Now().Ticks, Epoch::PrimaryEpochMask + 2),
        Epoch(Constants::InvalidDataLossVersion, Constants::InvalidConfigurationVersion)),
      idString_(failoverUnitId.ToString()),
      fm_(nullptr),
      serviceName_(serviceName),
      serviceInfo_(serviceInfo),
      flags_(isStateful, hasPersistedState, isStateful),
      previousConfiguration_(false, EndIterator),
      currentConfiguration_(true, EndIterator),
      isConfigurationValid_(true),
      lookupVersion_(0),
      lastActionTime_(StopwatchTime::Zero),
      lastUpdated_(DateTime::Now()),
      updateVersion_(0),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      extraDescription_(),
      currentHealthState_(FailoverUnitHealthState::Placement),
      updateForHealthReport_(false),
      replicaDifference_(0),
      lastQuorumLossTime_(0),
      processingStartTime_(StopwatchTime::Zero),
      reconfigurationStartTime_(DateTime::Zero),
      placementStartTime_(DateTime::Now()),
      isPersistencePending_(false)
{
    failoverUnitDesc_.TargetReplicaSetSize = serviceInfo_->ServiceDescription.TargetReplicaSetSize;
    failoverUnitDesc_.MinReplicaSetSize = serviceInfo_->ServiceDescription.MinReplicaSetSize;
}

FailoverUnit::FailoverUnit(
    FailoverUnitId failoverUnitId,
    ConsistencyUnitDescription const& consistencyUnitDescription,
    ServiceInfoSPtr const& serviceInfo,
    FailoverUnitFlags::Flags flags,
    Reliability::Epoch previousConfigurationEpoch,              
    Reliability::Epoch currentConfigurationEpoch,
    int64 lookupVersion,
    Common::DateTime lastUpdated,
    int64 updateVersion,
    int64 operationLSN,
    PersistenceState::Enum persistenceState)
    : StoreData(operationLSN, persistenceState),
      failoverUnitDesc_(
        failoverUnitId, 
        consistencyUnitDescription, 
        currentConfigurationEpoch,
        previousConfigurationEpoch),
      idString_(failoverUnitId.ToString()),
      fm_(nullptr),
      serviceName_(serviceInfo->Name),
      serviceInfo_(serviceInfo),
      flags_(flags),
      previousConfiguration_(false, EndIterator),
      currentConfiguration_(true, EndIterator),
      lookupVersion_(lookupVersion),
      isConfigurationValid_(false),
      lastActionTime_(StopwatchTime::Zero),
      lastUpdated_(lastUpdated),
      updateVersion_(updateVersion),
      extraDescription_(),
      currentHealthState_(FailoverUnitHealthState::Invalid),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      updateForHealthReport_(false),
      replicaDifference_(0),
      lastQuorumLossTime_(0),
      processingStartTime_(StopwatchTime::Zero),
      reconfigurationStartTime_(DateTime::Zero),
      placementStartTime_(DateTime::Now()),
      isPersistencePending_(false)
{
    failoverUnitDesc_.TargetReplicaSetSize = serviceInfo_->ServiceDescription.TargetReplicaSetSize;
    failoverUnitDesc_.MinReplicaSetSize = serviceInfo_->ServiceDescription.MinReplicaSetSize;
}

FailoverUnit::FailoverUnit(FailoverUnit const& other)
    : StoreData(other),
      failoverUnitDesc_(other.failoverUnitDesc_),
      idString_(other.idString_),
      fm_(other.fm_),
      serviceName_(other.serviceName_),
      serviceInfo_(other.serviceInfo_),
      flags_(other.flags_),
      previousConfiguration_(false, EndIterator),
      currentConfiguration_(true, EndIterator),
      lookupVersion_(other.lookupVersion_),
      isConfigurationValid_(false),
      lastActionTime_(other.lastActionTime_),
      lastUpdated_(other.lastUpdated_),
      updateVersion_(other.updateVersion_),
      extraDescription_(other.extraDescription_),
      currentHealthState_(other.currentHealthState_),
      healthSequence_(other.healthSequence_),
      updateForHealthReport_(other.updateForHealthReport_),
      replicaDifference_(other.replicaDifference_),
      lastQuorumLossTime_(other.lastQuorumLossTime_),
      processingStartTime_(other.processingStartTime_),
      reconfigurationStartTime_(other.reconfigurationStartTime_),
      placementStartTime_(other.placementStartTime_),
      isPersistencePending_(other.isPersistencePending_)
{
    for (Replica const& replica : other.replicas_)
    {
        replicas_.push_back(Replica(this, replica));
    }
}

FailoverUnit::FailoverUnit(FailoverUnit && other)
    : StoreData(other),
      failoverUnitDesc_(move(other.failoverUnitDesc_)),
      idString_(move(other.idString_)),
      fm_(other.fm_),
      serviceName_(move(other.serviceName_)),
      serviceInfo_(move(other.serviceInfo_)),
      flags_(other.flags_),
      previousConfiguration_(false, EndIterator),
      currentConfiguration_(true, EndIterator),
      lookupVersion_(other.lookupVersion_),
      isConfigurationValid_(false),
      lastActionTime_(other.lastActionTime_),
      lastUpdated_(other.lastUpdated_),
      updateVersion_(other.updateVersion_),
      extraDescription_(other.extraDescription_),
      currentHealthState_(other.currentHealthState_),
      healthSequence_(other.healthSequence_),
      updateForHealthReport_(other.updateForHealthReport_),
      replicaDifference_(other.replicaDifference_),
      lastQuorumLossTime_(other.lastQuorumLossTime_),
      processingStartTime_(other.processingStartTime_),
      reconfigurationStartTime_(other.reconfigurationStartTime_),
      placementStartTime_(other.placementStartTime_),
      isPersistencePending_(other.isPersistencePending_)
{
    for (Replica const& replica : other.replicas_)
    {
        replicas_.push_back(Replica(this, replica));
    }
}

FailoverUnit::~FailoverUnit()
{
}

FailoverUnit & FailoverUnit::operator=(FailoverUnit && other)
{
    if (this != &other)
    {
        this->PostRead(other.UpdateVersion);
        fm_ = other.fm_;
        serviceInfo_ = move(other.serviceInfo_);
        failoverUnitDesc_ = move(other.failoverUnitDesc_);
        serviceName_ = move(other.serviceName_);
        idString_ = move(other.idString_);
        replicas_ = move(other.replicas_);
        flags_ = move(other.flags_);
        previousConfiguration_.isCurrentConfiguration_ = false;
        currentConfiguration_.isCurrentConfiguration_ = true;
        isConfigurationValid_ = false;
        lookupVersion_ = other.lookupVersion_;
        lastActionTime_ = other.lastActionTime_;
        lastUpdated_ = other.lastUpdated_;
        updateVersion_ = other.updateVersion_;
        extraDescription_ = other.extraDescription_;
        currentHealthState_ = other.currentHealthState_;
        healthSequence_ = other.healthSequence_,
        updateForHealthReport_ = other.updateForHealthReport_;
        replicaDifference_ = other.replicaDifference_;
        lastQuorumLossTime_ = other.lastQuorumLossTime_;
        processingStartTime_= other.processingStartTime_;
        reconfigurationStartTime_ = other.reconfigurationStartTime_;
        placementStartTime_ = other.placementStartTime_;
        isPersistencePending_ = other.isPersistencePending_;
    }

    return *this;
}

wstring const& FailoverUnit::GetStoreType()
{
    return FailoverManagerStore::FailoverUnitType;
}

wstring const& FailoverUnit::GetStoreKey() const
{
    return IdString;
}

TimeSpan FailoverUnit::get_LastQuorumLossDuration() const
{
    if (IsQuorumLost())
    {
        return TimeSpan::FromTicks(DateTime::Now().Ticks - lastQuorumLossTime_);
    }
    else
    {
        return TimeSpan::FromTicks(lastQuorumLossTime_);
    }
}

void FailoverUnit::SetQuorumLossTime(int64 ticks)
{
    lastQuorumLossTime_ = ticks;
}

wstring const& FailoverUnit::get_IdString() const
{
    if (idString_.empty())
    {
        idString_ = Id.ToString();
    }

    return idString_;
}

void FailoverUnit::set_UpdateForHealthReport(bool value)
{
    updateForHealthReport_ = value; 
    PersistenceState = PersistenceState::ToBeUpdated;
}

void FailoverUnit::set_PersistenceState(PersistenceState::Enum value)
{
    // If original state is ToBeInserted and
    // new state is ToBeUpdated, then keep it ToBeInserted
    if ((PersistenceState != PersistenceState::ToBeInserted) ||
        (value != PersistenceState::ToBeUpdated))
    {
        StoreData::PersistenceState = value;
    }

    if (PersistenceState != PersistenceState::NoChange)
    {
        lastActionTime_ = StopwatchTime::Zero;
    }
}

void FailoverUnit::SetToBeDeleted()
{
    if (!IsToBeDeleted)
    {
        flags_.SetToBeDeleted();

        for (auto it = BeginIterator; it != EndIterator; ++it)
        {
            it->IsToBeDroppedByFM = true;
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void FailoverUnit::set_TargetReplicaSetSize(int value)
{
    if (TargetReplicaSetSize != value)
    {
        failoverUnitDesc_.TargetReplicaSetSize = value;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

int FailoverUnit::get_TargetReplicaSetSize() const 
{
    // If no autoscaling is defined we should keep on using the target size from service description.
    // Otherwise, if auto scaling is done by changing instance count, then use the one from FT
    if (serviceInfo_->ServiceDescription.ScalingPolicies.size() > 0 && serviceInfo_->ServiceDescription.ScalingPolicies[0].IsPartitionScaled())
    {
        return failoverUnitDesc_.TargetReplicaSetSize;
    }
    else
    {
        return serviceInfo_->ServiceDescription.TargetReplicaSetSize;
    }
}

void FailoverUnit::set_IsSwappingPrimary(bool value)
{
    if (IsSwappingPrimary != value)
    {
        flags_.SetSwappingPrimary(value);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void FailoverUnit::SetOrphaned(FailoverManager & fm)
{
    if (!IsOrphaned)
    {
        flags_.SetOrphaned();
        serviceInfo_ = fm.ServiceCacheObj.GetService(Constants::TombstoneServiceName);

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void FailoverUnit::set_IsUpgrading(bool value)
{
    if (IsUpgrading != value)
    {
        flags_.SetUpgrading(value);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

bool FailoverUnit::get_ToBePromotedReplicaExists() const
{
    bool result = false;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsToBePromoted)
        {
            result = true;
            break;
        }
    }

    return result;
}

bool FailoverUnit::get_ToBePromotedSecondaryExists() const
{
    bool result = false;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsCurrentConfigurationSecondary && replica->IsToBePromoted)
        {
            result = true;
            break;
        }
    }

    return result;
}

bool FailoverUnit::get_PreferredPrimaryLocationExists() const
{
    bool result = false;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsPreferredPrimaryLocation)
        {
            result = true;
            break;
        }
    }

    return result;
}

void FailoverUnit::set_NoData(bool value)
{
    if (NoData != value)
    {
        flags_.SetNoData(value);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

size_t FailoverUnit::get_UpReplicaCount() const
{
    size_t count = 0;

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->IsUp)
        {
            count++;
        }
    }

    return count;
}

size_t FailoverUnit::get_AvailableReplicaCount() const
{
    size_t count = 0;

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->IsAvailable)
        {
            count++;
        }
    }

    return count;
}

size_t FailoverUnit::get_PotentialReplicaCount() const
{
    size_t result = 0;

    ForEachUpReplica([&](Replica const & replica)
    {
        if (!replica.IsInCurrentConfiguration && !replica.IsToBeDropped && !replica.IsDropped)
        {
            result++;
        }
    });

    return result;
}

size_t FailoverUnit::get_InBuildReplicaCount() const
{
    size_t result = 0;

    ForEachUpReplica([&](Replica const & replica)
    {
        if (replica.IsInBuild)
        {
            result++;
        }
    });

    return result;
}

size_t FailoverUnit::get_IdleReadyReplicaCount() const
{
    size_t result = 0;

    ForEachUpReplica([&](Replica const & replica)
    {
        if (replica.CurrentConfigurationRole == ReplicaRole::Idle && replica.IsReady)
        {
            result++;
        }
    });

    return result;
}

size_t FailoverUnit::get_OfflineReplicaCount() const
{
    size_t count = 0;

    if (HasPersistedState)
    {
        for (auto replica = BeginIterator; replica != EndIterator; ++replica)
        {
            if (replica->IsOffline)
            {
                ++count;
            }
        }
    }

    return count;
}

size_t FailoverUnit::get_StandByReplicaCount() const
{
    size_t count = 0;

    if (HasPersistedState)
    {
        for (auto replica = BeginIterator; replica != EndIterator; ++replica)
        {
            if (replica->IsStandBy)
            {
                ++count;
            }
        }
    }

    return count;
}

size_t FailoverUnit::get_DroppedReplicaCount() const
{
    size_t count = 0;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsDropped)
        {
            ++count;
        }
    }

    return count;
}

size_t FailoverUnit::get_DeletedReplicaCount() const
{
    size_t count = 0;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsDeleted)
        {
            ++count;
        }
    }

    return count;
}

FailoverUnitConfiguration const& FailoverUnit::get_PreviousConfiguration() const
{
    if (!isConfigurationValid_)
    {
        ConstructConfiguration();
    }
    return previousConfiguration_;
}

FailoverUnitConfiguration const& FailoverUnit::get_CurrentConfiguration() const
{
    if (!isConfigurationValid_)
    {
        ConstructConfiguration();
    }
    return currentConfiguration_;
}

bool FailoverUnit::get_IsCreatingPrimary() const
{
    return Primary != EndIterator && Primary->IsUp && Primary->IsInBuild && PreviousConfiguration.IsEmpty && IsChangingConfiguration;
}

bool FailoverUnit::get_IsDroppingPrimary() const
{
    return (!CurrentConfiguration.IsEmpty &&
        PreviousConfiguration.Primary == CurrentConfiguration.Primary &&
        PreviousConfiguration.SecondaryCount == 0 &&
        CurrentConfiguration.SecondaryCount == 0 &&
        Primary->IsReady);
}

bool FailoverUnit::get_IsStable() const
{
    if (IsStateful)
    {
        return (!IsChangingConfiguration &&
            static_cast<int>(CurrentConfiguration.StableReplicaCount) == TargetReplicaSetSize &&
            static_cast<int>(CurrentConfiguration.AvailableCount) == TargetReplicaSetSize &&
            CurrentConfiguration.DownSecondaryCount == 0 &&
            !ToBePromotedReplicaExists);
    }
    else
    {
        int count = 0;
        for (Replica const& replica : replicas_)
        {
            if (replica.IsUp)
            {
                if (!replica.IsStable)
                {
                    return false;
                }
                else
                {
                    count ++;
                }
            }
        }

        return (count == TargetReplicaSetSize);
    }
}

FABRIC_QUERY_SERVICE_PARTITION_STATUS FailoverUnit::get_PartitionStatus() const
{
    if (IsToBeDeleted)
    {
        return FABRIC_QUERY_SERVICE_PARTITION_STATUS_DELETING;
    }
    else if (IsStateful)
    {
        if (IsQuorumLost())
        {
            return FABRIC_QUERY_SERVICE_PARTITION_STATUS_IN_QUORUM_LOSS;
        }
        else if (IsInReconfiguration)
        {
          return FABRIC_QUERY_SERVICE_PARTITION_STATUS_RECONFIGURING;
        }
        else if (CurrentConfiguration.AvailableCount >= MinReplicaSetSize)
        {
            return  FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY;
        }
        else
        {
            return FABRIC_QUERY_SERVICE_PARTITION_STATUS_NOT_READY;
        }
    }
    else
    {
        if (UpReplicaCount >= 1u)
        {
            return FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY;
        }
        else
        {
            return FABRIC_QUERY_SERVICE_PARTITION_STATUS_NOT_READY;
        }
    }
}

Replica const* FailoverUnit::GetReplica(Federation::NodeId id) const
{
    auto it = GetReplicaIterator(id);
    return it == replicas_.end() ? nullptr : &(*it);
}

Replica * FailoverUnit::GetReplica(Federation::NodeId id)
{
    auto it = GetReplicaIterator(id);
    return it == replicas_.end() ? nullptr : &(*it);
}

ReplicaIterator FailoverUnit::GetReplicaIterator(Federation::NodeId id)
{
    return find_if(replicas_.begin(), replicas_.end(), 
        [=](Replica & replica) -> bool { return replica.FederationNodeId == id; });
}

ReplicaConstIterator FailoverUnit::GetReplicaIterator(Federation::NodeId id) const
{
    return find_if(replicas_.begin(), replicas_.end(), 
        [=](Replica const & replica) -> bool { return replica.FederationNodeId == id; });
}

void FailoverUnit::ForEachReplica(std::function<void(Replica &)> processor)
{
    for_each(replicas_.begin(), replicas_.end(), processor);
}

void FailoverUnit::ForEachReplica(std::function<void(Replica const &)> processor) const
{
    for_each(replicas_.begin(), replicas_.end(), processor);
}

void FailoverUnit::ForEachUpReplica(std::function<void(Replica &)> processor)
{
    for_each(replicas_.begin(), replicas_.end(), [&](Replica& replica)
    {
        if (replica.IsUp)
        {
            processor(replica);
        }
    });
}

void FailoverUnit::ForEachUpReplica(std::function<void(Replica const &)> processor) const
{
    for_each(replicas_.begin(), replicas_.end(), [&](Replica const& replica)
    {
        if (replica.IsUp)
        {
            processor(replica);
        }
    });
}

bool FailoverUnit::HasPendingRemoveIdleReplica() const
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->IsPendingRemove && !it->IsInConfiguration)
        {
            return true;
        }
    }

    return false;
}

ReplicaIterator FailoverUnit::CreateReplica(NodeInfoSPtr && node)
{
    // create a new replica on the specified node
    Federation::NodeId nid = node->NodeInstance.Id;

    ReplicaIterator it = find_if(replicas_.begin(), replicas_.end(), 
        [=](Replica & replica) -> bool { return replica.FederationNodeId == nid; });

    if (it == replicas_.end())
    {
        it = replicas_.insert(replicas_.end(), Replica(this, move(node)));
        PersistenceState = PersistenceState::ToBeUpdated;
        isConfigurationValid_ = false;
    }
    else
    {
        ASSERT_IFNOT(it->IsDropped, "FailoverUnit {0} creating a new replica on {1} when old replica exists", *this, nid);
        it->Reuse(move(node));
        PersistenceState = PersistenceState::ToBeUpdated;
    }

    return it;
}

void FailoverUnit::AddReplica(
    ReplicaDescription const& replicaDescription,
    uint64 serviceUpdateVersion,
    ReplicaFlags::Enum flags,
    PersistenceState::Enum persistenceState,
    Common::DateTime lastUpdated)
{
    replicas_.insert(
        replicas_.end(), 
        Replica(this, replicaDescription, serviceUpdateVersion, flags, persistenceState, lastUpdated));

    PersistenceState = PersistenceState::ToBeUpdated;
    isConfigurationValid_ = false;
}

ReplicaIterator FailoverUnit::AddReplica(
    NodeInfoSPtr && nodeInfo,
    int64 replicaId,
    int64 instanceId,
    ReplicaStates::Enum state,
    ReplicaFlags::Enum flags,
    Reliability::ReplicaRole::Enum pcRole,
    Reliability::ReplicaRole::Enum ccRole,
    bool isUp,
    uint64 serviceUpdateVersion,
    ServiceModel::ServicePackageVersionInstance const& servicePackageVersionInstance,
    PersistenceState::Enum persistenceState,
    Common::DateTime lastUpdated)
{
    ASSERT_IFNOT(
        GetReplica(nodeInfo->NodeInstance.Id) == nullptr,
        "Replica on node {0} already exists: {1}", *this);

    ReplicaIterator it = replicas_.insert(
        replicas_.end(), 
        Replica(
            this,
            move(nodeInfo),
            replicaId,
            instanceId,
            state,
            flags,
            pcRole,
            ccRole,
            isUp, 
            servicePackageVersionInstance,
            persistenceState,
            lastUpdated));

    it->ServiceUpdateVersion = serviceUpdateVersion;

    PersistenceState = PersistenceState::ToBeUpdated;
    isConfigurationValid_ = false;

    return it;
}

void FailoverUnit::RemoveAllReplicas()
{
    ASSERT_IFNOT(Id.Guid == Constants::FMServiceGuid, "RemoveAllReplicas should only be called for the FM Service FailoverUnit.");

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        OnReplicaDropped(*replica);
        replica->PersistenceState = PersistenceState::ToBeDeleted;
    }

    int64 dataLossVersion = (PreviousConfigurationEpoch.DataLossVersion == CurrentConfigurationEpoch.DataLossVersion ? 
        0 : PreviousConfigurationEpoch.DataLossVersion);
    failoverUnitDesc_.PreviousConfigurationEpoch = Epoch(dataLossVersion, 0);
}

void FailoverUnit::UpdateReplicaPreviousConfigurationRole(ReplicaIterator replica, Reliability::ReplicaRole::Enum newRole)
{
    ASSERT_IF(&(replica->FailoverUnitObj) != this, 
        "Replica to change pcrole not belong to this failoverUnit");

    ASSERT_IFNOT(IsStateful, "Only stateful FailoverUnit has configuration");

    if (replica->PreviousConfigurationRole != newRole)
    {
        if (isConfigurationValid_ && replica->IsInPreviousConfiguration)
        {
            previousConfiguration_.RemoveReplica(replica);
        }

        replica->replicaDesc_.PreviousConfigurationRole = newRole;
        PersistenceState = PersistenceState::ToBeUpdated;
        if (isConfigurationValid_ && replica->IsInPreviousConfiguration)
        {
            previousConfiguration_.AddReplica(replica);
        }
    }
}

void FailoverUnit::UpdateReplicaCurrentConfigurationRole(ReplicaIterator replica, Reliability::ReplicaRole::Enum newRole)
{
    ASSERT_IF(&(replica->FailoverUnitObj) != this, 
        "Replica to change ccrole not belong to this failoverUnit");

    ASSERT_IFNOT(IsStateful, "Only stateful FailoverUnit has configuration");

    if (replica->CurrentConfigurationRole != newRole)
    {
        if (isConfigurationValid_ && replica->IsInCurrentConfiguration)
        {
            currentConfiguration_.RemoveReplica(replica);
        }

        replica->CurrentConfigurationRole = newRole;
        PersistenceState = PersistenceState::ToBeUpdated;
        if (isConfigurationValid_ && replica->IsInCurrentConfiguration)
        {
            currentConfiguration_.AddReplica(replica);
        }
    }
}

void FailoverUnit::RemoveFromCurrentConfiguration(ReplicaIterator replica)
{
    replica->IsPendingRemove = false;
    auto newRole = (HasPersistedState && !replica->IsUp && !replica->IsDropped? ReplicaRole::Idle : ReplicaRole::None);
    UpdateReplicaCurrentConfigurationRole(replica, newRole);
}

void FailoverUnit::UpdateEpochForDataLoss()
{
    failoverUnitDesc_.CurrentConfigurationEpoch = Epoch(
        FailoverConfig::GetConfig().IsTestMode ?
        failoverUnitDesc_.CurrentConfigurationEpoch.DataLossVersion + 1 :
        max(DateTime::Now().Ticks, failoverUnitDesc_.CurrentConfigurationEpoch.DataLossVersion + 1),
        failoverUnitDesc_.CurrentConfigurationEpoch.ConfigurationVersion);

    PersistenceState = PersistenceState::ToBeUpdated;
}

void FailoverUnit::UpdateEpochForConfigurationChange(bool isPrimaryChange, bool resetLSN)
{
    int64 newConfigurationVersion = failoverUnitDesc_.CurrentConfigurationEpoch.ConfigurationVersion + 1;
    if (isPrimaryChange)
    {
        newConfigurationVersion = (newConfigurationVersion + Epoch::PrimaryEpochMask + 1);
    }

    failoverUnitDesc_.CurrentConfigurationEpoch = Epoch(
        failoverUnitDesc_.CurrentConfigurationEpoch.DataLossVersion,
        newConfigurationVersion);

    if (isPrimaryChange && resetLSN)
    {
        ResetLSN();
    }

    PersistenceState = PersistenceState::ToBeUpdated;
}

void FailoverUnit::UpdatePointers(FailoverManager & fm, NodeCache const& nodeCache, ServiceCache & serviceCache)
{
    if (fm_ == nullptr)
    {
        fm_ = &fm;
    }

    if (ServiceInfoObj == nullptr || ServiceInfoObj->IsStale || ServiceInfoObj->ServiceType->IsStale)
    {
        ServiceInfoObj = serviceCache.GetService(IsOrphaned ? Constants::TombstoneServiceName : serviceName_);
        ASSERT_IF(ServiceInfoObj == nullptr, "Service {0} not found for {1}",
            serviceName_, IdString);

        //if there is a scaling policy we should not be reseting the target count
        //the target count from SD should be only used during initial placement
        //if the user clears an existing scaling policy we should set the target back from SD
        if (ServiceInfoObj->ServiceDescription.ScalingPolicies.size() == 0 || !ServiceInfoObj->ServiceDescription.ScalingPolicies[0].IsPartitionScaled())
        {
            failoverUnitDesc_.TargetReplicaSetSize = ServiceInfoObj->ServiceDescription.TargetReplicaSetSize;
        }
        failoverUnitDesc_.MinReplicaSetSize = MinReplicaSetSize;
    }

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->NodeInfoObj == nullptr || replica->NodeInfoObj->IsStale)
        {
            replica->NodeInfoObj = nodeCache.GetNode(replica->FederationNodeId);

            if (!replica->NodeInfoObj)
            {
                // This can happen if NodeStateRemoved has been called for the
                // the node the replica was hosted on, and the node's entry has
                // been removed from the NodeCache.
                ASSERT_IFNOT(
                    replica->IsDeleted,
                    "Replica is not deleted for non-existent node {0}: {1}",
                    replica->FederationNodeId, *this);

                replica->NodeInfoObj = make_shared<NodeInfo>(
                    replica->FederationNodeInstance,
                    NodeDescription(replica->FederationNodeId),
                    false, // IsUp
                    false, // IsReplicaUploaded
                    true, // IsNodeStateRemoved
                    DateTime::Zero); // LastUpdated
            }

            ASSERT_IF(
                replica->NodeInfoObj == nullptr,
                "Node {0} not found for {1}", replica->FederationNodeId, *this);
        }
    }
}

void FailoverUnit::PostRead(int64 operationLSN)
{
    StoreData::PostRead(operationLSN);

    idString_ = Id.ToString();
    for_each(replicas_.begin(), replicas_.end(), [=](Replica & replica)
    {
        replica.failoverUnit_ = this;
    });
}

ServiceReplicaSet FailoverUnit::CreateServiceReplicaSet() const
{
    if (IsStateful)
    {
        bool isPrimaryLocationValid = (!IsChangingConfiguration && CurrentConfiguration.IsPrimaryAvailable) ||
                                      (IsChangingConfiguration && CurrentConfiguration.Primary->IsEndpointAvailable);

        wstring primaryLocation = (isPrimaryLocationValid ? CurrentConfiguration.Primary->ServiceLocation : Constants::InvalidPrimaryLocation);

        vector<wstring> secondaryLocations;
        for (ReplicaConstIterator const& replica : CurrentConfiguration.Replicas)
        {
            if (replica->CurrentConfigurationRole == ReplicaRole::Secondary &&
                ((!IsChangingConfiguration && replica->IsAvailable) ||
                (IsChangingConfiguration && replica->IsEndpointAvailable)))
            {
                secondaryLocations.push_back(replica->ServiceLocation);
            }
        }

        return ServiceReplicaSet(true, isPrimaryLocationValid, move(primaryLocation), move(secondaryLocations), lookupVersion_);
    }
    else
    {
        vector<wstring> replicaLocations;
        for (Replica const& replica : replicas_)
        {
            if (replica.IsStable)
            {
                replicaLocations.push_back(replica.ServiceLocation);
            }
        }

        wstring primaryLocation = Constants::NotApplicable;
        return ServiceReplicaSet(false, false, move(primaryLocation), move(replicaLocations), lookupVersion_);
    }
}

void FailoverUnit::PostUpdate(DateTime updatedTime, bool isPersistenceDone)
{
    for (auto it = replicas_.begin(); it != replicas_.end(); )
    {
        if (it->PersistenceState == PersistenceState::ToBeDeleted)
        {
            it = replicas_.erase(it);
            isConfigurationValid_ = false;
        }
        else
        {
            it->PostUpdate(updatedTime);
            ++it;
        }
    }

    if (PersistenceState == PersistenceState::ToBeInserted ||
        PersistenceState == PersistenceState::ToBeUpdated)
    {
        if (!updateForHealthReport_)
        {
            lastUpdated_ = updatedTime;
        }
        ++updateVersion_;
    }

    updateForHealthReport_ = false;
    
    if (isPersistenceDone)
    {
        isPersistencePending_ = false;
    }
}

void FailoverUnit::SwapPrimary(ReplicaIterator newPrimary)
{
    ReplicaIterator oldPrimary = Primary;

    if (oldPrimary == newPrimary)
    {
        return;
    }

    newPrimary->CurrentConfigurationRole = ReplicaRole::Primary;
    oldPrimary->CurrentConfigurationRole = ReplicaRole::Secondary;

    isConfigurationValid_ = false;
}

ReplicaConstIterator FailoverUnit::get_ReconfigurationPrimary() const
{
    if (IsSwappingPrimary && PreviousConfiguration.IsPrimaryAvailable)
    {
        return PreviousConfiguration.Primary;
    }
    else
    {
        return Primary;
    }
}

ReplicaIterator FailoverUnit::get_ReconfigurationPrimary()
{
    if (IsSwappingPrimary && PreviousConfiguration.IsPrimaryAvailable)
    {
        return ConvertIterator(PreviousConfiguration.Primary);
    }
    else
    {
        return ConvertIterator(Primary);
    }
}

bool FailoverUnit::IsReconfigurationPrimaryActive() const
{
    ReplicaConstIterator primary = get_ReconfigurationPrimary();
    return (primary != EndIterator && primary->IsUp && !primary->IsStandBy);
}

bool FailoverUnit::IsReconfigurationPrimaryAvailable() const
{
    ReplicaConstIterator primary = get_ReconfigurationPrimary();
    return (primary != EndIterator && primary->IsAvailable);
}

void FailoverUnit::StartReconfiguration(bool isPrimaryChange)
{
    ASSERT_IF(IsChangingConfiguration,
        "Previous configuration not empty when trying to start reconfiguration: {0}", *this);

    int64 dataLossVersion = (PreviousConfigurationEpoch.DataLossVersion == Constants::InvalidDataLossVersion ? 
        CurrentConfigurationEpoch.DataLossVersion : PreviousConfigurationEpoch.DataLossVersion);
    failoverUnitDesc_.PreviousConfigurationEpoch = Epoch(dataLossVersion, CurrentConfigurationEpoch.ConfigurationVersion);

    for (auto it = BeginIterator; it != EndIterator; ++it)
    {
        it->PreviousConfigurationRole = it->CurrentConfigurationRole;
        it->PersistenceState = PersistenceState::ToBeUpdated;
    }

    isConfigurationValid_ = false;
    UpdateEpochForConfigurationChange(isPrimaryChange);

    reconfigurationStartTime_ = DateTime::Now();
}

void FailoverUnit::CompleteReconfiguration()
{
    ASSERT_IFNOT(IsChangingConfiguration, "FailoverUnit is not changing configuration");

    if (Primary->IsAvailable || PreviousConfigurationEpoch.DataLossVersion == CurrentConfigurationEpoch.DataLossVersion)
    {
        failoverUnitDesc_.PreviousConfigurationEpoch = Epoch::InvalidEpoch();
    }
    else
    {
        failoverUnitDesc_.PreviousConfigurationEpoch = Epoch(PreviousConfigurationEpoch.DataLossVersion, 0);
    }

    IsSwappingPrimary = false;

    for (auto it = BeginIterator; it != EndIterator; ++it)
    {
        if (it->CurrentConfigurationRole == ReplicaRole::None)
        {
            OnReplicaDropped(*it);
        }
        else if (it->CurrentConfigurationRole == ReplicaRole::Primary)
        {
            it->IsToBePromoted = false;

            if (it->IsPrimaryToBePlaced)
            {
                it->IsPreferredPrimaryLocation = false;
                it->IsPrimaryToBePlaced = false;
            }
        }
        else if (it->IsAvailable)
        {
            it->IsPreferredReplicaLocation = false;
            it->IsReplicaToBePlaced = false;
        }

        it->PreviousConfigurationRole = ReplicaRole::None;
        it->IsEndpointAvailable = false;
    }

    ResetLSN();

    PersistenceState = PersistenceState::ToBeUpdated;
    isConfigurationValid_ = false;
}

void FailoverUnit::RecoverFromDataLoss()
{
    if (!HasPersistedState)
    {
        return;
    }

    if (OfflineReplicaCount > 0 &&
        DateTime::Now() - lastUpdated_ < FailoverConfig::GetConfig().RecoverOnDataLossWaitDuration)
    {
        return;
    }

    bool isPrimarySelected = false;
    for (auto it = BeginIterator; it != EndIterator; it++)
    {
        if (it->IsUp)
        {
            if (isPrimarySelected)
            {
                it->CurrentConfigurationRole = ReplicaRole::Secondary;
            }
            else
            {
                it->CurrentConfigurationRole = ReplicaRole::Primary;
                it->ReplicaState = ReplicaStates::InBuild;
                isPrimarySelected = true;
            }
        }
    }

    if (isPrimarySelected)
    {
        isConfigurationValid_ = false;
    }
}

void FailoverUnit::ClearConfiguration()
{
    bool dataloss = !NoData || (static_cast<int>(CurrentConfiguration.ReplicaCount) >= MinReplicaSetSize);

    if (IsChangingConfiguration)
    {
        CompleteReconfiguration();
    }

    isConfigurationValid_ = false;

    for (auto it = BeginIterator; it != EndIterator; it++)
    {
        if (it->IsInConfiguration)
        {
            ASSERT_IF(!it->IsDropped,
                "ClearConfiguration finds replica in configuration: {0}",
                *this);

            it->CurrentConfigurationRole = ReplicaRole::None;
        }
    }

    RecoverFromDataLoss();

    if (dataloss)
    {
        NoData = false;
        failoverUnitDesc_.PreviousConfigurationEpoch = Epoch(CurrentConfigurationEpoch.DataLossVersion, Constants::InvalidConfigurationVersion);
        UpdateEpochForDataLoss();
    }
}

void FailoverUnit::ClearPreviousConfiguration()
{
    int64 dataLossVersion = (PreviousConfigurationEpoch.DataLossVersion == CurrentConfigurationEpoch.DataLossVersion ? 
        0 : PreviousConfigurationEpoch.DataLossVersion);
    failoverUnitDesc_.PreviousConfigurationEpoch = Epoch(dataLossVersion, 0);

    for (auto it = BeginIterator; it != EndIterator; ++it)
    {
        it->PreviousConfigurationRole = ReplicaRole::None;
    }

    PersistenceState = PersistenceState::ToBeUpdated;
    isConfigurationValid_ = false;
}

void FailoverUnit::ClearCurrentConfiguration()
{
    if (!NoData || (static_cast<int>(CurrentConfiguration.ReplicaCount) >= MinReplicaSetSize))
    {
        NoData = false;
        UpdateEpochForDataLoss();
    }

    ReplicaIterator newPrimary = EndIterator;
    bool foundPrimary = false;
    for (auto it = BeginIterator; it != EndIterator; it++)
    {
        if (!it->IsInCurrentConfiguration && it->IsInPreviousConfiguration && it->IsUp)
        {
            if (it->IsPreviousConfigurationPrimary)
            {
                foundPrimary = true;
                newPrimary = it;
            }
            else if (!foundPrimary && (newPrimary == EndIterator || it->IsAvailable))
            {
                newPrimary = it;
            }

            it->CurrentConfigurationRole = ReplicaRole::Secondary;
        }
    }

    ASSERT_IF(newPrimary == EndIterator, "ClearCurrentConfiguration no primary: {0}", *this);

    UpdateEpochForConfigurationChange(true /*isPrimaryChange*/);
    SwapPrimary(newPrimary);
}

void FailoverUnit::OnReplicaDropped(Replica & replica)
{
    OnReplicaDown(replica, true);
}

void FailoverUnit::OnReplicaDown(Replica & replica, bool isDropped)
{
    isDropped = isDropped || !HasPersistedState;
    bool droppedStateChanged = (isDropped && !replica.IsDropped);

    if (replica.IsUp)
    {
        bool wasReplicaMarkedToBeDropped = replica.IsToBeDropped;

        replica.IsUp = false;

        if (replica.ReplicaDescription.FirstAcknowledgedLSN != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            replica.ReplicaDescription.FirstAcknowledgedLSN = 0;
        }

        if (replica.IsInConfiguration)
        {
            isConfigurationValid_ = false;
        }

        if (IsStateful && IsReconfigurationPrimaryAvailable() && replica.CurrentConfigurationRole != ReplicaRole::None)
        {
            replica.IsPendingRemove = true;
        }

        if (replica.CurrentConfigurationRole == ReplicaRole::Primary ||
            replica.PreviousConfigurationRole == ReplicaRole::Primary)
        {
            OnPrimaryDown();
        }
        else if (!HasPersistedState &&
                 !replica.IsPreferredPrimaryLocation &&
                 !replica.IsPreferredReplicaLocation &&
                 !wasReplicaMarkedToBeDropped)
        {
            if (FailoverConfig::GetConfig().RestoreReplicaLocationAfterUpgrade)
            {
                ApplicationInfoSPtr applicationInfo = ServiceInfoObj->ServiceType->Application;

                if (fm_ && fm_->FabricUpgradeManager.Upgrade &&
                    fm_->FabricUpgradeManager.Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
                {
                    replica.IsPreferredReplicaLocation = true;
                }
                else if (applicationInfo->Upgrade &&
                    applicationInfo->Upgrade->IsDomainStarted(replica.NodeInfoObj->ActualUpgradeDomainId))
                {
                    replica.IsPreferredReplicaLocation = true;
                }
            }
        }
    }
    else if (droppedStateChanged && replica.IsInCurrentConfiguration && IsReconfigurationPrimaryAvailable())
    {
        replica.IsPendingRemove = true;
    }

    if (droppedStateChanged)
    {
        replica.ReplicaState = ReplicaStates::Dropped;
        if (!replica.IsInConfiguration)
        {
            replica.CurrentConfigurationRole = ReplicaRole::None;
            replica.PreviousConfigurationRole = ReplicaRole::None;
        }
    }
}

void FailoverUnit::OnPrimaryDown()
{
    for (auto it = BeginIterator; it != EndIterator; ++it)
    {
        if (!it->IsInConfiguration)
        {
            if (HasPersistedState)
            {
                if (it->IsReady)
                {
                    it->ReplicaState = ReplicaStates::InBuild;
                }
            }
            else
            {
                it->IsToBeDroppedByFM = true;
            }

            it->IsPendingRemove = false;
        }

        it->IsToBePromoted = false;
    }
}

void FailoverUnit::VerifyConsistency() const
{
    ASSERT_IF(serviceInfo_ == nullptr, "FailoverUnit {0} doesn't have valid service info", *this);
    ASSERT_IF(serviceInfo_->ServiceType == nullptr, "Service '{0}' doesn't have a valid ServiceType.", serviceInfo_->Name);

    if (serviceInfo_->ServiceDescription.ScalingPolicies.size() == 0 || !serviceInfo_->ServiceDescription.ScalingPolicies[0].IsPartitionScaled())
    {
        ASSERT_IFNOT(
            failoverUnitDesc_.TargetReplicaSetSize == serviceInfo_->ServiceDescription.TargetReplicaSetSize,
            "TargetReplicaSetSize {0} is not consistent with ServiceInfo: {1}", failoverUnitDesc_.TargetReplicaSetSize, *this);
    }

    ASSERT_IFNOT(
        failoverUnitDesc_.MinReplicaSetSize == serviceInfo_->ServiceDescription.MinReplicaSetSize,
        "MinReplicaSetSize {0} is not consistent with ServiceInfo: {1}", failoverUnitDesc_.MinReplicaSetSize, *this);

    for (Replica const & replica : replicas_)
    {
        replica.VerifyConsistency();

        // verify replica's failoverUnit ref 
        ASSERT_IFNOT(&(replica.FailoverUnitObj) == this, "Replica {0} not belong to this failoverUnit", replica);
    }

    if (isConfigurationValid_)
    {
        set<ReplicaConstIterator> pcSet;
        set<ReplicaConstIterator> ccSet;

        for (ReplicaConstIterator it = replicas_.begin(); it != replicas_.end(); it++)
        {
            // insert replica to PreviousConfiguration or CurrentConfiguration set
            if (it->IsInPreviousConfiguration)
            {
                pcSet.insert(it);
            }

            if (it->IsInCurrentConfiguration)
            {
                ccSet.insert(it);
            }
        }

        set<ReplicaConstIterator> pcSet2;
        set<ReplicaConstIterator> ccSet2;

        for (ReplicaConstIterator it : previousConfiguration_.replicas_)
        {
            pcSet2.insert(it);
        }
        for (ReplicaConstIterator it : currentConfiguration_.replicas_)
        {
            ccSet2.insert(it);
        }

        ASSERT_IFNOT(pcSet == pcSet2,
            "The PreviousConfiguration {0} is not consistent {1}",
            previousConfiguration_, *this);

        ASSERT_IFNOT(ccSet == ccSet2,
            "The CurrentConfiguration {0} is not consistent {1}",
            currentConfiguration_, *this);
    }
}

void FailoverUnit::TraceState() const
{
    fm_->FTEvents.FTState(Id.Guid, *this);
}

void FailoverUnit::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7} {8} {9}", 
        serviceName_,
        serviceInfo_->ServiceDescription.TargetReplicaSetSize,
        serviceInfo_->ServiceDescription.MinReplicaSetSize,
        failoverUnitDesc_,
        flags_,
        updateVersion_,
        lookupVersion_,
        healthSequence_,
        OperationLSN,
        isPersistencePending_);

    if (PersistenceState != PersistenceState::NoChange)
    {
        writer.Write(" {0}", PersistenceState);
    }

    writer.WriteLine(" {0}", lastUpdated_);

    for (Replica const & replica : replicas_)
    {		
        if (replica.ShouldBePrinted())
        {
            writer.WriteLine("{0}", replica);
        }        
    }
}

void FailoverUnit::WriteToEtw(uint16 contextSequenceId) const
{
    wstring serviceName;
    if (DateTime::Now() - lastUpdated_ >= FailoverConfig::GetConfig().FTDetailedTraceInterval)
    {
        serviceName = serviceName_;
    }

    FailoverManager::FMEventSource->FMFailoverUnit(
        contextSequenceId,
        serviceName,
        serviceInfo_->ServiceDescription.TargetReplicaSetSize,
        serviceInfo_->ServiceDescription.MinReplicaSetSize,
        failoverUnitDesc_,
        wformatString(flags_),
        updateVersion_,
        lookupVersion_,
        healthSequence_,
        OperationLSN,
        isPersistencePending_,
        LastUpdated,
        replicas_);
}

void FailoverUnit::ConstructConfiguration() const
{
    currentConfiguration_.Clear();
    previousConfiguration_.Clear();

    currentConfiguration_.primary_ = EndIterator;
    previousConfiguration_.primary_ = EndIterator;

    currentConfiguration_.end_ = EndIterator;
    previousConfiguration_.end_ = EndIterator;

    for (ReplicaConstIterator it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->IsInCurrentConfiguration)
        {
            currentConfiguration_.AddReplica(it);
        }

        if (it->IsInPreviousConfiguration)
        {
            previousConfiguration_.AddReplica(it);
        }
    }

    isConfigurationValid_ = true;
}

DoReconfigurationMessageBody FailoverUnit::CreateDoReconfigurationMessageBody() const
{
    vector<ReplicaDescription> replicaDescriptions;
    ForEachReplica([&] (Replica const & replica)
    {
        if (replica.IsInConfiguration)
        {
            if (replica.IsCreating)
            {
                ServiceModel::ServicePackageVersionInstance version;
                version = ServiceInfoObj->GetTargetVersion(replica.NodeInfoObj->ActualUpgradeDomainId);

                ReplicaDescription description(replica.ReplicaDescription);
                description.PackageVersionInstance = version;
                replicaDescriptions.push_back(move(description));
            }
            else
            {
                replicaDescriptions.push_back(replica.ReplicaDescription);
            }
        }
    });

    return DoReconfigurationMessageBody(FailoverUnitDescription, ServiceInfoObj->ServiceDescription, move(replicaDescriptions));
}

FailoverUnitMessageBody FailoverUnit::CreateFailoverUnitMessageBody() const
{
    return FailoverUnitMessageBody(FailoverUnitDescription);
}

StopwatchTime FailoverUnit::GetUpdateTime() const 
{ 
    return (PersistenceState == PersistenceState::NoChange ? StopwatchTime::FromDateTime(lastUpdated_) : processingStartTime_); 
}

bool FailoverUnit::CanGenerateAction() const
{
    Common::StopwatchTime now = Stopwatch::Now();
    FailoverConfig const& config = FailoverConfig::GetConfig();
    Common::TimeSpan interval = (now > GetUpdateTime() + config.MaxActionRetryIntervalPerReplica ? config.MaxActionRetryIntervalPerReplica : config.MinActionRetryIntervalPerReplica);
    return (now > lastActionTime_ + interval);
}

bool FailoverUnit::ShouldGenerateReconfigurationAction()
{
    if (CanGenerateAction())
    {
        lastActionTime_ = Stopwatch::Now();
        return true;
    }

    return false;
}

bool FailoverUnit::IsQuorumLost() const
{
    if (!HasPersistedState || CurrentConfiguration.IsEmpty)
    {
        return false;
    }

    if (CurrentConfiguration.UpCount < CurrentConfiguration.WriteQuorumSize)
    {
        return true;
    }

    return (IsInReconfiguration && PreviousConfiguration.UpCount < PreviousConfiguration.ReadQuorumSize);
}

void FailoverUnit::DropOfflineReplicas()
{
    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->IsOffline)
        {
            this->OnReplicaDropped(*replica);
        }
    }
}

void FailoverUnit::ResetLSN()
{
    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        replica->ReplicaDescription.FirstAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
        replica->ReplicaDescription.LastAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
    }
}

LoadBalancingComponent::FailoverUnitDescription FailoverUnit::GetPLBFailoverUnitDescription(StopwatchTime processingStartTime) const
{
    auto itToBePromoted = find_if(replicas_.begin(), replicas_.end(), [](Replica const& r)
    {
        return ((r.CurrentConfigurationRole == ReplicaRole::Idle || r.IsCurrentConfigurationSecondary) && r.IsToBePromoted && !r.IsToBeDropped);
    });

    bool hasSufficientReplicas = (ServiceInfoObj && CurrentConfiguration.AvailableCount >= static_cast<size_t>(TargetReplicaSetSize));
    vector<LoadBalancingComponent::ReplicaDescription> replicas;

    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        ReplicaRole::Enum currentRole = replica->ReplicaDescription.CurrentConfigurationRole;
        LoadBalancingComponent::ReplicaRole::Enum plbReplicaRole = LoadBalancingComponent::ReplicaRole::None;
        bool plbReplicaRoleDefined = false;

        if (replica->IsDropped)
        {
            if (replica->IsPrimaryToBePlaced || replica->IsReplicaToBePlaced)
            {
                plbReplicaRole = LoadBalancingComponent::ReplicaRole::Dropped;
                plbReplicaRoleDefined = true;
            }
            else
            {
                continue;
            }
        }

        if (replica->IsOffline)
        {
            // it is down but not dropped, so it should be a stateful persisted failover unit
            // we need to convert to None after RRWD due to how scaleout is treated in PLB
            // None replicas will not be accounted in scaleout so PLB will be able to add a new replica after RRWD
            // If the replica remained in its Idle role from FM PLB would not place a replica due to scaleout constraint
            if (hasSufficientReplicas ||
                (ServiceInfoObj && processingStartTime.ToDateTime() - replica->LastDownTime > ServiceInfoObj->ServiceDescription.ReplicaRestartWaitDuration))
            {
                currentRole = ReplicaRole::None;
            }
        }
        else if (!plbReplicaRoleDefined && replica->IsStandBy && !replica->IsInCurrentConfiguration)
        {
            plbReplicaRole = LoadBalancingComponent::ReplicaRole::StandBy;
            plbReplicaRoleDefined = true;
        }

        if (replica == itToBePromoted)
        {
            currentRole = ReplicaRole::Primary;
        }
        else if (currentRole == ReplicaRole::Primary && itToBePromoted != replicas_.end())
        {
            currentRole = ReplicaRole::Secondary;
        }

        replicas.push_back(LoadBalancingComponent::ReplicaDescription(
            replica->FederationNodeInstance,
            plbReplicaRoleDefined ? plbReplicaRole : ReplicaRole::ConvertToPLBReplicaRole(IsStateful, currentRole),
            replica->ReplicaState,
            replica->IsUp,
            replica->Flags));
    }

    return LoadBalancingComponent::FailoverUnitDescription(
        FailoverUnitDescription.FailoverUnitId.Guid,
        wstring(serviceName_),
        this->UpdateVersion,
        move(replicas),
        ReplicaDifference,
        this->FailoverUnitFlags,
        IsChangingConfiguration || IsStateful && !CurrentConfiguration.IsPrimaryAvailable,
        IsQuorumLost(),
        FailoverUnitDescription.TargetReplicaSetSize);
}

bool FailoverUnit::get_IsUnhealthy() const
{
    if (IsStateful)
    {
        if (IsQuorumLost())
        {
            return true;
        }
        else if (IsChangingConfiguration)
        {
            return true;
        }
        else if (static_cast<int>(CurrentConfiguration.AvailableCount) < TargetReplicaSetSize)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (static_cast<int>(AvailableReplicaCount) == 0 || static_cast<int>(AvailableReplicaCount) < TargetReplicaSetSize)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool FailoverUnit::get_IsPlacementNeeded() const
{
    if (IsStateful)
    {
        return CurrentConfiguration.AvailableCount < static_cast<int>(TargetReplicaSetSize);
    }
    else
    {
        return AvailableReplicaCount < static_cast<int>(TargetReplicaSetSize);
    }
}

bool FailoverUnit::get_HasPendingUpgradeOrDeactivateNodeReplica() const
{
    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (replica->NodeInfoObj->IsPendingUpgradeOrDeactivateNode())
        {
            return true;
        }
    }

    return false;
}

int FailoverUnit::GetInitialReplicaPlacementCount() const
{
    TESTASSERT_IFNOT(IsStateful, "GetInitialReplicaPlacementCount() called for a stateless service partition: {0}", *this);

    for (auto const& placementPolicy : serviceInfo_->ServiceDescription.PlacementPolicies)
    {
        if (placementPolicy.Type == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE)
        {
            return TargetReplicaSetSize;
        }
    }

    return 1;
}

bool FailoverUnit::IsBelowMinReplicaSetSize() const
{
    return (IsStateful && CurrentConfiguration.AvailableCount < static_cast<size_t>(MinReplicaSetSize));
}

void FailoverUnit::AddDetailedHealthReportDescription(__out std::wstring & extraDescription) const
{
    FailoverConfig & config = FailoverConfig::GetConfig();

    StringWriter writer(extraDescription);
    writer.WriteLine();
    writer.WriteLine("{0} {1} {2} {3}",
        ServiceName,
        TargetReplicaSetSize,
        MinReplicaSetSize,
        IdString);
    
    if (IsStateful)
    {
        ForEachReplica([&writer](Replica const & replica)
        {
            if (replica.IsInConfiguration || !replica.IsDropped)
            {
                writer.WriteLine("  {0}/{1} {2} {3} {4}",
                    replica.PreviousConfigurationRole,
                    replica.CurrentConfigurationRole,
                    replica.ReplicaStatus,
                    replica.NodeInfoObj->NodeName,
                    replica.ReplicaId);
            }
        });
    } 
    else
    {
        int numTotalReplicas = 0;
        int availableReplicaCount = 0;
        int numReplicasWritten = 0;
        std::vector<size_t> availableReplicasIndexes;
        for (size_t i = 0; i != replicas_.size(); i++)
        {
            if (replicas_[i].IsDropped)
            {
                continue;
            }

            numTotalReplicas++;

            if (!replicas_[i].IsAvailable)
            {
                if (numReplicasWritten < config.MaxReplicasInHealthReportDescription)
                {
                    writer.WriteLine("  {0} {1} {2}",
                        replicas_[i].ReplicaStatus,
                        replicas_[i].NodeInfoObj->NodeName,
                        replicas_[i].ReplicaId);
                    numReplicasWritten++;
                }
            } 
            else
            {
                availableReplicaCount++;
                availableReplicasIndexes.push_back(i);
            }
        }

        auto it = availableReplicasIndexes.cbegin();

        while (it != availableReplicasIndexes.cend() && numReplicasWritten < config.MaxReplicasInHealthReportDescription)
        {
            writer.WriteLine("  {0} {1} {2}",
                replicas_[*it].ReplicaStatus,
                replicas_[*it].NodeInfoObj->NodeName,
                replicas_[*it].ReplicaId);

            numReplicasWritten++;
            it++;
        }

        writer.WriteLine("  (Showing {0} out of {1} instances. Total available instances: {2})", numReplicasWritten, numTotalReplicas, availableReplicaCount);
    }

    if (ServiceInfoObj->ServiceDescription.ScalingPolicies.size() > 0 && ServiceInfoObj->ServiceDescription.ScalingPolicies[0].IsPartitionScaled())
    {
        writer.WriteLine("Autoscaling is enabled for this partition. Desired instance count is {0}", TargetReplicaSetSize);
    }
    writer.WriteLine();
    writer.Write("For more information see: http://aka.ms/sfhealth");
}

FailoverUnitHealthState::Enum FailoverUnit::GetCurrentHealth(__out std::wstring & extraDescription) const
{
    FailoverConfig & config = FailoverConfig::GetConfig();
    DateTime now = DateTime::Now();

    FailoverUnitHealthState::Enum result = FailoverUnitHealthState::Healthy;
    extraDescription = L"";

    if (this->IsToBeDeleted)
    {
        return FailoverUnitHealthState::DeletionInProgress;
    }

    if (IsStateful)
    {
        if (IsQuorumLost())
        {
            AddDetailedHealthReportDescription(extraDescription);
            return FailoverUnitHealthState::QuorumLost;
        }

        if (IsInReconfiguration)
        {
            if (now > reconfigurationStartTime_ + config.ReconfigurationTimeLimit)
            {
                AddDetailedHealthReportDescription(extraDescription);
                return FailoverUnitHealthState::ReconfigurationStuck;
            }
            else
            {
                result = FailoverUnitHealthState::Reconfiguration;
            }
        }
        else if (static_cast<int>(CurrentConfiguration.AvailableCount) < TargetReplicaSetSize)
        {
            result = FailoverUnitHealthState::Placement;
        }
    }
    else
    {
        if (static_cast<int>(AvailableReplicaCount) == 0 || static_cast<int>(AvailableReplicaCount) < TargetReplicaSetSize)
        {
            result = FailoverUnitHealthState::Placement;
        }
    }

    if ((result == FailoverUnitHealthState::Placement) &&
        (now > placementStartTime_ + config.PlacementTimeLimit))
    {
        if (IsStateful)
        {
            AddDetailedHealthReportDescription(extraDescription);
            return (IsBelowMinReplicaSetSize() ? FailoverUnitHealthState::PlacementStuckBelowMinReplicaCount : FailoverUnitHealthState::PlacementStuck);
        }
        else
        {
            AddDetailedHealthReportDescription(extraDescription);
            return (AvailableReplicaCount == 0u ? FailoverUnitHealthState::PlacementStuckAtZeroReplicaCount : FailoverUnitHealthState::PlacementStuck);
        }
    }

    return result;
}

FailoverUnitHealthState::Enum FailoverUnit::GetEffectiveHealth(FailoverUnitHealthState::Enum state)
{
    if (state == FailoverUnitHealthState::Placement || state == FailoverUnitHealthState::Build || state == FailoverUnitHealthState::Reconfiguration)
    {
        return FailoverUnitHealthState::Healthy;
    }

    return state;
}

bool FailoverUnit::UpdateHealthState()
{
    wstring extraDescription;
    FailoverUnitHealthState::Enum state = GetCurrentHealth(extraDescription);

    bool result = false;
    if (state != currentHealthState_)
    {
        result = (GetEffectiveHealth(state) != GetEffectiveHealth(currentHealthState_));
        currentHealthState_ = state;
    }

    if (extraDescription != extraDescription_)
    {
        result = true;
        extraDescription_ = move(extraDescription);
    }

    return result || updateForHealthReport_;
}

bool FailoverUnit::IsHealthReportNeeded()
{
    if (currentHealthState_ == FailoverUnitHealthState::Healthy || 
        currentHealthState_ == FailoverUnitHealthState::QuorumLost ||
        currentHealthState_ == FailoverUnitHealthState::PlacementStuck ||
        currentHealthState_ == FailoverUnitHealthState::PlacementStuckBelowMinReplicaCount ||
        currentHealthState_ == FailoverUnitHealthState::PlacementStuckAtZeroReplicaCount ||
        currentHealthState_ == FailoverUnitHealthState::ReconfigurationStuck)
    {
        return false;
    }

    wstring extraDescription;
    FailoverUnitHealthState::Enum state = GetCurrentHealth(extraDescription);

    bool result = (GetEffectiveHealth(state) != GetEffectiveHealth(currentHealthState_));

    return (result || extraDescription != extraDescription_);
}

int FailoverUnit::GetRandomInteger(int max) const
{
    return (Stopwatch::Now().Ticks + abs(Id.Guid.GetHashCode())) % max;
}

bool FailoverUnit::IsSafeToCloseReplica(Replica const& replica, __out bool & isQuorumCheckNeeded, __out UpgradeSafetyCheckKind::Enum & kind) const
{
    ASSERT_IFNOT(
        replica.IsUp,
        "IsSafeToCloseReplica should only be called for an Up replica: {0}", replica); 

    isQuorumCheckNeeded = false;
    bool isSafeToCloseReplica = true;

    if (IsToBeDeleted)
    {
        isSafeToCloseReplica = true;
    }
    else if (IsStateful)
    {
        if (replica.IsInConfiguration)
        {
            isQuorumCheckNeeded = true;
        }

        if (IsQuorumLost())
        {
            if (replica.IsCurrentConfigurationPrimary && replica.IsReady)
            {
                isSafeToCloseReplica = false;
                kind = UpgradeSafetyCheckKind::EnsureAvailability;
            }
            else
            {
                isSafeToCloseReplica = true;
            }
        }
        else if (IsCreatingPrimary)
        {
            if (TargetReplicaSetSize > 1)
            {
                isSafeToCloseReplica = false;
                kind = UpgradeSafetyCheckKind::WaitForInBuildReplica;
            }
        }
        else if (IsChangingConfiguration)
        {
            isSafeToCloseReplica = false;
            kind = UpgradeSafetyCheckKind::WaitForReconfiguration;
        }
        else if (replica.IsInBuild && HasPersistedState)
        {
            isSafeToCloseReplica = false;
            kind = UpgradeSafetyCheckKind::WaitForInBuildReplica;
        }
        else if (replica.IsCurrentConfigurationPrimary && replica.IsReady && InBuildReplicaCount > 0)
        {
            isSafeToCloseReplica = false;
            kind = UpgradeSafetyCheckKind::WaitForInBuildReplica;
        }
        else if (!replica.IsInConfiguration)
        {
            isSafeToCloseReplica = true;
        }
    }
    else
    {
        isQuorumCheckNeeded = true;
    }

    return isSafeToCloseReplica;
}

int FailoverUnit::GetSafeReplicaCloseCount(FailoverManager const& fm, ApplicationInfo const& applicationInfo) const
{
    size_t effectiveUpCount = 0;
    size_t minRequired = 0;

    if (IsStateful)
    {
        size_t ccEffectiveUpCount = 0;
        size_t pcEffectiveUpCount = 0;

        for (auto replica = replicas_.begin(); replica != replicas_.end(); replica++)
        {
            if ((FailoverConfig::GetConfig().IsStrongSafetyCheckEnabled ? replica->IsAvailable : replica->IsUp) &&
                !replica->NodeInfoObj->IsPendingClose(fm, applicationInfo))
            {
                if (replica->IsInCurrentConfiguration)
                {
                    ccEffectiveUpCount++;
                }
                
                if (replica->IsInPreviousConfiguration)
                {
                    pcEffectiveUpCount++;
                }
            }
        }

        effectiveUpCount = (IsInReconfiguration ? min(ccEffectiveUpCount, pcEffectiveUpCount) : ccEffectiveUpCount);

        size_t ccMinRequired = max(CurrentConfiguration.WriteQuorumSize, static_cast<size_t>(MinReplicaSetSize));
        size_t pcMinRequired = max(PreviousConfiguration.ReadQuorumSize, static_cast<size_t>(MinReplicaSetSize));
        minRequired = (IsInReconfiguration ? max(ccMinRequired, pcMinRequired) : ccMinRequired);
    }
    else
    {
        for (auto replica = replicas_.begin(); replica != replicas_.end(); replica++)
        {
            if ((FailoverConfig::GetConfig().IsStrongSafetyCheckEnabled ? replica->IsAvailable : replica->IsUp) &&
                !replica->NodeInfoObj->IsPendingClose(fm, applicationInfo))
            {
                effectiveUpCount++;
            }
        }
        
        minRequired = 1;
    }

    int readyCount = static_cast<int>(effectiveUpCount) - static_cast<int>(minRequired);

    if ((readyCount == 0 && effectiveUpCount >= static_cast<size_t>(TargetReplicaSetSize)))
    {
        readyCount = 1;
    }

    return readyCount;
}

bool FailoverUnit::IsSafeToRemove(wstring const& batchId) const
{
    if (NoData || IsToBeDeleted)
    {
        return true;
    }

    int effectiveDroppedCount = 0;
    for (ReplicaConstIterator const& replica : CurrentConfiguration.Replicas)
    {
        NodeDeactivationInfo const & nodeDeactivationInfo = replica->NodeInfoObj->DeactivationInfo;
        if ((replica->IsDropped) ||
            (nodeDeactivationInfo.IsRemove &&
                (nodeDeactivationInfo.IsDeactivationComplete ||
                 nodeDeactivationInfo.ContainsBatch(batchId))))
        {
            effectiveDroppedCount++;
        }
    }

    int minRequired = static_cast<int>(CurrentConfiguration.WriteQuorumSize);

    int readyCount = static_cast<int>(CurrentConfiguration.ReplicaCount) - effectiveDroppedCount - minRequired;

    return (readyCount >= 0);
}

bool FailoverUnit::IsSafeToUpgrade(DateTime upgradeStartTime) const
{
    return (IsToBeDeleted || !IsQuorumLost() || (lastQuorumLossTime_ < upgradeStartTime.Ticks));
}

bool FailoverUnit::AreAllReplicasInSameUD() const
{
    set<wstring> UDs;

    for (auto replica = BeginIterator; replica != EndIterator; ++replica)
    {
        if (!replica->IsDropped)
        {
            UDs.insert(replica->NodeInfoObj->ActualUpgradeDomainId);
        }
    }

    return (UDs.size() == 1u);
}

bool FailoverUnit::IsReplicaMoveNeeded(Replica const& replica) const
{
    return (!IsToBeDeleted &&
            replica.IsCurrentConfigurationPrimary &&
            ((FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgrade) ||
             TargetReplicaSetSize > 1) &&
            !IsQuorumLost());
}

bool FailoverUnit::IsReplicaMoveNeededDuringUpgrade(Replica const& replica) const
{
    if (TargetReplicaSetSize > 1 && AreAllReplicasInSameUD())
    {
        return false;
    }

    return IsReplicaMoveNeeded(replica);
}

bool FailoverUnit::IsReplicaMoveNeededDuringDeactivateNode(Replica const& replica) const
{
    if (!replica.NodeInfoObj->DeactivationInfo.IsRestart)
    {
        return false;
    }

    return IsReplicaMoveNeeded(replica);
}
