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

    StringLiteral const TestFailoverUnitSource = "TestFailoverUnitSource";

    class TestFailoverUnit
    {
    protected:
        TestFailoverUnit() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestFailoverUnit()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestFailoverUnitSuite, TestFailoverUnit)

    BOOST_AUTO_TEST_CASE(CreateServiceDefaultMoveCostTest)
    {
        wstring const testName = L"CreateServiceDefaultMoveCostTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas with low default move cost
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify default move cost after service creation
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Primary, FABRIC_MOVE_COST_LOW));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Secondary, FABRIC_MOVE_COST_LOW));
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDefaultMoveCostTest)
    {
        wstring const testName = L"UpdateServiceDefaultMoveCostTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas with low default move cost
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update service to set high default move cost
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_HIGH));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify default move cost after service update
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Primary, FABRIC_MOVE_COST_HIGH));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Secondary, FABRIC_MOVE_COST_HIGH));
    }

    BOOST_AUTO_TEST_CASE(ReportMoveCostWithoutUseSeparateSecondaryLoadTest)
    {
        wstring const testName = L"ReportMoveCostWithoutUseSeparateSecondaryLoadTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas with low default move cost
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report move cost: primary - high, secondary - medium
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(
            0, serviceName, *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
            FABRIC_MOVE_COST_HIGH, FABRIC_MOVE_COST_MEDIUM));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify reported move cost
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Primary, FABRIC_MOVE_COST_HIGH));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Secondary, FABRIC_MOVE_COST_MEDIUM));
    }

    BOOST_AUTO_TEST_CASE(ReportMoveCostWithUseSeparateSecondaryLoadTest)
    {
        wstring const testName = L"ReportMoveCostWithUseSeparateSecondaryLoadTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas with low default move cost
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report move cost: primary - high, secondary - medium. Move cost doesn't have UseSeparateSecondaryLoad supported, but it
        // requires that LoadOrMoveCostDescription use secondaryLoads map.
        std::map<Federation::NodeId, uint> secondaryMoveCost;
        secondaryMoveCost.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), FABRIC_MOVE_COST_MEDIUM));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(
            0, serviceName, *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
            FABRIC_MOVE_COST_HIGH, secondaryMoveCost));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify reported move cost
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Primary, FABRIC_MOVE_COST_HIGH));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Secondary, FABRIC_MOVE_COST_MEDIUM));
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDefaultMoveCostAfterReportMoveCostTest)
    {
        wstring const testName = L"UpdateServiceDefaultMoveCostAfterReportMoveCostTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas with low default move cost
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report move cost: primary - high, secondary - medium
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(
            0, serviceName, *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
            FABRIC_MOVE_COST_HIGH, FABRIC_MOVE_COST_MEDIUM));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update service to set low default move cost
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify that reported move cost didn't change
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Primary, FABRIC_MOVE_COST_HIGH));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckMoveCostValue(serviceName, ftId, ReplicaRole::Secondary, FABRIC_MOVE_COST_MEDIUM));
    }

    BOOST_AUTO_TEST_CASE(ServiceLoadTest)
    {
        wstring const testName = L"ServiceLoadTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        wstring metric = L"MyMetric";

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - two replicas; Default loads are set
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/11/12", metric))));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Verify default load after service creation
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 11, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 12, CreateNodeId(1)));

        // Update service to set new default load values
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/20/5", metric))));

        // Verify default load values after service update
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 20, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 5, CreateNodeId(1)));

        // Report loads
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 30, 31));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Verify reported loads
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 30, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 31, CreateNodeId(1)));

        // Update service to set new default load
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/41/42", metric))));

        // Verify that reported load values didn't change
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 30, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 31, CreateNodeId(1)));

        // Report loads with same values as defaults
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 41, 42));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update service to set new default load values
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/51/52", metric))));

        // Check if reported values have changed (they shouldn't change)
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 41, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 42, CreateNodeId(1)));
    }

    BOOST_AUTO_TEST_CASE(ReportedLoadChangeSeparateSecondaryLoadTest)
    {
        wstring const testName = L"ReportedLoadChangeSeparateSecondaryLoadTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        wstring metric = L"MyMetric";
        wstring metric2 = L"MyMetric2";
        wstring metric3 = L"MyMetric3";

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - three replicas; Default loads are set; Two metrics
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/11/12,{1}/2.0/11/12", metric, metric2))));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report loads with same values as defaults
        std::map<Federation::NodeId, uint> SecondaryLoad;
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), 12));
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(2), 12));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 11, SecondaryLoad));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric2, 11, SecondaryLoad));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update service to set new default load values
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/21/22,{1}/2.0/311/312", metric, metric2))));

        // Check if reported values have changed (they shouldn't change)
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 11, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 12, CreateNodeId(1), true));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 12, CreateNodeId(2), true));

        // Update service-remove metric
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/2.0/311/312", metric2))));

        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Primary, 11, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Secondary, 12, CreateNodeId(1), true));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Secondary, 12, CreateNodeId(2), true));

        // Update service-add metrics
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/111/112,{1}/2.0/221/222,{2}/3.0/331/332", metric, metric2, metric3))));

        fm_->RefreshPLB(Stopwatch::Now());

        // Verify load values
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 111, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 112, CreateNodeId(1), true));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 112, CreateNodeId(2), true));

        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Primary, 11, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Secondary, 12, CreateNodeId(1), true));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric2, ReplicaRole::Secondary, 12, CreateNodeId(2), true));

        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric3, ReplicaRole::Primary, 331, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric3, ReplicaRole::Secondary, 332, CreateNodeId(1), true));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric3, ReplicaRole::Secondary, 332, CreateNodeId(2), true));
    }

    BOOST_AUTO_TEST_CASE(ReportedLoadChangeSeparateSecondaryLoadTestWithMovements)
    {
        wstring const testName = L"ReportedLoadChangeSeparateSecondaryLoadTestWithMovements";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        wstring metric = L"MyMetric";
        wstring metric2 = L"MyMetric2";
        wstring metric3 = L"MyMetric3";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/100", metric)));

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - three replicas; Default loads are set
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/0/0", metric))));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report load values
        std::map<Federation::NodeId, uint> SecondaryLoad;
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), 22));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 21, SecondaryLoad));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Check loads before move 1->3:
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(3)));

        // Move secondary 1->3
        NodeId newSecondary = CreateNodeId(3);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(wstring(serviceName), ftId, CreateNodeId(1), newSecondary, true).IsSuccess());

        // Apply move 1->3
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            ftId, wstring(serviceName), 1, CreateReplicas(L"P/0,S/1/D,S/2,I/3/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check loads after move 1->3:
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(3)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 2, CreateReplicas(L"P/0,S/2,S/3"), 1));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check loads after applyping move 1->3:
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(3)));

        fm_->ClearMoveActions();

        NodeId newPrimary = CreateNodeId(2);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerSwapPrimary(wstring(serviceName), ftId, CreateNodeId(0), newPrimary, true).IsSuccess());

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 3, CreateReplicas(L"S/0,P/2,S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check loads after swapping primary 0->2:
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(2)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(3)));

        // Update Failover Unit - add secondary replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 4, CreateReplicas(L"S/0,S/1,P/2,S/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check loads after adding secondary replica:
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 11, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(2)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(ReportedLoadChangeSeparateSecondaryLoadTestPrimaryActions)
    {
        wstring const testName = L"ReportedLoadChangeSeparateSecondaryLoadTestPrimaryActions";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        wstring metric = L"MyMetric";

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - three replicas; Default loads are set
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/0/0", metric))));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"S/0, S/1, P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report load values
        std::map<Federation::NodeId, uint> SecondaryLoad;
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), 22));
        //SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(2), 13));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 21, SecondaryLoad));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Check load values
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(2)));

        fm_->ClearMoveActions();

        // Promote replica at 0 to primary (swap primary secondary)
        NodeId newPrimary = CreateNodeId(0);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerPromoteToPrimary(wstring(serviceName), ftId, newPrimary).IsSuccess());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>0", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values after promotion :
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));

        // Drop primary replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 2, CreateReplicas(L"P/0/D, S/1, S/2"), -1));
        fm_->RefreshPLB(Stopwatch::Now());
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 3, CreateReplicas(L"S/1,S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values primary drop :
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Dropped, 0, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));

        fm_->ClearMoveActions();

        // Promote replica at 1 to primary
        newPrimary = CreateNodeId(1);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerPromoteToPrimary(wstring(serviceName), ftId, newPrimary).IsSuccess());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 promote secondary 1", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 4, CreateReplicas(L"P/1,S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values after promotion :
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));

        // Add replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 5, CreateReplicas(L"P/1,S/2,N/3/B"), 1));
        fm_->RefreshPLB(Stopwatch::Now());

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 6, CreateReplicas(L"P/1,S/2,N/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        fm_->ClearMoveActions();

        // Move primary to 3 (promote replica at 3 to primary)
        newPrimary = CreateNodeId(3);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerPromoteToPrimary(wstring(serviceName), ftId, newPrimary).IsSuccess());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 1=>3", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 7, CreateReplicas(L"S/2,P/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values after promotion :
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(3)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 0, CreateNodeId(2)));

        // Drop primary replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 8, CreateReplicas(L"P/0/D,S/2"), -1));
        fm_->RefreshPLB(Stopwatch::Now());
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 9, CreateReplicas(L"S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Add replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 10, CreateReplicas(L"S/2,N/1/B"), 1));
        fm_->RefreshPLB(Stopwatch::Now());

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 11, CreateReplicas(L"S/2,N/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        fm_->ClearMoveActions();

        // Promote secondary replica to primary
        newPrimary = CreateNodeId(1);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerPromoteToPrimary(wstring(serviceName), ftId, newPrimary).IsSuccess());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add primary 1", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 12, CreateReplicas(L"S/2,P/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values after promotion :
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(1)));
    }

    BOOST_AUTO_TEST_CASE(ResetPartitionLoadTest)
    {
        wstring const testName = L"ResetPartitionLoadTest";
        Trace.WriteInfo(TestFailoverUnitSource, "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        wstring metric = L"MyMetric";

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // Create one service with one partition - three replicas; Default loads are set
        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));

        Guid ftId = CreateGuid(0);

        FailoverUnitDescription ft1 = FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"S/0, S/1, P/2"), 0);
        plb.UpdateFailoverUnit(move(FailoverUnitDescription(ft1)));

        fm_->RefreshPLB(Stopwatch::Now());

        // Report load values
        std::map<Federation::NodeId, uint> SecondaryLoad;
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(0), 22));
        SecondaryLoad.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), 23));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 21, SecondaryLoad));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Check reported values
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 22, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 23, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 21, CreateNodeId(2)));

        // Reset load
        plb.ResetPartitionLoad(Reliability::FailoverUnitId(ft1.FUId), ft1.ServiceName, true);

        fm_->RefreshPLB(Stopwatch::Now());

        // Check load values after reset
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 10, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 10, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 10, CreateNodeId(2)));

        // Update service with the new default load values
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(wformatString("{0}/1.0/30/20", metric))));

        // Check load values after update
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 20, CreateNodeId(0)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Secondary, 20, CreateNodeId(1)));
        VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, ftId, metric, ReplicaRole::Primary, 30, CreateNodeId(2)));
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestFailoverUnit::ClassSetup()
    {
        Trace.WriteInfo("TestFailoverUnitSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestFailoverUnit::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestFailoverUnit::ClassCleanup()
    {
        Trace.WriteInfo("TestFailoverUnitSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
