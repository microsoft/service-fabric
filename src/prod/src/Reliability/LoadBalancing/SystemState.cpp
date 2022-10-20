// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SystemState.h"
#include "PLBDiagnostics.h"
#include "Placement.h"
#include "Checker.h"
#include "PLBConfig.h"
#include "ServiceDomain.h"
#include "BalanceChecker.h"
#include "TempSolution.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

SystemState::SystemState(ServiceDomain & serviceDomain, PLBEventSource const& trace, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr)
    : serviceDomain_(serviceDomain),
    serviceCount_(serviceDomain.serviceTable_.size()),
    failoverUnitCount_(serviceDomain.failoverUnitTable_.size()),
    inTransitionPartitionCount_(static_cast<size_t>(serviceDomain.inTransitionPartitionCount_)),
    newReplicaCount_(static_cast<size_t>(serviceDomain.newReplicaCount_)),
    existingReplicaCount_(static_cast<size_t>(serviceDomain.existingReplicaCount_)),
    movableReplicaCount_(static_cast<size_t>(serviceDomain.movableReplicaCount_)),
    partitionsWithExtraReplicaCount_(serviceDomain.partitionsWithExtraReplicas_.size()),
    partitionsWithNewReplicaCount_(serviceDomain.partitionsWithNewReplicas_.size()),
    partitionsWithInstOnEveryNodeCount_(serviceDomain.partitionsWithInstOnEveryNode_.size()),
    partitionClosure_(nullptr),
    balanceChecker_(nullptr),
    placement_(nullptr),
    checker_(nullptr),
    avgStdDev_(-1),
    upgradePartitionCount_(serviceDomain.partitionsWithInUpgradeReplicas_.size()),
    trace_(trace),
    plbDiagnosticsSPtr_(plbDiagnosticsSPtr)
{
}

SystemState::SystemState(SystemState && other)
    : serviceDomain_(other.serviceDomain_),
    serviceCount_(other.serviceCount_),
    failoverUnitCount_(other.failoverUnitCount_),
    inTransitionPartitionCount_(other.inTransitionPartitionCount_),
    newReplicaCount_(other.newReplicaCount_),
    existingReplicaCount_(other.existingReplicaCount_),
    movableReplicaCount_(other.movableReplicaCount_),
    partitionsWithExtraReplicaCount_(other.partitionsWithExtraReplicaCount_),
    partitionsWithNewReplicaCount_(other.partitionsWithNewReplicaCount_),
    partitionsWithInstOnEveryNodeCount_(other.partitionsWithInstOnEveryNodeCount_),
    partitionClosure_(move(other.partitionClosure_)),
    balanceChecker_(move(other.balanceChecker_)),
    placement_(move(other.placement_)),
    checker_(move(other.checker_)),
    avgStdDev_(other.avgStdDev_),
    upgradePartitionCount_(other.upgradePartitionCount_),
    trace_(other.trace_),
    plbDiagnosticsSPtr_(move(other.plbDiagnosticsSPtr_))
{
}

SystemState::~SystemState()
{
}

bool SystemState::HasNewReplica() const
{
    return newReplicaCount_ > 0;
}

bool SystemState::HasPartitionWithReplicaInUpgrade() const
{
    return upgradePartitionCount_ > 0;
}

bool SystemState::HasMovableReplica() const
{
    return movableReplicaCount_ > 0;
}

bool SystemState::HasExtraReplicas() const
{
    return partitionsWithExtraReplicaCount_ > 0;
}

bool SystemState::HasPartitionInAppUpgrade() const 
{ 
    return serviceDomain_.PartitionsInAppUpgradeCount > 0;
}

size_t SystemState::BalancingMovementCount() const
{
    return serviceDomain_.BalancingMovementCount();
}

size_t SystemState::PlacementMovementCount() const
{
    return serviceDomain_.PlacementMovementCount();
}

bool SystemState::IsConstraintSatisfied(__out std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData) const
{
    PlacementUPtr const& pl = GetPlacement(PartitionClosureType::ConstraintCheck);
    bool ret = true;
    if (pl->TotalReplicaCount > 0u)
    {
        CandidateSolution solution(&(*pl), vector<Movement>(), vector<Movement>(), PLBSchedulerActionType::ConstraintCheck);
        TempSolution tempSolution(solution);
        set<Guid> invalidPartitions = checker_->CheckSolutionForInvalidPartitions(
            tempSolution, 
            Checker::DiagnosticsOption::ConstraintCheck,
            constraintsDiagnosticsData,
            pl->Random);

        if (!invalidPartitions.empty())
        {
            ret = false;
        }

        serviceDomain_.UpdateContraintCheckClosure(move(invalidPartitions));
    }

    return ret;
}

