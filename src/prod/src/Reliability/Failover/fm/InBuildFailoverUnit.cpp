// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;

const int64 PrimaryEpochIncrement = 4;

StringLiteral const InBuildFailoverUnitCheckReplicaTag("CheckReplica");

InBuildFailoverUnit::InBuildFailoverUnit(InBuildFailoverUnit const& other)
    : StoreData(other),
      failoverUnitId_(other.failoverUnitId_),
      consistencyUnitDescription_(other.consistencyUnitDescription_),
      serviceDescription_(other.serviceDescription_),
      cc_(other.cc_ ? make_unique<ReportedConfiguration>(*other.cc_) : nullptr),
      pc_(other.pc_ ? make_unique<ReportedConfiguration>(*other.pc_) : nullptr),
      reportingNodes_(other.reportingNodes_),
      replicas_(other.replicas_),
      maxEpoch_(other.maxEpoch_),
      isCCActivated_(other.isCCActivated_),
      isPrimaryAvailable_(other.isPrimaryAvailable_),
      isSecondaryAvailable_(other.isSecondaryAvailable_),
      isSwapPrimary_(other.isSwapPrimary_),
      isToBeDeleted_(other.isToBeDeleted_),
      pcEpochByReadyPrimary_(other.pcEpochByReadyPrimary_),
      createdTime_(other.createdTime_),
      lastUpdated_(other.lastUpdated_),
      idString_(other.idString_),
      healthSequence_(other.healthSequence_),
      isHealthReported_(other.isHealthReported_)
{
}

InBuildFailoverUnit::InBuildFailoverUnit(
    FailoverUnitId const& failoverUnitId,
    ConsistencyUnitDescription const& consistencyUnitDescription,
    ServiceDescription const& serviceDescription)
    : StoreData(),
      failoverUnitId_(failoverUnitId),
      consistencyUnitDescription_(consistencyUnitDescription),
      serviceDescription_(serviceDescription),
      cc_(nullptr),
      pc_(nullptr),
      reportingNodes_(),
      replicas_(),
      maxEpoch_(),
      isCCActivated_(false),
      isPrimaryAvailable_(false),
      isSecondaryAvailable_(false),
      isSwapPrimary_(false),
      isToBeDeleted_(false),
      pcEpochByReadyPrimary_(),
      createdTime_(DateTime::Now()),
      lastUpdated_(DateTime::Now()),
      idString_(),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      isHealthReported_(false)
{
}

wstring const& InBuildFailoverUnit::GetStoreType()
{
    return FailoverManagerStore::InBuildFailoverUnitType;
}

wstring const& InBuildFailoverUnit::GetStoreKey() const
{
    return IdString;
}

wstring const& InBuildFailoverUnit::get_IdString() const
{
    if (idString_.empty())
    {
        idString_ = Id.ToString();
    }

    return idString_;
}

