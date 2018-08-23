// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace Client;
    using namespace Federation;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;
    using namespace std;

    class TestStateMachineTask
    {
    protected:

        TestStateMachineTask() { BOOST_REQUIRE(ClassSetup()); }
        TEST_CLASS_SETUP(ClassSetup);

        ~TestStateMachineTask() { BOOST_REQUIRE(ClassCleanup()); }
        TEST_CLASS_CLEANUP(ClassCleanup);

        void CheckState(
            wstring const &testName,
            wstring inputFailoverUnitStr,
            wstring expectedOutputFailoverUnitStr,
            wstring expectedActions,
            int expectedReplicaDifference = 0);

        void CheckMessage(
            wstring const & testName,
            wstring const& msgAction,
            wstring msgBodyStr,
            wstring inputFailoverUnitStr,
            wstring expectedOutputFailoverUnitStr,
            wstring expectedActionsStr,
            wstring fromNode);

        void CheckPlbMovement(
            wstring const& testName,
            wstring movementStr,
            wstring inputFailoverUnitStr,
            wstring expectedOutputFailoverUnitStr,
            wstring expectedActionsStr,
            int expectedReplicaDifference);

        void CheckPlbAutoScale(
            wstring const& testName,
            wstring inputFailoverUnitStr,
            int expectedTargetCount,
            int expectedReplicaDifference
        );
        
        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;

        vector<StateMachineTaskUPtr> statelessTasks_;
        vector<StateMachineTaskUPtr> statefulTasks_;
    };

    void TestStateMachineTask::CheckState(
        wstring const & testName,
        wstring inputFailoverUnitStr,
        wstring expectedOutputFailoverUnitStr,
        wstring expectedActions,
        int expectedReplicaDifference)
    {
        Trace.WriteInfo("StateMachineTaskTestSource", "{0}", testName);

        // Remove extra spaces
        TestHelper::TrimExtraSpaces(inputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(inputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedOutputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedActions);

        FailoverUnitUPtr ft = TestHelper::FailoverUnitFromString(inputFailoverUnitStr);
        ft->PostUpdate(DateTime::Now());
        ft->PostCommit(ft->OperationLSN + 1);

        ServiceDescription serviceDescription = ft->ServiceInfoObj->ServiceDescription;

        FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*fm_, move(ft));
        bool isDeleted;
        entry->Lock(TimeSpan::Zero, true, isDeleted);
        LockedFailoverUnitPtr failoverUnit(entry);

        vector<StateMachineActionUPtr> actions;
        vector<StateMachineTaskUPtr> const & tasks = failoverUnit->IsStateful ? statefulTasks_ : statelessTasks_;

        failoverUnit->UpdatePointers(*fm_, fm_->NodeCacheObj, fm_->ServiceCacheObj);

        for (StateMachineTaskUPtr const& task : tasks)
        {
            task->CheckFailoverUnit(failoverUnit, actions);
        }

        wstring actualOutputFailoverUnitStr = TestHelper::FailoverUnitToString(failoverUnit.Current, serviceDescription);

        vector<wstring> expectedActionVector;
        StringUtility::Split<wstring>(expectedActions, expectedActionVector, L"|");
        sort(expectedActionVector.begin(), expectedActionVector.end());
        vector<wstring> actualActionVector = TestHelper::ActionsToString(actions);

        if (failoverUnit->IsQuorumLost())
        {
            actualActionVector.push_back(L"QuorumLost");
            sort(actualActionVector.begin(), actualActionVector.end());
        }

        int actualReplicaDifference = failoverUnit->ReplicaDifference;

        TestHelper::AssertEqual(expectedOutputFailoverUnitStr, actualOutputFailoverUnitStr, testName);
        TestHelper::AssertEqual(expectedActionVector, actualActionVector, testName);
        TestHelper::AssertEqual(expectedReplicaDifference, actualReplicaDifference, testName);

        if (inputFailoverUnitStr == expectedOutputFailoverUnitStr)
        {
            VERIFY_IS_TRUE((!failoverUnit.IsUpdating || actualReplicaDifference != 0) && failoverUnit->PersistenceState == PersistenceState::NoChange);
        }
        else
        {
            VERIFY_IS_TRUE(failoverUnit.IsUpdating && failoverUnit->PersistenceState != PersistenceState::NoChange);
        }
    }

    void TestStateMachineTask::CheckMessage(
        wstring const & testName,
        wstring const& msgAction,
        wstring msgBodyStr,
        wstring inputFailoverUnitStr,
        wstring expectedOutputFailoverUnitStr,
        wstring expectedActionsStr,
        wstring fromNode)
    {
        Trace.WriteInfo("StateMachineTaskTestSource", "{0}", testName);

        // Remove extra spaces
        TestHelper::TrimExtraSpaces(msgBodyStr);
        TestHelper::TrimExtraSpaces(inputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedOutputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedActionsStr);

        FailoverUnitUPtr fuTemp = TestHelper::FailoverUnitFromString(inputFailoverUnitStr);
        FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*fm_, move(fuTemp));
        bool isDeleted;
        entry->Lock(TimeSpan::Zero, true, isDeleted);
        LockedFailoverUnitPtr failoverUnit(entry);

        FailoverUnitUPtr msgFailoverUnit = TestHelper::FailoverUnitFromString(msgBodyStr);

        vector<ReplicaDescription> msgReplicaDescriptions;
        msgFailoverUnit->ForEachReplica([&](Replica& replica)
        {
            msgReplicaDescriptions.push_back(replica.ReplicaDescription);
        });

        NodeId fromNodeId;
        NodeId::TryParse(fromNode, fromNodeId);
        auto it = find_if(
            msgReplicaDescriptions.begin(),
            msgReplicaDescriptions.end(),
            [fromNodeId](ReplicaDescription const& replicaDescription) { return (replicaDescription.FederationNodeId == fromNodeId); });
        NodeInstance from = it->FederationNodeInstance;

        vector<StateMachineActionUPtr> actions;

        if (msgAction == RSMessage::GetAddInstanceReply().Action ||
            msgAction == RSMessage::GetRemoveInstanceReply().Action ||
            msgAction == RSMessage::GetAddPrimaryReply().Action ||
            msgAction == RSMessage::GetAddReplicaReply().Action ||
            msgAction == RSMessage::GetRemoveReplicaReply().Action ||
            msgAction == RSMessage::GetDeleteReplicaReply().Action)
        {
            VERIFY_ARE_EQUAL(msgReplicaDescriptions.size(), 1u);

            auto body = make_unique<ReplicaReplyMessageBody>(
                msgFailoverUnit->FailoverUnitDescription,
                move(msgReplicaDescriptions.front()),
                ErrorCode(ErrorCodeValue::Success));

            FailoverUnitMessageTask<ReplicaReplyMessageBody> task(msgAction, move(body), *fm_, from);
            task.CheckFailoverUnit(failoverUnit, actions);
        }
        else if (msgAction == RSMessage::GetDoReconfigurationReply().Action)
        {
            auto body = make_unique<ConfigurationReplyMessageBody>(
                msgFailoverUnit->FailoverUnitDescription,
                move(msgReplicaDescriptions),
                ErrorCode(ErrorCodeValue::Success));

            FailoverUnitMessageTask<ConfigurationReplyMessageBody> task(msgAction, move(body), *fm_, from);
            task.CheckFailoverUnit(failoverUnit, actions);
        }
        else if (msgAction == RSMessage::GetChangeConfiguration().Action)
        {
            auto body = make_unique<ConfigurationMessageBody>(
                msgFailoverUnit->FailoverUnitDescription,
                msgFailoverUnit->ServiceInfoObj->ServiceDescription,
                move(msgReplicaDescriptions));

            FailoverUnitMessageTask<ConfigurationMessageBody> task(msgAction, move(body), *fm_, from);
            task.CheckFailoverUnit(failoverUnit, actions);
        }

        wstring actualOutputFailoverUnitStr = TestHelper::FailoverUnitToString(failoverUnit.Current);
        vector<wstring> expectedActionsVector;
        StringUtility::Split<wstring>(expectedActionsStr, expectedActionsVector, L"|");
        sort(expectedActionsVector.begin(), expectedActionsVector.end());
        vector<wstring> actualActionsVector = TestHelper::ActionsToString(actions);

        TestHelper::AssertEqual(expectedOutputFailoverUnitStr, actualOutputFailoverUnitStr, testName);
        TestHelper::AssertEqual(expectedActionsVector, actualActionsVector, testName);
    }

    void TestStateMachineTask::CheckPlbMovement(
        wstring const& testName,
        wstring movementStr,
        wstring inputFailoverUnitStr,
        wstring expectedOutputFailoverUnitStr,
        wstring expectedActionsStr,
        int expectedReplicaDifference)
    {
        Trace.WriteInfo("StateMachineTaskTestSource", "{0}", testName);

        // Remove extra spaces
        TestHelper::TrimExtraSpaces(movementStr);
        TestHelper::TrimExtraSpaces(inputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedOutputFailoverUnitStr);
        TestHelper::TrimExtraSpaces(expectedActionsStr);

        FailoverUnitUPtr ft = TestHelper::FailoverUnitFromString(inputFailoverUnitStr);
        ft->PostUpdate(DateTime::Now());
        ft->PostCommit(ft->OperationLSN + 1);

        ServiceDescription serviceDescription = ft->ServiceInfoObj->ServiceDescription;

        FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*fm_, move(ft));
        bool isDeleted;
        entry->Lock(TimeSpan::Zero, true, isDeleted);
        LockedFailoverUnitPtr failoverUnit(entry);
        failoverUnit->UpdatePointers(*fm_, fm_->NodeCacheObj, fm_->ServiceCacheObj);

        failoverUnit->ForEachReplica([this](Replica const& replica) 
        {
            if (replica.NodeInfoObj->IsUp)
            {
                NodeInfoSPtr nodeInfo = replica.NodeInfoObj;

                FabricVersionInstance versionInstance;
                fm_->NodeCacheObj.NodeUp(move(nodeInfo), true /* IsVersionGatekeepingNeeded */, versionInstance);
            }
            else
            {
                fm_->NodeCacheObj.NodeDown(replica.NodeInfoObj->NodeInstance);
            }
        });

        vector<wstring> plbActionsStr;
        StringUtility::Split<wstring>(movementStr, plbActionsStr, L"[");

        vector<LoadBalancingComponent::FailoverUnitMovement::PLBAction> plbActions;
        for (size_t i = 0; i < plbActionsStr.size(); i++)
        {
            StringUtility::TrimSpaces(plbActionsStr[i]);
            StringUtility::TrimTrailing<wstring>(plbActionsStr[i], L"]");

            vector<wstring> tokens;
            StringUtility::Split<wstring>(plbActionsStr[i], tokens, L" ");

            LoadBalancingComponent::FailoverUnitMovementType::Enum plbActionType = TestHelper::PlbMovementActionTypeFromString(tokens[0]);
            NodeId sourceNodeId = TestHelper::CreateNodeId(Int32_Parse(tokens[1]));
            NodeId targetNodeId = TestHelper::CreateNodeId(Int32_Parse(tokens[2]));

            FabricVersionInstance versionInstance;
            fm_->NodeCacheObj.NodeUp(TestHelper::CreateNodeInfo(NodeInstance(sourceNodeId, 1)), true /* IsVersionGatekeepingNeeded */, versionInstance);
            fm_->NodeCacheObj.NodeUp(TestHelper::CreateNodeInfo(NodeInstance(targetNodeId, 1)), true /* IsVersionGatekeepingNeeded */, versionInstance);

            LoadBalancingComponent::FailoverUnitMovement::PLBAction plbAction(
                sourceNodeId,
                targetNodeId,
                plbActionType,
                Reliability::LoadBalancingComponent::PLBSchedulerActionType::None);

            plbActions.push_back(move(plbAction));
        }

        // Get the movement
        wstring serviceName = failoverUnit->ServiceName;
        LoadBalancingComponent::FailoverUnitMovement movement(
            failoverUnit->Id.Guid,
            move(serviceName),
            failoverUnit->IsStateful,
            failoverUnit->UpdateVersion,
            false, // IsFailoverUnitInTransition
            move(plbActions));

        vector<StateMachineActionUPtr> actions;

        DynamicStateMachineTaskUPtr movementTask(
            new MovementTask(*fm_, fm_->NodeCacheObj, move(movement), move(LoadBalancingComponent::DecisionToken(Guid::Empty(), 0))));
        movementTask->CheckFailoverUnit(failoverUnit, actions);

        vector<StateMachineTaskUPtr> const& tasks = failoverUnit->IsStateful ? statefulTasks_ : statelessTasks_;
        for (StateMachineTaskUPtr const& task : tasks)
        {
            task->CheckFailoverUnit(failoverUnit, actions);
        }

        wstring actualOutputFailoverUnitStr = TestHelper::FailoverUnitToString(failoverUnit.Current, serviceDescription);

        vector<wstring> expectedActionVector;
        StringUtility::Split<wstring>(expectedActionsStr, expectedActionVector, L"|");
        sort(expectedActionVector.begin(), expectedActionVector.end());
        vector<wstring> actualActionVector = TestHelper::ActionsToString(actions);

        if (failoverUnit->IsQuorumLost())
        {
            actualActionVector.push_back(L"QuorumLost");
            sort(actualActionVector.begin(), actualActionVector.end());
        }

        int actualReplicaDifference = failoverUnit->ReplicaDifference;

        TestHelper::AssertEqual(expectedOutputFailoverUnitStr, actualOutputFailoverUnitStr, testName);
        TestHelper::AssertEqual(expectedActionVector, actualActionVector, testName);
        TestHelper::AssertEqual(expectedReplicaDifference, actualReplicaDifference, testName);

        if (inputFailoverUnitStr == expectedOutputFailoverUnitStr)
        {
            VERIFY_IS_TRUE((!failoverUnit.IsUpdating || actualReplicaDifference != 0) && failoverUnit->PersistenceState == PersistenceState::NoChange);
        }
        else
        {
            VERIFY_IS_TRUE(failoverUnit.IsUpdating && failoverUnit->PersistenceState != PersistenceState::NoChange);
        }
    }

    void TestStateMachineTask::CheckPlbAutoScale(
        wstring const& testName,
        wstring inputFailoverUnitStr,
        int expectedTargetCount,
        int expectedReplicaDifference)
    {
        Trace.WriteInfo("StateMachineTaskTestSource", "{0}", testName);

        // Remove extra spaces
        TestHelper::TrimExtraSpaces(inputFailoverUnitStr);

        //create a scaling policy
        vector<ServiceScalingPolicyDescription> scalingPolicies;
        ScalingMechanismSPtr mechanism = make_shared<Reliability::InstanceCountScalingMechanism>(1, 2, 1);
        ScalingTriggerSPtr trigger = make_shared<Reliability::AveragePartitionLoadScalingTrigger>(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 100, 200, 40);
        scalingPolicies.push_back(ServiceScalingPolicyDescription(trigger, mechanism));

        FailoverUnitUPtr ft = TestHelper::FailoverUnitFromString(inputFailoverUnitStr, scalingPolicies);
        ft->PostUpdate(DateTime::Now());
        ft->PostCommit(ft->OperationLSN + 1);

        ServiceDescription serviceDescription = ft->ServiceInfoObj->ServiceDescription;

        FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*fm_, move(ft));
        bool isDeleted;
        entry->Lock(TimeSpan::Zero, true, isDeleted);
        LockedFailoverUnitPtr failoverUnit(entry);
        failoverUnit->UpdatePointers(*fm_, fm_->NodeCacheObj, fm_->ServiceCacheObj);

        vector<StateMachineActionUPtr> actions;

        DynamicStateMachineTaskUPtr autoscaleTask(
            new AutoScalingTask(expectedTargetCount));

        autoscaleTask->CheckFailoverUnit(failoverUnit, actions);

        vector<StateMachineTaskUPtr> const& tasks = failoverUnit->IsStateful ? statefulTasks_ : statelessTasks_;
        for (StateMachineTaskUPtr const& task : tasks)
        {
            task->CheckFailoverUnit(failoverUnit, actions);
        }

        int actualReplicaDifference = failoverUnit.Current->ReplicaDifference;

        TestHelper::AssertEqual(expectedReplicaDifference, actualReplicaDifference, testName);
        TestHelper::AssertEqual(failoverUnit.Current->TargetReplicaSetSize, expectedTargetCount, testName);
    }

    BOOST_FIXTURE_TEST_SUITE(TestStateMachineTaskSuite,TestStateMachineTask)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        // test if the text parser works
        ServiceModel::ApplicationIdentifier appId;
        ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
        ApplicationInfoSPtr applicationInfo = make_shared<ApplicationInfo>(appId, NamingUri(L"fabric:/TestApp"), 1);
        ApplicationEntrySPtr applicationEntry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
        wstring serviceTypeName = L"TestServiceType";
        ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);
        wstring placementConstraints = L"";
        int scaleoutCount = 0;
        auto serviceType = make_shared<ServiceType>(typeId, applicationEntry);
        ServiceInfoSPtr serviceInfo = make_shared<ServiceInfo>(
            ServiceDescription(L"TestService", 0, 0, 1, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>()),
            serviceType,
            FABRIC_INVALID_SEQUENCE_NUMBER,
            false);

        FailoverUnitUPtr failoverUnit1 = FailoverUnitUPtr(
            new FailoverUnit(
                FailoverUnitId(),
                ConsistencyUnitDescription(),
                serviceInfo,
                FailoverUnitFlags::Flags(true, true),
                Epoch(1, 1),
                Epoch(1, 1),
                0, // LookupVersion
                DateTime::Now(), // LastUpdated
                0, // UpdateVersion
                0, // OperationLSN
                PersistenceState::NoChange));

        NodeInstance nodeInstance = TestHelper::CreateNodeInstance(0, 1);
        NodeInfoSPtr nodeInfo = make_shared<NodeInfo>(nodeInstance, NodeDescription(), true, true, false, DateTime::Now());

        failoverUnit1->AddReplica(
            move(nodeInfo),
            0, // ReplicaId
            1, // InstanceId
            ReplicaStates::Ready,
            ReplicaFlags::None,
            Reliability::ReplicaRole::Idle,
            Reliability::ReplicaRole::Primary,
            true, // IsUp
            serviceInfo->ServiceDescription.UpdateVersion, 
            ServiceModel::ServicePackageVersionInstance(), 
            PersistenceState::NoChange, 
            DateTime::Now()); // LastUpdated

        failoverUnit1->PostUpdate(DateTime::Now());
        failoverUnit1->PostCommit(failoverUnit1->OperationLSN + 1);

        wstring str = TestHelper::FailoverUnitToString(failoverUnit1);

        FailoverUnitUPtr failoverUnit2 = TestHelper::FailoverUnitFromString(str);

        failoverUnit2->VerifyConsistency();

        VERIFY_ARE_EQUAL(failoverUnit1->ServiceInfoObj->Name, failoverUnit2->ServiceInfoObj->Name);
        VERIFY_ARE_EQUAL(failoverUnit1->ServiceInfoObj->ServiceDescription.TargetReplicaSetSize, failoverUnit2->ServiceInfoObj->ServiceDescription.TargetReplicaSetSize);
        VERIFY_ARE_EQUAL(failoverUnit1->ServiceInfoObj->ServiceDescription.MinReplicaSetSize, failoverUnit2->ServiceInfoObj->ServiceDescription.MinReplicaSetSize);
        VERIFY_ARE_EQUAL(failoverUnit1->TargetReplicaSetSize, failoverUnit2->TargetReplicaSetSize);
        VERIFY_ARE_EQUAL(failoverUnit1->MinReplicaSetSize, failoverUnit2->MinReplicaSetSize);
        VERIFY_ARE_EQUAL(failoverUnit1->FailoverUnitFlags.Value, failoverUnit2->FailoverUnitFlags.Value);
        VERIFY_ARE_EQUAL(failoverUnit1->PreviousConfigurationEpoch, failoverUnit2->PreviousConfigurationEpoch);
        VERIFY_ARE_EQUAL(failoverUnit1->CurrentConfigurationEpoch, failoverUnit2->CurrentConfigurationEpoch);
        VERIFY_ARE_EQUAL(failoverUnit1->UpReplicaCount, failoverUnit2->UpReplicaCount);

        Replica const* replica1 = failoverUnit1->GetReplica(nodeInstance.Id);
        Replica const* replica2 = failoverUnit2->GetReplica(nodeInstance.Id);

        VERIFY_ARE_EQUAL(replica1->FederationNodeInstance, replica2->FederationNodeInstance);
        VERIFY_ARE_EQUAL(replica1->ReplicaId, replica2->ReplicaId);
        VERIFY_ARE_EQUAL(replica1->Flags, replica2->Flags);
        VERIFY_ARE_EQUAL(replica1->PreviousConfigurationRole, replica2->PreviousConfigurationRole);
        VERIFY_ARE_EQUAL(replica1->CurrentConfigurationRole, replica2->CurrentConfigurationRole);
        VERIFY_ARE_EQUAL(replica1->NodeInfoObj->IsUp, replica2->NodeInfoObj->IsUp);
    }

    BOOST_AUTO_TEST_CASE(StatelessTest)
    {
        CheckState(
            L"Stateless: Empty FailoverUnit",
            L"3 0 - 000/111",
            L"3 0 - 000/111",
            L"",
            3);

        CheckState(
            L"Stateless: One extra RD replica",
            L"3 0 - 000/111 [0 N/N RD R Up] [1 N/N IB - Up] [2 N/N RD - Up] [3 N/N DD - Down]",
            L"3 0 - 000/111 [0 N/N RD R Up] [1 N/N IB - Up] [2 N/N RD - Up] [3 N/N DD - Down]",
            L"AddInstance->1 [1 N/N]|RemoveInstance->0 [0 N/N]|RemoveInstance->3 [3 N/N]",
            1);

        CheckState(
            L"Stateless: One extra IB replica",
            L"2 0 - 000/111 [0 N/N RD - Up] [1 N/N RD - Up] [2 N/N IB - Up]",
            L"2 0 - 000/111 [0 N/N RD - Up] [1 N/N RD - Up] [2 N/N IB - Up]",
            L"AddInstance->2 [2 N/N]",
            -1);

        CheckState(
            L"Stateless: Initial placement for 3 instances",
            L"3 0 - 000/111 [0 N/N IB - Up] [1 N/N IB - Up] [2 N/N IB - Up]",
            L"3 0 - 000/111 [0 N/N IB - Up] [1 N/N IB - Up] [2 N/N IB - Up]",
            L"AddInstance->0 [0 N/N]|AddInstance->1 [1 N/N]|AddInstance->2 [2 N/N]");

        CheckMessage(
            L"Stateless: AddInstanceReply message processing for adding a new replica instance",
            RSMessage::GetAddInstanceReply().Action,
            L"3 0 - 000/111                 [1 N/N RD - Up]",
            L"3 0 - 000/111 [0 N/N IB - Up] [1 N/N IB - Up] [2 N/N IB - Up]",
            L"3 0 - 000/111 [0 N/N IB - Up] [1 N/N RD - Up] [2 N/N IB - Up]",
            L"",
            L"1");

        CheckState(
            L"Stateless: One node failure",
            L"3 0 - 000/111 [0 N/N RD - Up] [1:0 N/N RD - Up  ] [2 N/N RD - Up]",
            L"3 0 - 000/111 [0 N/N RD - Up] [1:0 N/N DD - Down] [2 N/N RD - Up]",
            L"",
            1);

        CheckState(
            L"Stateless: FailoverUnit is marked as ToBeDeleted",
            L"3 0 D 000/111 [0 N/N RD R Up] [1 N/N RD R Up] [2 N/N RD R Up]",
            L"3 0 D 000/111 [0 N/N RD R Up] [1 N/N RD R Up] [2 N/N RD R Up]",
            L"RemoveInstance->0 [0 N/N]|RemoveInstance->1 [1 N/N]|RemoveInstance->2 [2 N/N]");

        CheckMessage(
            L"Stateless: RemoveInstanceReply message processing for removing a replica instance",
            RSMessage::GetRemoveInstanceReply().Action,
            L"2 0 D 000/111                 [2 N/N DD -  Down]",
            L"2 0 D 000/111 [1 N/N RD R Up] [2 N/N RD RD Up  ]",
            L"2 0 D 000/111 [1 N/N RD R Up] [2 N/N DD Z  Down]",
            L"",
            L"2");

        CheckState(
            L"Stateless: All Replicas are Dropped",
            L"2 0 D  000/111 [1 N/N DD - Down] [2 N/N DD - Down]",
            L"2 0 DO 000/111 [1 N/N DD - Down] [2 N/N DD - Down]",
            L"DeleteService|RemoveInstance->1 [1 N/N]|RemoveInstance->2 [2 N/N]");

        CheckState(
            L"Stateless: All Replicas are Deleted",
            L"2 0 D  000/111 [1 N/N DD Z Down] [2 N/N DD Z Down]",
            L"2 0 DO 000/111 [1 N/N DD Z Down] [2 N/N DD Z Down]",
            L"DeleteService");

        CheckState(
            L"Stateless: Replica move with IB replica",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD V Up] [4 N/N IB C Up]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD V Up] [4 N/N IB C Up]",
            L"AddInstance->4 [4 N/N]");

        CheckState(
            L"Stateless: Replica move with RD replica",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD V Up] [4 N/N RD - Up]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD V Up] [4 N/N RD - Up]",
            L"",
            -1);

        CheckState(
            L"Stateless: Replica move with down IB replica",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD V Up] [4 N/N DD C Down]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD - Up] [4 N/N DD C Down]",
            L"RemoveInstance->4 [4 N/N]");

        CheckState(
            L"Stateless: To be moved replica node is down",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3:0 N/N RD V Up  ] [4 N/N IB C Up]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3:0 N/N DD - Down] [4 N/N IB C Up]",
            L"AddInstance->4 [4 N/N]");

        CheckState(
            L"Stateless: Two replica moves with one RD replica",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD V Up] [3 N/N RD V Up] [4 N/N RD - Up] [5 N/N IB C Up]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD V Up] [3 N/N RD V Up] [4 N/N RD - Up] [5 N/N IB C Up]",
            L"AddInstance->5 [5 N/N]",
            -1);

        CheckState(
            L"Stateless: Two replica moves with one IB replica being down",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD V Up] [3 N/N RD V Up] [4 N/N IB C Up] [5 N/N DD C Down]",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD V Up] [3 N/N RD V Up] [4 N/N IB C Up] [5 N/N DD C Down]",
            L"AddInstance->4 [4 N/N]|RemoveInstance->5 [5 N/N]");
    }

    BOOST_AUTO_TEST_CASE(StatefulPlacementTest)
    {
        CheckState(
            L"Empty FailoverUnit",
            L"3 2 S 000/111",
            L"3 2 S 000/111",
            L"",
            1);

        CheckState(
            L"Initial primary is down (node is up) when MinReplicaSetSize > 1",
            L"3 2 SPE 000/111 [1 N/P IB - Down]",
            L"3 2 SPE 000/111 [1 N/P IB - Down]",
            L"QuorumLost");

        CheckState(
            L"Initial primary node is down when MinReplicaSetSize > 1",
            L"3 2 SPE 000/111 [1:0 N/P IB - Down]",
            L"3 2 SPE 000/111 [1:0 N/P IB - Down]",
            L"QuorumLost");

        CheckState(
            L"The Primary replica is marked as ToBeDropped and no Secondary replicas are available for swap",
            L"3 2 S 000/111 [0 N/P RD R Up]",
            L"3 2 S 000/111 [0 N/P RD - Up]",
            L"",
            2);

        CheckState(
            L"The Primary replica is marked as ToBeDropped and no Secondary replicas are available for swap",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB - Up]",
            L"AddReplica->0 [1 N/I]",
            1);

        CheckState(
            L"Missing Secondary Replicas",
            L"3 2 S 000/111 [0 N/P RD - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up]",
            L"",
            2);

        CheckState(
            L"A Secondary replica is marked as ToBeDropped",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD R Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]",
            L"");

        CheckState(
            L"A Secondary replica is marked as ToBeDropped while down Idle replica exists",
            L"3 2 S 000/111 [1 N/S RD R Up] [2 N/S RD - Up] [3 N/I RD - Down] [4 N/P RD - Up]",
            L"3 2 S 000/111 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/I RD - Down] [4 N/P RD - Up]",
            L"");

        CheckState(
            L"Move Secondary - IB Idle",
            L"3 2 S 000/111 [1 N/S RD R Up] [2 N/S RD - Up] [3 N/I IB - Up] [4 N/P RD - Up]",
            L"3 2 S 000/111 [1 N/S RD R Up] [2 N/S RD - Up] [3 N/I IB - Up] [4 N/P RD - Up]",
            L"AddReplica->4 [3 N/I]");

        CheckState(
            L"Move Secondary - RD Idle",
            L"3 2 S 000/111 [1 N/S RD R Up] [2 N/S RD - Up] [3 N/I RD - Up] [4 N/P RD - Up]",
            L"3 2 S 111/112 [1 S/N RD R Up] [2 S/S RD - Up] [3 I/S RD - Up] [4 P/P RD - Up]",
            L"DoReconfiguration->4 [1 S/N] [2 S/S] [3 I/S] [4 P/P]");

        CheckState(
            L"Move Primary - IB Idle",
            L"3 2 S 000/111 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/I IB P Up] [4 N/P RD R Up]",
            L"3 2 S 000/111 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/I IB P Up] [4 N/P RD R Up]",
            L"AddReplica->4 [3 N/I]");

        CheckState(
            L"Move Primary - RD Idle",
            L"3 2 S 000/111 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/I RD P Up] [4 N/P RD R Up]",
            L"3 2 S 111/112 [1 S/S RD - Up] [2 S/S RD - Up] [3 I/S RD P Up] [4 P/P RD R Up]",
            L"DoReconfiguration->4 [1 S/S] [2 S/S] [3 I/S] [4 P/P]");

        CheckState(
            L"No placement while primary IB",
            L"3 2 S 111/122 [0 N/P IB - Up]",
            L"3 2 S 111/122 [0 N/P IB - Up]",
            L"AddPrimary->0 [0 N/P]");

        CheckState(
            L"No placement while reconfiguration in progress",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 I/S RD - Up] [2 S/N RD R Up]",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 I/S RD - Up] [2 S/N RD R Up]",
            L"DoReconfiguration->0 [0 P/P] [1 I/S] [2 S/N]");

        CheckState(
            L"Only Idle replica is present",
            L"2 1 S 000/164 [1 N/I IB R Up]",
            L"2 1 S 000/164 [1 N/I IB R Up]",
            L"DeleteReplica->1 [1 N/I]",
            1);

        CheckState(
            L"Extra secondary replicas",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"",
            -1);

        CheckState(
            L"ToBeDropped primary and extra secondary replica",
            L"3 2 SP 000/111 [1 N/I RD DR Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD P Up] [5 N/P RD R Up]",
            L"3 2 SP 000/111 [1 N/I RD DR Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD P Up] [5 N/P RD R Up]",
            L"DeleteReplica->1 [1 N/I]");

        CheckState(
            L"InBuild primary replica, quorum not available",
            L"3 3 SP 000/111 [1 N/P IB - Up] [2 N/S RD - Down] [3 N/S RD - Down]",
            L"3 3 SP 000/111 [1 N/P IB - Up] [2 N/S RD - Down] [3 N/S RD - Down]",
            L"QuorumLost");

        CheckState(
            L"InBuild primary replica with no reconfigurtion going on",
            L"3 3 SP 000/111 [1 N/P IB - Up] [2 N/S SB - Up] [3 N/S SB - Up]",
            L"3 3 SP 111/122 [1 P/P IB - Up] [2 S/S IB - Up] [3 S/S IB - Up]",
            L"DoReconfiguration->1 [1 P/P] [2 S/S] [3 S/S]");

        CheckState(
            L"Idle ready replica when TargetReplicaSetSize is met",
            L"3 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I RD - Up]",
            L"3 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I RD - Up]",
            L"",
            -1);
    }

    BOOST_AUTO_TEST_CASE(StatefulAddRemoveReplicaTest)
    {
        CheckState(
            L"RemoveReplica for ToBeDropped/PendingRemove IB Idle replica",
            L"2 1 SP 000/122 [1 N/P RD - Up] [2 N/I IB RN Up]",
            L"2 1 SP 000/122 [1 N/P RD - Up] [2 N/I IB RN Up]",
            L"DeleteReplica->2 [2 N/I]|RemoveReplica->1 [2 N/I]",
            1);

        CheckMessage(
            L"AddPrimaryReply Message Processing for Primary Replica",
            RSMessage::GetAddPrimaryReply().Action,
            L"3 2 SE 111/122 [0 N/P RD - Up]",
            L"3 2 SE 111/122 [0 N/P IB - Up]",
            L"3 2 SE 000/122 [0 N/P RD - Up]",
            L"",
            L"0");

        CheckState(
            L"Initial Placement of Secondary Replicas",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB - Up] [2 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB - Up] [2 N/I IB - Up]",
            L"AddReplica->0 [1 N/I]|AddReplica->0 [2 N/I]");

        CheckMessage(
            L"AddReplicaReply Message Processing for 1st Secondary Replica",
            RSMessage::GetAddReplicaReply().Action,
            L"3 2 SE 000/111                 [1 N/I RD - Up]                ",
            L"3 2 SE 000/111 [0 N/P RD - Up] [1 N/I IB - Up] [2 N/I IB - Up]",
            L"3 2 SE 000/111 [0 N/P RD - Up] [1 N/I RD - Up] [2 N/I IB - Up]",
            L"",
            L"1");

        CheckState(
            L"Reconfiguration to promote Idle replicas to the Secondary role",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I RD - Up] [2 N/I RD - Up]",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 I/S RD - Up] [2 I/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 I/S] [2 I/S]");

        CheckMessage(
            L"DoReconfiguration message processing for promoting the Idle replicas to Sedondary replicas",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S 111/112 [0 P/P RD - Up] [1 I/S RD - Up] [2 I/S RD - Up]",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 I/S RD - Up] [2 I/S RD - Up]",
            L"3 2 S 000/112 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]",
            L"",
            L"0");

        CheckState(
            L"Removal of all Replicas",
            L"3 2 SD 000/111 [0 N/P RD R Up] [1 N/S RD R Up] [2 N/S RD R Up]",
            L"3 2 SD 000/111 [0 N/P RD R Up] [1 N/S RD R Up] [2 N/S RD R Up]",
            L"DeleteReplica->0 [0 N/P]|DeleteReplica->1 [1 N/S]|DeleteReplica->2 [2 N/S]");

        CheckMessage(
            L"DoReconfigurationReply Message Processing for Removing Secondary Replicas",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S 111/112 [0 P/P RD - Up]                    [2 I/S RD - Up]",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 S/N RD RD Up  ] [2 I/S RD - Up]",
            L"3 2 S 000/112 [0 N/P RD - Up] [1 N/N DD -  Down] [2 N/S RD - Up]",
            L"",
            L"0");

        CheckState(
            L"RemovalReplica for down replica",
            L"3 2 S 000/111 [0 N/P RD - Up] [1:0 N/I RD - Up  ] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1:0 N/N DD N Down] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"AddReplica->0 [3 N/I]|RemoveReplica->0 [1 N/N]");

        CheckState(
            L"Primary is marked as ToBeDropped and no ToBePromoted replica exists",
            L"4 2 S 000/111 [0 N/P RD R Up] [1 N/S RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"4 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"");

        CheckState(
            L"Primary is marked as ToBeDropped while a ToBePromoted IB Idle exists",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/I IB P Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/I IB P Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"AddReplica->0 [1 N/I]");

        CheckState(
            L"Primary is marked as ToBeDropped while a ToBePromoted RD Idle exists",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/I RD P Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"3 2 S 111/112 [0 P/P RD R Up] [1 I/S RD P Up] [2 S/S RD - Up] [3 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 I/S] [2 S/S] [3 S/S]");

        CheckMessage(
            L"RemoveReplicaReply Message Processing for down replica",
            RSMessage::GetRemoveReplicaReply().Action,
            L"3 2 S 000/111                 [1 N/N DD - Down]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I DD N Down] [2 N/S RD - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I DD - Down] [2 N/S RD - Up]",
            L"",
            L"1");

        CheckState(
            L"RemovalReplica for ToBeDropped replica",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB R Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB R Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"AddReplica->0 [3 N/I]|DeleteReplica->1 [1 N/I]");

        CheckMessage(
            L"RemoveReplicaReply Message Processing for ToBeDropped",
            RSMessage::GetRemoveReplicaReply().Action,
            L"3 2 S 000/111                 [1 N/N DD -  Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB RN Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/I IB R  Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"",
            L"1");

        CheckState(
            L"Removal of Primary Replica",
            L"3 2 SD 000/111 [0 N/P RD R Up] [1 N/N DD - Down] [2 N/N DD - Down]",
            L"3 2 SD 000/111 [0 N/P RD R Up] [1 N/N DD - Down] [2 N/N DD - Down]",
            L"DeleteReplica->0 [0 N/P]|DeleteReplica->1 [1 N/N]|DeleteReplica->2 [2 N/N]");

        CheckState(
            L"FailoverUnit is marked as ToBeDeleted when the Primary replica is Down",
            L"3 2 SD 000/111 [0:0 N/P RD D Up  ] [1 N/I RD D Up] [2 N/I RD D Up]",
            L"3 2 SD 000/111 [0:0 N/P DD - Down] [1 N/I RD D Up] [2 N/I RD D Up]",
            L"DeleteReplica->1 [1 N/I]|DeleteReplica->2 [2 N/I]");

        CheckMessage(
            L"DeleteReplicaReply Message Processing",
            RSMessage::GetDeleteReplicaReply().Action,
            L"3 1 S 000/111                 [2 N/I IB -  Up  ]                ",
            L"3 1 S 000/111 [1 N/P RD - Up] [2 N/I IB RN Up  ] [3 N/S RD R Up]",
            L"3 1 S 000/111 [1 N/P RD - Up] [2 N/N DD NZ Down] [3 N/S RD R Up]",
            L"",
            L"2");

        CheckMessage(
            L"DeleteReplicaReply containing older epoch for same replica instance",
            RSMessage::GetDeleteReplicaReply().Action,
            L"3 1 SP 000/111                                 [3 N/N DD D  Down]",
            L"3 1 SP 000/123 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/I SB -  Up  ]",
            L"3 1 SP 000/123 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/N DD NZ Down]",
            L"",
            L"3");

        CheckState(
            L"A Secondary replica is marked as ToBeDropped when there is an extra Secondary replica",
            L"3 2 S 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD R Up] [3 N/S RD - Up]",
            L"3 2 S 111/112 [0 P/P RD - Up] [1 S/S RD - Up] [2 S/N RD R Up] [3 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/N] [3 S/S]");

        CheckState(
            L"Promote an existing secondary, drop the old primary and add a new secondary",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/S RD P Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"3 2 S 000/111 [0 N/P RD R Up] [1 N/S RD P Up] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"AddReplica->0 [3 N/I]");

        CheckState(
            L"One primary and one idle replica are ToBeDropped, and no replica is ToBePromoted",
            L"3 2 S 000/111 [1 N/P RD R Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB R Up]",
            L"3 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB R Up]",
            L"DeleteReplica->4 [4 N/I]");

        CheckState(
            L"Primary is marked as ToBeDroppedByFM and no replica is marked as ToBePromoted",
            L"3 2 S 000/111 [1 N/P RD D Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"3 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            L"");

        CheckState(
            L"Primary is marked as ToBeDroppedByFM with one extra secondary replica",
            L"1 1 S 000/111 [1 N/P RD D Up] [2 N/S RD - Up]",
            L"1 1 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up]",
            L"",
            -1);

        CheckState(
            L"Secondary move with IB idle replica",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD V Up] [4 N/I IB C Up]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD V Up] [4 N/I IB C Up]",
            L"AddReplica->1 [4 N/I]");

        CheckState(
            L"Secondary move with RD idle replica",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD V Up] [4 N/I RD - Up]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD V Up] [4 N/I RD - Up]",
            L"",
            -1);

        CheckState(
            L"Secondary move when idle replica has gone down",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD V Up] [4 N/I IB C Down]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB C Down]",
            L"");

        CheckState(
            L"To be moved secondary node is down",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3:0 N/S RD V Up  ] [4 N/I IB C Up]",
            L"3 2 SP 111/112 [1 P/P RD - Up] [2 S/S RD - Up] [3:0 S/I RD - Down] [4 I/I IB C Up]",
            L"DoReconfiguration->1 [1 P/P] [2 S/S] [3 S/I]");

        CheckState(
            L"Two secondary moves with one RD idle replica",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD V Up] [3 N/S RD V Up] [4 N/I RD - Up] [5 N/I IB C Up]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD V Up] [3 N/S RD V Up] [4 N/I RD - Up] [5 N/I IB C Up]",
            L"AddReplica->1 [5 N/I]",
            -1);

        CheckState(
            L"Two secondary moves with one IB replica being down",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD V Up] [3 N/S RD V Up] [4 N/I IB C Up] [5 N/I IB C Down]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD V Up] [3 N/S RD V Up] [4 N/I IB C Up] [5 N/I IB C Down]",
            L"AddReplica->1 [4 N/I]");

        CheckState(
            L"A creating SB replica",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/I SB C Up]",
            L"3 2 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/I SB C Up]",
            L"",
            1);

        CheckState(
            L"A creating IB secondary replica",
            L"3 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S IB C Up]",
            L"3 3 SP 111/112 [1 P/P RD - Up] [2 S/S RD - Up] [3 S/S IB C Up]",
            L"DoReconfiguration->1 [1 P/P] [2 S/S] [3 S/S]");
    }

    BOOST_AUTO_TEST_CASE(StatefulSwapPrimaryTest)
    {
        CheckState(
            L"Sufficient Secondary Replicas: A Secondary replica is marked as ToBePromoted",
            L"3 2 S  000/111 [0 N/P RD - Up] [1 N/S RD P Up] [2 N/S RD - Up]",
            L"3 2 SW 111/122 [0 P/S RD - Up] [1 S/P RD P Up] [2 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/S] [1 S/P] [2 S/S]");

        CheckMessage(
            L"Sufficient Secondary Replicas: DoReconfigurationReply message processing for swapping the the Primary replica",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S  111/122 [0 P/S RD - Up] [1 S/P RD - Up] [2 S/S RD - Up]",
            L"3 2 SW 111/122 [0 P/S RD - Up] [1 S/P RD P Up] [2 S/S RD - Up]",
            L"3 2 S  000/122 [0 N/S RD - Up] [1 N/P RD - Up] [2 N/S RD - Up]",
            L"",
            L"0");

        CheckState(
            L"Send RemoveReplica during swap primary",
            L"3 2 SW  111/122 [0 P/S RD - Up] [1 S/P RD P Up] [2 N/N DD N Down]",
            L"3 2 SW  111/122 [0 P/S RD - Up] [1 S/P RD P Up] [2 N/N DD N Down]",
            L"DoReconfiguration->0 [0 P/S] [1 S/P]|RemoveReplica->0 [2 N/N]");

        CheckState(
            L"Old primary down during swap primary",
            L"3 2 SW  111/122 [0:0 P/S RD - Up  ] [1 S/P RD P Up] [2 N/N DD N Down]",
            L"3 2 S   111/122 [0:0 P/S DD N Down] [1 S/P RD - Up] [2 N/N DD - Down]",
            L"DoReconfiguration->1 [0 P/S] [1 S/P]");

        CheckState(
            L"A secondary is marked as ToBePromoted while another SB secondary exists",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD P Up] [3 N/S RD - Up] [4 N/S SB - Up]",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD P Up] [3 N/S RD - Up] [4 N/S IB - Up]",
            L"AddReplica->1 [4 N/S]");
    }

    BOOST_AUTO_TEST_CASE(StatefulFailoverTest)
    {
        CheckState(
            L"Primary replica down",
            L"3 2 S 000/111 [0:0 N/P RD - Up  ] [1 N/N DD N Down] [2 N/S RD - Up] [3 N/I IB - Up] [4 N/S RD - Up]",
            L"3 2 S 111/122 [0:0 P/N DD - Down] [1 N/N DD - Down] [2 S/S RD - Up] [3 I/I IB D Up] [4 S/P RD - Up]",
            L"DoReconfiguration->4 [0 P/N] [2 S/S] [4 S/P]");

        CheckState(
            L"Secondary replica is Down",
            L"3 2 S 000/111 [0 N/S DD - Down] [1 N/P RD - Up] [2 N/S RD - Up]",
            L"3 2 S 111/112 [0 S/N DD - Down] [1 P/P RD - Up] [2 S/S RD - Up]",
            L"DoReconfiguration->1 [0 S/N] [1 P/P] [2 S/S]");

        CheckState(
            L"A Secondary replica node is Down during reconfiguration",
            L"3 2 SW 111/122 [0:0 P/S RD - Up  ] [1 S/P RD - Up] [2 S/S RD - Up]",
            L"3 2 S  111/122 [0:0 P/S DD N Down] [1 S/P RD - Up] [2 S/S RD - Up]",
            L"DoReconfiguration->1 [0 P/S] [1 S/P] [2 S/S]");

        CheckState(
            L"The new Primary replica node is Down during reconfiguration",
            L"3 2 S 111/122 [0 P/S RD - Up] [1:0 S/P RD - Up  ] [2 S/S RD - Up]",
            L"3 2 S 111/133 [0 P/P RD - Up] [1:0 S/S DD - Down] [2 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/S]");

        CheckState(
            L"Primary is marked as ToBeDroppedByFM",
            L"3 2 S 000/111 [1 N/P RD D Up] [2 N/I RD - Up]",
            L"3 2 S 111/112 [1 P/P RD - Up] [2 I/S RD - Up]",
            L"DoReconfiguration->1 [1 P/P] [2 I/S]");
    }

    BOOST_AUTO_TEST_CASE(PreferredPrimaryTest)
    {
        CheckState(
            L"Ready secondary replica is marked as PreferredPrimary",
            L"3 1 SP 000/111 [1 N/S RD L Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"3 1 SP 000/111 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"");

        CheckState(
            L"StandBy idle replica is marked as PreferredPrimary",
            L"3 1 SP 000/111 [1 N/I SB L Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"3 1 SP 000/111 [1 N/I SB - Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"",
            1);

        CheckState(
            L"StandBy secondary replica is marked as PreferredPrimary",
            L"3 3 SP 000/111 [1 N/S SB L Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"3 3 SP 000/111 [1 N/S IB - Up] [2 N/S RD - Up] [3 N/P RD - Up]",
            L"AddReplica->3 [1 N/S]");
    }

    BOOST_AUTO_TEST_CASE(PersistentServicePrimaryUpBeforeReconfiguration)
    {
        CheckState(
            L"Quorum available with down replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2:0 N/S RD - Up  ]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/S RD - Up] [2:0 S/I RD - Down]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/I]");

        CheckState(
            L"Quorum available with SB replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S RD - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S IB - Up] [2 N/S RD - Up]",
            L"AddReplica->0 [1 N/S]");

        CheckState(
            L"Quorum available after building SB replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S SB - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S IB - Up] [2 N/S IB - Up]",
            L"AddReplica->0 [1 N/S]|AddReplica->0 [2 N/S]");

        CheckState(
            L"Quorum available after building 1 SB replica with down replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S DD - Down]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/S IB - Up] [2 S/N DD - Down]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/N]");

        CheckState(
            L"Quorum available after building 2 SB replica with down replica",
            L"5 3 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S RD - Down] [3 N/S SB - Up] [4 N/S RD - Up]",
            L"5 3 SP 111/112 [0 P/P RD - Up] [1 S/S IB - Up] [2 S/I RD - Down] [3 S/S IB - Up] [4 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/I] [3 S/S] [4 S/S]");

        CheckState(
            L"Quorum available with SB and down replica",
            L"5 3 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S RD - Down] [3 N/S RD - Up] [4 N/S RD - Up]",
            L"5 3 SP 111/112 [0 P/P RD - Up] [1 S/S SB - Up] [2 S/I RD - Down] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/I] [3 S/S] [4 S/S]");

        CheckState(
            L"Below Quorum",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Down] [2:0 N/S RD - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/I RD - Down] [2:0 S/S RD N Down]",
            L"DoReconfiguration->0 [0 P/P] [1 S/I] [2 S/S]|QuorumLost");

        CheckState(
            L"Below Quorum after building SB replica",
            L"5 3 SP 000/111 [0 N/P RD - Up] [1 N/S SB - Up] [2 N/S RD - Down] [3 N/S RD - Down] [4 N/S DD - Down]",
            L"5 3 SP 111/112 [0 P/P RD - Up] [1 S/S IB - Up] [2 S/I RD - Down] [3 S/S RD - Down] [4 S/N DD - Down]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/I] [3 S/S] [4 S/N]|QuorumLost");

        CheckState(
            L"Build idle SB replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up]",
            L"",
            1);

        CheckState(
            L"Standby replica not needed",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up] [3 N/I IB - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up] [3 N/I IB - Up]",
            L"AddReplica->0 [3 N/I]");

        CheckState(
            L"Extra SB replica",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up] [3 N/I SB - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Up] [1 N/S RD - Up] [2 N/I SB - Up] [3 N/I SB - Up]",
            L"",
            1);
    }

    BOOST_AUTO_TEST_CASE(PersistentServicePrimaryDownBeforeReconfiguration)
    {
        CheckState(
            L"Quorum available with SB replica",
            L"5 3 SP 000/111 [0 N/P RD - Down] [1 N/S SB - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD - Up]",
            L"5 3 SP 111/122 [0 P/I RD - Down] [1 S/S SB - Up] [2 S/S RD - Up] [3 S/S RD - Up] [4 S/P RD - Up]",
            L"DoReconfiguration->4 [0 P/I] [1 S/S] [2 S/S] [3 S/S] [4 S/P]");

        CheckState(
            L"Quorum available after building SB with down replica",
            L"5 3 SP 000/111 [0 N/P RD - Down] [1 N/S SB - Up] [2 N/S SB - Up] [3 N/S DD - Down] [4 N/S RD - Up]",
            L"5 3 SP 111/122 [0 P/I RD - Down] [1 S/S IB - Up] [2 S/S IB - Up] [3 S/N DD - Down] [4 S/P RD - Up]",
            L"DoReconfiguration->4 [0 P/I] [1 S/S] [2 S/S] [3 S/N] [4 S/P]");

        CheckState(
            L"Quorum available with only SB replica",
            L"5 3 SP 000/111 [0 N/P RD - Down] [1 N/S SB - Up] [2 N/S SB - Up] [3 N/S DD - Down] [4 N/S SB - Up]",
            L"5 3 SP 111/122 [0 P/I RD - Down] [1 S/S IB - Up] [2 S/S IB - Up] [3 S/N DD - Down] [4 S/P IB - Up]",
            L"DoReconfiguration->4 [0 P/I] [1 S/S] [2 S/S] [3 S/N] [4 S/P]");

        CheckState(
            L"Below Quorum",
            L"3 2 SP 000/111 [0 N/P RD - Down] [1 N/S DD - Down] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"3 2 SP 000/111 [0 N/P RD - Down] [1 N/S DD - Down] [2 N/S RD - Up] [3 N/I IB - Up]",
            L"DeleteReplica->1 [1 N/S]|QuorumLost");

        CheckState(
            L"Below Quorum all down replicas dropped",
            L"3 2 SP 000/111 [0 N/P DD - Down] [1 N/S DD - Down] [2 N/S RD - Up]",
            L"3 2 SP 111/122 [0 P/N DD - Down] [1 S/S DD - Down] [2 S/P RD - Up]",
            L"DoReconfiguration->2 [0 P/N] [1 S/S] [2 S/P]|QuorumLost");

        CheckState(
            L"Below Quorum all down replicas DD with SB replica",
            L"5 3 SP 000/111 [0 N/P DD - Down] [1 N/S DD - Down] [2 N/S RD - Up] [3 N/S DD - Down] [4 N/S SB - Up]",
            L"5 3 SP 111/122 [0 P/N DD - Down] [1 S/N DD - Down] [2 S/P RD - Up] [3 S/S DD - Down] [4 S/S IB - Up]",
            L"DoReconfiguration->2 [0 P/N] [1 S/N] [2 S/P] [3 S/S] [4 S/S]|QuorumLost");

        CheckState(
            L"All replicas in configuration DD with no data in previous configuration",
            L"5 3 SPE 000/111 [0 N/P DD - Down] [1 N/S DD - Down]",
            L"5 3 SPE 000/111 [0 N/N DD - Down] [1 N/N DD - Down]",
            L"DeleteReplica->0 [0 N/N]|DeleteReplica->1 [1 N/N]",
            1);

        CheckState(
            L"Only a creating replica is left in the CC",
            L"3 3 SP 000/111 [1 N/P DD - Down] [2 N/S DD - Down] [3 N/S IB C Up]",
            L"3 3 SP 111/122 [1 P/S DD - Down] [2 S/S DD - Down] [3 S/P IB C Up]",
            L"DoReconfiguration->3 [1 P/S] [2 S/S] [3 S/P]|QuorumLost");

        CheckState(
            L"Only a down replica is left in the CC",
            L"3 3 SP 000/111 [1 N/P DD - Down] [2 N/S DD - Down] [3 N/S RD - Down]",
            L"3 3 SP 000/111 [1 N/P DD - Down] [2 N/S DD - Down] [3 N/S RD - Down]",
            L"DeleteReplica->1 [1 N/P]|DeleteReplica->2 [2 N/S]|QuorumLost");
    }

    BOOST_AUTO_TEST_CASE(PersistentServiceInReconfiguration)
    {
        CheckState(
            L"Primary up SB replica in PC",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S RD - Up]  [3 I/S RD - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S RD - Up]  [3 I/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/N] [2 I/S] [3 I/S]");

        CheckState(
            L"Primary up SB replica in CC with quorum",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S SB - Up]  [3 I/S RD - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S SB - Up]  [3 I/S RD - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/N] [2 I/S] [3 I/S]");

        CheckState(
            L"Primary up SB replica in CC below quorum",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S SB - Up]  [3 I/S SB - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N SB - Up] [2 I/S IB - Up]  [3 I/S IB - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/N] [2 I/S] [3 I/S]");

        CheckState(
            L"Primary up quorum loss in PC",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N RD - Down] [2 S/S RD - Down]  [3 I/S SB - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Up] [1 S/N RD - Down] [2 S/S RD - Down]  [3 I/S IB - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/N] [2 S/S] [3 I/S]|QuorumLost");

        CheckState(
            L"Primary down quorum available",
            L"5 3 SP 111/112 [0 P/P RD - Down] [1 S/N RD - Down] [2 I/S RD - Up] [3 S/S RD - Up] [4 I/S SB - Up] [5 S/S RD - Up]",
            L"5 3 SP 111/123 [0 P/S RD - Down] [1 S/N RD - Down] [2 I/S RD - Up] [3 S/S RD - Up] [4 I/S SB - Up] [5 S/P RD - Up]",
            L"DoReconfiguration->5 [0 P/S] [1 S/N] [2 I/S] [3 S/S] [4 I/S] [5 S/P]");

        CheckState(
            L"Primary down quorum stuck in PC",
            L"5 3 SP 111/112 [0 P/P RD - Down] [1 S/N DD - Down] [2 I/S RD - Up]  [3 I/S RD - Up] [4 I/S SB - Up] [5 S/S DD - Down]",
            L"5 3 SP 111/112 [0 P/P RD - Down] [1 S/N DD - Down] [2 I/S RD - Up]  [3 I/S RD - Up] [4 I/S SB - Up] [5 S/S DD - Down]",
            L"QuorumLost");

        CheckState(
            L"Primary down all replicas DD in PC",
            L"3 2 SP 111/112 [0 P/P DD - Down] [1 S/N DD - Down] [2 S/S DD - Down] [3 I/S SB - Up]",
            L"3 2 SP 111/123 [0 P/S DD - Down] [1 S/N DD - Down] [2 S/S DD - Down] [3 I/P IB - Up]",
            L"DoReconfiguration->3 [0 P/S] [1 S/N] [2 S/S] [3 I/P]|QuorumLost");

        CheckState(
            L"Primary down quorum lost in CC",
            L"3 2 SP 111/112 [0 P/P RD - Down] [1 S/N RD - Up] [2 S/S SB - Up] [3 I/S DD - Down] [4 I/S DD - Down] [5 I/S RD - Up]",
            L"3 2 SP 111/112 [0 P/P RD - Down] [1 S/N RD - Up] [2 S/S SB - Up] [3 I/S DD - Down] [4 I/S DD - Down] [5 I/S RD - Up]",
            L"QuorumLost");

        CheckState(
            L"Primary down quorum lost in CC all down replicas dropped",
            L"3 2 SP 111/112 [0 P/P DD - Down] [1 S/N RD - Up] [2 S/S SB - Up] [3 I/S DD - Down] [4 I/S DD - Down] [5 I/S RD - Up]",
            L"3 2 SP 111/123 [0 P/S DD - Down] [1 S/N RD - Up] [2 S/S IB - Up] [3 I/S DD - Down] [4 I/S DD - Down] [5 I/P RD - Up]",
            L"DoReconfiguration->5 [0 P/S] [1 S/N] [2 S/S] [3 I/S] [4 I/S] [5 I/P]|QuorumLost");

        CheckState(
            L"CC Primary down during swap primary build secondary needed",
            L"5 3 SPW 111/122 [0 P/S RD - Up] [1 S/P RD - Down] [2 S/S SB - Up]",
            L"5 3 SP  111/133 [0 P/P RD - Up] [1 S/S RD - Down] [2 S/S IB - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/S]");

        CheckState(
            L"CC Primary down during swap primary below quorum",
            L"5 3 SPW 111/122 [0 P/S RD - Up] [1 S/P RD - Down] [2 S/S RD - Down]",
            L"5 3 SP  111/133 [0 P/P RD - Up] [1 S/S RD - Down] [2 S/S RD - Down]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/S]|QuorumLost");

        CheckState(
            L"All replicas in CC dropped during reconfiguration PC quorum available",
            L"3 2 SP 111/112 [0 P/P DD - Down] [1 S/N RD - Up] [2 S/N SB - Up] [3 I/S DD - Down] [4 I/S DD - Down]",
            L"3 2 SP 111/223 [0 P/S DD - Down] [1 S/P RD - Up] [2 S/S IB - Up] [3 I/S DD - Down] [4 I/S DD - Down]",
            L"DataLoss:1|DoReconfiguration->1 [0 P/S] [1 S/P] [2 S/S] [3 I/S] [4 I/S]|QuorumLost");

        CheckState(
            L"All replicas in CC dropped during reconfiguration PC below quorum",
            L"3 2 SP 111/112 [0 P/P DD - Down] [1 S/N RD - Down] [2 S/N SB - Up] [3 I/S DD - Down] [4 I/S DD - Down]",
            L"3 2 SP 111/112 [0 P/P DD - Down] [1 S/N RD - Down] [2 S/N SB - Up] [3 I/S DD - Down] [4 I/S DD - Down]",
            L"QuorumLost");

        CheckState(
            L"Primary gets dropped during reconfiguration with one down and one creating replica",
            L"3 3 SP 111/112 [1 P/P DD - Down] [2 S/S RD - Down] [3 S/S IB C Up]",
            L"3 3 SP 111/112 [1 P/P DD - Down] [2 S/S RD - Down] [3 S/S IB C Up]",
            L"QuorumLost");

        CheckState(
            L"Primary is dropped during reconfiguration with one SB and one creating replica",
            L"3 3 SP 111/112 [1 P/P DD - Down] [2 S/S SB - Up] [3 S/S IB C Up]",
            L"3 3 SP 111/123 [1 P/S DD - Down] [2 S/P IB - Up] [3 S/S IB C Up]",
            L"DoReconfiguration->2 [1 P/S] [2 S/P] [3 S/S]");

        CheckState(
            L"Primary down and a creating secondary is up",
            L"2 2 SP 111/112 [1 P/P RD - Down] [2 S/S IB C Up]",
            L"2 2 SP 111/123 [1 P/S RD - Down] [2 S/P IB C Up]",
            L"DoReconfiguration->2 [1 P/S] [2 S/P]|QuorumLost");

        CheckState(
            L"Only creating replicas are left in the configuration",
            L"3 3 SP 111/122 [1 P/S DD - Down] [2 S/S SB CN Up] [3 S/P SB CN Up]",
            L"3 3 SP 111/133 [1 P/S DD - Down] [2 S/S IB NC Up] [3 S/P IB NC Up]",
            L"DoReconfiguration->3 [1 P/S] [2 S/S] [3 S/P]");
    }

    BOOST_AUTO_TEST_CASE(PersistentServicePrimaryStandBy)
    {
        CheckState(
            L"Single primary restart",
            L"1 1 SP 000/111 [0 N/P SB - Up]",
            L"1 1 SP 111/122 [0 P/P IB - Up]",
            L"DoReconfiguration->0 [0 P/P]");

        CheckState(
            L"All replicas restart",
            L"3 2 SP 000/111 [0 N/P SB - Up] [1 N/S SB - Up] [2 N/S SB - Up]",
            L"3 2 SP 111/122 [0 P/P IB - Up] [1 S/S IB - Up] [2 S/S IB - Up]",
            L"DoReconfiguration->0 [0 P/P] [1 S/S] [2 S/S]");

        CheckState(
            L"Primary SB below Quorum",
            L"3 2 SP 000/111 [0 N/P SB - Up] [1 N/S RD - Down] [2 N/S RD - Down]",
            L"3 2 SP 000/111 [0 N/P SB - Up] [1 N/S RD - Down] [2 N/S RD - Down]",
            L"QuorumLost");

        CheckState(
            L"Primary SB Quorum available",
            L"3 2 SP 000/111 [0 N/P SB - Up] [1 N/S RD - Up] [2 N/S RD - Up]",
            L"3 2 SP 111/122 [0 P/S SB - Up] [1 S/S RD - Up] [2 S/P RD - Up]",
            L"DoReconfiguration->2 [0 P/S] [1 S/S] [2 S/P]");

        CheckState(
            L"Primary SB Quorum available with down replica",
            L"3 2 SP 000/111 [0 N/P SB - Up] [1 N/S RD - Up] [2 N/S RD - Down]",
            L"3 2 SP 111/122 [0 P/S IB - Up] [1 S/P RD - Up] [2 S/I RD - Down]",
            L"DoReconfiguration->1 [0 P/S] [1 S/P] [2 S/I]");

        CheckState(
            L"Primary SB and SB replica in CC with quorum",
            L"3 2 SP 111/112 [0 P/P SB - Up] [1 S/N SB - Up] [2 I/S SB - Up]  [3 I/S RD - Up]",
            L"3 2 SP 111/123 [0 P/S IB - Up] [1 S/N SB - Up] [2 I/S IB - Up]  [3 I/P RD - Up]",
            L"DoReconfiguration->3 [0 P/S] [1 S/N] [2 I/S] [3 I/P]");

        CheckState(
            L"Primary SB data lost in PC",
            L"5 3 SP 111/112 [0 P/P SB - Up] [1 S/N DD - Down] [2 I/S RD - Up] [3 I/S RD - Up] [4 I/S SB - Up] [5 S/S DD - Down]",
            L"5 3 SP 111/123 [0 P/S IB - Up] [1 S/N DD - Down] [2 I/S RD - Up] [3 I/P RD - Up] [4 I/S IB - Up] [5 S/S DD - Down]",
            L"DoReconfiguration->3 [0 P/S] [1 S/N] [2 I/S] [3 I/P] [4 I/S] [5 S/S]|QuorumLost");

        CheckState(
            L"All replicas restarted during swap primary",
            L"3 2 SPW 111/122 [0 P/S SB - Up] [1 S/P SB - Up] [2 S/S SB - Up]",
            L"3 2 SP  111/133 [0 P/S IB - Up] [1 S/P IB - Up] [2 S/S IB - Up]",
            L"DoReconfiguration->1 [0 P/S] [1 S/P] [2 S/S]");
    }

    BOOST_AUTO_TEST_CASE(DataLoss)
    {
        CheckState(
            L"All the replicas in the configuration are dropped and idle replica is down",
            L"2 1 SP 111/233 [1 P/P DD - Down] [2 S/S DD - Down] [3 I/I IB - Down]",
            L"2 1 SP 200/333 [1 N/N DD - Down] [2 N/N DD - Down] [3 N/I IB - Down]",
            L"DataLoss:2|DeleteReplica->1 [1 N/N]|DeleteReplica->2 [2 N/N]",
            1);

        CheckState(
            L"All the replicas in the configuration are dropped and one idle replica is up",
            L"2 1 SP 111/233 [1 P/P DD - Down] [2 S/S DD - Down] [3 I/I IB - Up]",
            L"2 1 SP 200/333 [1 N/N DD - Down] [2 N/N DD - Down] [3 N/P IB - Up]",
            L"DataLoss:2|DeleteReplica->1 [1 N/N]|DeleteReplica->2 [2 N/N]");

        CheckState(
            L"All the replicas in the configuration are dropped and multiple idle replicas are up",
            L"2 1 SP 111/233 [1 P/P DD - Down] [2 S/S DD - Down] [3 I/I IB - Up] [4 I/I SB - Up]",
            L"2 1 SP 200/333 [1 N/N DD - Down] [2 N/N DD - Down] [3 N/P IB - Up] [4 N/S SB - Up]",
            L"DataLoss:2|DeleteReplica->1 [1 N/N]|DeleteReplica->2 [2 N/N]");
    }

    BOOST_AUTO_TEST_CASE(PersistentServiceMessageProcessing)
    {
        CheckMessage(
            L"DoReconfigurationReply for restarted primary",
            RSMessage::GetDoReconfigurationReply().Action,
            L"1 1 S 111/122 [0 N/P RD - Up]",
            L"1 1 S 111/122 [0 P/P IB - Up]",
            L"1 1 S 000/122 [0 N/P RD - Up]",
            L"",
            L"0");

        CheckMessage(
            L"DoReconfigurationReply with IB secondary",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S 111/123 [0 N/P RD - Up]                   [2 N/S RD - Up] [3 N/S IB - Up]",
            L"3 2 S 111/123 [0 P/P IB - Up] [1 S/N RD - Up  ] [2 S/S IB - Up] [3 S/S IB - Up]",
            L"3 2 S 000/123 [0 N/P RD - Up] [1 N/N DD - Down] [2 N/S RD - Up] [3 N/S IB - Up]",
            L"",
            L"0");

        CheckMessage(
            L"DoReconfigurationReply report SB replica",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S 111/123 [0 N/P RD - Up]                   [2 N/I SB - Up] [3 N/S IB - Up]",
            L"3 2 S 111/123 [0 P/P IB - Up] [1 S/N RD - Up  ] [2 S/S RD - Up] [3 S/S IB - Up]",
            L"3 2 S 000/123 [0 N/P RD - Up] [1 N/N DD - Down] [2 N/I SB - Up] [3 N/S IB - Up]",
            L"",
            L"0");

        CheckMessage(
            L"DoReconfigurationReply removed DD replica",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 S 111/123 [0 N/P RD - Up]                                     [3 N/S RD - Up]",
            L"3 2 S 111/123 [0 P/P RD - Up] [1 S/N RD - Up  ] [2 S/S DD - Down] [3 I/S RD - Up]",
            L"3 2 S 000/123 [0 N/P RD - Up] [1 N/N DD - Down] [2 N/N DD - Down] [3 N/S RD - Up]",
            L"",
            L"0");

        CheckMessage(
            L"ChangeConfiguration to in-build secondary",
            RSMessage::GetChangeConfiguration().Action,
            L"5 3 SP 111/123 [0 N/S RD - Down] [1 N/P RD - Up 5 1] [2 N/S IB - Up 10 1] [3 N/S RD - Up 5 1] [4 N/S RD - Up 6 1]",
            L"5 3 SP 111/123 [0 P/S RD - Down] [1 I/P RD - Up] [2 S/S IB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"5 3 SP 111/134 [0 P/S RD - Down] [1 I/S RD - Up] [2 S/P IB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"ChangeConfigurationReply->1 [S_OK]|DoReconfiguration->2 [0 P/S] [1 I/S] [2 S/P] [3 S/S] [4 S/S]",
            L"1");

        CheckMessage(
            L"ChangeConfiguration to SB secondary",
            RSMessage::GetChangeConfiguration().Action,
            L"5 3 SP 111/123 [0 N/S RD - Down] [1 N/P RD - Up 5 1] [2 N/S SB - Up 10 1] [3 N/S RD - Up 5 1] [4 N/S RD - Up 6 1]",
            L"5 3 SP 111/123 [0 P/S RD - Down] [1 I/P RD - Up] [2 S/S SB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"5 3 SP 111/134 [0 P/S RD - Down] [1 I/S RD - Up] [2 S/P IB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"ChangeConfigurationReply->1 [S_OK]|DoReconfiguration->2 [0 P/S] [1 I/S] [2 S/P] [3 S/S] [4 S/S]",
            L"1");

        CheckMessage(
            L"ChangeConfiguration to replica in PC",
            RSMessage::GetChangeConfiguration().Action,
            L"5 3 SP 111/123 [0 N/S RD - Down] [1 N/P RD - Up 5 1] [2 N/S IB - Up 10 1] [3 N/S RD - Up 5 1] [4 N/S RD - Up 6 1]",
            L"5 3 SP 111/123 [0 P/S RD - Down] [1 I/P RD - Up] [2 S/N IB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"5 3 SP 111/134 [0 P/S RD - Down] [1 I/S RD - Up] [2 S/P IB - Up] [3 S/S RD - Up] [4 S/S RD - Up]",
            L"ChangeConfigurationReply->1 [S_OK]|DoReconfiguration->2 [0 P/S] [1 I/S] [2 S/P] [3 S/S] [4 S/S]",
            L"1");

        CheckMessage(
            L"ChangeConfiguration to elect a down replica as the new primary",
            RSMessage::GetChangeConfiguration().Action,
            L"5 1 SP 111/123 [1 N/P RD - Up] [2 N/S IB - Down 5 0] [3 N/I DD - Down] [4 N/S IB - Up]",
            L"5 3 SP 111/123 [1 P/P IB - Up] [2 S/S IB - Down] [3 S/I DD - Down] [4 S/S IB - Up]",
            L"5 3 SP 111/134 [1 P/S IB - Up] [2 S/P IB - Down] [3 S/I DD - Down] [4 S/S IB - Up]",
            L"ChangeConfigurationReply->1 [S_OK]",
            L"1");

        CheckMessage(
            L"AddReplicaReply for secondary",
            RSMessage::GetAddReplicaReply().Action,
            L"5 3 SP 111/112                 [1 N/S RD - Up]",
            L"5 3 SP 111/112 [0 P/P RD - Up] [1 S/S IB - Up] [2 I/S RD - Down]",
            L"5 3 SP 111/112 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Down]",
            L"",
            L"1");

        CheckMessage(
            L"Replica Down before getting dropped - Idle replica in DoReconfigurationReply",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 SP 112/113 [1 N/P RD - Up]                    [3 N/S RD - Up] [4 N/S RD - Up]",
            L"3 2 SP 112/113 [1 P/P RD - Up] [2 S/N RD  R Down] [3 S/S RD - Up] [4 I/S RD - Up]",
            L"3 2 SP 000/113 [1 N/P RD - Up] [2 N/I RD DR Down] [3 N/S RD - Up] [4 N/S RD - Up]",
            L"",
            L"1");

        CheckMessage(
            L"Down InBuild secondary is marked as PendingRemove",
            RSMessage::GetDoReconfigurationReply().Action,
            L"3 2 SP 000/112 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S SB - Down]",
            L"3 2 SP 111/112 [1 P/P RD - Up] [2 S/S RD - Up] [3 S/S IB N Down]",
            L"3 2 SP 000/112 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S SB - Down]",
            L"",
            L"1");

        CheckMessage(
            L"Replica with highest LSN has restarted and is down",
            RSMessage::GetChangeConfiguration().Action,
            L"3 3 SP 111/123 [1 N/P SB - Up -1 -1] [2 N/S SB - Down -1 -1] [3 N/S IB - Down 5 0]",
            L"3 3 SP 111/123 [1 S/P IB - Up]       [2 S/S IB - Down]       [3:1:2 P/S IB - Down]",
            L"3 3 SP 111/134 [1 S/S IB - Up]       [2 S/S IB - Down]       [3:1:2 P/P IB - Down]",
            L"ChangeConfigurationReply->1 [S_OK]",
            L"1");

        CheckMessage(
            L"ChangeConfiguration during swap-primary - I",
            RSMessage::GetChangeConfiguration().Action,
            L"2 2 SP  111/123 [1 P/S RD - Up 5 1] [2 S/P RD - Up 3 1]",
            L"2 2 SPW 111/123 [1 P/S RD - Up] [2 S/P RD - Up]",
            L"2 2 SP  111/134 [1 P/P RD - Up] [2 S/S RD - Up]",
            L"ChangeConfigurationReply->1 [S_OK]|DoReconfiguration->1 [1 P/P] [2 S/S]",
            L"1");

        CheckMessage(
            L"ChangeConfiguration during swap-primary - II",
            RSMessage::GetChangeConfiguration().Action,
            L"3 2 SP  111/123 [1 P/S RD - Up 3 1] [2 S/P RD - Up 3 1] [3 S/S RD - Up 5 1]",
            L"3 2 SPW 111/123 [1 P/S RD - Up] [2 S/P RD - Up] [3 S/S RD - Up]",
            L"3 2 SPW 111/134 [1 P/S RD - Up] [2 S/S RD - Up] [3 S/P RD - Up]",
            L"ChangeConfigurationReply->1 [S_OK]|DoReconfiguration->1 [1 P/S] [2 S/S] [3 S/P]",
            L"1");
    }

    BOOST_AUTO_TEST_CASE(MovementTask)
    {
        CheckPlbMovement(
            L"Swap primary/secondary when another ToBePromoted replica already exists",
            L"[SwapPrimarySecondary 1 2]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P Up]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P Up]",
            L"AddReplica->1 [4 N/I]",
            0);

        CheckPlbMovement(
            L"Promote secondary when another ToBePromoted replica already exists",
            L"[PromoteSecondary 0 2]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P Up]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P Up]",
            L"AddReplica->1 [4 N/I]",
            0);

        CheckPlbMovement(
            L"Move primary to a new node when another ToBePromoted replica already exists",
            L"[MovePrimary 4 5]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P  Up]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB PV Up]",
            L"00000000-0000-0000-0000-000000000000:(MovePrimary:4=>5)|AddReplica->1 [4 N/I]",
            -1);

        CheckPlbMovement(
            L"Promote an idle StandBy replica when another ToBePromoted replica already exists",
            L"[MovePrimary 4 5]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB P  Up] [5 N/I SB - Up]",
            L"3 2 SP 000/111 [1 N/P RD V Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/I IB PV Up] [5 N/I SB - Up]",
            L"00000000-0000-0000-0000-000000000000:(MovePrimary:4=>5)|AddReplica->1 [4 N/I]",
            -1);
    }

    BOOST_AUTO_TEST_CASE(AutoScaling)
    {
        CheckPlbAutoScale(
            L"AutoScaling: Target increase",
            L"3 0 - 000/111 [0 N/N RD - Up] [1 N/N RD - Up] [2 N/N RD - Up]",
            4,
            1);

        CheckPlbAutoScale(
            L"AutoScaling: Target decrease",
            L"3 0 - 000/111 [0 N/N RD - Up] [1 N/N RD - Up]",
            2,
            0);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestStateMachineTask::ClassSetup()
    {
        FailoverConfig::Test_Reset();
        FailoverConfig & config = FailoverConfig::GetConfig();
        config.IsTestMode = true;
        config.DummyPLBEnabled = true;
        config.PeriodicStateScanInterval = TimeSpan::MaxValue;

        auto & config_plb = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
        config_plb.IsTestMode = true;
        config_plb.DummyPLBEnabled = true;
        config_plb.PLBRefreshInterval = (TimeSpan::MaxValue); //set so that PLB won't spawn threads to balance empty data structures in functional tests
        config_plb.ProcessPendingUpdatesInterval = (TimeSpan::MaxValue);//set so that PLB won't spawn threads to balance empty data structures in functional tests

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        statelessTasks_.push_back(make_unique<StateUpdateTask>(*fm_, fm_->NodeCacheObj));
        statelessTasks_.push_back(make_unique<StatelessCheckTask>(*fm_, fm_->NodeCacheObj));
        statelessTasks_.push_back(make_unique<PendingTask>(*fm_));

        statefulTasks_.push_back(make_unique<StateUpdateTask>(*fm_, fm_->NodeCacheObj));
        statefulTasks_.push_back(make_unique<ReconfigurationTask>(fm_.get()));
        statefulTasks_.push_back(make_unique<PlacementTask>(*fm_));
        statefulTasks_.push_back(make_unique<PendingTask>(*fm_));

        return true;
    }

    bool TestStateMachineTask::ClassCleanup()
    {
        fm_->Close(true /* isStoreCloseNeeded */);
        FailoverConfig::Test_Reset();
        return true;
    }

}
