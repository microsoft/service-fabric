// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestFM.h"
#include "Reliability/Failover/common/FailoverConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;
using namespace PlacementAndLoadBalancingUnitTest;

StringLiteral const PLBTestSource("PLBTest");

TestFM::TestFM()
{
    numberOfUpdatesFromPLB_ = 0;
}

void TestFM::ProcessFailoverUnitMovements(map<Guid, FailoverUnitMovement> && moveActions, Reliability::LoadBalancingComponent::DecisionToken && dToken)
{
    UNREFERENCED_PARAMETER(dToken);

    ++numberOfUpdatesFromPLB_;

    for (auto it = moveActions.begin(); it != moveActions.end(); ++it)
    {
        auto const& fuAction = (it->second);

        // limit the number of moves to print out
        if (moveActions.size() < PLBConfig::GetConfig().GlobalMovementThrottleThreshold)
        {
            for (FailoverUnitMovement::PLBAction const& action : fuAction.Actions)
            {
                Trace.WriteInfo(PLBTestSource, "FT {0} action: From {1} to {2} do {3}", fuAction.FailoverUnitId.AsGUID().Data1, GetNodeId(action.SourceNode), GetNodeId(action.TargetNode), action.Action);
            }
        }

        moveActions_.insert(make_pair(it->first, move(it->second)));
    }

}

void TestFM::UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const & appId)
{
    appUpgradeChecks_.insert(appId);
}

bool TestFM::IsSafeToUpgradeApp(ServiceModel::ApplicationIdentifier const& appId)
{
    if (appUpgradeChecks_.find(appId) == appUpgradeChecks_.end())
    {
        return false;
    }

    return true;
}

void TestFM::UpdateFailoverUnitTargetReplicaCount(Common::Guid const & partitionId, int targetCount)
{
    autoscalingTargetCount_[partitionId] = targetCount;
}

void TestFM::Load()
{
    Load(PLBConfig::GetConfig().InitialRandomSeed);
}

void TestFM::Load(int initialPLBSeed)
{
    int originalRandomSeed = PLBConfig::GetConfig().InitialRandomSeed;
    PLBConfig::GetConfig().InitialRandomSeed = initialPLBSeed;
    if (plbTestHelper_)
    {
        // Disposes PLB as well.
        plbTestHelper_->Dispose();
    }

    plb_ = make_unique<PlacementAndLoadBalancing>(
        *this, *this, false, true,
        vector<NodeDescription>(),
        vector<ApplicationDescription>(),
        vector<ServiceTypeDescription>(),
        vector<ServiceDescription>(),
        vector<FailoverUnitDescription>(),
        vector<LoadOrMoveCostDescription>(),
        CreateDummyHealthReportingComponentSPtr(),
        Api::IServiceManagementClientPtr(),
        Reliability::FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry);

    plbTestHelper_ = make_unique<PlacementAndLoadBalancingTestHelper>(*plb_);

    moveActions_.clear();
    fuMap_.clear();

    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        plbTestHelper_->UpdateApplication(ApplicationDescription(
            wstring(L"fabric:/NastyApplication314"),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            1000));

        plbTestHelper_->UpdateApplication(ApplicationDescription(
            wstring(L"DummyApp"),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            0));
    }
    PLBConfig::GetConfig().InitialRandomSeed = originalRandomSeed;
}

void TestFM::Clear()
{
    numberOfUpdatesFromPLB_ = 0;
    moveActions_.clear();
    fuMap_.clear();
    appUpgradeChecks_.clear();
    autoscalingTargetCount_.clear();
    autoscalingPartitionCountChange_.clear();
}

void TestFM::ClearMoveActions()
{
    moveActions_.clear();
}

std::vector<ReplicaDescription>::iterator TestFM::GetReplicaIterator(
    std::vector<Reliability::LoadBalancingComponent::ReplicaDescription> & replicas,
    Federation::NodeId const& node,
    Reliability::LoadBalancingComponent::ReplicaRole::Enum role)
{
    std::vector<Reliability::LoadBalancingComponent::ReplicaDescription>::iterator replica = replicas.end();
    for (auto replicaIt = replicas.begin(); replicaIt != replicas.end(); ++replicaIt)
    {
        if ((*replicaIt).CurrentRole == role && (*replicaIt).NodeId == node)
        {
            replica = replicaIt;
            break;
        }
    }

    return replica;
}

