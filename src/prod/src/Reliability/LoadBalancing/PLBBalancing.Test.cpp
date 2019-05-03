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


    class TestPLBBalancing
    {
    protected:
        TestPLBBalancing() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBBalancing()
        {
            BOOST_REQUIRE(ClassCleanup());
            Trace.WriteInfo("PLBBalancingTestSource", "Destructor called.");
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        void BalancingWithStandByReplicaHelper(wstring testName, PlacementAndLoadBalancing & plb);

        int BalancingWithDeactivatedNodeHelper(wstring testName, PlacementAndLoadBalancing & plb);
        int DefragWithDeactivatedNodeHelper(wstring testName, PlacementAndLoadBalancing & plb, wstring & serviceName);

        StopwatchTime BalancingNotNeededWithBlocklistedMetricHelper(
            PlacementAndLoadBalancing & plb,
            std::wstring metric,
            bool runPlbAndCheck);

        void BalancingThrottlingHelper(uint limit);

        void BalancingWithGlobalAndIsolatedMetricTest();
        void BalancingWithToBePlacedReplicasTest();

        void BalancingWithDownNodesHelper(wstring const& testName, bool replicasOnDownNode);

        // Enable test after bug 6591233 is fixed:
        void BalancingInterruptedByFuUpdatesTest();

        void SwapCostTestHelper(PlacementAndLoadBalancing & plb, wstring const& testName);

    void LoadBalancingWithShouldDisappearLoadTestHelper(wstring const& testName, bool featureSwitch);

        shared_ptr<TestFM> fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBBalancingSuite, TestPLBBalancing)
        /*
        BOOST_AUTO_TEST_CASE(BalancingWithGlobalAndIsolatedMetricTest2)
        {
        BalancingWithGlobalAndIsolatedMetricTest();
        }
        BOOST_AUTO_TEST_CASE(BalancingWithToBePlacedReplicasTest2)
        {
        BalancingWithToBePlacedReplicasTest();
        }*/

        BOOST_AUTO_TEST_CASE(BalancingBasicTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //simply spread primary replicas first, then spread secondary replicas
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move secondary *=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingMultipleServiceDomainsBasicTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingMultipleServiceDomainsBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric1/50,MyMetric2/50"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType1", false, CreateMetrics(L"MyMetric1/1.0/10/10")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType1", false, CreateMetrics(L"MyMetric2/1.0/10/10")));

        int fuId = 0;
        for (int i = 0; i < 4; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId++), wstring(L"TestService1"), 0, CreateReplicas(L"I/0"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId++), wstring(L"TestService2"), 0, CreateReplicas(L"I/0"), 0));
        }
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(static_cast<uint64>(plb.GetServiceDomains().size()), fm_->numberOfUpdatesFromPLB);
    }

    BOOST_AUTO_TEST_CASE(BalancingPreferredPrimary)
    {
        Trace.WriteInfo("PLBBalancingTestSource", " BalancingPreferredPrimary ");

        //this test checks whether we are considering proper loads during upgrade
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/50"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/15/10")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag));
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"S/0/L,P/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 1, CreateReplicas(L"S/0/L,P/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        //the cluster is perfectly balanced, no action should be needed (load is 30 on both nodes)
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBuiltInMetricTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithBuiltInMetricTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        vector<ServiceMetric> metrics;
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::PrimaryCountName), 1.0, 0, 0));
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::ReplicaCountName), 1.0, 0, 0));
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::CountName), 1.0, 0, 0));

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"I/0, I/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"I/2, I/3"), 0));

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        ScopeChangeObjectMaxSimulatedAnnealingIterations.SetValue(20);

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //every node will have 1 replica and 2 replica+instance
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move * 1=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2|3 move * 3=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithCapacityTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithCapacityTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Count/25"));
        plb.UpdateNode(CreateNodeDescription(1));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        for (int i = 0; i < 100; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"I/1"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //node0 has a capacity of 25, so there should only have 25 move
        VERIFY_ARE_EQUAL(25u, actionList.size());
        VERIFY_ARE_EQUAL(25, CountIf(actionList, ActionMatch(L"* move instance 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithCustomMetricTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithCustomMetricTest");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 200);
        fm_->Load();

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0)));

        fm_->UpdatePlb();

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 8, 3)); // P/2, S/0 is optimal placement
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"MyMetric", 4, 2));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"MyMetric", 2, 3));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService", L"MyMetric", 2, 1)); // P/1, S/0 is optimal placement
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService", L"MyMetric", 3, 2));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, L"MyMetric", 10);
        VerifyNodeLoadQuery(plb, 1, L"MyMetric", 10);
        VerifyNodeLoadQuery(plb, 2, L"MyMetric", 10);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBalancingThresholdTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithBalancingThresholdTest");
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        balancingThresholds.insert(make_pair(ServiceMetric::ReplicaCountName, 3));
        balancingThresholds.insert(make_pair(ServiceMetric::InstanceCountName, 1.0));
        balancingThresholds.insert(make_pair(L"MyMetric", 1.5));
        balancingThresholds.insert(make_pair(L"ZeroThresholdMetric", 0.0)); // if the balancing threshold is negative, the metric is always balanced
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0,ZeroThresholdMetric/1.0/0/0,ReplicaCount/1.0/0/0")));

        int fuIndex = 0;
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 20, 20));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"ZeroThresholdMetric", 1, 1));
            fuIndex++;
        }
        for (int i = 0; i < 25; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 20, 20));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"ZeroThresholdMetric", 100, 100));
            fuIndex++;
        }
        for (int i = 0; i < 60; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 10, 10));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"ZeroThresholdMetric", 10000, 10000));
            fuIndex++;
        }

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0u, actions.size());

        fm_->Clear();

        PLBConfig::KeyDoubleValueMap balancingThresholds2;
        balancingThresholds2.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        balancingThresholds2.insert(make_pair(ServiceMetric::ReplicaCountName, 3));
        balancingThresholds2.insert(make_pair(ServiceMetric::InstanceCountName, 1.0));
        balancingThresholds2.insert(make_pair(L"MyMetric", 1.49));
        ScopeChangeObjectMetricBalancingThresholds.SetValue(balancingThresholds2);
        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(actions.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBalancingThresholdTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithBalancingThresholdTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));

        int fuIndex = 0;
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
        }

        for (int i = 0; i < 25; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        }

        for (int i = 0; i < 30; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        }

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0u, actions.size());

        fm_->Clear();

        PLBConfigScopeChange(LocalBalancingThreshold, double, 1.49);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1);
        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(actions.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithScopedBalancingThresholdTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithScopedBalancingThresholdTest");
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"MyMetric", 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        blockList.insert(CreateNodeId(1));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        int fuIndex = 0;
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 20, 20));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 20, 20));
            fuIndex++;
        }
        for (int i = 0; i < 25; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"P/3"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 20, 20));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"P/3"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 20, 20));
            fuIndex++;
        }
        for (int i = 0; i < 60; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"P/4"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 10, 10));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"P/4"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 10, 10));
            fuIndex++;
        }

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0u, actions.size());

        fm_->Clear();

        PLBConfig::KeyDoubleValueMap balancingThresholds2;
        balancingThresholds2.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        balancingThresholds2.insert(make_pair(ServiceMetric::ReplicaCountName, 3));
        balancingThresholds2.insert(make_pair(ServiceMetric::InstanceCountName, 1.0));
        balancingThresholds2.insert(make_pair(L"MyMetric", 1.49));
        ScopeChangeObjectMetricBalancingThresholds.SetValue(balancingThresholds2);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1);
        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(actions.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithActivityThresholdTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithActivityThresholdTest");
        PLBConfig::KeyIntegerValueMap activityThresholds;
        activityThresholds.insert(make_pair(ServiceMetric::InstanceCountName, 30));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, CreateMetrics(L"InstanceCount/1.0/0/0")));

        int fuIndex = 0;
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));
        }
        for (int i = 0; i < 25; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService"), 0, CreateReplicas(L"I/1"), 0));
        }
        for (int i = 0; i < 30; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService"), 0, CreateReplicas(L"I/2"), 0));
        }

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());

        fm_->Clear();

        PLBConfig::KeyIntegerValueMap activityThresholds2;
        activityThresholds2.insert(make_pair(ServiceMetric::InstanceCountName, 29));
        ScopeChangeObjectMetricActivityThresholds.SetValue(activityThresholds2);
        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_IS_TRUE(actions.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithActivityThresholdTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithActivityThresholdTest2");
        PLBConfig::KeyIntegerValueMap activityThresholds;
        activityThresholds.insert(make_pair(L"MyMetric", 300));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"SystemType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"SystemService", L"SystemType", true));

        int fuIndex = 0;
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"I/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 1));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"I/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 1));
            fuIndex++;
        }
        for (int i = 0; i < 12; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"I/1"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 1));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"I/1"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 1));
            fuIndex++;
        }
        for (int i = 0; i < 15; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService1"), 0, CreateReplicas(L"I/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService1", L"MyMetric", 1));
            fuIndex++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService2"), 0, CreateReplicas(L"I/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService2", L"MyMetric", 1));
            fuIndex++;
        }
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"SystemService"), 0, CreateReplicas(L"P/0, S/1"), 0));

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());

        fm_->Clear();

        PLBConfig::KeyIntegerValueMap activityThresholds2;
        activityThresholds2.insert(make_pair(ServiceMetric::InstanceCountName, 299));
        ScopeChangeObjectMetricActivityThresholds.SetValue(activityThresholds2);
        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_IS_TRUE(actions.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithFaultDomainTest)
    {
        wstring testName = L"BalancingWithFaultDomainTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(5),
            FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(6),
            FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(7),
            FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(8),
            FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(9),
            FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(10),
            FailoverUnitDescription(CreateGuid(10), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(11),
            FailoverUnitDescription(CreateGuid(11), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));

        fm_->UpdatePlb();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        fm_->ApplyActions();
        fm_->UpdatePlb();

        fm_->RefreshPLB(Stopwatch::Now());

        // Each node should have 1 or 2 primary replicas
        std::vector<uint> expectedPrimaryValues;
        expectedPrimaryValues.push_back(1);
        expectedPrimaryValues.push_back(2);
        for (int i = 0; i < 8; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"PrimaryCount", expectedPrimaryValues);
        }

        // 4 nodes in the domain 0 should have 3 replicas each and other 4 nodes should have 6 replicas each
        std::vector<uint> expectedTotalValues;
        expectedTotalValues.push_back(3);
        expectedTotalValues.push_back(6);
        for (int i = 0; i < 8; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"Count", expectedTotalValues);
        }
    }

    BOOST_AUTO_TEST_CASE(BalancingWithFaultAndUpgradeDomainTest1)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithFaultAndUpgradeDomainTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc3", L"0"));

        plb.UpdateNode(CreateNodeDescription(2, L"dc0", L"1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1", L"1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc0", L"2"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1", L"2"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc2", L"2"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2", L"3"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc3", L"3"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));

        for (int i = 0; i < 10; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/3, S/6, S/8"), 0));
        }

        fm_->UpdatePlb();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0u);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * *=>*", value)) > 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithQuorumBasedSfrpCaseFdWithSixthNodeTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithQuorumBasedSfrpCaseFdWithSixthNodeTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateNode(CreateNodeDescription(i, wformatString(L"dc{0}", i), wformatString(L"{0}", i)));
        }

        plb.UpdateNode(CreateNodeDescription(5, L"dc0", L"1"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"", CreateMetrics(L""), false, 5));

        Reliability::FailoverUnitFlags::Flags noneFlag_;

        for (int i = 0; i < 6; ++i)
        {
            wstring replicas = wformatString(L"P/{0}", i % 5);
            for (int secondaryReplica = 0; secondaryReplica < 5; ++secondaryReplica)
            {
                if (secondaryReplica == i % 5)
                {
                    continue;
                }

                replicas.append(wformatString(L",S/{0}", secondaryReplica));
            }

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(replicas), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0u);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>5", value)));
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move secondary *=>5", value)) > 2);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move secondary 2|3|4=>5", value)) > 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithQuorumBasedSfrpCaseUdWithSixthNodeTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithQuorumBasedSfrpCaseUdWithSixthNodeTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateNode(CreateNodeDescription(i, wformatString(L"dc{0}", i), wformatString(L"{0}", i)));
        }

        plb.UpdateNode(CreateNodeDescription(5, L"dc1", L"0"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"", CreateMetrics(L""), false, 5));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        for (int i = 0; i < 6; ++i)
        {
            wstring replicas = wformatString(L"P/{0}", i % 5);
            for (int secondaryReplica = 0; secondaryReplica < 5; ++secondaryReplica)
            {
                if (secondaryReplica == i % 5)
                {
                    continue;
                }

                replicas.append(wformatString(L",S/{0}", secondaryReplica));
            }

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(replicas), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0u);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>5", value)));
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move secondary *=>5", value)) > 2);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move secondary 2|3|4=>5", value)) > 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wchar_t metricStr[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false, 5));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(5),
            FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(6),
            FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(7),
            FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(8),
            FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(9),
            FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(10),
            FailoverUnitDescription(CreateGuid(10), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(11),
            FailoverUnitDescription(CreateGuid(11), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/2, P/3"), 0)));

        fm_->UpdatePlb();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() < 23u);

        fm_->ApplyActions();
        fm_->UpdatePlb();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");

        // 8 nodes, 12 primary replicas => each node should have 1 or 2 primary replicas
        std::vector<uint> expectedPrimaryValues;
        expectedPrimaryValues.push_back(1);
        expectedPrimaryValues.push_back(2);

        for (int i = 0; i < 8; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"PrimaryCount", expectedPrimaryValues);
        }

        // 8 nodes, 36 replicas in total => each node should have 4 or 5 replicas
        std::vector<uint> expectedTotalValues;
        expectedTotalValues.push_back(4);
        expectedTotalValues.push_back(5);
        for (int i = 0; i < 8; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"Count", expectedTotalValues);
        }
    }

    BOOST_AUTO_TEST_CASE(BalancingWithQuorumBasedFaultAndUpgradeDomainTest)
    {
        wstring testName = L"BalancingWithQuorumBasedFaultAndUpgradeDomainTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);

        // Disable RG because the balancing action validation will fail
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc3", L"0"));

        plb.UpdateNode(CreateNodeDescription(2, L"dc0", L"1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1", L"1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc0", L"2"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1", L"2"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc2", L"2"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2", L"3"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc3", L"3"));

        plb.UpdateNode(CreateNodeDescription(9, L"dc3", L"4"));
        plb.UpdateNode(CreateNodeDescription(10, L"dc4", L"4"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wchar_t metricStr[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceId = 0;
        vector<int> initialReplicaPositions = { 0, 3, 6, 8, 10 };

        for (int targetReplicaCount = 1; targetReplicaCount <= 5; ++targetReplicaCount)
        {
            for (int serviceCount = 0; serviceCount < 6 - targetReplicaCount; ++serviceCount)
            {
                wstring serviceName = wformatString(L"TestService{0}", serviceId);

                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false, targetReplicaCount));

                wstring replicas = L"P/0";

                for (int i = 1; i < targetReplicaCount; ++i)
                {
                    replicas.append(wformatString(L",S/{0}", initialReplicaPositions[i]));
                }

                fm_->FuMap.insert(make_pair(CreateGuid(serviceId),
                    FailoverUnitDescription(CreateGuid(serviceId), wstring(serviceName), 0, CreateReplicas(replicas), 0)));

                serviceId++;
            }
        }

        // replicas are only on nodes: 0, 3, 6, 8, 10 -> with loads for the Count metric -> 15, 10, 6, 3, 1 -> 35 in total
        // primary replicas are only on the node 0 -> with the load for the PrimaryCount metric -> 15
        // let fast/slow balancing run for 10 times
        StopwatchTime now = Stopwatch::Now();
        for (int i = 0; i < 10; ++i)
        {
            fm_->UpdatePlb();
            fm_->RefreshPLB(now);
            fm_->ApplyActions();

            // temp test type, just to make cluster change so we have balancing running
            plb.UpdateServiceType(ServiceTypeDescription(wformatString(L"TestType{0}", i), set<NodeId>()));
        }

        fm_->UpdatePlb();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");

        // 11 nodes, 15 primary replicas => each node should have 1 or 2 primary replicas
        std::vector<uint> expectedPrimaryValues;
        expectedPrimaryValues.push_back(1);
        expectedPrimaryValues.push_back(2);

        for (int i = 0; i < 11; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"PrimaryCount", expectedPrimaryValues);
        }

        // 11 nodes, 35 replicas in total => each node should have 3 or 4 replicas
        std::vector<uint> expectedTotalValues;
        expectedTotalValues.push_back(3);
        expectedTotalValues.push_back(4);
        for (int i = 0; i < 11; ++i)
        {
            VerifyNodeLoadQuery(plb, i, L"Count", expectedTotalValues);
        }
    }

    BOOST_AUTO_TEST_CASE(BalancingByPercentage)
    {
        // Test scenario there are 4 types of nodes with capacities 10/20/30/40
        // All replicas are on node 3, and they suppose to get balanced
        wstring testName = L"BalancingByPercentage";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 10000);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 5);

        wstring metric = L"Metric";

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 0));
        PLBConfigScopeChange(PlacementStrategy, PLBConfig::KeyIntegerValueMap, placementStrategy);

        PLBConfig::KeyBoolValueMap balancingByPercentage;
        balancingByPercentage.insert(make_pair(metric, true));
        PLBConfigScopeChange(BalancingByPercentage, PLBConfig::KeyBoolValueMap, balancingByPercentage);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/10"));
        }
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/20"));
        }
        for (; nodeIndex < 6; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"C"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/30"));
        }
        for (; nodeIndex < 8; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"D"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/40"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"Metric/1.0/1/1";
        int services = 20;
        for (int i = 0; i < services; i++) {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(metricStr)));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/3"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        
        // Ideal balance would be 1/1/2/2/3/3/4/4 replicas on nodes 0-7, and in the start all replicas were on node 3
        // Small deviations from ideal balance are allowed

        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>0|1", value)) >= 1);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>0|1", value)) <= 3);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>2", value)) >= 1);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>2", value)) <= 3);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>4|5", value)) >= 5);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>4|5", value)) <= 7);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>6|7", value)) >= 7);
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 3=>6|7", value)) <= 9);
    }

    BOOST_AUTO_TEST_CASE(ReservationOfLoadWithBalancingByPercentage)
    {
        // Scenario:
        // Nodes:        0   1   2   3   4   5   6   7   8   9  10  11
        // Loads:       18  18  18  18  10  10  10  10  10  10  10  10 
        // Capacities:  22  22  22  22  22  22  22  22  22  22  16  16
        // Goal is to have 2 empty nodes with maximum capacity, and the rest is balanced by percentage
        // The ideal balance would be around 73%, which means around 16/22 usage on larger nodes and around 11-12/16 on smaller nodes, with 2 large nodes empty 

        wstring testName = L"ReservationOfLoadWithBalancingByPercentage";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        wstring metric = L"Metric";

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy, PLBConfig::KeyIntegerValueMap, placementStrategy);

        PLBConfig::KeyBoolValueMap balancingByPercentage;
        balancingByPercentage.insert(make_pair(metric, true));
        PLBConfigScopeChange(BalancingByPercentage, PLBConfig::KeyBoolValueMap, balancingByPercentage);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        int freeNodesTarget = 2;
        float defragWeight = 0.5f; // Balance beside emptying

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 10);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 100);

        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0.001);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeIndex = 0;
        for (; nodeIndex < 10; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"DB"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex % 5), wformatString(L"ud{0}", nodeIndex % 5), move(nodeProperties), L"Metric/22"));
        }
        for (; nodeIndex < 12; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"WF"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex % 5), wformatString(L"ud{0}", nodeIndex % 5), move(nodeProperties), L"Metric/16"));
        }
        
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypePrem"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 1;
        int serviceCount = 0;
        int failoverUnitId = 0;
        // Classic services are placed on all nodes, they have low moveCost and 2 replicas
        for (; serviceCount < 60; ++serviceCount)
        {
            serviceName = wformatString("Service_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(),                          //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                2,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}", serviceCount % 12, (serviceCount + 1) % 12)), 0));
            failoverUnitId++;
        }
        // Prem services are placed only on DB nodes, they are larger (size 4), have high moveCost and 4 replicas
        replicaSize = 4;
        for (; serviceCount < 62; ++serviceCount)
        {
            serviceName = wformatString("ServicePrem{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestTypePrem"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==DB"),           //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize)),
                FABRIC_MOVE_COST_HIGH,
                false,
                1,                                  // partition count
                4,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
            failoverUnitId++;
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // There should be max 2 moves to nodes 
        nodeIndex = 0;
        int emptyNodes = 0;
        int balancedNodes = 0;
        for (; nodeIndex < 4; ++nodeIndex) 
        {
            // Nodes 0-3 have more load and with higher moveCost replicas therefore they shouldn't be emptied
            auto fromNode = CountIf(actionList, ActionMatch(wformatString("* move * {0}=>*", nodeIndex), value));
            auto toNode = CountIf(actionList, ActionMatch(wformatString("* move * *=>{0}", nodeIndex), value));
            auto load = 18 - fromNode + toNode;
            if (load >= 15 && load <= 17)
            {
                balancedNodes++;
            }
        }

        for (; nodeIndex < 10; ++nodeIndex) 
        {
            // Node 4-9 are beneficial for emptying
            auto fromNode = CountIf(actionList, ActionMatch(wformatString("* move * {0}=>*", nodeIndex), value));
            auto toNode = CountIf(actionList, ActionMatch(wformatString("* move * *=>{0}", nodeIndex), value));
            auto load = 10 - fromNode + toNode;
            if (load == 0)
            {
                emptyNodes++;
            }
            if (load >= 15 && load <= 17)
            {
                balancedNodes++;
            }
        }

        for (; nodeIndex < 12; ++nodeIndex) 
        {
            // Nodes 10 and 11 (WF nodes) are too small for emptying, also they should have less load 11-12 (compared to DB nodes) to be considered balanced 
            auto fromNode = CountIf(actionList, ActionMatch(wformatString("* move * {0}=>*", nodeIndex), value));
            auto toNode = CountIf(actionList, ActionMatch(wformatString("* move * *=>{0}", nodeIndex), value));
            auto load = 10 - fromNode + toNode;
            if (load == 11 || load == 12)
            {
                balancedNodes++;
            }
        }
        // Nodes must be emptied. Expectation is that all the other nodes will be perfectly balanced, but criteria os lowered to 9/10
        VERIFY_IS_TRUE(emptyNodes == 2);
        VERIFY_IS_TRUE(balancedNodes >= 9);
        // Prem services shouldn't be moved
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"60|61 move * *=>*", value)), 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithAffinityTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService2", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary *=>2", value)), CountIf(actionList, ActionMatch(L"1 move primary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move secondary *=>2", value)), CountIf(actionList, ActionMatch(L"1 move secondary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move primary *=>2", value)), CountIf(actionList, ActionMatch(L"3 move primary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move secondary *=>2", value)), CountIf(actionList, ActionMatch(L"3 move secondary *=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithAffinityTest2: simulate lync's 3 nodes scenario");
        vector<ServiceMetric> metrics;
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::PrimaryCountName), 1.0, 0, 0));
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::SecondaryCountName), 1.0, 0, 0));

        vector<ServiceMetric> dummyMetrics;
        dummyMetrics.push_back(ServiceMetric(wstring(ServiceMetric::CountName), 0.0, 0, 0));

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ParentType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ChildType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P00", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C00", L"ChildType", true, L"GroupA_P00", vector<ServiceMetric>(dummyMetrics)));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P01", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C01", L"ChildType", true, L"GroupA_P01", vector<ServiceMetric>(dummyMetrics)));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P02", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C02", L"ChildType", true, L"GroupA_P02", vector<ServiceMetric>(dummyMetrics)));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P03", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C03", L"ChildType", true, L"GroupA_P03", vector<ServiceMetric>(dummyMetrics)));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P04", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C04", L"ChildType", true, L"GroupA_P04", vector<ServiceMetric>(dummyMetrics)));
        plb.UpdateService(CreateServiceDescription(L"GroupA_P05", L"ParentType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"GroupA_C05", L"ChildType", true, L"GroupA_P05", vector<ServiceMetric>(dummyMetrics)));

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P00"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C00"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P01"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C01"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P02"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C02"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P03"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C03"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P04"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C04"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P05"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C05"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityTest3)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithAffinityTest3");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wchar_t metricStr[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        // All 3 services affnitized to service0
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move primary 0=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithNonMovableReplicasTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithNonMovableReplicasTest");

        PLBConfigScopeChange(MoveCostMediumValue, int, 1100000000);
        PLBConfigScopeChange(MoveCostHighValue, int, 2000000000);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;


        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"SB/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"SB/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"SB/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"SB/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        // Report move cost: primary - high, secondary - medium. Move cost doesn't have UseSeparateSecondaryLoad supported, but it
        // requires that LoadOrMoveCostDescription use secondaryLoads map.
        std::map<Federation::NodeId, uint> secondaryMoveCost;
        secondaryMoveCost.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), FABRIC_MOVE_COST_MEDIUM));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(
            6, L"TestService3", *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
            FABRIC_MOVE_COST_HIGH, secondaryMoveCost));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManySingletonServicesTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithManySingletonServicesTest");
        PLBConfigScopeChange(FastBalancingTemperatureDecayRate, double, 0.6);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        for (int i = 0; i < 20; i++)
        {
            wstring serviceName = wstring(L"TestService") + StringUtility::ToWString(i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        // TODO: more robust validation
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManySingletonServicesMixedWithPartitionedServicesTest)
    {
        wstring testName = L"BalancingWithManySingletonServicesMixedWithPartitionedServicesTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(FastBalancingTemperatureDecayRate, double, 0.6);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        const int nodeCount = 5;
        const int singletonServiceCount = nodeCount * 40;
        const int partitionedServiceCount = 2;
        const int partitionCount = nodeCount * 2;

        PlacementAndLoadBalancing & plb = fm_->PLB;

        vector<ServiceMetric> metrics;
        metrics.push_back(ServiceMetric(wstring(ServiceMetric::PrimaryCountName), 1.0, 0, 0));

        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int fuId = 0;

        // singleton services
        for (int i = 0; i < singletonServiceCount / 2; i++)
        {
            wstring serviceName = wstring(L"SingletonService") + StringUtility::ToWString(i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, vector<ServiceMetric>(metrics)));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        // partitioned services
        for (int i = 0; i < 2; i++)
        {
            wstring serviceName = wstring(L"PartitionedService") + StringUtility::ToWString(i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, vector<ServiceMetric>(metrics)));
            for (int j = 0; j < partitionCount; j++)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            }
        }

        // singleton services again
        for (int i = singletonServiceCount / 2; i < singletonServiceCount; i++)
        {
            wstring serviceName = wstring(L"SingletonService") + StringUtility::ToWString(i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, vector<ServiceMetric>(metrics)));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // verify the global domain is balanced
        int totalPartitionCount = singletonServiceCount + partitionedServiceCount * partitionCount;
        int replicaPerNode = totalPartitionCount / nodeCount;

        VERIFY_ARE_EQUAL(replicaPerNode, CountIf(actionList, ActionMatch(L"* move primary *=>1", value)));
        VERIFY_ARE_EQUAL(replicaPerNode, CountIf(actionList, ActionMatch(L"* move primary *=>2", value)));
        VERIFY_ARE_EQUAL(replicaPerNode, CountIf(actionList, ActionMatch(L"* move primary *=>3", value)));
        VERIFY_ARE_EQUAL(replicaPerNode, CountIf(actionList, ActionMatch(L"* move primary *=>4", value)));

        // verify the local domain for one of the partitioned service is balanced
        int replicaPerNodeForPartitionedService = partitionCount / nodeCount;
        wstring partitionIdPattern;
        for (int i = 0; i < partitionCount; i++)
        {
            partitionIdPattern += (i == 0 ? L"" : L"|") + StringUtility::ToWString(i + singletonServiceCount / 2);
        }
        VERIFY_ARE_EQUAL(replicaPerNodeForPartitionedService, CountIf(actionList, ActionMatch(partitionIdPattern + L" move primary *=>1", value)));
        VERIFY_ARE_EQUAL(replicaPerNodeForPartitionedService, CountIf(actionList, ActionMatch(partitionIdPattern + L" move primary *=>2", value)));
        VERIFY_ARE_EQUAL(replicaPerNodeForPartitionedService, CountIf(actionList, ActionMatch(partitionIdPattern + L" move primary *=>3", value)));
        VERIFY_ARE_EQUAL(replicaPerNodeForPartitionedService, CountIf(actionList, ActionMatch(partitionIdPattern + L" move primary *=>4", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithEmptyPlacementObjectTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithEmptyPlacementObjectTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        for (int i = 0; i < 20; i++)
        {
            wstring serviceName = wstring(L"TestService") + StringUtility::ToWString(i);
            plb.UpdateService(CreateServiceDescription(
                serviceName, L"TestType", true, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, vector<ReplicaDescription>(), 0));
        }

        // The placement object will contain no services, partitions or replicas
        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0u, GetActionRawCount(fm_->MoveActions));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithScoreImprovementThresholdTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithScoreImprovementThresholdTest");
        PLBConfigScopeChange(ScoreImprovementThreshold, double, 0.1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/0.0/0/0")));

        int fuIndex = 0;
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 20, 20));
            fuIndex++;
        }
        for (int i = 0; i < 25; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 20, 20));
            fuIndex++;
        }
        for (int i = 0; i < 30; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(fuIndex, L"TestService", L"MyMetric", 20, 20));
            fuIndex++;
        }

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0u, actions.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithSwapPrimaryThrottlingTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithSwapPrimaryThrottlingTest");
        PLBConfigScopeChange(SwapPrimaryThrottlingEnabled, bool, true);
        PLBConfigScopeChange(SwapPrimaryThrottlingAssociatedMetric, wstring, L"Metric1");
        PLBConfigScopeChange(SwapPrimaryThrottlingGlobalMaxValue, int, 10);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"Metric1/1.0/1/1,Metric2/1.0/1/1")));

        Reliability::FailoverUnitFlags::Flags swappingPrimaryFlag;
        swappingPrimaryFlag.SetSwappingPrimary(true);

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/1/B, S/0"), 0, swappingPrimaryFlag));
        }
        for (int i = 5; i < 60; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0));
        }

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, CountIf(
            actionList, ActionMatch(L"* move primary *=>*", value) + ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingUnstableFUsTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingUnstableFUsTest");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, false);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::FromTicks(1));
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"BalancingUnstableFUsTest_TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"BalancingUnstableFUsTest_TestService", L"BalancingUnstableFUsTest_TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"BalancingUnstableFUsTest_TestService"), 0, CreateReplicas(L"P/0, S/1"), 1));

        Sleep(1); // add a slight delay so we can control the searches PLB performs
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        ScopeChangeObjectMaxSimulatedAnnealingIterations.SetValue(20);
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now); // call it twice so that the second call should do the balancing

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //should have 3 movements
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingMinimizeUnnecessaryMovementsTest)
    {
        wstring testName = L"BalancingMinimizeUnnecessaryMovementsTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        int nodeCount = 10;
        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/0")));

        int fuIndex = 0;
        for (int i = 0; i < nodeCount; i++)
        {
            int targetNode;
            if (i < 3)
            {
                targetNode = 0;
            }
            else if (i < 5)
            {
                targetNode = 1;
            }
            else
            {
                targetNode = i - 3;
            }
            for (int j = 0; j < 10; j++)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"TestService"), 0, CreateReplicas(wformatString("P/{0}", targetNode).c_str()), 0));
            }
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(30u, actionList.size());
        VERIFY_ARE_EQUAL(20, CountIf(actionList, ActionMatch(L"* move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move primary 1=>*", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move primary *=>7", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move primary *=>8", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move primary *=>9", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingMinimizeUnnecessaryMovementsTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingMinimizeUnnecessaryMovementsTest2");
        PLBConfigScopeChange(MaxPercentageToMove, double, 0.5);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeCount = 3;
        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"")));

        for (int i = 0; i < 300; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(200u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingMinimizeUnnecessaryMovementsOptimalTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingMinimizeUnnecessaryMovementsOptimalTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeCount = 3;
        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 100, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"MyMetric", 100, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"MyMetric", 1, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService", L"MyMetric", 99, 0));

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        ScopeChangeObjectMaxSimulatedAnnealingIterations.SetValue(20);

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // this solution will not be discovered by the overflow->underflow only search, but will be found by the more exhaustive search
        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithInvalidFaultDomainTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithInvalidFaultDomainTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/r0/c1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/r1/c1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, count_if(actionList.begin(), actionList.end(), [](wstring const& a)
        {
            return ActionMatch(L"* move * *=>0", a);
        }));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithInvalidFaultDomainTestWithDelay)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithInvalidFaultDomainTestWithDelay");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/r0/c1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/r1/c1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/4"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());
        }


        vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

        //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
        VERIFY_ARE_EQUAL(0u, actionList0.size());

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());

        VERIFY_ARE_EQUAL(1, count_if(actionList.begin(), actionList.end(), [](wstring const& a)
        {
            return ActionMatch(L"* move * *=>0", a);
        }));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithServiceTypeBlockListTest)
    {
        wstring testName = L"BalancingWithServiceTypeBlockListTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);
        // service type disabled on some nodes, so balancing shouldn't move existing replicas to those nodes

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(4));
        blockList.insert(CreateNodeId(5));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType1", true));
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(4));
        blockList2.insert(CreateNodeId(5));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType2", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(12u, actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * *=>4", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * *=>5", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithServiceTypeBlockListTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithServiceTypeBlockListTest2: should not move service type disabled replicas for balancing");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType1", true));
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType2", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * 0=>*", value)));
        VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(L"* move * 1=>*", value)) > 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingInitialTemperatureTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingInitialTemperatureTest");
        PLBConfig & config = PLBConfig::GetConfig();

        auto globalMetricWeights = PLBConfig::KeyDoubleValueMap();
        globalMetricWeights.insert(make_pair(L"Metric1", 1.0));
        globalMetricWeights.insert(make_pair(L"Metric2", 1.0));
        config.GlobalMetricWeights = globalMetricWeights;

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeCount = 8;
        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"Metric1/0.0/50/50,Metric2/0.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric2", 45, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"Metric2", 30, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"Metric2", 30, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService", L"Metric1", 40, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService", L"Metric2", 45, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService", L"Metric2", 30, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService", L"Metric2", 45, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(6, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(6, L"TestService", L"Metric2", 30, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(7, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(7, L"TestService", L"Metric2", 45, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L"P/4"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(8, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(8, L"TestService", L"Metric2", 45, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L"P/4"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(9, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(9, L"TestService", L"Metric2", 30, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService"), 0, CreateReplicas(L"P/5"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(10, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(10, L"TestService", L"Metric2", 30, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService"), 0, CreateReplicas(L"P/5"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(11, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(11, L"TestService", L"Metric2", 45, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(12), wstring(L"TestService"), 0, CreateReplicas(L"P/6"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(12, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(12, L"TestService", L"Metric2", 45, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(13), wstring(L"TestService"), 0, CreateReplicas(L"P/6"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(13, L"TestService", L"Metric1", 10, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(13, L"TestService", L"Metric2", 30, 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(14), wstring(L"TestService"), 0, CreateReplicas(L"P/7"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(14, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(14, L"TestService", L"Metric2", 45, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(15), wstring(L"TestService"), 0, CreateReplicas(L"P/7"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(15, L"TestService", L"Metric1", 45, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(15, L"TestService", L"Metric2", 30, 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() > 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingPlacementObjectCreationVerificationTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingPlacementObjectCreationVerificationTest");
        PLBConfigScopeChange(MaxMovementExecutionTime, TimeSpan, TimeSpan::FromTicks(0));
        PLBConfigScopeChange(AvgStdDevDeltaThrottleThreshold, double, 100.0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"Service", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service"), 0, CreateReplicas(L"P/0"), 0));

        // Will do balancing
        fm_->RefreshPLB(Stopwatch::Now());

        fm_->Clear();

        // due to the MaxMovementExecutionTime setting the scheduler would reset; but due to the AvgStdDevDeltaThrottleThreshold we won't do balancing
        // this still creates the placement object
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithPrimaryReplicaFDConstraintsTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithPrimaryReplicaFDConstraintsTest");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack1"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^P fd:/dc0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/5"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() > 0);
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move primary 0=>*", value)));

    }

    // Test with all nodes down in the preferred domain scenarios
    BOOST_AUTO_TEST_CASE(BalancingWithPrimaryReplicaFDConstraintsTest2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithPrimaryReplicaFDConstraintsTest2");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        //plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        //plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^P fd:/dc0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/2, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VERIFY_IS_TRUE(plb.GetServiceDomains().begin()->schedulerAction_.Action >  PLBSchedulerActionType::ConstraintCheck);

    }

    BOOST_AUTO_TEST_CASE(BalancingWithFDNonpackingDistributionTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithFDNonpackingDistributionTest");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r2"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2/r0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc2/r1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FDPolicy ~ Nonpacking"));

        // DC0 has two replica, which violate the FD distribution policy
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/2, S/6"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithFailoverUnitVersionChangeEverythingDropped)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithFailoverUnitVersionChangeEverythingDropped");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Necessary in order for InterruptSearcherThread to run
        PLBConfigScopeChange(IsTestMode, bool, true);

        // Make sure that this code path is disabled
        PLBConfigScopeChange(InterruptBalancingForAllFailoverUnitUpdates, bool, false);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));

        plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"I/0"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 1, CreateReplicas(L"I/0"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 1, CreateReplicas(L"I/0"), 0));
            plb.ProcessPendingUpdatesPeriodicTask();
            plb.InterruptSearcherRunFinished = true;
        }, true);

        plb.InterruptSearcherRunFinished = false;

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingShouldSwapTest)
    {
        wstring testName = L"BalancingShouldSwapTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring balancingMetric = L"BalancingMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(IsTestMode, bool, false); // TODO: remove with fix for 3422258
                                                       // PLBConfigScopeChange(SwapPrimaryProbability, double, 1); // TODO: allow with fix for 3422258

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/100", balancingMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/10", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3, S/2"), 0));

        // Initial node loads:
        // Balancing metric:  100, 10,  100, 10   limit: 100
        // Primary replica with load 40 is on the node 0 and primary replica with load 10 is on node 3
        // PLB should swap primary with 40 to the node 1 and primary with 10 to the node 2 in order to get this state:
        // Balancing metric:   60, 50,   60, 50,  limit: 100
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 swap primary 3<=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingShouldNotSwapTest)
    {
        wstring testName = L"BalancingShouldNotSwapTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring balancingMetric = L"BalancingMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(SwapPrimaryProbability, double, 1);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/100", balancingMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/10", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/50", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3, S/2"), 0));

        // Initial node loads:
        // Balancing metric:   50, 60,   50, 60   limit: 100
        // Primary replica with load 40 is on the node 0 and primary replica with load 10 is on node 3
        // PLB should NOT swap anything
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragShouldNotWorkTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "DefragShouldNotWorkTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 2));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i / 2), wformatString("{0}/100", defragMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/2"), 0));

        for (int i = 0; i < 3; i++)
        {
            serviceName = wformatString("TestService{0}", index);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/3"), 0));
        }

        //  initial state
        //  MetricBalancingThresholds is 2
        //  Every max/min domain load ratio is not <= 2
        //  0   0   1   1
        //  -------------
        //      3       3
        //      2       2
        //  0   1   0   1
        //  -------------
        //  0   1   2   3
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }
    BOOST_AUTO_TEST_CASE(ResetPartitionLoadTest)
    {
        wstring testName = L"ResetPartitionLoadTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));

        FailoverUnitDescription ft1 = FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/0"), 0);
        FailoverUnitDescription ft2 = FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2"), 0);

        plb.UpdateFailoverUnit(move(FailoverUnitDescription(ft1))); // P/2, S/0 is optimal placement
        plb.UpdateFailoverUnit(move(FailoverUnitDescription(ft2)));
        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        fm_->Clear();

        // Update move cost
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"Metric1", 10, 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"Metric1", 10, 10));
        plb.ProcessPendingUpdatesPeriodicTask();
        now += (PLBConfig::GetConfig().MaxMovementExecutionTime + PLBConfig::GetConfig().MinLoadBalancingInterval);
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        // 1 move is needed (any partition is OK, loads are the same).
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 1=>3", value)));

        // reset to default load
        fm_->Clear();
        plb.ResetPartitionLoad(Reliability::FailoverUnitId(ft1.FUId), ft1.ServiceName, true);
        plb.ResetPartitionLoad(Reliability::FailoverUnitId(ft2.FUId), ft2.ServiceName, true);

        plb.ProcessPendingUpdatesPeriodicTask();
        now += (PLBConfig::GetConfig().MaxMovementExecutionTime + PLBConfig::GetConfig().MinLoadBalancingInterval);
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        // there should be no movement after reset because default load are all-0
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragShouldWorkWithoutBalancingPartitionsTest)
    {
        wstring testName = L"DefragShouldWorkWithoutBalancingPartitionsTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";
        wstring balancingMetric = L"BalancingMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 3));
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i / 2), wformatString("{0}/100,{1}/100", defragMetric, balancingMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int index = 0;

        // defrag partitions:
        wstring serviceName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/2"), 0));

        for (int i = 0; i < 3; i++)
        {
            serviceName = wformatString("TestService{0}", index);
            plb.UpdateService(CreateServiceDescription(
                serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/3"), 0));
        }

        // balancing partitions:
        serviceName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/2"), 0));

        serviceName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", balancingMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/3"), 0));

        //  initial state for defrag partitions:
        //  MetricBalancingThresholds is 3
        //  Every max/min domain load ratio is <= 3
        //  0   0   1   1
        //  -------------
        //      3       3
        //      2       2
        //  0   1   0   1
        //  -------------
        //  0   1   2   3
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary *=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary *=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragShouldWorkTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "DefragShouldWorkTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        double metricBalancingThreshold = 1;
        if (rand() % 2 == 0)
        {
            metricBalancingThreshold = 3;
        }

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, metricBalancingThreshold));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i / 2), wformatString("{0}/100", defragMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int index = 0;

        // defrag partitions:
        wstring serviceName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/3"), 0));

        for (int i = 0; i < 3; i++)
        {
            serviceName = wformatString("TestService{0}", index);
            plb.UpdateService(CreateServiceDescription(
                serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/3"), 0));
        }

        //  initial state for defrag partitions:
        //  When MetricBalancingThresholds is 3 defrag should work as one max/min domain load ratio is <= 3
        //  When MetricBalancingThresholds is 1 defrag should work always
        //  0   0   1   1
        //  -------------
        //              3
        //      3       2
        //      2       1
        //  0   1       0
        //  -------------
        //  0   1   2   3
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragShouldSwapTest)
    {
        wstring testName = L"DefragShouldSwapTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(SwapPrimaryProbability, double, 1);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2, S/3"), 0));

        // Initial node loads:
        // Defrag metric:   60, 50,   60, 50   limit: 100   avg: 55
        // Primary replica with load 50 is on the node 1 and primary replica with load 10 is on node 2
        // PLB should swap primary with 50 to the node 0 and primary with 10 to the node 3 in order to get this state:
        // Defrag metric:  100, 10,  100, 10,  limit: 100   avg: 55
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 1<=>0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 swap primary 2<=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragShouldNotSwapTest)
    {
        wstring testName = L"DefragShouldNotSwapTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(SwapPrimaryProbability, double, 1);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2, S/3"), 0));

        // Initial node loads:
        // Defrag metric:   10, 100,   10, 100   limit: 100   avg: 55
        // Primary replica with load 50 is on the node 1 and primary replica with load 10 is on node 2
        // PLB should NOT swap anything, it can only merge two replicas with load 10 but only swaps are allowed
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragWithHighMoveCost)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "DefragWithHighMoveCost");
        wstring defragMetric = L"DefragMetric";

        Common::Random random;

        fm_->Load(random.Next());

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        // Balancing threshold of 1 to always execute defrag
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);
        // Use reported move cost when calculating score
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i), wformatString("{0}/100", defragMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"DefragWithHighMoveCost_TestType"), set<NodeId>()));

        // Create one service with one partition - two replicas with load 1000 each
        wstring serviceName = L"DefragWithHighMoveCost_TestService";
        plb.UpdateService(CreateServiceDescription(
            serviceName, L"DefragWithHighMoveCost_TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        //  Report high move cost for the service that we just created.
        std::map<Federation::NodeId, uint> secondaryMoveCost;
        secondaryMoveCost.insert(make_pair<Federation::NodeId, uint>(CreateNodeId(1), FABRIC_MOVE_COST_HIGH));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(
            0, L"DefragWithHighMoveCost_TestService", *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
            FABRIC_MOVE_COST_HIGH, secondaryMoveCost));

        //  Starting score will be negative due to the fact that we have two replicas with high load.
        //  When searching for solution, we will multiply the score with move cost to calculate the energy.
        //  More replicas we move, energy will be lower even if score remains the same (as in this case).
        //  PLB should be able to recognize this situation and not to allow useless moves.

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyPLBAction(plb, L"LoadBalancing");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragTogetherWithBalancingTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "DefragTogetherWithBalancingTest");
        wstring defragMetric = L"DefragMetric";
        wstring balancingMetric = L"BalancingMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping DefragTogetherWithBalancingTest (IsTestMode == false).");
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 6;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % 3), wformatString("{0}/100,{1}/100", defragMetric, balancingMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int replicaIndex = 0;
        for (int i = 0; i < 6; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(
                serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10,{1}/1.0/1/0", defragMetric, balancingMetric))));

            wstring replicas = wformatString("P/{0}, S/{1}, S/{2}", replicaIndex, replicaIndex + 1, replicaIndex + 2);
            replicaIndex += 3;
            replicaIndex %= nodes;

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(replicas), 0));
        }

        //  initial state:
        //  0   1   2   0   1   2
        //  ---------------------
        // (4)  4   4  (5)  5   5
        // (2)  2   2  (3)  3   3
        // (0)  0   0  (1)  1   1
        //  ---------------------
        //  0   1   2   3   4   5
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        for (wstring action : actionList)
        {
            Trace.WriteInfo("PLBBalancingTestSource", "Action: {0}", action);
        }

        int primaryReplicas[6] = { 3, 0, 0, 3, 0, 0 };
        int replicas[6] = { 3, 3, 3, 3, 3, 3 };

        for (auto const& actionPair : fm_->MoveActions)
        {
            vector<FailoverUnitMovement::PLBAction> const& allActions = actionPair.second.Actions;
            for (FailoverUnitMovement::PLBAction currentAction : allActions)
            {
                switch (currentAction.Action)
                {
                case FailoverUnitMovementType::MoveSecondary:
                case FailoverUnitMovementType::MoveInstance:
                    replicas[GetNodeId(currentAction.SourceNode)]--;
                    replicas[GetNodeId(currentAction.TargetNode)]++;
                    break;

                case FailoverUnitMovementType::MovePrimary:
                    replicas[GetNodeId(currentAction.SourceNode)]--;
                    replicas[GetNodeId(currentAction.TargetNode)]++;
                    primaryReplicas[GetNodeId(currentAction.SourceNode)]--;
                    primaryReplicas[GetNodeId(currentAction.TargetNode)]++;
                    break;

                case FailoverUnitMovementType::SwapPrimarySecondary:
                    primaryReplicas[GetNodeId(currentAction.SourceNode)]--;
                    primaryReplicas[GetNodeId(currentAction.TargetNode)]++;
                    break;

                case FailoverUnitMovementType::AddPrimary:
                    primaryReplicas[GetNodeId(currentAction.TargetNode)]++;
                    break;

                case FailoverUnitMovementType::AddSecondary:
                case FailoverUnitMovementType::AddInstance:
                    replicas[GetNodeId(currentAction.TargetNode)]++;
                    break;

                case FailoverUnitMovementType::RequestedPlacementNotPossible:
                    break;

                default:
                    Assert::CodingError("Unknown Replica Move Type");
                    break;
                }
            }
        }

        int nodesWithMoreThan3Replicas = 0;
        int nodesWith1PrimaryReplica = 0;
        for (int i = 0; i < nodes; i++)
        {
            Trace.WriteInfo("PLBBalancingTestSource", "Node {0} replicas {1} primary replicas {2}", i, replicas[i], primaryReplicas[i]);
            if (replicas[i] > 3)
            {
                nodesWithMoreThan3Replicas++;
            }
            if (primaryReplicas[i] == 1)
            {
                nodesWith1PrimaryReplica++;
            }
        }

        VERIFY_IS_TRUE(nodesWithMoreThan3Replicas > 0);
        VERIFY_ARE_EQUAL(nodesWith1PrimaryReplica, nodes);
    }

    BOOST_AUTO_TEST_CASE(DefragTogetherWithBalancingPerformDefragSwapToOverloadedTest)
    {
        wstring testName = L"DefragTogetherWithBalancingPerformDefragSwapToOverloadedTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring balancingMetric = L"BalancingMetric";
        wstring defragMetric = L"DefragMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(SwapPrimaryProbability, double, 1);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 2;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/100,{1}/100", balancingMetric, defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/1/1/1,{1}/1/50/50", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/1/49/50,{1}/1/10/50", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        // Initial node loads:
        // Balancing metric:  50,  50   limit: 100   avg: 50
        // Defrag metric:     60,  50   limit: 100   avg: 55

        // Primary replica with load 50 is on the node 1 for defrag metric
        // PLB should do swap in order to get this state:
        // Balancing metric:  51,  49   limit: 100   avg: 50
        // Defrag metric:    100,  10   limit: 100   avg: 55
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragTogetherWithBalancingPerformBalancingSwapToOverloadedTest)
    {
        wstring testName = L"DefragTogetherWithBalancingPerformBalancingSwapToOverloadedTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";
        wstring balancingMetric = L"BalancingMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(IsTestMode, bool, false); // TODO: remove with fix for 3422258

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodes = 4;
        for (int i = 0; i < nodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % nodes), wformatString("{0}/120,{1}/100", balancingMetric, defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/1/60/10,{1}/1.0/10/10", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        for (int serviceIterator = 0; serviceIterator < 2; serviceIterator++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(
                serviceName, testType, true, CreateMetrics(wformatString("{0}/1/30/5,{1}/1.0/10/10", balancingMetric, defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/1/70/70,{1}/1.0/10/10", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2, S/3"), 0));

        // Initial node loads:
        // Defrag metric:    100, 100, 10, 10 limit: 100
        // Balancing metric: 120,  20, 70, 70 limit: 120

        // PLB should aim to have node loads:
        // Defrag metric:    100, 100, 10, 10 limit: 100
        // Balancing metric:  70,  70, 70, 70 limit: 120
        // Final state can be achieved with SWAP primary replica of partition 0 from Node0 to the Node1.
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragMoveOvercommitInIntermediateStateTest)
    {
        wstring testName = L"DefragMoveOvercommitInIntermediateStateTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/70", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = L"DefragMoveOvercommitInIntermediateStateType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/0", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/0", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/30/0", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/0", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        // Node0 load is 50, replicas loads: 40, 10
        // Node1 load is 40, replicas loads: 30, 10
        // Defrag should aim to have node loads 70 and 20 but it should NOT overcommit nodes in intermediate state.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 move primary 0 => 1", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"2 move primary 1 => 0", value)));
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragSwapOvercommitInIntermediateStateTest)
    {
        wstring testName = L"DefragSwapOvercommitInIntermediateStateTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";
        wstring balancingMetric = L"BalancingMetric";

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        balancingThresholds.insert(make_pair(balancingMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % 2), wformatString("{0}/200,{1}/1000", balancingMetric, defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = L"DefragSwapOvercommitInIntermediateStateType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;

        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/0.1/80/10,{1}/1.0/10/10", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            serviceName, testType, true, CreateMetrics(wformatString("{0}/0.1/80/10,{1}/1.0/10/10", balancingMetric, defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1, S/0"), 0));

        // 6 replicas on Node0 and 8 replicas on Node1, in order to have balancing metric not balanced (IsBalanced)
        for (int i = 0; i < 14; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(
                serviceName, testType, true, CreateMetrics(wformatString("{0}/0.1/10/10,{1}/1.0/10/10", balancingMetric, defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i < 6 ? 0 : 1)), 0));
        }

        // Initial node loads:
        // Balancing metric: 150, 170, limit:  200
        // Defrag metric:     80, 100, limit: 1000

        // PLB should aim to have node loads:
        // Balancing metric: 160, 160, limit:  200
        // Defrag metric:     20, 160, limit: 1000
        // Final state can be achieved with SWAP primary to the Node0 and 6 moves out of the Node0.
        // But that should NOT happen as swap will make node over capacity limit.
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0);
        VERIFY_IS_TRUE(actionList.size() < 6);
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithMoveCost)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithMoveCost");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping BalancingWithMoveCost (IsTestMode == false).");
            return;
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);
        // Use reported move cost when calculating score
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        // Force processing of pending updates
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 50, 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"MyMetric", 25, 25));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"MyMetric", 25, 25));

        std::map<Federation::NodeId, uint> secondaryMoveCost;
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName, FABRIC_MOVE_COST_HIGH, secondaryMoveCost));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // We should move only replicas with low move cost
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 move primary *=>*", value)));

        fm_->Clear();

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName, FABRIC_MOVE_COST_ZERO, secondaryMoveCost));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);

        // We should move only one replica with zero move cost
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary *=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithMoveCostIgnored)
    {
        // Test to verify that PLB is ignoring move cost reports when feature is off.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithMoveCost");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping BalancingWithMoveCost (IsTestMode == false).");
            return;
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);
        // Do not use reported move cost when calculating score
        PLBConfigScopeChange(UseMoveCostReports, bool, false);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        // Force processing of pending updates
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 50, 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"MyMetric", 25, 25));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"MyMetric", 25, 25));

        std::map<Federation::NodeId, uint> secondaryMoveCost;
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName, FABRIC_MOVE_COST_HIGH, secondaryMoveCost));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Since we ignore move cost report, we should make smallest number of moves possible and move the replica with high cost
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary *=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithZeroSwapCost)
    {
        // Test case: move has high cost, swap has zero - prefer swap
        wstring testName = L"BalancingWithZeroSwapCost";
        // Test to verify that PLB is ignoring move cost reports when feature is off.
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Swaps will have cost of zero
        PLBConfigScopeChange(SwapCost, double, 0.0);
        // Use reported move cost scored
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        SwapCostTestHelper(plb, testName);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithHighSwapCost)
    {
        // Test case: move has high cost, swap has zero - prefer swap
        wstring testName = L"BalancingWithZeroSwapCost";
        // Test to verify that PLB is ignoring move cost reports when feature is off.
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Swaps will have high move cost
        PLBConfigScopeChange(SwapCost, double, 1.0 * PLBConfig::GetConfig().MoveCostHighValue);
        // Use reported move cost scored
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        SwapCostTestHelper(plb, testName);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() > 0u);
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDeletingServiceSplitDomainTest1)
    {
        // this will test a delete service senario without domain split
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithDeletingServiceSplitDomainTest1");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/20"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType0"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"BalancingWithDeletingServiceSplitDomainTest1_Service0",
            L"ServiceType0", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"BalancingWithDeletingServiceSplitDomainTest1_Service1",
            L"ServiceType0", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"BalancingWithDeletingServiceSplitDomainTest1_Service2",
            L"ServiceType0", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"BalancingWithDeletingServiceSplitDomainTest1_Service0"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"BalancingWithDeletingServiceSplitDomainTest1_Service1"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"BalancingWithDeletingServiceSplitDomainTest1_Service2"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"BalancingWithDeletingServiceSplitDomainTest1_Service0", L"MyMetric", 11, 5));

        plb.DeleteFailoverUnit(L"BalancingWithDeletingServiceSplitDomainTest1_Service1", CreateGuid(1));
        plb.DeleteService(L"BalancingWithDeletingServiceSplitDomainTest1_Service1");

        // split domain will run, no domain should be splited
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        // node 0 would be over capacity, primary can only be swapped with S/3 to correct the capacity violation
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDeletingServiceSplitDomainTest2)
    {
        // this will test a delete service senario with domain split
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithDeletingServiceSplitDomainTest2");

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric1/10"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric1/20"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType0"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"BalancingWithDeletingServiceSplitDomainTest2_Service0",
            L"ServiceType0", L"DummyApp0", true, CreateMetrics(L"MyMetric1/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"BalancingWithDeletingServiceSplitDomainTest2_Service1",
            L"ServiceType0", L"DummyApp1", true, CreateMetrics(L"MyMetric1/1.0/0/0,MyMetric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"BalancingWithDeletingServiceSplitDomainTest2_Service2",
            L"ServiceType0", L"DummyApp2", true, CreateMetrics(L"MyMetric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"BalancingWithDeletingServiceSplitDomainTest2_Service0"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"BalancingWithDeletingServiceSplitDomainTest2_Service1"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"BalancingWithDeletingServiceSplitDomainTest2_Service2"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"BalancingWithDeletingServiceSplitDomainTest2_Service0", L"MyMetric1", 11, 5));

        plb.DeleteFailoverUnit(L"BalancingWithDeletingServiceSplitDomainTest2_Service1", CreateGuid(1));
        plb.DeleteService(L"BalancingWithDeletingServiceSplitDomainTest2_Service1");

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        // node 0 would be over capacity, primary can only be swapped with S/3 to correct the capacity violation
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(BalancingWithSeparateSecondaryLoadTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithSeparateSecondaryLoadTest");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/4/8")));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 9));
        loads.insert(make_pair(CreateNodeId(2), 8));
        // Loads initially is (4+4), (8+1), 8
        // Loads after upddate is is (4+4), (9+1), 8
        // After refresh, service1 secondary should be moved to node2 to make the loads as (4+4), 9, (8+1)
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService0", L"MyMetric", 4, loads));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move secondary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingNodeLoadsTrace)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNodeLoadsTrace");
        PLBConfigScopeChange(NodeLoadsTracingInterval, TimeSpan, TimeSpan::FromSeconds(0.1));

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 70; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"m1/10, m2/10, m3/10, m4/10, m5/10, m6/10, m7/10, m8/10, m9/10, m10/10, m11/10, m12/10, m13/10, m14/10, m15/10, m16/10, m17/10, m18/10, m19/10, m20/10"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"BalancingNodeLoadsTrace_Service0", L"ServiceType0", true,
            CreateMetrics(L"m1/1.0/1/1, m2/1.0/1/1, m3/1.0/1/1, m4/1.0/1/1, m5/1.0/1/1, m6/1.0/1/1, m7/1.0/1/1, m8/1.0/1/1, m9/1.0/1/1, m10/1.0/1/1, m11/1.0/1/1, m12/1.0/1/1, m13/1.0/1/1, m14/1.0/1/1, m15/1.0/1/1, m16/1.0/1/1, m17/1.0/1/1, m18/1.0/1/1, m19/1.0/1/1, m20/1.0/1/1")));

        for (int j = 1; j < 100; j++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(j), wstring(L"BalancingNodeLoadsTrace_Service0"), 0, CreateReplicas(wformatString("P/{0}", j).c_str()), 0));
        }

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNodeLoadsTrace2)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNodeLoadsTrace2");
        PLBConfigScopeChange(NodeLoadsTracingInterval, TimeSpan, TimeSpan::FromSeconds(0.1));
        PLBConfigScopeChange(PLBNodeLoadTraceEntryCount, uint, 10);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 30; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"m1/10,m2/10,m3/10"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"BalancingNodeLoadsTrace2_Service0", L"ServiceType0", true,
            CreateMetrics(L"m1/1.0/1/1,m2/1.0/2/2,m3/1.0/3/3")));

        for (int j = 1; j < 7; j++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(j), wstring(L"BalancingNodeLoadsTrace2_Service0"), 0, CreateReplicas(wformatString("P/{0}", j * 3).c_str()), 0));
        }

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithToBeDroppedReplicasTest)
    {
        wstring testName = L"BalancingWithToBeDroppedReplicasTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";

        for (int i = 0; i < 7; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", metric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = L"BalancingWithToBeDroppedReplicasTestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;

        // Service with standBy replica ToBeDroppedByFM
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,SB/2/D"), 0));

        // Service with secondary replica ToBeDroppedByPLB and InBuild and MoveInProgress replica
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3,S/4/R,I/5/B,S/6/V"), 0));

        for (int i = 0; i < 3; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", metric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2,S/4,S/6"), 0));
        }

        // PLB balancing phase should move out only 1 replica out of nodes 2 and 4
        // as load from ToBeDropped/MoveInProgress replicas should not be used for standard deviation calculations
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        // PLB should not move ToBeDropped or InBuild or MoveInProgress replicas
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2|3|4 move primary 2=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2|3|4 move secondary 4=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2|3|4 move secondary 6=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithNodeDeactivationTest)
    {
        wstring testName = L"BalancingWithNodeDeactivationTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeId = 0;
        for (; nodeId < 3; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId, L"", L"MyMetric/200"));
        }

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId++, L"", L"MyMetric/100",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // All replicas are on nodes 0-2 and other nodes are empty - cluster is not balanced.
        // Balancing should NOT move replicas to deactivated/deactivating nodes.
        // Nodes:
        // Nodes:
        // NodeId   Replicas#   Load    Capacity    TargetNode    DeactivationIntent  DeactivationStatus
        // 0        6           120     200                       None                None
        // 1        6           120     200                       None                None
        // 2        6           120     200                       None                None
        // 3                            200         Allow         None                DeactivationSafetyCheckInProgress
        // 4                            200         Don�t Allow   Pause               DeactivationSafetyCheckInProgress
        // 5                            200         Don�t Allow   Restart             DeactivationSafetyCheckInProgress
        // 6                            200         Don�t Allow   RemoveData          DeactivationSafetyCheckInProgress
        // 7                            200         Don�t Allow   RemoveNode          DeactivationSafetyCheckInProgress
        // 8                            200         Allow         None                DeactivationSafetyCheckComplete
        // 9                            200         Don�t Allow   Pause               DeactivationSafetyCheckComplete
        // 10                           200         Don�t Allow   Restart             DeactivationSafetyCheckComplete
        // 11                           200         Don�t Allow   RemoveData          DeactivationSafetyCheckComplete
        // 12                           200         Don�t Allow   RemoveNode          DeactivationSafetyCheckComplete
        // 13                           200         Allow         None                DeactivationComplete
        // 14                           200         Don't Allow   Pause               DeactivationComplete
        // 15                           200         Don't Allow   Restart             DeactivationComplete
        // 16                           200         Don't Allow   RemoveData          DeactivationComplete
        // 17                           200         Don't Allow   RemoveNode          DeactivationComplete
        // 18                           200         Allow         None                ActivationInProgress
        // 19                           200         Don�t Allow   Pause               ActivationInProgress
        // 20                           200         Don�t Allow   Restart             ActivationInProgress
        // 21                           200         Don�t Allow   RemoveData          ActivationInProgress
        // 22                           200         Don�t Allow   RemoveNode          ActivationInProgress
        // 23                           200         Allow         None                None
        // 24                           200         Allow         Pause               None
        // 25                           200         Allow         Restart             None
        // 26                           200         Allow         RemoveData          None
        // 27                           200         Allow         RemoveNode          None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"* move * *=>4|5|6|7|9|10|11|12|14|15|16|17|19|20|21|22", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithDeactivatedNodeBelowAvgTest)
    {
        wstring testName = L"BalancingNotNeededWithDeactivatedNodeBelowAvgTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = BalancingWithDeactivatedNodeHelper(testName, plb);

        wstring testType = wformatString("{0}{1}_Type", testName, index);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}{1}_Service", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        0           10      200         Pause               DeactivationSafetyCheckInProgress
        // 1        5           100     200         None                None
        // 2        5           100     200         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithDeactivatedNodeAboveAvgTest)
    {
        wstring testName = L"BalancingNotNeededWithDeactivatedNodeAboveAvgTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = BalancingWithDeactivatedNodeHelper(testName, plb);

        wstring testType = wformatString("{0}{1}_Type", testName, index);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}{1}_Service", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        10          200     200         Pause               DeactivationSafetyCheckInProgress
        // 1        5           100     200         None                None
        // 2        5           100     200         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(BalancingNeededWithDeactivatedNodeBelowAvgTest)
    {
        wstring testName = L"BalancingNeededWithDeactivatedNodeBelowAvgTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = BalancingWithDeactivatedNodeHelper(testName, plb);

        wstring testType = wformatString("{0}{1}_Type", testName, index);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}{1}_Service", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/10/10")));

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));
        }
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Expectation is that balancing phase will not clean up deactivated node
        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        1           10      200         Pause               DeactivationSafetyCheckInProgress
        // 1        5           100     200         None                None
        // 2        7           130     200         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * 2=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingNeededWithDeactivatedNodeAboveAvgTest)
    {
        wstring testName = L"BalancingNeededWithDeactivatedNodeAboveAvgTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = BalancingWithDeactivatedNodeHelper(testName, plb);

        wstring testType = wformatString("{0}{1}_Type", testName, index);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}{1}_Service", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // Expectation is that balancing phase will not clean up deactivated node
        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        10          200     200         Pause               DeactivationSafetyCheckInProgress
        // 1        5           100     200         None                None
        // 2        7           140     200         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * 2=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragNotNeededWithEmptyDeactivatedNodeTest)
    {
        wstring testName = L"DefragNotNeededWithEmptyDeactivatedNodeTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(L"MyMetric", true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"MyMetric", 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring serviceName;
        DefragWithDeactivatedNodeHelper(testName, plb, serviceName);

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        0           0       100         Pause               DeactivationSafetyCheckInProgress
        // 1        0           0       100         None                None
        // 2        4           80      100         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(DefragNotNeededWithNotEmptyDeactivatedNodeTest)
    {
        wstring testName = L"DefragNotNeededWithNotEmptyDeactivatedNodeTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(L"MyMetric", true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"MyMetric", 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring serviceName;
        int index = DefragWithDeactivatedNodeHelper(testName, plb, serviceName);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        1           10      100         Pause               DeactivationSafetyCheckInProgress
        // 1        0           0       100         None                None
        // 2        4           80      100         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(DefragNeededWithEmptyDeactivatedNodeTest)
    {
        wstring testName = L"DefragNeededWithEmptyDeactivatedNodeTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(L"MyMetric", true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"MyMetric", 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring serviceName;
        int index = DefragWithDeactivatedNodeHelper(testName, plb, serviceName);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        0           0       100         Pause               DeactivationSafetyCheckInProgress
        // 1        1           10      100         None                None
        // 2        4           80      100         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragNeededWithNotEmptyDeactivatedNodeTest)
    {
        wstring testName = L"DefragNeededWithNotEmptyDeactivatedNodeTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(L"MyMetric", true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"MyMetric", 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring serviceName;
        int index = DefragWithDeactivatedNodeHelper(testName, plb, serviceName);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Expectation is that balancing phase (doing defrag) will not clean up deactivated node
        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        1           10      100         Pause               DeactivationSafetyCheckInProgress
        // 1        1           10      100         None                None
        // 2        4           80      100         None                None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragmentationAssertsWhenNodeRestarts)
    {
        wstring testName = L"DefragmentationAssertsWhenNodeRestarts";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(L"MaxCpuUsage", true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        auto globalMetricWeights = PLBConfig::KeyDoubleValueMap();
        globalMetricWeights.insert(make_pair(L"GenericMetric1", 0.0));
        globalMetricWeights.insert(make_pair(L"GenericMetric2", 0.0));
        globalMetricWeights.insert(make_pair(L"CapacityUnit", 0.0));
        globalMetricWeights.insert(make_pair(L"MaxCpuUsage", 1.0));
        globalMetricWeights.insert(make_pair(L"VipMCU", 0.0));
        globalMetricWeights.insert(make_pair(L"ReservationUnit", 0.0));
        globalMetricWeights.insert(make_pair(L"InstanceDiskSpaceUsed", 0.3));
        globalMetricWeights.insert(make_pair(L"Count", 0.0));
        globalMetricWeights.insert(make_pair(L"PrimaryCount", 0.0));
        globalMetricWeights.insert(make_pair(L"ReplicaCount", 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalMetricWeights);

        PLBConfig::KeyIntegerValueMap activityThresholds;
        activityThresholds.insert(make_pair(L"GenericMetric1", 100));
        activityThresholds.insert(make_pair(L"GenericMetric2", 100));
        activityThresholds.insert(make_pair(L"CapacityUnit", 1199000));
        activityThresholds.insert(make_pair(L"MaxCpuUsage", 1199000));
        activityThresholds.insert(make_pair(L"VipMCU", 1199000));
        activityThresholds.insert(make_pair(L"ReservationUnit", 1199000));
        activityThresholds.insert(make_pair(L"InstanceDiskSpaceUsed", 1));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholds);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(L"GenericMetric1", 1));
        balancingThresholds.insert(make_pair(L"GenericMetric2", 1));
        balancingThresholds.insert(make_pair(L"CapacityUnit", 1));
        balancingThresholds.insert(make_pair(L"MaxCpuUsage", 50000));
        balancingThresholds.insert(make_pair(L"VipMCU", 50000));
        balancingThresholds.insert(make_pair(L"ReservationUnit", 50000));
        balancingThresholds.insert(make_pair(L"InstanceDiskSpaceUsed", 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        wstring capacityString = L"GenericMetric1/100,GenericMetric2/100,InstanceDiskSpaceUsed/1677312,MaxCpuUsage/800000,ReservationUnit/800000,VipMCU/800000";
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd:/0", L"0", capacityString));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd:/4", L"4", capacityString, Reliability::NodeDeactivationIntent::Enum::Restart));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd:/2", L"2", capacityString));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd:/1", L"2", capacityString));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd:/1", L"1", capacityString));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"fd:/0", L"1", capacityString));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(7, L"fd:/3", L"3", capacityString));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App10:SQL:SQL.LogicalServer.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App10:SQL:SQL.UserDb.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App11:SQL:SQL.LogicalServer.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App11:SQL:SQL.UserDb.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App12:SQL:SQL.LogicalServer.LS"), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App12:SQL:SQL.UserDb.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App13:SQL:SQL.LogicalServer.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO.Premium_App13:SQL:SQL.UserDb.LS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App14:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App14:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App15:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App15:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App16:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App16:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App17:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App17:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App18:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App18:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App19:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App19:SQL:SQL.UserDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App7:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App7:SQL:SQL.MasterDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App8:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App8:SQL:SQL.MasterDb.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App9:SQL:SQL.LogicalServer.RS"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Worker.ISO_App9:SQL:SQL.MasterDb.RS"), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Parent services
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/edeb225844e5/SQL.LogicalServer/edeb225844e5"), wstring(L"Worker.ISO_App15:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.LogicalServer/a190dc331d8c"), wstring(L"Worker.ISO.Premium_App13:SQL:SQL.LogicalServer.LS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/200000/200000,VipMCU/0.3/200000/200000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO.Premium/be238662c259/SQL.LogicalServer/be238662c259"), wstring(L"Worker.ISO.Premium_App10:SQL:SQL.LogicalServer.LS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/200000/200000,VipMCU/0.3/200000/200000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.LogicalServer/e37655c88904"), wstring(L"Worker.ISO.Premium_App11:SQL:SQL.LogicalServer.LS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/200000/200000,VipMCU/0.3/200000/200000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.LogicalServer/f2ef27efa24c"), wstring(L"Worker.ISO.Premium_App12:SQL:SQL.LogicalServer.LS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/200000/200000,VipMCU/0.3/200000/200000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/a1e074e25539/SQL.LogicalServer/a1e074e25539"), wstring(L"Worker.ISO_App9:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/100000/100000,VipMCU/0.3/100000/100000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.LogicalServer/a5f0d8dabf3e"), wstring(L"Worker.ISO_App16:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.LogicalServer/b1da6f9688eb"), wstring(L"Worker.ISO_App17:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/b5e31cbbb423/SQL.LogicalServer/b5e31cbbb423"), wstring(L"Worker.ISO_App7:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/100000/100000,VipMCU/0.3/100000/100000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.LogicalServer/d3cdd01dd7a1"), wstring(L"Worker.ISO_App14:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/dde0f29af569/SQL.LogicalServer/dde0f29af569"), wstring(L"Worker.ISO_App8:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/100000/100000,VipMCU/0.3/100000/100000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.LogicalServer/ea804c7e6b43"), wstring(L"Worker.ISO_App19:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));
        plb.UpdateService(CreateServiceDescription(wstring(L"fabric:/Worker.ISO/edd672058903/SQL.LogicalServer/edd672058903"), wstring(L"Worker.ISO_App18:SQL:SQL.LogicalServer.RS"), true, CreateMetrics(wstring(L"CapacityUnit/0.3/0/0,MaxCpuUsage/0.3/50000/50000,VipMCU/0.3/50000/50000,ReservationUnit/0.3/0/0,GenericMetric1/0.3/0/0,GenericMetric2/0.3/0/0,InstanceDiskSpaceUsed/0.3/0/0"))));

        plb.ProcessPendingUpdatesPeriodicTask();

        //Child services
        wstring childMetricsStr = L"PrimaryCount/1/0/0, ReplicaCount/0.3/0/0, Count/0.1/0/0";
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.UserDb/cd718467-e55e-4dc1-a271-0a3d1f0f82b1", L"Worker.ISO.Premium_App13:SQL:SQL.UserDb.LS", true, L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.LogicalServer/a190dc331d8c", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO.Premium/be238662c259/SQL.UserDb/fb17997c-b69b-416c-b7d6-64e21aab1a81", L"Worker.ISO.Premium_App10:SQL:SQL.UserDb.LS", true, L"fabric:/Worker.ISO.Premium/be238662c259/SQL.LogicalServer/be238662c259", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.UserDb/4041ae07-d691-482d-8111-256d66ee1710", L"Worker.ISO.Premium_App11:SQL:SQL.UserDb.LS", true, L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.LogicalServer/e37655c88904", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.UserDb/7ffc6bd5-ca41-44b5-8013-3284ac31f9c9", L"Worker.ISO.Premium_App12:SQL:SQL.UserDb.LS", true, L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.LogicalServer/f2ef27efa24c", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/a1e074e25539/SQL.MasterDb/2e5aef8b-75c4-4ffc-9f68-f8126c170aab", L"Worker.ISO_App9:SQL:SQL.MasterDb.RS", true, L"fabric:/Worker.ISO/a1e074e25539/SQL.LogicalServer/a1e074e25539", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.UserDb/84015360-1d79-447c-b48e-89014b51d3cd", L"Worker.ISO_App16:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.LogicalServer/a5f0d8dabf3e", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.UserDb/c488099f-d54a-432c-9a46-fa4444e3de4e", L"Worker.ISO_App16:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.LogicalServer/a5f0d8dabf3e", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.UserDb/bce3195f-4b84-486e-ab02-728d09e29c5d", L"Worker.ISO_App17:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/b1da6f9688eb/SQL.LogicalServer/b1da6f9688eb", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.UserDb/d203a874-af31-451a-8692-50cb4b5b2268", L"Worker.ISO_App17:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/b1da6f9688eb/SQL.LogicalServer/b1da6f9688eb", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/b5e31cbbb423/SQL.MasterDb/1c122247-b3c2-4327-a57c-422d91ad396f", L"Worker.ISO_App7:SQL:SQL.MasterDb.RS", true, L"fabric:/Worker.ISO/b5e31cbbb423/SQL.LogicalServer/b5e31cbbb423", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.UserDb/7030238e-cd6d-430e-a968-734deed305af", L"Worker.ISO_App14:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.LogicalServer/d3cdd01dd7a1", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.UserDb/866cca50-c97d-416c-9a84-3f4c8c13661f", L"Worker.ISO_App14:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.LogicalServer/d3cdd01dd7a1", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/dde0f29af569/SQL.MasterDb/97252ad7-296f-4930-8a1e-d1595818fceb", L"Worker.ISO_App8:SQL:SQL.MasterDb.RS", true, L"fabric:/Worker.ISO/dde0f29af569/SQL.LogicalServer/dde0f29af569", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.UserDb/21afed4e-78ef-45ab-869e-5b20d34c57cb", L"Worker.ISO_App19:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/ea804c7e6b43/SQL.LogicalServer/ea804c7e6b43", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.UserDb/7c99b5f2-ca22-4dc2-b5a6-28d5510f1e39", L"Worker.ISO_App19:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/ea804c7e6b43/SQL.LogicalServer/ea804c7e6b43", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/edd672058903/SQL.UserDb/0e0f2521-79ca-494f-8207-c2d979d5b0c9", L"Worker.ISO_App18:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/edd672058903/SQL.LogicalServer/edd672058903", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/edd672058903/SQL.UserDb/f36c48e0-22e3-44ef-8db0-12b6455818cf", L"Worker.ISO_App18:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/edd672058903/SQL.LogicalServer/edd672058903", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/edeb225844e5/SQL.UserDb/5c3f34fc-705f-4458-a8c0-6de04a5f8dd3", L"Worker.ISO_App15:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/edeb225844e5/SQL.LogicalServer/edeb225844e5", CreateMetrics(childMetricsStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"fabric:/Worker.ISO/edeb225844e5/SQL.UserDb/86f29f6d-1735-4477-8ad6-87a26e1c1064", L"Worker.ISO_App15:SQL:SQL.UserDb.RS", true, L"fabric:/Worker.ISO/edeb225844e5/SQL.LogicalServer/edeb225844e5", CreateMetrics(childMetricsStr)));

        plb.ProcessPendingUpdatesPeriodicTask();

        // FailoverUnits
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"fabric:/Worker.ISO/a1e074e25539/SQL.MasterDb/2e5aef8b-75c4-4ffc-9f68-f8126c170aab"), 45, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.LogicalServer/e37655c88904"), 110, CreateReplicas(L"P/6, S/2/B, S/7, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.UserDb/7ffc6bd5-ca41-44b5-8013-3284ac31f9c9"), 130, CreateReplicas(L"P/6, S/2/B, S/7, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.LogicalServer/d3cdd01dd7a1"), 130, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"fabric:/Worker.ISO/edeb225844e5/SQL.UserDb/5c3f34fc-705f-4458-a8c0-6de04a5f8dd3"), 116, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"fabric:/Worker.ISO/edd672058903/SQL.UserDb/f36c48e0-22e3-44ef-8db0-12b6455818cf"), 71, CreateReplicas(L"P/1"), 0));    // CHECK
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.UserDb/bce3195f-4b84-486e-ab02-728d09e29c5d"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.UserDb/7030238e-cd6d-430e-a968-734deed305af"), 63, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.LogicalServer/a5f0d8dabf3e"), 56, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"fabric:/Worker.ISO/b5e31cbbb423/SQL.LogicalServer/b5e31cbbb423"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"fabric:/Worker.ISO/b5e31cbbb423/SQL.MasterDb/1c122247-b3c2-4327-a57c-422d91ad396f"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.UserDb/cd718467-e55e-4dc1-a271-0a3d1f0f82b1"), 96, CreateReplicas(L"P/3, S/7, S/2/B, S/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(12), wstring(L"fabric:/Worker.ISO/edeb225844e5/SQL.UserDb/86f29f6d-1735-4477-8ad6-87a26e1c1064"), 116, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(13), wstring(L"fabric:/Worker.ISO/edeb225844e5/SQL.LogicalServer/edeb225844e5"), 111, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(14), wstring(L"fabric:/Worker.ISO/a1e074e25539/SQL.LogicalServer/a1e074e25539"), 46, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(15), wstring(L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.UserDb/866cca50-c97d-416c-9a84-3f4c8c13661f"), 62, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(16), wstring(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.UserDb/84015360-1d79-447c-b48e-89014b51d3cd"), 59, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(17), wstring(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.LogicalServer/b1da6f9688eb"), 25, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(18), wstring(L"fabric:/Worker.ISO/dde0f29af569/SQL.LogicalServer/dde0f29af569"), 62, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(19), wstring(L"fabric:/Worker.ISO.Premium/be238662c259/SQL.LogicalServer/be238662c259"), 128, CreateReplicas(L"P/5, S/7, S/2/B, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(20), wstring(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.UserDb/21afed4e-78ef-45ab-869e-5b20d34c57cb"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(21), wstring(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.LogicalServer/ea804c7e6b43"), 19, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(22), wstring(L"fabric:/Worker.ISO/edd672058903/SQL.UserDb/0e0f2521-79ca-494f-8207-c2d979d5b0c9"), 71, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(23), wstring(L"fabric:/Worker.ISO/ea804c7e6b43/SQL.UserDb/7c99b5f2-ca22-4dc2-b5a6-28d5510f1e39"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(24), wstring(L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.UserDb/4041ae07-d691-482d-8111-256d66ee1710"), 88, CreateReplicas(L"P/6, S/2/B, S/7,  S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(25), wstring(L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.LogicalServer/a190dc331d8c"), 144, CreateReplicas(L"P/7, S/2/B, S/6, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(26), wstring(L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.UserDb/c488099f-d54a-432c-9a46-fa4444e3de4e"), 60, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(27), wstring(L"fabric:/Worker.ISO/b1da6f9688eb/SQL.UserDb/d203a874-af31-451a-8692-50cb4b5b2268"), 24, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(29), wstring(L"fabric:/Worker.ISO.Premium/be238662c259/SQL.UserDb/fb17997c-b69b-416c-b7d6-64e21aab1a81"), 87, CreateReplicas(L"P/7, S/2/B, S/1, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(30), wstring(L"fabric:/Worker.ISO/edd672058903/SQL.LogicalServer/edd672058903"), 73, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(31), wstring(L"fabric:/Worker.ISO/dde0f29af569/SQL.MasterDb/97252ad7-296f-4930-8a1e-d1595818fceb"), 61, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(32), wstring(L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.LogicalServer/f2ef27efa24c"), 154, CreateReplicas(L"P/7, S/2/B, S/6, S/4"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"fabric:/Worker.ISO.Premium/e37655c88904/SQL.LogicalServer/e37655c88904", L"InstanceDiskSpaceUsed", 584, 583));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"fabric:/Worker.ISO/d3cdd01dd7a1/SQL.LogicalServer/d3cdd01dd7a1", L"InstanceDiskSpaceUsed", 302, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(8, L"fabric:/Worker.ISO/a5f0d8dabf3e/SQL.LogicalServer/a5f0d8dabf3e", L"InstanceDiskSpaceUsed", 307, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(9, L"fabric:/Worker.ISO/b5e31cbbb423/SQL.LogicalServer/b5e31cbbb423", L"InstanceDiskSpaceUsed", 309, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(13, L"fabric:/Worker.ISO/edeb225844e5/SQL.LogicalServer/edeb225844e5", L"InstanceDiskSpaceUsed", 302, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(14, L"fabric:/Worker.ISO/a1e074e25539/SQL.LogicalServer/a1e074e25539", L"InstanceDiskSpaceUsed", 311, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(32, L"fabric:/Worker.ISO.Premium/f2ef27efa24c/SQL.LogicalServer/f2ef27efa24c", L"InstanceDiskSpaceUsed", 582, 584));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(17, L"fabric:/Worker.ISO/b1da6f9688eb/SQL.LogicalServer/b1da6f9688eb", L"InstanceDiskSpaceUsed", 313, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(18, L"fabric:/Worker.ISO/dde0f29af569/SQL.LogicalServer/dde0f29af569", L"InstanceDiskSpaceUsed", 306, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(19, L"fabric:/Worker.ISO.Premium/be238662c259/SQL.LogicalServer/be238662c259", L"InstanceDiskSpaceUsed", 580, 581));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(21, L"fabric:/Worker.ISO/ea804c7e6b43/SQL.LogicalServer/ea804c7e6b43", L"InstanceDiskSpaceUsed", 302, 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(25, L"fabric:/Worker.ISO.Premium/a190dc331d8c/SQL.LogicalServer/a190dc331d8c", L"InstanceDiskSpaceUsed", 589, 582));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(30, L"fabric:/Worker.ISO/edd672058903/SQL.LogicalServer/edd672058903", L"InstanceDiskSpaceUsed", 302, 0));

        fm_->RefreshPLB(Stopwatch::Now());
        // We just want to make sure that PLB will not assert, so this test is not checking anything.
    }

    BOOST_AUTO_TEST_CASE(DefragPercentOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesLessThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 5 empty nodes N3, N7, N11, N15 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragPercentOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesLessThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create services and make only nodes N3, N7, N11, N15 and N19 to be empty
        for (int i = 0; i < 3; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}, S/{3}, S/{4}", i, i + 4, i + 2 * 4, i + 3 * 4, i + 4 * 4)), 0));
        }

        {
            // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 10% which is 2 nodes and this shoudn't trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.1));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now());
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // As we have 3 empty nodes in cluster and we defined that we want 2 empty nodes, defrag shouldn't be triggered
            VerifyPLBAction(plb, L"NoActionNeeded");
        }

        {
            // Add one more service and occupy 2 more nodes, so now 3 nodes are empty
            wstring serviceName = wformatString("TestService{0}", 3);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}", 3, 3 + 4)), 0));

            // Change percent of free nodes in order to trigger defrag, we define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 20% which is 4 nodes and this shoud trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.2));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragNumberOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesLessThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 5 empty nodes N3, N7, N11, N15 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNumberOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesLessThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create services and make only nodes N3, N7, N11, N15 and N19 to be empty
        for (int i = 0; i < 3; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}, S/{3}, S/{4}", i, i + 4, i + 2 * 4, i + 3 * 4, i + 4 * 4)), 0));
        }

        {
            // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 2.0 which is 2 empty nodes and this shoudn't trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 2.0));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now());
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // As we have 3 empty nodes in cluster and we defined that we want 2 empty nodes, defrag shouldn't be triggered
            VerifyPLBAction(plb, L"NoActionNeeded");
        }

        {
            // Add one more service and occupy 2 more nodes, so now 3 nodes are empty
            wstring serviceName = wformatString("TestService{0}", 3);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}", 3, 3 + 4)), 0));

            // Change number of free nodes in order to trigger defrag, we define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 4.0 which is 4 empty nodes and this shoud trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 4.0));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragPercentOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesMoreThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 10 empty nodes N2, N3, N6, N7, N10, N11, N14, N15, N18 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragPercentOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesMoreThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create services and make only nodes N2, N3, N6, N7, N10, N11, N14, N15, N18 and N19 to be empty
        for (int i = 0; i < 2; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}, S/{3}, S/{4}", i, i + 4, i + 2 * 4, i + 3 * 4, i + 4 * 4)), 0));
        }

        {
            // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 30% which is 6 nodes and this shoudn't trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.3));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now());
            // force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // As we have 10 empty nodes in cluster and we defined that we want 6 empty nodes, defrag shouldn't be triggered
            VerifyPLBAction(plb, L"NoActionNeeded");
        }

        {
            // Add one more service and occupy 3 more nodes, so now 8 nodes are empty
            wstring serviceName = wformatString("TestService{0}", 3);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}", 2, 2 + 4, 2 + 4 + 4)), 0));

            // Change percent of free nodes in order to trigger defrag, we define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 40% which is 8 empty nodes and this shoud trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.4));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragNumberOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesMoreThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 10 empty nodes N2, N3, N6, N7, N10, N11, N14, N15, N18 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNumberOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesMoreThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create 2 services and make only nodes N2, N3, N6, N7, N10, N11, N14, N15, N18 and N19 to be empty
        for (int i = 0; i < 2; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}, S/{3}, S/{4}", i, i + 4, i + 2 * 4, i + 3 * 4, i + 4 * 4)), 0));
        }

        {
            // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 6.0 which is 6 empty nodes and this shoudn't trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 6.0));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now());
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // As we have 10 empty nodes in cluster and we defined that we want 6 empty nodes, defrag shouldn't be triggered
            VerifyPLBAction(plb, L"NoActionNeeded");
        }

        {
            // Add one more service and occupy 3 more nodes, so now 7 nodes are empty
            wstring serviceName = wformatString("TestService{0}", 3);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}", 2, 2 + 4, 2 + 4 + 4)), 0));

            // Change number of free nodes in order to trigger defrag, we define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 8.0 which is 8 empty nodes and this shoud trigger defrag
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 8.0));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragNumberOfEmptyNodesWithoutDistributionNumberOfEmptyNodesLessThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 4 empty nodes N16, N17, N18 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNumberOfEmptyNodesWithoutDistributionNumberOfEmptyNodesLessThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(defragMetric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 4.0 which is 4 empty nodes and this shoudn't trigger defrag
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 4.0));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create 4 services and make only nodes N16, N17, N18 and N19 to be empty
        for (int i = 0; i < 4; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}, S/{3}", i, i + 4, i + 2 * 4, i + 3 * 4)), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        // Force round of slow balancing
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        // As we have 4 empty nodes in cluster and we defined that we want 4 empty nodes, defrag shouldn't be triggered
        VerifyPLBAction(plb, L"NoActionNeeded");

        {
            // Add one more service and occupy 1 more node, so now 3 nodes are empty, we should trigger defrag
            wstring serviceName = wformatString("TestService{0}", 4);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", 16)), 0));

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragNumberOfEmptyNodesWithoutDistributionNumberOfEmptyNodesMoreThenNumberOfDomainsTest)
    {
        // Cluster have 5 fds/uds and in every domain it has 4 nodes
        //  ---FD0-----FD1------FD2-------FD3--------FD4---
        //  | N0 N1 | N4 N5 | N8  N9  | N12 N13 | N16 N17 |
        //  | N2 N3 | N6 N7 | N10 N11 | N14 N15 | N18 N19 |
        //  -----------------------------------------------
        //  Initially it has 8 empty nodes N12, N13, N14, N15, N16, N17, N18 and N19
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNumberOfEmptyNodesAcrossFDsUDsNumberOfEmptyNodesMoreThenNumberOfDomainsTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(defragMetric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        // First define defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold to 30% which is 6 empty nodes and this shoudn't trigger defrag
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.3));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 4), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create 4 services and make only nodes N12, N13, N14, N15, N16, N17, N18 and N19 to be empty
        for (int i = 0; i < 4; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}", i, i + 4, i + 2 * 4)), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        // Force round of slow balancing
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        // As we have 8 empty nodes in cluster and we defined that we want 6 empty nodes, defrag shouldn't be triggered
        VerifyPLBAction(plb, L"NoActionNeeded");

        {
            // Add 2 more services and occupy 3 more node, so now 5 nodes are empty, we should trigger defrag
            wstring serviceName = wformatString("TestService{0}", 4);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}", 12, 16)), 0));

            serviceName = wformatString("TestService{0}", 5);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", 13)), 0));

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Verify that defrag occured
            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }

    BOOST_AUTO_TEST_CASE(DefragPercentOfEmptyNodesInClusterNotTriggeringDefragTest)
    {
        // Creates 10 node cluster and one replica on first 9 nodes. All nodes are in different FDs.
        // Last one is empty which should make defrag not issue any actions
        // due to defragmentationMetricsPercentOfFreeNodesThreshold which is 0.1 => 10% = 1 node.
        Trace.WriteInfo("PLBBalancingTestSource", "DefragPercentOfEmptyNodesInClusterNotTriggeringDefragTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i <= 9; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i), wformatString("{0}", i), L"DefragMetric/100"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Place the partitions on all the nodes. Keep the last one free.
        for (int i = 0; i <= 2; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}, S/{2}", 3 * i, 3 * i + 1, 3 * i + 2)), 0));
        }

        {
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 0.1));
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            fm_->RefreshPLB(Stopwatch::Now());
            VerifyPLBAction(plb, L"NoActionNeeded");
        }

        {
            // Remove defragmentationMetricsPercentOfFreeNodesThreshold and assert that cluster
            // gets defragmented.
            PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                PLBConfig::KeyDoubleValueMap,
                defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

            // Add one more service
            wstring serviceName = wformatString("TestService{0}", 3);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", 9)), 0));

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
            // Force round of slow balancing to find solution
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
            VerifyPLBAction(plb, L"LoadBalancing");
            //VerifyPLBStatusBefore(plb, defragMetric, false);
        }
    }

    BOOST_AUTO_TEST_CASE(DefragNumberOfEmptyNodesInClusterTriggeringDefragTest)
    {
        // Creates 10 node cluster and one replica on first 8 nodes. All nodes are in different FDs and UDs.
        // Last two nodes are empty.
        // defragmentationMetricsPercentOfFreeNodesThreshold is set to 30% so defrag on SHOULD be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNumberOfEmptyNodesInClusterTriggeringDefragTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, 3));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i <= 9; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i), to_wstring((long long)i), L"DefragMetric/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Place the replicas on first 8 nodes. Last two should be free.
        for (int i = 0; i <= 3; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}, S/{1}", 2 * i, 2 * i + 1)), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        // Do one more refresh to force LoadBalancing run since the fast one is not able to find any solution.
        // Opening bug 7217336 to track this.
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_FALSE(actionList.empty(), L"Defrag should have triggered actions to defragment the cluster");
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BalancingWithLocalDomainsOnly)
    {
        // Two services, each with two partitions, each with 3 relpicas.
        // All 12 replicas are evenly distributed across 6 nodes, and there are 2 empty nodes.
        // Local domains are balanced, and global domain is not.
        // We'll set the weight of local domains to 1, and expect PLB not to move anything.
        wstring testName = L"BalancingWithLocalDomainsOnly";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithLocalDomainsOnly");
        PLBConfigScopeChange(LocalDomainWeight, double, 1.0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 8; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_Type", testName);
        wstring serviceName1 = wformatString("{0}_Service1", testName);
        wstring serviceName2 = wformatString("{0}_Service2", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(serviceName1, serviceTypeName, true));
        plb.UpdateService(CreateServiceDescription(serviceName2, serviceTypeName, true));

        // These will go to local domain 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/3,S/4,S/5"), 0));
        // These will go to local domain 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/5,S/4,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithLocalDomainsAndGlobalDomain)
    {
        // Two services, each with two partitions, each with 3 relpicas.
        // All 12 replicas are evenly distributed across 6 nodes, and there are 2 empty nodes.
        // Local domains are balanced, and global domain is not.
        // We'll set the weight of local domains to 0.25, and expect PLB to move two replicas to 2 nodes.
        wstring testName = L"BalancingWithLocalDomainsAndGlobalDomain";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithLocalDomainsAndGlobalDomain");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 8; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_Type", testName);
        wstring serviceName1 = wformatString("{0}_Service1", testName);
        wstring serviceName2 = wformatString("{0}_Service2", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(serviceName1, serviceTypeName, true));
        plb.UpdateService(CreateServiceDescription(serviceName2, serviceTypeName, true));

        // These will go to local domain 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/3,S/4,S/5"), 0));
        // These will go to local domain 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/5,S/4,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithImbalancedLocalDomains)
    {
        // Two services, each with two partitions, each with 1 relpica.
        // Two primaries of the same service are on the same node.
        // There are two nodes - two primaries each - hence global balance is OK.
        // Local balance per service of bad (2 primaries on 1 node) - we need to move.
        wstring testName = L"BalancingWithImbalancedLocalDomains";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithImbalancedLocalDomains");
        PLBConfigScopeChange(LocalBalancingThreshold, double, 1.0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_Type", testName);
        wstring serviceName1 = wformatString("{0}_Service1", testName);
        wstring serviceName2 = wformatString("{0}_Service2", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(serviceName1, serviceTypeName, true));
        plb.UpdateService(CreateServiceDescription(serviceName2, serviceTypeName, true));

        // These will go to local domain 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        // These will go to local domain 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2|3 move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithApplicationBasicTest1)
    {
        // Multiple applications on the same set of node and some replicas need to be moved
        wstring testName = L"BalancingWithApplicationBasicTest1";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithApplicationBasicTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        for (int i = 0; i < 7; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 3;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/30/10/0")));

        wstring serviceWithScaleoutCount1 = wformatString("{0}_{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}_{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/3/3")));

        wstring appName2 = wformatString("{0}Application2", testName);
        appScaleoutCount = 2;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName2), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/20/10/0")));

        wstring serviceWithScaleoutCount3 = wformatString("{0}_{1}ScaleoutCount", testName, L"3");
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount3, testType, appName2, true, CreateMetrics(L"MyMetric/1.0/2/2")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWithScaleoutCount3), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        //VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move primary 0=>3|4", value)));
        //VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 1=>3|4", value)));

    }

    BOOST_AUTO_TEST_CASE(BalancingWithoutApplicationBasicTest1)
    {
        // Same set of services as above test but with no application
        wstring testName = L"BalancingWithoutApplicationBasicTest1";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithoutApplicationBasicTest1");

        PlacementAndLoadBalancing & plb = fm_->PLB;
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceWOScaleoutCount1 = wformatString("{0}_{1}", testName, L"1");
        wstring serviceWOScaleoutCount2 = wformatString("{0}_{1}", testName, L"2");

        plb.UpdateService(CreateServiceDescription(serviceWOScaleoutCount1, testType, true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescription(serviceWOScaleoutCount2, testType, true, CreateMetrics(L"MyMetric/1.0/3/3")));

        wstring serviceWOScaleoutCount3 = wformatString("{0}_{1}", testName, L"3");
        plb.UpdateService(CreateServiceDescription(serviceWOScaleoutCount3, testType, true, CreateMetrics(L"MyMetric/1.0/5/5")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWOScaleoutCount1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWOScaleoutCount2), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceWOScaleoutCount3), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move primary 0=>3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move secondary 1=>3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithApplicationCommonMetricsTest)
    {
        // Multiple applications on the same set of node and some replicas need to be moved
        wstring testName = L"BalancingWithApplicationCommonMetricsTest";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithApplicationCommonMetricsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 6;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/60/10/0")));

        wstring serviceWithScaleoutCount1 = wformatString("{0}_{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}_{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/3/3")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/3/3")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * *=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * *=>4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * *=>5", value)));

    }

    BOOST_AUTO_TEST_CASE(DefragWithAppGroupsCapacity)
    {
        wstring testName = L"DefragWithAppGroupsCapacity";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0/r0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0/r0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc0/r1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc0/r1", L""));

        wstring applicationName = wformatString("{0}_Application", testName);

        // Application: MaxNodes == 4, NodeCapacity == 3, ApplicationCapacity == 12
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            4,
            CreateApplicationCapacities(wformatString("{0}/12/3/0", defragMetric))));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            serviceName,
            testType,
            applicationName,
            true,
            CreateMetrics(wformatString("{0}/1/1/1", defragMetric))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/1,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/1,S/3"), 0));

        // Loads per node are: N0: 2, N1: 2, N2: 2, N3: 2
        // Defragmentation will aim to pack two nodes with loads of 4 each
        // AppGroups capacity should stop that and should make sure that loads are 3, 1, 3, 1

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNotInterruptedByCriticalInTransitionFuUpdatesTest)
    {
        // InTransition partitions should not be moved
        wstring testName = L"BalancingNotInterruptedByCriticalInTransitionFuUpdatesTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 500);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/10", metric))));
        int fus = 22;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // FT became InTransition - balancing operation should NOT be accepted
        //  - IsInBuild
        //  - IsToBePromoted
        //  - started upgrade
        //  - started configuration change (isToBeDeleted, isReconfigurationInProgress, isInQuorumLost)
        Common::TimerSPtr thread0 = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            int i = 0;
            // IsInBuild
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2/B"), 0));
            // IsToBePromoted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2/P"), 0));

            // started upgrade:
            Reliability::FailoverUnitFlags::Flags upgradingFlag;
            upgradingFlag.SetUpgrading(true);
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0, upgradingFlag, true));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0/I,S/1,S/2"), 0, upgradingFlag));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2/J"), 0, upgradingFlag));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,SB/2/K"), 0, upgradingFlag));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,D/2/K"), 0, upgradingFlag));

            // isToBeDeleted
            Reliability::FailoverUnitFlags::Flags toBeDeletedFlag;
            toBeDeletedFlag.SetToBeDeleted();
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0, toBeDeletedFlag));
            // isReconfigurationInProgress
            Reliability::FailoverUnitFlags::Flags emptyFlag;
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0, emptyFlag, true));
            // isInQuorumLost
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0, emptyFlag, false, true));

            plb.ProcessPendingUpdatesPeriodicTask();
        }, true);

        thread0->Change(TimeSpan::Zero, TimeSpan::MaxValue);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        thread0->Cancel();

        // only last FT is movable
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0u);
        // First 10 FTs will not be movable due to updates
        for (int i = 0; i < 10; ++i)
        {
            VERIFY_IS_TRUE(CountIf(actionList, ActionMatch(wformatString(L"{0} move * *=>*", i), value)) == 0u);
        }
    }

    BOOST_AUTO_TEST_CASE(BalancingNotInterruptedByNonCriticalFuUpdatesTest)
    {
        wstring testName = L"BalancingNotInterruptedByNonCriticalFuUpdatesTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 500);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/10", metric))));
        int fus = 7;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        // Temp service with one partition, just to make others go out of the nodes 0, 1 and 2
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(3));
        blockList.insert(CreateNodeId(4));
        blockList.insert(CreateNodeId(5));
        wstring tempTestType = wformatString("{0}_TempServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(tempTestType), move(blockList)));
        wstring tempServiceName = wformatString("{0}_TempService", testName);
        plb.UpdateService(CreateServiceDescription(tempServiceName, tempTestType, true, CreateMetrics(wformatString("{0}/1/50/50", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(fus), wstring(tempServiceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // FT operation should be accepted even if PLB received update with:
        //  - extra replicas (replicaDiff < 0)
        //  - StandBy replica
        //  - ToBeDropped replica
        //  - MoveInProgress replica
        //  - other replica flag changes

        // FT operation should not be accepted if:
        //  - version changes
        set<int> inTransitionIds;
        Common::TimerSPtr thread0 = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            int i = 0;
            // Extra replica
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), -1));
            // IsStandBy
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,SB/2"), 0));
            // IsToBeDroppedByFM
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/D"), 0));
            // IsToBeDroppedByPLB
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/R"), 0));
            // FailoverUnit changed MoveInProgress flag
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/V"), 0));

            // Other flag changes, e.g. /N, /Z, /L, /E, /M
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/N"), 0));

            // Version id change
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i++), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 0));

            plb.ProcessPendingUpdatesPeriodicTask();
        }, true);

        thread0->Change(TimeSpan::Zero, TimeSpan::MaxValue);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        thread0->Cancel();

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0u);
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(wformatString(L"{0} move * *=>*", fus), value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithLimitedMaxPercentageToMoveTest)
    {
        wstring testName = L"BalancingWithGlobalMovementThrottlingTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MaxPercentageToMove, double, 0.23);    // Most conservative
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 27);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.32);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.0);

        BalancingThrottlingHelper(23);

    }

    BOOST_AUTO_TEST_CASE(BalancingWithGlobalMovementThrottlingTest)
    {
        wstring testName = L"BalancingWithGlobalMovementThrottlingTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxPercentageToMove, double, 1.0);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 23);    // Most conservative
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.0);
        BalancingThrottlingHelper(23);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithGlobalPercentageThrottlingTest)
    {
        wstring testName = L"BalancingWithGlobalPercentageThrottlingTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxPercentageToMove, double, 1.0);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 13);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.02);  // Most conservative
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 57);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.9);

        BalancingThrottlingHelper(2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBalancingThresholdThrottlingTest)
    {
        wstring testName = L"BalancingWithBalancingThresholdThrottlingTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxPercentageToMove, double, 1.0);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 11);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.12);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 2); // Most conservative
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.9);

        BalancingThrottlingHelper(2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBalancingPercentageThrottlingTest)
    {
        wstring testName = L"BalancingWithBalancingPercentageThrottlingTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxPercentageToMove, double, 1.0);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 11);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.12);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 4);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.02);  // Most conservative

        BalancingThrottlingHelper(2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithGlobalMovementThrottlingAndMultipleServiceDomainsTest)
    {
        wstring testName = L"BalancingWithGlobalMovementThrottlingAndMultipleServiceDomainsTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric1 = L"myMetricA";
        wstring metric2 = L"myMetricB";
        wstring serviceName1 = L"ThrottlingTestServiceA";
        wstring serviceName2 = L"ThrottlingTestServiceB";
        wstring testType = L"TestType";

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 23);
        vector<wstring> actionList;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // force processing of pending updates so that service can be created
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(serviceName1, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric1))));
        plb.UpdateService(CreateServiceDescription(serviceName2, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric2))));

        int fuIndex = 0;

        for (int i = 0; i < 50; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        }
        for (int i = 0; i < 50; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(serviceName2), 0, CreateReplicas(L"P/0"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() <= 23u);
        VERIFY_IS_TRUE(actionList.size() > 0u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithStandByReplicaTest)
    {
        wstring testName = L"BalancingWithStandByReplicaTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        BalancingWithStandByReplicaHelper(testName, plb);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithStandByReplicaAndNoTransientOvercommitTest)
    {
        wstring testName = L"BalancingWithStandByReplicaAndNoTransientOvercommitTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        BalancingWithStandByReplicaHelper(testName, plb);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithThrottlingMultipleRefreshTest)
    {
        wstring testName = L"BalancingWithThrottlingMultipleRefreshTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 10);
        PLBConfigScopeChange(GlobalMovementThrottleCountingInterval, TimeSpan, TimeSpan::FromHours(24));

        fm_->Load();
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        for (int i = 0; i < 9; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), L"TestService", 0, CreateReplicas(L"I/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), L"TestService", 0, CreateReplicas(L"I/0"), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 6u);
        VerifyPLBAction(plb, L"LoadBalancing");

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->Clear();

        for (int i = 9; i < 100; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), L"TestService", 0, CreateReplicas(L"I/0"), 0));
        }
        
        plb.ProcessPendingUpdatesPeriodicTask();

        // Refresh again
        // We can only make 10 - 6 = 4 movements now
        now += TimeSpan::FromHours(12);
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 4u);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BalancingWithBigLoads)
    {
        wstring testName = L"BalancingWithBigLoads";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceSmall", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(
            L"TestServiceBig", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/2147483647/2147483647")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestServiceSmall"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestServiceSmall"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestServiceBig"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestServiceBig"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Big load for two services
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestServiceSmall", L"SurprizeMetric", INT_MAX, std::map<Federation::NodeId, uint>()));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestServiceSmall", L"SurprizeMetric", INT_MAX, std::map<Federation::NodeId, uint>()));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 2u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)), 2u);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithStaticConstraintsTest)
    {
        // Bug: 6748288
        // Setup of the test:
        //  Nodes:
        //      2  nodes with NodeType==Tenant
        //      98 nodes with NodeType==Other
        //  Services:
        //      1 stateful service with placement constraints "NoteType==Tenant"
        //          100 partitions of this service.
        //          1 replica per partition (Primary) placed on node 0
        //      1 stateless service without placement constraints
        //          1   partition of this service
        //          100 replicas, one on each node.
        //  Balancing is configured so that it can execute only 100 random moves, and temperature decay is high.
        // Expectation:
        //      Replicas of first service can be balanced between nodes 0 and 1. When static constraints are not used to eliminate nodes
        //      when making a random move then each node is a valid target and number of moves is small.
        //      When static constraints are used to eliminate nodes when making a random move, number of moves should be bigger.
        // Prior to the change:
        // 2016-07-01 12:44:56.952,Info    ,16364,PLB.PLBEnd,PLB generated 19 actions, 27 ms used (20 ms used for engine run.)
        // After the change
        //2016-07-01 12:30:37.476,Info    ,6940,PLB.PLBEnd,PLB generated 38 actions, 24 ms used (17 ms used for engine run.)

        wstring testName = L"BalancingWithStaticConstraintsTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 10);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 10);
        PLBConfigScopeChange(FastBalancingTemperatureDecayRate, double, 0.99);


        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties)));
        }
        for (; nodeIndex < 100; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int fuIndex = 0;

        wstring serviceName = wformatString("{0}_MultiPartitionedService", testName);

        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(L"TestType"),
            wstring(L""),
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            false,
            100,                                // partition count
            1,                                  // target replica set size
            false));                            // persisted

                                                // 1 service with 100 partitions, all with primary on node 0
        for (; fuIndex < 100; ++fuIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(fuIndex),
                wstring(serviceName),
                1,
                CreateReplicas(L"P/0"),
                0));
        }

        // One service that will go to all nodes (to put all nodes into domain).
        serviceName = wformatString("{0}_BigService", testName);
        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(L"TestType"),
            wstring(L""),
            false,
            wstring(L""),   // Placement constraints
            wstring(L""),
            true,
            vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            false,
            1,
            100,
            false));

        vector<ReplicaDescription> replicaVector;
        for (int replicaNo = 0; replicaNo < 100; ++replicaNo)
        {
            replicaVector.push_back(ReplicaDescription(
                CreateNodeInstance(replicaNo, 0),
                ReplicaRole::Secondary,
                Reliability::ReplicaStates::Ready,
                true, // isUp
                Reliability::ReplicaFlags::None));
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(fuIndex++),
            wstring(serviceName),
            0,
            move(replicaVector),
            0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 35u);
    }

    BOOST_AUTO_TEST_CASE(BalancingPartitionInQuorumLostTest)
    {
        wstring testName = L"BalancingPartitionInQuorumLostTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring metric = L"MyMetric";
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString(L"{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString(L"{0}/100", metric)));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Test service that can only be on node 0
        wstring testType = L"TestType";
        set<NodeId> blocklist;
        blocklist.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), move(blocklist)));

        wstring testService = L"TestService";
        plb.UpdateService(CreateServiceDescription(
            testService, testType, true, CreateMetrics(wformatString("{0}/1.0/50/0", metric))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testService), 0, CreateReplicas(L"P/0"), 0));

        // In quorum lost test service that should not be moved
        wstring inQuorumLostType = L"InQuorumLostTestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(inQuorumLostType), move(blocklist)));

        wstring inQuorumLostService = L"InQuorumLostService";
        plb.UpdateService(CreateServiceDescription(
            inQuorumLostService, inQuorumLostType, true, CreateMetrics(wformatString("{0}/1.0/50/0", metric))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(inQuorumLostService), 0,
            CreateReplicas(L"P/0"), 2, Reliability::FailoverUnitFlags::Flags::None, false, true));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityAndReportLoadTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithAffinityAndReportLoadTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wchar_t metricStr1[] = L"PrimaryCount/1.0/0/0";
        wchar_t metricStr2[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0";
        wchar_t metricStr3[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0";
        wchar_t metricStr4[] = L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0,Count/1.0/0/0,MickoMetric/1.0/1/1";

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr1)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr2)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(metricStr3)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService2", CreateMetrics(metricStr4)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 9));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService3", L"MickoMetric", 12, loads));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary *=>2", value)), CountIf(actionList, ActionMatch(L"1 move primary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move secondary *=>2", value)), CountIf(actionList, ActionMatch(L"1 move secondary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move primary *=>2", value)), CountIf(actionList, ActionMatch(L"3 move primary *=>2", value)));
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move secondary *=>2", value)), CountIf(actionList, ActionMatch(L"3 move secondary *=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDownNodes)
    {
        BalancingWithDownNodesHelper(L"BalancingWithDownNodes", false);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDownNodesAndReplicasOnThem)
    {
        BalancingWithDownNodesHelper(L"BalancingWithDownNodesAndReplicasOnThem", true);
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByFuUpdatesTest)
    {
        wstring testName = L"BalancingInterruptedByFuUpdatesTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(IsTestMode, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto VerifyNoBalancingMovements = [&](std::function<void()> addFailoverUnit) -> void
        {
            plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
            {
                addFailoverUnit();
                plb.ProcessPendingUpdatesPeriodicTask();
                plb.InterruptSearcherRunFinished = true;
            }, true);

            plb.InterruptSearcherRunFinished = false;
            StopwatchTime now = Stopwatch::Now();
            fm_->RefreshPLB(now);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);
            VerifyPLBAction(plb, L"LoadBalancing");
            VERIFY_ARE_EQUAL(0u, actionList.size());
        };

        VerifyNoBalancingMovements([&]() -> void
        {
            // FailoverUnit is created
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fus), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        });

        VerifyNoBalancingMovements([&]() -> void
        {
            // FailoverUnit is deleted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName)));
        });

        VerifyNoBalancingMovements([&]() -> void
        {
            // FailoverUnit has missing replicas (replicaDiff > 0)
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 1));
        });

        // remove missing replica in order not to have NewReplicaPlacement phase
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 2, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VerifyNoBalancingMovements([&]() -> void
        {
            // FailoverUnit changed position of primary replica
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 1, CreateReplicas(L"S/0,P/1,S/2"), 0));
        });

        VerifyNoBalancingMovements([&]() -> void
        {
            // FailoverUnit got changed number of active/standBy replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));
        });
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByServiceTypeUpdateExistingServiceTypeTest)
    {
        wstring testName = L"BalancingInterruptedByServiceTypeUpdateExistingServiceTypeTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(IsTestMode, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto VerifyBalancingMovements = [&](std::function<void()> action) -> void
        {
            plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
            {
                action();
                plb.ProcessPendingUpdatesPeriodicTask();
                plb.InterruptSearcherRunFinished = true;
            }, true);

            plb.InterruptSearcherRunFinished = false;

            StopwatchTime now = Stopwatch::Now();
            fm_->RefreshPLB(now);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VerifyPLBAction(plb, L"LoadBalancing");

            VERIFY_ARE_NOT_EQUAL(0u, actionList.size());
        };

        VerifyBalancingMovements([&]() -> void
        {
            // Service type is updated
            wstring newServiceType = testType;
            plb.UpdateServiceType(ServiceTypeDescription(wstring(newServiceType), set<NodeId>()));
        });
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByServiceTypeUpdateBlockListTest)
    {
        wstring testName = L"BalancingInterruptedByServiceTypeUpdateBlockListTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(IsTestMode, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto VerifyBalancingMovements = [&](std::function<void()> action) -> void
        {
            plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
            {
                action();
                plb.ProcessPendingUpdatesPeriodicTask();
                plb.InterruptSearcherRunFinished = true;
            }, true);

            plb.InterruptSearcherRunFinished = false;

            StopwatchTime now = Stopwatch::Now();
            fm_->RefreshPLB(now);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VerifyPLBAction(plb, L"LoadBalancing");

            VERIFY_ARE_EQUAL(0u, actionList.size());
        };

        VerifyBalancingMovements([&]() -> void
        {
            // Service type is updated
            set<NodeId> blockList;
            blockList.insert(CreateNodeId(3));
            blockList.insert(CreateNodeId(4));
            blockList.insert(CreateNodeId(5));
            wstring newServiceType = wformatString("{0}_AnotherServiceType", testName);
            plb.UpdateServiceType(ServiceTypeDescription(wstring(newServiceType), move(blockList)));
        });
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByServiceTypeUpdateBlockListEmptyTest)
    {
        wstring testName = L"BalancingInterruptedByServiceTypeUpdateBlockListEmptyTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(IsTestMode, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        auto VerifyBalancingMovements = [&](std::function<void()> action) -> void
        {
            plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
            {
                action();
                plb.ProcessPendingUpdatesPeriodicTask();
                plb.InterruptSearcherRunFinished = true;
            }, true);

            plb.InterruptSearcherRunFinished = false;

            StopwatchTime now = Stopwatch::Now();
            fm_->RefreshPLB(now);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VerifyPLBAction(plb, L"LoadBalancing");

            VERIFY_ARE_NOT_EQUAL(0u, actionList.size());
        };

        VerifyBalancingMovements([&]() -> void
        {
            // Service type is updated
            wstring newServiceType = wformatString("{0}_AnotherServiceType", testName);
            plb.UpdateServiceType(ServiceTypeDescription(wstring(newServiceType), set<NodeId>()));
        });
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByApplicationUpdateScaleoutTest)
    {
        wstring testName = L"BalancingInterruptedByApplicationUpdateScaleoutTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(IsTestMode, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            wstring appName1 = wformatString("{0}_Application", testName);
            int appScaleoutCount = 3;
            // App capacity: [total, perNode, reserved]
            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(wformatString("{0}/30/10/0", metric))));

            plb.ProcessPendingUpdatesPeriodicTask();
            plb.InterruptSearcherRunFinished = true;
        }, true);

        plb.InterruptSearcherRunFinished = false;

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingInterruptedByApplicationUpdateNoScaleoutTest)
    {
        wstring testName = L"BalancingInterruptedByApplicationUpdateNoScaleoutTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/200", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One service, each replica with default load of 1
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/10/5", metric))));

        int fus = 10;
        for (int i = 0; i < fus; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            wstring appName1 = wformatString("{0}_Application", testName);
            int appScaleoutCount = 0;
            // App capacity: [total, perNode, reserved]
            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(wformatString("{0}/30/10/0", metric))));

            plb.ProcessPendingUpdatesPeriodicTask();
            plb.InterruptSearcherRunFinished = true;
        }, true);

        plb.InterruptSearcherRunFinished = false;

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_NOT_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragNotNeededWithBlocklistedMetricTest)
    {
        // Replicas with the placement constraint are defragmented on the first node - defrag should NOT be triggered.
        // If we add stateless service with replicas on every node (without placement constraint) - defrag should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNotNeededWithBlocklistedMetricTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"0", move(nodeProperties), wformatString("{0}/100", metric)));
        }
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"1", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            L"StatefulService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateService(ServiceDescription(
            L"StatelessService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"StatelessService"), 0, CreateReplicas(L"S/0"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->Clear();

        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/0/0", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DefragNeededWithBlocklistedMetricTest)
    {
        // Replicas with the placement constraint are fragmented on first two nodes - defrag SHOULD be triggered.
        // If we add stateless service with replicas on every node (without placement constraint) - defrag SHOULD be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "DefragNeededWithBlocklistedMetricTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 10));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"0", move(nodeProperties), wformatString("{0}/100", metric)));
        }
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"1", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            L"StatefulService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateService(ServiceDescription(
            L"StatelessService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"StatelessService"), 0, CreateReplicas(L"S/1"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move * 0|1=>0|1", value)));

        fm_->Clear();

        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/0/0", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move * 0|1=>0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(PercentageOfEmptyNodesDefragNeededTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "PercentageOfEmptyNodesDefragNeededTest");

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        double numberOfNodesRequested = 0.5;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties), wformatString("{0}/100", metric)));
        }
        for (; nodeIndex < 6; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateServiceType(ServiceTypeDescription(wformatString(L"TestType{0}", i), set<NodeId>()));
            plb.UpdateService(ServiceDescription(
                wformatString(L"TestType{0}", i),
                wformatString(L"TestType{0}", i),
                wstring(L""),                       //app name
                true,                               //bool isStateful,
                wstring(L"NodeType==Tenant"),       //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/1/10/10", metric)),
                FABRIC_MOVE_COST_LOW,
                false,                              // on every node
                0,                                  // partition count
                0,                                  // target replica set size
                false));                            // persisted
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestType0"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestType1"), 0, CreateReplicas(L"P/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");

        // Initial placement:
        //       |    Other    |    Other    |   Tenant    |   Tenant    |   Tenant    |   Tenant    |
        // Nodes | N0(CPU:100) | N1(CPU:100) | N2(CPU:100) | N3(CPU:100) | N4(CPU:100) | N5(CPU:100) |
        // Test0 |             |             |      P      |             |             |             |
        // Test1 |             |             |             |      P      |             |             |
    }

    BOOST_AUTO_TEST_CASE(BalancingNeededWithBlocklistedMetricTest)
    {
        // Replicas with the placement constraint are defragmented on the first node - balancing SHOULD be triggered.
        // If we add stateless service with replicas on every node (without placement constraint) - balancing SHOULD be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNeededWithBlocklistedMetricTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";

        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"0", move(nodeProperties), wformatString("{0}/100", metric)));
        }
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"1", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            L"StatefulService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateService(ServiceDescription(
            L"StatelessService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"StatelessService"), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"StatelessService"), 0, CreateReplicas(L"S/0"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|1|2|3 move * 0=>1", value)));

        fm_->Clear();

        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|1|2|3 move * 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeTest)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service with replicas on every node (without placement constraint) - balancing should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, true);

        fm_->Clear();

        // stateless on every node without placement constraint
        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            1,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeTest1)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service with replicas on every node with the same placement constraint - balancing should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeTest1");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        // stateless on every node with placement constraint
        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNodeWithPlacementConstraint",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            1,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"StatelessOnEveryNodeWithPlacementConstraint"), 0, CreateReplicas(L"I/0, I/1"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeComplexTest)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add couple of stateless services
        // with replicas on every node with some placement constraints - balancing should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricStatelessOnEveryNodeTest1");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        for (int nodeIndex = 4; nodeIndex < 6; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Test"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"2", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // stateless on every node with placement constraint Tenant || Other
        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNodeWithPlacementConstraint1",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant || NodeType==Other"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            1,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

                                                // stateless on every node with placement constraint Tenant || Test
        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNodeWithPlacementConstraint2",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant || NodeType==Test"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            1,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"StatelessOnEveryNodeWithPlacementConstraint1"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5), wstring(L"StatelessOnEveryNodeWithPlacementConstraint2"), 0, CreateReplicas(L"I/0, I/1, I/4, I/5"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNeededWithBlocklistedMetricStatelessTest)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service
        // with target greather than total node count (without placement constraint) - balancing should be triggered with no action.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricStatelessTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        // stateless without placement constraint with target > number of nodes
        plb.UpdateService(ServiceDescription(
            L"StatelessServiceWithHugeTarget",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            1,                                  // partition count
            5,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"StatelessServiceWithHugeTarget"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithBlocklistedMetricStatelessTest1)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service
        // with target greather than total node count with the same placement constraint - balancing should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricStatelessTest1");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        // stateless with placement constraint with target > number of nodes
        plb.UpdateService(ServiceDescription(
            L"StatelessWithPlacementConstraintService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            1,                                  // partition count
            5,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"StatelessWithPlacementConstraintService"), 0, CreateReplicas(L"S/0, S/1"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNotNeededWithBlocklistedMetricAndAppOnEveryNodeTest)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service
        // with replicas on every node (without the placement constraint)
        // with scaleOut equal to the total node count - balancing should NOT be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNotNeededWithBlocklistedMetricAndAppOnEveryNodeTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        wstring applicationName = L"Application";

        // Application: MaxNodes == 4, NodeCapacity == 100, ApplicationCapacity == 400
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            4,
            CreateApplicationCapacities(wformatString("{0}/400/100/0", metric))));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(applicationName),           //app name
            false,                              //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingNeededWithBlocklistedMetricAndAppOnEveryNodeTest)
    {
        // Replicas with the placement constraint are perfectly balanced on first two nodes - balancing should NOT be triggered.
        // If we add stateless service
        // with replicas on every node (without the placement constraint)
        // with scaleOut smaller than the total node count - balancing SHOULD be triggered.
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingNeededWithBlocklistedMetricAndAppOnEveryNodeTest");
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        StopwatchTime now = BalancingNotNeededWithBlocklistedMetricHelper(plb, metric, false);

        fm_->Clear();

        wstring applicationName = L"Application";

        // Application: MaxNodes == 3, NodeCapacity == 100, ApplicationCapacity == 400
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            3,
            CreateApplicationCapacities(wformatString("{0}/400/100/0", metric))));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(ServiceDescription(
            L"StatelessOnEveryNode",
            wstring(L"TestType"),
            wstring(applicationName),           //application name
            false,                              //boolean isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            true,                               // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(
            FailoverUnitDescription(CreateGuid(4), wstring(L"StatelessOnEveryNode"), 0, CreateReplicas(L"I/0, I/1, I/2"), 0));

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        // Filed bug: 8692697
        //VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 move * 0|1=>3", value)));

        // updating the application not to use scale-out and application capacity so we can delete it
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            0,
            CreateApplicationCapacities(L"")));

        // we should be able to delete both application and the service
        plb.DeleteApplication(applicationName);
        plb.DeleteService(L"StatelessOnEveryNode");
    }

    BOOST_AUTO_TEST_CASE(DefragBalancedStartVerifyEmptyNodes)
    {
        wstring testName = L"DefragBalancedStartVerifyEmptyNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 50, 25,
            9, 50, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&]() -> int
        {
            int count = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad == 0) ++count;
                }
            }

            return count;
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = countEmptyNodes();

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(DefragBalancedStartVerifyEmptyNodesWithEmptyNodeThreshold)
    {
        wstring testName = L"DefragBalancedStartVerifyEmptyNodesWithEmptyNodeThreshold";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance
        int emptyNodeThreshold = 25; // Consider load of 25 as "empty"

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyIntegerValueMap defragmentationMetricsEmptyNodeThresholds;
        defragmentationMetricsEmptyNodeThresholds.insert(make_pair(metric, emptyNodeThreshold));
        PLBConfigScopeChange(MetricEmptyNodeThresholds, PLBConfig::KeyIntegerValueMap, defragmentationMetricsEmptyNodeThresholds);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 50, 25,
            9, 50, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&]() -> int
        {
            int count = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad <= emptyNodeThreshold) ++count;
                }
            }

            return count;
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = countEmptyNodes();

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(DefragOneMoreEmptyNodeNeeded)
    {
        wstring testName = L"DefragOneMoreEmptyNodeNeeded";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&]() -> int
        {
            int count = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad == 0) ++count;
                }
            }

            return count;
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = countEmptyNodes();

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(DefragBalancedStartVerifyEmptyNodesInDifferentDomains)
    {
        wstring testName = L"DefragBalancedStartVerifyEmptyNodesInDifferentDomains";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 50, 25,
            9, 50, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&](int& numOfEmptyNodes, int& numOfFDs, int& numOfUDs) -> void
        {
            bool usedFD[8];
            bool usedUD[8];

            for (int i = 0; i < 8; ++i)
            {
                usedFD[i] = usedUD[i] = false;
            }

            numOfEmptyNodes = numOfFDs = numOfUDs = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad == 0)
                    {
                        ++numOfEmptyNodes;

                        if (!usedFD[i % 4])
                        {
                            ++numOfFDs;
                            usedFD[i % 4] = true;
                        }

                        if (!usedUD[i % 3])
                        {
                            ++numOfUDs;
                            usedUD[i % 3] = true;
                        }
                    }
                }
            }
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = 0, FDCount = 0, UDCount = 0;
        countEmptyNodes(emptyNodeCount, FDCount, UDCount);

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
        VERIFY_IS_TRUE(FDCount == freeNodesTarget);
        VERIFY_IS_TRUE(UDCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(DefragNodesWithReservationOverlapping)
    {
        wstring testName = L"DefragNodesWithReservationOverlapping";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";
        wstring metric2 = L"DefragMetric2";

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(NodesWithReservedLoadOverlap, bool, true);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        defragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        scopedDefragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyIntegerValueMap defragmentationMetricsEmptyNodeThresholds;
        defragmentationMetricsEmptyNodeThresholds.insert(make_pair(metric2, 10));
        PLBConfigScopeChange(MetricEmptyNodeThresholds, PLBConfig::KeyIntegerValueMap, defragmentationMetricsEmptyNodeThresholds);


        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric2, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric2, freeNodesTarget + 1));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        scopedDefragEmptyWeight.insert(make_pair(metric2, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        wstring capacities = wformatString("{0}/100,{1}/100", metric, metric2);
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), capacities));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize, wstring metricx) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                if (replicaSize == 20)
                {
                    plb.UpdateService(
                        CreateServiceDescription(
                            serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2},{3}/0.3/{1}/{2}", metric, replicaSize, 0, metric2))));

                    plb.UpdateFailoverUnit(
                        FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                    fm_->FuMap.insert(
                        make_pair(
                            CreateGuid(cnt),
                            FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
                    ++cnt;

                    plb.UpdateFailoverUnit(
                        FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex + 1)), 0));
                    fm_->FuMap.insert(
                        make_pair(
                            CreateGuid(cnt), 
                            FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex + 1)), 0)));
                }
                else
                {
                    plb.UpdateService(
                        CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metricx, replicaSize, 0))));

                    plb.UpdateFailoverUnit(
                        FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                    fm_->FuMap.insert(make_pair(
                        CreateGuid(cnt), 
                        FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
                }
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 0, 25,
            9, 0, 25
        };
        int config2[nodeCount][3] =
        {
            0, 40, 20,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 75, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
        {
            if (config[i][1] > 0)
            {
                fillNode(config[i][0], config[i][1], config[i][2], metric);
            }
        }
        for (int i = 0; i < nodeCount; ++i)
        {
            if (config2[i][1] > 0)
            {
                fillNode(config2[i][0], config2[i][1], config2[i][2], metric2);
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&]() -> int
        {
            int count = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad == 0) ++count;
                }
            }

            return count;
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = countEmptyNodes();

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(DefragTelemetryTest)
    {
        wstring testName = L"DefragMultiplePartitionsServicesOverlappingWithBlocklistedNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric0 = L"BalancingMetric1"; //set as Defrag metric, but overrided with PlacementStrategy,
                                               //also no service reports this metric but it has weight, so it should be counted
        wstring metric1 = L"BalancingMetric2"; //if nothing set, metric will be balancing
        wstring metric2 = L"DefragMetric"; //set with DefragmentationMetrics
        wstring metric3 = L"BalancingWithReservationMetric1"; //set with PlacementStrategy
        wstring metric4 = L"BalancingWithReservationMetric2"; //set with DefragMetric and EnableScopedDefrag
        wstring metric5 = L"ReservationMetric"; //set with PlacementStrategy
        wstring metric6 = L"DefragMetricWeight0"; //if weight is 0, it should be counted as otherMetric
        // MetricCount per strategies 2/2/1/0/1, +1 other metric (weight 0)

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric0, true));
        defragMetricMap.insert(make_pair(metric2, true));
        defragMetricMap.insert(make_pair(metric4, true));
        defragMetricMap.insert(make_pair(metric6, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric4, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric0, 0));
        placementStrategy.insert(make_pair(metric3, 1));
        placementStrategy.insert(make_pair(metric5, 2));
        PLBConfigScopeChange(PlacementStrategy, PLBConfig::KeyIntegerValueMap, placementStrategy);

        //Not important for test
        PLBConfig::KeyIntegerValueMap reservationLoads;
        reservationLoads.insert(make_pair(metric3, 100));
        reservationLoads.insert(make_pair(metric4, 100));
        reservationLoads.insert(make_pair(metric5, 100));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservationLoads);

        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric3, Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric4, Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric5, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric2, 1));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric3, 1));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric4, 1));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric5, 1));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric6, 1));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric3, 0.5));
        scopedDefragEmptyWeight.insert(make_pair(metric4, 0.5));
        scopedDefragEmptyWeight.insert(make_pair(metric5, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 10);

        PLBConfig::KeyDoubleValueMap globalMetricWeights;
        globalMetricWeights.insert(make_pair(metric0, 1.0));
        globalMetricWeights.insert(make_pair(metric1, 1.0));
        globalMetricWeights.insert(make_pair(metric6, 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalMetricWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring capacities = wformatString("{0}/100,{1}/100,{2}/100,{3}/100,{4}/100,{5}/100,{6}/100", metric0, metric1, metric2, metric3, metric4, metric5, metric6);
        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), capacities));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring metrics = wformatString("{0}/0.3/10/10,{1}/0.3/10/10,{2}/0.3/10/10,{3}/0.3/10/10,{4}/0.3/10/10,{5}/0.3/10/10", metric1, metric2, metric3, metric4, metric5, metric6);
        wstring serviceName = wformatString("ServiceOnANodeType_{0}", 1);
        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(L"TestType"),
            wstring(L""),
            true,                               //bool isStateful,
            wstring(),                          //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(metrics),
            FABRIC_MOVE_COST_LOW,
            false,
            1,                                  // partition count
            2,                                  // target replica set size
            false));                            // persisted
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;

        VERIFY_IS_TRUE(plbTestHelper.CheckDefragStatistics(2, 2, 1, 0, 1));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(plbTestHelper.CheckDefragStatistics(2, 2, 1, 0, 1));

        PLBConfig::KeyIntegerValueMap placementStrategyUpgrade;
        placementStrategyUpgrade.insert(make_pair(metric0, 0));
        placementStrategyUpgrade.insert(make_pair(metric3, 1));
        placementStrategyUpgrade.insert(make_pair(metric5, 3)); // Strategy for this metric is changed from Reservation to ReservationWithPacking
        PLBConfigScopeModify(PlacementStrategy, placementStrategyUpgrade);

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(plbTestHelper.CheckDefragStatistics(2, 2, 0, 1, 1));

    }
    BOOST_AUTO_TEST_CASE(DefragMultiplePartitionsServicesOverlappingWithBlocklistedNodes)
    {
        // Test scenario, 10 nodes with 100 capacity for both metrics, and 2 node types with loads as below:
        // Node/type   0/A  1/A  2/A  3/A  4/A  5/B  6/B  7/B  8/B  9/B  
        // Metric1      80   80	  60   60             40   40   40   20
        // Metric2      60   60   60   60                            20
        //
        // There are 4 types of services, 2 of them multipartition, some reports only 1, some reports 2 metrics 
        // With and without placement constraint to one of node types with replicas of 10 load for each reporting metric
        // Goal is to have 3 fully empty nodes (100 reservation load) for both metrics (since they overlap) and 1 additional empty node for metric2
        // Low temperature should increase heuristic moves, so 9th node should be emptied without any additional moves, moving only 2 replicas

        wstring testName = L"DefragMultiplePartitionsServicesOverlappingWithBlocklistedNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric1";
        wstring metric2 = L"DefragMetric2";

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(NodesWithReservedLoadOverlap, bool, true);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        defragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        scopedDefragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyIntegerValueMap reservationLoads;
        reservationLoads.insert(make_pair(metric, 100));
        reservationLoads.insert(make_pair(metric2, 100));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservationLoads);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric2, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric2, freeNodesTarget + 1));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        scopedDefragEmptyWeight.insert(make_pair(metric2, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0.001);

        wstring capacities = wformatString("{0}/100,{1}/100", metric, metric2);
        int nodeIndex = 0;
        for (; nodeIndex < 5; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacities));
        }
        for (; nodeIndex < 10; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacities));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeOnBNodes"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MultipartitionServiceType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MultipartitionServiceTypeOnANodes"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 10;
        int serviceCount = 0;
        int failoverUnitId = 0;
        for (; serviceCount < 2; ++serviceCount)
        {
            serviceName = wformatString("ServiceOnANodeType_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==A"),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                2,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
            failoverUnitId++;
        }

        for (; serviceCount < 4; ++serviceCount)
        {
            serviceName = wformatString("Service_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestTypeOnBNodes"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==B"),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2},{3}/0.3/{1}/{2}", metric, replicaSize, 0, metric2)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                1,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/9"), 0));
            failoverUnitId++;
        }

        for (; serviceCount < 6; ++serviceCount)
        {
            serviceName = wformatString("ServiceOnANodeType_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"MultipartitionServiceTypeOnANodes"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==A"),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2},{3}/0.3/{1}/{2}", metric, replicaSize, replicaSize, metric2)),
                FABRIC_MOVE_COST_LOW,
                false,
                3,                                  // partition count
                4,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
            failoverUnitId++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
            failoverUnitId++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
            failoverUnitId++;
        }

        for (; serviceCount < 8; ++serviceCount)
        {
            serviceName = wformatString("Service_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"MultipartitionServiceType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L""),                       //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize)),
                FABRIC_MOVE_COST_LOW,
                false,
                2,                                  // partition count
                3,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/6, S/7, S/8"), 0));
            failoverUnitId++;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/6, S/7, S/8"), 0));
            failoverUnitId++;
        }

        fm_->RefreshPLB(Stopwatch::Now());

        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;

        VERIFY_IS_TRUE(plbTestHelper.CheckDefragStatistics(0, 2, 0, 0, 0));

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() == 2);
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move * 9=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragReservationWithNodesWithLowerCapacityThanRequired)
    {
        // Test scenario, 9 nodes with capacity for both metrics and 3 node types with loads as below:
        // Node/type         0/A  1/A  2/A  3/B  4/B  5/B  6/C  7/C  8/C    
        // Load/Metric1       50   50    0   50   50    0   50   50    0
        // Capacity/Metric1  120  120  120  100  100  100   50   50   50 
        // Load/Metric2       50   50    0   50   50    0   50   50    0                        
        // Capacity/Metric2   80   80   80  100  100  100  200  200  200
        //
        // Reservation goal is to reserve 100 load on 3 nodes for each metric
        // For Metric 1, only A and B type have enough load if emptied; in start there are 2 enough empty nodes 2 and 5
        // Beside node 8 is empty, it doesn't have enough load so it shouldn't be considered as reserved so one of nodes of A or B type
        // Should be emptied a bit
        // For Metric 2, only B and C type have enough load if emptied; in start there are 4 enough empty nodes 5, 6, 7 and 8
        // Beside node 3 is empty, it doesn't have enough load so it shouldn't be considered as reserved while 6 and 7 should since there
        // is already 150 empty amount. There are enough empty nodes for this metric
        // Empty loads doesn't need to overlap so some replicas from nodes A or B should be moved to have enough reserved load for Metric1

        wstring testName = L"DefragReservationWithNodesWithLowerCapacityThanRequired";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric1";
        wstring metric2 = L"DefragMetric2";

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        defragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        scopedDefragMetricMap.insert(make_pair(metric2, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyIntegerValueMap reservationLoads;
        reservationLoads.insert(make_pair(metric, 100));
        reservationLoads.insert(make_pair(metric2, 100));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservationLoads);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric2, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric2, freeNodesTarget));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        scopedDefragEmptyWeight.insert(make_pair(metric2, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 10);

        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0.001);

        wstring capacities1 = wformatString("{0}/120,{1}/80", metric, metric2);
        wstring capacities2 = wformatString("{0}/100,{1}/100", metric, metric2);
        wstring capacities3 = wformatString("{0}/50,{1}/200", metric, metric2);
        
        int nodeIndex = 0;
        for (; nodeIndex < 3; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacities1));
        }
        for (; nodeIndex < 6; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacities2));
        }
        for (; nodeIndex < 9; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacities3));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 10;
        int serviceCount = 0;
        int failoverUnitId = 0;

        for (; serviceCount < 5; ++serviceCount)
        {
            serviceName = wformatString("ServiceOnANodeType_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(),                          //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2},{3}/0.3/{1}/{2}", metric, replicaSize, replicaSize, metric2)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                6,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/3, S/4, S/6, S/7"), 0));
            failoverUnitId++;
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Only 3 replicas should be moved from node 0 or 1, no additional moves should be made
        VERIFY_IS_TRUE(actionList.size() == 3);
    }

    BOOST_AUTO_TEST_CASE(DefragReservationWithBlocklistedNodes)
    {
        // Test scenario, 8 nodes of 2 types, with capacity and with loads as below:
        // Node/type         0/A  1/A  2/A  3/A  4/B  5/B  6/B  7/B    
        // Load/Metric1      150  100   50    0    0    0    0    0
        // Capacity/Metric1  200  200  200  200  200  200  200  200 
        
        // Reservation goal is to reserve 200 load on 2 nodes
        // All services are constrained on A nodeType, so even B type is empty, it is not good to reserve capacity on B type
        // Due to that, 2 nodes of A type should have 200 reserved load
        // Lower temperature should made only heuristic moves, least possible, so that would be emptying node 2 with removing
        // all load from it - 5 replicas (each service has 1 replica of 10 load). Due to reservation with balancing, additional moves can be made

        wstring testName = L"DefragReservationWithBlocklistedNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric1";

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyIntegerValueMap reservationLoads;
        reservationLoads.insert(make_pair(metric, 200));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservationLoads);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int freeNodesTarget = 2;
        float defragWeight = 0.5f; //Balance also

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0.001);

        wstring capacity = wformatString("{0}/200", metric);

        int nodeIndex = 0;
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacity));
        }
        for (; nodeIndex < 8; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), capacity));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 10;
        int serviceCount = 0;
        int failoverUnitId = 0;

        for (; serviceCount < 30; ++serviceCount)
        {
            serviceName = wformatString("ServiceOnANodeType_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==A"),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                1,                                  // target replica set size
                false));                            // persisted
            
            // 15 services on node 0, 10 on node 1, and 5 on node 2
            int nodeId = 0;
            if (failoverUnitId < 15)
            {
                nodeId = 0;
            }
            else if (failoverUnitId < 25)
            {
                nodeId = 1;
            }
            else
            {
                nodeId = 2;
            }
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeId)), 0));
            failoverUnitId++;
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(5, CountIf(actionList, ActionMatch(L"* move * 2=>0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragMultipleMetricsPickMaxCapacityForReservation)
    {
        // Test scenario, 8 nodes of 2 types, with capacity and with loads as below:
        // Node/type         0/A  1/A  2/A  3/A  4/A  5/A  6/B  7/B    
        // Load/Metric1      100  100   50   50   10    0   10    0
        // Capacity/Metric1  200  200  200  200  200  200  150  150 

        // Amount of reservation load per node is not set only emptyNodeThreshold on 0, so we pick nodes with max capacity
        // Number of nodes with reservation is 2, and A type have higher capacity, so one of A type should be emptied
        // despite node 7 is empty(it is B type - doesn't have 200 capacity)
        // Beside that reservation, all replicas have also 10 load for metric2 which is balancing metric and shouldn't destroy defrag goal

        wstring testName = L"DefragMultipleMetricsPickMaxCapacityForReservation";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring defragMetric = L"DefragMetric";
        wstring balancingMetric = L"BalancingMetric";

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(defragMetric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int freeNodesTarget = 2;
        float defragWeight = 0.5f; //Balance also

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(defragMetric, freeNodesTarget));
        PLBConfigScopeChange(
            DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(defragMetric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 10);

        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0.001);

        wstring capacities1 = wformatString("{0}/200,{1}/300", defragMetric, balancingMetric);
        wstring capacities2 = wformatString("{0}/150,{1}/200", defragMetric, balancingMetric);

        int nodeIndex = 0;
        for (; nodeIndex < 6; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex % 3), wformatString(L"ud{0}", nodeIndex % 3), move(nodeProperties), capacities1));
        }
        for (; nodeIndex < 8; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex % 3), wformatString(L"ud{0}", nodeIndex % 3), move(nodeProperties), capacities2));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 10;
        int serviceCount = 0;
        int failoverUnitId = 0;

        for (; serviceCount < 32; ++serviceCount)
        {
            serviceName = wformatString("Service_{0}", serviceCount);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2},{3}/0.3/{1}/{2}", defragMetric, replicaSize, 0, balancingMetric)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                1,                                  // target replica set size
                false));                            // persisted

            // 10 services on node 0, 10 on node 1, 5 on node 2, 5 on node 3, 1 on node 4 and 1 on node 6
            int nodeId = 0;
            if (failoverUnitId < 10)
            {
                nodeId = 0;
            }
            else if (failoverUnitId < 20)
            {
                nodeId = 1;
            }
            else if (failoverUnitId < 25)
            {
                nodeId = 2;
            }
            else if (failoverUnitId < 30)
            {
                nodeId = 3;
            }
            else if (failoverUnitId < 31)
            {
                nodeId = 4;
            }
            else
            {
                nodeId = 6;
            }

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(failoverUnitId), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeId)), 0));
            failoverUnitId++;
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Low temperature should pick next move for defrag.
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"30 move primary 4=>*", value)));
        // There will be more moves because of balancing also
        VERIFY_IS_TRUE(actionList.size() > 0);
    }

    BOOST_AUTO_TEST_CASE(DefragBalancedStartVerifyEmptyNodesInDifferentDomainsMultiplePartitionServices)
    {
        wstring testName = L"DefragBalancedStartVerifyEmptyNodesInDifferentDomainsMultiplePartitionServices";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1500);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize / 2;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));

                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));

                ++cnt;
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 40, 20,
            1, 40, 20,
            2, 40, 20,
            3, 40, 20,
            4, 40, 20,
            5, 40, 20,
            6, 40, 20,
            7, 40, 20,
            8, 40, 20,
            9, 40, 20
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        auto countEmptyNodes = [&](int& numOfEmptyNodes, int& numOfFDs, int& numOfUDs) -> void
        {
            bool usedFD[8];
            bool usedUD[8];

            for (int i = 0; i < 8; ++i)
            {
                usedFD[i] = usedUD[i] = false;
            }

            numOfEmptyNodes = numOfFDs = numOfUDs = 0;

            for (size_t i = 0; i < nodeCount; ++i)
            {
                ServiceModel::NodeLoadInformationQueryResult queryResult;

                ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(NodeId(LargeInteger(0, i)), queryResult);

                for (auto it = queryResult.NodeLoadMetricInformation.begin(); it != queryResult.NodeLoadMetricInformation.end(); ++it)
                {
                    if (!StringUtility::AreEqualCaseInsensitive(it->Name, metric)) continue;

                    if (it->NodeLoad == 0)
                    {
                        ++numOfEmptyNodes;

                        if (!usedFD[i % 4])
                        {
                            ++numOfFDs;
                            usedFD[i % 4] = true;
                        }

                        if (!usedUD[i % 3])
                        {
                            ++numOfUDs;
                            usedUD[i % 3] = true;
                        }
                    }
                }
            }
        };

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        fm_->ApplyActions();
        for (auto fud = fm_->FuMap.begin(); fud != fm_->FuMap.end(); ++fud)
        {
            plb.UpdateFailoverUnit(move(FailoverUnitDescription(fud->second)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        fm_->RefreshPLB(now);

        int emptyNodeCount = 0, FDCount = 0, UDCount = 0;
        countEmptyNodes(emptyNodeCount, FDCount, UDCount);

        VERIFY_IS_TRUE(emptyNodeCount == freeNodesTarget);
        VERIFY_IS_TRUE(FDCount == freeNodesTarget);
        VERIFY_IS_TRUE(UDCount == freeNodesTarget);
    }

    BOOST_AUTO_TEST_CASE(LoadReservationWithDifferentNodeCapacityTest)
    {
        // Test scenario:
        // Node/type   0/A  1/A  2/A  3/A  4/A  5/B  6/B  7/B  8/B  9/B  
        // Capacity    200  200  200  200  200  110  110  110  110  110 
        // Load        150  150   50   50   50   50   50   50   50   50
        //
        // Each replica has 10 load, there are 10 services with replicas on nodes 1 and 2, constrained to A nodeType
        // and 5 other (primary)replicas from different services on each node
        // PLB tries to reserve 70 load on 4 nodes (there are 3 "empty enough" nodes in start")
        // Goal should be done by making only 1 move of replica from nodes 5-9

        wstring testName = L"LoadReservationWithDifferentNodeCapacityTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 10);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 10);

        wstring metric = L"Metric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 2));
        PLBConfigScopeChange(PlacementStrategy, PLBConfig::KeyIntegerValueMap, placementStrategy);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        int freeNodesTarget = 4;
        float emptyNodeWeight = 1.0;
        int reservedLoadOnNodes = 70;

        PLBConfig::KeyDoubleValueMap nodesWithReservation;
        nodesWithReservation.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, nodesWithReservation);

        PLBConfig::KeyIntegerValueMap reservedLoadPerNode;
        reservedLoadPerNode.insert(make_pair(metric, reservedLoadOnNodes));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservedLoadPerNode);

        PLBConfig::KeyDoubleValueMap defragmentationEmptyNodeWeight;
        defragmentationEmptyNodeWeight.insert(make_pair(metric, emptyNodeWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, defragmentationEmptyNodeWeight);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeIndex = 0;
        for (; nodeIndex < 5; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"A"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/200"));
        }
        for (; nodeIndex < 10; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"B"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, wformatString(L"fd/{0}", nodeIndex), wformatString(L"ud{0}", nodeIndex), move(nodeProperties), L"Metric/110"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"OtherType"), set<NodeId>()));

        wstring serviceName;

        int replicaSize = 10;
        int j = 0;
        for (; j < 10; ++j)
        {
            serviceName = wformatString("ServiceOnANodeType_{0}", j);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"TestType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L"NodeType==A"),            //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                2,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(j), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
        }

        for (; j < 60; ++j)
        {
            serviceName = wformatString("Service_{0}", j);
            plb.UpdateService(ServiceDescription(
                wstring(serviceName),
                wstring(L"OtherType"),
                wstring(L""),
                true,                               //bool isStateful,
                wstring(L""),                       //placementConstraints,
                wstring(),                          //affinitizedService,
                true,                               //alignedAffinity,
                CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                FABRIC_MOVE_COST_LOW,
                false,
                1,                                  // partition count
                1,                                  // target replica set size
                false));                            // persisted
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(j), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", j % 10)), 0));
        }
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(1 <= CountIf(actionList, ActionMatch(L"* move * 5|6|7|8|9=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragScopedTriggeringEmptyNodesPositive)
    {
        wstring testName = L"DefragScopedTriggeringEmptyNodesPositive";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 50, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"LoadBalancing");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_FALSE(actionList.empty());
    }

    BOOST_AUTO_TEST_CASE(DefragScopedTriggeringEmptyNodesNegative)
    {
        wstring testName = L"DefragScopedTriggeringEmptyNodesNegative";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 50, 25,
            1, 50, 25,
            2, 50, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 0, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"NoActionNeeded");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.empty());
    }

    BOOST_AUTO_TEST_CASE(DefragScopedTriggeringBalancingThresholdNegative)
    {
        wstring testName = L"DefragScopedTriggeringBalancingThresholdNegative";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 1.0f; // Don't balance

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 3.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 200, 25,
            1, 200, 25,
            2, 200, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 0, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"NoActionNeeded");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.empty());
    }

    BOOST_AUTO_TEST_CASE(DefragScopedTriggeringBalancingThresholdNegative2)
    {
        wstring testName = L"DefragScopedTriggeringBalancingThresholdNegative2";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 0.9f; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 3.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 100, 25,
            1, 100, 25,
            2, 100, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 0, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"NoActionNeeded");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.empty());
    }

    BOOST_AUTO_TEST_CASE(DefragScopedTriggeringBalancingThresholdPositive)
    {
        wstring testName = L"DefragScopedTriggeringBalancingThresholdPositive";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        int freeNodesTarget = 3;
        float defragWeight = 0.9f; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 3.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/200", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        int config[nodeCount][3] =
        {
            0, 200, 25,
            1, 200, 25,
            2, 200, 25,
            3, 50, 25,
            4, 50, 25,
            5, 50, 25,
            6, 50, 25,
            7, 0, 25,
            8, 0, 25,
            9, 0, 25
        };

        for (int i = 0; i < nodeCount; ++i)
            if (config[i][1] > 0) fillNode(config[i][0], config[i][1], config[i][2]);

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"LoadBalancing");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_FALSE(actionList.empty());
    }

    BOOST_AUTO_TEST_CASE(DefragScopedNonEmptyNodesWeightTest)
    {
        wstring testName = L"DefragScopedNonEmptyNodesWeightTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(metric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent, PLBConfig::KeyDoubleValueMap, placementHeuristicEmptySpacePercent);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 4;
        int freeNodesTarget = 1;
        float defragEmptyNodesWeight = 0.5;
        float defragNonEmptyNodesWeight = 0;

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragEmptyNodesWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap scopedDefragNonEmptyWeight;
        scopedDefragNonEmptyWeight.insert(make_pair(metric, defragNonEmptyNodesWeight));
        PLBConfigScopeChange(DefragmentationNonEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragNonEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 3.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("dc0/r{0}", i % 4), wformatString("{0}", i % 3), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        auto fillNode = [&](int nodeIndex, int loadToAdd, int replicaSize) -> void
        {
            int numToAdd = loadToAdd / replicaSize;

            for (int i = 0; i < numToAdd; ++i)
            {
                wstring serviceName = wformatString("{0}_Service_{1}", testName, ++cnt);
                plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/{1}/{2}", metric, replicaSize, 0))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(cnt), FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", nodeIndex)), 0)));
            }
        };

        fillNode(0, 10, 10);
        fillNode(0, 40, 40);
        fillNode(1, 10, 10);
        fillNode(1, 20, 20);
        fillNode(2, 10, 10);
        fillNode(2, 20, 20);
        fillNode(3, 10, 10);

        // Initial placement:
        // Nodes    | N0(M1:50/100) | N1(M1:30/100) | N2(M1:30/100) | N2(M1:10/100) |
        // Service1 |       P       |               |               |               |
        // Service2 |       P       |               |               |               |
        // Service3 |               |       P       |               |               |
        // Service4 |               |       P       |               |               |
        // Service5 |               |               |       P       |               |
        // Service6 |               |               |       P       |               |
        // Service7 |               |               |               |       P       |

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();

        fm_->ClearMoveActions();
        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"LoadBalancing");

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 move primary 3=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(RestrictedDefragBasicTest)
    {
        // There are 50 nodes, each with 8 replicas
        // Replicas on 7 nodes (one node in each FD and UD) have low move cost and higher load, while on the other moveCost is high but load is lower
        // Defrag tries to empty 5 nodes with the least moves, without moving replicas with high move cost
        // The solution would be to choose 5 out of 7 nodes with low move cost, and to move only those replicas making 40 lowcost moves

        wstring testName = L"RestrictedDefragBasicTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 50;
        int freeNodesTarget = 5;
        float defragWeight = 1.0; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(RestrictedDefragmentationHeuristicEnabled, bool, true);


        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 7), wformatString("{0}", i % 7), wformatString("{0}/1000", metric)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("TestType");
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        for (int i = 0; i < nodeCount; i++)
        {
            if (i % 8)
            {
                for (int j = 0; j < 8; ++j)
                {
                    int replicaSize = 10 + j;
                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        FABRIC_MOVE_COST_HIGH,
                        false,
                        1,                                // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
            else
            {
                for (int j = 0; j < 8; ++j)
                {
                    int replicaSize = 20 + j;
                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        FABRIC_MOVE_COST_LOW,
                        false,
                        1,                                // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 5);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() == 40);
    }

    BOOST_AUTO_TEST_CASE(RestrictedDefragTestEmptyNodes)
    {
        // Testing restricted defrag when emptying nodes, without taking care of FD and UD
        // 100 nodes, 15 nodes with more replicas and more load but with low move cost, the other nodes with less replicas, although some have high movecost
        // Those 15 nodes are in 2 FDs, while defrag empty 10 nodes
        // Defrag needs just to empty 10 out of 15 nodes with low move cost, without making additional moves

        wstring testName = L"RestrictedDefragTestEmptyNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 100;
        int freeNodesTarget = 10;
        float defragWeight = 1.0; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(RestrictedDefragmentationHeuristicEnabled, bool, true);

        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 7), wformatString("{0}", i % 7), wformatString("{0}/200", metric)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("TestType");
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        for (int i = 0; i < nodeCount; i++)
        {
            if (i > 15)
            {
                for (int j = 0; j < 6; ++j)
                {
                    int replicaSize = 1;
                    auto moveCost = FABRIC_MOVE_COST_LOW;
                    if (j % 3)
                    {
                        moveCost = FABRIC_MOVE_COST_HIGH;
                    }
                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        moveCost,
                        false,
                        1,                                  // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
            else
            {
                for (int j = 0; j < 6; ++j)
                {
                    int replicaSize = 20;
                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        FABRIC_MOVE_COST_LOW,
                        false,
                        1,                                  // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 5);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() == 60);
    }

    BOOST_AUTO_TEST_CASE(RestrictedDefragTestEmptyNodesAcrossDomains)
    {
        // Testing restricted defrag when emptying nodes in different FDs and UDs while also efficiency
        // 100 nodes are distributed in 10 FDs and 10 UDs, 10 nodes in each
        // 4 nodes with more replicas and more load but with low move cost, the other nodes with less replicas, although some have high movecost
        // Those 4 nodes are in 2 FDs and 2 UDs, while defrag empty 10 nodes
        // To be optimal defrag needs to clear 2 of those 4 nodes,
        // and to clear additional 8 from those who contains some high movecost replicas but without making additional moves

        wstring testName = L"RestrictedDefragTestEmptyNodesAcrossDomains";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBBalancingTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        wstring metric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 100;
        int freeNodesTarget = 10;
        float defragWeight = 1.0; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(RestrictedDefragmentationHeuristicEnabled, bool, true);

        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString(L"dc0/r{0}", i / 10), wformatString("{0}", i % 10), wformatString("{0}/200", metric)));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("TestType");
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int cnt = 0;

        for (int i = 0; i < nodeCount; i++)
        {
            // Nodes 0th and 1st FD and UD
            if (i == 0 || i == 10 || i == 1 || i == 11)
            {
                for (int j = 0; j < 8; ++j)
                {
                    int replicaSize = 9 + j;
                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        FABRIC_MOVE_COST_LOW,
                        false,
                        1,                                  // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
            else
            {
                for (int j = 0; j < 6; ++j)
                {
                    int replicaSize = 10 + j;
                    auto moveCost = FABRIC_MOVE_COST_LOW;
                    if (j % 3 == 1)
                    {
                        moveCost = FABRIC_MOVE_COST_MEDIUM;
                    }
                    if (j % 3 == 2)
                    {
                        moveCost = FABRIC_MOVE_COST_HIGH;
                    }

                    wstring serviceName = wformatString("Service_{0}", ++cnt);
                    plb.UpdateService(ServiceDescription(
                        wstring(serviceName),
                        wstring(L"TestType"),
                        wstring(L""),
                        true,                               //bool isStateful,
                        wstring(L""),                       //placementConstraints,
                        wstring(),                          //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, 0)),
                        moveCost,
                        false,
                        1,                                  // partition count
                        1,                                  // target replica set size
                        false));                            // persisted
                    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(cnt), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                }
            }
        }
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 2);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() == 64);
    }

    BOOST_AUTO_TEST_CASE(RestrictedDefragTestWithDeactivatedNodes)
    {
        // Test checks if nodes with Deactivation intent, while safety check in progress can be picked for emptying in defrag

        wstring testName = L"RestrictedDefragTestWithDeactivatedNodes";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);

        wstring metric = L"DefragMetric";

        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(metric, true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(metric, Metric::DefragDistributionType::SpreadAcrossFDs_UDs));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int freeNodesTarget = 2;
        float defragWeight = 1.0; // Balance after emptying out nodes

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, freeNodesTarget));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, defragWeight));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(RestrictedDefragmentationHeuristicEnabled, bool, true);
        PLBConfigScopeChange(AllowConstraintCheckFixesDuringApplicationUpgrade, bool, true);

        // Nodes 1-5 have deactivation intent, and 2 replicas on them
        // Node 6 has 4 replicas, node 7 has 3 replicas and node 8 is empty
        // Defrag goal is to empty 2 nodes
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"10_fd", L"UD1", wformatString("{0}/100", metric),
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"20_fd", L"UD2", wformatString("{0}/100", metric),
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"30_fd", L"UD3", wformatString("{0}/100", metric),
            Reliability::NodeDeactivationIntent::Enum::RemoveData,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"40_fd", L"UD4", wformatString("{0}/100", metric),
            Reliability::NodeDeactivationIntent::Enum::RemoveNode,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"50_fd", L"UD5", wformatString("{0}/100", metric),
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"60_fd", L"UD6", wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(7, L"70_fd", L"UD7", wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(8, L"80_fd", L"UD8", wformatString("{0}/100", metric)));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int replicaSize = 10;
        plb.UpdateService(CreateServiceDescription(wstring(L"Service1"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service2"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service3"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service4"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service5"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service6"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));
        plb.UpdateService(CreateServiceDescription(wstring(L"Service7"), testType, true, CreateMetrics(wformatString("{0}/0.3/{1}/{2}", metric, replicaSize, replicaSize))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L"P/1,S/2,S/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L"P/2,S/3,S/7"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"Service3"), 0, CreateReplicas(L"P/3,S/4,S/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"Service4"), 0, CreateReplicas(L"P/4,S/5,S/7"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"Service5"), 0, CreateReplicas(L"P/5,S/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"Service6"), 0, CreateReplicas(L"P/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"Service7"), 0, CreateReplicas(L"P/7,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"* move * 7=>6", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingDuringApplicationUpgrade)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingDuringApplicationUpgrade");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::FromTicks(1));

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"TestType", L"App1", true, CreateMetrics(L"")));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"LoadBalancing");

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));

        PLBConfigScopeChange(AllowBalancingDuringApplicationUpgrade, bool, false);
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        VerifyPLBAction(plb, L"NoActionNeeded");

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 1, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 1, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BalancingDuringMultipleApplicationUpgrade)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingDuringMultipleApplicationUpgrade");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(AllowBalancingDuringApplicationUpgrade, bool, false);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            true));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"")));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/1, S/0"), 0, upgradingFlag));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        VerifyPLBAction(plb, L"NoActionNeeded");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 2, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().MinLoadBalancingInterval);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BalancingDuringApplicationUpgradeDifferentDomains)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingDuringApplicationUpgradeDifferentDomains");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(AllowBalancingDuringApplicationUpgrade, bool, false);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            false));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"Cpu/1.0/100/50")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"Memory/1.0/100/50")));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"LoadBalancing", L"Cpu");
        VerifyPLBAction(plb, L"NoActionNeeded", L"Memory");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            true));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            10,
            10,
            CreateApplicationCapacities(wformatString(L"")),
            false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        VerifyPLBAction(plb, L"NoActionNeeded", L"Cpu");
        VerifyPLBAction(plb, L"LoadBalancing", L"Memory");
    }

    BOOST_AUTO_TEST_CASE(BalancingStartTemperatureTest)
    {
        // Scenario is testing effectiveness of different initial temperature calculation on small cluster.
        // It verifies the situation when heuristic moves can be picked to balance cluster as best solution.
        // There are 10 nodes, 5 with 60+60+20 and 5 with 50+50 load
        // Balancing should find solution with only 5 moves, that make perfect balance of the cluster
        // Without change, how initial temperature is calculating, test fails.
        wstring testName = L"BalancingTestCase";
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingTestCase");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int i = 0; i < 5; ++i)
        {
            wstring serviceName = wformatString("{0}_Service_{1}", testName, 3 * i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/60/60")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3 * i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));

            serviceName = wformatString("{0}_Service_{1}", testName, 3 * i + 1);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/60/60")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3 * i + 1), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));

            serviceName = wformatString("{0}_Service_{1}", testName, 3 * i + 2);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/20/20")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3 * i + 2), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));

        }

        for (int i = 5; i < 10; ++i)
        {
            wstring serviceName = wformatString("{0}_Service_{1}", testName, 3 * i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/50/50")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3 * i), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));

            serviceName = wformatString("{0}_Service_{1}", testName, 3 * i + 1);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/50/50")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3 * i + 1), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 5);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);
        PLBConfigScopeChange(MaxPercentageToMove, double, 0.4);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);
        PLBConfigScopeChange(ClusterSpecificTemperatureCoefficient, double, 0);

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //testing if it will choose heuristic moves
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 1=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 2=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 3=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 4=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(StatelessServicesPlacementAndBalancingTest)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "StatelessServicesPlacementAndBalancingTest");

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000); //number of iterations per round
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1); //number of rounds

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);

        const int partitionCount = 10;

        Common::Random random;

        int fdCount = random.Next(2, 6);
        int udCount = fdCount == 2 ? random.Next(3, 6) : random.Next(2, 6);
        int nodeCount = random.Next(6, fdCount * udCount + 1);
        int instanceCount = random.Next(5, nodeCount);

        fm_->Load(random.Next());
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::vector<std::vector<bool>> placed(fdCount, vector<bool>(udCount, false));

        // Node placement
        for (int nodeId = 0, i = 0, j = 0; nodeId < nodeCount; ++nodeId)
        {
            while (placed[i % fdCount][j % udCount])
            {
                j++;
            }

            plb.UpdateNode(CreateNodeDescription(nodeId, wformatString("{0}", i % fdCount), wformatString("{0}", j % udCount)));
            placed[i++ % fdCount][j++ % udCount] = true;

        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceName = wstring(L"PartitionedService");
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false, std::vector<ServiceMetric>(), 1, false, instanceCount));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), instanceCount));

        // Replica placement
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        int countAgain = 1;
        while (actionList.size() != instanceCount && countAgain <= 5)
        {
            PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
            fm_->ClearMoveActions();
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), instanceCount));
            plb.Refresh(Stopwatch::Now());
            actionList = GetActionListString(fm_->MoveActions);
        }

        VERIFY_ARE_EQUAL(instanceCount, CountIf(actionList, ActionMatch(L"0 add * *", value)));

        fm_->ClearMoveActions();
        wstring distribution = L"";
        for (int i = 0; i < instanceCount; ++i)
        {
            vector<wstring> split;
            StringUtility::Split<wstring>(actionList[i], split, L" ");
            distribution += wstring(L"S/");
            distribution += split[3];
            distribution += wstring(L",");
        }

        Trace.WriteInfo("PLBStatelessServicesPlacementAndBalancingTestSource",
            "Setup: FD count: {0}, UD count: {1}, Node count: {2}, Instance count: {3}, Distribution: {4}",
            fdCount, udCount, nodeCount, instanceCount, distribution);

        for (int fuId = 0; fuId < partitionCount; ++fuId)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName), 1, CreateReplicas(wstring(distribution)), 0));
        }

        plb.Refresh(Stopwatch::Now());

        VerifyPLBAction(plb, L"LoadBalancing");
        if (!(fdCount == 3 && udCount == 3 && nodeCount == 7 && instanceCount == 6))
        {
            VERIFY_ARE_NOT_EQUAL(0, fm_->MoveActions.size());
        }

    }

    BOOST_AUTO_TEST_CASE(LoadBalancingWithShouldDisappearLoadTest)
    {
        LoadBalancingWithShouldDisappearLoadTestHelper(L"LoadBalancingWithShouldDisappearLoadTest", true);
    }

    BOOST_AUTO_TEST_CASE(LoadBalancingWithShouldDisappearLoadTestFeatureSwitchOff)
    {
        LoadBalancingWithShouldDisappearLoadTestHelper(L"LoadBalancingWithShouldDisappearLoadTestFeatureSwitchOff", false);
    }

    // This test is the simulation that systematically checks if unused nodes can be utilized.
    // Formula for switching between '+1' and quorum based logic is determined by this simulation.
    // It's not 'classic' boost test so shouldn't be used on regular basis. 
    // Be aware that it can execute for hours in debug mode, so run it in retail mode (<10min with current constants) if necessary.
    /*
    BOOST_AUTO_TEST_CASE(StatelessServicesPlacementAndBalancingSimulation)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "StatelessServicesPlacementAndBalancingSimulation");

        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000); //number of iterations per round
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1); //number of rounds

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);

        const int fdCount = 5;
        const int udCount = 5;
        const int partitionCount = 10;
        const int minNumberOfInstances = 5;

        size_t randomReplicaPlacementCount = 10;

        Common::Random random;

        for (int fd = 2; fd <= fdCount; ++fd)
        {
            for (int ud = fd; ud <= udCount; ++ud)
            {
                for (int nodeCount = min(fd, ud); nodeCount <= fd * ud; ++nodeCount)
                {
                    for (int instance = minNumberOfInstances; instance <= nodeCount - 1; ++instance)
                    {
                        for (int iteration = 0; iteration < randomReplicaPlacementCount; ++iteration)
                        {
                            fm_->Load(random.Next());
                            PlacementAndLoadBalancing & plb = fm_->PLB;

                            std::vector<std::vector<bool>> placed(fd, vector<bool>(ud, false));

                            // Node placement
                            for (int nodeId = 0, i = 0, j = 0; nodeId < nodeCount; ++nodeId)
                            {
                                while (placed[i % fd][j % ud])
                                {
                                    j++;
                                }

                                plb.UpdateNode(CreateNodeDescription(nodeId, wformatString("{0}", i % fd), wformatString("{0}", j % ud)));
                                placed[i++ % fd][j++ % ud] = true;

                            }

                            // Force processing of pending updates so that service can be created.
                            plb.ProcessPendingUpdatesPeriodicTask();

                            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

                            wstring serviceName = wstring(L"PartitionedService");
                            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false, std::vector<ServiceMetric>(), 1, false, instance));

                            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), instance));

                            // Replica placement
                            plb.Refresh(Stopwatch::Now());

                            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

                            int countAgain = 1;
                            while (actionList.size() != instance && countAgain <= 5)
                            {
                                PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
                                fm_->ClearMoveActions();
                                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), instance));
                                plb.Refresh(Stopwatch::Now());
                                actionList = GetActionListString(fm_->MoveActions);

                            }

                            VERIFY_ARE_EQUAL(instance, CountIf(actionList, ActionMatch(L"0 add * *", value)));

                            fm_->ClearMoveActions();

                            wstring distribution = L"";

                            for (int i = 0; i < instance; ++i)
                            {
                                vector<wstring> split;
                                StringUtility::Split<wstring>(actionList[i], split, L" ");
                                distribution += wstring(L"S/");
                                distribution += split[3];
                                distribution += wstring(L",");
                            }

                            for (int fuId = 0; fuId < partitionCount; ++fuId)
                            {
                                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName), 1, CreateReplicas(wstring(distribution)), 0));
                            }

                            plb.Refresh(Stopwatch::Now());

                            if (fm_->MoveActions.size() == 0)
                            {
                                printf("No LoadBalancing actions\n");
                                printf("FD count: %d\n", fd);
                                printf("UD count: %d\n", ud);
                                printf("Node count: %d\n", nodeCount);
                                printf("Instance count: %d\n", instance);
                                printf("------------------------------\n");

                                Trace.WriteInfo("PLBStatelessServicesPlacementAndBalancingSimulationSource",
                                    "No LoadBalancing actions: FD count: {0}, UD count: {1}, Node count: {2}, Instance count: {3}, Distribution: {4}",
                                    fd, ud, nodeCount, instance, distribution);
                            }

                            VerifyPLBAction(plb, L"LoadBalancing");
                        }
                    }
                }
            }
        }
    }
    */
    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBBalancing::LoadBalancingWithShouldDisappearLoadTestHelper(wstring const& testName, bool featureSwitch)
    {
        // Node capacity is 100 for both nodes.
        //  Node 0: 100 load, 0 disappearing load
        //  Node 1: 0 load, 100 disappearing load
        // Cluster is imbalanced:
        //      Feature switch on: no moves can be made because node 1 is full of disappearing load.
        //      Feature switch off: moves can be made regardless of node 1 that is full of disappearing load.
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(CountDisappearingLoadForSimulatedAnnealing, bool, featureSwitch);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            serviceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/1/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/1/D"), 0));

        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        size_t expectedMoves = featureSwitch ? 0 : 1;
        VERIFY_ARE_EQUAL(expectedMoves, actionList.size());
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    void TestPLBBalancing::BalancingThrottlingHelper(uint limit)
    {
        // This test helper expects that number of movements is throttled to limit
        // Throttling is set in the test that is calling the helper
        PlacementAndLoadBalancing & plb = fm_->PLB;

        vector<wstring> actionList;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // force processing of pending updates so that service can be created
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"ThrottlingTestService", L"TestType", true));

        int fuIndex = 0;

        for (int i = 0; i < 100; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"ThrottlingTestService"), 0, CreateReplicas(L"P/0"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_IS_TRUE(actionList.size() <= limit);
        VERIFY_IS_TRUE(actionList.size() > 0u);
    }

    void TestPLBBalancing::BalancingWithDownNodesHelper(wstring const& testName, bool replicasOnDownNode)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // 2 up nodes
        int nodeIndex = 0;
        plb.UpdateNode(CreateNodeDescription(nodeIndex++));
        plb.UpdateNode(CreateNodeDescription(nodeIndex++));

        // One down node!
        plb.UpdateNode(NodeDescription(CreateNodeInstance(nodeIndex++, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        // One stateless service without any metrics
        wstring serviceTypeName = wformatString("{0}_Type", testName);
        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(wstring(serviceName), wstring(serviceTypeName), false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1"), 0));

        if (replicasOnDownNode)
        {
            // 4 instances on down node - to push the node over the average load.
            // This should not happen in prduction, but we are testing two different things with these replicas:
            //  - That constraint check will not kick in to move them from the down node (first check with NoActionNeeded).
            //  - That fast balancing will correctly calculate the over/under nodes and that it will move things around.
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"I/2"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName), 0, CreateReplicas(L"I/2"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(L"I/2"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(serviceName), 0, CreateReplicas(L"I/2"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 0u);
        VerifyPLBAction(plb, L"NoActionNeeded");

        // Add one more node and make sure that balancing moves 2 instances to it!
        plb.UpdateNode(CreateNodeDescription(nodeIndex++));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 2u);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move instance 0|1 => 3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    bool TestPLBBalancing::ClassSetup()
    {
        Trace.WriteInfo("PLBBalancingTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBBalancing::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBBalancing::ClassCleanup()
    {
        Trace.WriteInfo("PLBBalancingTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }

    void TestPLBBalancing::BalancingWithStandByReplicaHelper(wstring testName, PlacementAndLoadBalancing & plb)
    {
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/110", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/50", metric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/50/50", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,SB/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/60/60", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
    }

    int TestPLBBalancing::BalancingWithDeactivatedNodeHelper(wstring testName, PlacementAndLoadBalancing & plb)
    {
        int nodeId = 0;
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, wformatString(L"dc0/r{0}", 0), L"MyMetric/200",
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));

        for (; nodeId < 3; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId, wformatString(L"dc0/r{0}", nodeId), L"MyMetric/200"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        int index = 0;
        wstring testType = wformatString("{0}{1}_Type", testName, index);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        wstring serviceName = wformatString("{0}{1}_InitialService", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1,S/2"), 0));
        }

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        0           0       200         Pause               DeactivationSafetyCheckInProgress
        // 1        5           100     200         None                None
        // 2        5           100     200         None                None
        plb.ProcessPendingUpdatesPeriodicTask();

        return index;
    }

    int TestPLBBalancing::DefragWithDeactivatedNodeHelper(wstring testName, PlacementAndLoadBalancing & plb, wstring & serviceName)
    {
        int nodeId = 0;
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
            nodeId++, L"dc0/r0", L"0", L"MyMetric/100",
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));

        for (; nodeId < 3; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(nodeId, L"dc0/r0", L"0", L"MyMetric/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/10/10")));

        int index = 0;
        for (int i = 0; i < 4; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));
        }

        // NodeId   Replicas#   Load    Capacity    DeactivationIntent  DeactivationStatus
        // 0        0           0       100         Pause               DeactivationSafetyCheckInProgress
        // 1        0           0       100         None                None
        // 2        4           80      100         None                None
        plb.ProcessPendingUpdatesPeriodicTask();

        return index;
    }

    StopwatchTime TestPLBBalancing::BalancingNotNeededWithBlocklistedMetricHelper(
        PlacementAndLoadBalancing & plb,
        wstring metric,
        bool runPlbAndCheck)
    {
        int nodeIndex = 0;
        for (; nodeIndex < 2; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"0", move(nodeProperties), wformatString("{0}/100", metric)));
        }
        for (; nodeIndex < 4; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"1", move(nodeProperties), wformatString("{0}/100", metric)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            L"StatefulService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateService(ServiceDescription(
            L"StatelessService",
            wstring(L"TestType"),
            wstring(L""),                       //app name
            false,                              //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            CreateMetrics(wformatString("{0}/1/10/10", metric)),
            FABRIC_MOVE_COST_LOW,
            false,                              // on every node
            0,                                  // partition count
            0,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"StatefulService"), 0, CreateReplicas(L"S/0, P/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"StatelessService"), 0, CreateReplicas(L"S/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"StatelessService"), 0, CreateReplicas(L"S/0, S/1"), 0));

        StopwatchTime now = Stopwatch::Now();

        if (runPlbAndCheck)
        {
            fm_->RefreshPLB(now);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);
            VerifyPLBAction(plb, L"NoActionNeeded");
            VERIFY_ARE_EQUAL(0u, actionList.size());
        }

        return now;
    }

    void TestPLBBalancing::SwapCostTestHelper(PlacementAndLoadBalancing & plb, wstring const& testName)
    {
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        // Force processing of pending updates
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        wstring serviceNameHigh = wformatString("{0}_HighLoadService", testName);
        wstring serviceNameLow = wformatString("{0}_LowLoadService", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(wstring(serviceNameLow), wstring(serviceTypeName), true, CreateMetrics(L"MyMetric/1.0/10/10")));
        plb.UpdateService(CreateServiceDescription(wstring(serviceNameHigh), wstring(serviceTypeName), true, CreateMetrics(L"MyMetric/1.0/30/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceNameHigh), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceNameLow), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceNameLow), 0, CreateReplicas(L"P/0"), 0));

        // Node Loads:
        // Node 0 == 50, Node 1 == 10
        // Two options exist (depends on test settings):
        //   - Move two primaries of low service to node 1 to achieve (30, 30)
        //   - Swap high service to achieve (30, 30)

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
    }

    void TestPLBBalancing::BalancingWithGlobalAndIsolatedMetricTest()
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithGlobalAndIsolatedMetricTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int index = 0;
        for (int i = 0; i < 100; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2, S/3"), 0));
        }

        for (int i = 100; i < 200; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"PrimaryCount/1.0/0/0,ReplicaCount/1.0/0/0")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2, S/3"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        map<Guid, FailoverUnitMovement> const& actionList = fm_->MoveActions;

        VERIFY_ARE_EQUAL(0, CountIf(actionList, value.first.AsGUID().Data1 < 200));
        VERIFY_IS_TRUE(0 <  actionList.size());
    }

    void TestPLBBalancing::BalancingWithToBePlacedReplicasTest()
    {
        Trace.WriteInfo("PLBBalancingTestSource", "BalancingWithToBePlacedReplicasTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1)); // P/0, S/3
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2/B"), 0)); // P/1, S/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/1"), 0));  // P/4, S/0
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/1/B"), 0)); // P/3, S/1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/4"), 0)); // P/2, S/4

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        ScopeChangeObjectMaxSimulatedAnnealingIterations.SetValue(20);
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now); // call it twice so that the second call should do the balancing

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //should have 3 movements
        // the two primary moves could be 3->4,1->2 or 3->2,1->4
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* add secondary 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary *=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 3=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move primary 1=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move secondary 1=>0", value)));
    }
}
