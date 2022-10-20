// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLB.h"
#include "PlacementParser.h"
#include "Reliability/LoadBalancing/PLBDiagnostics.h"
#include "Reliability/LoadBalancing/Searcher.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"
#include "Reliability/LoadBalancing/ServiceDomain.h"
#include "Utility.h"
#include "Client/ClientPointers.h"
#include "Client/INotificationClientSettings.h"
#include "Client/FabricClientInternalSettingsHolder.h"
#include "Client/HealthReportingComponent.h"
#include "Client/HealthReportingTransport.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;
using namespace Reliability::LoadBalancingComponent;

PLBEventSource const PLB::trace_(TraceTaskCodes::PLB);

PLB::PLB()
{
    settings_.RefreshSettings();
}

void PLB::Load(wstring const& fileName)
{
    PlacementParser parser(settings_);
    parser.Parse(fileName);

    pl_ = parser.CreatePlacement();

    Client::HealthReportingComponentSPtr healthClientPtr = nullptr;
    bool verboseBool = true;
    std::vector<Reliability::LoadBalancingComponent::Node> nodeTable;
    std::map<ServiceDomain::DomainId, ServiceDomain> serviceDomainTable;
    checker_ = make_unique<Checker>(&(*pl_), trace_, make_shared<PLBDiagnostics>(healthClientPtr, verboseBool, nodeTable, serviceDomainTable, trace_));
}

void PLB::RunCreation()
{
    if (pl_ == nullptr)
    {
        return;
    }

    PLBSchedulerAction action;
    action.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
    wstring domainId(L"defaultDomain");
    Score originalScore(
        pl_->TotalMetricCount,
        pl_->LBDomains,
        pl_->TotalReplicaCount,
        pl_->BalanceCheckerObj->FaultDomainLoads,
        pl_->BalanceCheckerObj->UpgradeDomainLoads,
        pl_->BalanceCheckerObj->ExistDefragMetric,
        pl_->BalanceCheckerObj->ExistScopedDefragMetric,
        settings_,
        &pl_->BalanceCheckerObj->DynamicNodeLoads);

    trace_.PLBDomainStart(
        domainId,
        action,
        pl_->NodeCount,
        pl_->Services.size(),
        pl_->GetImbalancedServices(),
        pl_->PartitionCount,
        pl_->ExistingReplicaCount,
        pl_->NewReplicaCount,
        pl_->PartitionsInUpgradeCount,
        originalScore.AvgStdDev,
        pl_->QuorumBasedServicesCount,
        pl_->QuorumBasedPartitionsCount);
    trace_.PlacementDump(L"defaultDomain", *pl_);
    StopwatchTime domainStartTime = Stopwatch::Now();

    atomic_bool stopSearching;
    atomic_bool movementEnabled;

    Client::HealthReportingComponentSPtr healthClientPtr = nullptr;
    bool verboseBool = true;
    std::vector<Reliability::LoadBalancingComponent::Node> nodeTable;
    std::map<ServiceDomain::DomainId, ServiceDomain> serviceDomainTable;

    Searcher searcher(trace_, stopSearching, movementEnabled, make_shared<PLBDiagnostics>(healthClientPtr, verboseBool, nodeTable, serviceDomainTable, trace_), 0, 12345);

    CandidateSolution solution = searcher.SearchForSolution(action, *pl_, *checker_, domainId, false, UINT32_MAX);

    TimeSpan domainDelta = Stopwatch::Now() - domainStartTime;

    trace_.PLBDomainEnd(domainId, static_cast<int>(solution.ValidMoveCount), action, originalScore.AvgStdDev, solution.AvgStdDev, domainDelta.TotalMilliseconds());
}

void PLB::RunConstraintCheck()
{
    if (pl_ == nullptr)
    {
        return;
    }
}

void PLB::RunLoadBalancing()
{
    if (pl_ == nullptr)
    {
        return;
    }
}