bool InBuildFailoverUnit::Add(FailoverUnitInfo const& report, NodeInstance const& from, FailoverManager& fm)
{
    if (IsDuplicate(report, from) || IsStale(report, from))
    {
        return false;
    }

    auto selfReport = report.Replicas.end();

    bool fromReadyPrimary = report.IsReportFromPrimary;
    bool fromReadySecondary = false;

    for (auto it = report.Replicas.begin(); it != report.Replicas.end(); ++it)
    {
        ReplicaDescription const & description = it->Description;
        auto ccOrICRole = (report.ICEpoch == Epoch::InvalidEpoch() ? description.CurrentConfigurationRole : it->ICRole);

        if (description.FederationNodeId == from.Id)
        {
            selfReport = it;
            if (fromReadyPrimary && ccOrICRole == ReplicaRole::Secondary)
            {
                isSwapPrimary_ = true;
            }
        }

        if (description.FederationNodeId == from.Id &&
            !fromReadyPrimary &&
            description.IsAvailable &&
            (ccOrICRole == ReplicaRole::Secondary ||
             description.PreviousConfigurationRole == ReplicaRole::Secondary))
        {
            fromReadySecondary = true;
        }
    }

    TESTASSERT_IF(selfReport == report.Replicas.end(),
        "Report {0} from {1} does not contain itself",
        report, from);

    TESTASSERT_IF(
        report.CCEpoch == Epoch::InvalidEpoch(),
        "{0} reported invalid ccEpoch: {1}", from, report);

    ASSERT_IF(
        report.ServiceDescription.Instance != serviceDescription_.Instance,
        "Replica reporting different service instance {0} for {1} from {2}",
        report, *this, from);

    // Update the max epoch reported by a replica.
    if (report.CCEpoch > maxEpoch_)
    {
        maxEpoch_ = report.CCEpoch;
    }

    if (report.ServiceDescription.UpdateVersion > serviceDescription_.UpdateVersion)
    {
        serviceDescription_ = report.ServiceDescription;
    }

    if (selfReport->Description.PackageVersionInstance.InstanceId < serviceDescription_.PackageVersionInstance.InstanceId)
    {
        serviceDescription_.PackageVersionInstance = report.ServiceDescription.PackageVersionInstance;
    }

    // Add the sender to the set of reporting nodes.
    reportingNodes_.push_back(from.Id);

    isPrimaryAvailable_ = isPrimaryAvailable_ || fromReadyPrimary;
    isSecondaryAvailable_ = isSecondaryAvailable_ || fromReadySecondary;

    if (fromReadyPrimary)
    {
        pcEpochByReadyPrimary_ = report.PCEpoch;
    }

    bool discardPC = false;
    for (ReplicaInfo const& replica : report.Replicas)
    {
        CheckReplica(replica, report, from, fromReadyPrimary, fromReadySecondary, fm, discardPC);
    }

    if (discardPC)
    {
        pc_ = nullptr;
    }

    auto ccOrICRole = (report.ICEpoch == Epoch::InvalidEpoch() ? selfReport->Description.CurrentConfigurationRole : selfReport->ICRole);

    if (IsStateful && !IsIdle(selfReport->Description.PreviousConfigurationRole, ccOrICRole) && !selfReport->Description.IsDropped)
    {
        if (report.PCEpoch == Epoch::InvalidEpoch())
        {
            unique_ptr<ReportedConfiguration> cc = make_unique<ReportedConfiguration>(report, from, Type::CC);
            ConsiderCC(cc, true);
        }
        else
        {
            if (fromReadyPrimary)
            {
                unique_ptr<ReportedConfiguration> cc = make_unique<ReportedConfiguration>(report, from, Type::CC);
                ConsiderCC(cc, false);

                unique_ptr<ReportedConfiguration> pc = make_unique<ReportedConfiguration>(report, from, Type::PC);
                ConsiderPC(pc);
            }
            else if (report.ICEpoch != Epoch::InvalidEpoch())
            {
                unique_ptr<ReportedConfiguration> ic = make_unique<ReportedConfiguration>(report, from, Type::IC);
                ConsiderCC(ic, false);

                unique_ptr<ReportedConfiguration> pc = make_unique<ReportedConfiguration>(report, from, Type::PC);
                ConsiderPC(pc);
            }
            else
            {
                unique_ptr<ReportedConfiguration> pc = make_unique<ReportedConfiguration>(report, from, Type::PC);
                ConsiderCC(pc, true);
            }
        }
    }

    return true;
}

bool InBuildFailoverUnit::IsDuplicate(FailoverUnitInfo const& report, NodeInstance const& from)
{
    bool result = false;

    if (find(reportingNodes_.begin(), reportingNodes_.end(), from.Id) != reportingNodes_.end())
    {
        for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
        {
            ReplicaDescription & existingReplica = it->Description;

            if (existingReplica.FederationNodeInstance == report.LocalReplica.FederationNodeInstance &&
                existingReplica.ReplicaId == report.LocalReplica.ReplicaId &&
                existingReplica.InstanceId == report.LocalReplica.InstanceId &&
                existingReplica.IsUp == report.LocalReplica.IsUp)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

bool InBuildFailoverUnit::IsStale(FailoverUnitInfo const& report, NodeInstance const& from) const
{
    bool result = false;

    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (replica->IsSelfReported && replica->Description.FederationNodeId == from.Id)
        {
            result = from.InstanceId < replica->Description.FederationNodeInstance.InstanceId ||
                     report.LocalReplica.InstanceId < replica->Description.InstanceId;

            break;
        }
    }

    return result;
}

bool InBuildFailoverUnit::InBuildReplicaExistsForNode(NodeId const& nodeId) const
{
    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (replica->Description.FederationNodeId == nodeId)
        {
            return true;
        }
    }

    return false;
}

bool InBuildFailoverUnit::IsServiceUpdateNeeded() const
{
    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (replica->ServiceUpdateVersion < serviceDescription_.UpdateVersion)
        {
            return true;
        }
    }

    return false;
}

