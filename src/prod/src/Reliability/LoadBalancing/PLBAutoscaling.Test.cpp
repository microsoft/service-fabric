// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"
#include "TestFM.h"
#include "PLBConfig.h"
#include "PlacementAndLoadBalancing.h"
#include "IFailoverManager.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability::LoadBalancingComponent;

    StringLiteral const PLBAssertTestSource("PLBAssertTest");

    class TestPLBAutoscaling
    {
    protected:
        TestPLBAutoscaling() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }
        TEST_CLASS_SETUP(ClassSetup);
        TEST_METHOD_SETUP(TestSetup);

        void PartitionsScalingTestHelper(std::wstring const& metricName, uint mergeSplit = 0);

        shared_ptr<TestFM> fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBAutoscalingSuite, TestPLBAutoscaling)

    BOOST_AUTO_TEST_CASE(AutoScalingBasicTest)
    {
        // Test case: average load is over maximum --> expect target count increase.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/0, S/1, S/2"), 1, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));
        now = now + PLBConfig::GetConfig().MinPlacementInterval + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingInvalidPolicyTest)
    {
        // This test verify default scale interval = 1sec, 
        // in case scale interval is not defined or when it is set to zero to avoid infinite loop in autoscaling
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingInvalidPolicyTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            0,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));

        auto scalingPolicy1 = CreateAutoScalingPolicyForService(
            L"servicefabric:/_CpuCores", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            0,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            CreateMetrics(L"Metric1/1.0/0/0"),
            3));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        // PLB should set new target partitions for TestService1.
        VERIFY_ARE_EQUAL(INT_MAX, fm_->GetPartitionCountChangeForService(L"TestService1"));

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/0, S/1, S/2"), 1, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));
        now = now + PLBConfig::GetConfig().MinPlacementInterval + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingEdgeCaseScaling)
    {
        // Test case: MaxTarget is above number of suitable nodes, autoscaler should not stuck. (3 nodes with A nodetype, 5 nodes in total)
        //             Placement    Current    Current    Autoscaling                         Should     Goal    
        //             constraint   replicas   target     MaxCount     MinCount   Increment   scale      target
        // Service 1:  A nodetype      5          5          -1            2           1        Up          3  - -1 should scale on all the available nodes
        // Service 2:  A nodetype      5          5          -1            2           1        No          5  - target shouldn't change even replicas are on invalid nodetype
        // Service 3:  A nodetype      5          5          -1            2           1        Down        4  - scale down should just lower target for increment

        // Service 4:  A nodetype      5          5           5            3           1        Up          5  - target should not be lowered due to number of proper nodes, if it is already above
        // Service 5:  A nodetype      5          5           5            3           1        No          5  - target should stay the same as no autoscaling action needed

        // Service 6:  A nodetype      5          5           5            1           3        Down        2  - target is lowered by increment no matter of valid nodes 

        // Service 7:  Nothing         5          5          -1            2           1        Up          5  - 5 nodes in total, scaleup should keep target on 5

        // Service 8:  A nodetype      3          3           4            2           1        Up          4  - only 3 valid nodes, but scaleup should set target to 4
        // Service 9:  Nothing         3          3           4            2           1        Up          4  - 5 valid nodes, scaleup should increase target to 4
        // Service 10: A nodetype      5          5          -1            4           1        Up          4  - same as 1, but shouldn't go below minimum count

        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingEdgeCaseScaling");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProp0;
        nodeProp0[L"Type"] = L"TypeA";
        map<wstring, wstring> nodeProp1;
        nodeProp1[L"Type"] = L"TypeB";
        map<wstring, wstring> nodeProp2;
        nodeProp2[L"Type"] = L"TypeB";
        map<wstring, wstring> nodeProp3;
        nodeProp3[L"Type"] = L"TypeA";
        map<wstring, wstring> nodeProp4;
        nodeProp4[L"Type"] = L"TypeA";

        plb.UpdateNode(CreateNodeDescription(0, std::wstring(), std::wstring(), move(nodeProp0), std::wstring(), true));
        plb.UpdateNode(CreateNodeDescription(1, std::wstring(), std::wstring(), move(nodeProp1), std::wstring(), true));
        plb.UpdateNode(CreateNodeDescription(2, std::wstring(), std::wstring(), move(nodeProp2), std::wstring(), true));
        plb.UpdateNode(CreateNodeDescription(3, std::wstring(), std::wstring(), move(nodeProp3), std::wstring(), true));
        plb.UpdateNode(CreateNodeDescription(4, std::wstring(), std::wstring(), move(nodeProp4), std::wstring(), true));
        

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy1 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            1);         // Scale Increment
        auto scalingPolicy2 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            1);         // Scale Increment
        auto scalingPolicy3 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            1);         // Scale Increment

        auto scalingPolicy4 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            3,          // Minimum Instance Count
            5,          // Maximum Instance Count
            1);         // Scale Increment
        auto scalingPolicy5 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            3,          // Minimum Instance Count
            5,          // Maximum Instance Count
            1);         // Scale Increment

        auto scalingPolicy6 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            1,          // Minimum Instance Count
            5,          // Maximum Instance Count
            3);         // Scale Increment

        auto scalingPolicy7 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            1);         // Scale Increment

        auto scalingPolicy8 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment
        auto scalingPolicy9 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment

        auto scalingPolicy10 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            4,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService( ServiceDescription(L"TestService1", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true,	CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false,	1, 7, true, 
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy1)));

        plb.UpdateService(ServiceDescription(L"TestService2", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy2)));

        plb.UpdateService(ServiceDescription(L"TestService3", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy3)));

        plb.UpdateService(ServiceDescription(L"TestService4", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy4)));

        plb.UpdateService(ServiceDescription(L"TestService5", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy5)));

        plb.UpdateService(ServiceDescription(L"TestService6", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy6)));

        plb.UpdateService(ServiceDescription(L"TestService7", L"TestType", wstring(L""), false, L"",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy7)));

        plb.UpdateService(ServiceDescription(L"TestService8", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy8)));

        plb.UpdateService(ServiceDescription(L"TestService9", L"TestType", wstring(L""), false, L"",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy9)));

        plb.UpdateService(ServiceDescription(L"TestService10", L"TestType", wstring(L""), false, L"Type==TypeA",
            wstring(L""), true, CreateMetrics(L"Metric1/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, 7, true,
            ServiceModel::ServicePackageIdentifier(), ServiceModel::ServicePackageActivationMode::ExclusiveProcess, 0, move(scalingPolicy10)));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService5"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService6"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService7"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService8"), 0, CreateReplicas(L"S/0, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService9"), 0, CreateReplicas(L"S/0, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService10"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 5));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService3", L"Metric1", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService4", L"Metric1", 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService6", L"Metric1", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(6, L"TestService7", L"Metric1", 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(7, L"TestService8", L"Metric1", 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(8, L"TestService9", L"Metric1", 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(9, L"TestService10", L"Metric1", 150));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(3, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(1))); //-1 is not changed
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(2)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(3)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(4))); //-1 is not changed
        VERIFY_ARE_EQUAL(2, fm_->GetTargetReplicaCountForPartition(CreateGuid(5)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(6))); //-1 is not changed
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(7))); 
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(8)));
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(9)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingMultiPartitionTest)
    {
        // Test case: 
        //      Partition 0: average load is over maximum  --> target count increase.
        //      Partition 1: average load is below minimum --> target count decrease.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"Metric1", 5));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(2, fm_->GetTargetReplicaCountForPartition(CreateGuid(1)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingMultipleServices)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingMultipleServices");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy1 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            5,          // Maximum Instance Count
            1);         // Scale Increment

        auto scalingPolicy2 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            3,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            5,          // Maximum Instance Count
            2);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy1),
            CreateMetrics(L"Metric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            3,
            move(scalingPolicy2),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 1000));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService2", L"Metric1", 1000));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(1);
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(1)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/0, S/1,S/2,S/3"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));

        now = now + TimeSpan::FromSeconds(2);
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(5, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(5, fm_->GetTargetReplicaCountForPartition(CreateGuid(1)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingMaxUnlimited)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingMaxUnlimited");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            -1,         // Maximum Instance Count
            10);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        // Since there are only 5 nodes, targetReplicaCount should become 5 (scaling to every node due to -1).
        VERIFY_ARE_EQUAL(5, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 2, CreateReplicas(L"S/0, S/1, S/2"), 2, Reliability::FailoverUnitFlags::Flags::None, false, false, 13));
        now = now + PLBConfig::GetConfig().MinPlacementInterval + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add instance *", value)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingTelemetryTest)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingTelemetryTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto now = Stopwatch::Now();

        auto scalingPolicy1 = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1) ;        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            CreateMetrics(L"Metric1/1.0/0/0"),
            1));

        auto scalingPolicy2 = CreateAutoScalingPolicyForPartition(
            L"servicefabric:/_CpuCores", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,         // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy2),
            CreateMetrics(L""),
            1));

        fm_->RefreshPLB(now);

        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        VERIFY_IS_TRUE(plbTestHelper.CheckAutoScalingStatistics(L"Metric1", 1));
        VERIFY_IS_TRUE(plbTestHelper.CheckAutoScalingStatistics(L"servicefabric:/_CpuCores", 1));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(),
            CreateMetrics(L""),
            1));

        fm_->RefreshPLB(now);
        VERIFY_IS_TRUE(plbTestHelper.CheckAutoScalingStatistics(L"servicefabric:/_CpuCores", 0));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            CreateMetrics(L"Metric1/1.0/0/0"),
            2));
        fm_->RefreshPLB(now);
        VERIFY_IS_TRUE(plbTestHelper.CheckAutoScalingStatistics(L"Metric1", 2));
    }


    BOOST_AUTO_TEST_CASE(AutoScalingSplitMerge1)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingSplitMerge1");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            2);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));


        plb.UpdateService(CreateServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType"),
            false,
            CreateMetrics(L"Metric2/1.0/3/3")));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        //now domains are merged
        plb.UpdateService(CreateServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType"),
            false,
            CreateMetrics(L"Metric1/1.0/3/3")));

        fm_->Clear();
        fm_->RefreshPLB(now);
        //even though the time has not expired we should be sending updates to FM
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        //now we split again
        plb.UpdateService(CreateServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType"),
            false,
            CreateMetrics(L"Metric2/1.0/3/3")));

        fm_->Clear();
        fm_->RefreshPLB(now);
        //even though the time has not expired we should be sending updates to FM
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingSplitMerge2)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingSplitMerge2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            2);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));

        //domains have merged
        plb.UpdateService(CreateServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType"),
            false,
            CreateMetrics(L"Metric1/1.0/3/3")));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        //after merge we should perserve the queue of pending services
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingLimitsEnforcedTest)
    {
        // Test cases:
        //    - Do not decrease instance count if it will go below minimum.
        //    - Do not increase instance count if it will go above maximum.
        wstring testName = L"AutoScalingLimitsEnforcedTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(L"TestService1", L"TestType1", false, 3, move(scalingPolicy), CreateMetrics(L"Metric1/1.0/0/0")));

        // FT 0: 4 instances, target is 4, avg load > 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));
        // FT 1: 2 instances, target is 2, avg load < 20
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 2));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 300));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService2", L"Metric1", 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // Check that nothing has scaled.
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(1)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingIntervalsTest)
    {
        // Test cases:
        //  - Load over average, but auto-scale interval has not passed.
        //  - Return load below average, expect auto-scale to do nothing.
        //  - Go over average, and increase.
        wstring testName = L"AutoScalingIntervalsTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            40,         // Upper Load Threshold
            300,        // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(L"TestService1", L"TestType1", false, 3, move(scalingPolicy), CreateMetrics(L"Metric1/1.0/0/0")));

        // FT 0: 3 instances, target is 3, avg load > 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 300));
        plb.ProcessPendingUpdatesPeriodicTask();

        auto now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // Check that nothing has scaled (timeout not elapsed).
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        // Return the load below average, timeout passes now.
        map<Federation::NodeId, uint> instanceLoads;
        instanceLoads[CreateNodeId(0)] = 30;
        instanceLoads[CreateNodeId(1)] = 29;
        instanceLoads[CreateNodeId(2)] = 31;
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 0, instanceLoads, false));

        // 300s is auto-scaling interval, so this should suffice
        now = now + TimeSpan::FromSeconds(400);
        fm_->RefreshPLB(now);

        // Check that nothing has scaled (average load is below limit).
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        // Return the load above average, timeout passes now.
        instanceLoads[CreateNodeId(0)] = 50;
        instanceLoads[CreateNodeId(1)] = 41;
        instanceLoads[CreateNodeId(2)] = 59;
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 0, instanceLoads, false));
        plb.ProcessPendingUpdatesPeriodicTask();

        // 300s is auto-scaling interval, so this should suffice
        now = now + TimeSpan::FromSeconds(400);
        fm_->RefreshPLB(now);

        // Check that nothing has scaled (average load is below limit).
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingServiceUpdateTest)
    {
        // Test cases:
        //  - Create service with AS, change load to trigger, then update service to remove AS.
        //  - Create service without AS, change load to trigger, then update service to add AS.
        wstring testName = L"AutoScalingServiceUpdateTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        // Test service 1 has auto-scale.
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        // Test service 1 does not have auto-scale.
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType1",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(),
            CreateMetrics(L"Metric1/1.0/0/0")));

        // FT 0: 3 instances, target is 3, avg load > 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 300));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"Metric1", 300));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update services: remove AS from TestService1, add AS to TestService2
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(),
            CreateMetrics(L"Metric1/1.0/0/0")));

        // Test service 1 does not have auto-scale.
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType1",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(1)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingDefaultLoadTest)
    {
        // Test case:
        //  - Default load is above maximum, so we scale up immediately.
        //  - Default load changes to be below minimum, so we scale down.
        //  - Go over average, and increase.
        wstring testName = L"AutoScalingDefaultLoadTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            40,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            false,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/100/100")));

        // FT 0: 3 instances, target is 3
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));
        plb.ProcessPendingUpdatesPeriodicTask();

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(400);
        fm_->RefreshPLB(now);

        // Default load is 100: it's > 40 so scale up.
        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            false,
            4,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/5/5")));

        // FT 0: 4 instances, target is 4
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"S/0, S/1, S/2, S/3"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));

        now = now + TimeSpan::FromSeconds(400);
        fm_->RefreshPLB(now);

        // Scale down based on default load.
        VERIFY_ARE_EQUAL(3, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingTriggeringMechanismTest)
    {
        // Test case:
        //  - Partition's target replica count is 4, which is equal to MaxInstanceCount.
        //  - Partition has only 3 up replicas.
        //  - Sanity check: partition should not be scaled (we care only about target, not up replica count).
        wstring testName = L"AutoScalingTriggeringMechanismTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService",
            L"TestType",
            false,
            3,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 90));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should not set new target replica count for this partition.
        VERIFY_ARE_EQUAL(-1, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingWithQuorumLogicTest)
    {
        // Test case:
        //  - Partition will be in FD and UD violation if we are respecting +-1 logic.
        //  - Quorum-based switches are off, but auto-scaling should force PLB to use quorum-based logic.
        //  - No violation should be reported.
        wstring testName = L"AutoScalingDefaultLoadTest";
        Trace.WriteInfo("PLBAutoscalingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // All Quorum-based logic is turned off.
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, false);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, false);
        PLBConfigScopeChange(QuorumBasedLogicAutoSwitch, bool, false);

        plb.UpdateNode(CreateNodeDescription(0, L"fd:/0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/0", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd:/1", L"1"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd:/1", L"1"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd:/2", L"2"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd:/2", L"2"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd:/3", L"3"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd:/3", L"3"));

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            40,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            8,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType1",
            false,
            6,
            move(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0")));

        // FT 0: 6 instances, target is 6, all instances on nodes 0-5 (2 per FD and UD for domains 0, 1, 2).
        // Domain 3 is empty, but this is not a problem because auto-scale should force quorum logic.
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(L"TestService1"),
            0,
            CreateReplicas(L"S/0, S/1, S/2, S/3, S/4, S/5"),
            0,
            Reliability::FailoverUnitFlags::Flags::None,
            false,
            false,
            6));
        plb.ProcessPendingUpdatesPeriodicTask();

        auto now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(AutoScalingResourceLoadTestBasic)
    {
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingResourceLoadTestBasic");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring metricName = *ServiceModel::Constants::SystemMetricNameMemoryInMB;

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForPartition(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Instance Count
            4,          // Maximum Instance Count
            1);        // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metricList;
        wstring metricNameBackup = metricName;

        metricList.push_back(ServiceMetric(move(metricNameBackup), 1.0, 0, 0));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(L"TestService", L"TestType", false, 3, move(scalingPolicy), move(metricList)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/1, S/2, S/3"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 3));

        map<Federation::NodeId, uint> loads;
        loads.insert({ CreateNodeId(1), 30 });
        loads.insert({ CreateNodeId(2), 30 });
        loads.insert({ CreateNodeId(3), 30 });
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", metricName, 0, loads, false));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now() + TimeSpan::FromSeconds(3));

        VERIFY_ARE_EQUAL(4, fm_->GetTargetReplicaCountForPartition(CreateGuid(0)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/1, S/2, S/3"), 1, Reliability::FailoverUnitFlags::Flags::None, false, false, 4));

        fm_->RefreshPLB(Stopwatch::Now() + TimeSpan::FromSeconds(3));

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 0|4", value)));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsBasicTest)
    {
        // Test case: average load for two services is over maximum and below minimum
        //      --> expect target count increase and decrease.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForService(
            L"Metric1", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0"),
            3));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0"),
            3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 120 / 3 = 40 --> Increase partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 90, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"Metric1", 15, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", L"Metric1", 15, 0));

        // Second service
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 21 / 3 = 7 --> Decrease partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService2", L"Metric1", 19, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService2", L"Metric1", 1, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService2", L"Metric1", 1, 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
        VERIFY_ARE_EQUAL(-1, fm_->GetPartitionCountChangeForService(L"TestService2"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingNoReplicasBuilt)
    {
        //Here we are checking that when no replicas are available no scaling will happen
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingNoReplicasBuilt");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForService(
            L"servicefabric:/_CpuCores", // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0"),
            3));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L""), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(INT_MAX, fm_->GetPartitionCountChangeForService(L"TestService1"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsUpThenDownTest)
    {
        // Test case: average load for one service is above the upper threshold.
        //          Service is scaled up by adding one partition.
        //          Service is updated so that load is below lower threshold.
        //          Service is scaled down by removin gone partition.
        //          Second service does exactly the opposite.
        //  Purpose: Basic test for update of auto scaling properties.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsUpThenDownTest");
        PartitionsScalingTestHelper(L"Metric1");
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsWithMemoryResourceTest)
    {
        // Same as AutoScalingPartitionsUpThenDownTest, only with memory resource
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsWithMemoryResourceTest");
        PartitionsScalingTestHelper(*ServiceModel::Constants::SystemMetricNameMemoryInMB);
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsWithCPUResourceTest)
    {
        // Same as AutoScalingPartitionsUpThenDownTest, only with CPU resource
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsWithCPUResourceTest");
        PartitionsScalingTestHelper(*ServiceModel::Constants::SystemMetricNameCpuCores);
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsLimitsRespectedTest)
    {
        // Check that both upper and lower limits are respected.
        // Scale increment will be 100, but we can add or remove only 1 partition due to limits.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsLimitsRespectedTest");
        std::wstring metricName = L"Metric1";
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));

        plb.UpdateNode(CreateNodeDescription(1));

        plb.UpdateNode(CreateNodeDescription(2));

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<ServiceMetric> defaultMetrics;
        wstring metricNameBackup = metricName;

        defaultMetrics.push_back(ServiceMetric(move(metricNameBackup), 1.0, 0, 0));

        auto scalingPolicy1 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            100,        // Scale Increment
            true);      // UseOnlyPrimaryLoad

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            vector<ServiceMetric>(defaultMetrics),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 120 / 3 = 40 --> Increase partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", metricName, 90, 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", metricName, 15, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", metricName, 15, 0));


        auto scalingPolicy2 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            100,        // Lower Load Threshold
            200,        // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            100);       // Scale Increment

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy2),
            vector<ServiceMetric>(defaultMetrics),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average load is 12 / 3 = 40 --> Decrease partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService2", metricName, 90));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService2", metricName, 15));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService2", metricName, 15));

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
        VERIFY_ARE_EQUAL(-1, fm_->GetPartitionCountChangeForService(L"TestService2"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionOnAverageAndPrimaryReplicaLoad)
    {
        // Check that stateful services can scale by average and by primary replica load only.
        // 2 services scales different due to UseOnlyPrimaryLoad flag.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionOnAverageAndPrimaryReplicaLoad");

        std::wstring metricName = L"Metric1";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        plb.ProcessPendingUpdatesPeriodicTask();
        auto now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<ServiceMetric> defaultMetrics;
        wstring metricNameBackup = metricName;

        defaultMetrics.push_back(ServiceMetric(move(metricNameBackup), 1.0, 0, 0));

        auto scalingPolicy1 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            100,        // Scale Increment
            true);      // UseOnlyPrimaryLoad

        auto scalingPolicy2 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            100,        // Scale Increment
            false);     // UseOnlyPrimaryLoad
        
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            vector<ServiceMetric>(defaultMetrics),
            3));

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            true,
            3,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy2),
            vector<ServiceMetric>(defaultMetrics),
            3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/2, S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/0, S/1"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Use load of only primary replicas
        // Average primary load is 18 < 20 there should be no repartitioning
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 18, 24));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"Metric1", 18, 24));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", L"Metric1", 18, 24));

        // Second service
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/1, S/2, S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"P/2, S/0, S/1"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Use average load of all replicas
        // Average load is (18+24*2)/3=22 > 20, new partition should be added
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService2", L"Metric1", 18, 24));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService2", L"Metric1", 18, 24));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService2", L"Metric1", 18, 24));

        plb.ProcessPendingUpdatesPeriodicTask();
        now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target partition count for service2
        VERIFY_ARE_EQUAL(INT_MAX, fm_->GetPartitionCountChangeForService(L"TestService1"));
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService2"));

        // Now average primary load is 22 > 20, so service1 should also be scaled up
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"Metric1", 22, 2));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"Metric1", 22, 2));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", L"Metric1", 22, 2));

        plb.ProcessPendingUpdatesPeriodicTask();
        now = Stopwatch::Now() + TimeSpan::FromSeconds(20);
        fm_->RefreshPLB(now);

        // PLB should set new target partition count for service1
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsIntervalsTest)
    {
        // Check that scaling interval is respected. check it also with service update.
        // Test case:
        //      PLB auto scales the service.
        //      Service is updated to reflect the new partition count and scale interval increased.
        //      Call refresh before interval has passed - nothing should happen.
        //      One more refresh after the interval has passed.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsScaleFailureTest");
        std::wstring metricName = L"Metric1";
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<ServiceMetric> defaultMetrics;
        wstring metricNameBackup = metricName;

        defaultMetrics.push_back(ServiceMetric(move(metricNameBackup), 1.0, 0, 0));

        auto scalingPolicy = CreateAutoScalingPolicyForService(
            metricName, // metricName
            1,          // Lower Load Threshold
            5,          // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            5,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            move(scalingPolicy),
            vector<ServiceMetric>(defaultMetrics),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 120 / 3 = 40 --> Increase partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", metricName, 90, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", metricName, 15, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", metricName, 15, 0));

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
        fm_->PLBTestHelper.ClearPendingRepartitions();

        // Update service, increase scale interval and add one more partition.
        scalingPolicy = CreateAutoScalingPolicyForService(
            metricName, // metricName
            1,          // Lower Load Threshold
            5,          // Upper Load Threshold
            100,        // Scale Interval In Seconds
            2,          // Minimum Partition Count
            5,          // Maximum Partition Count
            1);         // Scale Increment
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            move(scalingPolicy),
            vector<ServiceMetric>(defaultMetrics),
            4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average load is above the threshold, but interval did not pass
        now = now + TimeSpan::FromSeconds(50);
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(INT_MAX, fm_->GetPartitionCountChangeForService(L"TestService1"));

        // Now the interval has passed. One more partition should be added.

        now = now + TimeSpan::FromSeconds(120);
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsDomainMergeTest)
    {
        // Check that auto scaling is preserved on domain merge.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsDomainMergeTest");
        PartitionsScalingTestHelper(L"Metric1", 1);
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsDomainSplitTest)
    {
        // Check that auto scaling is preserved on domain split.
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsDomainSplitTest");
        PartitionsScalingTestHelper(L"Metric1", 2);
    }

    BOOST_AUTO_TEST_CASE(AutoScalingPartitionsScaleFailureTest)
    {
        // Check that auto scaling reacts well on failure in repartitioning.
        // Test case:
        //      PLB should attempt to auto scale the service.
        //      No update from FM - AutoScaler should hold on.
        //      Update that auto scaling has failed
        //      PLB should retry auto scaling operation
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingPartitionsScaleFailureTest");
        std::wstring metricName = L"Metric1";
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<ServiceMetric> defaultMetrics;
        wstring metricNameBackup = metricName;

        defaultMetrics.push_back(ServiceMetric(move(metricNameBackup), 1.0, 0, 0));

        auto scalingPolicy1 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);       // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            vector<ServiceMetric>(defaultMetrics),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 120 / 3 = 40 --> Increase partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", metricName, 90, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", metricName, 15, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", metricName, 15, 0));

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
        fm_->PLBTestHelper.ClearPendingRepartitions();

        // After 20 seconds still not update from FM. PLB should not attempt auto scaling.
        now = now + TimeSpan::FromSeconds(20);
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(INT_MAX, fm_->GetPartitionCountChangeForService(L"TestService1"));

        // Repartition has failed, PLB should attempt again.
        fm_->PLBTestHelper.InduceRepartitioningFailure(L"TestService1", Common::ErrorCodeValue::Timeout);

        now = now + TimeSpan::FromSeconds(20);
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
    }

    BOOST_AUTO_TEST_CASE(AutoScalingStatefulServiceUpdateLoadWithoutPrimary)
    {
        // This test is updating load for partition that doesn't have primary replica
        // and checks that we'll guard from asserting
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingStatefulServiceUpdateLoadWithoutPrimary");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

       auto scalingPolicy = CreateAutoScalingPolicyForService(
            L"servicefabric:/_CpuCores", // metricName
            1,         // Lower Load Threshold
            3,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L""),
            3));

        // Create a failoverUnit without primary replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L""),0));
        plb.Refresh(Stopwatch::Now());

        // Update load for CPU metric although there is no primary
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"servicefabric:/_CpuCores", 2, 0));
        plb.Refresh(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(AutoScalingStatelessServiceUpdateLoadWithoutInstances)
    {
        // This test is updating load for partition that doesn't have instances
        // and checks that no asserts will happen
        Trace.WriteInfo("PLBAutoscalingTestSource", "AutoScalingStatelessServiceUpdateLoadWithoutInstances");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        auto scalingPolicy = CreateAutoScalingPolicyForService(
            L"servicefabric:/_CpuCores", // metricName
            1,         // Lower Load Threshold
            3,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            false,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy),
            CreateMetrics(L"Metric1/1.0/0/0"),
            3));

        // Create a failoverUnit without instances
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L""), 0));
        plb.Refresh(Stopwatch::Now());

        // Update load for Memory metric although there are no instances
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"servicefabric:/_MemoryInMB", 2000));
        plb.Refresh(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBAutoscaling::PartitionsScalingTestHelper(std::wstring const& metricName, uint mergeSplit)
    {
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        // mergeSplit == 0: Do nothing, all services have the same metrics.
        // mergeSplit == 1: Two services will have different metrics, third service will merge them.
        // mergeSplit == 2: Two services will have different metrics, one service will first merge them and then split them.
        PlacementAndLoadBalancing & plb = fm_->PLB;

        uint loadMultiplier = 1;

        if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            loadMultiplier = ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
        }

        plb.UpdateNode(CreateNodeDescription(0));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring metricName2 = metricName;

        if (mergeSplit > 0)
        {
            metricName2 = L"SecondMetric";
        }

        vector<ServiceMetric> defaultMetrics1;
        defaultMetrics1.push_back(ServiceMetric(wstring(metricName), 1.0, 0, 0));

        vector<ServiceMetric> defaultMetrics2;
        defaultMetrics2.push_back(ServiceMetric(wstring(metricName2), 1.0, 0, 0));

        vector<ServiceMetric> defaultMetrics3;
        defaultMetrics3.push_back(ServiceMetric(wstring(metricName), 1.0, 0, 0));
        defaultMetrics3.push_back(ServiceMetric(wstring(metricName2), 1.0, 0, 0));

        auto scalingPolicy1 = CreateAutoScalingPolicyForService(
            metricName, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            vector<ServiceMetric>(defaultMetrics1),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average primary load is 120 / 3 = 40 --> Increase partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", metricName, 90 * loadMultiplier, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", metricName, 15 * loadMultiplier, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", metricName, 15 * loadMultiplier, 0));


        auto scalingPolicy2 = CreateAutoScalingPolicyForService(
            metricName2,// metricName
            100,        // Lower Load Threshold
            200,        // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            2);         // Scale Increment

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy2),
            vector<ServiceMetric>(defaultMetrics2),
            3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        // Average load is 12 / 3 = 40 --> Decrease partition count
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService2", metricName2, 90 * loadMultiplier));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService2", metricName2, 15 * loadMultiplier));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService2", metricName2, 15 * loadMultiplier));

        plb.ProcessPendingUpdatesPeriodicTask();

        if (mergeSplit > 0)
        {
            VERIFY_ARE_EQUAL(2, plb.GetServiceDomains().size());
            // Merge domains.
            plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
                L"AnnoyingService",
                L"TestType",
                false,
                1,
                vector<Reliability::ServiceScalingPolicyDescription>(),
                vector<ServiceMetric>(defaultMetrics3),
                3));
            VERIFY_ARE_EQUAL(1, plb.GetServiceDomains().size());
        }

        if (mergeSplit == 2)
        {
            // Split the domain now
            plb.DeleteService(L"AnnoyingService");
            plb.ProcessPendingUpdatesPeriodicTask();
            VERIFY_ARE_EQUAL(2, plb.GetServiceDomains().size());
        }

        auto now = Stopwatch::Now() + TimeSpan::FromSeconds(10);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService1"));
        VERIFY_ARE_EQUAL(-1, fm_->GetPartitionCountChangeForService(L"TestService2"));
        fm_->PLBTestHelper.ClearPendingRepartitions();

        // Change scaling policies so that TS1 scales down, and TS2 scales up

        scalingPolicy1 = CreateAutoScalingPolicyForService(
            metricName,// metricName
            100,        // Lower Load Threshold
            200,        // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            2);         // Scale Increment

        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService1",
            L"TestType",
            true,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy1),
            vector<ServiceMetric>(defaultMetrics1),
            4));

        // Average primary load is 120 / 4 = 30 --> Decrease partition count
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0, Reliability::FailoverUnitFlags::Flags::None, false, false, 1));

        scalingPolicy2 = CreateAutoScalingPolicyForService(
            metricName2, // metricName
            10,         // Lower Load Threshold
            20,         // Upper Load Threshold
            1,          // Scale Interval In Seconds
            2,          // Minimum Partition Count
            4,          // Maximum Partition Count
            1);         // Scale Increment
        plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
            L"TestService2",
            L"TestType",
            false,
            1,
            vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicy2),
            vector<ServiceMetric>(defaultMetrics2),
            2));

        // Average load is 105 / 2 = 52.5 --> Increase partition count.
        plb.DeleteFailoverUnit(L"TestService2", CreateGuid(5));

        plb.ProcessPendingUpdatesPeriodicTask();

        if (mergeSplit == 2)
        {
            VERIFY_ARE_EQUAL(2, plb.GetServiceDomains().size());
            // Again: Merge domains.
            plb.UpdateService(CreateServiceDescriptionWithAutoscalingPolicy(
                L"AnnoyingService",
                L"TestType",
                false,
                1,
                vector<Reliability::ServiceScalingPolicyDescription>(),
                vector<ServiceMetric>(defaultMetrics3),
                3));
            VERIFY_ARE_EQUAL(1, plb.GetServiceDomains().size());
            // Split the domain now
            plb.DeleteService(L"AnnoyingService");
            plb.ProcessPendingUpdatesPeriodicTask();
            VERIFY_ARE_EQUAL(2, plb.GetServiceDomains().size());
        }

        // Make sure that interval has passed.
        now = now + TimeSpan::FromSeconds(100);
        fm_->RefreshPLB(now);

        // PLB should set new target replica count for this partition.
        VERIFY_ARE_EQUAL(-2, fm_->GetPartitionCountChangeForService(L"TestService1"));
        VERIFY_ARE_EQUAL(1, fm_->GetPartitionCountChangeForService(L"TestService2"));
    }

    bool TestPLBAutoscaling::ClassSetup()
    {
        Trace.WriteInfo("TestPLBAutoscalingTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBAutoscaling::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }
}
