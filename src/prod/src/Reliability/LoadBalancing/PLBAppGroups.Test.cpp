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


    class TestPLBAppGroups
    {
    protected:
        TestPLBAppGroups() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBAppGroups()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        void VerifyReservedCapacityAndLoad(wstring const& metricName, int64 expectedReservedCapacity, int64 expectedReservedLoadUsed);
        void VerifyAllApplicationReservedLoadPerNode(wstring const& metricName, int64 expectedReservedLoadUsed, Federation::NodeId nodeId);

        shared_ptr<TestFM> fm_;

        Reliability::FailoverUnitFlags::Flags upgradingFlag_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBAppGroupsSuite, TestPLBAppGroups)

    BOOST_AUTO_TEST_CASE(AppGroupsReservationAssertNoneReplicaRepro)
    {

        wstring testName = L"AppGroupsReservationAssertNoneReplicaRepro";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"", false));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"", true));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            2,
            CreateApplicationCapacities(wformatString(L"DwNodeUnit/2/1/1"))));


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // This one reports the metric!
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"ParentService", L"TestType", L"App1", true, CreateMetrics(L"DwNodeUnit/1/1/1")));

        plb.UpdateService(ServiceDescription(wstring(L"IrrelevantService"),
            wstring(L"TestType"),
            wstring(L"App1"),
            true,
            wstring(L""),
            wstring(L"ParentService"),
            true,
            move(CreateMetrics(L"CPU/1/1/1")),
            FABRIC_MOVE_COST_LOW,
            false));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ParentService"), 0, CreateReplicas(L"S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"ParentService"), 0, CreateReplicas(L"S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"IrrelevantService"), 0, CreateReplicas(L"S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // This update can assert if reserved load is miscalculated earlier.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ParentService"), 1, CreateReplicas(L"N/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"ParentService"), 1, CreateReplicas(L"N/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"IrrelevantService"), 1, CreateReplicas(L"N/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
    }

    BOOST_AUTO_TEST_CASE(AppGroupsComplexCase)
    {
        //in this test we have a huge number of apps in app capacity violation (stacking of primary replicas)
        //there is also affinity, reservation
        //this can be fixed with swaps only
        wstring testName = L"AppGroupsComplexCase";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(ConstraintCheckIterationsPerRound, int, 5);

        for (int i = 0; i < 60; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wstring(L"Cpu/100")));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceIndex = 0;
        int guidIndex = 0;
        int nodeIndex = 0;
        for (int i = 0; i < 20; ++i)
        {
            wstring appName = wformatString(L"App{0}", i);
            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                appName,
                2,
                2,
                CreateApplicationCapacities(wformatString(L"DwNodeUnit/100000/3/1,Cpu/100000/10000/100"))));

            auto parent1 = wformatString(L"TestService{0}", serviceIndex++);
            auto parent2 = wformatString(L"TestService{0}", serviceIndex++);

            auto replicaPlacement = wformatString(L"P/{0},S/{1}", nodeIndex, nodeIndex + 1);
            nodeIndex += 2;

            plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(parent1), L"TestType", appName, true, CreateMetrics(L"DwNodeUnit/1/2/1")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(parent1), 0, CreateReplicas(wstring(replicaPlacement)), 0));
            plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(parent2), L"TestType", appName, true, CreateMetrics(L"DwNodeUnit/1/2/1")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(parent2), 0, CreateReplicas(wstring(replicaPlacement)), 0));
            for (int j = 0; j < 10; ++j)
            {
                auto childService = wformatString(L"TestService{0}", serviceIndex++);
                plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(wstring(childService), L"TestType", true, appName, parent1, CreateMetrics(L"")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(childService), 0, CreateReplicas(wstring(replicaPlacement)), 0));
            }
            for (int j = 0; j < 10; ++j)
            {
                auto childService = wformatString(L"TestService{0}", serviceIndex++);
                plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(wstring(childService), L"TestType", true, appName, parent2, CreateMetrics(L"")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(childService), 0, CreateReplicas(wstring(replicaPlacement)), 0));
            }
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(220u, actionList.size());
        VERIFY_ARE_EQUAL(220, CountIf(actionList, ActionMatch(L"* swap primary *<=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservationAssertRepro)
    {
        // This is the case that was causing assert to happen:
        //  - Application has reservation on some metric A.
        //  - There is a single replica from that app on one node.
        //  - Load is reported for any metric for that partition.
        //  - Next update (load or FT) is going to cause assert.
        // We also include affinity in this test to repro the original case found in SQL cluster.
        wstring testName = L"AppGroupsReservationAssertRepro";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"", false));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"", true));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            2,
            CreateApplicationCapacities(wformatString(L"DwNodeUnit/2/1/1"))));


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // This one reports the metric!
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"ParentService", L"TestType", L"App1", true, CreateMetrics(L"DwNodeUnit/1/1/1")));

        plb.UpdateService(ServiceDescription(wstring(L"TestService1"),
            wstring(L"TestType"),
            wstring(L"App1"),
            true,
            wstring(L""),
            wstring(L"ParentService"),
            true,
            move(CreateMetrics(L"CPU/1/1/1")),
            FABRIC_MOVE_COST_LOW,
            false));

        plb.UpdateService(ServiceDescription(wstring(L"IrrelevantService"),
            wstring(L"TestType"),
            wstring(L"App1"),
            true,
            wstring(L""),
            wstring(L"ParentService"),
            true,
            move(CreateMetrics(L"CPU/1/1/1")),
            FABRIC_MOVE_COST_LOW,
            false));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"ParentService"), 0, CreateReplicas(L"P/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(13), wstring(L"IrrelevantService"), 0, CreateReplicas(L"P/1/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"ParentService"), 1, CreateReplicas(L"P/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // This one can cause problems.
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"CPU", 10, 10));

        plb.ProcessPendingUpdatesPeriodicTask();

        // This update can assert if reserved load is miscalculated earlier.
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"CPU", 8, 8));
        plb.ProcessPendingUpdatesPeriodicTask();

        // This update can assert if reserved load is miscalculated earlier.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"S/0/V,P/1/BP"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
    }

    BOOST_AUTO_TEST_CASE(AppGroupsDownNodeScaleout)
    {
        wstring testName = L"AppGroupsDownNodeScaleout";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"", false));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"", true));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
             L"App1",
             1,
             1,
             CreateApplicationCapacities(wformatString(L""))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1", false), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithViolation)
    {
        wstring testName = L"AppGroupsPlacementWithViolation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L""));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L""));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"", map<wstring, wstring>(), L""));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            1,
            CreateApplicationCapacities(wformatString(L""))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0|1", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsDownNodeAppCapacity)
    {
        wstring testName = L"AppGroupsDownNodeAppCapacity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"", false));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"", true));


        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            2,
            2,
            CreateApplicationCapacities(wformatString(L"CPU/10000/500/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/300/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/300/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservationSBAndShouldDisappear)
    {
        wstring testName = L"AppGroupsReservationSBAndShouldDisappear";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"CPU/1000"));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"CPU/1000"));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"", map<wstring, wstring>(), L"CPU/300"));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"", map<wstring, wstring>(), L"CPU/300"));
        plb.UpdateNode(CreateNodeDescription(4, L"", L"", map<wstring, wstring>(), L"CPU/1000"));


        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"CPU/10000/100000/400"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,SB/2,S/3/V"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we cannot fix violation on node 2 because we would need to move both replicas in order to remove reservation
        //but SB replicas are not moveable
        //on node 3 we can because should disappear replicas do not carry reservation
        
        bool justFixingViolation = (CountIf(actionList, ActionMatch(L"0 move secondary 3=>1|4", value)) == 1);
        bool fixingAndBalancing = (CountIf(actionList, ActionMatch(L"0 move secondary 3=>0", value)) == 1 && CountIf(actionList, ActionMatch(L"0 move primary 0=>1|4", value)) == 1);
        bool isOk = justFixingViolation || fixingAndBalancing;
        VERIFY_IS_TRUE(isOk);
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservationNewReplicaViolation)
    {
        wstring testName = L"AppGroupsReservationNewReplicaViolation";
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"CPU/1000"));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"CPU/1000"));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"", map<wstring, wstring>(), L"CPU/500"));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"", map<wstring, wstring>(), L"CPU/1000"));


        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"CPU/10000/100000/600"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/3,S/2/B"), 1));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(5u, CountIf(actionList, ActionMatch(L"0|1|2 add secondary 0|1|3", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 1, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 1, CreateReplicas(L"P/3,S/2/B,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        //violation cannot be fixed because IB replica cannot be moved but it still carries reservation
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }
    BOOST_AUTO_TEST_CASE(ReallyBigPartitionLoadTest)
    {
        wstring testName = L"ReallyBigPartitionLoadTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric1/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Small load
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"SurprizeMetric", 10, 10));

        // Big load
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"SurprizeMetric", INT_MAX / 4, INT_MAX / 4));

        // Bigger load
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"SurprizeMetric", INT_MAX / 2, INT_MAX / 2));

        // Really big load
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService", L"SurprizeMetric", INT_MAX, INT_MAX));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService", L"SurprizeMetric", INT_MAX, INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();
        // No testing here - if PLB does not assert in update load calls above we're good!
    }

    BOOST_AUTO_TEST_CASE(ReservedLoadBasicTest)
    {
        wstring testName = L"ReservedLoadBasicTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/100/50/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"SurprizeMetric/1.0/10/10,StrangeMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 0, 0);

        // Add one metric to capacity and check reservation.
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/100/50/10")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 10, 10);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 0, 0);

        // Add different metric to capacity and remove the first one
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/10")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 10, 10);

        // Now add both metrics with reservation 0 in metric list
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/0,SurprizeMetric/100/50/0")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 0, 0);

        // Increase reservation to be smaller than total load and check
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/5,SurprizeMetric/100/50/3")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 3, 3);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 5, 5);

        // Increase both to be greater than total load and check
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/15,SurprizeMetric/100/50/13")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 13, 10);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 15, 10);

        // Remove minimum nodes and check that reservation is zero
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/15,SurprizeMetric/100/50/13")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 0, 0);

        // Again: Increase both to be greater than total load and check
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"StrangeMetric/100/50/15,SurprizeMetric/100/50/13")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 13, 10);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 15, 10);

        // Minimum nodes remain > 0, but metrics are removed
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);
        VerifyReservedCapacityAndLoad(L"StrangeMetric", 0, 0);
    }

    BOOST_AUTO_TEST_CASE(ReservedLoadAboveAndBelow)
    {
        // Create AppGroup with reservation, and to add FTs with enough load to reach the reservation
        // Then, to change one FT so that it (internally) we remove it and re-add it again.
        // Next, add one FT to go over the reservation.
        // Finally remove two of them to go under the reservation.
        // Purpose: stress internal structures in ServiceDomain to make sure that load sums do not go under zero and we do not assert.
        wstring testName = L"ReservedLoadAboveAndBelow";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/100/50/30")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"SurprizeMetric/1.0/10/10")));

        // Application load: 20, reserved load used: 20
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 20);

        // Application load: 40, reserved load used: 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 40);

        // Application load: 60, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 60);

        // Application load: 60, reserved load used: 60
        // Internally, we will reduce application load to 40 and increase reserved load to 20 as an intermediate step
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 1, CreateReplicas(L"S/0,P/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 60);

        // Application load: 80, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 60);

        // Delete FT: Application load: 60, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 60);

        // Delete FT: Application load: 40, reserved load used: 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 60, 40);
    }

    BOOST_AUTO_TEST_CASE(ReservedLoadWithApplicationUpdate)
    {
        wstring testName = L"ReservedLoadWithApplicationUpdate";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes -- no reservation!
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"SurprizeMetric/1.0/10/10")));

        // Application load: 20, reserved load used: 20
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);

        // Application load: 40, reserved load used: 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);

        // Application load: 60, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VerifyReservedCapacityAndLoad(L"SurprizeMetric", 0, 0);

        // Reserved load goes to 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/100/50/30")));

        // Application load: 60, reserved load used: 60
        // Internally, we will reduce application load to 40 and increase reserved load to 20 as an intermediate step
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 1, CreateReplicas(L"S/0,P/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Application load: 80, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Delete FT: Application load: 60, reserved load used: 60
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService")));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Delete FT: Application load: 40, reserved load used: 40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService")));
        plb.ProcessPendingUpdatesPeriodicTask();

        // No testing here - if PLB does not assert in Update FT calls above we're good!
    }

    BOOST_AUTO_TEST_CASE(ReservedLoadWithDefragmentation)
    {
        wstring testName = L"ReservedLoadWithDefragmentation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        wstring metricName = L"AppGroups_Defrag";

        // Defragmentation is on
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(metricName, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        for (int nodeNum = 0; nodeNum < 9; nodeNum++)
        {
            plb.UpdateNode(CreateNodeDescription(nodeNum));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int numFU = 0; numFU < 1; numFU++)
        {
            wstring serviceName = wformatString("{0}_Service_{1}", applicationName, numFU);
            plb.UpdateService(CreateServiceDescriptionWithApplication(
                wstring(serviceName),
                L"TestType",
                wstring(applicationName),
                true,
                CreateMetrics(wformatString("{0}/1.0/4/4", metricName))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(numFU), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyReservedCapacityAndLoad(metricName, 0, 0);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            6,  // MinimumNodes
            9,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/2700/300/200", metricName))));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyReservedCapacityAndLoad(metricName, 1200, 12);
    }

    //
    // Testing of application updates
    //
    BOOST_AUTO_TEST_CASE(ApplicationUpdateWithReservation)
    {
        wstring testName = L"ApplicationUpdateWithReservation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        wstring metricName = L"MyMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Total cluster capacity is 200
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metricName)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metricName)));

        plb.ProcessPendingUpdatesPeriodicTask();
        ErrorCode error;

        wstring applicationName = wformatString("{0}_BadApplication", testName);
        // Total capacity = 600, Max node capacity = 300, Reservation capacity = 200
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/600/300/200", metricName))));
        // This creation should not pass
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 0, -1);

        applicationName = wformatString("{0}_GoodApplication", testName);
        // No reservation because MinimumNodes == 0
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/600/300/200", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 0, -1);

        // No reservation because there are no capacities
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 0, -1);

        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 50
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 100, -1);

        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 70 (INCREASE)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/70", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 140, -1);

        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 100 (AT LIMIT)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/100", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 200, -1);

        wstring anotherApplicationName = wformatString("{0}_AnotherApplication", testName);
        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 50 (no space)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            anotherApplicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/70", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 200, -1);

        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 50 (Reduced)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 100, -1);

        // Total capacity == 200, MaxNodeCapacity == 100, NodeReservation == 50 (AT LIMIT)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            anotherApplicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 200, -1);
    }


    BOOST_AUTO_TEST_CASE(ApplicationUpdateWithOutsideService)
    {
        wstring testName = L"ApplicationUpdateWithOutsideService";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        wstring metricName = L"MyMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Total cluster capacity is 200
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metricName)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metricName)));

        plb.ProcessPendingUpdatesPeriodicTask();

        // One outside service and a FT that will use 100 load total
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));
        wstring serviceName = wformatString("{0}_OutsideService", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName),
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(wformatString("{0}/1.0/50/50", metricName))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        ErrorCode error;

        wstring applicationName = wformatString("{0}_BadApplication", testName);
        // Total capacity = 200, Max node capacity = 100, Reservation capacity = 160
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/80", metricName))));
        // This creation should not pass
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 0, 0);

        // Application with 100 reserved load should pass
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/50", metricName))));
        // This creation should not pass
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 100, 0);

        // Try to increase reservation to 120 and expect failure
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/60", metricName))));
        // This creation should not pass
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 100, 0);

        // Decrease reservation to 20
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/10", metricName))));
        // This creation should not pass
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 20, 0);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAppGroupReservation)
    {
        wstring testName = L"PlacementWithAppGroupReservation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int nodeId = 0; nodeId < 3; ++nodeId)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(nodeId, L"M1/100,M2/100,M3/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            10,  // MaximumNodes
            CreateApplicationCapacities(L"M1/1000/80/80,M2/1000/80/80")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            0,  // MinimumNodes
            10,  // MaximumNodes
            CreateApplicationCapacities(L"M1/1000/80/80,M2/1000/80/80")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App3",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"")));


        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App4",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"M3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"M3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", L"App3", true, CreateMetrics(L"M1/1.0/10/10,M3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestType", L"App4", true, CreateMetrics(L"M2/1.0/10/10,M3/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L""), 1));

        // There is no place to add new replicas because of reservation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckScaleoutCountZero)
    {
        wstring testName = L"ConstraintCheckScaleoutCountZero";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/50/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckStandBy)
    {
        wstring testName = L"ConstraintCheckStandBy";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/50/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,SB/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckStandBy1)
    {
        wstring testName = L"ConstraintCheckStandByAndPrimaryOnTheSameNode";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/50/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType",
            L"App1",
            true,
            CreateMetrics(L"CPU/1/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            L"App1",
            true,
            CreateMetrics(L"CPU/1/30/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(L"TestService1"),
            0, CreateReplicas(L"P/0,SB/1,SB/2"),
            0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(L"TestService2"),
            0,
            CreateReplicas(L"P/0,SB/1,SB/2"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1|2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutViolationWithStandByTest)
    {
        wstring testName = L"ScaleoutViolationWithStandByTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));


        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);
        wstring otherServiceName = wformatString("{0}_OtherService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            1,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/100/100/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/100/100")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"P/0, SB/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithAppGroupReservation)
    {
        wstring testName = L"ConstraintCheckWithAppGroupReservation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int nodeId = 0; nodeId < 2; ++nodeId)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(nodeId, L"M1/100,M3/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"M1/1000/80/80")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"M3/1/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"M1/1/15/15,M3/1/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", L"App2", true, CreateMetrics(L"M1/1/15/15,M3/1/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeNegativeLoadDiff)
    {
        wstring testName = L"AppGroupsUpgradeNegativeLoadDiff";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/200/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/50/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/100/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementClosureTest1)
    {
        //this test is testing that we respect scaleout and app capacity even if not all of the partitions are in the closure during placement
        wstring testName = L"AppGroupsPlacementClosureTest1";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/500"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            3,
            CreateApplicationCapacities(wformatString(L"CPU/1000/300/0"))));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            1,
            3,
            CreateApplicationCapacities(wformatString(L"CPU/1000/500/350"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/300/150")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/150")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", L"App2", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        //node 3 is not good due to scaleout
        //node 2 is not good due to node app capacity
        //node 1 is not good due to node capacity because of reservation
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementClosureTest2)
    {
        //this test is testing whether we respect max app capacity with more than 1 metric and scaleout
        wstring testName = L"AppGroupsPlacementClosureTest2";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/1000,Memory/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/1000,Memory/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/1000,Memory/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/600,Memory/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"CPU/1000,Memory/1000"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/100000/0,Memory/1100/1000000/0"))));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            0,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/10000/1000/100,Memory/10000/1000/50"))));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App3",
            0,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/10000/1000/50,Memory/10000/1000/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/100/100,Memory/1/100/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"CPU/1/0/0,Memory/1/500/0")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", L"App3", true, CreateMetrics(L"CPU/1/350/350,Memory/1/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/4,S/1,S/2,S/3,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L""), 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L""), 4));

        fm_->RefreshPLB(Stopwatch::Now());

        //we can only place 5 replicas due to the limit on max CPU usage
        //node 5 is bad due to scaleout, node 4 and 3 due to node capacity
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(5u, CountIf(actionList, ActionMatch(L"4|5 add * 0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPartialConstraintCheck1)
    {
        //this test is verifying that partial constraint check closure is correctly expanded with all partitions of an app when needed
        wstring testName = L"AppGroupsPartialConstraintCheck1";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            1,
            CreateApplicationCapacities(wformatString(L""))));
        set<NodeId> blockList1;
        set<NodeId> blockList2;
        blockList1.insert(CreateNodeId(0));
        blockList1.insert(CreateNodeId(1));
        blockList2.insert(CreateNodeId(0));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            5,
            5,
            CreateApplicationCapacities(wformatString(L""))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>(blockList1)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>(blockList2)));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType1", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType3", L"", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType3", true, L"TestService2", CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestType3", L"App2", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestType1", L"App2", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService5"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"ConstraintCheck");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we cannot fix affinity
        //we need this to enforce a partial constraint check closure
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService6", L"TestType2", L"App1", true, CreateMetrics(L"")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService6"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            1,
            1,
            CreateApplicationCapacities(wformatString(L""))));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        VerifyPLBAction(plb, L"ConstraintCheck");
        actionList = GetActionListString(fm_->MoveActions);
        //we can only fix scaleout of app1 by moving the old FT, we need to be sure that both are in the closure
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        //the app2 is now in violation due to scaleout, we can only fix by moving service5
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPartialConstraintCheck2)
    {
        //this test is verifying that we properly expand the closure and detect node capacity violations when we have reservation
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        wstring testName = L"AppGroupsPartialConstraintCheck2";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/4000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"CPU/4000"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/100000/100000/400"))));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            5,
            5,
            CreateApplicationCapacities(wformatString(L""))));
        set<NodeId> blockList1;
        set<NodeId> blockList2;
        blockList1.insert(CreateNodeId(1));
        blockList1.insert(CreateNodeId(2));
        blockList1.insert(CreateNodeId(3));
        blockList1.insert(CreateNodeId(4));
        blockList2.insert(CreateNodeId(0));
        blockList2.insert(CreateNodeId(2));
        blockList2.insert(CreateNodeId(3));
        blockList2.insert(CreateNodeId(4));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>(blockList1)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>(blockList2)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType1", L"", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType2", true, L"TestService1", CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType3", L"", true, CreateMetrics(L"CPU/1.0/100/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestType3", L"App1", true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestType3", L"App2", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService5"), 0, CreateReplicas(L"P/1,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"ConstraintCheck");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we cannot fix affinity
        VERIFY_ARE_EQUAL(0u, actionList.size());

        map<NodeId, uint> newLoads;
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService3", L"CPU", 200, newLoads));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            5,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/100000/100000/600"))));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        VerifyPLBAction(plb, L"ConstraintCheck");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        //we are in violation on node 0 because load is 200 + 400
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2|3 move primary 0=>*", value)));
        //on node 1 and 2 we are in violation due to reservation
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"4 move * 1|2=>3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeNodeCapacity)
    {
        wstring testName = L"AppGroupsNodeCapacity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/500,Memory/500"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/200/0,Disc/1000/200/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/100/100,Memory/1/200/200")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/3,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0/I,S/1,S/2"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we can only swap to node 1 since swapping to node 2 would cause application capacity constraint violation
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeDifferentApps)
    {
        wstring testName = L"AppGroupsUpgradeDifferentApps";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/500,Memory/500"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/200/0"))));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/200/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App2", true, CreateMetrics(L"CPU/1/150/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/2/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //App2 should not prevent App1 upgrade due to node capacity and vice verca
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 swap primary 2<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeShouldDisappear)
    {
        wstring testName = L"AppGroupsUpgradeShouldDisappear";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500,Memory/500"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/500,Memory/500"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/450/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/2,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 1, CreateReplicas(L"P/2,S/1/D"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //should disappear replicas should also be considered therefore we can promote only a single replica
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsAffinityBasicUpgradeTest)
    {
        wstring testName = L"AppGroupsAffinityBasicUpgradeTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);
        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/1000,Memory/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/1000,Memory/1000"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            10,
            CreateApplicationCapacities(wformatString(L"Memory/1000/400/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"Memory/1/200/150")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService2", L"TestType", true, L"App1", L"TestService1", CreateMetrics(L"Memory/1/250/200")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we cannot promote both to node 1 due to capacity violation
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsAffinityBothInUpgrade)
    {
        wstring testName = L"AppGroupsAffinityBothInUpgrade";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);
        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Memory/2000"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            10,
            CreateApplicationCapacities(wformatString(L"Memory/1000/200/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"Memory/1/150/50")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService2", L"TestType", true, L"App1", L"TestService1", CreateMetrics(L"Memory/1/100/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/2/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we can promote only a child or parent here
        //affinity check is best effort
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsAffinityAdvancedUpgradeTest)
    {
        wstring testName = L"AppGroupsAffinityAdvancedUpgradeTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);
        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"CPU/2000,Memory/2000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(6, L"CPU/2000,Memory/2000"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            10,
            CreateApplicationCapacities(wformatString(L"CPU/1000/400/0"))));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            1,
            10,
            CreateApplicationCapacities(wformatString(L"CPU/1000/350/0,Memory/1000/600/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/200/100,Memory/1/200/200")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService2", L"TestType", true, L"App1", L"TestService1", CreateMetrics(L"CPU/1/150/100,Memory/1/200/200")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService3", L"TestType", true, L"App2", L"TestService1", CreateMetrics(L"CPU/1/150/100,Memory/1/250/200")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/100/100,Memory/1/200/200")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestType", L"App2", true, CreateMetrics(L"CPU/1/250/250")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService6", L"TestType", L"App2", true, CreateMetrics(L"CPU/1/100/100,Memory/1/200/200")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/6,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService5"), 0, CreateReplicas(L"P/5,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService6"), 0, CreateReplicas(L"P/5,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService6"), 0, CreateReplicas(L"P/6,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/I,S/1,S/2,S/3,S/4"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we can only swap to node 1
        //app2 is violating node2 for memory, app2 is violating node3 for cpu and app1 is violating node4 for cpu when promoting both parent and child
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsMetricConnectionsTest)
    {
        //this test has no verify(), it should simply avoid causing an assert...
        wstring testName = L"AppGroupsMetricConnectionsTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        wstring metricName = L"MyMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Just so that we have non-zero nodes...
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metricName)));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        //Update Min nodes only
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/8/16/64,{1}/1024/4096/16384", L"CPU", L"Memory"))));

        //Update Max Nodes
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/8/16/64,{1}/1024/4096/16384", L"CPU", L"Memory"))));

        //Update Capacities
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/1024/4096/16384", L"Memory"))));

        //Clear Capacities
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        //change all
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            4,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/200/100/50", metricName))));

        //remove all
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        //Delete Application
        plb.DeleteApplication(applicationName);
    }

    BOOST_AUTO_TEST_CASE(ScaleoutConstraintReplicaStateNoneDropped)
    {
        wstring testName = L"ScaleoutConstraintReplicaStateNoneDropped";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, N/2, D/3"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        //there is one None and one Dropped replica so it should not violate scaleout constraint
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithReservation)
    {
        // Scenario: replica can't be placed because reservation that it carries can't fit anywhere
        // Node 0: Capacity of 100, no load on it
        // Node 1: Capacity of 100, load of 10 from external service
        // Replica that needs to be placed has load of 10, and application has reservation of 100 - so only node 0
        wstring testName = L"AppGroupsPlacementWithReservation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring outsideServiceName = wformatString("{0}_OutsideService", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/100")));

        // Add one application that will have metric, but no services - just to make sure that noting asserts.
        // Potential problem is when calculating global metric indices for this application because no service has them.
        wstring irrelevantApplicationName = wformatString("{0}_IrrelevantApplication", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            irrelevantApplicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"IrrelevantCPU/1000/500/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            outsideServiceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(outsideServiceName), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Node 1 can't accept replica because it does not have capacity to accept reservation of 100
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithReservationMultipleReplicasCantPlaceTest)
    {
        // Scenario: reservation is equal to node capacity, some replicas can fit, one cant
        wstring testName = L"AppGroupsPlacementWithReservationMultipleReplicasCantPlaceTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Node 1 can't accept replica because it does not have capacity to accept reservation of 100
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithReservationAndMetricWithoutService)
    {
        // Scenario: Application has capacity and reservation for metric CPU, but its only service reports only metric Memory.
        wstring testName = L"AppGroupsPlacementWithReservationAndMetricWithoutService";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/150")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"Memory/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Node 1 can't accept replica because it does not have capacity to accept reservation of 100
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithReservationCantPlace)
    {
        // Scenario: 1 app with 1 service and 1 replica. Replica load is 10, but reservation is 100.
        // Node capacity is 90, so we can't place the replica.
        wstring testName = L"AppGroupsPlacementWithReservationCantPlace";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/90"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            serviceName,
            L"TestType",
            applicationName,
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementMultipleAppsWithReservationCantPlace)
    {
        // Scenario: 2 applications, each with same reservation that is not used.
        // Node capacity is enough to fit both applications with 1 service each and reservation
        // We can't place new service in either aplication because it would have to increase load so that it can't be placed.
        wstring testName = L"AppGroupsPlacementMultipleAppsWithReservationCantPlace";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring application1Name = wformatString("{0}_Application1", testName);
        wstring service11Name = wformatString("{0}_Service11", testName);
        wstring service12Name = wformatString("{0}_Service12", testName);
        wstring application2Name = wformatString("{0}_Application2", testName);
        wstring service21Name = wformatString("{0}_Service21", testName);
        wstring service22Name = wformatString("{0}_Service22", testName);


        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application1Name,
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/100/100/50")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application2Name,
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/100/100/50")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            service11Name,
            L"TestType",
            application1Name,
            true,
            CreateMetrics(L"CPU/1.0/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            service12Name,
            L"TestType",
            application1Name,
            true,
            CreateMetrics(L"CPU/1.0/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            service21Name,
            L"TestType",
            application2Name,
            true,
            CreateMetrics(L"CPU/1.0/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            service22Name,
            L"TestType",
            application2Name,
            true,
            CreateMetrics(L"CPU/1.0/30/30")));

        // Application 1 reserves 50, uses 30 
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service11Name), 0, CreateReplicas(L"P/0"), 0));
        // Application 1 reserves 50, uses 30.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service21Name), 0, CreateReplicas(L"P/0"), 0));

        // Two services with new replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service12Name), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service22Name), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithReservationMultipleReplicasTest)
    {
        // Scenario: Placing a new replica on nodes that have reserved load, and new replica fits into it.
        // Both nodes have capacity of 100 for metric CPU, application has reservation of 100 for metric CPU.
        // FT 0 has one replica on node 0 (load 10, reserving 100 load), and needs one extra replica (load 10).
        // FT 1 does not have replicas and needs two extra replicas (both with load 100).
        // After placing everything we should have reservation of 100 and load of 20 on both nodes.
        wstring testName = L"AppGroupsPlacementWithReservationMultipleReplicasTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"P/0"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(applicationServiceName), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1 add * 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithPartialReservationTest)
    {
        // Scenario: Placing a new replica on nodes that have reserved load, and new replica will use part of reservation.
        // Both nodes have capacity of 100 for metric CPU, application has reservation of 90 for metric CPU.
        // FT 0 has one replica on node 0 (load 5, reserving 90 load), and one replica on node 1 (load 60, reserving 90).
        // FT 1 has default load of 95 and needs 2 replicas: we can place only on node 0
        wstring testName = L"AppGroupsPlacementWithPartialReservationTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/90")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/95/95")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(applicationServiceName), 0, CreateReplicas(L""), 2));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, applicationServiceName, L"CPU", 5, 60));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPlacementWithStandByAndReservationTest)
    {
        wstring testName = L"AppGroupsPlacementWithStandByAndReservationTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);
        wstring otherServiceName = wformatString("{0}_OtherService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            1,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/100/100/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/50/50")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"SB/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(otherServiceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AppGroupsCapacityViolationWithStandByAndReservationTest)
    {
        // Scenario: application is on node 0, replicas report no load, but reservation violates.
        // There is a standby replica on the node that can't be moved, so reservation will remain on node 0.
        // Expectation: we should not move anything from this application.
        wstring testName = L"AppGroupsCapacityViolationWithStandByAndReservationTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        // Only one attempt should be enough to resolve the only violation that we have because there are 2 nodes.
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/500"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);
        wstring outsideServiceName = wformatString("{0}_OutsideService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/200/100/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/0/0")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            outsideServiceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/120/120")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"SB/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(applicationServiceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(outsideServiceName), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>1", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsDropWithReservationTest)
    {
        // Scenario: we need to drop a replica of a service when application has reservation.
        // Checks that structures will be updated correctly when one of nodes in the movement is nullptr.
        wstring testName = L"AppGroupsDropWithReservationTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);


        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/300/100/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"P/0,S/1,S/2"), -1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 drop secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeSwapWithReservationTest)
    {
        // Scenario: Swapping a primary with higher load to a secondary with lower load when reservation is equal to node capacity.
        // FT 0 has a primary on node 0 (load 100, reservation 100), and one secondary on node 1 (load 50, reservation 100).
        // FT 1 has default load of 95 and needs 2 replicas: we can place only on node 0
        wstring testName = L"AppGroupsUpgradeSwapWithReservationTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationServiceName = wformatString("{0}_ApplicationService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/200/100/100")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            applicationServiceName,
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/100/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(applicationServiceName), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservedLoadMoveBasicTest)
    {
        // Scenario: 2 applications, both with reservation of 100, with one replica each placed on node 0.
        // Node 0 has capacity, but replicas are in affinity violation so they need to be moved to node 1.
        // Node 1 has the parent replica, but does not have capacity to accept both reservations.
        wstring testName = L"AppGroupsReservedLoadMoveBasicTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring application1Name = wformatString("{0}_Application1", testName);
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring application2Name = wformatString("{0}_Application2", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);
        wstring parentServiceName = wformatString("{0}_ParentService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application1Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/60")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application2Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/1000/500/60")));

        // Add one application that will have metric, but no services - just to make sure that noting asserts.
        // Potential problem is when calculating global metric indices for this application because no service has them.
        wstring irrelevantApplicationName = wformatString("{0}_IrrelevantApplication", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            irrelevantApplicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"IrrelevantCPU/1000/500/100")));

        plb.UpdateService(CreateServiceDescription(parentServiceName, L"TestType", true));

        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"TestType"),
            wstring(application1Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            true,
            move(CreateMetrics(L"CPU/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"TestType"),
            wstring(application2Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            true,
            move(CreateMetrics(L"CPU/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentServiceName), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));

        // Add one more node to force full closure. Irrelevant for this test because it has no capacity.
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/0"));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        size_t movesApp1 = CountIf(actionList, ActionMatch(L"1|2 move primary 0=>1", value));
        size_t movesApp2 = CountIf(actionList, ActionMatch(L"3|4 move primary 0=>1", value));
        VERIFY_IS_TRUE(movesApp1 == 2 || movesApp2 == 2);
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservedLoadMoveMultiMetricTest)
    {
        // Scenario: 3 applications, but different metrics:
        //      Application1:  Metric1 - Reservation 100, Metric2 - Reservation 100
        //      Application11: Metric1 - Reservation 100, Metric2 - Reservation 100
        //      Application2:  Metric2 - Reservation 100, Metric3 - Reservation 100
        //      Applicaiton3:  Metric1 - Reservation 100, Metric3 - Reservation 100
        // Each application will have one primary on node 0, but affinity will be broken - parent partition will be on nodes 1 and 2.
        // Nodes 1 and 2 have enough capacity to accept just 1 replica:
        //      Node 1 can accept the primary of Application1 or of Application11, but not both
        //      Node 2 can accept the primary of Application2
        wstring testName = L"AppGroupsReservedLoadMoveMultiMetricTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/200,Metric2/200,Metric3/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100,Metric2/100,Metric3/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/10,Metric2/100,Metric3/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring application1Name = wformatString("{0}_Application1", testName);
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring application11Name = wformatString("{0}_Application11", testName);
        wstring service11Name = wformatString("{0}_Service11", testName);
        wstring application2Name = wformatString("{0}_Application2", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);
        wstring application3Name = wformatString("{0}_Application3", testName);
        wstring service3Name = wformatString("{0}_Service3", testName);
        wstring parentServiceName = wformatString("{0}_ParentService", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application1Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/200/100/100,Metric2/200/100/100")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application11Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/200/100/100,Metric2/200/100/100")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application2Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric2/200/100/100,Metric3/200/100/100")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            application3Name,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/200/100/100,Metric3/200/100/100")));

        plb.UpdateService(CreateServiceDescription(parentServiceName, L"TestType", true));

        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"TestType"),
            wstring(application1Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            false,
            move(CreateMetrics(L"Metric1/1.0/10/10,Metric2/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service11Name),
            wstring(L"TestType"),
            wstring(application11Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            false,
            move(CreateMetrics(L"Metric1/1.0/10/10,Metric2/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"TestType"),
            wstring(application2Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            false,
            move(CreateMetrics(L"Metric2/1.0/10/10,Metric3/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"TestType"),
            wstring(application3Name),
            true,
            wstring(L""),
            wstring(parentServiceName),
            false,
            move(CreateMetrics(L"Metric1/1.0/10/10,Metric3/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentServiceName), 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service3Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(service11Name), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1|4 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>2", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservedLoadMoveMultiMetricEdgeCaseTest)
    {
        // Scenario: 3 applications, but different metrics. Edge case when each service has different metric:
        // Single application with reservation of 100 for 3 metrics.
        // 3 services, each with different metric
        // Each service will have one primary on node 0, but placement constraints will be broken - enough capacity is on node 0.
        // No moves can be generated:
        //   Service1 will report only Metric1: it can fit to Node1 (together with reservation), but it brings reservation for other metrics that can't fit.
        //   Service2 will report only Metric2: it can fit to Node2 (together with reservation), but it brings reservation for other metrics that can't fit.
        //   Service3 will report only Metric2: it can fit to Node3 (together with reservation), but it brings reservation for other metrics that can't fit.
        wstring testName = L"AppGroupsReservedLoadMoveMultiMetricEdgeCaseTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        if (PLBConfig::GetConfig().IsTestMode)
        {
            // This test can create intermediate violations due to bug in node capacity logic with reservation.
            // Skip it in test mode (without test mode additional violations should not happen).
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == true).", testName);
            return;
        }

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::map<wstring, wstring> node1Properties;
        node1Properties.insert(make_pair(L"Property", L"NotOK"));
        std::map<wstring, wstring> node2Properties;
        node2Properties.insert(make_pair(L"Property", L"OK"));
        std::map<wstring, wstring> node3Properties;
        node3Properties.insert(make_pair(L"Property", L"OK"));
        std::map<wstring, wstring> node4Properties;
        node4Properties.insert(make_pair(L"Property", L"OK"));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(0, L"Metric1/100,Metric2/100,Metric3/100", move(node1Properties)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(1, L"Metric1/100,Metric2/10,Metric3/10", move(node2Properties)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(2, L"Metric1/10,Metric2/100,Metric3/10", move(node3Properties)));
        // TODO: if we duplicate 2 instead of creating node 3 - by mistake - we will end up with a move to node 1 of P0
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(3, L"Metric1/10,Metric2/10,Metric3/100", move(node4Properties)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring applicationName = wformatString("{0}_Application1", testName);
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);
        wstring service3Name = wformatString("{0}_Service3", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/200/100/100,Metric2/200/100/100,Metric3/200/100/100")));

        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"TestType"),
            wstring(applicationName),
            true,
            wstring(L"Property==OK"),
            wstring(L""),
            true,
            move(CreateMetrics(L"Metric1/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"TestType"),
            wstring(applicationName),
            true,
            wstring(L"Property==OK"),
            wstring(L""),
            true,
            move(CreateMetrics(L"Metric2/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"TestType"),
            wstring(applicationName),
            true,
            wstring(L"Property==OK"),
            wstring(L""),
            true,
            move(CreateMetrics(L"Metric3/1.0/10/10")),
            FABRIC_MOVE_COST_LOW,
            false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservedLoadNodeCapacityTest)
    {
        // Scenario: Application reports one metric with reservation.
        //           One multi-partitioned service reports no metrics at all one node 0.
        //           One multi-partitioned service reports load smaller than reservation on node 1.
        //           Reservation is enough to violate capacity on nodes 0 and 1.
        wstring testName = L"AppGroupsReservedLoadNodeCapacityTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring applicationName = wformatString("{0}_Application1", testName);
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/300/150/150")));

        // This service will report default metrics
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(service1Name),
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"")));

        // This service will report Metric1, but well below reservation
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(service2Name),
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"Metric1/1.0/10/10")));

        // On node 0, load for Metric1 is 0, but reservation is 150.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        // One node 1, load for Metric1 is 20, but reservation is 150.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(4u, CountIf(actionList, ActionMatch(L"* move primary 0|1=>2", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AppGroupsZeroLoadReplicaPlacement)
    {
        wstring testName = L"AppGroupsZeroLoadReplicaPlacement";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/10,M2/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/10,M2/10"));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = L"TestApplication";
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"M1/8/4/0,M2/10/5/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"ServiceWithMetricM1",
            L"ServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"M1/1.0/5/5")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"ServiceWithZeroMetricM1",
            L"ServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"M1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"ServiceWithMetricM2",
            L"ServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"M2/1.0/5/5")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ServiceWithMetricM1"), 0, CreateReplicas(L"P/0,S/1"), 0));

        // Insert new replicas of service which reports zero load for the overcommited metric
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"ServiceWithZeroMetricM1"), 0, CreateReplicas(L""), 2));
        // Insert new replicas within of service which doesn't report the overcommited metric load
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"ServiceWithMetricM2"), 0, CreateReplicas(L""), 2));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsPromoteSBPlacementCheck)
    {
        wstring testName = L"AppGroupsPromoteSBPlacementCheck";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Primary replica location
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100"));
        // Candidate node
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100"));
        // Node which doesn't have enough M1 metric capacity
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/50,M2/100"));
        // Node which doesn't have enough M2 metric capacity
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/100,M2/50"));
        // Block-listed node
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"M1/100,M2/100"));
        // Location of SB replica which doesn't have enough M1 metric load to be promoted
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"M1/100,M2/100"));
        // Location of SB replica which doesn't have enough M2 metric load to be promoted
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(6, L"M1/100,M2/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp",
            1,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L"M1/530/100/0,M2/530/100/0"))); //Full capacity on both M1 and M2

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(4));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/100/100,M2/1/100/100")));

        // Create new secondary replica and place it on nodeId=5, as SB replica on nodeId=2 doesn't have big enough load on metric M1
        // Default load for the new secondary is: M1 = 71, M2 = 71
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,SB/1,SB/2,SB/3,SB/4,SB/5,SB/6"), 1));

        map<NodeId, uint> newLoads;
        // Update load for metric M1
        newLoads.insert(make_pair(CreateNodeId(1), 90)); // M1: nodeId=1, capacity = 100, SB_load = 90
        newLoads.insert(make_pair(CreateNodeId(2), 50));
        newLoads.insert(make_pair(CreateNodeId(3), 100));
        newLoads.insert(make_pair(CreateNodeId(4), 100));
        newLoads.insert(make_pair(CreateNodeId(5), 0)); // M1: nodeId=5, capacity = 100, SB_load = 0
        newLoads.insert(make_pair(CreateNodeId(6), 90)); // M1: nodeId=6, capacity = 100, SB_load = 90
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"M1", 100, newLoads));

        //Update load for metric M2
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(1), 90)); // M2: nodeId=1, capacity = 100, SB_load = 90
        newLoads.insert(make_pair(CreateNodeId(2), 100));
        newLoads.insert(make_pair(CreateNodeId(3), 50));
        newLoads.insert(make_pair(CreateNodeId(4), 100));
        newLoads.insert(make_pair(CreateNodeId(5), 90)); // M2: nodeId=5, capacity = 100, SB_load = 90
        newLoads.insert(make_pair(CreateNodeId(6), 0));  // M2: nodeId=6, capacity = 100, SB_load = 0
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"M2", 100, newLoads));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateAppLoadOnAddAndDropMoves)
    {
        wstring testName = L"AppGroupsUpdateAppLoadOnAddAndDropMoves";
        Trace.WriteInfo("AppGroupsUpdateAppLoadOnAddAndDropMoves", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        for (int nodeId = 0; nodeId < 4; ++nodeId)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(nodeId, L"M1/200"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp",
            1, // MinimumNodes
            4, // MaximumNodes
            CreateApplicationCapacities(L"M1/340/200/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/100/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"S/0,SB/1,P/2,SB/3"), 2));

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(0), 100));
        newLoads.insert(make_pair(CreateNodeId(1), 20));
        newLoads.insert(make_pair(CreateNodeId(3), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService2", L"M1", 100, newLoads));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 drop secondary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateAppLoadInPlacementWithMove)
    {
        wstring testName = L"AppGroupsUpdateAppLoadInPlacementWithMove";
        Trace.WriteInfo("AppGroupsUpdateAppLoadInPlacementWithMove", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.0));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5.0));
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/60"));
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp",
            1, // MinimumNodes
            4, // MaximumNodes
            CreateApplicationCapacities(L"M1/340/100/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/100/100")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,SB/3"), 1));

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(1), 100));
        newLoads.insert(make_pair(CreateNodeId(2), 60));
        newLoads.insert(make_pair(CreateNodeId(3), 60));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"M1", 100, newLoads));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacementWithMove");
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateAppLoadMultipleMetrics)
    {
        wstring testName = L"AppGroupsUpdateAppLoadMultipleMetrics";
        Trace.WriteInfo("AppGroupsUpdateAppLoadMultipleMetrics", "{0}", testName);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/100,M2/100"));
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp",
            1, // MinimumNodes
            4, // MaximumNodes
            CreateApplicationCapacities(L"M1/240/100/0,M2/240/100/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/20/20,M2/1/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M1/1/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M2/1/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestServiceType", L"TestApp", true, CreateMetrics(L"M5/1/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"SB/0,P/1,S/2"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"SB/0,S/1,P/2"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"SB/0,SB/1,SB/2,P/3"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService5"), 0, CreateReplicas(L"P/0,S/1,S/3"), 1));

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(0), 20));
        newLoads.insert(make_pair(CreateNodeId(2), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService2", L"M1", 30, newLoads));
        
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(0), 20));
        newLoads.insert(make_pair(CreateNodeId(1), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService3", L"M2", 30, newLoads));

        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(0), 25));
        newLoads.insert(make_pair(CreateNodeId(1), 0));
        newLoads.insert(make_pair(CreateNodeId(2), 25));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService1", L"M1", 30, newLoads));
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(0), 25));
        newLoads.insert(make_pair(CreateNodeId(1), 25));
        newLoads.insert(make_pair(CreateNodeId(2), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService1", L"M2", 30, newLoads));


        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 drop secondary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5 add secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateAppLoadMultipleApps)
    {
        wstring testName = L"AppGroupsUpdateAppLoadMultipleApps";
        Trace.WriteInfo("AppGroupsUpdateAppLoadMultipleApps", "{0}", testName);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100,M3/100,M4/100,M5/100,M6/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100,M3/100,M4/100,M5/100,M6/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100,M2/100,M3/100,M4/100,M5/100,M6/100"));
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp1",
            1, // MinimumNodes
            3, // MaximumNodes
            CreateApplicationCapacities(L"M1/110/60/0,M2/90/60/0,M3/60/60/0")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp2",
            1, // MinimumNodes
            3, // MaximumNodes
            CreateApplicationCapacities(L"M4/150/60/0,M5/150/60/0,M6/150/60/0")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp3",
            1, // MinimumNodes
            3, // MaximumNodes
            CreateApplicationCapacities(L"M3/20/10/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/10/10,M2/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/20/20,M2/1/20/20,M3/1/20/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1,S/2"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"SB/0,S/1,P/2"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"S/0,S/1,P/2"), -1));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestServiceType", L"TestApp2", true, CreateMetrics(L"M4/1/20/20,M5/1/20/20,M6/1/20/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/1,S/2"), 1));
        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(2), 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService4", L"M4", 10, newLoads));
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(2), 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService4", L"M5", 10, newLoads));
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(2), 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(4, L"TestService4", L"M6", 10, newLoads));
        newLoads.clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService4"), 0, CreateReplicas(L"SB/0,S/1,P/2"), 1));
        newLoads.insert(make_pair(CreateNodeId(0), 10));
        newLoads.insert(make_pair(CreateNodeId(1), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService4", L"M4", 30, newLoads));
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(0), 10));
        newLoads.insert(make_pair(CreateNodeId(1), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService4", L"M5", 30, newLoads));
        newLoads.clear();
        newLoads.insert(make_pair(CreateNodeId(0), 10));
        newLoads.insert(make_pair(CreateNodeId(1), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(5, L"TestService4", L"M6", 30, newLoads));
        newLoads.clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService4"), 0, CreateReplicas(L"S/0,S/1,P/2"), -1));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestServiceType", L"TestApp3", true, CreateMetrics(L"M3/1/10/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService5"), 0, CreateReplicas(L"P/1,S/2"), 1));


        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 drop secondary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"6 drop secondary *", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"7 add secondary *", value)));

    }
    
    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservation)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservation";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/9"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/9"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"MyMetric/300/30/6"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("serviceName1");

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/6/2")));

        plb.UpdateService(CreateServiceDescription(
            wstring(serviceName1),
            wstring(serviceTypeName),
            false,
            CreateMetrics(L"MyMetric/1.0/3/3")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"I/2, I/3"),
            0));

        // Node:                    0         1          2           3  
        // Node Capacity           10        10          9           9
        // App Capacity            30        30          30       30 
        //----------------------------------------------------------
        // service0                   P(6)        S(2)     S(2)       S(2) 
        // service1                    -        -         S(3)       S(3)
        //----------------------------------------------------------
        // App Reservation                    +R(4)     +R(4)       +R(4)

        fm_->RefreshPLB(Stopwatch::Now());

        // score better for Node1
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));

        // same because both have reservation for the same application
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithMultipleApplicationReservation)
    {
        wstring testName = L"PromoteSecondaryWithMultipleApplicationReservation";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/25"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName1 = wformatString("fabric:/{0}_Application1", testName);
        wstring applicationName2 = wformatString("fabric:/{0}_Application2", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"MyMetric/300/30/10"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName2),
            CreateApplicationCapacities(L"MyMetric/300/30/11"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName1);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName2);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName2);
        wstring serviceName3 = wformatString("{0}/serviceName3", applicationName1);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"MyMetric/1.0/7/5")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName2),
            true,
            CreateMetrics(L"MyMetric/1.0/10/7")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName2),
            true,
            CreateMetrics(L"MyMetric/1.0/8/5")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName3),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"MyMetric/1.0/3/2")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/2, I/1, I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"P/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3),
            wstring(serviceName3),
            0,
            CreateReplicas(L"I/1, I/2, I/3"),
            0));


        // Node:                    0         1          2           3  
        // Node Capacity           10        25          25       25
        //-------------------------------------------------------------------
        // App1 Reservation - 10
        // App2 Reservation - 11
        //-------------------------------------------------------------------
        // service0                   P(7)        S(5)     S(5)       S(5)        -app1
        // service1                    -        S(7)     P(10)       S(7)        -app2
        // service2                    -        P(8)     S(5)       S(5)        -app2    
        // service3                    -        S(2)     S(2)       S(2)        -app1
        //-------------------------------------------------------------------
        // App Reservation1           +R(3)    +R(3)     +R(3)       +R(3)
        // App Reservation2           -        -          -            -

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));

        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithMultipleApplicationReservation2)
    {
        wstring testName = L"PromoteSecondaryWithMultipleApplicationReservation2";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/55"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName0 = wformatString("fabric:/{0}_Application1", testName);
        wstring applicationName1 = wformatString("fabric:/{0}_Application2", testName);
        wstring applicationName2 = wformatString("fabric:/{0}_Application3", testName);

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName0),
            CreateApplicationCapacities(L"MyMetric/300/30/15"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"MyMetric/300/30/20"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName2),
            CreateApplicationCapacities(L"MyMetric/300/30/20"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName0);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName1);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName2);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName0),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName2),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"I/1, I/2, I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"S/0, S/2, S/3"),
            0));



        // Node:                    0         1          2           3  
        // Node Capacity           100        40         60           55
        //------------------------------------------------------------------------
        // service0                   P(20)    S(10)     S(10)       S(10)        -app1
        // service1                    -        S(10)     S(10)       S(10)        -app2
        // service2                   S(10)    -        S(10)       S(10)        -app3    
        //------------------------------------------------------------------------
        // App Reservation0           -        +R(5)     +R(5)       +R(5)
        // App Reservation1           -        +R(10)     +R(10)       +R(10)
        // App Reservation2           +R(10)    -        +R(10)       +R(10)

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));

        // score better for Node1
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationServiceNoMetric)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationServiceNoMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/120"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/80"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/130"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName0 = wformatString("fabric:/{0}_Application0", testName);
        wstring applicationName1 = wformatString("fabric:/{0}_Application1", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName0),
            CreateApplicationCapacities(L"MyMetric/300/50/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"MyMetric/300/50/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName0);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName1);
        wstring serviceName2 = wformatString("serviceName2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName0),
            true));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"MyMetric/1.0/40/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName0),
            true,
            CreateMetrics(L"MyMetric/1.0/40/30")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/0, I/1, I/2, I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"P/0, S/1, S/3"),
            0));

        // Node:                    0         1          2           3  
        // Node Capacity           100        120          80       130
        //---------------------------------------------------------------------
        // service0                    P        S           -         S        - app0
        // service1                   P(40)    S(20)     S(20)       S(20)    - app1
        // service2                P(40)    S(30)      -       S(30)    - app0
        //---------------------------------------------------------------------
        // App Reservation0           +R(10)    +R(20)               +R(20)
        // App Reservation1           +R(10)    +R(30)     +R(30)    +R(30)       

        fm_->RefreshPLB(Stopwatch::Now());


        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(-1, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(-1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationSecondaryNoLoad)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationSecondaryNoLoad";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/65"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/70"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"MyMetric/300/50/30"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("/serviceName1");

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/40/0")));

        plb.UpdateService(CreateServiceDescription(
            wstring(serviceName1),
            wstring(serviceTypeName),
            false,
            CreateMetrics(L"MyMetric/1.0/30/30")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"I/1, I/2, I/3"),
            0));

        // Node:                    0         1          2           3  
        // Node Capacity           100        65          60       70
        //----------------------------------------------------------------------
        // service0                   P(40)    S           S        S        - app0
        // service1                   -        S(30)     S(30)       S(30)    - no app
        //----------------------------------------------------------------------
        // App Reservation                    +R(30)     +R(30)    +R(30)

        fm_->RefreshPLB(Stopwatch::Now());


        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationAndApplicationUpdate)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationAndApplicationUpdate";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/17"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/17"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 10;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"MyMetric/400/30/12"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("serviceName0");
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName);
        wstring serviceName2 = wformatString("serviceName2");
        wstring serviceName3 = wformatString("{0}serviceName3", applicationName);

        plb.UpdateService(CreateServiceDescription(
            wstring(serviceName0),
            wstring(serviceTypeName),
            true,
            CreateMetrics(L"MyMetric/1.0/6/5")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            false,
            CreateMetrics(L"MyMetric/1.0/4/4")));

        plb.UpdateService(CreateServiceDescription(
            wstring(serviceName2),
            wstring(serviceTypeName),
            true,
            CreateMetrics(L"MyMetric/1.0/11/4")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName3),
            wstring(serviceTypeName),
            wstring(applicationName),
            false,
            CreateMetrics(L"MyMetric/1.0/7/7")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"I/1, I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"I/1, P/2"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3),
            wstring(serviceName3),
            0,
            CreateReplicas(L"I/1, I/3"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Node:                    0         1          2          3  
        // Node Capacity           10        20         17          17
        // App Capacity            30        30         30          30 
        //---------------------------------------------------------------
        // service0                   P(6)        -         S(5)      S(5) -noApp
        // service1                    -        S(4)     -          S(4) -app
        // service2                    -        S(4)     P(11)      -
        // service3                    -        S(7)     -          S(7) -app
        //---------------------------------------------------------------
        // App Reservation            -        +R(1)     -          +R(1)

        // no app reservation on Node2
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"MyMetric/400/30/11"),
            minimumNodes,
            appScaleoutCount));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Node:                    0         1          2          3  
        // Node Capacity           10        20         17          17
        // App Capacity            30        30         30          30 
        //--------------------------------------------------------
        // service0                   P(6)        -         S(5)      S(5) -noApp
        // service1                    -        S(4)     -          S(4) 
        // service2                    -        S(4)     P(11)      -
        // service3                    -        S(7)     -          S(7)
        //---------------------------------------------------------
        // App Reservation            -        +R(0)     -          +R(0)

        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationServiceNoMetricApplicationUpdate)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationServiceNoMetricApplicationUpdate";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/130"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/90"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/130"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName0 = wformatString("fabric:/{0}_Application0", testName);
        wstring applicationName1 = wformatString("fabric:/{0}_Application1", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName0),
            CreateApplicationCapacities(L"MyMetric/300/50/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"MyMetric/300/50/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName0);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName1);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName0);
        wstring serviceName3 = wformatString("serviceName3");

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName0),
            true));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"MyMetric/1.0/40/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName0),
            true,
            CreateMetrics(L"MyMetric/1.0/40/30")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName3),
            wstring(serviceTypeName),
            wstring(),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/0, I/1, I/2, I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"P/0, S/1, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3),
            wstring(serviceName3),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        // Node:                    0         1          2           3  
        // Node Capacity           100        130          90       130
        //----------------------------------------------------------------------
        // service0                    P        S           -         S        - app0
        // service1                   P(40)    S(20)     S(20)       S(20)    - app1
        // service2                P(40)    S(30)      -       S(30)    - app0
        // service3                P(20)    S(10)     S(10)    S(10)    - no app
        //----------------------------------------------------------------------
        // App Reservation0           +R(10)    +R(20)               +R(20)
        // App Reservation1           +R(10)    +R(30)     +R(30)    +R(30)       

        fm_->RefreshPLB(Stopwatch::Now());


        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(-1, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));


        // update apps
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName0),
            CreateApplicationCapacities(L""),
            minimumNodes,
            appScaleoutCount));


        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"MyMetric/1000/150/75"),
            minimumNodes,
            appScaleoutCount));

        plb.ProcessPendingUpdatesPeriodicTask();


        // Node:                    0         1          2           3  
        // Node Capacity           100        130          90       130
        //----------------------------------------------------------------------
        // service0                    P        S           -         S        - app0
        // service1                   P(40)    S(20)     S(20)       S(20)    - app1
        // service2                P(40)    S(30)      -       S(30)    - app0
        // service3                P(20)    S(10)    S(10)     S(10)    - no app
        //----------------------------------------------------------------------
        // App Reservation1           -        +R(55)     +R(55)    +R(55)       

        fm_->RefreshPLB(Stopwatch::Now());


        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName3), CreateGuid(3), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName3), CreateGuid(3), CreateNodeId(2), CreateNodeId(3)));


        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationMultipleMetrics)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationMultipleMetrics";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/60, HDD/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/60, HDD/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/60, HDD/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/60, HDD/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/50/50, HDD/1000/70/30"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName);
        wstring serviceName3 = wformatString("{0}/serviceName3", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/40/20, HDD/1.0/20/15")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/30/10")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName3),
            wstring(serviceTypeName),
            wstring(),
            true,
            CreateMetrics(L"HDD/1.0/20/10")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/1, S/2, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"P/0, S/1, S/2"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3),
            wstring(serviceName3),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3"),
            0));

        // Node:                    0             1            2            3  
        // Node Capacity           60/40        60/30        60/30        60/30
        //-------------------------------------------------------------------------------
        // Application res - [CPU - 50 / HDD - 30]
        //-------------------------------------------------------------------------------
        // service0                    P            S            S              S           - app
        // service1                   -            P(40/20)    S(20/15)    S(20/15)    - app
        // service2                P(30/-)      S(10/-)     S(10/-)       -         - app
        // service3                P(-/20)      S(-/10)     S(-/10)     S(-/10)     - app
        //-------------------------------------------------------------------------------
        // App CPU reservation:       +R(20)        +R(0)        +R(20)      +R(30)    
        // App HDD reservation:       +R(10)        +R(0)        +R(5)       +R(5)   

        fm_->RefreshPLB(Stopwatch::Now());

        // Better score for N3
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName3), CreateGuid(3), CreateNodeId(2), CreateNodeId(1)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName3), CreateGuid(3), CreateNodeId(3), CreateNodeId(1)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName3), CreateGuid(3), CreateNodeId(2), CreateNodeId(3)));
    }
    

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationReservationMultipleMetricsMergeServiceDomain)
    {
        wstring testName = L"PromoteSecondaryWithApplicationReservationMultipleMetricsMergeServiceDomain";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/20,M2/30,M3/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/20,M2/28,M3/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/20,M2/30,M3/42"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/17,M2/30,M3/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"M1/20,M2/30,M3/45"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName1 = wformatString("fabric:/{0}_Application1", testName);
        wstring applicationName2 = wformatString("fabric:/{0}_Application2", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName1),
            CreateApplicationCapacities(L"M1/500/50/15, M2/1000/80/25"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName2),
            CreateApplicationCapacities(L"M3/700/80/40"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName1);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName1);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName2);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"M1/1.0/20/10")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName1),
            true,
            CreateMetrics(L"M2/1.0/30/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName2),
            true,
            CreateMetrics(L"M3/1.0/45/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1, S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/0, S/1, S/2"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"),
            0));
         

        fm_->RefreshPLB(Stopwatch::Now());

        // Node:                    0             1            2            3            4
        // Node Capacity           20/30/60        20/28/40    20/30/42    17/30/40     20/30/45    [M1/M2/M3]
        //---------------------------------------------------------------------------------------------
        // Application1 res - [M1 - 15 / M2 - 25]
        // Application2 res - [M3 - 40] 
        //---------------------------------------------------------------------------------------------
        // Domain1
        // service0                   P(M1-20)        S(M1-10)        -          S(M1-10)      -         - app1
        // service1                   P(M2-30)     S(M2-20)    S(M2-20)    -             -         - app1
        // App1 M1 reservation:       +R(0)        +R(5)        -           +R(5)         - 
        // App2 M2 reservation:       +R(0)        +R(5)        +R(5)        -            -

        //----------------------------------------------------------------------------------------------
        // Domain2
        // service2                P(M3-45)     S(M3-30)    S(M3-30)    S(M3-30)      S(M3-30)   - app2
        // App2 M3 reservation:       +R(0)        +R(10)        +R(10)      +R(10)        +R(10)  

       
        VERIFY_ARE_EQUAL(-1, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(1), CreateNodeId(2)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(4)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(4)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(3), CreateNodeId(4)));

        // affinity toward service0 ->  merge domains
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            true,
            wstring(applicationName2),
            wstring(serviceName0),
            CreateMetrics(L"M3/1.0/40/30")));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(-1, plb.CompareNodeForPromotion(wstring(serviceName0), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(wstring(serviceName1), CreateGuid(1), CreateNodeId(1), CreateNodeId(2)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(3)));

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(1), CreateNodeId(4)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(2), CreateNodeId(4)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(wstring(serviceName2), CreateGuid(2), CreateNodeId(3), CreateNodeId(4)));     
    }

    BOOST_AUTO_TEST_CASE(ApplicationUpdateWithReservationAndClusterCapacity)
    {
        wstring testName = L"ApplicationUpdateWithReservationAndClusterCapacity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        wstring metricName = L"MyMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Total cluster capacity is 500
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/300", metricName)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/200", metricName)));

        plb.ProcessPendingUpdatesPeriodicTask();
        ErrorCode error;

        wstring applicationName1 = wformatString("{0}_Application1", testName);
        // Total capacity = 500, Max node capacity = 50, Reservation capacity = 50
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            5,  // MinimumNodes
            5,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/500/50/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 250, -1);

        wstring applicationName2 = wformatString("{0}_Application2", testName);
        // Total capacity == 500, MaxNodeCapacity == 100, NodeReservation == 70
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            5,  // MinimumNodes
            5,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/500/100/70", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 250, -1);

        // Total capacity == 500, MaxNodeCapacity == 100, NodeReservation == 50 (decrease)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            5,  // MinimumNodes
            5,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/500/100/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 500, -1);

        plb.DeleteApplication(applicationName2);
        plb.ProcessPendingUpdatesPeriodicTask();

        // Total capacity == 500, MaxNodeCapacity == 100, NodeReservation == 60
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            5,  // MinimumNodes
            5,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/500/100/60", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
        VerifyReservedCapacityAndLoad(metricName, 300, -1);

        // Total capacity == 500, MaxNodeCapacity == 100, NodeReservation == 50 (no space)
        error = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            5,  // MinimumNodes
            5,  // MaximumNodes
            CreateApplicationCapacities(wformatString("{0}/500/100/50", metricName))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
        VerifyReservedCapacityAndLoad(metricName, 300, -1);
    }

    BOOST_AUTO_TEST_CASE(VerifyApplicationReservationUpdateLoadCost)
    {
        // single service
        wstring testName = L"VerifyApplicationReservationUpdateLoadCost";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/100/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/65/55")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 40, 30));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 10, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(1));
    }
    
    BOOST_AUTO_TEST_CASE(VerifyApplicationReservationUpdateLoadCost2)
    {
        // multiple services
        wstring testName = L"VerifyApplicationReservationUpdateLoadCost2";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/100/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/65/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 30, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, serviceName1, L"CPU", 40, 30));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 5, 5));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 5, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 15, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, serviceName1, L"CPU", 10, 5));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 35, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 40, CreateNodeId(1));
    }

    BOOST_AUTO_TEST_CASE(VerifyApplicationReservationUpdateLoadCost3)
    {
        // service no metrics but has application reservation
        wstring testName = L"VerifyApplicationReservationUpdateLoadCost3";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/100/50"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 50, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 50, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 30, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 40, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/25/15")));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 5, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 25, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 10, 5));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 35, CreateNodeId(1));

        //--------------------------------------------------------------------
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            true));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 40, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 45, CreateNodeId(1));
    }

    BOOST_AUTO_TEST_CASE(VerifyApplicationReservationUpdateLoadCost4)
    {
        // multiple metrics
        wstring testName = L"VerifyApplicationReservationUpdateLoadCost4";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100,HDD/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100,HDD/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100,HDD/150"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/100/60,HDD/500/70/45"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);
        wstring serviceName1 = wformatString("{0}/serviceName1", applicationName);
        wstring serviceName2 = wformatString("{0}/serviceName2", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/40/20,HDD/1.0/30/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName1),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName2),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"HDD/1.0/25/15")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 15, CreateNodeId(0));

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 40, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 25, CreateNodeId(1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(serviceName1),
            0,
            CreateReplicas(L"P/1, S/2"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 15, CreateNodeId(0));

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 25, CreateNodeId(1));

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 50, CreateNodeId(2));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 45, CreateNodeId(2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(serviceName2),
            0,
            CreateReplicas(L"S/0, P/2"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 0, CreateNodeId(0));

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 20, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 25, CreateNodeId(1));

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 50, CreateNodeId(2));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 20, CreateNodeId(2));
        //-----------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 10, 5));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 50, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 35, CreateNodeId(1));


        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"HDD", 10, 5));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 20, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 40, CreateNodeId(1));

        //-----------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, serviceName1, L"CPU", 30, 25));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 25, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 35, CreateNodeId(2));

        //-----------------------------------------------------------------------
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, serviceName2, L"HDD", 35, 30));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 5, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"HDD", 10, CreateNodeId(2));

    }

    BOOST_AUTO_TEST_CASE(VerifyApplicationReservationUpdateLoadCost5)
    {
        // single service different situations
        wstring testName = L"VerifyApplicationReservationUpdateLoadCost5";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        int minimumNodes = 2;
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/500/100/95"),
            minimumNodes,
            appScaleoutCount));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        wstring serviceName0 = wformatString("{0}/serviceName0", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName0),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"CPU/1.0/30/15")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName0),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 65, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 80, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 70, 60));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 25, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 35, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 100, 100));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 95, 90));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 5, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 70, 50));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 25, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 45, CreateNodeId(1));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/1000/150/65"),
            minimumNodes,
            appScaleoutCount));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 15, CreateNodeId(1));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/1000/150/40"),
            minimumNodes,
            appScaleoutCount));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 30, 15));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 10, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 25, CreateNodeId(1));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 45, 10));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 30, CreateNodeId(1));

        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"CPU/1000/150/0"),
            minimumNodes,
            appScaleoutCount));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName0, L"CPU", 20, 15));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"CPU", 0, CreateNodeId(1));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpgradeSwapBack)
    {
        wstring testName = L"AppGroupsUpgradeSwapBack";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/1000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/1000"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            1,
            5,
            CreateApplicationCapacities(wformatString(L"CPU/1000/450/0"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/150/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", L"App1", true, CreateMetrics(L"CPU/1/100/100")));

        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0,S/1/J"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0,S/1/J"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 1, CreateReplicas(L"P/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //only 1 replica can be swapped back due to app capacity constraint
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0|1 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0|1 void movement on 1", value)));
        VERIFY_ARE_NOT_EQUAL(actionList[0][0], actionList[1][0]);
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateCCClosureOnAppUpdate)
    {
        wstring testName = L"AppGroupsUpdateCCClosureOnAppUpdate";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(wstring(L"M1"), 30));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/40"));
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp1", 1, 4, CreateApplicationCapacities(L"M1/80/20/0")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp2", 1, 4, CreateApplicationCapacities(L"M1/80/20/0")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp3", 1, 3, CreateApplicationCapacities(L"M1/32/8/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp2", true, CreateMetrics(L"M1/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType", L"TestApp3", true, CreateMetrics(L"M1/1/2/2")));

        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4), FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(5), FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(6), FailoverUnitDescription(CreateGuid(6), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(7), FailoverUnitDescription(CreateGuid(7), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(8), FailoverUnitDescription(CreateGuid(8), wstring(L"TestService3"), 0, CreateReplicas(L"P/3"), 0)));
        fm_->UpdatePlb();

        // Initial placement (N0 in capacity violation):
        // Nodes:            N0(M1:20)    N1(M1:40)    N2(M1: 40)    N3(M1:40)
        // App1, S1, FT1:    P(M1:10)                  S(M1:10)
        // App1, S1, FT2:    P(M1:10)                                S(M1:10)
        // App1, S1, FT3:    P(M1:10)
        // App2, S2, FT4:                 P(M1:10)
        // App2, S2, FT5:                 P(M1:10)     S(M1:10)
        // App3, S3, FT6:                 P(M1:2)
        // App3, S3, FT7:                              P(M1:2)
        // App3, S3, FT8:                                            P(M1:2)

        // Replicas on node N0 are in node capacity violation, hence one of them will be moved
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1|2|3 move primary 0=>*", value)));
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Lower down App2 capacity per instance to 15, hence application will end up in violation on node 1
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp2", 1, 4, CreateApplicationCapacities(L"M1/80/15/0")));
        // Lower down App3 scaleout, hence application will end up in scaleout violaion
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp3", 1, 1, CreateApplicationCapacities(L"M1/32/8/0")));

        
        // There are no placements, and cluster is balanced,
        // hence constraint check is run to fix the app per instance capacity violation on node 1
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");

        bool justFixViolation = (CountIf(actionList, ActionMatch(L"4|5 move primary 1=>3", value)) == 1);
        bool fixAndBalance = (CountIf(actionList, ActionMatch(L"4|5 move primary 1=>0", value)) == 1 && CountIf(actionList, ActionMatch(L"1|2|3 move primary 0=>1", value)) == 1);
        bool isOk = justFixViolation || fixAndBalance;
        VERIFY_IS_TRUE(isOk);

        // Fix scaleout violation
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"6|7|8 move primary *=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateCCClosureOnAppGroupCreation)
    {
        wstring testName = L"AppGroupsUpdateCCClosureOnAppGroupCreation";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(wstring(L"M1"), 40));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/15"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/40"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApp1")));
        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApp2")));
        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApp3")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp2", true, CreateMetrics(L"M1/1/10/5")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType", L"TestApp3", true, CreateMetrics(L"M1/1/2/2")));

        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4), FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(5), FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(6), FailoverUnitDescription(CreateGuid(6), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0)));
        fm_->UpdatePlb();

        // Initial placement (N0 in capacity violation):
        // Nodes:            N0(M1:15)    N1(M1:40)    N2(M1: 40)
        // App1, S1, FT1:    P(M1:10)
        // App1, S1, FT2:    P(M1:10)
        // App2, S2, FT3:                 P(M1:10)
        // App2, S2, FT4:                 P(M1:10)
        // App3, S3, FT5:                 P(M1:2)
        // App3, S3, FT6:                              P(M1:2)

        // Replicas on node N0 are in node capacity violation, hence one of them will be moved
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1|2 move primary 0=>*", value)));
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Create AppGroup from TestApp2, which produces per instance capacity violation on node 1
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp2", 1, 3, CreateApplicationCapacities(L"M1/45/15/0")));
        // Create AppGroup from TestApp3, which produces scalecout violation
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp3", 1, 1, CreateApplicationCapacities(L"")));

        // Constraint check closure should be updated to include partitions from TestApp2 appGroup,
        // hence constraint check run would fix the violation
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3|4 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5|6 move primary 1|2=>1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsUpdateCCClosureOnAppGroupRemoval)
    {
        wstring testName = L"AppGroupsUpdateCCClosureOnAppGroupRemoval";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(wstring(L"M1"), 40));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/15"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/40"));
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApp1")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp2", 1, 2, CreateApplicationCapacities(L"M1/80/15/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType", L"TestApp1", true, CreateMetrics(L"M1/1/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType", L"TestApp2", true, CreateMetrics(L"M1/1/10/5")));

        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4), FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0)));
        fm_->UpdatePlb();

        // Initial placement (N0 in capacity violation):
        // Nodes:            N0(M1:15)    N1(M1:40)    N2(M1: 40)    N3(M1:40)
        // App1, S1, FT1:    P(M1:10)
        // App1, S1, FT2:    P(M1:10)
        // App2, S2, FT3:                 P(M1:10)     S(M1:5)
        // App2, S2, FT4:                              P(M1:10)
        // App2, S2, FT5:                 P(M1:10)..................S(M1:5)

        // Replicas on node N0 are in node capacity violation, hence one of them will be moved
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1|2|3 move primary 0=>*", value)));
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Add new failover unit which produces both scaleout and max node capacity violation for TestApp2 application
        fm_->FuMap.insert(make_pair(CreateGuid(5), FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/3"), 0)));
        fm_->UpdatePlb();

        // Convert the application group TestApp2 to a simple application
        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApp2")));

        // No movements are expected in ConstraintCheck action,
        // as there are no violaions after applicaion update
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountRelaxWithNodeCapacity)
    {
        wstring testName = L"ScaleoutCountRelaxWithNodeCapacity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/100,Memory/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100,Memory/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100,Memory/30"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/100/100")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/0/0,Memory/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // move replica R2 from Node 0 -> Node 2 to fix node capacity
        // scaleoutCount cannot be fixed
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountRelaxWithAffinity)
    {
        wstring testName = L"ScaleoutCountRelaxWithAffinity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/45"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/45"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/55"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"CPU/30"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/30/20")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/50/30")));

        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService3", L"TestType", true, L"", L"TestService0", CreateMetrics(L"CPU/1.0/25/15")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/3"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // move replica R3 from Node 3 -> Node 0 to fix affinity
        // scaleoutCount cannot be fixed
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 3=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountRelaxNoMovementAllowed)
    {
        wstring testName = L"ScaleoutCountRelaxNoMovementAllowed";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        // force only balancing
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/45"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/55"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        set<NodeId> blockList0;
        blockList0.insert(CreateNodeId(1));
        blockList0.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType0"), move(blockList0)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestServiceType0",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/30/10")));

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        blockList1.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestServiceType1",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // scaleoutCount cannot be fixed
        // check if balancing is going to make worse violation
        // it is not supposed to move replica R2 to Node3 and increase scaleoutCount for App1
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountRelaxAllowBalancing)
    {
        wstring testName = L"ScaleoutCountRelaxAllowBalancing";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        // force only balancing
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/45"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/45"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/55"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestType",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/30/10")));

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        blockList1.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestServiceType1",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(1));
        blockList2.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestServiceType2",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // scaleoutCount cannot be fixed
        // allow movement: R0 should be moved from Node0 to Node2
        // it will be more balanced cluster and scaleoutCount for App1 is the same (it is not getting worse)
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountRelaxFixViolations)
    {
        wstring testName = L"ScaleoutCountRelaxFixViolations";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/65"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/65"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/40"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"CPU/100/35/15")));

        set<NodeId> blockList0;
        blockList0.insert(CreateNodeId(1));
        blockList0.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), move(blockList0)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestType0",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/10/10")));

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType1",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/20/20")));

        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(0));
        blockList2.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType2",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/15/10")));

        set<NodeId> blockList3;
        blockList3.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), move(blockList3)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestType3",
            wstring(L"App1"),
            true,
            CreateMetrics(L"CPU/1.0/20/10")));

        set<NodeId> blockList4;
        blockList4.insert(CreateNodeId(0));
        blockList4.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType4"), move(blockList4)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService4",
            L"TestType4",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/30/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // ScaleoutCount cannot be fixed completely, but it can be reduced from 3 to 2 by moving replicas on Node0 & Node2 respecting block listed nodes
        // This will fix application capacity on Node1 as well
        // replica of failoverUnit4 will remain on Node1
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountMoveAllReplicasToNewNode)
    {
        wstring testName = L"ScaleoutCountMoveAllReplicasToNewNode";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/200"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        set<NodeId> blockList0;
        blockList0.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), move(blockList0)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestType0",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType1",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType2",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // move replicas R0, R1 & R2 -> Node 2 to fix scaleout
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ScaleoutCountCorrectNodeCapacityConstraint)
    {
        wstring testName = L"ScaleoutCountCorrectNodeCapacityConstraint";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/100"));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App",
            0,  // MinimumNodes - no reservation
            1,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        set<NodeId> blockList0;
        blockList0.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), move(blockList0)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestType0",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType1",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType2",
            wstring(L"App"),
            true,
            CreateMetrics(L"CPU/1.0/40/20")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestType3",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/120/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // move P3 to Node1 to fix node capacity constraint
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 2=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsFixingAlignedAffinityUsingSwaps)
    {
        wstring testName = L"AppGroupsFixingAlignedAffinityUsingSwaps";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        int numberOfChildren = 120;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"FD0", L"UD0", L"M1/10"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"FD1", L"UD1", L"M1/5"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"FD2", L"UD2", L"M1/5"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp", 1, 3, CreateApplicationCapacities(L"")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ParentServiceType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ChildServiceType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            wstring(L"ParentService"),
            wstring(L"ParentServiceType"),
            wstring(L"TestApp"),
            true,                   // isStateful
            wstring(L""),           // placementConstraints
            wstring(L""),           // affinitizedService
            true,                   // alignedAffinity
            CreateMetrics(L"M1/1/10/5"),
            FABRIC_MOVE_COST_LOW,
            false,                  // onEveryNode
            1,                      // partitionCount
            2,                      // targetReplicaSetSize
            false));                // hasPersistedState
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(L"ParentService"), 0, CreateReplicas(L"P/1,S/0"), 0)));

        plb.UpdateService(ServiceDescription(
            wstring(L"ChildService"),
            wstring(L"ChildServiceType"),
            wstring(L"TestApp"),
            true,                               // isStateful
            wstring(L""),                       // placementConstraints
            wstring(L"ParentService"),          // affinitizedService
            true,                               // alignedAffinity
            CreateMetrics(L""),
            FABRIC_MOVE_COST_LOW,
            false,                              // onEveryNode
            numberOfChildren,                   // partitionCount
            2,                                  // targetReplicaSetSize
            true));                             // hasPersistedState

        for (int i = 0; i<numberOfChildren; i++)
        {
            fm_->FuMap.insert(make_pair(CreateGuid(i + 2), FailoverUnitDescription(CreateGuid(i + 2), wstring(L"ChildService"), 0, CreateReplicas(L"P/1,S/0"), 0)));
        }
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:   |   N0(M1:10)    |   N2(M1:5)     |   N3(M1:5)    |
        // Parent:  | Parent - S     | Parent - P     |               |
        // Child:   | P1 - S         | P1 - P         |               |
        //          | P2 - S         | P2 - P         |               |

        // Fixing node capacity and newly created affinity violations
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Create the expected action lists
        wstring swapPrimaryList = L"";
        for (int i = 1; i < numberOfChildren + 2; i++)
        {
            swapPrimaryList += std::to_wstring(i);
            if (i < numberOfChildren + 1)
            {
                swapPrimaryList += L"|";
            }
        }
        swapPrimaryList += L" swap primary 1<=>0";
        VERIFY_ARE_EQUAL(121u, actionList.size());
        VERIFY_ARE_EQUAL(121u, CountIf(actionList, ActionMatch(swapPrimaryList, value)));
    }

    BOOST_AUTO_TEST_CASE(AppGroupsFixingAlignedAffinityDWScenario)
    {
        wstring testName = L"AppGroupsFixingAlignedAffinityDWScenario";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;
        int numberOfParents = 3;
        int numberOfChildren = 120;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"FD0", L"UD0", L"CPU/15"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"FD1", L"UD1", L"CPU/15"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"FD2", L"UD2", L"CPU/15"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"DWComputeApp", 1, 3, CreateApplicationCapacities(L"CPU/100/30/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"LogicalServerComputeType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"DistributionDBType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"CacheType"), set<NodeId>()));

        for (int i = 0; i < numberOfParents; i++)
        {
            plb.UpdateService(ServiceDescription(
                wstring(L"LogicalServerCompute" + std::to_wstring(i)),
                wstring(L"LogicalServerComputeType"),
                wstring(L"DWComputeApp"),
                true,                   // isStateful
                wstring(L""),           // placementConstraints
                wstring(L""),           // affinitizedService
                true,                   // alignedAffinity
                CreateMetrics(L"CPU/1/10/5"),
                FABRIC_MOVE_COST_LOW,
                false,                  // onEveryNode
                1,                      // partitionCount
                2,                      // targetReplicaSetSize
                false));                // hasPersistedState
        }
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(L"LogicalServerCompute0"), 0, CreateReplicas(L"P/1,S/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(L"LogicalServerCompute1"), 0, CreateReplicas(L"P/1,S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(L"LogicalServerCompute2"), 0, CreateReplicas(L"P/2,S/0"), 0)));


        for (int i = 0; i < numberOfChildren; i++)
        {
            for (int j = 0; j < numberOfParents; j++)
            {
                plb.UpdateService(ServiceDescription(
                    wstring(L"Cache" + std::to_wstring(i*numberOfParents + j)),
                    wstring(L"CacheType"),
                    wstring(L"DWComputeApp"),
                    true,                                                   // isStateful
                    wstring(L""),                                           // placementConstraints
                    wstring(L"LogicalServerCompute" + std::to_wstring(j)),  // affinitizedService
                    true,                                                   // alignedAffinity
                    CreateMetrics(L"Disk/1/10/10"),
                    FABRIC_MOVE_COST_LOW,
                    false,                                                  // onEveryNode
                    1,                                                      // partitionCount
                    2,                                                      // targetReplicaSetSize
                    true));                                                 // hasPersistedState

                plb.UpdateService(ServiceDescription(
                    wstring(L"DistributionDB" + std::to_wstring(i*numberOfParents + j)),
                    wstring(L"DistributionDBType"),
                    wstring(L"DWComputeApp"),
                    true,                                                   // isStateful
                    wstring(L""),                                           // placementConstraints
                    wstring(L"LogicalServerCompute" + std::to_wstring(j)),  // affinitizedService
                    true,                                                   // alignedAffinity
                    CreateMetrics(L"Disk/1/10/10"),
                    FABRIC_MOVE_COST_LOW,
                    false,                                                  // onEveryNode
                    1,                                                      // partitionCount
                    2,                                                      // targetReplicaSetSize
                    false));                                                // hasPersistedState
            }

            fm_->FuMap.insert(make_pair(CreateGuid(4 + i),
                FailoverUnitDescription(CreateGuid(4 + i), wstring(L"Cache" + std::to_wstring(i*numberOfParents + 0)), 0, CreateReplicas(L"P/1,S/0"), 0)));
            fm_->FuMap.insert(make_pair(CreateGuid((4 + numberOfChildren) + i),
                FailoverUnitDescription(CreateGuid((4 + numberOfChildren) + i), wstring(L"DistributionDB" + std::to_wstring(i*numberOfParents + 0)), 0, CreateReplicas(L"P/1"), 0)));
            fm_->FuMap.insert(make_pair(CreateGuid((4 + 2*numberOfChildren) + i),
                FailoverUnitDescription(CreateGuid((4 + 2*numberOfChildren) + i), wstring(L"Cache" + std::to_wstring(i*numberOfParents + 1)), 0, CreateReplicas(L"P/1,S/2"), 0)));
            fm_->FuMap.insert(make_pair(CreateGuid((4 + 3*numberOfChildren) + i),
                FailoverUnitDescription(CreateGuid((4 + 3*numberOfChildren) + i), wstring(L"DistributionDB" + std::to_wstring(i*numberOfParents + 1)), 0, CreateReplicas(L"P/1"), 0)));
            fm_->FuMap.insert(make_pair(CreateGuid((4 + 4*numberOfChildren) + i),
                FailoverUnitDescription(CreateGuid((4 + 4*numberOfChildren) + i), wstring(L"Cache" + std::to_wstring(i*numberOfParents + 2)), 0, CreateReplicas(L"P/2,S/0"), 0)));
            fm_->FuMap.insert(make_pair(CreateGuid((4 + 5*numberOfChildren) + i),
                FailoverUnitDescription(CreateGuid((4 + 5*numberOfChildren) + i), wstring(L"DistributionDB" + std::to_wstring(i*numberOfParents + 2)), 0, CreateReplicas(L"P/2"), 0)));
        }
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:   |   N0(CPU:10)   |   N1(CPU:10)   |   N2(CPU: 10)   |
        // LSCs:    | LSC1s   LSC3s  | LSC1p   LSC2p  | LSC2s    LSC3p  |
        // Caches:  | C1s     C3s    | C1p     C2p    | C2s      C3p    |
        // DBs:     |                | DB1p    DB2p   |          DB3p   |

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Create the expected action lists
        wstring swapPrimaryList = L"1|";
        wstring movePrimaryList = L"";
        for (int i = 0; i < numberOfChildren; i++)
        {
            swapPrimaryList += std::to_wstring(4 + i);
            movePrimaryList += std::to_wstring((4 + numberOfChildren) + i);
            if (i < numberOfChildren - 1)
            {
                swapPrimaryList += L"|";
                movePrimaryList += L"|";
            }
        }
        swapPrimaryList += L" swap primary 1<=>0";
        movePrimaryList += L" move primary 1=>0";
        VERIFY_ARE_EQUAL(241u, actionList.size());
        VERIFY_ARE_EQUAL(121u, CountIf(actionList, ActionMatch(swapPrimaryList, value)));
        VERIFY_ARE_EQUAL(120u, CountIf(actionList, ActionMatch(movePrimaryList, value)));
    }

    BOOST_AUTO_TEST_CASE(PLB_AppGroupsIncrementalConstraintCheckFix)
    {
        wstring testName = L"PLB_AppGroupsIncrementalConstraintCheckFix";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, Common::TimeSpan::FromSeconds(0.5));
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, -1);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        int numOfNodes = 9;
        int numOfServices = 100;

        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"M1/300"));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = L"fabric:/LoadBalancingAppType";
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            1,
            4,
            CreateApplicationCapacities(wformatString(L""))));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceTypeTest"), set<NodeId>()));

        for (int serviceIndex = 0; serviceIndex < numOfServices; serviceIndex++)
        {
            wstring serviceName = wformatString("Service_{0}",serviceIndex);
            plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, L"ServiceTypeTest", applicationName, true, CreateMetrics(L"M1/1.0/4/4")));
            wstring replicaString = wformatString("P/{0},S/{1},S/{2}", serviceIndex % numOfNodes, (serviceIndex + 1) % numOfNodes, (serviceIndex + 2) % numOfNodes);
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(serviceIndex), wstring(serviceName), 0, CreateReplicas(replicaString), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_NOT_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ApplicationCreateDeleteServiceCreateTest)
    {
        wstring testName = L"BalancingInterruptedByApplicationUpdateNoScaleoutTest";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        wstring metric = L"myMetric";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/200", metric)));
        }

        plb.UpdateApplication(ApplicationDescription(wstring(L"App1"), std::map<std::wstring, ApplicationCapacitiesDescription>(), 1));
        plb.DeleteApplication(L"App1");

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        ErrorCode error = plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, L"App1", true, CreateMetrics(wformatString("{0}/1/50/50", metric))));
        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::ApplicationInstanceDeleted);
    }

    BOOST_AUTO_TEST_CASE(AppGroupsReservationInsufficientCapacity)
    {
        wstring testName = L"AppGroupsReservationInsufficientCapacity";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeCount = 3;
        int nodeIndex = 0;
        int guidIndex = 0;

        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(), L"", true));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appName = L"App1";
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            appName,
            1,
            241,
            CreateApplicationCapacities(wformatString(L"DwNodeUnit/241/1/1"))));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int i = 0; i < 3; ++i)
        {
            auto serviceName = wformatString(L"{0}_Service_{1}", appName, i);
            plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), L"TestType", wstring(appName), true, CreateMetrics(L"DwNodeUnit/0/1/0")));
        }

        for (int i = 0; i < 3; ++i)
        {
            auto serviceName = wformatString(L"{0}_Service_{1}", appName, i);
            auto replicaPlacement = wformatString(L"P/{0}", nodeIndex++);
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(serviceName),
                0, CreateReplicas(wstring(replicaPlacement)), 0));
        }

        appName = L"App2";
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            appName,
            1,
            241,
            CreateApplicationCapacities(wformatString(L"DwNodeUnit/241/1/1"))));

        for (int i = 0; i < 3; ++i)
        {
            auto serviceName = wformatString(L"{0}_Service_{1}", appName, i);
            plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), L"TestType", wstring(appName), true, CreateMetrics(L"DwNodeUnit/0/1/0")));
        }

        for (int i = 0; i < 3; ++i)
        {
            auto serviceName = wformatString(L"{0}_Service_{1}", appName, i);
            auto replicaPlacement = wformatString(L"P/{0}", nodeIndex++);
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(guidIndex++), wstring(serviceName),
                0, CreateReplicas(wstring(replicaPlacement)), 0));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        wstring metric1 = L"MyMetric1";
        wstring metric2 = L"DwNodeUnit";

        // there should be domain merge
        plb.UpdateService(CreateServiceDescription(L"Service1", L"TestType", true, CreateMetrics(wformatString("{0}/1.0/1/1", metric1))));
        plb.UpdateService(CreateServiceDescription(L"Service2", L"TestType", true, CreateMetrics(wformatString("{0}/1.0/1/1,{1}/1.0/1/1", metric1, metric2))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // 2 applications with reserved load = 1 => reserved capacity and load = 2
        VerifyReservedCapacityAndLoad(metric2, 2, 2);

        // verify that creating new app will succeed
        appName = L"App3";
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            appName,
            1,
            241,
            CreateApplicationCapacities(wformatString(L"DwNodeUnit/241/1/1"))));

        fm_->RefreshPLB(Stopwatch::Now());

        // 3 applications with reserved load = 1 => reserved capacity = 3
        // reserved load for app3 still doesn't exist
        VerifyReservedCapacityAndLoad(metric2, 3, 2);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBAppGroups::VerifyAllApplicationReservedLoadPerNode(wstring const& metricName, int64 expectedReservedLoad, Federation::NodeId nodeId)
    {
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        int64 reservedLoad(0);
        
        plbTestHelper.GetReservedLoadNode(metricName, reservedLoad, nodeId);

        Trace.WriteInfo("PLBAppGroupsTestSource", "VerifyAllApplicationReservedLoadPerNode: All reservations on node {1} : {2} (expected {3})",
            nodeId,
            reservedLoad,
            expectedReservedLoad);

        VERIFY_ARE_EQUAL(reservedLoad, expectedReservedLoad);
    }

    void TestPLBAppGroups::VerifyReservedCapacityAndLoad(wstring const& metricName,
        int64 expectedReservedCapacity,
        int64 expectedReservedLoadUsed)
    {
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        int64 reservedCapacity(0);
        int64 reservedLoadUsed(0);

        plbTestHelper.GetReservedLoad(metricName, reservedCapacity, reservedLoadUsed);

        Trace.WriteInfo("PLBAppGroupsTestSource", "VerifyReservedCapacityAndLoad: Capacity: {0} (expected {1}), loadUsed: {2} (expected {3})",
            reservedCapacity,
            expectedReservedCapacity,
            reservedLoadUsed,
            expectedReservedLoadUsed);

        VERIFY_IS_TRUE(true);
        VERIFY_ARE_EQUAL(reservedCapacity, expectedReservedCapacity);
        VERIFY_ARE_EQUAL(reservedLoadUsed, expectedReservedLoadUsed);
    }

    bool TestPLBAppGroups::ClassSetup()
    {
        Trace.WriteInfo("PLBAppGroupsTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        upgradingFlag_.SetUpgrading(true);

        return TRUE;
    }

    bool TestPLBAppGroups::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBAppGroups::ClassCleanup()
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