bool InBuildFailoverUnit::OnReplicaDropped(NodeId const& nodeId, bool isDeleted)
{
    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (replica->Description.FederationNodeId == nodeId)
        {
            bool result = false;

            if (!isDeleted && !replica->Description.IsDropped)
            {
                replica->Description.State = ReplicaStates::Dropped;
                result = true;
            }
            else if (isDeleted && !replica->IsDeleted)
            {
                replica->IsDeleted = true;
                result = true;
            }

            return result;
        }
    }

    return false;
}

bool InBuildFailoverUnit::DropOfflineReplicas(FailoverUnit & failoverUnit, NodeCache const& nodeCache)
{
    bool hasDownReplicaOnUpNode = false;

    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (!replica->Description.IsUp && !replica->Description.IsDropped)
        {
            NodeInfoSPtr nodeInfo = nodeCache.GetNode(replica->Description.FederationNodeId);

            if (!nodeInfo || !nodeInfo->IsUp)
            {
                replica->Description.State = ReplicaStates::Dropped;
                failoverUnit.OnReplicaDropped(*failoverUnit.GetReplica(replica->Description.FederationNodeId));
            }
            else if (!replica->IsSelfReported)
            {
                hasDownReplicaOnUpNode = true;
            }
        }
    }

    return !hasDownReplicaOnUpNode;
}

bool InBuildFailoverUnit::IsIdle(ReplicaRole::Enum pcRole, ReplicaRole::Enum ccOrICRole)
{
    return ((ccOrICRole == ReplicaRole::Idle) ||
            (pcRole == ReplicaRole::Idle && ccOrICRole == ReplicaRole::None));
}

ReplicaRole::Enum InBuildFailoverUnit::GetRole(unique_ptr<ReportedConfiguration> const& configuration, ReplicaDescription const& replica)
{
    if (configuration && !configuration->IsEmpty())
    {
        if (configuration->IsPrimary(replica.FederationNodeId))
        {
            return ReplicaRole::Primary;
        }
        else if (configuration->IsSecondary(replica.FederationNodeId))
        {
            return ReplicaRole::Secondary;
        }
    }
            
    return ReplicaRole::None;
}