void TestFM::ApplySwapAction(
    Common::Guid fuId,
    Federation::NodeId const& sourceNode,
    Federation::NodeId const& targetNode)
{
    std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription>::iterator itFu = fuMap_.find(fuId);
    if (itFu == fuMap_.end())
    {
        Assert::CodingError("FailoverUnit {0} from action list doesn't exits in the FailoverUnit map", GetFUId(fuId));
    }

    FailoverUnitDescription const& fUnit = (*itFu).second;
    vector<ReplicaDescription> replicas(fUnit.Replicas);
    replicas.erase(GetReplicaIterator(replicas, sourceNode, ReplicaRole::Primary));
    replicas.erase(GetReplicaIterator(replicas, targetNode, ReplicaRole::Secondary));

    replicas.push_back(ReplicaDescription(
        CreateNodeInstance((int)targetNode.IdValue.Low, 0),
        ReplicaRole::Primary,
        Reliability::ReplicaStates::Ready,
        true)); // isUp

    replicas.push_back(ReplicaDescription(
        CreateNodeInstance((int)sourceNode.IdValue.Low, 0),
        ReplicaRole::Secondary,
        Reliability::ReplicaStates::Ready,
        true)); // isUp

    FailoverUnitDescription tempFu(fUnit);
    fuMap_.erase(tempFu.FUId);
    fuMap_.insert(make_pair(tempFu.FUId, FailoverUnitDescription(
        tempFu.FUId,
        wstring(tempFu.ServiceName),
        tempFu.Version + 1,
        move(replicas),
        0,          // replicaDifference
        Reliability::FailoverUnitFlags::Flags::None,
        false,      // isReconfigurationInProgress
        false)));   // isInQuorumLost
}

void TestFM::ApplyMoveAction(
    Common::Guid fuId,
    Federation::NodeId const& sourceNode,
    Federation::NodeId const& targetNode,
    Reliability::LoadBalancingComponent::ReplicaRole::Enum role)
{
    std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription>::iterator itFu = fuMap_.find(fuId);
    if (itFu == fuMap_.end())
    {
        Assert::CodingError("FailoverUnit {0} from action list doesn't exits in the FailoverUnit map", GetFUId(fuId));
    }

    FailoverUnitDescription const& fUnit = (*itFu).second;
    vector<ReplicaDescription> replicas(fUnit.Replicas);
    replicas.erase(GetReplicaIterator(replicas, sourceNode, role));

    replicas.push_back(ReplicaDescription(
        CreateNodeInstance((int)targetNode.IdValue.Low, 0),
        role,
        Reliability::ReplicaStates::Ready,
        true)); // isUp

    FailoverUnitDescription tempFu(fUnit);
    fuMap_.erase(tempFu.FUId);
    fuMap_.insert(make_pair(tempFu.FUId, FailoverUnitDescription(
        tempFu.FUId,
        wstring(tempFu.ServiceName),
        tempFu.Version + 1,
        move(replicas),
        0,          // replicaDifference
        Reliability::FailoverUnitFlags::Flags::None,
        false,      // isReconfigurationInProgress
        false)));   // isInQuorumLost
}

void TestFM::ApplyAddAction(
    Common::Guid fuId,
    Federation::NodeId const& targetNode,
    Reliability::LoadBalancingComponent::ReplicaRole::Enum role)
{
    std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription>::iterator itFu = fuMap_.find(fuId);
    if (itFu == fuMap_.end())
    {
        Assert::CodingError("FailoverUnit {0} from action list doesn't exits in the FailoverUnit map", GetFUId(fuId));
    }

    FailoverUnitDescription const& fUnit = (*itFu).second;
    vector<ReplicaDescription> replicas(fUnit.Replicas);
    replicas.push_back(ReplicaDescription(
        CreateNodeInstance((int)targetNode.IdValue.Low, 0),
        role,
        Reliability::ReplicaStates::Ready,
        true)); // isUp

    FailoverUnitDescription tempFu(fUnit);
    fuMap_.erase(tempFu.FUId);
    fuMap_.insert(make_pair(tempFu.FUId, FailoverUnitDescription(
        tempFu.FUId,
        wstring(tempFu.ServiceName),
        tempFu.Version + 1,
        move(replicas),
        ((tempFu.ReplicaDifference - 1) > 0) ? (tempFu.ReplicaDifference - 1) : 0,          // replicaDifference
        Reliability::FailoverUnitFlags::Flags::None,
        false,      // isReconfigurationInProgress
        false)));   // isInQuorumLost
}

void TestFM::AddReplica(Common::Guid fuId, int count)
{
    std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription>::iterator itFu = fuMap_.find(fuId);
    if (itFu == fuMap_.end())
    {
        Assert::CodingError("FailoverUnit {0} from action list doesn't exits in the FailoverUnit map", GetFUId(fuId));
    }

    FailoverUnitDescription const& fUnit = (*itFu).second;
    vector<ReplicaDescription> replicas(fUnit.Replicas);

    FailoverUnitDescription tempFu(fUnit);
    fuMap_.erase(tempFu.FUId);
    fuMap_.insert(make_pair(tempFu.FUId, FailoverUnitDescription(
        tempFu.FUId,
        wstring(tempFu.ServiceName),
        tempFu.Version + 1,
        move(replicas),
        (tempFu.ReplicaDifference + count),          // replicaDifference
        Reliability::FailoverUnitFlags::Flags::None,
        false,      // isReconfigurationInProgress
        false)));   // isInQuorumLost
}

