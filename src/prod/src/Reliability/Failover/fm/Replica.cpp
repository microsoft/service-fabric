// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

FABRIC_QUERY_SERVICE_REPLICA_STATUS Replica::get_ReplicaStatus() const
{
    if (IsUp)
    {
        if (IsInBuild)
        {
            return FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD;
        }
        else if (IsStandBy)
        {
            return FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY;
        }
        else
        {
            return FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY;
        }
    }
    else if (IsDropped)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED;
    }
    else
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN;
    }
}

void Replica::set_ServiceLocation(std::wstring const& value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    replicaDesc_.ServiceLocation = value;
    PersistenceState = PersistenceState::ToBeUpdated;
}

void Replica::set_ReplicationEndpoint(std::wstring const& value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    replicaDesc_.ReplicationEndpoint = value;
    PersistenceState = PersistenceState::ToBeUpdated;
}

void Replica::OnSetToBeDropped()
{
    IsToBePromoted = false;
    if (!IsInCurrentConfiguration && !IsStandBy &&
		failoverUnit_->IsStateful && failoverUnit_->IsReconfigurationPrimaryAvailable())
    {
        IsPendingRemove = true;
    }
}

void Replica::set_IsToBeDroppedByFM(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if ((IsToBeDroppedByFM != value) && (!value || !IsDropped))
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::ToBeDroppedByFM);
        if (value)
        {
            OnSetToBeDropped();
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsToBeDroppedByPLB(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if ((IsToBeDroppedByPLB != value) && (!value || !IsDropped))
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::ToBeDroppedByPLB);
        if (value)
        {
            OnSetToBeDropped();
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsToBeDroppedForNodeDeactivation(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if ((IsToBeDroppedForNodeDeactivation != value) && (!value || !IsDropped))
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::ToBeDroppedForNodeDeactivation);
        if (value)
        {
            OnSetToBeDropped();
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsToBePromoted(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if ((IsToBePromoted != value) && (!value || !IsDropped))
    {
        ASSERT_IF(value && failoverUnit_->ToBePromotedReplicaExists, "A ToBePromoted replica already exists: {0}", *failoverUnit_);

        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::ToBePromoted);
        if (value)
        {
            IsToBeDroppedByPLB = false;
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsPendingRemove(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    if (IsPendingRemove != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::PendingRemove);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsDeleted(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    if (IsDeleted != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::Deleted);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsPreferredPrimaryLocation(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if (IsPreferredPrimaryLocation != value)
    {
        ASSERT_IF(value && failoverUnit_->PreferredPrimaryLocationExists, "Replica with PreferredPrimaryLocation already exists: {0}", *failoverUnit_);

        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::PreferredPrimaryLocation);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsPreferredReplicaLocation(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    ASSERT_IF(value && failoverUnit_->HasPersistedState, "IsPreferredReplicaLocation can only be set for non-persisted replicas: {0}", *failoverUnit_);

    if (IsPreferredReplicaLocation != value)
    {
        ASSERT_IF(value && IsPreferredPrimaryLocation, "Replica is already marked as PreferredPrimaryLocation: {0}", *failoverUnit_);

        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::PreferredReplicaLocation);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsPrimaryToBeSwappedOut(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    ASSERT_IF(value && CurrentConfigurationRole != ReplicaRole::Primary, "Non-primary replica cannot be swapped out: {0}", *failoverUnit_);

    if (IsPrimaryToBeSwappedOut != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::PrimaryToBeSwappedOut);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsPrimaryToBePlaced(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    ASSERT_IF(value && !failoverUnit_->IsStateful, "PrimaryToBePlaced can only be set for stateful replicas: {0}", *failoverUnit_);

    if (IsPrimaryToBePlaced != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::PrimaryToBePlaced);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsReplicaToBePlaced(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    ASSERT_IF(value && failoverUnit_->HasPersistedState, "ReplicaToBePlaced can only be set for non-persisted replicas: {0}", *failoverUnit_);

    if (IsReplicaToBePlaced != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::ReplicaToBePlaced);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsMoveInProgress(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if (IsMoveInProgress != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::MoveInProgress);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsEndpointAvailable(bool value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");
    if (IsEndpointAvailable != value)
    {
        flags_ = static_cast<ReplicaFlags::Enum>(flags_ ^ ReplicaFlags::EndpointAvailable);
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_FederationNodeInstance(Federation::NodeInstance const & value)
{ 
    replicaDesc_.FederationNodeInstance = value;
    PersistenceState = PersistenceState::ToBeUpdated;
}

void Replica::set_ReplicaId(int64 value)
{
    if (replicaDesc_.ReplicaId != value)
    {
        replicaDesc_.ReplicaId = value;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_InstanceId(int64 value)
{
    if (replicaDesc_.InstanceId != value)
    {
        replicaDesc_.InstanceId = value;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_IsUp(bool isUp)
{
    if (replicaDesc_.IsUp != isUp)
    {
        replicaDesc_.IsUp = isUp;
        IsToBePromoted = false;
        IsMoveInProgress = false;
        IsEndpointAvailable = false;

        if (!isUp && !failoverUnit_->HasPersistedState)
        {
            ReplicaState = ReplicaStates::Dropped;
        }

        lastUpDownTime_ = DateTime::Now();

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_State(ReplicaStates::Enum value)
{
    ASSERT_IF(failoverUnit_ == nullptr, "Invalid replica which has null FailoverUnit");

    if (ReplicaState != value)
    {
        bool wasInBuild = IsInBuild;

        replicaDesc_.State = value;

        if (value == ReplicaStates::InBuild)
        {
            lastBuildTime_ = DateTime::Now().Ticks;
        }
        else
        {
            if (wasInBuild)
            {
                lastBuildTime_ = DateTime::Now().Ticks - lastBuildTime_;
            }

            if (value == ReplicaStates::Dropped)
            {
                IsUp = false;
                IsToBeDroppedByFM = false;
                IsToBeDroppedByPLB = false;
                IsToBeDroppedForNodeDeactivation = false;
                IsToBePromoted = false;
            }
        }

        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_PreviousConfigurationRole(Reliability::ReplicaRole::Enum role)
{
    if (PreviousConfigurationRole != role)
    {
        replicaDesc_.PreviousConfigurationRole = role;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_CurrentConfigurationRole(Reliability::ReplicaRole::Enum role)
{
    if (CurrentConfigurationRole != role)
    {
        if (role != ReplicaRole::Primary)
        {
            IsPrimaryToBeSwappedOut = false;
        }

        replicaDesc_.CurrentConfigurationRole = role;
        IsEndpointAvailable = false;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_VersionInstance(ServiceModel::ServicePackageVersionInstance const& value)
{
    if (VersionInstance != value)
    {
        replicaDesc_.PackageVersionInstance = value;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

void Replica::set_ServiceUpdateVersion(uint64 value)
{
    if (serviceUpdateVersion_ != value)
    {
        serviceUpdateVersion_ = value;
        PersistenceState = PersistenceState::ToBeUpdated;
    }
}

bool Replica::get_IsStable() const
{
    if (IsToBeDropped)
    {
        return false;
    }

    // A primary that is being built is considered stable
    // as long as it is up.
    if (IsCurrentConfigurationPrimary)
    {
        return IsUp;
    }

    return (IsUp && IsReady);
}

StopwatchTime Replica::GetUpdateTime() const
{
    return (persistenceState_ == PersistenceState::NoChange ? StopwatchTime::FromDateTime(lastUpdated_) : failoverUnit_->ProcessingStartTime);
}

DateTime Replica::get_LastUpTime() const
{
    ASSERT_IFNOT(IsUp, "Replica is down: {0}", *this);

    return (lastUpDownTime_ == DateTime::MaxValue ? GetUpdateTime().ToDateTime() : lastUpDownTime_);
}

DateTime Replica::get_LastDownTime() const
{
    ASSERT_IF(IsUp, "Replica is up: {0}", *this);

    return (lastUpDownTime_ == DateTime::MaxValue ? GetUpdateTime().ToDateTime() : lastUpDownTime_);
}

TimeSpan Replica::get_LastBuildDuration() const
{
    if (IsInBuild)
    {
        return TimeSpan::FromTicks(DateTime::Now().Ticks - lastBuildTime_);
    }
    else
    {
        return TimeSpan::FromTicks(lastBuildTime_);
    }
}

void Replica::UpdateFailoverUnitPtr(FailoverUnit * failoverUnit)
{
    failoverUnit_ = failoverUnit;
}

void Replica::VerifyConsistency() const
{
    ASSERT_IF(
        nodeInfo_ == nullptr, 
        "Replica {0} of {1} doesn't have valid NodeInfo.", *this, failoverUnit_->Id);

    if (ReplicaState == ReplicaStates::Dropped)
    {
        ASSERT_IF(
            IsToBeDropped || IsToBePromoted || IsUp,
            "Replica {0} is Dropped, but is either IsToBeDropped, IsToBePromoted, or IsUp.", *this);
    }
}

Replica & Replica::operator = (Replica && other)
{
    if (this != &other)
    {
        failoverUnit_ = other.failoverUnit_;
        replicaDesc_ = other.replicaDesc_;
        nodeInfo_ = move(other.nodeInfo_);
        serviceUpdateVersion_ = other.serviceUpdateVersion_;
        flags_ = other.flags_;
        persistenceState_ = other.persistenceState_;
        lastActionTime_ = other.lastActionTime_;
        lastUpdated_ = other.lastUpdated_;
        lastBuildTime_ = other.lastBuildTime_;
        lastUpDownTime_ = other.lastUpDownTime_;
    }

    return *this;
}

Replica::Replica(Replica && other)
    : failoverUnit_(other.failoverUnit_),
      nodeInfo_(move(other.nodeInfo_)),
      replicaDesc_(other.replicaDesc_),
      serviceUpdateVersion_(other.serviceUpdateVersion_),
      flags_(other.flags_),
      persistenceState_(other.persistenceState_),
      lastActionTime_(other.lastActionTime_),
      lastUpdated_(other.lastUpdated_),
      lastBuildTime_(other.lastBuildTime_),
      lastUpDownTime_(other.lastUpDownTime_)
{
}

Replica::Replica() 
    : failoverUnit_(nullptr), 
      nodeInfo_(nullptr), 
      serviceUpdateVersion_(0),
      persistenceState_(PersistenceState::NoChange),
      lastBuildTime_(0),
      lastUpDownTime_(DateTime::MaxValue)
{
}

Replica::Replica(FailoverUnit * failoverUnit, NodeInfoSPtr && nodeInfo)
    : failoverUnit_(failoverUnit),
      nodeInfo_(move(nodeInfo)),
      serviceUpdateVersion_(failoverUnit->ServiceInfoObj->ServiceDescription.UpdateVersion),
      flags_(ReplicaFlags::None),
      persistenceState_(PersistenceState::ToBeInserted),
      lastActionTime_(StopwatchTime::Zero),
      lastUpdated_(DateTime::Now()),
      lastBuildTime_(DateTime::Now().Ticks),
      lastUpDownTime_(DateTime::Now())
{
    int64 instanceId = DateTime::Now().Ticks;
    int64 replicaId = instanceId;

    for (auto replica = failoverUnit_->BeginIterator; replica != failoverUnit_->EndIterator; ++replica)
    {
        if (replicaId <= replica->ReplicaId)
        {
            replicaId = replica->ReplicaId + 1;
            instanceId = replicaId;
        }
    }

    replicaDesc_ = Reliability::ReplicaDescription(nodeInfo_->NodeInstance, replicaId, instanceId, ReplicaRole::None, ReplicaRole::None, ReplicaStates::InBuild);
}

Replica::Replica(FailoverUnit * failoverUnit,
                 NodeInfoSPtr && nodeInfo,
                 int64 replicaId,
                 int64 instanceId,
                 ReplicaStates::Enum state,
                 ReplicaFlags::Enum flags,
                 Reliability::ReplicaRole::Enum previousConfigurationRole,
                 Reliability::ReplicaRole::Enum currentConfigurationRole,
                 bool isUp,
                 ServiceModel::ServicePackageVersionInstance const & version,
                 PersistenceState::Enum persistenceState,
                 Common::DateTime lastUpdated)
    : failoverUnit_(failoverUnit), 
      nodeInfo_(move(nodeInfo)),
      replicaDesc_(nodeInfo_->NodeInstance, replicaId, instanceId, previousConfigurationRole, currentConfigurationRole, state, isUp),
      serviceUpdateVersion_(failoverUnit->ServiceInfoObj->ServiceDescription.UpdateVersion),
      flags_(flags),
      persistenceState_(persistenceState),
      lastActionTime_(StopwatchTime::Zero),
      lastUpdated_(lastUpdated),
      lastBuildTime_(0),
      lastUpDownTime_(DateTime::Now())
{
    replicaDesc_.PackageVersionInstance = version;
}

Replica::Replica(
    FailoverUnit * failoverUnit,
    Reliability::ReplicaDescription const& replicaDescription,
    uint64 serviceUpdateVersion,
    ReplicaFlags::Enum flags,
    PersistenceState::Enum persistenceState,
    Common::DateTime lastUpdated)
    : failoverUnit_(failoverUnit),
      replicaDesc_(replicaDescription),
      serviceUpdateVersion_(serviceUpdateVersion),
      flags_(flags),
      persistenceState_(persistenceState),
      lastActionTime_(StopwatchTime::Zero),
      lastUpdated_(lastUpdated),
      lastBuildTime_(IsInBuild ? DateTime::Now().Ticks : DateTime::Zero.Ticks),
      lastUpDownTime_(DateTime::Now())
{
}

Replica::Replica(FailoverUnit * failoverUnit, Replica const& other)
    : failoverUnit_(failoverUnit), 
      nodeInfo_(other.nodeInfo_),
      replicaDesc_(other.replicaDesc_),
      serviceUpdateVersion_(other.serviceUpdateVersion_),
      flags_(other.flags_),
      persistenceState_(other.persistenceState_),
      lastActionTime_(other.lastActionTime_),
      lastUpdated_(other.lastUpdated_),
      lastBuildTime_(other.lastBuildTime_),
      lastUpDownTime_(other.lastUpDownTime_)
{
}

Replica::~Replica()
{
}

void Replica::Reuse(NodeInfoSPtr && node)
{
    int64 instanceId = max(DateTime::Now().Ticks, InstanceId + 1);
    int64 replicaId = instanceId;

    for (auto replica = failoverUnit_->BeginIterator; replica != failoverUnit_->EndIterator; ++replica)
    {
        if (replicaId <= replica->ReplicaId)
        {
            replicaId = replica->ReplicaId + 1;
            instanceId = replicaId;
        }
    }

    replicaDesc_.FederationNodeInstance = node->NodeInstance;
    replicaDesc_.ReplicaId = replicaId;
    replicaDesc_.InstanceId = instanceId;
    replicaDesc_.State = ReplicaStates::InBuild;
    nodeInfo_ = move(node);
    serviceUpdateVersion_ = failoverUnit_->ServiceInfoObj->ServiceDescription.UpdateVersion;
    replicaDesc_.PreviousConfigurationRole = ReplicaRole::None;
    replicaDesc_.CurrentConfigurationRole = ReplicaRole::None;
    replicaDesc_.IsUp = true;
    PersistenceState = PersistenceState::ToBeUpdated;
    replicaDesc_.ServiceLocation = wstring();
    replicaDesc_.ReplicationEndpoint = wstring();
    replicaDesc_.PackageVersionInstance = ServicePackageVersionInstance::Invalid;
    lastActionTime_ = StopwatchTime::Zero;
    lastBuildTime_ = DateTime::Now().Ticks;
    lastUpDownTime_ = DateTime::Now();

    if (IsPreferredPrimaryLocation)
    {
        flags_ = ReplicaFlags::PreferredPrimaryLocation;
    }
    else if (IsPreferredReplicaLocation)
    {
        flags_ = ReplicaFlags::PreferredReplicaLocation;
    }
    else
    {
        flags_ = ReplicaFlags::None;
    }
}

void Replica::set_PersistenceState(PersistenceState::Enum value)
{
    // if original state is ToBeInserted and
    // new state is ToBeUpdated, then keep it ToBeInserted
    if ((persistenceState_ != PersistenceState::ToBeInserted) ||
        (value != PersistenceState::ToBeUpdated))
    {
        persistenceState_ = value;

        if (persistenceState_ != PersistenceState::NoChange)
        {
            failoverUnit_->PersistenceState = PersistenceState::ToBeUpdated;
        }
    }

    if (persistenceState_ != PersistenceState::NoChange)
    {
        lastActionTime_ = StopwatchTime::Zero;
    }
}

void Replica::PostUpdate(DateTime updatedTime)
{
    if (PersistenceState == PersistenceState::ToBeInserted ||
        PersistenceState == PersistenceState::ToBeUpdated)
    {
        lastUpdated_ = updatedTime;
    }

    PersistenceState = PersistenceState::NoChange;
}

ReplicaMessageBody Replica::CreateReplicaMessageBody(bool isNewReplica) const
{
    ServiceModel::ServicePackageVersionInstance versionInstance;
    if (isNewReplica)
    {
        versionInstance = failoverUnit_->ServiceInfoObj->GetTargetVersion(NodeInfoObj->ActualUpgradeDomainId);
    }
    else
    {
        versionInstance = VersionInstance;
    }
 
    return ReplicaMessageBody(
        failoverUnit_->FailoverUnitDescription,
        replicaDesc_,
        failoverUnit_->ServiceInfoObj->ServiceDescription,
        versionInstance);
}

DeleteReplicaMessageBody Replica::CreateDeleteReplicaMessageBody() const
{
    return DeleteReplicaMessageBody(
        failoverUnit_->FailoverUnitDescription,
        replicaDesc_,
        failoverUnit_->ServiceInfoObj->ServiceDescription,
        failoverUnit_->ServiceInfoObj->IsForceDelete,
        ServiceModel::ServicePackageVersionInstance());
}

bool Replica::CanGenerateAction() const
{
    Common::StopwatchTime now = Stopwatch::Now();
    FailoverConfig const & config = FailoverConfig::GetConfig();
    Common::TimeSpan interval = (now > failoverUnit_->GetUpdateTime() + config.MaxActionRetryIntervalPerReplica ? config.MaxActionRetryIntervalPerReplica : config.MinActionRetryIntervalPerReplica);
    return (now > lastActionTime_ + interval);
}

bool Reliability::FailoverManagerComponent::Replica::ShouldBePrinted() const
{
    if (failoverUnit_->IsStateful || failoverUnit_->TargetReplicaSetSize > 0)
    {
        return true;
    }

    bool notAvailableOrNone = this->IsAvailable && this->Flags != ReplicaFlags::None;
    bool recentlyUpdated = Common::DateTime::Now() - this->LastUpdated < FailoverConfig::GetConfig().StatelessPartitionTracingInterval;
    
    return notAvailableOrNone || recentlyUpdated;
}

std::wstring Replica::GetStateString() const
{
    wstring pendingCloseFlags;

    if (NodeInfoObj)
    {
        if (NodeInfoObj->IsPendingApplicationUpgrade(FailoverUnitObj.ServiceInfoObj->ServiceType->Application->ApplicationId))
        {
            pendingCloseFlags += L"A";
        }

        if (NodeInfoObj->IsPendingFabricUpgrade)
        {
            pendingCloseFlags += L"F";
        }

        if (NodeInfoObj->IsPendingDeactivateNode)
        {
            pendingCloseFlags += L"G";
        }
    }

    if (failoverUnit_->IsStateful)
    {
        return wformatString(
            "{0}/{1} {2} {3} {4}{5}",
            PreviousConfigurationRole,
            CurrentConfigurationRole,
            ReplicaState,
            IsUp ? L"U" : L"D",
            flags_,
            pendingCloseFlags);
    }
    else
    {
        return wformatString(
            "{0} {1} {2}{3}",
            ReplicaState,
            IsUp ? L"U" : L"D",
            flags_,
            pendingCloseFlags);
    }
}

void Replica::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write(
        "{0} {1} {2}:{3} {4} {5} {6}/{7}",
        GetStateString(),
        FederationNodeInstance,
        ReplicaId,
        InstanceId,
        serviceUpdateVersion_,
        VersionInstance,
        lastUpDownTime_,
        LastUpdated);
}

void Replica::WriteToEtw(uint16 contextSequenceId) const
{
    if (ShouldBePrinted())
    {
        FailoverManager::FMEventSource->FMReplica(
            contextSequenceId,
            GetStateString(),
            FederationNodeInstance,
            ReplicaId,
            InstanceId,
            serviceUpdateVersion_,
            VersionInstance,
            lastUpDownTime_,
            LastUpdated);
    }    
}