void InBuildFailoverUnit::CheckReplica(ReplicaInfo const & replicaInfo, FailoverUnitInfo const& report, NodeInstance const& from, bool fromReadyPrimary, bool fromReadySecondary, FailoverManager& fm, __out bool & discardPC)
{
    ReplicaDescription replica = replicaInfo.Description;

    // Indicates if the replica hosted by the reporting node.
    bool isSelfReport = (replica.FederationNodeId == from.Id);
    if (isSelfReport)
    {
        TESTASSERT_IFNOT(
            replica.FederationNodeInstance == from,
            "Invalid self report from {0}. NodeInstance does not match: {1}",
            from, report);
    }

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        ReplicaDescription & existingReplica = it->Description;

        if (existingReplica.FederationNodeId == replica.FederationNodeId)
        {
            it->IsReportedByPrimary |= fromReadyPrimary;
            it->IsReportedBySecondary |= fromReadySecondary;

            if (isSelfReport)
            {
                existingReplica.FederationNodeInstance = replica.FederationNodeInstance;

                fm.Events.EndpointsUpdated(
                    IdString,
                    replica.FederationNodeInstance,
                    replica.ReplicaId,
                    replica.InstanceId,
                    InBuildFailoverUnitCheckReplicaTag,
                    replica.ServiceLocation,
                    replica.ReplicationEndpoint);

                // Endpoint information not useful if the replica itself is not up
                existingReplica.ServiceLocation = replica.ServiceLocation;
                existingReplica.ReplicationEndpoint = replica.ReplicationEndpoint;

                // Information below only apply to self report
                it->CCEpoch = report.CCEpoch;
                it->ICEpoch = report.ICEpoch;
                it->PCEpoch = report.PCEpoch;

                it->Version = report.LocalReplica.PackageVersionInstance;
            }

            // We prefer the role(s) reported by the primary. This is needed to 
            // correctly assign roles for Idle replicas and for S/N configurations.

            if (fromReadyPrimary || (isSelfReport && !it->IsReportedByPrimary && !it->IsReportedBySecondary))
            {
                existingReplica.PreviousConfigurationRole = replica.PreviousConfigurationRole;
                existingReplica.CurrentConfigurationRole = replica.CurrentConfigurationRole;
            }
            else if (!isPrimaryAvailable_ && fromReadySecondary)
            {
                if (report.ICEpoch != Epoch::InvalidEpoch())
                {
                    existingReplica.PreviousConfigurationRole = replica.PreviousConfigurationRole;
                    existingReplica.CurrentConfigurationRole = replicaInfo.ICRole;
                }
                else
                {
                    existingReplica.PreviousConfigurationRole = ReplicaRole::None;
                    existingReplica.CurrentConfigurationRole = replica.CurrentConfigurationRole;

                    discardPC = true;
                }
            }

            if (replica.InstanceId > existingReplica.InstanceId)
            {
                ASSERT_IF(
                    (replica.ReplicaId < existingReplica.ReplicaId) ||
                    (replica.FederationNodeInstance.InstanceId < existingReplica.FederationNodeInstance.InstanceId),
                    "Higher incoming replica instance has lower replica id/node instance: current: {0}\r\nReport: {1} from {2}",
                    *this, report, from);

                existingReplica = replica;
            }
            else if (replica.InstanceId == existingReplica.InstanceId)
            {
                ASSERT_IF(
                    (existingReplica.FederationNodeInstance != replica.FederationNodeInstance && existingReplica.IsUp && replica.IsUp) ||
                    (existingReplica.ReplicaId != replica.ReplicaId),
                    "NodeInstance/ReplicaId does not match: current: {0}\r\nReport: {1} from {2}",
                    *this, report, from);

                if (existingReplica.State < replica.State)
                {
                    existingReplica.State = replica.State;
                }

                existingReplica.IsUp &= replica.IsUp;
            }
            else
            {
                ASSERT_IF(isSelfReport && replica.ReplicaId >= existingReplica.ReplicaId,
                    "Self report contains smaller instance: {0}\r\nReport: {1} from {2}",
                    *this, report, from);
            }

            return;
        }
    }

    InBuildReplica inBuildReplica(replica, fromReadyPrimary, fromReadySecondary);
    if (isSelfReport)
    {
        inBuildReplica.CCEpoch = report.CCEpoch;
        inBuildReplica.ICEpoch = report.ICEpoch;
        inBuildReplica.PCEpoch = report.PCEpoch;

        inBuildReplica.ServiceUpdateVersion = report.ServiceDescription.UpdateVersion;
        inBuildReplica.Version = report.LocalReplica.PackageVersionInstance;
    }

    replicas_.push_back(move(inBuildReplica));
}

void InBuildFailoverUnit::ConsiderCC(unique_ptr<ReportedConfiguration> & cc, bool isCCActivated)
{
    if (cc_ == nullptr)
    {
        // If this is the first report for CC, use it first.
        cc_ = move(cc);
        isCCActivated_ = isCCActivated;
    }
    else if (cc_->Epoch == cc->Epoch)
    {
        // If the versions are the same, one configuration must be a subset
        // of the other configuration, and we keep the smaller configuration.
        // This can happen if there is a downshift from RA and the missing
        // replicas are dropped.
        if (!cc->Contains(*cc_))
        {
            swap(cc_, cc);

            ASSERT_IFNOT(
                cc->Contains(*cc_),
                "The smaller configuration '{0}' is not a subset of the larger configuration '{1}' for {2}",
                *cc_, *cc, *this);
        }

        isCCActivated_ = (isCCActivated_ || isCCActivated);
    }
    else
    {
        // Keep the latest version as CC and consider the smaller
        // as uncertain.
        if (cc_->Epoch < cc->Epoch)
        {
            swap(cc_, cc);
            swap(isCCActivated_, isCCActivated);
        }

        if (isCCActivated)
        {
            ConsiderPC(cc);
        }
    }
}

void InBuildFailoverUnit::ConsiderPC(unique_ptr<ReportedConfiguration> & pc)
{
    if (pc == nullptr)
    {
        return;
    }

    if (pc_ == nullptr)
    {
        // If this is the first PC reported, use it.
        pc_ = move(pc);
    }
    else if (pc_->Epoch == pc->Epoch)
    {
        ASSERT_IFNOT(
            pc_->Contains(*pc) && pc->Contains(*pc_),
            "Two PCs with the same epoch are not the same:\r\nExistingPC: {0}\r\nIncomingPC: {1}\r\n{2}",
            *pc_, *pc, *this);
    }
    else if (pc->Epoch > pc_->Epoch && pc_->Epoch != pcEpochByReadyPrimary_)
    {
        // Keep the highest version configuration as PC.
        pc_ = move(pc);
    }
}