void TestFM::ApplyDropAction(
    Common::Guid fuId,
    Federation::NodeId const& targetNode)
{
    std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitDescription>::iterator itFu = fuMap_.find(fuId);
    if (itFu == fuMap_.end())
    {
        Assert::CodingError("FailoverUnit {0} from action list doesn't exits in the FailoverUnit map", GetFUId(fuId));
    }

    FailoverUnitDescription const& fUnit = (*itFu).second;
    vector<ReplicaDescription> replicas(fUnit.Replicas);
    replicas.erase(GetReplicaIterator(replicas, targetNode, ReplicaRole::Secondary));

    FailoverUnitDescription tempFu(fUnit);
    fuMap_.erase(tempFu.FUId);
    fuMap_.insert(make_pair(tempFu.FUId, FailoverUnitDescription(
        tempFu.FUId,
        wstring(tempFu.ServiceName),
        tempFu.Version + 1,
        move(replicas),
        0,          // replicaDifference
        Reliability::FailoverUnitFlags::Flags::None,
        false,      // isReconfigurationInProgress
        false)));   // isInQuorumLost
}

void TestFM::ApplyActions()
{
    for (auto const& actionPair : moveActions_)
    {
        for (FailoverUnitMovement::PLBAction currentAction : actionPair.second.Actions)
        {
            switch (currentAction.Action)
            {
            case FailoverUnitMovementType::MoveSecondary:
            case FailoverUnitMovementType::MoveInstance:
                // move secondary/instance {0}=>{1}
                ApplyMoveAction(actionPair.first, currentAction.SourceNode, currentAction.TargetNode, ReplicaRole::Secondary);
                break;

            case FailoverUnitMovementType::MovePrimary:
                // move primary {0}=>{1}
                ApplyMoveAction(actionPair.first, currentAction.SourceNode, currentAction.TargetNode, ReplicaRole::Primary);
                break;

            case FailoverUnitMovementType::SwapPrimarySecondary:
                // swap primary {0}<=>{1}
                ApplySwapAction(actionPair.first, currentAction.SourceNode, currentAction.TargetNode);
                break;

            case FailoverUnitMovementType::AddPrimary:
                // add primary {0}
                ApplyAddAction(actionPair.first, currentAction.TargetNode, ReplicaRole::Primary);
                break;

            case FailoverUnitMovementType::AddSecondary:
            case FailoverUnitMovementType::AddInstance:
                // add secondary/instance
                ApplyAddAction(actionPair.first, currentAction.TargetNode, ReplicaRole::Secondary);
                break;

            case FailoverUnitMovementType::RequestedPlacementNotPossible:
                // void movement on {0}
                // TODO: clear flegs on the existing replica
                break;

            case FailoverUnitMovementType::DropPrimary:
            case FailoverUnitMovementType::DropSecondary:
            case FailoverUnitMovementType::DropInstance:
                // drop primary/secondary/instance
                ApplyDropAction(actionPair.first, currentAction.SourceNode);
                break;

            case FailoverUnitMovementType::PromoteSecondary:
                Assert::CodingError("PromoteSecondary used");
                break;

            default:
                Assert::CodingError("Unknown Replica Move Type");
                break;
            }
        }
    }

    moveActions_.clear();
}

void TestFM::UpdatePlb()
{
    for (auto it = fuMap_.begin(); it != fuMap_.end(); ++it)
    {
        plbTestHelper_->UpdateFailoverUnit(FailoverUnitDescription((*it).second));
    }

    plbTestHelper_->ProcessPendingUpdatesPeriodicTask();
}

void TestFM::RefreshPLB(Common::StopwatchTime now)
{
    plbTestHelper_->Refresh(now);
}

TestFM::DummyHealthReportingTransport::DummyHealthReportingTransport(ComponentRoot const& root)
    : Client::HealthReportingTransport(root)
{
}

AsyncOperationSPtr TestFM::DummyHealthReportingTransport::BeginReport(
    Transport::MessageUPtr && message,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(timeout);

    return make_shared<CompletedAsyncOperation>(ErrorCodeValue::OperationCanceled, callback, parent);
}

ErrorCode TestFM::DummyHealthReportingTransport::EndReport(
    AsyncOperationSPtr const& operation,
    __out Transport::MessageUPtr & reply)
{
    UNREFERENCED_PARAMETER(reply);

    return CompletedAsyncOperation::End(operation);
}

Client::HealthReportingComponentSPtr TestFM::CreateDummyHealthReportingComponentSPtr()
{
    auto root = make_shared<ComponentRoot>();
    auto healthTransport = make_unique<DummyHealthReportingTransport>(*root);
    auto healthClient = make_shared<Client::HealthReportingComponent>(*healthTransport, L"Dummy", false);
    return Client::HealthReportingComponentSPtr(healthClient);
}

int TestFM::GetTargetReplicaCountForPartition(Common::Guid const& partitionId)
{
    auto autoScaleCountIt = autoscalingTargetCount_.find(partitionId);
    if (autoScaleCountIt != autoscalingTargetCount_.end())
    {
        return autoScaleCountIt->second;
    }
    return -1;
}

int TestFM::GetPartitionCountChangeForService(std::wstring const & serviceName)
{
    return plbTestHelper_->GetPartitionCountChangeForService(serviceName);
}