bool SystemState::IsBalanced() const
{
    return GetBalanceChecker(PartitionClosureType::Full)->IsBalanced;
}

set<Guid> SystemState::GetImbalancedFailoverUnits() const
{
    set<Guid> imbalanced;

    PlacementUPtr const & placement = GetPlacement(PartitionClosureType::Full);

    for (size_t i = 0; i < placement->PartitionCount; i++)
    {
        auto & partition = placement->SelectPartition(i);
        if (!partition.Service->IsBalanced)
        {
            imbalanced.insert(partition.PartitionId);
        }
    }

    return imbalanced;
}

double SystemState::GetAvgStdDev() const
{
    if (avgStdDev_ < 0)
    {
        BalanceCheckerUPtr const& bc = GetBalanceChecker(PartitionClosureType::Full);
        Score score(
            bc->TotalMetricCount,
            bc->LBDomains,
            0,
            bc->FaultDomainLoads,
            bc->UpgradeDomainLoads,
            bc->ExistDefragMetric,
            bc->ExistScopedDefragMetric,
            bc->Settings,
            &bc->DynamicNodeLoads);

        avgStdDev_ = score.AvgStdDev;

        if (!bc->ExistDefragMetric)
        {
            ASSERT_IF(avgStdDev_ < 0, "Invalid AvgStdDev:{0}", avgStdDev_);
        }
    }

    return avgStdDev_;
}

void SystemState::CreatePlacementAndChecker(PartitionClosureType::Enum closureType) const
{
    GetPlacement(closureType);
}

void SystemState::CreatePlacementAndChecker(PLBSchedulerActionType::Enum action) const
{
    GetPlacement(PartitionClosureType::FromPLBSchedulerAction(action));
    if (placement_->Action != action)
    {
        // It is possible that placement was created with another action, so checker needs to be updated.
        placement_->UpdateAction(action);
        checker_ = make_unique<Checker>(&(*placement_), trace_, plbDiagnosticsSPtr_);
    }
}

/////////////////////////////////////////////////////////
// private members
/////////////////////////////////////////////////////////

BalanceCheckerUPtr const& SystemState::GetBalanceChecker(PartitionClosureType::Enum closureType) const
{
    if (partitionClosure_ != nullptr && partitionClosure_->Type != closureType)
    {
        ClearAll();
    }

    if (placement_ != nullptr)
    {
        return placement_->BalanceCheckerObj;
    }
    else if (balanceChecker_ != nullptr)
    {
        return balanceChecker_;
    }
    else
    {
        if (partitionClosure_ == nullptr)
        {
            partitionClosure_ = serviceDomain_.GetPartitionClosure(closureType);
        }
        balanceChecker_ = serviceDomain_.GetBalanceChecker(partitionClosure_);
        return balanceChecker_;
    }
}

PlacementUPtr const& SystemState::GetPlacement(PartitionClosureType::Enum closureType) const
{
    if (partitionClosure_ != nullptr && partitionClosure_->Type != closureType)
    {
        ClearAll();
    }

    if (placement_ == nullptr)
    {
        GetBalanceChecker(closureType);

        set<Guid> throttledPartitions =
            (closureType == PartitionClosureType::Full || closureType == PartitionClosureType::NewReplicaPlacementWithMove) ?
            serviceDomain_.Scheduler.GetMovementThrottledFailoverUnits() :
            set<Guid>();

        placement_ = serviceDomain_.GetPlacement(partitionClosure_, move(balanceChecker_), move(throttledPartitions));
        balanceChecker_ = nullptr;
        checker_ = make_unique<Checker>(&(*placement_), trace_, plbDiagnosticsSPtr_);
    }
    return placement_;
}

void SystemState::ClearAll() const
{
    partitionClosure_ = nullptr;
    balanceChecker_ = nullptr;
    placement_ = nullptr;
    checker_ = nullptr;
    avgStdDev_ = -1;
}