void InBuildFailoverUnit::GenerateReplica(FailoverUnit & failoverUnit, InBuildReplica & inBuildReplica)
{
    ReplicaDescription & replica = inBuildReplica.Description;

    // Assuming we know the correct PC & CC set, the roles for primary & secondary must be correct 
    // (except for the case of losing PC/CC).  However, we still need to determine the correct 
    // role between none and idle.
    ReplicaRole::Enum ccRole = GetRole(cc_, replica);
    ReplicaRole::Enum pcRole = GetRole(pc_, replica);

    // If a replica is not reported by itself, it must be Down.
    auto it = find(reportingNodes_.begin(), reportingNodes_.end(), replica.FederationNodeId);
    if (it == reportingNodes_.end())
    {
        replica.IsUp = false;
    }

    ReplicaStates::Enum state = replica.State;

    // If a non-persisted replica is Down, its state should be Dropped.
    if (!replica.IsUp && !HasPersistedState)
    {
        state = ReplicaStates::Dropped;
    }

    // If a replica is not in CC, we should keep it as idle unless it has
    // been demoted and it is not-persistent (and so not much value for keeping
    // it).
    if (ccRole == ReplicaRole::None && (HasPersistedState || pcRole <= ReplicaRole::Idle))
    {
        if ((inBuildReplica.IsReportedByPrimary || inBuildReplica.IsReportedBySecondary) &&
            replica.CurrentConfigurationRole != ReplicaRole::Primary)
        {
            ccRole = replica.CurrentConfigurationRole;
        }
        else
        {
            ccRole = ReplicaRole::Idle;
        }
    }

    if (pc_ != nullptr && !pc_->IsEmpty() && pcRole == ReplicaRole::None && ccRole != ReplicaRole::None)
    {
        pcRole = ReplicaRole::Idle;
    }

    ReplicaFlags::Enum flags = ReplicaFlags::None;
    if (isPrimaryAvailable_ && inBuildReplica.CCEpoch > cc_->Epoch)
    {
        ASSERT_IF(replica.State != ReplicaStates::StandBy,
            "Replica {0} has higher epoch than CC for {1}",
            replica, *this);

        flags = ReplicaFlags::ToBeDroppedByFM;
    }

    if (!replica.IsUp &&
        ccRole < ReplicaRole::Secondary && 
        pcRole < ReplicaRole::Secondary && 
        (inBuildReplica.IsReportedByPrimary || inBuildReplica.IsReportedBySecondary))
    {
        flags = static_cast<ReplicaFlags::Enum>(flags ^ ReplicaFlags::PendingRemove);
    }

    replica.CurrentConfigurationRole = ccRole;
    replica.PreviousConfigurationRole = pcRole;
    replica.State = state;

    failoverUnit.AddReplica(
        replica,
        inBuildReplica.ServiceUpdateVersion,
        flags,
        PersistenceState::ToBeInserted,
        DateTime::Now());
}

size_t CalculateWriteQuorum(size_t total)
{
    return (total / 2) + 1;
}

FailoverUnitUPtr InBuildFailoverUnit::Generate(NodeCache const& nodeCache, bool forceRecovery)
{
    if (isToBeDeleted_)
    {
        return nullptr;
    }

    if (isCCActivated_ && pc_ && pcEpochByReadyPrimary_ != pc_->Epoch)
    {
        // Clear PC
        pc_ = nullptr;
    }

    ServiceInfoSPtr serviceInfo = make_shared<ServiceInfo>(serviceDescription_, nullptr, FABRIC_INVALID_SEQUENCE_NUMBER, false);
    serviceInfo->IsStale = true;

    // Construct an empty FailoverUnit.
    FailoverUnitUPtr failoverUnit = make_unique<FailoverUnit>(
        failoverUnitId_,
        consistencyUnitDescription_,
        serviceDescription_.IsStateful,
        serviceDescription_.HasPersistedState,
        serviceDescription_.Name,
        serviceInfo);

    if (!IsStateful)
    {
        failoverUnit->CurrentConfigurationEpoch = maxEpoch_;

        for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
        {
            failoverUnit->AddReplica(
                replica->Description,
                replica->ServiceUpdateVersion,
                ReplicaFlags::None,
                PersistenceState::ToBeInserted,
                DateTime::Now());
        }

        return failoverUnit;
    }

    if (HasPersistedState && !cc_ && !forceRecovery)
    {
        return nullptr;
    }

    // Add replicas to the FailoverUnit.
    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        GenerateReplica(*failoverUnit, *replica);
    }

    // If we have received a report from a node, and a replica hosted on that node
    // is not in the report, then the replica must be Dropped. We should Drop these
    // replicas so that the we do not unnecessarily get stuck when the primary is
    // not available.
    for (auto replica = replicas_.begin(); replica != replicas_.end(); ++replica)
    {
        if (!replica->IsSelfReported)
        {
            NodeInfoSPtr nodeInfo = nodeCache.GetNode(replica->Description.FederationNodeId);
            if (nodeInfo && (nodeInfo->IsReplicaUploaded || nodeInfo->IsNodeStateRemoved))
            {
                replica->Description.State = ReplicaStates::Dropped;

				auto & r = *failoverUnit->GetReplica(replica->Description.FederationNodeId);
                failoverUnit->OnReplicaDropped(r);
				r.IsDeleted = true;
            }
        }
    }

    bool ignoreQuorum = forceRecovery && DropOfflineReplicas(*failoverUnit, nodeCache);

    failoverUnit->NoData = !failoverUnit->IsStateful ||
                           (!failoverUnit->CurrentConfiguration.IsEmpty &&
                            static_cast<int>(failoverUnit->CurrentConfiguration.ReplicaCount) < failoverUnit->MinReplicaSetSize);

    // Update FailoverUnit Epochs
    failoverUnit->CurrentConfigurationEpoch = (cc_ ? cc_->Epoch : maxEpoch_);
    failoverUnit->PreviousConfigurationEpoch = (pc_ ? pc_->Epoch : Epoch::InvalidEpoch());

    // Determine if rebuild is stuck because primary not available and
    // we do not have a quorum of up replicas corresponding to the CC.
    if (HasPersistedState && !isPrimaryAvailable_ && !isSecondaryAvailable_ && !ignoreQuorum)
    {
        size_t reportedCount = GetReportedCount(failoverUnit->CurrentConfigurationEpoch, failoverUnit->CurrentConfiguration);
        size_t totalCount = failoverUnit->CurrentConfiguration.ReplicaCount;

        if (reportedCount < CalculateWriteQuorum(totalCount))
        {
            // We do not have enough information to determine if the CC epoch is correct.
            // Wait for more reports.
            return nullptr;
        }
    }

    // Determine if rebuild is stuck because primary not available and
    // we do not have a quorum of up replicas corresponding to the PC.
    if (HasPersistedState && !isCCActivated_ && !isPrimaryAvailable_ && !isSecondaryAvailable_ && !ignoreQuorum && failoverUnit->IsInReconfiguration)
    {
        size_t reportedCount = GetReportedCount(failoverUnit->PreviousConfigurationEpoch, failoverUnit->PreviousConfiguration);
        size_t totalCount = failoverUnit->PreviousConfiguration.ReplicaCount;

        if (reportedCount < CalculateWriteQuorum(totalCount))
        {
            // We do not have enough information to determine if the PC epoch is correct.
            // Wait for more reports.
            return nullptr;
        }
    }

    // Update the FailoverUnit Flags
    failoverUnit->IsSwappingPrimary = isSwapPrimary_;

    for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; ++replica)
    {
        if (!replica->IsUp)
        {
            failoverUnit->OnReplicaDown(*replica, !HasPersistedState);
        }
    }

    if (!isPrimaryAvailable_)
    {
        if (!failoverUnit->CurrentConfiguration.IsEmpty)
        {
            failoverUnit->OnPrimaryDown();
        }

        LockedFailoverUnitPtr lockedFailoverUnit;
        ReconfigurationTask reconfigruationTask(nullptr);
        reconfigruationTask.ReconfigPrimary(lockedFailoverUnit, *failoverUnit);

        Epoch highEpoch(max(maxEpoch_.DataLossVersion, failoverUnit->CurrentConfigurationEpoch.DataLossVersion), maxEpoch_.ConfigurationVersion + PrimaryEpochIncrement * (Epoch::PrimaryEpochMask + 1) + 1);
        failoverUnit->CurrentConfigurationEpoch = highEpoch;
    }

    if (failoverUnit->IsQuorumLost())
    {
        failoverUnit->SetQuorumLossTime(DateTime::Now().Ticks);
    }

    if (forceRecovery)
    {
        failoverUnit->UpdateEpochForDataLoss();
    }

    return failoverUnit;
}

bool InBuildFailoverUnit::GenerateDeleteReplicaActions(NodeCache const& nodeCache, vector<StateMachineActionUPtr> & actions) const
{
    bool canDelete = true;

    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (!it->IsDeleted && it->IsSelfReported)
        {
            canDelete = false;

            NodeInfoSPtr nodeInfo = nodeCache.GetNode(it->Description.FederationNodeId);
            if (nodeInfo && nodeInfo->IsUp)
            {
                FailoverUnitDescription desc(
                    failoverUnitId_,
                    consistencyUnitDescription_,
                    maxEpoch_,
                    Epoch::InvalidEpoch());

                if (HasPersistedState)
                {
                    actions.push_back(make_unique<DeleteReplicaAction>(
                        it->Description.FederationNodeInstance, 
                        DeleteReplicaMessageBody(desc, it->Description, serviceDescription_)));
                }
                else
                {
                    actions.push_back(make_unique<RemoveInstanceAction>(
                        it->Description.FederationNodeInstance, 
                        DeleteReplicaMessageBody(desc, it->Description, serviceDescription_)));
                }
            }
        }
    }

    return canDelete;
}

bool InBuildFailoverUnit::IsConfigurationReported(Epoch const & epoch,  Federation::NodeId from) const
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->Description.FederationNodeId == from)
        {
            if (it->IsSelfReported)
            {
                Epoch replicaEpoch = (it->PCEpoch == Epoch::InvalidEpoch() ? it->CCEpoch : it->PCEpoch);
                return (replicaEpoch <= epoch);
            }

            return false;
        }
    }

    return false;
}

size_t InBuildFailoverUnit::GetReportedCount(Epoch const & epoch, FailoverUnitConfiguration const& configuration) const
{
    size_t count = 0;

    for (auto const& replica : configuration.Replicas)
    {
        if (IsConfigurationReported(epoch, replica->FederationNodeId))
        {
            count++;
        }
    }

    return count;
}

void InBuildFailoverUnit::PostUpdate(DateTime updatedTime)
{
    lastUpdated_ = updatedTime;
}

void InBuildFailoverUnit::PostRead(int64 operationLSN)
{
    StoreData::PostRead(operationLSN);

    isToBeDeleted_ = false;
}

void InBuildFailoverUnit::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    w.Write("{0}: {1} ", failoverUnitId_, serviceDescription_);

    if (isPrimaryAvailable_)
    {
        w.Write('P');
    }

    if (isSecondaryAvailable_)
    {
        w.Write('S');
    }

    if (isSwapPrimary_)
    {
        w.Write('W');
    }

    if (isToBeDeleted_)
    {
        w.Write('D');
    }

    w.WriteLine(" {0} {1} {2}", healthSequence_, OperationLSN, lastUpdated_);

    if (cc_)
    {
        w.WriteLine("{0}CC: {1}", (isCCActivated_ ? "Activated" : ""), *cc_);
    }
    if (pc_)
    {
        w.WriteLine("PC: {0}", *pc_);
    }
    w.WriteLine("MaxEpoch: {0}", maxEpoch_);

    w.Write("Reports from Nodes:");
    for (NodeId const& nodeId : reportingNodes_)
    {
        w.Write(" {0}", nodeId);
    }
    w.WriteLine();

    w.WriteLine("Replicas:");
    for (InBuildReplica const& replica : replicas_)
    {
        w.WriteLine("  {0}", replica);
    }
}

InBuildReplica::InBuildReplica()
    : isReportedByPrimary_(false),
      isReportedBySecondary_(false),
      isDeleted_(false)
{
}

InBuildReplica::InBuildReplica(InBuildReplica && other)
    : replicaDescription_(move(other.replicaDescription_)),
      serviceUpdateVersion_(other.serviceUpdateVersion_),
      isReportedByPrimary_(other.isReportedByPrimary_),
      isReportedBySecondary_(other.isReportedBySecondary_),
      isDeleted_(other.isDeleted_),
      ccEpoch_(other.ccEpoch_),
      icEpoch_(other.icEpoch_),
      pcEpoch_(other.pcEpoch_)
{
}

InBuildReplica::InBuildReplica(
    ReplicaDescription const & replicaDescription,
    bool isReportedByPrimary,
    bool isReportedBySecondary)
    : replicaDescription_(replicaDescription),
      isReportedByPrimary_(isReportedByPrimary),
      isReportedBySecondary_(isReportedBySecondary),
      isDeleted_(false)
{
}

InBuildReplica & InBuildReplica::operator=(InBuildReplica && other)
{
    if (this != &other)
    {
        replicaDescription_ = move(other.replicaDescription_);
        serviceUpdateVersion_ = other.serviceUpdateVersion_;
        isReportedByPrimary_ = other.isReportedByPrimary_;
        isReportedBySecondary_ = other.isReportedBySecondary_;
        isDeleted_ = other.isDeleted_;
        ccEpoch_ = other.ccEpoch_;
        icEpoch_ = other.icEpoch_;
        pcEpoch_ = other.pcEpoch_;
    }

    return *this;
}

void InBuildReplica::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0}/{1}/{2} {3} {4} {5}", pcEpoch_, icEpoch_, ccEpoch_, isDeleted_ ? "Deleted" : "", replicaDescription_, serviceUpdateVersion_);
}

InBuildFailoverUnit::ReportedConfiguration::ReportedConfiguration()
    : primary_(),
      secondaries_(),
      isEmpty_(true)
{
}

InBuildFailoverUnit::ReportedConfiguration::ReportedConfiguration(FailoverUnitInfo const& report, NodeInstance const& from, Type type)
    : primary_(),
      secondaries_(),
      isEmpty_(true)
{
    switch (type)
    {
    case Type::CC:
        epoch_ = report.CCEpoch;
        break;

    case Type::IC:
        epoch_ = report.ICEpoch;
        break;

    case Type::PC:
        epoch_ = report.PCEpoch;
        break;
    }

    if (epoch_ != Epoch::InvalidEpoch())
    {
        for (ReplicaInfo const& replicaInfo : report.Replicas)
        {
            ReplicaDescription const& replica = replicaInfo.Description;

            ReplicaRole::Enum role = ReplicaRole::None;
            switch (type)
            {
            case Type::CC:
                role = replica.CurrentConfigurationRole;
                break;

            case Type::IC:
                role = replicaInfo.ICRole;
                break;

            case Type::PC:
                role = replica.PreviousConfigurationRole;
                break;
            }

            if (role == ReplicaRole::Primary)
            {
                TESTASSERT_IF(
                    !isEmpty_,
                    "Multiple primary replicas in the report from {0}: {1}", from, report);

                primary_ = replica.FederationNodeId;
                isEmpty_ = false;
            }
            else if (role == ReplicaRole::Secondary)
            {
                secondaries_.push_back(replica.FederationNodeId);
            }
        }
    }

    TESTASSERT_IFNOT(!isEmpty_ || (secondaries_.size() == 0 && type == Type::PC),
        "Report from {0} contains no primary: {1}",
        from, report);
}

bool InBuildFailoverUnit::ReportedConfiguration::IsPrimary(Federation::NodeId nodeId) const
{
    return (primary_ == nodeId);
}

bool InBuildFailoverUnit::ReportedConfiguration::IsSecondary(Federation::NodeId nodeId) const
{
    auto it = find(secondaries_.begin(), secondaries_.end(), nodeId);
    return (it != secondaries_.end());
}

bool InBuildFailoverUnit::ReportedConfiguration::Contains(Federation::NodeId nodeId) const
{
    return (IsPrimary(nodeId) || IsSecondary(nodeId));
}

bool InBuildFailoverUnit::ReportedConfiguration::Contains(ReportedConfiguration const& configuration) const
{
    if (!Contains(configuration.primary_))
    {
        return false;
    }

    for (NodeId nodeId : configuration.secondaries_)
    {
        if (!Contains(nodeId))
        {
            return false;
        }
    }

    return true;
}

void InBuildFailoverUnit::ReportedConfiguration::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    w.Write("{0} : ", epoch_, primary_);

    if (isEmpty_)
    {
        w.Write("empty:");
    }
    else
    {
        w.Write(primary_);
        for (NodeId const& nodeId : secondaries_)
        {
            w.Write(" {0}", nodeId);
        }
    }
}
