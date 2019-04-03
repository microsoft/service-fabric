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
#include "Reliability/Failover/common/FailoverConfig.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability::LoadBalancingComponent;

    class TestPLBPlacement
    {
    protected:
        TestPLBPlacement() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBPlacement()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        void PlacementWithUpgradeAndStandByReplicasHelper(wstring testName, PlacementAndLoadBalancing & plb);
        void PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            wstring testName,
            PlacementAndLoadBalancing & plb,
            wstring & volatileParentService,
            wstring & volatileChildService,
            wstring & persistedParentService,
            wstring & persistedChildService);

        void PlacementMissingReplicasWithQuorumBasedDomainHelper(PlacementAndLoadBalancing & plb, bool fdTreeExists);

        void PlacementWithExistingReplicaMoveThrottleTestHelper(bool placementExpected);

        void NodeBufferPercentageWithPrimaryToBePlacedTest();

        void UseHeuristicDuringPlacementOfHugeBatchTest();

        void PlacementOnNodeWithShouldDisapperLoadTestHelper(wstring const& testName, bool featureSwitch);

        shared_ptr<TestFM> fm_;
        Reliability::FailoverUnitFlags::Flags upgradingFlag_;
        Reliability::FailoverUnitFlags::Flags swappingPrimaryFlag_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBPlacementSuite,TestPLBPlacement)

    /*BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithPrimaryToBePlacedTest2)
    {
            NodeBufferPercentageWithPrimaryToBePlacedTest(false);
    }*/

    BOOST_AUTO_TEST_CASE(PlacementBasicTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementBasicTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"I/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"N/1"), 1)); // when there is no primary but with down secondaries

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary *", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add instance *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"3 add * *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add primary *", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"4 add secondary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add primary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PromoteAndSwapbackPlacementConstraint)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PromoteAndSwapbackPlacementConstraint");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProp1;
        nodeProp1[L"Color"] = L"Blue";
        map<wstring, wstring> nodeProp2;
        nodeProp2[L"Color"] = L"Red";
        map<wstring, wstring> nodeProp3;
        nodeProp3[L"Color"] = L"Red";
        plb.UpdateNode(CreateNodeDescription(0, L"", L"", move(nodeProp1)));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", move(nodeProp2)));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"", move(nodeProp3)));

        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"Color == Blue"));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService2", L"TestType", true, L"Color == Red"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/I, S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1/J"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0/J"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/1/I, S/0"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/1/I, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L"P/1, S/2/J"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(5, actionList.size());
        //will get in violation
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 void movement on 1", value)));
        //already is in violation but if we swap primary will be fine so we should allow it
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 swap primary 1<=>0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 swap primary 1<=>0", value)));
        //these are fine
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 swap primary 1<=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 swap primary 1<=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementPreferredPrimary)
    {
        Trace.WriteInfo("PLBPlacementTestSource", " PlacementPreferredPrimary ");

        //this test checks whether we are considering proper loads during upgrade
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/30"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/30/5")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/25/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"S/0/L,P/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);
        //the primary on node 1 should report load of 30 and secondary on node 0 load of 5 so we can only place on node 0
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithLBDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithLBDomainTest");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 16; i++)
        {
            map<wstring, uint> capacityRatios;
            capacityRatios.insert(make_pair(ServiceMetric::PrimaryCountName, i == 0 ? 5 : 1));
            capacityRatios.insert(make_pair(ServiceMetric::ReplicaCountName, 1));
            plb.UpdateNode(NodeDescription(CreateNodeInstance(i, 0), true, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
                map<wstring, wstring>(), NodeDescription::DomainId(), wstring(), move(capacityRatios), map<wstring, uint>()));
        }

        set<NodeId> blockList;
        for (int i = 4; i < 16; i++)
        {
            blockList.insert(CreateNodeId(i));
        }
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        for (int i = 0; i < 8; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        }

        /* Perfect balanced result is like
        0: P0 P1 S0 S1 S2 S3
        1: P2 P3 S0 S1 S2 S3
        2: P4 P5 S4 S5 S6 S7
        3: P6 P7 S4 S5 S6 S7
        */

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(24u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"* add secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithFailoverUnitVersionChange)
    {
        Trace.WriteInfo("PLBBalancingTestSource", "PlacementWithFailoverUnitVersionChange");
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

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 1));

        plb.InterruptSearcherRunThread = Common::Timer::Create(TimerTagDefault, [&](Common::TimerSPtr const&) mutable
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L""), 1));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 1, CreateReplicas(L""), 1));
            plb.ProcessPendingUpdatesPeriodicTask();
            plb.InterruptSearcherRunFinished = true;
        }, true);

        plb.InterruptSearcherRunFinished = false;

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"0 add instance *", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 add instance *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add instance *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementBalancedTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementBalancedTest");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 60);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        for (int i = 0; i < 60; i ++)
        {
            if (i < 30)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
            }
            else if (i < 45)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1));
            }
            else
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 1));
            }
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(60u, actionList.size());
        VERIFY_ARE_EQUAL(5, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
        VERIFY_ARE_EQUAL(5, CountIf(actionList, ActionMatch(L"* add primary 1", value)));
        VERIFY_ARE_EQUAL(20, CountIf(actionList, ActionMatch(L"* add primary 2", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add secondary 0", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add secondary 1", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementAffinityNoParentPartition)
    {
        wstring testName = L"PlacementAffinityNoParentPartition";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"", false));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        auto now = Stopwatch::Now();

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", testType, wstring(L""), true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", testType, true, L"TestService1", CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we cannot place the child yet, we are checking that everything works fine when the parent replicas is on a down node
        //as then we do not create the partition entry
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NewReplicaPlacement");

        fm_->Clear();
        fm_->RefreshPLB(now += PLBConfig::GetConfig().MinPlacementInterval);
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 1, CreateReplicas(L"D/0,D/1"), 0));
        actionList = GetActionListString(fm_->MoveActions);
        //we cannot place the child yet, we are checking that everything works fine when the parent replicas are dropped
        //as then we do not create the partition entry
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 2, CreateReplicas(L"P/0,S/1"), 0));
        fm_->RefreshPLB(now += PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);
        //now we place the child
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add * 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementFaultDomainDeactivationCompleteNodes)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementFaultDomainDeactivatedNodes");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0/rack1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1/rack1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc2/rack1", L""));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/2"), 0));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2/rack0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc2/rack1", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/2"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementFaultDomainSafetyCheckCompleteNodes)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementFaultDomainSafetyCheckCompleteNodes");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0/rack1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1/rack1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc2/rack1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, L"dc3/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7, L"dc3/rack1", L""));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));


        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2/rack0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc2/rack1", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"P/0,S/2,S/4,S/7"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 6", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementFaultDomainDeactivatedNodesBlockList)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementFaultDomainDeactivatedNodesBlockList");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0/rack0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0/rack1", L""));
        int nodeId = 2;

        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId, wformatString("dc{0}/rack0", nodeId), L"",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
                ++nodeId;
            }
        }

        //we should not remove these again from FD/UD tree
        set<NodeId> blockList;
        for (int i = 2; i < nodeId; ++i)
        {
            blockList.insert(CreateNodeId(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"P/0"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithStandbyTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithStandbyTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"SB/3, SB/7"), 1));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(10u, actionList.size());
        VERIFY_ARE_EQUAL(5u, CountIf(actionList, ActionMatch(L"* add primary 3", value)));
        VERIFY_ARE_EQUAL(5u, CountIf(actionList, ActionMatch(L"* add primary 7", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest1)
    {
        // Scenario for flag SwapPrimary, only certain nodes are valid for swap
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/11/9")));

        // I is the flag used by FM to mark isSwapPrimary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3, S/4"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // only secondary on node 4 has the capacity
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>4", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithServiceUpdate)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithServiceUpdate");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/20"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Update service
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/10")));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest2)
    {
        // Scenario for flag IsPrimaryToBePlaced
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/200"));

        // Node5 has insufficient capacity for any replica
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/5"));
        // Node5 has insufficient capacity for any primary
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(6, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(7, L"MyMetric/200"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/11/9")));

        // I is the flag used by FM to mark isSwapPrimary
        // J is the flag used by FM to mark is primary ToBePlaced or moved
        // K is the flag used by FM to mark is secondary to be placed
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"D/0/J, P/1, S/2, S/3, S/4"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, SB/2/K, S/3, S/4"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2, S/3, D/4/K"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2, SB/3/J, S/4"), 1, upgradingFlag_));

        // Primary can only be swapped with 1 due to capacity on 6 and 7
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I, S/6, S/7"), 0, upgradingFlag_));
        // Primary CAN'T be placed on node5 because it does not have the capacity although the flag is set
        // A VOID movement should be generated to clear the FM flag
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2, S/4, D/5/J"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Node 0 is the preferred primary location
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary 4", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary 3", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 swap primary 0<=>7", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 void movement on *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest3)
    {
        // Scenario for flag IsPrimaryToBePlaced
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest3");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/200"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        // I is the flag used by FM to mark isSwapPrimary
        // J is the flag used by FM to mark is primary ToBePlaced or moved
        // K is the flag used by FM to mark is secondary to be placed
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1|2", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/0/J, P/1, S/2"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 1<=>0", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest4)
    {
        // Scenario for flag IsPrimaryToBePlaced
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest4");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/200"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        // I is the flag used by FM to mark isSwapPrimary
        // J is the flag used by FM to mark is primary ToBePlaced or moved
        // K is the flag used by FM to mark is secondary to be placed
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I"), 2, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0 add secondary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest5)
    {
        // Scenario for flag IsPrimaryToBePlaced
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest5: SwappedIn with primary already swapped back");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        // J: primary is already on target, ignore
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/J, S/1, S/2"), 0, upgradingFlag_));

        // K: replicaDiff is 0, return void movement for replica on node 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1/K, S/2"), 0, upgradingFlag_));

        // J: secondary on target, replicaDiff is 0, promote secondary on target
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1/J, S/2"), 0, upgradingFlag_));

        // J: secondary on target, replicaDiff is 1, promote secondary on target and add secondary on the new node
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1/J, S/2"), 1, upgradingFlag_));

        // J: secondary on target, but no primary. ReplicaDiff is 0, ignore
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1/J, S/2"), 0, upgradingFlag_));

        // K: replicaDiff is 1, return void movement for replica on node 1 and add new replica to the node 3
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1/K, S/2"), 1, upgradingFlag_));

        // K: replicaDiff is 2, return void movement for replica on node 1 and add new replica only at node 3 - there is not enough space
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1/K, S/2"), 2, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(7u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add secondary 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 void movement on 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add secondary 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"6 void movement on 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"6 add secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithPreferredLocationConstraintTest)
    {
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeWithPreferredLocationConstraintTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        // K: replicaDiff is 1, promote stand by replica to secondary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, SB/1/K, S/2"), 1, upgradingFlag_));

        // K: replicaDiff is 1, promote down replica to secondary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, D/1/K, S/2"), 1, upgradingFlag_));

        // K: replicaDiff is 2, promote stand by replica to secondary only as
        // that replica will be added according to preferred location constraint,
        // other replica cannot be added according to that constraint
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0, SB/1/K, S/2"), 2, upgradingFlag_));

        // K: replicaDiff is 2, promote down replica to secondary only as
        // that replica will be added according to preferred location constraint,
        // other replica cannot be added according to that constraint
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0, D/1/K, S/2"), 2, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeAndPrimaryToBeSwappedOutAndMissingReplicasTest)
    {
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        wstring testName = L"PlacementWithUpgradeAndPrimaryToBeSwappedOutAndMissingReplicasTest";
        // flag I with replicaDiff, do AddPrimary instead of MovePrimary
        Trace.WriteInfo("PLBPlacementTestSource", "{0}: I with AddPrimary", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/10"));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        // I is the flag used by FM to mark isSwapPrimary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0/I, SB/1"), 2, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/2/I, SB/3"), 2, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeAndStandByReplicasTest)
    {
        wstring testName = L"PlacementWithUpgradeAndStandByReplicasTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PlacementWithUpgradeAndStandByReplicasHelper(testName, plb);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeAndStandByReplicasWithNoTransientOvercommitTest)
    {
        wstring testName = L"PlacementWithUpgradeAndStandByReplicasWithNoTransientOvercommitTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        PlacementWithUpgradeAndStandByReplicasHelper(testName, plb);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest7)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest7: don't promote secondary in the same UD");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd0/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd0/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd0/r2", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd1/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd1/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd2/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd2/r1", L"ud1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServicePlacementWithUpgradeTest7", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestServicePlacementWithUpgradeTest7"), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3, S/4, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestServicePlacementWithUpgradeTest7"), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3, S/4, S/5, S/6"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 swap primary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 swap primary 0<=>6", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest8)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest8: SB and J for the same location");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I, SB/1/J"), 1));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 1, CreateReplicas(L"P/0/I, S/1/J"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest9)
    {
        // Scenario for flag IsPrimaryToBePlaced
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest3");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/40"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateless"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceStatelessPlacementWithUpgradeTest9", L"TestTypeStateless", false, CreateMetrics(L"MyMetric/1.0/10/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServicePlacementWithUpgradeTest9", L"TestType", true, CreateMetrics(L"MyMetric/1.0/19/9")));

        // Initial loads: 38/40, 18/20, 18/30, 18/40
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestServicePlacementWithUpgradeTest9"), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestServicePlacementWithUpgradeTest9"), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3"), 0, upgradingFlag_));

        // partition 2 can only be placed on node 2 and 3, the loads would be: 38/40, 18/20, 28/30, 28/40
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestServiceStatelessPlacementWithUpgradeTest9"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Based on the loads, only swap to node 3 is possible, but swap should be generated anyway
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>*", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"* void movement on *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest10)
    {
        // Scenario for flag I and all available nodes are deactivated
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest10");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(1, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric/10")));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(2, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric/10")));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric/10")));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithUpgradeTest10_Service0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"PlacementWithUpgradeTest10_Service0"), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // there should be no action since the nodes are deactivated for Restart intention
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesTest)
    {
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        int nodeId = 0;
        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId++, L"", L"",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));

        // Partitions with primary that should be swapped out to deactivated node
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/20/I,S/{0}", i)), 0, upgradingFlag_));
        }

        index = 30;
        // Partitions with primary that should be swapped back to deactivated node
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/20,S/{0}/J", i)), 0, upgradingFlag_));
        }

        index = 60;
        // Partitions with primary that should be swapped back to deactivated node with standBy replica
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/20,SB/{0}/K", i)), 1, upgradingFlag_));
        }

        index = 100;
        // Partitions with primary that should be swapped out from deactivated node
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/{0}/I,S/20", i)), 0, upgradingFlag_));
        }

        index = 130;
        // Partitions with primary that should be swapped back from deactivated node
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/{0},S/20/J", i)), 0, upgradingFlag_));
        }

        index = 160;
        // Partitions with primary that should be swapped back from deactivated node to the node with standBy replica
        for (int i = 0; i < nodeId; i++)
        {
            if (i == 20) { continue; }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString(L"P/{0},SB/20/K", i)), 1, upgradingFlag_));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // Nodes:
        // NodeId  Replicas                                                 Swap/Place        DeactIntent DeactivationStatus
        // 0       S00      S30/J  SB60/K  P100/I    P130       P160        Allow (N/A)       None        DeactivationSafetyCheckInProgress
        // 1       S01      S31/J  SB61/K  P101/I    P131       P161        Allow             Pause       DeactivationSafetyCheckInProgress
        // 2       S02      S32/J  SB62/K  P102/I    P132       P162        Allow             Restart     DeactivationSafetyCheckInProgress
        // 3       S03      S33/J  SB63/K  P103/I    P133       P163        Don't Allow       RemoveData  DeactivationSafetyCheckInProgress
        // 4       S04      S34/J  SB64/K  P104/I    P134       P164        Don't Allow       RemoveNode  DeactivationSafetyCheckInProgress
        // 5       S05      S35/J  SB65/K  P105/I    P135       P165        Allow (N/A)       None        DeactivationSafetyCheckComplete
        // 6       S06      S36/J  SB66/K  P106/I    P136       P166        Don't Allow       Pause       DeactivationSafetyCheckComplete
        // 7       S07      S37/J  SB67/K  P107/I    P137       P167        Don't Allow       Restart     DeactivationSafetyCheckComplete
        // 8       S08      S38/J  SB68/K  P108/I    P138       P168        Don�t Allow       RemoveData  DeactivationSafetyCheckComplete
        // 9       S09      S39/J  SB69/K  P109/I    P139       P169        Don�t Allow       RemoveNode  DeactivationSafetyCheckComplete
        // 10      S10      S40/J  SB70/K  P110/I    P140       P170        Allow (N/A)       None        DeactivationComplete
        // 11      S11      S41/J  SB71/K  P111/I    P141       P171        Don't Allow       Pause       DeactivationComplete
        // 12      S12      S42/J  SB72/K  P112/I    P142       P172        Don't Allow       Restart     DeactivationComplete
        // 13      S13      S43/J  SB73/K  P113/I    P143       P173        Don't Allow       RemoveData  DeactivationComplete
        // 14      S14      S44/J  SB74/K  P114/I    P144       P174        Don't Allow       RemoveNode  DeactivationComplete
        // 15      S15      S45/J  SB75/K  P115/I    P145       P175        Allow             None        ActivationInProgress
        // 16      S16      S46/J  SB76/K  P116/I    P146       P176        Don't Allow (N/A) Pause       ActivationInProgress
        // 17      S17      S47/J  SB77/K  P117/I    P147       P177        Don't Allow (N/A) Restart     ActivationInProgress
        // 18      S18      S48/J  SB78/K  P118/I    P148       P178        Don't Allow (N/A) RemoveData  ActivationInProgress
        // 19      S19      S49/J  SB79/K  P119/I    P149       P179        Don't Allow (N/A) RemoveNode  ActivationInProgress
        // 20      P0-P23/I P30-53 P60-83  S100-S123 S130-153/J SB160-183/K Allow             None        None
        // 21      S20      S50/J  SB80/K  P120/I    P150       P180        Don't Allow (N/A) Pause       None
        // 22      S21      S51/J  SB81/K  P121/I    P151       P181        Don't Allow (N/A) Restart     None
        // 23      S22      S52/J  SB82/K  P122/I    P152       P182        Don't Allow (N/A) RemoveData  None
        // 24      S23      S53/J  SB83/K  P123/I    P153       P183        Don't Allow (N/A) RemoveNode  None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(124u, actionList.size());

        // only normal nodes (4 from the test) are allowed as target nodes for primary replica swap out.
        // all nodes (24 from the test) are allowed as target nodes for primary replica swap back - TODO bug 6133979.
        // only normal, SCIP/Pause and SCIP/Restart deactivated nodes are allowed for secondary replica placement during upgrade.
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"0|5|10|15 swap primary 20<=>0|5|10|15", value)));
        VERIFY_ARE_EQUAL(4 + 24, CountIf(actionList, ActionMatch(L"* swap primary 20<=>*", value)));
        VERIFY_ARE_EQUAL(6, CountIf(actionList, ActionMatch(L"60|61|62|65|70|75 add secondary 0|1|2|5|10|15", value)));
        VERIFY_ARE_EQUAL(18, CountIf(actionList, ActionMatch(
            L"63|64|66|67|68|69|71|72|73|74|76|77|78|79|80|81|82|83 void movement on 3|4|6|7|8|9|11|12|13|14|16|17|18|19|21|22|23|24", value)));

        // primary replica swap out to normal node is allowed from any deactivated node
        // primary replica swap back to normal node is allowed from any deactivated node
        // secondary replica placement to normal node is allowed wherever primary replica is
        VERIFY_ARE_EQUAL(48, CountIf(actionList, ActionMatch(L"* swap primary *<=>20", value)));
        VERIFY_ARE_EQUAL(24, CountIf(actionList, ActionMatch(L"* add secondary 20", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesAndWithSt1Test)
    {
        // stateful target = 1
        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesAndWithSt1Test";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        int nodeId = 0;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::Restart,
            Reliability::NodeDeactivationStatus::DeactivationComplete));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring volatileServiceName = wformatString("{0}{1}", testName, index++);
        wstring persistedServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(volatileServiceName, testType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));
        plb.UpdateService(CreateServiceDescription(persistedServiceName, testType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, true));

        index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0/DLI,S/1"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0,S/1/B"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0,S/1"), -1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileServiceName), 0, CreateReplicas(L"P/0/DLI,S/2"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0/DLI,S/1"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0,S/1/B"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0,S/1"), -1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedServiceName), 0, CreateReplicas(L"P/0/DLI,S/2"), 0, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(10u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 void movement on 0", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 drop secondary 1", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 promote secondary 2", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"6 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 void movement on 0", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"8 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"9 drop secondary 1", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"11 promote secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test1)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0/DLI,S/1/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 void movement on 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test2)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0/DLI,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0/DLI,S/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0/DLI,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0/DLI,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 void movement on 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 void movement on 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test3)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0/DLI,S/2/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test4)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Test4";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0/DLI,S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0/DLI,S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0/DLI,S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0/DLI,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 promote secondary 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 promote secondary 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 promote secondary 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 promote secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithBlockedUpgradeWithDeactivatingNodesAndWithChildSt1Test1)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithBlockedUpgradeWithDeactivatingNodesAndWithChildSt1Test1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0,S/1/B"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0,S/1/B"), -1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0,S/1/B"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0,S/1/B"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithBlockedUpgradeWithDeactivatingNodesAndWithChildSt1Test2)
    {
        // stateful target = 1
        // parent in upgrade, child not in upgrade, child replacement replicas should be also removed
        wstring testName = L"PlacementWithBlockedUpgradeWithDeactivatingNodesAndWithChildSt1Test2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring volatileParentService;
        wstring volatileChildService;
        wstring persistedParentService;
        wstring persistedChildService;
        PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
            testName, plb, volatileParentService, volatileChildService, persistedParentService, persistedChildService);

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileParentService), 0, CreateReplicas(L"P/0,S/1"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(volatileChildService), 0, CreateReplicas(L"P/0,S/1"), -1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedParentService), 0, CreateReplicas(L"P/0,S/1"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(persistedChildService), 0, CreateReplicas(L"P/0,S/1"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 1", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeTest11)
    {
        // Scenario for flag I and all available nodes are deactivated
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeTest11");

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // J is the flag used by FM to mark PrimaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"S/2/B/J"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //should be no movements in response to inbuild replica with J flag...
        VERIFY_ARE_EQUAL(0u, actionList.size());
        //VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementAddPrimaryToNodeWithStandByTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementAddPrimaryToNodeWithStandByTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/80/80")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/80/80")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"SB/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        // Verify that primary replica of second service is NOT created
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementAddSecondaryToNodeWithStandByTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementAddSecondaryToNodeWithStandByTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/80/80")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/80/80")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"SB/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        // Verify that secondary replica of second service is NOT created
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementAddSecondaryBySelectingStandByTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementAddSecondaryBySelectingStandByTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/85"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementAddSecondaryBySelectingStandByTestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/90/80")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementAddSecondaryBySelectingStandByTestService1"), 0, CreateReplicas(L"SB/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementAddSecondaryBySelectingStandByTestService1"), 0, CreateReplicas(L"SB/2"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 1", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithoutDefaultLoadTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithoutDefaultLoadTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"MyMetric", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"MyMetric", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", L"MyMetric", 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* add instance 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDefaultLoadTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDefaultLoadTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(wstring(L"MyMetric"), 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric/0.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false, CreateMetrics(L"MyMetric/0.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"MyMetric", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"MyMetric", 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService1", L"MyMetric", 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"* add instance 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithServiceOnEveryNodeDefaultLoadConfig)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithServiceOnEveryNodeDefaultLoadConfig");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, false);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/25"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, CreateMetrics(L"CPU/1.0/15/15"), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), -1));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add instance 0|1", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"I/0, I/1"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(0), 25));
        newLoads.insert(make_pair(CreateNodeId(1), 20));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"CPU", 0, newLoads, false));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/15"));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        PLBConfigScopeChange(UseDefaultLoadForServiceOnEveryNode, bool, true);
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval + PLBConfig::GetConfig().MinPlacementInterval);
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementFaultDomainWithDeactivation)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementFaultDomainWithDeactivation");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"dc1", L"ud1", L"", Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"dc1", L"ud1", L"", Reliability::NodeDeactivationIntent::Enum::RemoveData, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"dc2", L"ud2", L"", Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"dc2", L"ud2", L"", Reliability::NodeDeactivationIntent::Enum::RemoveData, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0,S/1,S/3,P/4,S/7"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(PlacementFaultDomainMultiLevelWithDeactivation)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementFaultDomainMultiLevelWithDeactivation");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"rack0/dc0", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"rack0/dc1", L"ud4"));
        plb.UpdateNode(CreateNodeDescription(index++, L"rack1/dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"rack1/dc1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"rack1/dc2", L"ud2"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"rack2/dc0", L"ud5", L"", Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"rack2/dc1", L"ud6", L"", Reliability::NodeDeactivationIntent::Enum::RemoveNode, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"rack2/dc0", L"ud7", L"", Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"rack2/dc1", L"ud8", L"", Reliability::NodeDeactivationIntent::Enum::RemoveData, Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/2,S/4,S/5,S/6"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(PlacementWithCapacityTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithCapacityTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Count/25"));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        for (int i = 0; i < 100; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(100u, actionList.size());
        VERIFY_ARE_EQUAL(25, CountIf(actionList, ActionMatch(L"* add instance 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithCapacityTest2)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithCapacityTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/4000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/6000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/6000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/4000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/4000"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithBlockListTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithBlockListTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //should have 5 add
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* add * 1", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        fm_->RefreshPLB(Stopwatch::Now());

        //should have 10 add
        vector<wstring> actionList2 = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(10u, actionList2.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList2, ActionMatch(L"* add * 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementOnAllNodesWithBlockListTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementOnAllNodesWithBlockListTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        blockList.insert(CreateNodeId(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        // Placing the service on every node.
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"I/0,I/1,I/3,I/4"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* drop * 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainTest");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(30u, actionList.size());
        VERIFY_ARE_EQUAL(5, CountIf(actionList, ActionMatch(L"* add instance 0", value)));
        VERIFY_ARE_EQUAL(5, CountIf(actionList, ActionMatch(L"* add instance 1", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add instance 2", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add instance 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainAndUpgradeDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainAndUpgradeDomainTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd0", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd1", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd1", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd1", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(8, L"fd2", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(9, L"fd2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(10, L"fd2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(11, L"fd2", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(12, L"fd3", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(13, L"fd3", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(14, L"fd3", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(15, L"fd3", L"ud2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/13"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(8u, actionList.size());
        // TODO: more verifications to verify the target node are in different domains
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeDomainAndPlacementConstraints)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeDomainAndPlacementConstraints");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> goodProperties;
        goodProperties.insert(make_pair(L"NodeType", L"OK"));
        map<wstring, wstring> badProperties;
        badProperties.insert(make_pair(L"NodeType", L"NotOK"));

        // Entire UD0 not OK for placement
        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud0", map<wstring, wstring> (badProperties)));
        plb.UpdateNode(CreateNodeDescription(1, L"fd1", L"ud0", map<wstring, wstring>(badProperties)));
        plb.UpdateNode(CreateNodeDescription(2, L"fd2", L"ud1", map<wstring, wstring>(goodProperties)));
        plb.UpdateNode(CreateNodeDescription(3, L"fd3", L"ud1", map<wstring, wstring>(goodProperties)));
        plb.UpdateNode(CreateNodeDescription(4, L"fd4", L"ud2", map<wstring, wstring>(goodProperties)));
        plb.UpdateNode(CreateNodeDescription(5, L"fd5", L"ud2", map<wstring, wstring>(goodProperties)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateService(ServiceDescription(
            wstring(L"TestService"),      //std::wstring && serviceName,
            wstring(L"TestType"),   //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            true,                               //bool isStateful,
            wstring(L"NodeType==OK"),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            4,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/2,S/4"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithSkewFaultDomainAndUpgradeDomainTest)
    {
        wstring testName = L"PlacementWithSkewFaultDomainAndUpgradeDomainTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd2", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd3", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd4", L"ud4"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd0", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd1", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd2", L"ud3"));
        plb.UpdateNode(CreateNodeDescription(8, L"fd3", L"ud4"));
        plb.UpdateNode(CreateNodeDescription(9, L"fd4", L"ud0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L""), 5));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(25u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainAndBlockListTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainAndBlockListTest: service type disabled in one fd, we should still place in the other fd");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));

        set<Federation::NodeId> blockList;
        blockList.insert(CreateNodeId(2));
        blockList.insert(CreateNodeId(3));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add * 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add * 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest");
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest1_TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest1_TestService1", L"TestType", true, L"PlacementWithAffinityTest1_TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest1_TestService2", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest1_TestService3", L"TestType", true, L"PlacementWithAffinityTest1_TestService2", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest1_TestService0"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest1_TestService1"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"PlacementWithAffinityTest1_TestService2"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"PlacementWithAffinityTest1_TestService3"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6u, actionList.size());

        // TODO: more verification
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest2)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest2");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, false);
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest2_TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest2_TestService1", L"TestType", true, L"PlacementWithAffinityTest2_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest2_TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest2_TestService1"), 0, CreateReplicas(L"P/1"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        if (!PLBConfig::GetConfig().MoveExistingReplicaForPlacement)
        {
            VERIFY_ARE_EQUAL(0u, actionList.size());
        }
        else
        {
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
        }
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest3)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest3");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest3_TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest3_TestService1", L"TestType", true, L"PlacementWithAffinityTest3_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest3_TestService0"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest3_TestService1"), 0, CreateReplicas(L"P/1, S/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        // No need to verify any thing here. If we changed the behavior for parent service placement, we can add verification.
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest4)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest4");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest4_TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest4_TestService1", L"TestType", true, L"PlacementWithAffinityTest4_TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest4_TestService2", L"TestType", true, L"PlacementWithAffinityTest4_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest4_TestService0"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest4_TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest4_TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // No need to verify any thing here. If we changed the behavior for parent service placement, we can add verification.
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest5)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest5");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest5_TestService0", L"TestType", false, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest5_TestService1", L"TestType", false, L"PlacementWithAffinityTest5_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest5_TestService0"), 0, CreateReplicas(L"I/0, I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest5_TestService1"), 0, CreateReplicas(L"I/1"), 2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest5_TestService2", L"TestTypeStateful", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest5_TestService3", L"TestTypeStateful", true, L"PlacementWithAffinityTest5_TestService2", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"PlacementWithAffinityTest5_TestService2"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"PlacementWithAffinityTest5_TestService3"), 0, CreateReplicas(L"P/2"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        // Extra instances/replicas of child service should not get placed; replicaDifference is 2, but only 1 is placed
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add instance *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add secondary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest6)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest6");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest6_TestService0", L"TestTypeStateful", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest6_TestService1", L"TestTypeStateful", true, L"PlacementWithAffinityTest6_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest6_TestService0"), 0, CreateReplicas(L"P/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest6_TestService1"), 0, CreateReplicas(L"P/2"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"PlacementWithAffinityTest6_TestService1"), 0, CreateReplicas(L"P/2"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add secondary 3|4", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add secondary 3|4", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest7)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest7: parent and child should be together even if child FT id is less than parent FT id");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;
        for (int i = 0; i < 15; ++i)
        {
            blockList.insert(CreateNodeId(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeParent"), move(blockList)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeChild"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";

        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest7_TestService0", L"TestTypeParent", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest7_TestService1", L"TestTypeChild", true, L"PlacementWithAffinityTest7_TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest7_TestService1"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest7_TestService0"), 0, CreateReplicas(L""), 3));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6u, actionList.size());

        // TODO: verify colocation
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 add * 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest8)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest8");
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest8_TestService0", L"TestTypeStateful", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest8_TestService1", L"TestTypeStateful", true, L"PlacementWithAffinityTest8_TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest8_TestService1"), 0, CreateReplicas(L"P/2"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest9)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest9");
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest9_TestService0", L"TestTypeStateful", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest9_TestService1", L"TestTypeStateful", true, L"PlacementWithAffinityTest9_TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest9_TestService1"), 0, CreateReplicas(L"P/2"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityTest10)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityTest10");
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithAffinityTest10_TestService0", L"TestTypeStateful", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"PlacementWithAffinityTest10_TestService1", L"TestTypeStateful", true, L"PlacementWithAffinityTest10_TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest10_TestService0"), 0, CreateReplicas(L"P/0/B, SB/1, SB/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest10_TestService1"), 0, CreateReplicas(L""), 3));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest10_TestService1"), 0, CreateReplicas(L""), 3));
        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityTest10_TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityTest10_TestService1"), 0, CreateReplicas(L""), 3));
        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityNonPrimaryAlignmentTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityNonPrimaryAlignmentTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeStateful"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(ServiceDescription(wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService0"), wstring(L"TestTypeStateful"), wstring(L""), true, wstring(L""),
            wstring(L""), true, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));
        plb.UpdateService(ServiceDescription(wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService1"), wstring(L"TestTypeStateful"), wstring(L""), true, wstring(L""),
            wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService0"), false, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService0"), 0, CreateReplicas(L"P/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService1"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"PlacementWithAffinityNonPrimaryAlignmentTest_TestService1"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(6u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add primary 3|4", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityAndBlockList)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityAndBlockList");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, wformatString("faultdomain_{0}", i % 3)));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));

        wstring parentService = L"PlacementWithAffinityAndBlockList_TestService1";
        wstring childService = L"PlacementWithAffinityAndBlockList_TestService2";

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ParentType"), set<NodeId>(blockList)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ChildType"), set<NodeId>(blockList)));
        plb.UpdateService(CreateServiceDescription(parentService, L"ParentType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            childService, L"ChildType", true, parentService, vector<ServiceMetric>()));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0, S/1, S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/0, S/1"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerDropParentReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerDropParentReplica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Parent should be allowed to drop one of its colocated replicas because it would not violate affinity constraint
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1|2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerDropChildReplicas)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerDropChildReplicas");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // PLB should drop one of the non-colocated replicas for each child
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerDropParentReplicaBelowTarget)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerDropParentReplicaBelowTarget");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // PLB should drop parent secondary
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerDropChildAndParentReplicas)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerDropChildAndParentReplicas");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // PLB should drop non-colocated child and any of the parent secondaries
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1|2|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerDropChildToBeDropped)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerDropChildToBeDropped");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 2));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        // One secondary replica of the child partition is ToBeDropped
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2/R"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // PLB should drop the only valid child secondary
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingChildReplicas)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingChildReplicas");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Affinity constraint should allow creation of an additional, non-colocated, child replica for TestService2 because target replica counts allow it
        // TestService1 should have its replica created on node 1 per affinity constraint
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingChildReplicaParentBelowTarget)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingChildReplicaParentBelowTarget");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        // Allow placing additional child if all existing parents are collocated, even though parent has not reached target count
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingChildReplicaParentBelowTargetAndNotColocated)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingChildReplicaParentBelowTargetAndNotColocated");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        // While in affinity violation, allow the child replica to be created only on a node where parent replica exists
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingParentReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingParentReplica");
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 4));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Parent is allowed to be placed anywhere, in this case on the last node because dummy PLB is enabled
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingParentAndChildReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingParentAndChildReplica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // We process parents first, so PLB will first add missing replica for parent, which will fix the affinity constraint violation
        // and allow the creation of an additional child replica - all in the same run
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityChildTargetLargerMissingChildReplicaPlacementWithMove)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityChildTargetLargerMissingChildReplicaPlacementWithMove");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.1);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0", CreateMetrics(L""), false, 4));

        // TestService1 (child) has blocklisted Node 1, therefore TestService0 (parent) will be moved to fix the affinity
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3"), 1));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);

        // CreationWithMove will move parent from a node where child cannot be placed
        // in order to fix the affinity and allow additional child replica creation
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2|3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityParentTargetLargerDropParentReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityParentTargetLargerDropParentReplica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Parent should drop a non-colocated replica
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityParentTargetLargerDropChildReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityParentTargetLargerDropChildReplica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/4"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Child can drop any replica
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityParentTargetLargerMissingParentReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityParentTargetLargerMissingParentReplica");
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/3"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Parent is allowed to be placed anywhere, in this case on the last node because dummy PLB is enabled
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityParentTargetLargerMissingChildReplica)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityParentTargetLargerMissingChildReplica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/4"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Child should be placed with the parent
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithInitialViolationsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithInitialViolationsTest");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/3"), 2)); // 2 or 4
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 1)); // 2 or 4
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 2)); // 2 or 4 should has one, 1/2/3/4 has another
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 1)); // 2 or 4
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/3, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/3, S/2"), 1)); // 0 or 1

        fm_->RefreshPLB(Stopwatch::Now());

        // TODO: more strict check. should place on the domain with least number replicas
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add secondary 1|2|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 2|4", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add secondary 1|2|3|4", value)));
        VERIFY_ARE_NOT_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add secondary 1|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add secondary 2|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add secondary 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithMissingNodePropertiesTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithMissingNodePropertiesTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring properties = L"abcdefghij";
        int missingPropertyNodes = 10;
        for (int i = 0; i < missingPropertyNodes; i++)
        {
            map<wstring, wstring> nodeProperties;
            wstring nodeProperty;
            nodeProperty.push_back(properties[i]);
            nodeProperties.insert(make_pair(nodeProperty, L"1"));
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"z == 1"));


        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->Clear();

        for (int i = 0; i < 3; i++)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"z", L"1"));
            plb.UpdateNode(CreateNodeDescription(i + missingPropertyNodes, L"", L"", move(nodeProperties)));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }


    /*BOOST_AUTO_TEST_CASE(PlacementWithInBuildThrottlingTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithInBuildThrottlingTest");
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(InBuildThrottlingEnabled, bool, true);
        PLBConfigScopeChange(InBuildThrottlingAssociatedMetric, wstring, L"Metric1");
        PLBConfigScopeChange(InBuildThrottlingGlobalMaxValue, int, 10);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/1/1,Metric2/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService2", L"TestType", true, CreateMetrics(L"Metric2/1.0/1/1,Metric3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService3", L"TestType", true, CreateMetrics(L"Metric4/1.0/1/1,Metric5/1.0/1/1")));

        for (int i = 0; i < 5; ++i)
        {
            // inbuild replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1/B"), 0));
        }
        for (int i = 5; i < 50; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 1));
        }

        for (int i = 50; i < 75; ++i)
        {
            // to be created replicas with different metrics
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 1));
        }
        for (int i = 75; i < 100; ++i)
        {
            // to be created replicas in different service domains
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 1));
        }

        StopwatchTime now = Stopwatch::Now();

        // 0~49 are throttled, 50~99 is not throttled
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(55u, fm_->MoveActions.size());

        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1"), 0));
        }
        for (int i = 5; i < 10; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1/B"), 0));
        }
        for (int i = 50; i < 100; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1/B"), 0));
        }
        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(5u, fm_->MoveActions.size());

        for (int i = 5; i < 10; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 2, CreateReplicas(L"P/0, S/1"), 0));
        }
        for (int i = 10; i < 15; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 2, CreateReplicas(L"P/0, S/1"), 0));
        }
        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(10u, fm_->MoveActions.size());

        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(10u, fm_->MoveActions.size());
    }


    BOOST_AUTO_TEST_CASE(PlacementWithSwapPrimaryThrottlingTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithSwapPrimaryThrottlingTest");
        PLBConfigScopeChange(SwapPrimaryThrottlingEnabled, bool, true);
        PLBConfigScopeChange(SwapPrimaryThrottlingAssociatedMetric, wstring, L"Metric1");
        PLBConfigScopeChange(SwapPrimaryThrottlingGlobalMaxValue, int, 10);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/1/1,Metric2/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService2", L"TestType", true, CreateMetrics(L"Metric2/1.0/1/1,Metric3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService3", L"TestType", true, CreateMetrics(L"Metric4/1.0/1/1,Metric5/1.0/1/1")));

        for (int i = 0; i < 5; ++i)
        {
            // inbuild replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        }
        for (int i = 5; i < 50; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));
        }

        for (int i = 50; i < 70; ++i)
        {
            // to be created secondary replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 1));
        }
        for (int i = 70; i < 90; ++i)
        {
            // to be created replicas with different metrics
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        }
        for (int i = 90; i < 100; ++i)
        {
            // to be created replicas in different service domains
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));
        }

        StopwatchTime now = Stopwatch::Now();

        // 0~49 are throttled, 50~99 is not throttled
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(55u, fm_->MoveActions.size());

        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());

        for (int i = 0; i < 5; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 1, CreateReplicas(L"P/0"), 0));
        }
        for (int i = 5; i < 10; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService1"), 1, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        }
        for (int i = 50; i < 70; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1/B"), 0));
        }
        for (int i = 70; i < 100; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService2"), 1, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        }
        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(5u, fm_->MoveActions.size());

        for (int i = 5; i < 10; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 2, CreateReplicas(L"P/0"), 0));
        }
        for (int i = 10; i < 15; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 2, CreateReplicas(L"P/0"), 0));
        }
        fm_->Clear();
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(10u, fm_->MoveActions.size());

        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(10u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithInBuildSwapPrimaryThrottlingTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithInBuildSwapPrimaryThrottlingTest");
        PLBConfigScopeChange(InBuildThrottlingEnabled, bool, true);
        PLBConfigScopeChange(InBuildThrottlingAssociatedMetric, wstring, L"Metric1");
        PLBConfigScopeChange(InBuildThrottlingGlobalMaxValue, int, 10);
        PLBConfigScopeChange(SwapPrimaryThrottlingEnabled, bool, true);
        PLBConfigScopeChange(SwapPrimaryThrottlingAssociatedMetric, wstring, L"Metric2");
        PLBConfigScopeChange(SwapPrimaryThrottlingGlobalMaxValue, int, 10);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/1/1,Metric2/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService2", L"TestType", true, CreateMetrics(L"Metric2/1.0/1/1,Metric3/1.0/1/1")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService3", L"TestType", true, CreateMetrics(L"Metric4/1.0/1/1,Metric5/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L"P/0/B"), 0, swappingPrimaryFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1/B"), 0));

        for (int i = 10; i < 15; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));
        }
        for (int i = 15; i < 20; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 1));
        }
        for (int i = 20; i < 25; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        }
        for (int i = 25; i < 30; ++i)
        {
            // to be created replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 1));
        }

        for (int i = 50; i < 100; ++i)
        {
            // to be created secondary replicas
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 1));
        }

        StopwatchTime now = Stopwatch::Now();

        // 0~49 are throttled, 50~99 is not throttled
        fm_->RefreshPLB(now);
        VERIFY_ARE_EQUAL(66u, fm_->MoveActions.size());
    }
    */

    BOOST_AUTO_TEST_CASE(PromoteSecondaryTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PromoteSecondaryTest");
        PLBConfigScopeChange(PlacementSearchTimeout, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(FastBalancingSearchTimeout, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SlowBalancingSearchTimeout, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/55"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/55"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/55"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/60"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"MyMetric", 50));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"S/1, S/2, S/3, S/4"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService2", L"MyMetric", 10, 5));

        fm_->RefreshPLB(Stopwatch::Now());

        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_IS_TRUE(0 < plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(1, plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(3), CreateNodeId(4)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithAffinityTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PromoteSecondaryWithAffinityTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/1, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));

        // To be able to compare nodes
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_IS_TRUE(0 < plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_IS_TRUE(0 < plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(4), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(L"TestService2", CreateGuid(1), CreateNodeId(2), CreateNodeId(4)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithAffinityTest2)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PromoteSecondaryWithAffinityTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/55"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/5"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/5"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/80"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/2/1")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/30/1")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService4", L"TestType", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/30/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/0, S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"MyMetric", 5, 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService2", L"MyMetric", 5, 1));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService3", L"MyMetric", 20, 5));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(3, L"TestService4", L"MyMetric", 20, 5));

        plb.ProcessPendingUpdatesPeriodicTask();

        // both 2,3 has enough capacity for the primary but 2 does not have enough for affinitized services
        VERIFY_IS_TRUE(0 < plb.CompareNodeForPromotion(L"TestService1", CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));
        VERIFY_IS_TRUE(0 < plb.CompareNodeForPromotion(L"TestService1", CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_ARE_EQUAL(0, plb.CompareNodeForPromotion(L"TestService2", CreateGuid(0), CreateNodeId(2), CreateNodeId(1)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationCapacity)
    {
        wstring testName = L"PromoteSecondaryWithApplicationCapacity";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/40"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            10,
            CreateApplicationCapacities(L"MyMetric/300/30/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring statefulServiceName = wformatString("{0}/StatefulService", applicationName);
        wstring statelessServiceName = wformatString("{0}/StatelessService", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statefulServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));

        // Stateless service is here just to provide load
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statelessServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            false,
            CreateMetrics(L"MyMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(statefulServiceName),
            0,
            CreateReplicas(L"S/0, S/1, S/2, S/3, S/4"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(statelessServiceName),
            0,
            CreateReplicas(L"I/0, I/1, I/2, I/3, I/4"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(statelessServiceName),
            0,
            CreateReplicas(L"I/0, I/2, I/3"),
            0));

        // There is a secondary on every node using load of 10. Primary load is 20.
        // Node:                   0    1    2    3    4
        // Node Capacity:          -    20   30   40   30
        // App Capacity :          30   30   30   30   30
        // Application Load:       30   20   30   30   20
        // Can swap here?          N    N    N    N    Y
        // Reason (App,Node,Both): A    N    B    A    -

        // We need to prefer node 4 to any other node
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(4)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(1), CreateNodeId(4)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(2), CreateNodeId(4)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(3), CreateNodeId(4)));

        // All other combinations should return 0 when comparing because both will be in violation
        for (int node1 = 0; node1 < 4; ++node1)
        {
            for (int node2 = node1 + 1; node2 < 4; ++node2)
            {
                VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(node1), CreateNodeId(node2)));
            }
        }
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationCapacityAndOutsideService)
    {
        wstring testName = L"PromoteSecondaryWithApplicationCapacityAndOutsideService";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            10,
            CreateApplicationCapacities(L"MyMetric/300/30/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring statefulServiceName = wformatString("{0}/StatefulService", applicationName);
        wstring statelessServiceName = wformatString("{0}/StatelessService", applicationName);
        wstring outsideServiceName = L"fabric:/NoApplication/OutsideService";

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statefulServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/20/10")));

        // Stateless service is here just to provide load (within application)
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statelessServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            false,
            CreateMetrics(L"MyMetric/1.0/20/20")));

        // Outside service that will have instance on every node (load is over application capacity).
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(outsideServiceName),
            wstring(serviceTypeName),
            wstring(L""),
            false,
            CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(statefulServiceName),
            0,
            CreateReplicas(L"S/0,S/1,S/2,S/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(statelessServiceName),
            0,
            CreateReplicas(L"I/2,I/3"),
            0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wstring(outsideServiceName),
            0,
            CreateReplicas(L"I/0,I/1,I/2,I/3"),
            0));

        // There is a secondary on every node using load of 10. Primary load is 20.
        // Node:                   0    1    2    3
        // App Capacity:           30   30   30   30
        // Application Load:       10   10   30   30
        // Total Load:             60   60   80   80
        // Can swap here?          Y    Y    N    N

        // Nodes 2 and 3 should always be prefered to nodes 0 and 1 (violation).
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(2)));
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(3)));
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));
        VERIFY_IS_TRUE(-1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));

        // Nodes 0 and 1 are equal (same as nodes 2 and 3)
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(1)));
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));

        // Just to check symmetry: let's revert the nodes in  the first comparison.
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(2), CreateNodeId(0)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(3), CreateNodeId(0)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(2), CreateNodeId(1)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(3), CreateNodeId(1)));
    }

    BOOST_AUTO_TEST_CASE(PromoteSecondaryWithApplicationCapacityMultiMetrics)
    {
        // There are 3 metrics in this test:
        //      AppOnlyMetric   - capacity is defined only in application.
        //      CommonMetric    - capacity is defined both for node and application (but application will be more than enough).
        //      NodeOnlyMetric  - capacity is defined only one nodes.
        wstring testName = L"PromoteSecondaryWithApplicationCapacityMultiMetrics";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // This node will be limited only by application capacity.
        plb.UpdateNode(CreateNodeDescription(0));
        // This node will be limited by node capacity (load from FU0 will reach the limit for NodeOnlyMetric).
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"NodeOnlyMetric/10,CommonMetric/500"));
        // This node will be limited by application capacity only (node capacity is there, but can hold 5 replicas).
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"NodeOnlyMetric/50,CommonMetric/500"));
        // This node will be OK for swaps both in terms of application capacity and node capacity.
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"NodeOnlyMetric/50,CommonMetric/500"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);

        // CommonMetric - capacity is more than enough!
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            10,
            CreateApplicationCapacities(L"CommonMetric/2000/500/0,ApplicationOnlyMetric/8000/2000/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        wstring statefulServiceName = wformatString("{0}/StatefulService", applicationName);
        wstring statelessServiceName = wformatString("{0}/StatelessService", applicationName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statefulServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            true,
            CreateMetrics(L"NodeOnlyMetric/1.0/20/10,CommonMetric/1.0/200/100,ApplicationOnlyMetric/1.0/2000/1000")));

        // Stateless service is here just to provide load
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(statelessServiceName),
            wstring(serviceTypeName),
            wstring(applicationName),
            false,
            CreateMetrics(L"NodeOnlyMetric/1.0/20/10,CommonMetric/1.0/200/100,ApplicationOnlyMetric/1.0/2000/1000")));

        // With this update, each node has this load:
        //  NodeOnlyMetric          - 10 (node 1 is at capacity)
        //  Common metric           - 100
        //  ApplicationOnlyMetric   - 1000
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(statefulServiceName),
            0,
            CreateReplicas(L"S/0, S/1, S/2, S/3"),
            0));

        // With this update, each node will have
        //  NodeOnlyMetric          - 20   on nodes: 0, 2 (OK),
        //  CommonMetric            - 200  on nodes: 0, 2 (OK)
        //  ApplicationOnlyMetric   - 2000 on nodes: 0, 2 (NOT OK for application).
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wstring(statelessServiceName),
            0,
            CreateReplicas(L"I/0,I/2"),
            0));

        // We need to prefer node 3 to any other node
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(3)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(1), CreateNodeId(3)));
        VERIFY_IS_TRUE(1 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(2), CreateNodeId(3)));

        // All other node combinations should be equal
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(1)));
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(0), CreateNodeId(2)));
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(wstring(statefulServiceName), CreateGuid(0), CreateNodeId(1), CreateNodeId(2)));
    }
    BOOST_AUTO_TEST_CASE(NoServicesTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NoServicesTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(NoPartitionsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NoPartitionsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(NoReplicasTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NoReplicasTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L""), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(NoMatchingReplicasTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NoMatchingReplicasTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(3, 0), false, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), map<wstring, uint>()));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(4, 0), false, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), map<wstring, uint>()));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"Parent", L"TestType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"Child", L"TestType", true, L"Parent"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService3"), 0, CreateReplicas(L"P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"Parent"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"Child"), 0, CreateReplicas(L"P/4, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementManyServicesTest)
    {
        wstring testName = L"PlacementManyServicesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        // Even if we are running functional test - in test mode this test lasts much longer - 415 instead of 17 seconds
        // Only engine part of the run is x80 slower
        PLBConfigScopeChange(IsTestMode, bool, false);

        int nodeCount = 100;
        for (int i = 0; i < nodeCount; i++)
        {
            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"A", L"0"));
            plb.UpdateNode(NodeDescription(CreateNodeInstance(i, 0),
                true,
                Reliability::NodeDeactivationIntent::Enum::None,
                Reliability::NodeDeactivationStatus::Enum::None,
                move(nodeProperties),
                move(NodeDescription::DomainId()),
                wstring(L""),
                map<wstring, uint>(),
                CreateCapacities(L"MyMetric/1000000")));
        }

        set<NodeId> serviceTypeBlockList;
        serviceTypeBlockList.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(serviceTypeBlockList)));

        plb.ProcessPendingUpdatesPeriodicTask();

        int fuId = 0;
        for (int i = 0; i < 5000; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, L"TestType", true, L"A==0", CreateMetrics(L"MyMetric/1.0/1/1")));
            for (int j = 0; j < 10; j++)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(fuId++), wstring(serviceName), 0,
                    CreateReplicas(wformatString("P/{0},S/{1},S/{2}", fuId % nodeCount, (fuId + 1) % nodeCount, (fuId + 2) % nodeCount)),
                    i >= 4980 ? 2 : 0));
            }
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(400u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeWithFaultDomain)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementOnEveryNodeWithFaultDomain");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(wstring(L"TestService1"), wstring(L"TestType"), wstring(L""), false, wstring(L""), wstring(L""), true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), INT_MAX));

        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6, CountIf(actionList, ActionMatch(L"1 add instance *", value)));
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"2 add instance *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeWithCapacity)
    {
       Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithCapacityTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, uint> capacities;
        capacities.insert(make_pair(L"MyMetric", 50));

        int nodeCount = 4;
        for (int i = 0; i < nodeCount; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/50"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric/1.0/40/40")));
        plb.UpdateService(ServiceDescription(wstring(L"TestService2"), wstring(L"TestType"), wstring(L""), false, wstring(L""), wstring(L""), true, CreateMetrics(L"MyMetric/1.0/40/40"), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"I/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(nodeCount - 1, CountIf(actionList, ActionMatch(L"2 add instance *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"2 add instance 0", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeWithAddAndDrop)
    {
        wstring testName = L"PlacementOnEveryNodeWithAddAndDrop";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc3"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        // Create service on every node with constraints that prevent it from placement on node 0, and place instance on first 3 nodes
        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(serviceTypeName),
            wstring(L""),
            false, wstring(L"FaultDomain != fd:/dc0"),
            wstring(L""), true, vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wstring(serviceName),
            0,
            CreateReplicas(L"I/0,I/1,I/2"),
            INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();

        // We have total of 4 nodes, 1 node in blocklist and 3 replicas.
        // We should first add one instance, and then drop the one that is violating
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 3", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");

        // Then, update the FM and failover unit
        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"I/0,I/1,I/2,I/3"), INT_MAX));

        // In the second run we should drop an instance from node 0 (placement constraint)
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeNotNeededWithPausedNode)
    {
        wstring testName = L"PlacementOnEveryNodeNotNeededWithPausedNode";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"dc2", L"", L"", Reliability::NodeDeactivationIntent::Enum::Pause));
        plb.UpdateNode(CreateNodeDescription(3, L"dc3"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        // Create service on every node, and place instance on first 3 nodes
        plb.UpdateService(ServiceDescription(wstring(serviceName), wstring(serviceTypeName), wstring(L""), false, wstring(L"FaultDomain != fd:/dc0"), wstring(L""), true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1,I/2"), INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();

        // We have total of 4 nodes, 2 nodes in blocklist and 3 replicas.
        // Node 3 is paused, and we cannot do anything with replica that is on it
        // In the first run we should add one instance on node 3
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add instance 3", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");

        // Then, we update the new FailoverUnit
        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"I/0,I/1,I/2,I/3"), INT_MAX));

        // In the second run we should drop an instance from node 0 (placement constraint)
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(ServiceOnEveryNodeReplicaDrop)
    {
        wstring testName = L"ServiceOnEveryNodeReplicaDrop";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc3"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        // Create service on every node, and place instance on every node
        plb.UpdateService(ServiceDescription(wstring(serviceName), wstring(serviceTypeName), wstring(L""), false, wstring(L""), wstring(L""), true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1,I/2,I/3"), INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Update service so that it can go only to dc0
        plb.UpdateService(ServiceDescription(wstring(serviceName), wstring(serviceTypeName), wstring(L""), false, wstring(L"FaultDomain ^ fd:/dc0"), wstring(L""), true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));

        // We expect PLB to drop one replica
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"0 drop instance *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 drop instance 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeWithDeactivatedNodes)
    {
        wstring testName = L"PlacementOnEveryNodeWithDeactivatedNodes";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"")));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(4, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"")));

        wstring testType0 = wformatString("{0}{1}Type", testName, 0);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType0), set<NodeId>()));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(3));
        wstring testType1 = wformatString("{0}{1}Type", testName, 1);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType1), move(blockList)));

        for (int i = 0; i < 5; ++i)
        {
            blockList.insert(CreateNodeId(i));
        }
        wstring testType2 = wformatString("{0}{1}Type", testName, 2);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType2), move(blockList)));

        wstring serviceName0 = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(serviceName0, testType0, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName0), 0, CreateReplicas(L"I/0,I/1,I/2"), INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName0), 0, CreateReplicas(L"I/0,I/1,I/3"), INT_MAX));

        // Service 1 has replica on deactivated nodes and block list
        wstring serviceName1 = wformatString("{0}{1}", testName, 1);
        plb.UpdateService(CreateServiceDescription(serviceName1, testType1, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName1), 0, CreateReplicas(L"I/0,I/2,I/3,I/4"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Verify in the trace that there is no replica placement attempted.
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add instance 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add instance 1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementOnEveryNodeWithDeactivatedNodes2)
    {
        wstring testName = L"PlacementOnEveryNodeWithDeactivatedNodes2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(0, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"")));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(1, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckComplete,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"")));

        for (int i = 2; i < 50; ++i)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;

        // ServiceType1 has 50 nodes in block list
        for (int i = 1; i < 25; ++i)
        {
            blockList.insert(CreateNodeId(i));
        }
        wstring testType1 = wformatString("{0}{1}Type", testName, 1);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType1), move(blockList)));

        // ServiceType1 has 99 nodes in block list
        for (int i = 1; i < 50; ++i)
        {
            blockList.insert(CreateNodeId(i));
        }
        wstring testType2 = wformatString("{0}{1}Type", testName, 2);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType2), move(blockList)));

        wstring serviceName1 = wformatString("{0}{1}", testName, 1);
        plb.UpdateService(CreateServiceDescription(serviceName1, testType1, false));
        // Valid nodes are [25~49] minus N40 = 49
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"I/0,I/2,I/3,I/40"), INT_MAX));

        wstring serviceName2 = wformatString("{0}{1}", testName, 2);
        plb.UpdateService(CreateServiceDescription(serviceName2, testType2, false));
        // No valid nodes
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L""), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(24u, actionList.size());
        VERIFY_ARE_EQUAL(24, CountIf(actionList, ActionMatch(L"0 add instance *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 add instance *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithRelaxConstraintsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithRelaxConstraintsTest");
        PLBConfigScopeChange(AffinityConstraintPriority, int, 1);
        PLBConfigScopeChange(CapacityConstraintPriority, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/10"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/8/8")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 5, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService", L"MyMetric", 3, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService", L"MyMetric", 2, 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(1u, fm_->MoveActions.size());

        fm_->Clear();

        Sleep(1);
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 1, CreateReplicas(L"P/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithPrimaryReplicaFaultDomainConstraintsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithPrimaryReplicaFaultDomainConstraintsTest");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // Test service try to place primary replica in dc0
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^P fd:/dc0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(30u, actionList.size());
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* add primary 0", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithPrimaryReplicaFaultDomainConstraintsTest2)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithPrimaryReplicaFaultDomainConstraintsTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // Test service try to place primary replica in dc0
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^P fd:/dc0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        plb.ProcessPendingUpdatesPeriodicTask();
        // Conflicts of service FD constraint and primary replica FD constraint
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService2", L"TestType", true, L"(FaultDomain ^ fd:/dc2) && (FaultDomain ^P fd:/dc0)"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        blockList.insert(CreateNodeId(2));
        blockList.insert(CreateNodeId(3));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService3", L"TestType2", true, L"FaultDomain ^P fd:/dc0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(14u, actionList.size());
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"4 add primary *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"5 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithLimitedTimeTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithLimitedTimeTest");
        PLBConfigScopeChange(PlacementSearchTimeout, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd0", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd0", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd1", L"ud1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        for (int i = 0; i < 500; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        }

        fm_->RefreshPLB(Stopwatch::Now());
    }

   BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainNonpackDistributionTest)
    {
        // Nonpacking means one replica per sub-domain of root
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainNonpackDistributionTest");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L""), 4));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService2", L"TestType", true, L"FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 4));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add * 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add * 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 3", value)));
    }

   BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainNonpackDistributionTest2)
    {
        // Nonpacking means one replica per sub-domain of root
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainNonpackDistributionTest2");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // Test service try to place primary replica in dc0 and have to following the nonpacking policy
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^P fd:/dc0 && FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 4));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 3", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainViolationTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithFaultDomainViolationTest");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"I/1, I/3, I/5, I/6"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithDeactivatedNodesQuorumBasedDomainTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "PlacementMissingReplicasWithDeactivatedNodesQuorumBasedDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(10, L"dc1/r0", L"1"));
        plb.UpdateNode(CreateNodeDescription(40, L"dc1/r1", L"4"));

        plb.UpdateNode(CreateNodeDescription(50, L"dc2/r0", L"5"));
        plb.UpdateNode(CreateNodeDescription(20, L"dc2/r1", L"2"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(30, L"dc3/r0", L"3",
            L"", Reliability::NodeDeactivationIntent::RemoveData, Reliability::NodeDeactivationStatus::DeactivationComplete));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"Service0", serviceType, true, L"", CreateMetrics(L""), false, 3));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"Service1", serviceType, true, L"", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service0"), 0, CreateReplicas(L"P/10, S/20"), 1));
        vector<bool> isUp = { true, false, false };
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service1"), 0, CreateReplicas(L"P/40, SB/20/D, S/50/D", isUp), 2));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 40|50", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add secondary 10|20|50", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithEmptyDomainsQuorumBasedDomainTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "PlacementMissingReplicasWithEmptyDomainsQuorumBasedDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties1;
        nodeProperties1.insert(make_pair(L"A", L"0"));
        plb.UpdateNode(CreateNodeDescription(10, L"dc1/r0", L"1", move(nodeProperties1)));
        map<wstring, wstring> nodeProperties2;
        nodeProperties2.insert(make_pair(L"A", L"0"));
        plb.UpdateNode(CreateNodeDescription(20, L"dc1/r1", L"1", move(nodeProperties2)));

        map<wstring, wstring> nodeProperties3;
        nodeProperties3.insert(make_pair(L"A", L"0"));
        plb.UpdateNode(CreateNodeDescription(30, L"dc2/r0", L"2", move(nodeProperties3)));
        map<wstring, wstring> nodeProperties4;
        nodeProperties4.insert(make_pair(L"A", L"0"));
        plb.UpdateNode(CreateNodeDescription(40, L"dc2/r1", L"2", move(nodeProperties4)));

        map<wstring, wstring> nodeProperties5;
        nodeProperties5.insert(make_pair(L"A", L"1"));
        NodeDescription::DomainId fdId70 = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"dc4/r0").Segments;
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(70, 0),
            true,
            Reliability::NodeDeactivationIntent::RemoveData,
            Reliability::NodeDeactivationStatus::DeactivationComplete,
            move(nodeProperties5),
            move(fdId70),
            L"4",
            map<wstring, uint>(),
            CreateCapacities(L"")));

        map<wstring, wstring> nodeProperties6;
        nodeProperties6.insert(make_pair(L"A", L"1"));
        NodeDescription::DomainId fdId80 = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"dc4/r1").Segments;
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(80, 0),
            true,
            Reliability::NodeDeactivationIntent::RemoveData,
            Reliability::NodeDeactivationStatus::DeactivationComplete,
            move(nodeProperties6),
            move(fdId80),
            L"4",
            map<wstring, uint>(),
            CreateCapacities(L"")));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"Service0", serviceType, true, L"A==0", CreateMetrics(L""), false, 3));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"Service1", serviceType, true, L"A==1", CreateMetrics(L""), false, 2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service0"), 0, CreateReplicas(L"P/10, S/30"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service1"), 0, CreateReplicas(L"P/70"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 20|40", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0"));

        PlacementMissingReplicasWithQuorumBasedDomainHelper(plb, true);
    }

    BOOST_AUTO_TEST_CASE(PlacementUpgradeReplicasWithaultDomainViolationTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementUpgradeReplicasWithaultDomainViolationTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameTarget3 = L"StatefulPackingService3";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceNameTarget3, serviceType, true, L"", CreateMetrics(L""), false, 3));

        // J: add primary on node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, S/4, SB/6/J"), 1, upgradingFlag_));

        // J: promote secondary on the node 4 add secondary on the node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, D/1/J, S/4"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());

        // we should actually have "add primary 6" - opened bug 8863707
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add secondary 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 void movement on 6", value)));

        // we should actually send "void movement on 1" and "add secondary on 6" - opened bug 8863707
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithQuorumBasedUpgradeDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementMissingReplicasWithQuorumBasedUpgradeDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"ud0"));

        plb.UpdateNode(CreateNodeDescription(4, L"", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(5, L"", L"ud1"));

        plb.UpdateNode(CreateNodeDescription(6, L"", L"ud2"));

        PlacementMissingReplicasWithQuorumBasedDomainHelper(plb, false);
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithQuorumBasedUpgradeAndFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementMissingReplicasWithQuorumBasedUpgradeAndFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"ud0"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"ud1"));

        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0", L"ud2"));

        PlacementMissingReplicasWithQuorumBasedDomainHelper(plb, true);
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithQuorumBasedMultiLevelFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementMissingReplicasWithQuorumBasedMultiLevelFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"l0/l1/dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"l0/l1/dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(4, L"l0/l1/dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"l0/l1/dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(6, L"l0/l1/dc2/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // stateFull service
        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 7));
        // replica difference is 3 - we expect to add 2 secondary replicas in the first domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/4,S/5, S/6"), 3));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add secondary 1|2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementMissingReplicasWithTwoQuorumBasedFaultDomainsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementMissingReplicasWithTwoQuorumBasedFaultDomainsTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"l0/l1/dc0/rack0"));

        plb.UpdateNode(CreateNodeDescription(2, L"l0/l1/dc1/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0"), 2));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementUpgradeReplicasWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementUpgradeReplicasWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1"));

        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0", L"2"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameTarget3 = L"StatefulPackingService3";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceNameTarget3, serviceType, true, L"", CreateMetrics(L""), false, 3));

        // I: add 2 secondary replicas (not in the dc0) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0/I"), 2, upgradingFlag_));

        // I: add 2 secondary replicas (not in the dc0) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0/I, D/4"), 2, upgradingFlag_));

        // I: add 2 secondary replicas (not in the dc0) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0/I, SB/1"), 2, upgradingFlag_));

        // I: swap primary expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0/I, S/4, S/6"), 0, upgradingFlag_));

        // J: add primary on node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, S/4, SB/6/J"), 1, upgradingFlag_));

        // J: add secondary on the node 6 expected, without promoting to primary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, D/1/J, S/4"), 1, upgradingFlag_));

        // J: secondary on target, replicaDiff is 0, promote secondary on target, although it will remain in violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(6), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, S/4/J, S/5"), 0, upgradingFlag_));

        // J: secondary on target, replicaDiff is 1, promote secondary on target and add secondary on the node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(7), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, S/4/J"), 1, upgradingFlag_));

        // K: replicaDiff is 1, promote stand by replica to secondary
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(8), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, SB/4/K, S/6"), 1, upgradingFlag_));

        // K: replicaDiff is 1, don't promote down replica to secondary as it is in dc0, add secondary to the node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(9), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, D/1/K, S/4"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(12u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 4|5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 6", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 4|5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 6", value)));

        // no actions for FT 2 as there is no available solution respecting optimization constraints

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 swap primary 0 <=> 4|6", value)));

        // we should actually have "add primary 6" - opened bug 8863707
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add secondary 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 void movement on 6", value)));

        // we should actually send "void movement on 1" and "add secondary on 6" - opened bug 8863707
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add primary 1", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"6 swap primary 0 <=> 4", value)));

        // no adding replica for FT 7 as there is no available solution respecting optimization constraints
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 swap primary 0 <=> 4", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"8 add secondary 4", value)));

        // no adding replica for FT 9 as there is no available solution respecting optimization constraints
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"9 void movement on 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementUpgradeSingletonWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementUpgradeSingletonWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0"));

        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1", L"1"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameTarget1 = L"StatefulPackingService1";
        plb.UpdateService(CreateServiceDescription(serviceNameTarget1, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));

        // I: add replacement secondary replicas (not in the dc1 and not in ud1) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/2/DLI"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementUpgradeSingletonWithQuorumBasedFaultDomainWithSATest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementUpgradeSingletonWithQuorumBasedFaultDomainWithSATest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0"));

        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1", L"1"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameTarget1 = L"StatefulPackingService1";
        plb.UpdateService(CreateServiceDescription(serviceNameTarget1, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));

        // I: add replacement secondary replicas (not in the dc1 and not in ud1) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/2/DLI"), 1, upgradingFlag_));

        // adding regular replicas on the node 0 in order to make unbalanced cluster state
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceNameTarget1), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementUpgradeReplicasWithQuorumBasedFaultDomainNoOptimizationTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementUpgradeReplicasWithQuorumBasedFaultDomainNoOptimizationTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(6, L"dc2/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameTarget3 = L"StatefulPackingService3";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceNameTarget3, serviceType, true, L"", CreateMetrics(L""), false, 3));

        // I: add 2 secondary replicas (not in the dc0) expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0/I, SB/1"), 2, upgradingFlag_));

        // J: secondary on target, replicaDiff is 1, promote secondary on target and add secondary on the node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(7), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, S/4/J"), 1, upgradingFlag_));

        // K: replicaDiff is 1, don't promote down replica to secondary as it is in dc0, add secondary to the node 6 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(9), wstring(serviceNameTarget3), 0, CreateReplicas(L"P/0, D/1/K, S/4"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 4|5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 6", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 add secondary 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 swap primary 0 <=> 4", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"9 add secondary 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"9 void movement on 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementDropExtraReplicasWithQuorumBasedFaultDomainNoTreesTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementDropExtraReplicasWithQuorumBasedFaultDomainNoTreesTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Every node in its own upgrade and fault domain
        for (int nodeId = 0; nodeId < 4; ++nodeId)
        {
            plb.UpdateNode(CreateNodeDescription(nodeId));
        }

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName = L"StatefulService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), -1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1|2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementDropExtraReplicasWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementDropExtraReplicasWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc2/rack0"));

        plb.UpdateNode(CreateNodeDescription(9, L"dc3/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName6_5 = L"StatefulPackingService6_5";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName6_5, serviceType, true, L"", CreateMetrics(L""), false, 5));
        // Reducing the target replica count from 6 to 5 - quorum goes from 4 to 3
        // dropping extra replica from the overloaded domain is expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName6_5), 0, CreateReplicas(L"P/0,S/1,S/2, S/4,S/5, S/7"), -1));

        wstring serviceName7_5 = L"StatefulPackingService7_5";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName7_5, serviceType, true, L"", CreateMetrics(L""), false, 5));
        // Reducing the target replica count from 7 to 5 - quorum goes from 4 to 3
        // dropping extra replica from the overloaded domain is expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName7_5), 0, CreateReplicas(L"P/0,S/1,S/2, S/4,S/5,S/6, S/7"), -2));

        wstring serviceName4 = L"StatefulPackingService4";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName4, serviceType, true, L"", CreateMetrics(L""), false, 4));
        // Dropping extra replica should prioritize dropping shouldDisappear replicas even if they are from the underloaded domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName4), 0, CreateReplicas(L"P/0,S/1, S/4,S/5, S/7/V"), -1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 1|2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 1|2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 4|5|6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 7", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementDropExtraReplicasOfServiceOnEveryNodeWithQuorumBasedFaultDomainTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementDropExtraReplicasOfServiceOnEveryNodeWithQuorumBasedFaultDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // domains: 0, 1, 2
        // nodes:   4, 1, 1
        int nodeId = 0;
        for (; nodeId < 4; ++nodeId)
        {
            map<wstring, wstring> nodeProperties0;
            nodeProperties0.insert(make_pair(L"A", L"0"));
            plb.UpdateNode(CreateNodeDescription(nodeId, L"dc0/rack0", L"0", move(nodeProperties0)));
        }

        map<wstring, wstring> nodeProperties0;
        nodeProperties0.insert(make_pair(L"A", L"0"));
        plb.UpdateNode(CreateNodeDescription(nodeId++, L"dc1/rack0", L"1", move(nodeProperties0)));

        map<wstring, wstring> nodeProperties1;
        nodeProperties1.insert(make_pair(L"A", L"1"));
        plb.UpdateNode(CreateNodeDescription(nodeId++, L"dc2/rack0", L"2", move(nodeProperties1)));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        // stateless on every node, no placement constraints
        wstring serviceNameNoPlacementConstraint = L"StatelessNoPlacementConstraint";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceNameNoPlacementConstraint, serviceType, false, L"", CreateMetrics(L""), true, INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceNameNoPlacementConstraint), 0, CreateReplicas(L"I/0,I/1,I/2,I/3, I/4, I/5"), INT_MAX));

        // stateless on every node, one node block-listed by placement constraints
        wstring serviceNamePlacementConstraint = L"StatelessPlacementConstraint";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceNamePlacementConstraint, serviceType, false, L"A==0", CreateMetrics(L""), true, INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceNamePlacementConstraint), 0, CreateReplicas(L"I/0,I/1,I/2,I/3, I/4, I/5"), INT_MAX));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(3));
        wstring serviceTypeBlocklisted = L"TestTypeBlocklisted";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeBlocklisted), set<NodeId>(blockList)));

        // stateless on every node, one node block-listed by service type
        wstring serviceNameServiceType = L"StatelessServiceType";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceNameServiceType, serviceTypeBlocklisted, false, L"", CreateMetrics(L""), true, INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceNameServiceType), 0, CreateReplicas(L"I/0,I/1,I/2,I/3, I/4, I/5"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop instance 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementSmallServicesWithQuorumBasedFaultDomainsTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementSmallServicesWithQuorumBasedFaultDomainsTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"l0/l1/dc0/rack0"));

        plb.UpdateNode(CreateNodeDescription(2, L"l0/l1/dc1/rack0"));

        plb.UpdateNode(CreateNodeDescription(3, L"l0/l1/dc2/rack0"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring statefulServiceName1 = L"Stateful1";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(statefulServiceName1, serviceType, true, L"", CreateMetrics(L""), false, 1));
        // replica difference is 1 - we expect to add 1 primary replica anywhere
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(statefulServiceName1), 0, CreateReplicas(L""), 1));

        wstring statefulServiceName2 = L"Stateful2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(statefulServiceName2, serviceType, true, L"", CreateMetrics(L""), false, 2));
        // replica difference is 2 - we expect to add 1 primary and 1 secondary replica anywhere
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(statefulServiceName2), 0, CreateReplicas(L""), 2));
        // replica difference is 1 - we expect to add 1 secondary replica into the domain 1 or 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(statefulServiceName2), 0, CreateReplicas(L"P/0"), 1));

        wstring statelessServiceName1 = L"Stateless1";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(statelessServiceName1, serviceType, false, L"", CreateMetrics(L""), false, 1));
        // replica difference is 1 - we expect to add 1 secondary replica anywhere
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(statelessServiceName1), 0, CreateReplicas(L""), 1));

        wstring statelessServiceName2 = L"Stateless2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(statelessServiceName2, serviceType, false, L"", CreateMetrics(L""), false, 2));
        // replica difference is 2 - we expect to add 2 secondary replica anywhere
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(statelessServiceName2), 0, CreateReplicas(L""), 2));
        // replica difference is 1 - we expect to add 1 secondary replica into the domain 1 or 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(statelessServiceName2), 0, CreateReplicas(L"I/0"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary *", value)));

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add * *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 2|3", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add instance *", value)));

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"4 add instance *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add instance 2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityViolationTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithAffinityViolationTest");
        PLBConfigScopeChange(AffinityConstraintPriority, int, 1);

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(PlacementWithCapacityViolationTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithCapacityViolationTest");
        PLBConfigScopeChange(CapacityConstraintPriority, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/4000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/6000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/6000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/4000"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/4000"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"MyMetric/1.0/5000/5000")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithCapacityViolationNewPartitionTest)
    {
        wstring testName = L"PlacementWithCapacityViolationNewPartitionTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(CapacityConstraintPriority, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/10"));

        wstring testType = wformatString("{0}Type", testName);
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(service1Name, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));
        plb.UpdateService(CreateServiceDescription(service2Name, testType, true, CreateMetrics(L"MyMetric/1.0/5/5")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"S/0"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        // Node 0 is already over capacity, but that should not stop us from adding to Node1.
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 0", value)));
    }


    BOOST_AUTO_TEST_CASE(PlacementWithDirectionalAffinityAndBlockList)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDirectionalAffinityAndBlockList");
        // Relax affinity so that FailoverUnit(1) secondary can be placed
        PLBConfigScopeChange(AffinityConstraintPriority, int, 1);
        // Enable moving parent replicas so that parent FailoverUnit(0) can move secondary replica with child
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ParentType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ChildType"), set<NodeId>(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"ParentType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"ChildType", true, L"TestService1", vector<ServiceMetric>()));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 3", value)));

        // Do refresh a second time to trigger constraint check
        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 2=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveTest: move existing replica for placement");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 5 nodes with capaticy 10
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/9/9")));

        // Each node has two replica and load of 4, both replica need to be moved out to place the big replica of load 9
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMovementWithLimitedMaxPercentageToMoveTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMovementWithLimitedMaxPercentageToMoveTest");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.1);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 5 nodes with capaticy 10
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/9/9")));

        // Each node has two replica and load of 4, both replica need to be moved out to place the big replica of load 9
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove....
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacementWithMove");
        VERIFY_ARE_EQUAL(0, actionList.size());


        PLBConfigScopeModify(MaxPercentageToMoveForPlacement, 1.0);
        fm_->RefreshPLB(Stopwatch::Now());
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacementWithMove");
        VERIFY_ARE_NOT_EQUAL(0, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveThrottleTestNegative)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveThrottleTestNegative");
        // Throttle on 1 action based on absolute placement movement throttle threshold.
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 1);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double , 0.0);

        PlacementWithExistingReplicaMoveThrottleTestHelper(false);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveThrottleTestPositive)
    {
        // Test case: Placement with move cannot be execute because of absolute threshold
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveThrottleTestPositive");
        // 2 replicas can be moved - enough for placement
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 2);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double, 0.0);

        PlacementWithExistingReplicaMoveThrottleTestHelper(true);
    }


    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMovePercentageThrottleTestNegative)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMovePercentageThrottleTest");
        // Throttle on 1 action based on global movement percentage throttle threshold (10% of 10 replicas).
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double, 0.1);

        PlacementWithExistingReplicaMoveThrottleTestHelper(false);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMovePercentageThrottleTestPositive)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMovePercentageThrottleTest");
        // 20% of replicas (2) can be moved - enough to execute placement with move
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double, 0.2);

        PlacementWithExistingReplicaMoveThrottleTestHelper(true);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveGlobalThrottleTestNegative)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveGlobalThrottleTestNegative");
        // Throttle on 1 action based on absolute placement movement throttle threshold.
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 1);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.0);

        PlacementWithExistingReplicaMoveThrottleTestHelper(false);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveGlobalThrottleTestPositive)
    {
        // Test case: Placement with move cannot be execute because of absolute threshold
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveGlobalThrottleTestPositive");
        // 2 replicas can be moved - enough for placement
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 2);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.0);

        PlacementWithExistingReplicaMoveThrottleTestHelper(true);
    }


    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMovePercentageGlobalThrottleTestNegative)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMovePercentageGlobalThrottleTestNegative");
        // Throttle on 1 action based on global movement percentage throttle threshold (10% of 10 replicas).
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.1);

        PlacementWithExistingReplicaMoveThrottleTestHelper(false);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMovePercentageGlobalThrottleTestPositive)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMovePercentageGlobalThrottleTestPositive");
        // 20% of replicas (2) can be moved - enough to execute placement with move
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.2);

        PlacementWithExistingReplicaMoveThrottleTestHelper(true);
    }
    BOOST_AUTO_TEST_CASE(PlacementAndBalancingWithExistingReplicaMoveThrottleTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementAndBalancingWithExistingReplicaMoveThrottleTest");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 7 nodes with capaticy 10
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        wstring appName1 = L"SampleApplication";
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            10));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/9/9")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType2", appName1, true, CreateMetrics(L"MyMetric2/1.0/9/9")));

        // Each node has two replica and load of 4, both replica need to be moved out to place the big replica of load 9
        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/4, S/3"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(5),
            FailoverUnitDescription(CreateGuid(5), wstring(L"TestService3"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(6),
            FailoverUnitDescription(CreateGuid(6), wstring(L"TestService3"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0)));

        fm_->UpdatePlb();
        StopwatchTime now = Stopwatch::Now();
        now += PLBConfig::GetConfig().MinLoadBalancingInterval + PLBConfig::GetConfig().BalancingDelayAfterNewNode;
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(actionList.size() > 0);

        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Refresh again and the scheduler action should be Creation, since CWM is throttled....
        now += PLBConfig::GetConfig().MinPlacementInterval;
        now = max(now, Stopwatch::Now());
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveMultiNewReplicasTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveMultiNewReplicasTest: move existing replica for placement");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 5 nodes with capaticy 10
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/20"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/19/19")));

        // Each node has two replica and load of 4, both replica need to be moved out to place the big replica of load 9
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/4, S/3"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add primary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveNonSolutionTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithExistingReplicaMoveNonSolutionTest: TargetReplicaSet size greater than the number of nodes");
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 5 nodes with capaticy 10
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 1));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveAndDefragAndCapacityViolationTest)
    {
        wstring testName = L"PlacementWithExistingReplicaMoveAndDefragAndCapacityViolationTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

        std::wstring defragMetric = L"MyMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        for (int i = 0; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        for (int i = 1; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/95/95", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L""), 1));

        // Node0 is over capacity - 5 replicas with load 10 and 1 replica with load 60
        // Other 4 nodes contain 1 replica each, load 10.
        // New service with load 95 should be successfully created on one of the last 4 nodes.
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"10 add primary *", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"10 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExistingReplicaMoveAndDefragAndCapacityViolationAndNoChainViolationsTest)
    {
        wstring testName = L"PlacementWithExistingReplicaMoveAndDefragAndCapacityViolationAndNoChainViolationsTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

        std::wstring defragMetric = L"MyMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        for (int i = 0; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        for (int i = 1; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/95/95", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L""), 1));

        // Node0 is over capacity - 5 replicas with load 10 and 1 replica with load 60
        // Other 4 nodes contain 1 replica each, load 10.
        // New service with load 95 should NOT be successfully created as we prevented intemediate overcommits.
        // As constraint check is disabled, although node 0 is over capacity it should not be fixed in this phase.
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithNoneRoleReplicasTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithNoneRoleReplicasTest: there is down but not dropped replica");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/r2"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/r2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"N/0, P/2, S/4"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithDynamicPlacementConstraintTest1)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDynamicPlacementConstraintTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // Start with a impossible placement constraint
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^ fd:/dc1000"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // check if any is placed for an impossible placement constraint
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Update to DC0
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^ fd:/dc0"));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        // 4 primary should be placed on node 0 (DC0)
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"* add primary 0", value)));

        fm_->Clear();
        // Update it back to impossible placement constraint
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^ fd:/dc1000"));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Update to DC1, both primary and secondary can be placed
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"FaultDomain ^ fd:/dc1"));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDynamicNodePropertyTest1)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDynamicNodePropertyTest1");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        map<wstring, wstring> nodeProperties;
        nodeProperties[L"foo"] = L"a";
        nodeProperties[L"bar"] = L"b";
        // intial node has property foo == a
        plb.UpdateNode(NodeDescription(CreateNodeInstance(0, 0), true, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(nodeProperties), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), map<wstring, uint>()));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        fm_->RefreshPLB(Stopwatch::Now());
        // service require foo == c
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"foo ^ c"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // no replica can be placed because foo == c is not satisfied
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // change node property to foo == c
        nodeProperties[L"foo"] = L"c";
        plb.UpdateNode(NodeDescription(CreateNodeInstance(0, 1), true, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(nodeProperties), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), map<wstring, uint>()));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        // one replica is placed
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDynamicBlockListTest1)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDynamicBlockListTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));


        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        //replica cannot be placed because of block list
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // clearn block list dynanmically
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        fm_->RefreshPLB(Stopwatch::Now());

        // replica now can be placed
        vector<wstring> actionList2 = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList2.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList2, ActionMatch(L"* add * 0", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithFaultDomainConstraint)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "DroppingWithFaultDomainConstraint");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc2/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0,S/0,S/1,S/2,S/3"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // There has to be exactly one drop from node 0
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* drop instance 0", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithFaultAndUpgradeDomainConstraint)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "DroppingWithFaultDomainConstraint");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"rack0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"rack1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(2, L"rack2", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(3, L"rack3", L"ud0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"S/0,S/1,S/2,S/3"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // There must be exactly one drop from node 0 or node 3
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0|3", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMultipleConstraints)
    {
        wstring testName = L"DroppingWithFaultDomainAndAffinityConstraints";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        // FaultDomain is soft constraint, while affinity is hard
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 1);
        PLBConfigScopeChange(AffinityConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd:/dc2"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd:/dc2"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(wformatString("{0}_ParentService", testName), wstring(serviceTypeName), true));

        // There are 5 replicas, and one extra replica. FD constraint will not allow us to drop from node 4.
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wformatString("{0}_ParentService", testName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), -1));

        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            wformatString("{0}_ChildService", testName), wstring(serviceTypeName), true, wformatString("{0}_ParentService", testName)));

        // Child service has 4 replicas, nothing to drop. The only non-affinitized replica of the parent is on node 4
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wformatString("{0}_ChildService", testName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // FD constraint will allow drops only from nodes 0 - 3. Affinity constraint will allow only from node 4.
        // After failure, searcher should attempt only with hard constraints (affinity) and drop from node 4.
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMultiPartitionedAffinity)
    {
        wstring testName = L"DroppingWithMultiPartitionedAffinity";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(AffinityConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(wformatString("{0}_ParentService", testName), wstring(serviceTypeName), true));

        // There are 3 replicas, and one extra replica.
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wformatString("{0}_ParentService", testName), 0, CreateReplicas(L"P/0,S/1,S/2"), -1));

        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            wformatString("{0}_ChildService", testName), wstring(serviceTypeName), true, wformatString("{0}_ParentService", testName)));

        // Child partitions have 1 replica each. Only parent replica that can be dropped is from node 2.
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wformatString("{0}_ChildService", testName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wformatString("{0}_ChildService", testName), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wformatString("{0}_ChildService", testName), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndNodeBlocklistConstraint)
    {
        wstring testName = L"DroppingWithMoveInProgressAndNodeBlocklistConstraint";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        set<NodeId> blockList;
        for (int i = 0; i < 9; i++)
        {
            if (i % 3 == 0)
            {
                blockList.insert(CreateNodeId(i));
            }
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), move(blockList)));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0/V,S/1,S/2"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/3,S/4/V,S/5"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/6/V,S/7/V,S/8"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // Replicas from blocklisted nodes could be chosen for drop
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop instance 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndPlacementConstraint)
    {
        wstring testName = L"DroppingWithMoveInProgressAndPlacementConstraint";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 9; i++)
        {
            map<wstring, wstring> nodeProperties;
            if (i % 3 == 0)
            {
                nodeProperties.insert(make_pair(L"A", L"0"));
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
            }
            else
            {
                nodeProperties.insert(make_pair(L"B", L"0"));
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
            }
        }

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceTypeName, false, L"B==0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0/V,S/1,S/2"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceTypeName, false, L"B==0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/3,S/4/V,S/5"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceTypeName, false, L"B==0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/6/V,S/7/V,S/8"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // PLB should drop replicas without creating new violations
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop instance 6", value)));
    }

    BOOST_AUTO_TEST_CASE(MultipleDropsWithMoveInProgressAndPlacementConstraint)
    {
        wstring testName = L"MultipleDropsWithMoveInProgressAndPlacementConstraint";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Nodes 0, 1 and 2 have placement property A == 0
        // Nodes 3 and 4 have property A == 1
        for (int i = 0; i < 5; i++)
        {
            map<wstring, wstring> nodeProperties;
            if (i <= 2)
            {
                nodeProperties.insert(make_pair(L"A", L"0"));
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
            }
            else
            {
                nodeProperties.insert(make_pair(L"A", L"1"));
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
            }
        }

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Service will have placement constraint set to "A==1"
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceTypeName, false, L"A==1"));

        // Instances on nodes 0, 1 and 2 are violating placement constraints.
        // Instance on node 2 is MoveInProgress at the same time
        // Replica diff is -2: We should drop from node 2 (V flag and placement constraint), and one from nodes 0 or 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1,S/2/V,S/3,S/4"), -2));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(MultipleDropsWithMoveInProgressAndPlacementConstraintWithDomains)
    {
        wstring testName = L"MultipleDropsWithMoveInProgressAndPlacementConstraintWithDomains";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Nodes 0, 1 and 2 have placement property A == 0
        // Nodes 3 and 4 have property A == 1
        for (int i = 0; i < 5; i++)
        {
            map<wstring, wstring> nodeProperties;
            if (i <= 2)
            {
                nodeProperties.insert(make_pair(L"A", L"0"));
            }
            else
            {
                nodeProperties.insert(make_pair(L"A", L"1"));
            }

            plb.UpdateNode(CreateNodeDescription(i, wformatString("dc0/r{0}", i), wformatString("dc0/r{0}", i % 2), move(nodeProperties)));
        }

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Service will have placement constraint set to "A==1"
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceTypeName, false, L"A==1"));

        // Instances on nodes 0, 1 and 2 are violating placement constraints.
        // Instance on node 2 is MoveInProgress at the same time
        // Replica diff is -2: We should drop from node 2 (V flag and placement constraint), and one from nodes 0 or 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1,S/2/V,S/3,S/4"), -2));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndFaultDomainAndReplicaExclusionConstraint)
    {
        wstring testName = L"DroppingWithMoveInProgressAndFaultDomainAndReplicaExclusionConstraint";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/0/V,S/1,S/2"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/0,S/1/V,S/2"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/0/V,S/1/V,S/2"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // As by design dropping replicas should aim not to create new violations, partition 1 can drop replica on node 1
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|2 drop instance 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 1", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndFaultAndUpgradeDomainConstraint)
    {
        wstring testName = L"DroppingWithMoveInProgressAndFaultAndUpgradeDomainConstraint";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"rack0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"rack1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(2, L"rack2", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(3, L"rack3", L"ud0"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1,S/2,S/3/V"), -1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0/V,S/1,S/2,S/3"), -1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 0", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressReplicaInParentServiceAndAffinityConstraints)
    {
        wstring testName = L"DroppingWithMoveInProgressReplicaInParentServiceAndAffinityConstraints";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        // FaultDomain is soft constraint, while affinity is hard
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.UpdateNode(CreateNodeDescription(4));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, wstring(serviceTypeName), true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3/V"), -1));
        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, wstring(serviceTypeName), true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wformatString(childServiceName), 0, CreateReplicas(L"P/0,S/2"), 0));

        // PLB should prefer to drop MoveInProgress parent replica on node 3, rather than secondary parent replica on node 1
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressReplicaInChildServiceAndAffinityConstraints)
    {
        wstring testName = L"DroppingWithMoveInProgressReplicaInChildServiceAndAffinityConstraints";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        // FaultDomain is soft constraint, while affinity is hard
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.UpdateNode(CreateNodeDescription(4));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, wstring(serviceTypeName), true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), -1));
        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, wstring(serviceTypeName), true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wformatString(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/2/V,S/4/B"), 0));

        // PLB should prefer to drop replica on node 3.
        // It should not drop parent service replica where child service replica is in MoveInProgress or InBuild.
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndUpgradeAndFaultDomainAndAffinityTest)
    {
        wstring testName = L"DroppingWithMoveInProgressAndUpgradeAndFaultDomainAndAffinityTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"fd/0", L"0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd/1", L"1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd/0", L"2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd/1", L"3", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd/0", L"4", L""));

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd/1", L"0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"fd/0", L"1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(7, L"fd/1", L"2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(8, L"fd/0", L"3", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(9, L"fd/1", L"4", L""));

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(10, L"fd/1", L"1", L""));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(10));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), move(blockList)));

        int index = 0;
        wstring parentServiceName;
        parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"S/0,P/1,S/2,S/4,S/5,S/7,S/8"), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(L"S/0,P/1,S/4/B,S/7,S/8,S/10/V"), -1));

        // PLB should drop child service's replica from node 10
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 10", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithMoveInProgressAndFaultDomainAndAffinityConstraints)
    {
        wstring testName = L"DroppingWithMoveInProgressAndFaultDomainAndAffinityConstraints";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        // FaultDomain is soft constraint, while affinity is hard
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 1);
        PLBConfigScopeChange(AffinityConstraintPriority, int, 0);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd:/dc2"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd:/dc2"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        // Parent service: There are 5 replicas, and one extra replica.
        //                 Although FD constraint should not drop replica on node it should be dropped according to affinity.
        //                 Replica on node 4 is marked with MoveInProgress.
        // Child service:  There are 4 replicas, nothing to drop. The only non-affinitized replica of the parent is on node 4
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, wstring(serviceTypeName), true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4/V"), -1));
        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, wstring(serviceTypeName), true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wformatString(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        // The same pair of services only parent's replica on node 3 is marked with MoveInProgress
        parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, wstring(serviceTypeName), true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3/V,S/4"), -1));
        childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, wstring(serviceTypeName), true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wformatString(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        // The same pair of services only parent's replica on both nodes 3 and 4 are marked with MoveInProgress.
        parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, wstring(serviceTypeName), true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3/V,S/4/V"), -1));
        childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, wstring(serviceTypeName), true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wformatString(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // FD constraint will allow drops only from nodes 0 - 3. Affinity constraint will allow only from node 4.
        // After failure, searcher should attempt only with hard constraints (affinity) and drop from node 4 - in all three cases.
        // First PLB run cannot resolve partition 2 as we have some operations for 1 and 4 partitions with contratint check priority 2.
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|4 drop secondary 4", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(wformatString("{0}{1}", testName, 0)), 1, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(wformatString("{0}{1}", testName, 4)), 1, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime + PLBConfig::GetConfig().MinPlacementInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 drop secondary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(DropOneInstanceFromEachDomain)
    {
        wstring testName = L"DropOneInstanceFromEachDomain";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        // FaultDomain is soft constraint, while affinity is hard
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd:/dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd:/dc2"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd:/dc2"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));

        // FailoverUnit will have 6 instance on 6 nodes. We want to drop 3 of them.
        // We need to make sure that we will not drop more than 1 replica from each FD, otherwise we will create FD violation.
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1,S/2,S/3,S/4,S/5"), -3));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 0|1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 2|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithFailoverUnitInUpgradeTest)
    {
        wstring testName = L"DroppingWithFailoverUnitInUpgradeTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"rack0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"rack1", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(2, L"rack2", L"ud2"));
        plb.UpdateNode(CreateNodeDescription(3, L"rack3", L"ud3"));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/V,S/3/D"), -1, upgradingFlag_));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/1,S/2/V,S/3/D"), -1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 drop instance 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop instance 2", value)));
    }

    BOOST_AUTO_TEST_CASE(DroppingWithDeactivatingNodesTest)
    {
        wstring testName = L"DroppingWithDeactivatingNodesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        int nodeId = 0;
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId++, L"", L""));
        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId++, L"", L"",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring replicas = L"P/0";
        for (int i = 1; i < nodeId; ++i)
        {
            replicas.append(wformatString(",S/{0}", i));
        }

        int index = 0;
        // Statefull service
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(replicas), -nodeId));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Nodes:
        // NodeId   Dropping      DeactivationIntent  DeactivationStatus
        // 0        Primary       None                None
        // 1        Allow         None                DeactivationSafetyCheckInProgress
        // 2        Don�t Allow   Pause               DeactivationSafetyCheckInProgress
        // 3        Allow         Restart             DeactivationSafetyCheckInProgress
        // 4        Allow         RemoveData          DeactivationSafetyCheckInProgress
        // 5        Allow         RemoveNode          DeactivationSafetyCheckInProgress
        // 6        Allow         None                DeactivationSafetyCheckComplete
        // 7        Don�t Allow   Pause               DeactivationSafetyCheckComplete
        // 8        Allow         Restart             DeactivationSafetyCheckComplete
        // 9        Allow         RemoveData          DeactivationSafetyCheckComplete
        // 10       Allow         RemoveNode          DeactivationSafetyCheckComplete
        // 11       Allow         None                DeactivationComplete
        // 12       Don't Allow   Pause               DeactivationComplete
        // 13       Allow         Restart             DeactivationComplete
        // 14       Allow         RemoveData          DeactivationComplete
        // 15       Allow         RemoveNode          DeactivationComplete
        // 16       Allow         None                ActivationInProgress
        // 17       Don�t Allow   Pause               ActivationInProgress
        // 18       Allow         Restart             ActivationInProgress
        // 19       Allow         RemoveData          ActivationInProgress
        // 20       Allow         RemoveNode          ActivationInProgress
        // 21       Allow         None                None
        // 22       Don�t Allow   Pause               None
        // 23       Allow         Restart             None
        // 24       Allow         RemoveData          None
        // 25       Allow         RemoveNode          None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Only secondary replicas from Paused nodes should not be dropped
        VERIFY_ARE_EQUAL(20u, actionList.size());
        VERIFY_ARE_EQUAL(20, CountIf(actionList, ActionMatch(L"0 drop secondary 1|3|4|5|6|8|9|10|11|13|14|15|16|18|19|20|21|23|24|25", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithSeparateSecondaryLoadTest1)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithSeparateSecondaryLoadTest1");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"PlacementWithSeparateSecondaryLoadTest1ServiceType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"PlacementWithSeparateSecondaryLoadTest1Service0",
            L"PlacementWithSeparateSecondaryLoadTest1ServiceType", true, CreateMetrics(L"MyMetric/1.0/20/1")));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithSeparateSecondaryLoadTest1Service1",
            L"PlacementWithSeparateSecondaryLoadTest1ServiceType", true, CreateMetrics(L"MyMetric/1.0/10/1")));
        plb.UpdateService(CreateServiceDescription(L"PlacementWithSeparateSecondaryLoadTest1Service2",
            L"PlacementWithSeparateSecondaryLoadTest1ServiceType", true, CreateMetrics(L"MyMetric/1.0/10/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PlacementWithSeparateSecondaryLoadTest1Service0"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"PlacementWithSeparateSecondaryLoadTest1Service1"),
            0, CreateReplicas(L"P/1, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"PlacementWithSeparateSecondaryLoadTest1Service2"),
            0, CreateReplicas(L""), 1));

        // Loads initially is
        // 20, 1, 1
        // 1, 10
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(2), 21));
        // Loads after upddate is:
        // 20, 1, 21
        // 1, 10
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"PlacementWithSeparateSecondaryLoadTest1Service0", L"MyMetric", 20, loads));
        // After refresh, new replica should be placed on node 1 to make the load balance

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDefragTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithDefragTest");
        wstring defragMetric = L"DefragMetric";

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 300);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 9; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc0/r{0}", i % 3), wformatString("{0}/100", defragMetric)));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int index = 0;
        wstring servicName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(servicName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(servicName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        servicName = wformatString("TestService{0}", index);
        plb.UpdateService(CreateServiceDescription(servicName, L"TestType", true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(servicName), 0, CreateReplicas(L""), 3));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDefragWithCapacityViolationTest)
    {
        wstring testName = L"PlacementWithDefragWithCapacityViolationTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);

        std::wstring defragMetric = L"MyMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        for (int i = 0; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        for (int i = 1; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L""), 1));

        // Node0 is over capacity - 5 replicas with load 10 and 1 replica with load 60
        // Other 4 nodes contain 1 replica each, load 10.
        // New service with load 10 should be successfully created on one of the last 4 nodes.
        // As constraint check is disabled, although node 0 is over capacity it should not be fixed in this phase.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"10 add primary *", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"10 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithPrimaryToBeSwappedTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithPrimaryToBeSwappedTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap , nodeBufferPercentageMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // I is the flag used by FM to mark isSwapPrimary
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0/I, S/1"), 0, upgradingFlag_));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Primary should be swapped although two primary replicas on node 1 will violate NodeBufferPercentage
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithPrimaryToBePlacedTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithPrimaryToBePlacedTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // J is the flag used by FM to mark PrimaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1/J"), 0, upgradingFlag_));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Secondary replica on node 1 should be upgraded to primary although it will violate NodeBufferPercentage
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithStandByReplicaToBePlacedTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithStandByReplicaToBePlacedTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // K is the flag used by FM to mark SecondaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/40")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, SB/1/K"), 1, upgradingFlag_));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Secondary should be created on a node 1 although it will violate NodeBufferPercentage as replica diff is 1
        // Fleg K is ignored if replica diff is 0
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithDroppedReplicaToBePlacedTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithDroppedReplicaToBePlacedTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // K is the flag used by FM to mark SecondaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/40")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, D/1/K"), 1, upgradingFlag_));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Secondary should be created on a node 1 although it will violate NodeBufferPercentage as replica diff is 1
        // Fleg K is ignored if replica diff is 0
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(NodeBufferPercentageWithMissingReplicaTest)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithMissingReplicaTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // K is the flag used by FM to mark SecondaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 1, upgradingFlag_));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/80/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Secondary should be created on a node 1 even if it will violate NodeBufferPercentage
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDeactivatingNodesTest)
    {
        wstring testName = L"PlacementWithDeactivatingNodesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        int nodeId = 0;
        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId++, L"", L"",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        // Statefull service
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/20"), nodeId - 1));

        // Stateless service, not with replica on every node
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/20"), nodeId - 1));

        // Statelless service with replica on every node
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, false, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/20"), INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Nodes:
        // NodeId   Replicas   Placement           DeactivationIntent  DeactivationStatus
        // 0                   Allow (N/A)         None                DeactivationSafetyCheckInProgress
        // 1                   Allow               Pause               DeactivationSafetyCheckInProgress
        // 2                   Allow               Restart             DeactivationSafetyCheckInProgress
        // 3                   Don't Allow         RemoveData          DeactivationSafetyCheckInProgress
        // 4                   Don't Allow         RemoveNode          DeactivationSafetyCheckInProgress
        // 5                   Allow (N/A)         None                DeactivationSafetyCheckComplete
        // 6                   Allow (-1)          Pause               DeactivationSafetyCheckComplete
        // 7                   Allow (-1)          Restart             DeactivationSafetyCheckComplete
        // 8                   Don�t Allow         RemoveData          DeactivationSafetyCheckComplete
        // 9                   Don�t Allow         RemoveNode          DeactivationSafetyCheckComplete
        // 10                  Allow (N/A)         None                DeactivationComplete
        // 11                  Don't Allow         Pause               DeactivationComplete
        // 12                  Don't Allow         Restart             DeactivationComplete
        // 13                  Don't Allow         RemoveData          DeactivationComplete
        // 14                  Don't Allow         RemoveNode          DeactivationComplete
        // 15                  Allow               None                ActivationInProgress
        // 16                  Don't Allow (N/A)   Pause               ActivationInProgress
        // 17                  Don't Allow (N/A)   Restart             ActivationInProgress
        // 18                  Don't Allow (N/A)   RemoveData          ActivationInProgress
        // 19                  Don't Allow (N/A)   RemoveNode          ActivationInProgress
        // 20       P/S/I      Allow               None                None
        // 21                  Don't Allow (N/A)   Pause               None
        // 22                  Don't Allow (N/A)   Restart             None
        // 23                  Don't Allow (N/A)   RemoveData          None
        // 24                  Don't Allow (N/A)   RemoveNode          None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(20u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 15", value)));

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 0", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 1", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 2", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add instance 6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add instance 7", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 10", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add instance 15", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementStatefullVolatileTarget1WithDeactivatingNodesTest)
    {
        wstring testName = L"PlacementStatefullVolatileTarget1WithDeactivatingNodesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        plb.SetMovementEnabled(true, true);

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        int nodeId = 0;
        for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
            status <= Reliability::NodeDeactivationStatus::Enum::None;
            status++)
        {
            for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                intent++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    nodeId++, L"", L"",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
            }
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        blockList.insert(CreateNodeId(5));
        blockList.insert(CreateNodeId(10));
        blockList.insert(CreateNodeId(15));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), move(blockList)));

        int index = 0;
        // Statefull volatile service with target = 1
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/20"), 1, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Nodes:
        // NodeId   Replicas   Placement           DeactivationIntent  DeactivationStatus
        // 0                   Allow (N/A)         None                DeactivationSafetyCheckInProgress
        // 1                   Don't Allow         Pause               DeactivationSafetyCheckInProgress
        // 2                   Don't Allow         Restart             DeactivationSafetyCheckInProgress
        // 3                   Don't Allow         RemoveData          DeactivationSafetyCheckInProgress
        // 4                   Don't Allow         RemoveNode          DeactivationSafetyCheckInProgress
        // 5                   Allow (N/A)         None                DeactivationSafetyCheckComplete
        // 6                   Don't Allow         Pause               DeactivationSafetyCheckComplete
        // 7                   Don't Allow         Restart             DeactivationSafetyCheckComplete
        // 8                   Don�t Allow         RemoveData          DeactivationSafetyCheckComplete
        // 9                   Don�t Allow         RemoveNode          DeactivationSafetyCheckComplete
        // 10                  Allow (N/A)         None                DeactivationComplete
        // 11                  Don't Allow         Pause               DeactivationComplete
        // 12                  Don't Allow         Restart             DeactivationComplete
        // 13                  Don't Allow         RemoveData          DeactivationComplete
        // 14                  Don't Allow         RemoveNode          DeactivationComplete
        // 15                  Allow               None                ActivationInProgress
        // 16                  Don't Allow (N/A)   Pause               ActivationInProgress
        // 17                  Don't Allow (N/A)   Restart             ActivationInProgress
        // 18                  Don't Allow (N/A)   RemoveData          ActivationInProgress
        // 19                  Don't Allow (N/A)   RemoveNode          ActivationInProgress
        // 20       P/S/I      Allow               None                None
        // 21                  Don't Allow (N/A)   Pause               None
        // 22                  Don't Allow (N/A)   Restart             None
        // 23                  Don't Allow (N/A)   RemoveData          None
        // 24                  Don't Allow (N/A)   RemoveNode          None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithNodeDeactivationTest2)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithNodeDeactivationTest2");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties;
        nodeProperties.insert(make_pair(L"a", L"0"));

        int i = 0;
        for (; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, wformatString("dc{0}/r{1}", i%2, i/2), L"MyMetric/100"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            i, wformatString("dc{0}/r{1}", i % 2, i / 2), L"MyMetric/100", Reliability::NodeDeactivationIntent::Enum::Pause));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService0", L"TestType", true, L"FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/5"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithNodeDeactivationTest3)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithNodeDeactivationTest3");
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int i = 0;
        for (; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i, L"", L""));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            i, L"", L"", Reliability::NodeDeactivationIntent::Enum::Pause));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/3"), INT_MAX));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        for (wstring action : actionList)
        {
            wcout << action << endl;
        }
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add instance 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithClusterUpgradeRelaxAffinity)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithClusterUpgradeRelaxAffinity");

        PlacementAndLoadBalancing & plb = fm_->PLB;
        // Called during cluster upgrade
        plb.SetMovementEnabled(false, false);

        for (int i = 0; i < 3; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric2/20"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric2/10"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L"MyMetric2/1.0/10/15")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearReplicasTest)
    {
        wstring testName = L"PlacementWithShouldDisappearReplicasTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        std::wstring metric = L"MyMetric";

        for (int i = 0; i < 8; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", metric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;

        // Service with one standBy replica ToBeDroppedByFM and one MoveInProgress replica
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,SB/2/D,S/6/V"), 0));

        // Service with secondary replica ToBeDroppedByPLB and InBuild replica and one MoveInProgress replica
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3,S/4/R,I/5/B,S/7/V"), 0));

        // Service that should be created
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/90/90", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L""), 1));

        // Regular placement should not create missing replicas
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // PLB should not move ToBeDropped/MoveInProgress or InBuild replica
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"0 move * 2=>*", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 move * 4=>*", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 move * 4=>*", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"0 move * 6=>*", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 move * 7=>*", value)));

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearPrimaryTest)
    {
        wstring testName = L"PlacementWithShouldDisappearPrimaryTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/200"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        // Service with primary ToBeDroppedByPLB = R flag
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/R,S/1"), 1));

        // Service with primary MoveInTransition flag
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/V,S/1"), 1));

        // Regular placement should create new primary as current one is marked with ToBeDropped/MoveInProgress
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add primary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearPrimaryAndSwappedOutTest)
    {
        wstring testName = L"PlacementWithShouldDisappearPrimaryAndSwappedOutTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/200"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/D/I,S/1"), 1, upgradingFlag_));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/V/I,S/1"), 1, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 promote secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 promote secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearPrimaryAndSwappedOutToLowCapacityNodeTest)
    {
        wstring testName = L"PlacementWithShouldDisappearPrimaryAndSwappedOutToLowCapacityNodeTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/200"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/100"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/150/50")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/D/I,S/1"), 0, upgradingFlag_));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/150/50")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2/V/I,S/3"), 0, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        // We expect promote operations to make node capacity violation
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 promote secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 promote secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearPrimaryAndSwappedOutButNoSecondaryTest)
    {
        wstring testName = L"PlacementWithShouldDisappearPrimaryAndSwappedOutButNoSecondaryTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/D/I"), 1, upgradingFlag_));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/V/I"), 1, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));

        fm_->Clear();

        index = 0;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 1, CreateReplicas(L"P/0/D/I,S/1"), 0, upgradingFlag_));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 1, CreateReplicas(L"P/0/V/I,S/1"), 0, upgradingFlag_));

        plb.ProcessPendingUpdatesPeriodicTask();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime + PLBConfig::GetConfig().MinPlacementInterval;
        fm_->RefreshPLB(now);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 promote secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 promote secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithToBeDroppedAndOneExtraReplicaTest)
    {
        wstring testName = L"PlacementWithToBeDroppedAndOneExtraReplicaTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2/D,S/3/T"), -1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3,S/4/D"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        // ToBeDropped replicas should not be chosen for dropping extra replicas
        // FailoverUnit 0 contains 1 extra secondary replica that should be dropped
        // FailoverUnit 1 doesn't have secondary replica to be dropped
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearAndPrimaryToBeSwappedOutTest)
    {
        wstring testName = L"PlacementWithShouldDisappearAndPrimaryToBeSwappedOutTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100", metric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/I,S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/I,S/1/D,S/2/T"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/I,S/1/V"), 0));

        // PLB should not swap primary to ToBeDropped/MoveInProgress secondary
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithShouldDisappearAndPrimaryToBePlacedTest)
    {
        wstring testName = L"PlacementWithShouldDisappearAndPrimaryToBePlacedTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/J"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/D/J"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/V/J"), 0));

        // PLB should not place primary on a ToBeDropped/MoveInProgress secondary although it is marked with IsPrimaryToBePlaced (J fleg)
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithExtraReplicaTest)
    {
        wstring testName = L"PlacementWithExtraReplicaTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd1", L"ud0"));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring parentService = wformatString("{0}__Parent", testName);
        wstring childService = wformatString("{0}__Child", testName);

        wstring dummyService = wformatString("{0}__Dummy", testName);

        plb.UpdateService(CreateServiceDescription(parentService, testType, true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService, testType, true, parentService));

        plb.UpdateService(CreateServiceDescription(dummyService, testType, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(parentService), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(childService), 0, CreateReplicas(L"P/0"), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(dummyService), 0, CreateReplicas(L"P/0,S/2,S/3"), -1));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithPrimaryToBeSwappedOutAndOneExtraReplicaTest)
    {
        wstring testName = L"PlacementWithPrimaryToBeSwappedOutAndOneExtraReplicaTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));

        // Primary should be swapped out
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index), wstring(serviceName), 0, CreateReplicas(L"P/0/L/I,S/1,N/2,S/3"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 drop secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithOneParititionInUpgradeAndAnotherPartitionWithTwoExtraReplicasTest)
    {
        wstring testName = L"PlacementWithOneParititionInUpgradeAndAnotherPartitionWithTwoExtraReplicasTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        // Primary should be swapped out
        wstring inUpgradeServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(inUpgradeServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(inUpgradeServiceName), 0, CreateReplicas(L"P/0/L/I,S/1"), 0, upgradingFlag_));

        // Two secondary replicas should be dropped
        wstring extraReplicasServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(extraReplicasServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(extraReplicasServiceName), 0, CreateReplicas(L"S/0,S/1,P/2"), -2));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 drop secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeAndPreferLocationTest)
    {
        wstring testName = L"PlacementWithUpgradeAndPreferLocationTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        // Primary should be swapped out
        wstring inUpgradeServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(inUpgradeServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(inUpgradeServiceName), 0, CreateReplicas(L"P/0, D/1/K, S/2"), 1, upgradingFlag_));

        // Two secondary replicas should be added
        wstring ToBePlacedService = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(ToBePlacedService, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(ToBePlacedService), 0, CreateReplicas(L"S/0,P/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(ToBePlacedService), 0, CreateReplicas(L""), 2));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithOneParititionInUpgradeAndAnotherToBeCreatedTest)
    {
        wstring testName = L"PlacementWithOneParititionInUpgradeAndAnotherToBeCreatedTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring inUpgradeServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(inUpgradeServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(inUpgradeServiceName), 0, CreateReplicas(L"P/0,D/1/J"), 1, upgradingFlag_));

        wstring newServiceServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(newServiceServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(newServiceServiceName), 0, CreateReplicas(L""), 2));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        for (wstring action : actionList)
        {
            wcout << action << endl;
        }
        VERIFY_ARE_EQUAL(3u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithParititionInUpgradeAndOnePrimaryToBeSwappedOut)
    {
        wstring testName = L"PlacementWithParititionInUpgradeAndOnePrimaryToBeSwappedOut";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd0", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd0", L"ud0"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index(0);
        wstring inUpgradeServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(inUpgradeServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index), wstring(inUpgradeServiceName), 0, CreateReplicas(L"P/0/R/I"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        for (wstring action : actionList)
        {
            wcout << action << endl;
        }
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradePlaceReplicaBackAndFaultDomainTest)
    {
        // Specific production case that was a repro from FD violation problem that PLB was not able to fix.
        wstring testName = L"PlacementWithUpgradePlaceReplicaBackAndFaultDomainTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0,  L"fd:/0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1,  L"fd:/4", L"0"));
        plb.UpdateNode(CreateNodeDescription(2,  L"fd:/5", L"0"));
        plb.UpdateNode(CreateNodeDescription(3,  L"fd:/6", L"0"));
        plb.UpdateNode(CreateNodeDescription(4,  L"fd:/7", L"0"));
        plb.UpdateNode(CreateNodeDescription(5,  L"fd:/8", L"0"));
        plb.UpdateNode(CreateNodeDescription(6,  L"fd:/9", L"0"));
        plb.UpdateNode(CreateNodeDescription(7,  L"fd:/0", L"1"));
        plb.UpdateNode(CreateNodeDescription(8,  L"fd:/1", L"1"));
        plb.UpdateNode(CreateNodeDescription(9,  L"fd:/5", L"1"));
        plb.UpdateNode(CreateNodeDescription(10, L"fd:/6", L"1"));
        plb.UpdateNode(CreateNodeDescription(11, L"fd:/7", L"1"));
        plb.UpdateNode(CreateNodeDescription(12, L"fd:/8", L"1"));
        plb.UpdateNode(CreateNodeDescription(13, L"fd:/9", L"1"));
        plb.UpdateNode(CreateNodeDescription(14, L"fd:/0", L"2"));
        plb.UpdateNode(CreateNodeDescription(15, L"fd:/1", L"2"));
        plb.UpdateNode(CreateNodeDescription(16, L"fd:/2", L"2"));
        plb.UpdateNode(CreateNodeDescription(17, L"fd:/7", L"2"));
        plb.UpdateNode(CreateNodeDescription(18, L"fd:/8", L"2"));
        plb.UpdateNode(CreateNodeDescription(19, L"fd:/9", L"2"));
        plb.UpdateNode(CreateNodeDescription(20, L"fd:/0", L"3"));
        plb.UpdateNode(CreateNodeDescription(21, L"fd:/1", L"3"));
        plb.UpdateNode(CreateNodeDescription(22, L"fd:/2", L"3"));
        plb.UpdateNode(CreateNodeDescription(23, L"fd:/3", L"3"));
        plb.UpdateNode(CreateNodeDescription(24, L"fd:/8", L"3"));
        plb.UpdateNode(CreateNodeDescription(25, L"fd:/9", L"3"));
        plb.UpdateNode(CreateNodeDescription(26, L"fd:/0", L"4"));
        plb.UpdateNode(CreateNodeDescription(27, L"fd:/1", L"4"));
        plb.UpdateNode(CreateNodeDescription(28, L"fd:/2", L"4"));
        plb.UpdateNode(CreateNodeDescription(29, L"fd:/3", L"4"));
        plb.UpdateNode(CreateNodeDescription(30, L"fd:/4", L"4"));
        plb.UpdateNode(CreateNodeDescription(31, L"fd:/9", L"4"));
        plb.UpdateNode(CreateNodeDescription(32, L"fd:/0", L"5"));
        plb.UpdateNode(CreateNodeDescription(33, L"fd:/1", L"5"));
        plb.UpdateNode(CreateNodeDescription(34, L"fd:/2", L"5"));
        plb.UpdateNode(CreateNodeDescription(35, L"fd:/3", L"5"));
        plb.UpdateNode(CreateNodeDescription(36, L"fd:/4", L"5"));
        plb.UpdateNode(CreateNodeDescription(37, L"fd:/5", L"5"));
        plb.UpdateNode(CreateNodeDescription(38, L"fd:/0", L"6"));
        plb.UpdateNode(CreateNodeDescription(39, L"fd:/1", L"6"));
        plb.UpdateNode(CreateNodeDescription(40, L"fd:/2", L"6"));
        plb.UpdateNode(CreateNodeDescription(41, L"fd:/3", L"6"));
        plb.UpdateNode(CreateNodeDescription(42, L"fd:/4", L"6"));
        plb.UpdateNode(CreateNodeDescription(43, L"fd:/5", L"6"));
        plb.UpdateNode(CreateNodeDescription(44, L"fd:/6", L"6"));
        plb.UpdateNode(CreateNodeDescription(45, L"fd:/1", L"7"));
        plb.UpdateNode(CreateNodeDescription(46, L"fd:/2", L"7"));
        plb.UpdateNode(CreateNodeDescription(47, L"fd:/3", L"7"));
        plb.UpdateNode(CreateNodeDescription(48, L"fd:/4", L"7"));
        plb.UpdateNode(CreateNodeDescription(49, L"fd:/5", L"7"));
        plb.UpdateNode(CreateNodeDescription(50, L"fd:/6", L"7"));
        plb.UpdateNode(CreateNodeDescription(51, L"fd:/7", L"7"));
        plb.UpdateNode(CreateNodeDescription(52, L"fd:/2", L"8"));
        plb.UpdateNode(CreateNodeDescription(53, L"fd:/3", L"8"));
        plb.UpdateNode(CreateNodeDescription(54, L"fd:/4", L"8"));
        plb.UpdateNode(CreateNodeDescription(55, L"fd:/5", L"8"));
        plb.UpdateNode(CreateNodeDescription(56, L"fd:/6", L"8"));
        plb.UpdateNode(CreateNodeDescription(57, L"fd:/7", L"8"));
        plb.UpdateNode(CreateNodeDescription(58, L"fd:/8", L"8"));
        plb.UpdateNode(CreateNodeDescription(59, L"fd:/3", L"9"));
        plb.UpdateNode(CreateNodeDescription(60, L"fd:/4", L"9"));
        plb.UpdateNode(CreateNodeDescription(61, L"fd:/5", L"9"));
        plb.UpdateNode(CreateNodeDescription(62, L"fd:/6", L"9"));
        plb.UpdateNode(CreateNodeDescription(63, L"fd:/7", L"9"));
        plb.UpdateNode(CreateNodeDescription(64, L"fd:/8", L"9"));
        plb.UpdateNode(CreateNodeDescription(65, L"fd:/9", L"9"));
        wstring testType = wformatString("{0}{1}Type", testName, 0);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0,
            CreateReplicas(L"P/42,S/20,S/25,S/34,S/47,S/35,SB/12/K,S/37,S/0,S/30,S/55,S/16,S/17,S/15,S/63,S/27,S/56,S/1,S/62,S/51,S/50,S/58,S/24,S/65,S/53,S/26,S/40,S/44,S/21,S/2,SB/13/K,SB/11/K"),
            3, upgradingFlag_));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 11", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 12", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 13", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithNewReplicaToUnderloadedDomainTest)
    {
        wstring testName = L"PlacementWithNewReplicaToUnderloadedDomainTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        wstring myMetric = L"myMetric";
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"fd:/0", wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"fd:/0", wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"fd:/1", wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"fd:/1", wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"fd:/2", wformatString("{0}/50", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"fd:/2", wformatString("{0}/50", myMetric)));

        wstring testType = wformatString("{0}{1}Type", testName, 0);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 1));

        // New replica shouldn't be added to the domain 1 (with 1 replica) when we already have domain violation (domain 0 has 2 replicas)
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationScaleoutCountTest1)
    {
        wstring testName = L"PlacementWithApplicationScaleoutCountTest1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));


        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/1"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L""), 2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/0"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/2, S/0"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L""), 3));


        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(10u, actionList.size());

        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* add * 3", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* add * 4", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationScaleoutCountTest2)
    {
        // Testing when add new service and partition for a existing application, the placement can follow the scaleout rule
        // Two services has different metrics and they would be in the same SD
        wstring testName = L"PlacementWithApplicationScaleoutCountTest2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 3;
        for (int i = 0; i < appScaleoutCount; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        for (int i = appScaleoutCount; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/1"));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));

        wstring serviceWithScaleoutCount1 = wformatString("{0}_Service{1}", testName, L"1");
        // service1 can only be placed on node 0, 1 and 2 because of the node capacities
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/9/9")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"0 add * 0|1|2", value)));

        fm_->Clear();

        wstring serviceWithScaleoutCount2 = wformatString("{0}_Service{1}", testName, L"2");
        // service1 can be placed on any node because no capacity defined
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"DummyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"1 add * 0|1|2", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationScaleoutCountTest3)
    {
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

        // two services have different metrics, but they should be in same domain due to application
        // TODO: testing application upgrade from no scaleout
        wstring testName = L"PlacementWithApplicationScaleoutCountTest3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 3;
        for (int i = 0; i < appScaleoutCount; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/1"));
        }

        for (int i = appScaleoutCount; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));

        wstring serviceWithScaleoutCount1 = wformatString("{0}_Service{1}", testName, L"1");
        // service1 can only be placed on node 0, 1 and 2 because of the node capacities
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/3, S/4"), 0));

        wstring serviceWithScaleoutCount2 = wformatString("{0}_Service{1}", testName, L"2");
        // service2 can be placed on any node because no capacity defined
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/3, S/4"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 add * 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationScaleoutViolation)
    {
        // We will ask for placing new replicas of a partition that is violating scaleout.
        wstring testName = L"PlacementWithApplicationScaleoutViolation";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));


        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, appName1, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithWithApplicationCapacityTest1)
    {
        // Basic application capacity placement scenario test
        wstring testName = L"PlacementWithWithApplicationCapacityTest1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 2;
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/20/10/0")));

        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/6/2")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/3/3")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1"), 0));
        // 1 should be place don node 0 and 1 by the scaleout count and node load would be [8, 8]
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1 add * 0|1", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount1), 1, CreateReplicas(L"P/1, S/0"), 0));
        // 2 can't be placed since it is over capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(PlacementWithWithApplicationCapacityTest2)
    {
        // PerNode capacity is fine, but app capacity is violated, replica won't be placed
        wstring testName = L"PlacementWithWithApplicationCapacityTest2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 2;
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/20/10/0")));

        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/8/4")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0"), 0));
        // node load would be [16, 4], 2 can't be placed since it is over total app capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithWithApplicationCapacityTest3)
    {
        // Multiple application placement scenario test
        wstring testName = L"PlacementWithWithApplicationCapacityTest3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 3;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/30/10/0")));

        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/3/3")));

        wstring appName2 = wformatString("{0}Application2", testName);
        appScaleoutCount = 2;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName2), appScaleoutCount, CreateApplicationCapacities(L"MyMetric/20/10/0")));

        wstring serviceWithScaleoutCount3 = wformatString("{0}{1}ScaleoutCount", testName, L"3");
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount3, testType, appName2, true, CreateMetrics(L"MyMetric/1.0/2/2")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L""), 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceWithScaleoutCount3), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationCapacityAndDisappearingReplicas)
    {
        wstring testName = L"PlacementWithApplicationCapacityAndDisappearingReplicas";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 2;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        wstring applicationName = wformatString("{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L"ItsFoggyOutsideMetric/20/10/0"),
            appScaleoutCount));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, CreateMetrics(L"ItsFoggyOutsideMetric/1.0/10/10")));

        // There are 2 FUs of this service, both with a single primary (TBD and MoveInProgress) on node 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/1/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1/D"), 0));

        // one new FailoverUnit with replica diff 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L""), 2));


        // Scaleout count == 2, MaxNodeCapacity == 10 - NO VIOLATION:
        // Node0: Load: 0, Dissapearing load: 0
        // Node1: Load: 0, Dissapearing load: 20
        fm_->RefreshPLB(Stopwatch::Now());

        // One replica should be placed on node 0, nothing on node 1
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add * 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationAndClusterWideService)
    {
        wstring testName = L"PlacementWithApplicationAndClusterWideService";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring applicationName = wformatString("{0}_Application", testName);
        int appScaleoutCount = 3;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(applicationName), appScaleoutCount, map<std::wstring, ApplicationCapacitiesDescription>()));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // Application already has replicas on nodes 0, 1 and 2
        wstring statefulService = wformatString("{0}_StatefulService", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(statefulService, testType, applicationName, true, CreateMetrics(L"")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(statefulService), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(testType),
            wstring(applicationName),
            false,          // isStateful
            wstring(L""),   // placementConstraints
            wstring(L""),   // affinitizedService
            true,           // alignedAffinity
            std::vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            true,           // onEveryNode
            1,              // partitionCount
            -1));           // targetReplicaSetSize

        // We need to place 3 instances
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L""), INT_MAX));
        fm_->RefreshPLB(Stopwatch::Now());

        // Place 3 instances, none of them to node 4
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 add instance 4", value)));
        fm_->Clear();

        // 3 instances placed
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 1, CreateReplicas(L"I/0, I/1, I/2"), INT_MAX));

        // Now make sure that no placement happens.
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementALotOfParallelApplicationUpdatesTest)
    {
        wstring testName = L"PlacementALotOfParallelApplicationUpdatesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        int appsPerThread = 100;
        int numberOfThreads = 100;

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        Common::atomic_long failedQueryCount(0);

        auto ParallelApplicationUpdates = [&](int appScaleoutCount, int fuVersion) -> void
        {
            // application creation
            vector<Common::TimerSPtr> creationThreads;
            for (int threadId = 0; threadId < numberOfThreads; ++threadId)
            {

                creationThreads.push_back(Common::Timer::Create(
                    TimerTagDefault,
                    [testName, appsPerThread, threadId, appScaleoutCount, &plb, &failedQueryCount](Common::TimerSPtr const&) mutable
                {
                    for (int i = 0; i < appsPerThread; ++i)
                    {
                        wstring applicationName = wformatString("{0}_Application_thread{1}_{2}", testName, threadId, i);
                        // App capacity: [total, perNode, reserved]
                        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                            wstring(applicationName), appScaleoutCount, map<std::wstring, ApplicationCapacitiesDescription>()));

                        // Query for application load to put more stress on locking.
                        ServiceModel::ApplicationLoadInformationQueryResult applicationLoadQueryResult;
                        Common::ErrorCode errorCode = plb.GetApplicationLoadInformationResult(applicationName, applicationLoadQueryResult);
                        if (!errorCode.IsSuccess())
                        {
                            Trace.WriteError("PLBPlacementTestSource", "Failed to query load information for {0}, ErrorCode = {1}",
                                applicationName, errorCode);
                            failedQueryCount++;
                        }
                    }
                },false));
            }

            for (int threadId = 0; threadId < numberOfThreads; ++threadId)
            {
                //we want the thread to execute exactly once...
                creationThreads[threadId]->SetCancelWait();
                creationThreads[threadId]->Change(TimeSpan::Zero, TimeSpan::MaxValue);
            }

            VERIFY_ARE_EQUAL(failedQueryCount.load(), 0);

            for (int i = 0; i < numberOfThreads * appsPerThread; ++i)
            {
                wstring statefulService = wformatString("{0}_StatefulService_{1}", testName, i);
                wstring applicationName = wformatString("{0}_Application_thread{1}_{2}", testName, (i % 3), i);
                plb.UpdateService(CreateServiceDescriptionWithApplication(
                    statefulService, testType, applicationName, true, CreateMetrics(L"")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(i), wstring(statefulService), fuVersion, CreateReplicas(L"P/0,S/1"), 0));
            }
            //wait for all threads to finish...
            for (int threadId = 0; threadId < numberOfThreads; ++threadId)
            {
                creationThreads[threadId]->Cancel();
            }
        };

        ParallelApplicationUpdates(3, 0);
        ParallelApplicationUpdates(2, 1);

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationReservationCapacityAndOutsideService)
    {
        // Application will have no services, but will have reserved capacity so that outside service cannot be placed
        wstring testName = L"PlacementWithApplicationReservationCapacityAndOutsideService";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        // Need nodes in order to be able to update application
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 2;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            CreateApplicationCapacities(L"CPU/200/100/100"),
            appScaleoutCount,       // MinimumNodes == 2
            appScaleoutCount));     // MaximumNodes == 2

        fm_->RefreshPLB(Stopwatch::Now());

        wstring serviceWithoutScaleout = wformatString("{0}_RegularService", testName);
        ErrorCode error = plb.UpdateService(ServiceDescription(
            wstring(serviceWithoutScaleout),
            wstring(testType),
            wstring(L""),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(L""),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"CPU/1.0/301/301"),
            FABRIC_MOVE_COST_LOW,
            false,              // on every node
            1,                  // partition count
            1));                // target replica set size

        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationAndOutsideServicePositiveNegative)
    {
        // 2 nodes in the cluster, capacity of each node is 100 for metric CPU
        // Application will have services with load such that additional service can't be placed.
        // Then, we will adjust the load to be able to place new service
        wstring testName = L"PlacementWithApplicationAndOutsideServicePositiveNegative";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // 2 nodes, total capacity == 200
        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 2;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            CreateApplicationCapacities(L""),
            0,       // MinimumNodes == 0
            appScaleoutCount));     // MaximumNodes == 2

        // Add one service for the application and one partition with total load of 100!
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_ApplicationService", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName),
            testType,
            wstring(appName1),
            true, CreateMetrics(L"CPU/1.0/100/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        wstring serviceWithoutScaleout = wformatString("{0}_RegularService", testName);
        ErrorCode error = plb.UpdateService(ServiceDescription(
            wstring(serviceWithoutScaleout),
            wstring(testType),
            wstring(L""),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(L""),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"CPU/1.0/100/100"),
            FABRIC_MOVE_COST_LOW,
            false,              // on every node
            1,                  // partition count
            2));                // target replica set size

        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::InsufficientClusterCapacity);

        // Now, report load of 0 for the application
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, wstring(serviceName), L"CPU", 0, std::map<Federation::NodeId, uint>()));

        fm_->RefreshPLB(Stopwatch::Now());

        // Now we do have space to create application
        error = plb.UpdateService(ServiceDescription(
            wstring(serviceWithoutScaleout),
            wstring(testType),
            wstring(L""),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(L""),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"CPU/1.0/100/100"),
            FABRIC_MOVE_COST_LOW,
            false,              // on every node
            1,                  // partition count
            2));                // target replica set size

        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithApplicationReservationWithAppsCreationAndDeletion)
    {
        // Application have reserved capacity; check update and deletion
        wstring testName = L"PlacementWithApplicationReservationWithAppsCreationAndDeletion";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 5;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            CreateApplicationCapacities(L"CPU/200/100/90"),
            appScaleoutCount,       // MinimumNodes == 5
            appScaleoutCount));     // MaximumNodes == 5

        wstring service1OfApp1 = wformatString("Service0_{0}_Application1", testName);
        ErrorCode error = plb.UpdateService(ServiceDescription(
            wstring(service1OfApp1),
            wstring(testType),
            wstring(appName1),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(L""),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"CPU/1.0/80/80"),
            FABRIC_MOVE_COST_LOW,
            false,              // on every node
            1,                  // partition count
            5));                // target replica set size

        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1OfApp1), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        wstring serviceWithoutScaleout = wformatString("{0}_RegularService", testName);
        error = plb.UpdateService(ServiceDescription(
            wstring(serviceWithoutScaleout),
            wstring(testType),
            wstring(L""),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(L""),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"CPU/1.0/5/5"),
            FABRIC_MOVE_COST_LOW,
            false,              // on every node
            1,                  // partition count
            5));                // target replica set size

        VERIFY_ARE_EQUAL(error.ReadValue(), ErrorCodeValue::Success);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithBigLoads)
    {
        wstring testName = L"PlacementWithBigLoads";
        Trace.WriteInfo("PLBBalancingTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceSmall", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestServiceBig", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/2147483647/2147483647")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestServiceSmall"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestServiceSmall"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestServiceBig"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestServiceBig"), 0, CreateReplicas(L""), 1));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestServiceSmall", L"SurprizeMetric", INT_MAX, std::map<Federation::NodeId, uint>()));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestServiceSmall", L"SurprizeMetric", INT_MAX, std::map<Federation::NodeId, uint>()));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"3 add primary 1", value)), 1u);
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityUpgrade1)
    {
        // Aligned affinity replicas should be swapped together: happy path scenario
        wstring testName = L"PlacementWithAffinityUpgrade1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/10"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentApp = wformatString("{0}_parent_app", testName);
        wstring childApp1 = wformatString("{0}_child1_app", testName); // Aligned
        wstring childApp2 = wformatString("{0}_child2_app", testName); // Non-Aligned


        plb.UpdateApplication(ApplicationDescription(wstring(parentApp), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));
        plb.UpdateApplication(ApplicationDescription(wstring(childApp1), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));
        plb.UpdateApplication(ApplicationDescription(wstring(childApp2), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));

        wstring parentService = wformatString("{0}_parent", testName);
        wstring childService1 = wformatString("{0}_child1", testName); // Aligned
        wstring childService2 = wformatString("{0}_child2", testName); // Non-Aligned

        // Add services with new replica and extra replica for coverage
        wstring testService = wformatString("{0}_test", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, testType, parentApp, true, CreateMetrics(L"MyMetric/1.0/20/0")));

        // aligned
        plb.UpdateService(ServiceDescription(
            wstring(childService1),
            wstring(testType),
            wstring(childApp1),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(parentService),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"MyMetric/1.0/20/0"),
            FABRIC_MOVE_COST_LOW,
            false              // on every node
            ));

        // non-aligned
        plb.UpdateService(ServiceDescription(
            wstring(childService2),
            wstring(testType),
            wstring(childApp2),       // application
            true,               // statefull
            wstring(L""),       // placement constraints
            wstring(parentService),       // affinitized service
            false,               // alligned affinity
            CreateMetrics(L""),
            FABRIC_MOVE_COST_LOW,
            false              // on every node
            ));

        plb.UpdateService(CreateServiceDescription(testService, testType, true, CreateMetrics(L"")));

        // Regular scenario, aligned child1 should be swapped with parent, but not child2
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(childService2), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, upgradingFlag_));

        // Partitions with new and extra replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(testService), 0, CreateReplicas(L""), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 3u);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithAffinityUpgrade2)
    {
        // Aligned affinity replicas should be swapped together: capacity violation
        wstring testName = L"PlacementWithAffinityUpgrade2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"", L"UD0", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"", L"UD0", L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"", L"UD1", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"", L"UD1", L"MyMetric/10"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentApp = wformatString("{0}_parent_app", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(parentApp), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));

        wstring parentService = wformatString("{0}_parent", testName);
        wstring childService1 = wformatString("{0}_child1", testName); // Aligned
        wstring childService2 = wformatString("{0}_child2", testName); // Non-Aligned

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, testType, parentApp, true, CreateMetrics(L"MyMetric/1.0/20/0")));

        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService1, testType, true, parentService, CreateMetrics(L"MyMetric/1.0/10/0")));
        plb.UpdateService(CreateServiceDescriptionWithNonAlignedAffinity(childService2, testType, true, wstring(parentService)));

        // Add services with new replica and extra replica for coverage
        wstring testApp = wformatString("{0}_test_app", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(testApp), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));

        wstring testService = wformatString("{0}_test", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(testService, testType, testApp, true, CreateMetrics(L"")));

        // Regular scenario, aligned child1 should be swapped with parent, but not child2
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService1), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(childService2), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, upgradingFlag_));

        // Partitions with new and extra replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(testService), 0, CreateReplicas(L""), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(testService), 0, CreateReplicas(L"P/0, S/1, S/2"), -1, upgradingFlag_));


        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Capacity is relaxed, affinity group should be generated
        VERIFY_ARE_EQUAL(actionList.size(), 4u);
        // Due to UD constraint, node 2 is the only choice
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>2", value)));

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary *", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 drop secondary *", value)));

        fm_->Clear();

        PLBConfigScopeChange(RelaxCapacityConstraintForUpgrade, bool, false);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(testService), 1, CreateReplicas(L"P/3"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(testService), 1, CreateReplicas(L"P/0, S/2"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);

        // Capacity is NOT relaxed, Node1 and node2 only has capacity for 1 swap
        // With RelaxCapacity false, the search for upgrade is skipped
        VERIFY_ARE_EQUAL(actionList.size(), 0u);

    }

    BOOST_AUTO_TEST_CASE(ApplicationUpgradeWithReservedLoadAndAllignedAffinity)
    {
        wstring testName = L"ApplicationUpgradeWithReservedLoadAndAllignedAffinity";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring applicationName = wformatString("{0}_Application", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"MyMetric/300/100/90")));

        wstring parentService = wformatString("{0}_ParentService", testName);
        wstring childService = wformatString("{0}_ChildService", testName); // Aligned
        wstring otherService = wformatString("{0}_OtherService", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, testType, applicationName, true, CreateMetrics(L"MyMetric/1.0/50/40")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(childService, testType, true, applicationName, parentService, CreateMetrics(L"MyMetric/1.0/50/40")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(otherService, testType, L"", true, CreateMetrics(L"MyMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));
        // This service will not allow promotion to node 2 because of node capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(otherService), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 2u);
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0|1 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithStatelessServiceAndAlignedAffinity)
    {
        // Aligned affinity replicas should be swapped together: happy path scenario
        wstring testName = L"PlacementWithStatelessServiceAndAlignedAffinity";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentApp = wformatString("{0}_parent_app", testName);
        wstring childApp1 = wformatString("{0}_child1_app", testName); // Aligned

        plb.UpdateApplication(ApplicationDescription(wstring(parentApp), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));
        plb.UpdateApplication(ApplicationDescription(wstring(childApp1), std::map<std::wstring, ApplicationCapacitiesDescription>(), 0));

        wstring parentService = wformatString("{0}_parent", testName);
        wstring childService1 = wformatString("{0}_child1", testName); // Aligned

        // Add services with new replica and extra replica for coverage
        wstring testService = wformatString("{0}_test", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, testType, parentApp, true, CreateMetrics(L"MyMetric/1.0/20/0")));

        // child is stateless with aligned affinity
        plb.UpdateService(ServiceDescription(
            wstring(childService1),
            wstring(testType),
            wstring(childApp1),       // application
            false,               // stateful
            wstring(L""),       // placement constraints
            wstring(parentService),       // affinitized service
            true,               // alligned affinity
            CreateMetrics(L"MyMetric/1.0/20/0"),
            FABRIC_MOVE_COST_LOW,
            false              // on every node
            ));

        plb.UpdateService(CreateServiceDescription(testService, testType, true, CreateMetrics(L"")));

        // Regular scenario, aligned child1 should be swapped with parent, but not child2
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/I, S/1, S/2"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService1), 0, CreateReplicas(L"I/0, I/1, I/2"), 0, upgradingFlag_));

        // Partitions with new and extra replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(testService), 0, CreateReplicas(L""), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 2u);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1|2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary *", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementSingletonPromoteSecondary)
    {
        wstring testName = L"PlacementSingletonPromoteSecondary";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        Reliability::FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry.Test_SetValue(true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"ud1", map<wstring, wstring>(), L""));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"ud1", map<wstring, wstring>(), L""));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(wstring(L"MyService"), wstring(L"MyType"), true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"MyService"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateFailoverUnit((FailoverUnitDescription(CreateGuid(0), wstring(L"MyService"), 2, CreateReplicas(L"P/0/DI, S/1"), 0, upgradingFlag_)));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //we should promote singleton replicas irrespective of ud issues
        //do not generate void movement
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 promote secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDownNode)
    {
        wstring testName = L"PlacementWithDownNode";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(2, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, L"", true, CreateMetrics(L"")));

        // 1 new replica required
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 1u);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDownNodesAndUD)
    {
        wstring testName = L"PlacementWithDownNodeAndUD";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        //#Placement object dump, you can copy / paste into LBSimulator for debugging
        //#nodeId #nodeIndex faultDomainIndex upgradeDomainIndex loads disappearingLoads capacityRatios bufferedCapacities totalCapacities isDeactivated isUp
        //10 0 () (0) 0:0 1 : 0 2 : 0  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false false
        //30 1 () (1) 0 : 0 1 : 0 2 : 0  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false true
        //20 2 () (2) 0 : 0 1 : 0 2 : 0  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false true
        //40 3 () (0) 0 : 2 1 : 0 2 : 2  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false false
        //60 4 () (1) 0 : 7 1 : 4 2 : 6  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false true
        //50 5 () (2) 0 : 7 1 : 2 2 : 6  0 : 0 1 : 0 2 : 0  0 : 1 1 : 1 2 : 1  0 : -1 1 : -1 2 : -1  0 : -1 1 : -1 2 : -1  false true

        plb.UpdateNode(CreateNodeDescription(16, L"10_fd", L"UD1"));     // 10
        plb.UpdateNode(CreateNodeDescription(32, L"20_fd", L"UD2"));     // 20
        plb.UpdateNode(CreateNodeDescription(48, L"30_fd", L"UD3"));     // 30
        plb.UpdateNode(CreateNodeDescription(64, L"40_fd", L"UD1"));     // 40
        plb.UpdateNode(CreateNodeDescription(80, L"50_fd", L"UD2"));     // 50
        plb.UpdateNode(CreateNodeDescription(96, L"60_fd", L"UD3"));    // 60

        // Services and types
        //fabric: / stateless type : CalculatorServiceType application : stateful : false placementConstraints : affinity : alignedAffinity : true metrics : (PrimaryCount / 1 / 0 / 0 ReplicaCount / 0.3 / 0 / 0 Count / 0.1 / 0 / 0) defaultMoveCost : 1 everyNode : false partitionCount : 1 targetReplicaSetSize : 3
        //fabric : / volatile2 type : TestStoreServiceType application : stateful : true placementConstraints : affinity : alignedAffinity : true metrics : (PrimaryCount / 1 / 0 / 0 ReplicaCount / 0.3 / 0 / 0 Count / 0.1 / 0 / 0) defaultMoveCost : 1 everyNode : false partitionCount : 1 targetReplicaSetSize : 3
        //fabric : / volatile3 type : TestStoreServiceType application : stateful : true placementConstraints : affinity : alignedAffinity : true metrics : (PrimaryCount / 1 / 0 / 0 ReplicaCount / 0.3 / 0 / 0 Count / 0.1 / 0 / 0) defaultMoveCost : 1 everyNode : false partitionCount : 1 targetReplicaSetSize : 3

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"CalculatorServiceType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestStoreServiceType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/stateless"),      //std::wstring && serviceName,
            wstring(L"CalculatorServiceType"),  //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            false,                              //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/volatile2"),      //std::wstring && serviceName,
            wstring(L"TestStoreServiceType"),   //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            true,                               //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/volatile3"),      //std::wstring && serviceName,
            wstring(L"TestStoreServiceType"),   //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            true,                               //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"fabric:/stateless"), 0, CreateReplicas(L"I/64,I/80,I/96"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"fabric:/volatile2"), 0, CreateReplicas(L"S/64,P/80,S/96"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"fabric:/volatile3"), 0, CreateReplicas(L"S/64,S/80,P/96"), 0));

        //#partitionId serviceName version isInTransition movable primaryLoad secondaryLoad existingReplicas newReplicas standByLocations primaryUpgradeLocation secondaryUpgradeLocation inUpgrade isSwapPrimary upgradeIndex
        //949bc28d - 9885 - 4832 - a683 - 4ad71cad4896 fabric: / volatile2 13 false true 0 : 1 1 : 1 2 : 1  0 : 0 1 : 1 2 : 1  (60 / Secondary / true 50 / Primary / true) (Secondary)() () () (false) (false)
        //7d6cccc4 - a6f5 - 40e4 - 9051 - 9243631a383c fabric : / volatile3 16 false true 0 : 1 1 : 1 2 : 1  0 : 0 1 : 1 2 : 1  (60 / Primary / true 50 / Secondary / true) (Secondary)() () () (false) (false)
        //129eefdb - 4451 - 4bf0 - b117 - 409b067f49c5 fabric : / stateless 7 false true 0 : 0 1 : 0 2 : 1  0 : 0 1 : 0 2 : 1  (50 / Secondary / true 60 / Secondary / true) (Secondary)() () () (false) (false)

        // Nodes go down now!
        plb.UpdateNode(NodeDescription(CreateNodeInstance(16, 0),       // 10
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"10_fd").Segments),
            wstring(L"UD1"),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(64, 0),        // 40
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"40_fd").Segments),
            wstring(L"UD1"),
            map<wstring, uint>(),
            map<wstring, uint>()));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"fabric:/stateless"), 0, CreateReplicas(L"I/80,I/96"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"fabric:/volatile2"), 0, CreateReplicas(L"P/80,S/96"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"fabric:/volatile3"), 0, CreateReplicas(L"S/80,P/96"), 1));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 3u);
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"* add * 32|48", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDownNodesAndFD)
    {
        wstring testName = L"PlacementWithDownNodesAndFD";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Entire fd:/rack1 is down (nodes 16 and 64)
        plb.UpdateNode(NodeDescription(CreateNodeInstance(16, 0),       // 10
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"fd:/rack1").Segments),
            wstring(L"UD1"),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.UpdateNode(CreateNodeDescription(32, L"fd:/rack2", L"UD2"));     // 20
        plb.UpdateNode(CreateNodeDescription(48, L"fd:/rack3", L"UD3"));     // 30
        plb.UpdateNode(NodeDescription(CreateNodeInstance(64, 0),        // 40
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"fd:/rack1").Segments),
            wstring(L"UD4"),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.UpdateNode(CreateNodeDescription(80, L"fd:/rack2", L"UD5"));     // 50
        plb.UpdateNode(CreateNodeDescription(96, L"fd:/rack3", L"UD6"));     // 60

        // Services and types

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"CalculatorServiceType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestStoreServiceType"), set<NodeId>()));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/stateless"),      //std::wstring && serviceName,
            wstring(L"CalculatorServiceType"),  //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            false,                              //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/volatile2"),      //std::wstring && serviceName,
            wstring(L"TestStoreServiceType"),   //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            true,                               //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateService(ServiceDescription(
            wstring(L"fabric:/volatile3"),      //std::wstring && serviceName,
            wstring(L"TestStoreServiceType"),   //std::wstring && servicTypeName,
            wstring(L""),                       //std::wstring && applicationName,
            true,                               //bool isStateful,
            wstring(L""),                       //std::wstring && placementConstraints,
            wstring(L""),                       //std::wstring && affinitizedService,
            true,                               //bool alignedAffinity,
            std::vector<ServiceMetric>(),       //std::vector<ServiceMetric> && metrics,
            FABRIC_MOVE_COST_LOW,               //uint defaultMoveCost,
            false,                              //bool onEveryNode,
            1,                                  //int partitionCount = 0,
            3,                                  //int targetReplicaSetSize = 0,
            false));                            // bool hasPersistedState = true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"fabric:/stateless"), 0, CreateReplicas(L"I/80,I/96"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"fabric:/volatile2"), 0, CreateReplicas(L"P/80,S/96"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"fabric:/volatile3"), 0, CreateReplicas(L"S/80,P/96"), 1));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 3u);
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"* add * 32|48", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsInvertedSwap)
    {
        //Simple placement test which test if our logic is successfully turned off
        //by swapping one primary replica, with our logic turned off, it should select
        //any random secondary replica, but by adding additional replicas to nodes in UDs 0 and 1, PLB should avoid them
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeCompletedUDsInvertedSwap");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreferUpgradedUDs, bool, false);
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        completedUDs.insert(L"UD1");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I,S/1,S/2,S/4,S/8"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //For partition 0, replica should not be placed in UD1 out of 5 UDs since it is in completed UD
        VERIFY_ARE_EQUAL(actionList.size(), 1u);
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 swap primary 2|1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest1Inverted)
    {
        //Simple placement test which test if our logic is successfully turned off
        //by placing one secondary replica, with our logic turned off, it should select
        //any random node, but by adding additional replicas to nodes in UDs 0 and 1, PLB should avoid them
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeCompletedUDsTest1Inverted");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(PreferUpgradedUDs, bool, false);

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        completedUDs.insert(L"UD1");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/8, SB/10"), 1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/12, D/15/K"), 1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 3u);
        //For partition 0, replica should be placed outside of UD1 since it's upgraded UD, and we are trying to avoid it
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 add secondary 1|2|3", value)));

        //For partition 1, replica should be placed on SB location instead of completed UDs
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 10", value)));

        //For partition 2, replica should be placed on upgrade location instead of completed UDs
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 15", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest1)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeCompletedUDsTest1");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 40; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        completedUDs.insert(L"UD1");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/8, SB/10"), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/12, D/20/K"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //For partition 0, replica should be placed in UD1 out of 10 UDs since it is in completed UD
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2|3", value)));

        //For partition 1, replica should be placed on SB location instead of completed UDs
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 10", value)));

        //For partition 2, replica should be placed on upgrade location instead of completed UDs
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 20", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest2Inverted)
    {
        // ToBeSwapped case with Upgraded UDs and capacity violation
        // In this case, upgraded UDs logic is turned off, and simple service is added to make
        // PLB avoid those replicas in UD 0 and 1
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeCompletedUDsTest2Inverted");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(PreferUpgradedUDs, bool, false);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", "PlacementWithUpgradeCompletedUDsTest2Inverted");
            return;
        }

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"", L"UD0", L"UpgradeCompletedUDsTest2_Metric1/10"));
        // 2 nodes in each UD
        for (int i = 1; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"UpgradeCompletedUDsTest2_Metric1/1.0/20/10")));

        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/2/I, S/1, S/4, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/2/I, S/0, S/4, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 2u);

        //UD0 should be chosen
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>1", value)));
        // Capacity should be relaxed
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 swap primary 2<=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest2)
    {
        // ToBeSwapped case with Upgraded UDs and capacity violation
        Trace.WriteInfo("PLBPlacementTestSource", "PlacementWithUpgradeCompletedUDsTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"", L"UD0", L"UpgradeCompletedUDsTest2_Metric1/10"));
        // 2 nodes in each UD
        for (int i = 1; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"UpgradeCompletedUDsTest2_Metric1/1.0/20/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/2/I, S/1, S/4, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/2/I, S/0, S/4, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(actionList.size(), 2u);

        //UD0 should be chosen
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>1", value)));
        // Capacity should be relaxed
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 2<=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest3Inverted)
    {
        // Application upgraded UDs verification with preferring logic turned off
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(PreferUpgradedUDs, bool, false);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
        ));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"TestType", applicationName, true));
        
        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/4/I, S/1, S/2, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/6/I, S/1, S/4, S/8, S/10, S/12, S/14, S/16, S/18"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/8, S/12, S/14, S/16, S/18"), 1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());

        //Node 1 in UD0 is the only invalid node since it's in upgrade domain which we try to avoid
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* swap primary *<=>1", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"2 add secondary 0|1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest3)
    {
        // Application upgraded UDs verification
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
            ));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"TestType", applicationName, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/4/I, S/1, S/6, S/8, S/10, S/12, S/14, S/16, S/18"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/6/I, S/1, S/4, S/8, S/10, S/12, S/14, S/16, S/18"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/8, S/12, S/14, S/16, S/18"), 1));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());

        //Node 1 in UD0 is the only valid node
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* swap primary *<=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 0|1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest4Inverted)
    {
        // To be swapped primary replicas of two services with affinity between them
        // Nodes in UDs 1 and 0 should be avoided
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest4Inverted";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(PreferUpgradedUDs, bool, false);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBPlacementTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        for (int i = 0; i < 12; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("{0}", int(i))));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"0");
        completedUDs.insert(L"1");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
        ));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_P", testName);
        wstring childService = wformatString("{0}_C", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, parentType, wstring(applicationName), true));

        plb.UpdateService(ServiceDescription(
            wstring(childService),
            wstring(childType),
            wstring(applicationName),
            true,
            wstring(L""),
            wstring(parentService),
            false,
            std::vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            false)
        );

        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(childService), 0, CreateReplicas(L"P/4/I, S/1, S/5, S/6, S/7, S/8, S/9, S/10, S/11"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(parentService), 0, CreateReplicas(L"P/4/I, S/1, S/5, S/6, S/7, S/8, S/9, S/10, S/11"), 0, upgradingFlag_));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/5/I, S/1/B"), 0, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        //For partition 0/1, replica should be swapped to UD1
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* swap primary *<=>1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest4)
    {
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest4";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 12; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("{0}", int(i))));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"0");
        completedUDs.insert(L"1");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
            ));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_P", testName);
        wstring childService = wformatString("{0}_C", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(parentService, parentType, wstring(applicationName), true));

        plb.UpdateService(ServiceDescription(
            wstring(childService),
            wstring(childType),
            wstring(applicationName),
            true,
            wstring(L""),
            wstring(parentService),
            false,
            std::vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            false)
            );

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(childService), 0, CreateReplicas(L"P/4/I, S/1, S/5, S/6, S/7, S/8, S/9, S/10, S/11"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(parentService), 0, CreateReplicas(L"P/4/I, S/1, S/5, S/6, S/7, S/8, S/9, S/10, S/11"), 0));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/5/I, S/1/B"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //For partition 0/1, replica should be swapped to UD1
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* swap primary *<=>1", value)));

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest5)
    {
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest5";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 1);
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);
        PLBConfigScopeChange(PlacementSearchTimeout, TimeSpan, TimeSpan::FromSeconds(10.0));

        std::wstring defragMetric = L"Metric1";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        // Customer cluster node setting
        plb.UpdateNode(CreateNodeDescription(0, L"fd:/0", L"0", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/1", L"1", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(2, L"fd:/2", L"2", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(3, L"fd:/3", L"3", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(4, L"fd:/4", L"4", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(5, L"fd:/0", L"5", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(6, L"fd:/1", L"6", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(7, L"fd:/2", L"7", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(8, L"fd:/3", L"8", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(9, L"fd:/4", L"9", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(10, L"fd:/0", L"1", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(11, L"fd:/1", L"2", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(12, L"fd:/2", L"3", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(13, L"fd:/3", L"4", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(14, L"fd:/4", L"5", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(15, L"fd:/0", L"6", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(16, L"fd:/1", L"7", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(17, L"fd:/2", L"8", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(18, L"fd:/3", L"9", map<wstring, wstring>(), L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescription(19, L"fd:/4", L"0", map<wstring, wstring>(), L"Metric1/100"));

        // Cluster upgrade has finished 8 UDs out of 9
        std::set<std::wstring> fabricUpgradeCompletedUDs;
        fabricUpgradeCompletedUDs.insert(L"0");
        fabricUpgradeCompletedUDs.insert(L"1");
        fabricUpgradeCompletedUDs.insert(L"2");
        fabricUpgradeCompletedUDs.insert(L"3");
        fabricUpgradeCompletedUDs.insert(L"4");
        fabricUpgradeCompletedUDs.insert(L"5");
        fabricUpgradeCompletedUDs.insert(L"6");
        fabricUpgradeCompletedUDs.insert(L"7");
        fabricUpgradeCompletedUDs.insert(L"8");

        plb.UpdateClusterUpgrade(true, move(fabricUpgradeCompletedUDs));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            std::set<std::wstring>(completedUDs)
            ));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_P", testName);
        wstring childService = wformatString("{0}_C", testName);

        plb.UpdateService(ServiceDescription(
            wstring(parentService),
            wstring(parentType),
            wstring(applicationName),
            true,
            wstring(L""),
            wstring(L""),
            false,
            CreateMetrics(L"Metric1/1.0/10/10"),
            FABRIC_MOVE_COST_LOW,
            false,
            1,
            1)
            );

        plb.UpdateService(ServiceDescription(
            wstring(childService),
            wstring(childType),
            wstring(applicationName),
            true,
            wstring(L""),
            wstring(parentService),
            false,
            vector<ServiceMetric>(),
            FABRIC_MOVE_COST_LOW,
            false,
            1,
            1)
            );

        completedUDs.insert(L"0");
        completedUDs.insert(L"1");

        // Application upgrade finished 2 UDs
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
            ));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/12/DI"), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/12/DI"), 1));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"Metric1/1.0/80/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
        CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/5, S/10, S/11, S/19"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // App upgrade finished UD0/1, while cluster upgrade finished UD0 to UD8
        // Verify: new replica can be only be placed in UD0
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 0|1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsTest6)
    {
        wstring testName = L"PlacementWithUpgradeCompletedUDsTest6";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        completedUDs.insert(L"UD1");
        plb.UpdateClusterUpgrade(true, move(completedUDs));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceType = wformatString("{0}_TestType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceNameStateful = wformatString("{0}_Stateful", testName);
        plb.UpdateService(CreateServiceDescription(serviceNameStateful, serviceType, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceNameStateful), 0, CreateReplicas(L"P/0"), 1));

        wstring serviceNameStateless = wformatString("{0}_Stateless", testName);
        plb.UpdateService(ServiceDescription(wstring(serviceNameStateless), wstring(serviceType), wstring(L""), false,
            wstring(L""), wstring(L""), true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceNameStateless), 0, CreateReplicas(L"I/0, I/1, I/2, I/3"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //For partition 0, replica should be placed in UD1 out of 5 UDs since it is in completed UD
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2|3", value)));

        VERIFY_ARE_EQUAL(6, CountIf(actionList, ActionMatch(L"1 add instance *", value)));

    }

    BOOST_AUTO_TEST_CASE(BalancingExistingReplicaDuringPlacement)
    {
        // Balancing existing replicas during placement
        // Purpose: Regression test for the old vs new movement correlation assumptions,
        // during solution movement acceptance (RDBug 8098654)
        wstring testName = L"BalancingExistingReplicaDuringPlacement";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"M1/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp1", 1, 4, CreateApplicationCapacities(L"M1/100/60/0")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp2", 1, 4, CreateApplicationCapacities(L"M1/100/60/0")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"StatefulService1", L"ServiceType1", L"TestApp1", true, CreateMetrics(L"M1/1.0/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"StatelessService1", L"ServiceType1", L"TestApp2", false, CreateMetrics(L"M1/1.0/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"StatefulService2", L"ServiceType2", true, L"", CreateMetrics(L"M1/1.0/20/20")));

        upgradingFlag_.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"StatefulService1"), 0, CreateReplicas(L"P/0,SB/2,SB/4"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"StatelessService1"), 0, CreateReplicas(L"S/0,S/1,SB/2,SB/4"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"StatefulService2"), 0, CreateReplicas(L"P/0/I,S/1,D/2/K"), 1, upgradingFlag_));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithSingleReplicaMove)
    {
        wstring testName = L"ImprovePlacementWithSingleReplicaMove";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/150"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType0", true, CreateMetrics(L"M1/1.0/100/100")));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestServiceType1", true, CreateMetrics(L"M1/1.0/90/90")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB couldn't place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // try CreationWithMove with partial and full closure
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB with full closure placed replica
        // existing replica had to be moved to Node1, so the new one could be placed
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithAffinity)
    {
        wstring testName = L"ImprovePlacementWithMoveWithAffinity";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5.0));
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/110"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/120"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType", true, CreateMetrics(L"M1/1.0/40/30")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestServiceType", true, L"TestService0", CreateMetrics(L"M1/1.0/30/30")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestServiceType", true, L"TestService0", CreateMetrics(L"M1/1.0/20/20")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestServiceType", true, L"TestService0", CreateMetrics(L"M1/1.0/15/15")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService4", L"TestServiceType", true, L"TestService0", CreateMetrics(L"M1/1.0/15/15")));
        plb.UpdateService(CreateServiceDescription(L"TestService5", L"TestServiceType", true, CreateMetrics(L"M1/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService5"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB couldn't place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // try CreationWithMove with partial closure which includes R0, R1, R2, R3 and R4
        // R5 is not in the closure (no Affinity or AppGroup)
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        // move parent(P0) and all its children to Node1 => 1 move for parent + 2 moves for R1 and R2 (children)
        // then place new replicas on Node1 => 2 moves for new replicas
        // R5 remains on Node0
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithScaleoutCount)
    {
        wstring testName = L"ImprovePlacementWithMoveWithScaleoutCount";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/110"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/170"));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L""),
            1,       // MinimumNodes == 1
            1));     // MaximumNodes == 1

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService0",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/40/40")));


        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB couldn't place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());


        // try CreationWithMove with partial closure
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        // move parent(P0) and all children to Node1
        // then place new replica on Node1
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move secondary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithScaleoutCountAndAffinityWithTwoReplicas)
    {
        wstring testName = L"ImprovePlacementWithMoveWithScaleoutCountAndAffinityWithTwoReplicas";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/110"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType", true, CreateMetrics(L"MyMetric/1.0/40/40")));

        // parent - Service1 - blocklisted Node0
        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestServiceType1", true, CreateMetrics(L"MyMetric/1.0/40/40")));

        // child - Service2 - blocklisted Node1
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestServiceType2", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/20/20")));


        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L""),
            1,       // MinimumNodes == 0
            1));     // MaximumNodes == 1

        set<NodeId> blockList3;
        blockList3.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType3"), move(blockList3)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestServiceType3",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/50/40")));

        set<NodeId> blockList4;
        blockList4.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType4"), move(blockList4)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService4",
            L"TestServiceType4",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB couldn't place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());


        // try CreationWithMove with partial closure
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 2=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add primary 1", value)));

    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithScaleoutCountAndAffinityWithOneReplica)
    {
        wstring testName = L"ImprovePlacementWithMoveWithScaleoutCountAndAffinityWithOneReplica";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/110"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType", true, CreateMetrics(L"MyMetric/1.0/40/40")));

        // parent - Service1 - blocklisted Node0
        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestServiceType1", true, CreateMetrics(L"MyMetric/1.0/40/40")));

        // child - Service2 - blocklisted Node1
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestServiceType2", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/20/20")));


        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L""),
            1,       // MinimumNodes == 0
            1));     // MaximumNodes == 1

        set<NodeId> blockList3;
        blockList3.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType3"), move(blockList3)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestServiceType3",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/50/40")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService4",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/2"), 0));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // verify that CreationWithMove made moves and placed replicas
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 2=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 move primary 2=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithSingleReplicaAndAffinity)
    {
        wstring testName = L"ImprovePlacementWithMoveWithSingleReplicaAndAffinity";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/80"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType", true, CreateMetrics(L"MyMetric/1.0/60/60")));

        // parent - Service1 - blocklisted Node0
        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestServiceType1", true, CreateMetrics(L"MyMetric/1.0/40/40")));

        // child - Service
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestServiceType", true, L"TestService1", CreateMetrics(L"MyMetric/1.0/30/30")));

        // service with blocklisted Node2
        set<NodeId> blockList3;
        blockList3.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType3"), move(blockList3)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestServiceType3", true, CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // verify that CreationWithMove with partial closure made moves and placed replicas
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary 2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveWithSingleReplicaAndScaleoutCount)
    {
        wstring testName = L"ImprovePlacementWithMoveWithSingleReplicaAndScaleoutCount";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/80"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/80"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/80"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestServiceType", true, CreateMetrics(L"MyMetric/1.0/20/20")));

        // parent - Service1 - blocklisted Node2
        wstring applicationName = wformatString("fabric:/{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            CreateApplicationCapacities(L""),
            1,       // MinimumNodes == 1
            1));     // MaximumNodes == 1

        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType1"), move(blockList1)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestServiceType1",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestServiceType",
            wstring(applicationName),
            true,
            CreateMetrics(L"MyMetric/1.0/30/30")));

        set<NodeId> blockList3;
        blockList3.insert(CreateNodeId(1));
        blockList3.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestServiceType3"), move(blockList3)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestServiceType3", true, CreateMetrics(L"MyMetric/1.0/60/60")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB could not place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // verify that CreationWithMove with partial closure made moves and placed replicas
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMovePreventTransientOverCommitNoMovement)
    {
        wstring testName = L"ImprovePlacementWithMovePreventTransientOverCommitNoMovement";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProp0;
        nodeProp0[L"Type"] = L"TypeA";
        map<wstring, wstring> nodeProp1;
        nodeProp1[L"Type"] = L"TypeB";

        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(0, L"MyMetric/100", move(nodeProp0)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(1, L"MyMetric/100", move(nodeProp1)));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/100/100")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"Type == TypeA", CreateMetrics(L"MyMetric/1.0/100/100")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB could not place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMovePreventTransientOverCommitCreateMovement)
    {
        wstring testName = L"ImprovePlacementWithMovePreventTransientOverCommitCreateMovement";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProp0;
        nodeProp0[L"Type"] = L"TypeA";
        map<wstring, wstring> nodeProp1;
        nodeProp1[L"Type"] = L"TypeB";

        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(0, L"MyMetric/100", move(nodeProp0)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(1, L"MyMetric/100", move(nodeProp1)));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/100/100")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"Type == TypeA", CreateMetrics(L"MyMetric/1.0/100/100")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));

        // first try just Creation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB could not place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(ImprovePlacementWithMoveBigTest)
    {
        wstring testName = L"ImprovePlacementWithMoveBigTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 1);
        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 50);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 9; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/150"));
        }
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(9, L"MyMetric/200"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/10")));

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0,
                CreateReplicas(L"P/0, S/1, S/2, S/3, S/4, S/5, S/6, S/7, S/8, S/9"), 0));
        }


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeAffinity"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceParent", L"TestTypeAffinity", true, CreateMetrics(L"MyMetric/1.0/30/30")));

        // child1 - Service
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestServiceChild1", L"TestTypeAffinity", true, L"TestServiceParent", CreateMetrics(L"MyMetric/1.0/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestServiceParent"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestServiceChild1"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // child2 - Service
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestServiceChild2", L"TestTypeAffinity", true, L"TestServiceParent", CreateMetrics(L"MyMetric/1.0/15/15")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(12), wstring(L"TestServiceChild2"), 0, CreateReplicas(L""), 1));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->ClearMoveActions();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // verify that PLB could not place any replicas
        VERIFY_ARE_EQUAL(0u, actionList.size());

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementIncomingLoadFallbackTest)
    {
        wstring testName = L"UseHeuristicDuringPlacementFallbackTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.3));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // One node is full, second node is half full
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(
            wformatString("Service1"),
            L"TestType",
            true,
            CreateMetrics(wformatString("{0}/{1}/{2}/{3}",
                defragMetric,
                1.0,
                100,
                50))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wformatString("Service1"),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(0),
            move(FailoverUnitDescription(
                CreateGuid(0),
                wformatString("Service1"),
                0,
                CreateReplicas(L"P/0, S/1"),
                0))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Create new replica with replica diff +3
        plb.UpdateService(CreateServiceDescription(
            wformatString("New_Service"),
            L"TestType",
            true,
            CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                defragMetric,
                1.0,
                1))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wformatString("New_Service"),
            0,
            CreateReplicas(L""),
            3));

        fm_->FuMap.insert(make_pair<>(CreateGuid(1),
            move(FailoverUnitDescription(
                CreateGuid(1),
                wformatString("New_Service"),
                0,
                CreateReplicas(L""),
                3))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be placed on second node,
        // but not all of them should respect heuristc funciton because second node isn't available for placement, but placement won't fail, it will fallback on other nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 3);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 4; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 0);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementHalfEmptySpaceFallbackTest)
    {
        wstring testName = L"UseHeuristicDuringPlacementHalfEmptySpaceFallbackTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.5));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create new replicas which load is greater than capacity of three nodes
        // Incoming load is 500
        for (int i = 0; i < 50; i++)
        {
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    10))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i + 1),
                wformatString("New_Service{0}", i),
                0,
                CreateReplicas(L""),
                1));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i + 1),
                move(FailoverUnitDescription(
                    CreateGuid(i + 1),
                    wformatString("New_Service{0}", i),
                    0,
                    CreateReplicas(L""),
                    1))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that incoming load of 300 shold be on three nodes, while incoming load of 200 are spread across three nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 50);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        int fullNodes = 0;
        for (int i = 0; i < 6; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
            else if (queryResult.NodeLoadMetricInformation[0].NodeLoad == queryResult.NodeLoadMetricInformation[0].NodeCapacity)
            {
                fullNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 0 && fullNodes == 3);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementTestIncomingLoadFactor)
    {
        wstring testName = L"UseHeuristicDuringPlacementTestIncomingLoadFactor";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.3));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i,
                wformatString("dc0/r{0}", i%5),
                wformatString("{0}/{1}",
                defragMetric,
                100)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // Two nodes are half full
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(
            wformatString("Service1"),
            L"TestType",
            true,
            CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
            defragMetric,
            1.0,
            50))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wformatString("Service1"),
            0,
            CreateReplicas(L"P/0, S/1"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(0),
            move(FailoverUnitDescription(
            CreateGuid(0),
            wformatString("Service1"),
            0,
            CreateReplicas(L"P/0, S/1"),
            0))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Create new replica load of 100 defrag metric
        for (int i = 0; i < 50; i++)
        {
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                defragMetric,
                1.0,
                1))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i+1),
                wformatString("New_Service{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i+1),
                move(FailoverUnitDescription(
                CreateGuid(i + 1),
                wformatString("New_Service{0}", i),
                0,
                CreateReplicas(L""),
                2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should occupy 2 half full and 1 new node
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 100);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        //Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 7);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementHalfEmptySpace)
    {
        wstring testName = L"UseHeuristicDuringPlacementHalfEmptySpace";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.5));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 5 nodes in cluster
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(
            wformatString("Service1"),
            L"TestType",
            true,
            CreateMetrics(wformatString("{0}/{1}/{2}/{3}",
                defragMetric,
                1.0,
                100,
                50))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0),
            wformatString("Service1"),
            0,
            CreateReplicas(L"P/0"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(0),
            move(FailoverUnitDescription(
                CreateGuid(0),
                wformatString("Service1"),
                0,
                CreateReplicas(L"P/0"),
                0))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are placing 10 new replicas, and two nodes out of four should be beneficial
        for (int i = 0; i < 5; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    1))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i+1),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i+1),
                move(FailoverUnitDescription(
                    CreateGuid(i + 1),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be placed on three nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 10);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 5; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 2);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithBothParameters1)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithBothParameters";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.0));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.5));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // We are placing 30 new replicas, half of all the nodes are beneficial
        for (int i = 0; i < 15; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    10))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be placed on five nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 30);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 5);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithBothParameters2)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithBothParameters2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 2.0));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.5));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // We are placing 30 new replicas, 6/10 nodes are benefical -> max(2.0*300, 500) = 600
        for (int i = 0; i < 15; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    10))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be on 6 beneficial nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 30);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 4);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithoutHeuristicAndDefragOn)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithBothParameters";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        // Turn off heuristcs
        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        // We are placing 100 new replicas, there are no beneficial target nodes as we don't use heuristic
        for (int i = 0; i < 50; i++)
        {
            // Create new service with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    1))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // As heuristic is turned off, we are expecting that new replicas should be placed on every nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 100);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 0);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithoutHeuristictWithMixedMetrics)
    {
        wstring testName = L"PlacementWithoutHeuristictWithMixedMetrics";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        wstring balancingMetric1 = L"BalancingMetric1";
        wstring balancingMetric2 = L"BalancingMetric2";

        // Turn off heuristcs
        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // We are placing 100 new replicas, there are no beneficial target nodes as we don't use heuristic
        for (int i = 0; i < 50; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{3}/{4}/{4}, {1}/{3}/{4}/{4}, {2}/{3}/{4}/{4}",
                    defragMetric,
                    balancingMetric1,
                    balancingMetric2,
                    1.0,
                    1))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // As heuristic is turned off, we are expecting that new replicas should be placed on every nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 100);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 3);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 0);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithMixedMetrics)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithMixedMetrics";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        wstring balancingMetric1 = L"BalancingMetric1";
        wstring balancingMetric2 = L"BalancingMetric2";

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair<>(defragMetric, 0.25));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("{0}/{1}",
                    defragMetric,
                    100
                )));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // We are placing 20 new replicas, quarter of all the nodes are beneficial
        for (int i = 0; i < 10; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{3}/{4}/{4}, {1}/{3}/{4}/{4}, {2}/{3}/{4}/{4}",
                    defragMetric,
                    balancingMetric1,
                    balancingMetric2,
                    1.0,
                    10))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be placed the three nodes
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 20);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 3);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 7);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithBlockListedNodes)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithBlockListedNodes";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.0));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.5));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("DefragMetric/100")
            ));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        blockList.insert(CreateNodeId(2));
        blockList.insert(CreateNodeId(3));
        blockList.insert(CreateNodeId(4));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));

        // We are placing 10 new replicas, quarter of all the nodes are beneficial
        for (int i = 0; i < 10; i++)
        {
            // Create new replica with replica diff +2
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    1.0,
                    10))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                1));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    1))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are expecting that new replicas should be placed the only avaliable node
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 10);
        for (int i = 0; i < 10; i++)
        {
            VERIFY_IS_TRUE(ActionMatch(actionList[i], wformatString(L"{0} add primary 0", i)));
        }
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithLocalDomains)
    {
        wstring testName = L"UseHeuristicDuringPlacementWithLocalDomains";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"Metric2";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.2));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("Metric0/100,Metric1/110,Metric2/120,Metric3/130,Metric4/140,Metric5/150,Metric6/160")
                ));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create first service with two partitions
        plb.UpdateService(CreateServiceDescription(
            wformatString("New_Service_1"),
            L"TestType",
            true,
            CreateMetrics(L"Metric0/1.0/10/10,Metric1/1.0/10/10,Metric2/1.0/20/20,Metric3/1.0/10/10,Metric4/1.0/10/10,Metric5/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1),
            wformatString("New_Service_1"),
            0,
            CreateReplicas(L"P/0, S/1, S/2"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(1),
            move(FailoverUnitDescription(
                CreateGuid(1),
                wformatString("New_Service_1"),
                0,
                CreateReplicas(L"P/0, S/1, S/2"),
                0))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2),
            wformatString("New_Service_1"),
            0,
            CreateReplicas(L"P/0, S/1, S/2"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(2),
            move(FailoverUnitDescription(
                CreateGuid(2),
                wformatString("New_Service_1"),
                0,
                CreateReplicas(L"P/0, S/1, S/2"),
                0))));

        // Create second service with two partitions
        plb.UpdateService(CreateServiceDescription(
            wformatString("New_Service_2"),
            L"TestType",
            true,
            CreateMetrics(L"Metric3/1.0/10/10,Metric2/1.0/20/20,Metric5/1.0/10/10,Metric6/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3),
            wformatString("New_Service_2"),
            0,
            CreateReplicas(L"P/5, S/6, S/7"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(3),
            move(FailoverUnitDescription(
                CreateGuid(3),
                wformatString("New_Service_2"),
                0,
                CreateReplicas(L"P/5, S/6, S/7"),
                0))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4),
            wformatString("New_Service_2"),
            0,
            CreateReplicas(L"P/5, S/6, S/7"),
            0));

        fm_->FuMap.insert(make_pair<>(CreateGuid(4),
            move(FailoverUnitDescription(
                CreateGuid(4),
                wformatString("New_Service_2"),
                0,
                CreateReplicas(L"P/5, S/6, S/7"),
                0))));

        // Create two additionall services

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(5),
            wformatString("New_Service_1"),
            0,
            CreateReplicas(L""),
            1));

        fm_->FuMap.insert(make_pair<>(CreateGuid(5),
            move(FailoverUnitDescription(
                CreateGuid(5),
                wformatString("New_Service_1"),
                0,
                CreateReplicas(L""),
                1))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(6),
            wformatString("New_Service_2"),
            0,
            CreateReplicas(L""),
            1));

        fm_->FuMap.insert(make_pair<>(CreateGuid(6),
            move(FailoverUnitDescription(
                CreateGuid(6),
                wformatString("New_Service_2"),
                0,
                CreateReplicas(L""),
                1))));

        plb.ProcessPendingUpdatesPeriodicTask();

        // We are placing 20 new replicas, half of all the nodes are beneficial
        for (int i = 7; i < 17; i++)
        {
            // Create new replica with replica diff +3
            plb.UpdateService(CreateServiceDescription(
                wformatString("New_Service_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("Metric0/1.0/10/10,Metric1/1.0/10/10,Metric2/1.0/15/15,Metric3/1.0/10/10,Metric4/1.0/10/10,Metric5/1.0/10/10,Metric6/1.0/10/10"))));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wformatString("New_Service_{0}", i),
                0,
                CreateReplicas(L""),
                2));

            fm_->FuMap.insert(make_pair<>(CreateGuid(i),
                move(FailoverUnitDescription(
                    CreateGuid(i),
                    wformatString("New_Service_{0}", i),
                    0,
                    CreateReplicas(L""),
                    2))));
        }

        // We are expecting that new replicas should be placed on seven nodes
        // as 0.6% of free space should consider those nodes for all domains
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 22);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        // Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < 10; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 7);
            if (queryResult.NodeLoadMetricInformation[2].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }
        VERIFY_IS_TRUE(numOfEmptyNodes == 4);
    }

    BOOST_AUTO_TEST_CASE(UseHeuristicDuringPlacementWithCheckAffinityForUpgrade)
    {
        // Basic test for CheckAffinityForUpgradePlacement:
        // Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // Expectation is that we add one replica for parent and move child
        wstring testName = L"UseHeuristicDuringPlacementWithCheckAffinityForUpgrade";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.5));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        // Create 10 nodes in cluster
        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                wformatString("dc0/r{0}", i),
                wformatString("{0}", i),
                wformatString("DefragMetric/100")
            ));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_ParentService", testName);
        wstring childService = wformatString("{0}_ChildService", testName);
        plb.UpdateService(CreateServiceDescription(parentService, parentType, true, CreateMetrics(L"DefragMetric/1.0/10/10"), FABRIC_MOVE_COST_LOW, false, 1, false));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService, childType, true, parentService, CreateMetrics(L"DefragMetric/1.0/10/10"), false, 1, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        // Create one service on node 1 with load 60, so this node have 40 free capacity of DefragMetric
        wstring testType = wformatString("{0}_TestType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring testService = wformatString("{0}_TestService", testName);
        plb.UpdateService(CreateServiceDescription(testService, testType, true, CreateMetrics(L"DefragMetric/1.0/60/0"), FABRIC_MOVE_COST_LOW, false, 1, false));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(testService), 0, CreateReplicas(L"P/1"), 0));

        // Expectation is that heuristic suggests node 1 as it have enough free space for incoming replicas
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PromoteAndDrop)
    {
        Trace.WriteInfo("PLBPlacementTestSource", "PromoteAndDrop");
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        fm_->Clear();
        fm_->Load();

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, vector<ServiceMetric>(), FABRIC_MOVE_COST_LOW, false, 1));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I,S/1"), -1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/L"), -1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //plb should drop extra replicas even if we have I or L flag
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|1 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithFaultDomainAndPlacementConstraintNodeSetTest)
    {
        // Testing scenario covers when faultdomain candidate count is much less than the total node count
        wstring testName = L"PlacementWithFaultDomainAndPlacementConstraintNodeSetTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> ValidProperties;
        ValidProperties.insert(make_pair(L"NodeType", L"OK"));
        map<wstring, wstring> InvalidProperties;
        InvalidProperties.insert(make_pair(L"NodeType", L"NotOK"));

        const int nodeCount = 100;
        const int udCount = 5;
        const int fdCount = 5;

        int i = 0;
        for (; i < nodeCount - fdCount*2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), map<wstring, wstring>(InvalidProperties)));
        }

        for (; i < nodeCount + 1; i++)
        {
            // Make sure the number of valid nodes is greater than the number of domain to kick in FilterbyDomain logic
            plb.UpdateNode(CreateNodeDescription(i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), map<wstring, wstring>(ValidProperties)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring testService = L"TestService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(testService, L"TestType", true, L"NodeType==OK"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testService), 0, CreateReplicas(wformatString("P/{0}", nodeCount)), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreTargetUnderloadedNodes)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on underloaded non-empty nodes (the lowest 2 are needed in this test)

        wstring testName = L"PlacementRespectBalancingScoreTargetUnderloadedNodes";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0)));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("", i)), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreWithBalancingPlacementPolicy)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on least loaded non-empty nodes

        wstring testName = L"PlacementRespectBalancingScoreWithBalancingPlacementPolicy";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreWithPackingPlacementPolicy)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on most loaded nodes

        wstring testName = L"PlacementRespectBalancingScoreWithPackingPlacementPolicy";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 3));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 1));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 1|2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreWithNoPreferencePlacementPolicy)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on non-empty nodes

        wstring testName = L"PlacementRespectBalancingScoreWithNoPreferencePlacementPolicy";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 2));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 1|2|3|4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreTargetUnderloadedNodesUnderHalfEmptySpace)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on underloaded non-empty nodes
        // Both configs are defined and used, empty space percent asks for more load and should be respected
        // Results in lowest 3 nodes being targetted for placement

        wstring testName = L"PlacementRespectBalancingScoreTargetUnderloadedNodesUnderHalfEmptySpace";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementEmptySpacePercent;
        placementEmptySpacePercent.insert(make_pair(metric, 0.25));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent, PLBConfig::KeyDoubleValueMap, placementEmptySpacePercent);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0)));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("", i)), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 3|4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreTargetUnderloadedNodesOverHalfEmptySpace)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on underloaded non-empty nodes
        // Both configs are defined and used, incoming load factor asks for more load and should be respected
        // Results in lowest 4 nodes being targetted for placement

        wstring testName = L"PlacementRespectBalancingScoreTargetUnderloadedNodesOverHalfEmptySpace";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementEmptySpacePercent;
        placementEmptySpacePercent.insert(make_pair(metric, 0.25));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent, PLBConfig::KeyDoubleValueMap, placementEmptySpacePercent);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0)));
            }
        }

        for (int i = 0; i < 20; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("", i)), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(20u, actionList.size());
        VERIFY_ARE_EQUAL(20, CountIf(actionList, ActionMatch(L"* add primary 2|3|4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementRespectBalancingScoreTargetUnderloadedNodesInfiniteCapacity)
    {
        // Create a cluster state with nodes having various loads
        // Set the desired number of nodes
        // Try to create some new services
        // Verify that the new services are placed on underloaded non-empty nodes (the lowest 2 are needed in this test)
        // Nodes with infinite capacities are always targetted
        // Results in lowest 2 nodes along with nodes with infinite capacities being targetted for placement

        wstring testName = L"PlacementRespectBalancingScoreTargetUnderloadedNodesInfiniteCapacity";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        wstring metric = L"CPU";

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

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

        int numberOfNodesRequested = 4;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(metric, numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap placementStrategy;
        placementStrategy.insert(make_pair(metric, 1));
        PLBConfigScopeChange(PlacementStrategy,
            PLBConfig::KeyIntegerValueMap,
            placementStrategy);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyDoubleValueMap placementIncomingLoadFactor;
        placementIncomingLoadFactor.insert(make_pair(metric, 1.4));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor, PLBConfig::KeyDoubleValueMap, placementIncomingLoadFactor);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(EnableClusterSpecificInitialTemperature, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int infiniteCapacityNodeCount = 2;
        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            if (i < infiniteCapacityNodeCount)
            {
                plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L""));
            }
            else
            {
                plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
            }
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 20;
        int count = 0;
        for (int i = 0; i < serviceCount; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        // Fill nodes 1,2,3,4,5 with 85,65,45,25,5 load, respectufully
        for (int i = 1; i < 6; ++i)
        {
            for (int j = 0; j < (5 - i) * 4 + 1; ++j, ++count)
            {
                wstring serviceName = wformatString("TestService{0}", count);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
                fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0)));
            }
        }

        for (int i = 0; i < 8; ++i, ++count)
        {
            wstring serviceName = wformatString("TestService{0}", count);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("")), 1));
            fm_->FuMap.insert(make_pair(CreateGuid(count), FailoverUnitDescription(CreateGuid(count), wstring(serviceName), 0, CreateReplicas(wformatString("", i)), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        plb.Refresh(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());
        VERIFY_ARE_EQUAL(8, CountIf(actionList, ActionMatch(L"* add primary 0|1|4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(PrimaryReplicaMoveInUpgrade)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "PrimaryReplicaMoveInUpgrade");
        PLBConfigScopeChange(AllowConstraintCheckFixesDuringApplicationUpgrade, bool, true);
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/60"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/60"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(L"M1/1.0/50/30")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService1",
            L"TestType1",
            true,
            L"TestService0",
            CreateMetrics(L"M1/1.0/50/30")));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0/V, S/1/LJ"), 1, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now() + Common::TimeSpan::FromSeconds(3600));

        vector<wstring> actionList1 = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList1, ActionMatch(L"1 move primary 3=>1", value)));

        // Initial placement:
        // Nodes:  |   N0(M1:60)   |   N1(M1:100)   |   N2(M1:60)  |    N3(M1:60)   |
        // Parent: |       P       |       S        |              |                |
        // Child:  |               |                |              |       P        |
        // Other:  |               |                |              |                |
    }

    BOOST_AUTO_TEST_CASE(BatchPlacementTest1)
    {
        wstring testName = L"BatchPlacementTest1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PLBConfigScopeChange(PlacementReplicaCountPerBatch, int, 8);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 100;
        const int partitionCount = 9;
        const int instanceCount = 5;
        const int fdCount = 10;
        const int udCount = 10;

        for (int i = 0; i < nodeCount + 1; i++)
        {
            plb.UpdateNode(
                CreateNodeDescriptionWithDomainsAndCapacity(i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), L""));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int fuId = 0;

        // Service 1 need batch placment, but service 2 doesn't.
        wstring serviceName1 = wformatString("{0}_Service1", testName);
        plb.UpdateService(CreateServiceDescription(serviceName1, testType, false, CreateMetrics(L"MyMetric1/1.0/0/0")));
        wstring serviceName2 = wformatString("{0}_Service2", testName);
        plb.UpdateService(CreateServiceDescription(serviceName2, testType, false, CreateMetrics(L"MyMetric2/1.0/0/0")));

        for (int j = 0; j < partitionCount; j++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount));

            fm_->FuMap.insert(make_pair<>(
                CreateGuid(fuId),
                move(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount))));

            fuId++;
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName2), 0, CreateReplicas(L""), instanceCount));

        fm_->FuMap.insert(make_pair<>(
            CreateGuid(fuId),
            move(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName2), 0, CreateReplicas(L""), instanceCount))));

        fuId++;

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(50u, actionList.size());

        // 1 batch from the second service domain
        // 5 batches from the first service domain
        if (!PLBConfig::GetConfig().UseRGInBoost && !PLBConfig::GetConfig().UseAppGroupsInBoost)
        {
            VERIFY_ARE_EQUAL(6u, fm_->numberOfUpdatesFromPLB)
        }

        fm_->Clear();

        // Refresh again and the scheduler action should be no action
        fm_->RefreshPLB(StopwatchTime::MaxValue);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsRelaxTest1)
    {
        // The scenario is to block placement of service by already upgraded UDs and capacity
        // With RelaxConstraintForPlacement TRUE, the replica should be placed by relaxed the constraints

        wstring testName = L"PlacementWithUpgradeCompletedUDsRelaxTest1";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
            0, L"", L"UD0", L"M1/10"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
            1, L"", L"UD0", L"M1/10"));

        for (int i = 2; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString("UD{0}", int(i / 2))));
        }

        wstring applicationName = wformatString("{0}_Application", testName);
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes
            0,  // MaximumNodes
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)
            ));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestServiceWithApp", L"TestType", applicationName, true, CreateMetrics(L"M1/1/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestServiceWithApp"), 0, CreateReplicas(L""), 1));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"M1/1/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 1));

        {
            PLBConfigScopeChange(RelaxConstraintForPlacement, bool, true);

            fm_->RefreshPLB(Stopwatch::Now());
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // For partition 0, replica should be placed in UD1 since it is in completed UD, but it will violate capacity
            // After relaxed run, it partition 0 should be placed in other UDs
            VERIFY_ARE_EQUAL(2u, actionList.size());
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add * *", value)));
        }

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsRelaxTest2)
    {
        // The scenario is to make sure the relaxed placement won't cause unexpected affinity violations

        wstring testName = L"PlacementWithUpgradeCompletedUDsRelaxTest2";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestServiceP", L"TestType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestServiceC", L"TestType", true, L"TestServiceP"));

        plb.UpdateService(CreateServiceDescription(L"TestServicePlacable", L"TestType", true));

        //Parent
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestServiceP"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        //Child
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestServiceC"), 0, CreateReplicas(L"P/0, S/1, SB/3"), 1));

        {
            PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(2), wstring(L"TestServicePlacable"), 0, CreateReplicas(L""), 1));

            fm_->RefreshPLB(Stopwatch::Now());
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            //Child partition can't be placed because of the conflict of affinity and SB location
            VERIFY_ARE_EQUAL(1u, actionList.size());
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add * *", value)));
        }

        fm_->Clear();

        {
            PLBConfigScopeChange(RelaxConstraintForPlacement, bool, true);

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(2), wstring(L"TestServicePlacable"), 1, CreateReplicas(L""), 1));

            fm_->RefreshPLB(Stopwatch::Now());
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            //For partition 0, replica should be placed in UD1 out of 10 UDs since it is in completed UDs
            VERIFY_ARE_EQUAL(2u, actionList.size());
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add * 2", value)));
        }

    }

    BOOST_AUTO_TEST_CASE(PlacementWithUpgradeCompletedUDsRelaxTest3)
    {
        // The scenario cover both preferred locations and upgrade domain as soft

        wstring testName = L"PlacementWithUpgradeCompletedUDsRelaxTest3";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString(L"{0}", i/2),
                map<wstring, wstring>(), L"M1/10"));
        }

        for (int i = 6; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", wformatString(L"{0}", i/2),
                map<wstring, wstring>(), L"M1/1"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceWithUDViolation = wformatString("serviceWithUDViolation_{0}", testName);
        plb.UpdateService(CreateServiceDescription(serviceWithUDViolation, L"TestType", true, CreateMetrics(L"M1/1.0/2/2")));

        wstring servicePlacable = wformatString("servicePlacable{0}", testName);
        plb.UpdateService(CreateServiceDescription(servicePlacable, L"TestType", true, CreateMetrics(L"M1/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceWithUDViolation), 0, CreateReplicas(L"P/0, S/2, S/4"), 2));

        {
            PLBConfigScopeChange(RelaxConstraintForPlacement, bool, false);

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(2), wstring(servicePlacable), 0, CreateReplicas(L""), 1));

            fm_->RefreshPLB(Stopwatch::Now());
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            //Child partition can't be placed because of the conflict of affinity and SB location
            VERIFY_ARE_EQUAL(1u, actionList.size());
            VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"0 add * *", value)));
        }

        fm_->Clear();

        {
            PLBConfigScopeChange(RelaxConstraintForPlacement, bool, true);

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(2), wstring(servicePlacable), 1, CreateReplicas(L""), 1));

            fm_->RefreshPLB(Stopwatch::Now());
            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            //For partition 0, replica should be placed in UD1 out of 10 UDs since it is in completed UDs
            VERIFY_ARE_EQUAL(3u, actionList.size());
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add * 1|3|5", value)));
        }
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementBasicTest)
    {
        // Basic test to place container on node that has all required images
        wstring testName = L"PreferredContainerPlacementBasicTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        vector<wstring> availableImagesNode0 = { L"image1", L"image2" };
        vector<wstring> availableImagesNode1 = { L"image3", L"image4" };
        vector<wstring> availableImagesNode2 = { L"image1", L"image5" };
        vector<wstring> availableImagesNode3 = { L"image2" };

        int numOfNodes = 4;
        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateAvailableImagesPerNode(wstring(L"0"), availableImagesNode0);
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);
        plb.UpdateAvailableImagesPerNode(wstring(L"2"), availableImagesNode2);
        plb.UpdateAvailableImagesPerNode(wstring(L"3"), availableImagesNode3);

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        ServiceModel::ServicePackageIdentifier spID;
        vector<ServicePackageDescription> packages;
        packages.push_back(CreateServicePackageDescription(
            L"ServicePackageName",
            appTypeName,
            appName,
            1,
            1000,
            spID,
            move(availableImagesNode2)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"ServiceType", appName, true, CreateMetrics(L""), spID, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        // replica sould be placed on node 1 or 2 
        // as they are the only ones that satisied min bar for preferred container placement
        // size of requied images = 2 => min bar are nodes with at least half of the required size (=1)
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 0|2", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementBasicTestWithoutRG)
    {
        // Basic test that checks preferred placement for service packages
        // that don't have RG resources but have container images
        wstring testName = L"PreferredContainerPlacementBasicTestWithoutRG";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        vector<wstring> availableImagesNode0 = { L"image1", L"image2" };
        vector<wstring> availableImagesNode1 = { L"image3", L"image4" };
        vector<wstring> availableImagesNode2 = { L"image1", L"image5" };
        vector<wstring> availableImagesNode3 = { L"image2" };

        int numOfNodes = 4;
        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateAvailableImagesPerNode(wstring(L"0"), availableImagesNode0);
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);
        plb.UpdateAvailableImagesPerNode(wstring(L"2"), availableImagesNode2);
        plb.UpdateAvailableImagesPerNode(wstring(L"3"), availableImagesNode3);

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<wstring> requiredImages = { L"image4", L"image3", L"image7" };

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        ServiceModel::ServicePackageIdentifier spID;
        vector<ServicePackageDescription> packages;
        packages.push_back(CreateServicePackageDescription(
            L"ServicePackageName",
            appTypeName,
            appName,
            0,
            0,
            spID,
            move(requiredImages)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"ServiceType", appName, true, CreateMetrics(L""), spID, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        // replica sould be placed on node 1 or 2 
        // as they are the only ones that satisied min bar for preferred container placement
        // size of requied images = 2 => min bar are nodes with at least half of the required size (=1)
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementWithNodePropertiesBasicTest)
    {
        // Test containers placement with node properties
        wstring testName = L"PreferredContainerPlacementWithNodePropertiesBasicTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProp1;
        nodeProp1[L"Color"] = L"Blue";
        plb.UpdateNode(CreateNodeDescription(0, L"", L"", move(nodeProp1)));
        vector<wstring> availableImagesNode0 = { L"image1", L"image2" };
        plb.UpdateAvailableImagesPerNode(wstring(L"0"), availableImagesNode0);

        map<wstring, wstring> nodeProp2;
        nodeProp2[L"Color"] = L"Red";
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", move(nodeProp2)));
        vector<wstring> availableImagesNode1 = { L"image3", L"image5", L"image7" };
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<wstring> requiredImages = { L"image5", L"image3", L"image7" };
        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        ServiceModel::ServicePackageIdentifier spID;
        vector<ServicePackageDescription> packages;
        packages.push_back(CreateServicePackageDescription(
            L"ServicePackageName",
            appTypeName,
            appName,
            1,
            1000,
            spID,
            move(requiredImages)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithServicePackage(
            L"TestService",
            L"ServiceType",
            true,
            spID,
            CreateMetrics(L""),
            true,
            appName,
            L"Color==Blue"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        // Validate that node properties has higher priority than prefering container placement
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementWithNodeCapacityBasicTest)
    {
        // Test containers placement with node capacity
        wstring testName = L"PreferredContainerPlacementWithNodeCapacityBasicTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));

        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 1024));
        vector<wstring> availableImagesNode1 = { L"image1", L"image2" };
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);

        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 2200));
        vector<wstring> availableImagesNode2 = { L"image10"};
        plb.UpdateAvailableImagesPerNode(wstring(L"2"), availableImagesNode2);

        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 1000));
        vector<wstring> availableImagesNode3 = { L"image1" };
        plb.UpdateAvailableImagesPerNode(wstring(L"3"), availableImagesNode3);

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName0 = wformatString("{0}_AppType0", testName);
        wstring appName0 = wformatString("{0}_Application0", testName);
        vector<wstring> requiredImages0 = { L"image2", L"image1" };
        ServiceModel::ServicePackageIdentifier spID0;
        vector<ServicePackageDescription> packages0;
        packages0.push_back(CreateServicePackageDescription(
            L"ServicePackageName0",
            appTypeName0,
            appName0,
            1,
            1000,
            spID0,
            move(requiredImages0)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName0, appName0, packages0));

        wstring appTypeName1 = wformatString("{0}_AppType1", testName);
        wstring appName1 = wformatString("{0}_Application1", testName);
        vector<wstring> requiredImages1 = { L"image5", L"image6" };
        ServiceModel::ServicePackageIdentifier spID1;
        vector<ServicePackageDescription> packages1;
        packages1.push_back(CreateServicePackageDescription(
            L"ServicePackageName1",
            appTypeName1,
            appName1,
            3,
            2000,
            spID1,
            move(requiredImages1)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName1, appName1, packages1));

        wstring appTypeName2 = wformatString("{0}_AppType2", testName);
        wstring appName2 = wformatString("{0}_Application2", testName);
        vector<wstring> requiredImages2 = { L"image3"};
        ServiceModel::ServicePackageIdentifier spID2;
        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(
            L"ServicePackageName2",
            appTypeName2,
            appName2,
            1,
            500,
            spID2,
            move(requiredImages2)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName2, appName2, packages2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService0", L"ServiceType", appName0, true, CreateMetrics(L""), spID0, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"ServiceType", appName1, true, CreateMetrics(L""), spID1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"ServiceType", appName2, true, CreateMetrics(L""), spID2, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());

        // Validate that node capacity has higher priority than prefering container placement
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 1|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 2", value)));

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add * 0|1|3", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementWithNodeCapacityBasicTestWithoutRG)
    {
        // Test containers placement with node capacity
        wstring testName = L"PreferredContainerPlacementWithNodeCapacityBasicTestWithoutRG";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 5, 5000));
        vector<wstring> availableImagesNode0 = { L"image2" };
        plb.UpdateAvailableImagesPerNode(wstring(L"0"), availableImagesNode0);

        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 2000));
        vector<wstring> availableImagesNode1 = { L"image3" };
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);


        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName0 = wformatString("{0}_AppType0", testName);
        wstring appName0 = wformatString("{0}_Application0", testName);
        vector<wstring> requiredImages0 = { L"image3" };
        ServiceModel::ServicePackageIdentifier spID0;
        vector<ServicePackageDescription> packages0;
        packages0.push_back(CreateServicePackageDescription(
            L"ServicePackageName0",
            appTypeName0,
            appName0,
            0,
            0,
            spID0,
            move(requiredImages0)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName0, appName0, packages0));

        wstring appTypeName1 = wformatString("{0}_AppType1", testName);
        wstring appName1 = wformatString("{0}_Application1", testName);
        ServiceModel::ServicePackageIdentifier spID1;
        vector<ServicePackageDescription> packages1;
        packages1.push_back(CreateServicePackageDescription(
            L"ServicePackageName1",
            appTypeName1,
            appName1,
            5,
            5000,
            spID1,
            move(requiredImages0)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName1, appName1, packages1));

        wstring appTypeName2 = wformatString("{0}_AppType2", testName);
        wstring appName2 = wformatString("{0}_Application2", testName);
        vector<wstring> requiredImages2 = { L"image2" };
        ServiceModel::ServicePackageIdentifier spID2;
        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(
            L"ServicePackageName2",
            appTypeName2,
            appName2,
            1,
            500,
            spID2,
            move(requiredImages2)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName2, appName2, packages2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService0", L"ServiceType", appName0, true, CreateMetrics(L""), spID0, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"ServiceType", appName1, true, CreateMetrics(L""), spID1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"ServiceType", appName2, true, CreateMetrics(L""), spID2, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L""), 1));
        fm_->FuMap.insert(make_pair<>(CreateGuid(0),
            move(FailoverUnitDescription(
                CreateGuid(0),
                wstring(L"TestService0"),
                0,
                CreateReplicas(L""),
                1))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 1));
        fm_->FuMap.insert(make_pair<>(CreateGuid(1),
            move(FailoverUnitDescription(
                CreateGuid(1),
                wstring(L"TestService1"),
                0,
                CreateReplicas(L""),
                1))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        fm_->FuMap.insert(make_pair<>(CreateGuid(2),
            move(FailoverUnitDescription(
                CreateGuid(2),
                wstring(L"TestService2"),
                0,
                CreateReplicas(L""),
                2))));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add primary 1", value)));

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->Clear();

        // Refresh PLB to update SP loads
        plb.Refresh(Stopwatch::Now());

        ServiceModel::NodeLoadInformationQueryResult queryResult;
        ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(0), queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
        // RG System metrics(CPU and Memory) and Default metrics (PrimaryCount, ReplicaCount and Count) => count = 5
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 5);
        // CPU is fourth metric in the vector
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation[3].NodeLoad, 5);
        // Memory is last metric in the vector
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation[4].NodeLoad, 5000);

        retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(1), queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
        // RG System metrics(CPU and Memory) and Default metrics (PrimaryCount, ReplicaCount and Count) => count = 5
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 5);
        // CPU is fourth metric in the vector
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation[3].NodeLoad, 1);
        // Memory is last metric in the vector
        VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation[4].NodeLoad, 500);
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementNoBalancingTest)
    {
        // Basic test to place container on node that has all required images
        wstring testName = L"PreferredContainerPlacementNoBalancingTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int numOfNodes = 2;
        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        // Update only node0 to have images
        vector<wstring> availableImagesNode = { L"image1", L"image2", L"image3" };
        plb.UpdateAvailableImagesPerNode(wstring(L"0"), availableImagesNode);

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        ServiceModel::ServicePackageIdentifier spID;
        vector<ServicePackageDescription> packages;
        packages.push_back(CreateServicePackageDescription(
            L"ServicePackageName",
            appTypeName,
            appName,
            1,
            1000,
            spID,
            move(availableImagesNode)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"ServiceType", appName, true, CreateMetrics(L""), spID, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // replica sould be placed on node 0 because it has all necessary images
        // Verify that simulated annealing will not move replica for the replicas with preferred container placement
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementCCWithAffinityTest)
    {
        // Constraint check with affinity test
        wstring testName = L"PreferredContainerPlacementCCWithAffinityTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 5, 5096));

        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 7800));
        vector<wstring> availableImagesNode1 = { L"image3" };
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);

        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 2048));
        vector<wstring> availableImagesNode2 = { L"image1", L"image2" };
        plb.UpdateAvailableImagesPerNode(wstring(L"2"), availableImagesNode2);

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<wstring> requiredImages = { L"image1", L"image2" };
        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spIdParent;
        ServiceModel::ServicePackageIdentifier spIdChild;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageNameParent", appTypeName, appName, 3, 1024, spIdParent, move(requiredImages)));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageNameChild", appTypeName, appName, 1, 1024, spIdChild));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        wstring parentService = wformatString("{0}_ParentService", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            parentService,
            L"ServiceType",
            appName,
            true,
            CreateMetrics(L""),
            spIdParent,
            true));

        wstring childService = wformatString("{0}_ChildService", testName);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            childService,
            L"ServiceType",
            true,
            parentService,
            CreateMetrics(L""),
            false,
            0,
            false,
            true,
            spIdChild));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentService), 0, CreateReplicas(L""), 1));
        fm_->FuMap.insert(make_pair<>(CreateGuid(0),
            move(FailoverUnitDescription(
                CreateGuid(0),
                wstring(parentService),
                0,
                CreateReplicas(L""),
                1))));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(childService), 0, CreateReplicas(L""), 1));
        fm_->FuMap.insert(make_pair<>(CreateGuid(1),
            move(FailoverUnitDescription(
                CreateGuid(1),
                wstring(childService),
                0,
                CreateReplicas(L""),
                1))));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // Both parent and child are placed on the same node 
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 2", value)));

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->Clear();

        // Update child to report more CPU and memory so that creates node capacity violation
        vector<ServicePackageDescription> upgradedPackages;
        upgradedPackages.push_back(CreateServicePackageDescription(L"ServicePackageNameParent", appTypeName, appName, 3, 1024, spIdParent, move(requiredImages)));
        upgradedPackages.push_back(CreateServicePackageDescription(L"ServicePackageNameChild", appTypeName, appName, 2, 2000, spIdChild));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, upgradedPackages));

        fm_->RefreshPLB(Stopwatch::Now());

        // Check that CC has been fixed by moving replicas to another node
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 2=>0|1", value))); 
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 2=>0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(PreferredContainerPlacementBalanceOnSimilarNodesTest)
    {
        wstring testName = L"PreferredContainerPlacementBalanceOnSimilarNodesTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int numOfNodes = 4;
        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        vector<wstring> availableImagesNode1 = { L"image1", L"image2", L"image3" };
        plb.UpdateAvailableImagesPerNode(wstring(L"1"), availableImagesNode1);

        vector<wstring> availableImagesNode2 = { L"image2", L"image3", L"image4" };
        plb.UpdateAvailableImagesPerNode(wstring(L"2"), availableImagesNode2);

        vector<wstring> availableImagesNode3 = { L"image4" };
        plb.UpdateAvailableImagesPerNode(wstring(L"3"), availableImagesNode3);

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<wstring> requiredImages = { L"image1",L"image2", L"image3", L"image4" };
        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        ServiceModel::ServicePackageIdentifier spID;
        vector<ServicePackageDescription> packages;
        packages.push_back(CreateServicePackageDescription(
            L"ServicePackageName",
            appTypeName,
            appName,
            1,
            1000,
            spID,
            move(requiredImages)));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService", L"ServiceType", appName, true, CreateMetrics(L""), spID, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // Node 3 will be discarded as it has only 1 required image and min bar is 2
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 1|2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementOnNodeWithShouldDisapperLoadTest)
    {
        PlacementOnNodeWithShouldDisapperLoadTestHelper(L"PlacementOnNodeWithShouldDisapperLoadTest", true);
    }

    BOOST_AUTO_TEST_CASE(PlacementOnNodeWithShouldDisapperLoadTestFeatureSwitchOff)
    {
        PlacementOnNodeWithShouldDisapperLoadTestHelper(L"PlacementOnNodeWithShouldDisapperLoadTestFeatureSwitchOff", false);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestPLBPlacement::ClassSetup()
    {
        Trace.WriteInfo("PLBPlacementTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        upgradingFlag_.SetUpgrading(true);
        swappingPrimaryFlag_.SetSwappingPrimary(true);

        return TRUE;
    }

    bool TestPLBPlacement::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBPlacement::ClassCleanup()
    {
        Trace.WriteInfo("PLBPlacementTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }

    void TestPLBPlacement::PlacementOnNodeWithShouldDisapperLoadTestHelper(wstring const& testName, bool featureSwitch)
    {
        // 3 nodes with capacity 90 each:
        //  Node 0: 0 load, 50 disappearing load
        //  Node 1: 50 load
        //  Node 2: 40 load
        // New replica placement asking for 50, and the best target for balance is node 0.
        // Feature swtich on: do not allow placement on node 0, off: allow placement on node 0.
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(CountDisappearingLoadForSimulatedAnnealing, bool, featureSwitch);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"IDSU/90"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"IDSU/90"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"IDSU/90"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"IDSU/1.0/50/50")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"IDSU/1.0/50/50")));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"IDSU/1.0/40/40")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0/V,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        int expectedOnTwo = featureSwitch ? 1 : 0;
        int expectedOnZero = featureSwitch ? 0 : 1;
        VERIFY_ARE_EQUAL(expectedOnTwo, CountIf(actionList, ActionMatch(L"1 add primary 2", value)));
        VERIFY_ARE_EQUAL(expectedOnZero, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }


    void TestPLBPlacement::PlacementWithExistingReplicaMoveThrottleTestHelper(bool placementExpected)
    {
        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, true);
        PLBConfigScopeChange(MaxPercentageToMoveForPlacement, double, 0.5);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Create 5 nodes with capaticy 10
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/2/2")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/9/9")));

        // Each node has two replica and load of 4, both replica need to be moved out to place the big replica of load 9
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/4, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Refresh again and the scheduler action should be CreationWithMove
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"NewReplicaPlacementWithMove");
        if (placementExpected)
        {
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add primary *", value)));
        }
        else
        {
            VERIFY_ARE_EQUAL(0, actionList.size());
        }
    }

    void TestPLBPlacement::PlacementWithUpgradeAndStandByReplicasHelper(wstring testName, PlacementAndLoadBalancing & plb)
    {
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/100"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/50"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(6, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(7, L"MyMetric/200"));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/100/50")));

        // add primary on node 3 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/2, SB/3/J"), 1, upgradingFlag_));

        // add secondary on node 5 expected
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/4, SB/5/K"), 1, upgradingFlag_));

        // no operation expected as there is no secondary to do the swap-out
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/6/I, SB/7"), 0, upgradingFlag_));

        // no operation expected as there is no secondary to do the swap-back
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, SB/1/J"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add primary 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add secondary 5", value)));
    }

    void TestPLBPlacement::PlacementWithUpgradeWithDeactivatingNodesAndWithChildSt1Helper(
        wstring testName,
        PlacementAndLoadBalancing & plb,
        wstring & volatileParentService,
        wstring & volatileChildService,
        wstring & persistedParentService,
        wstring & persistedChildService)
    {
        PLBConfigScopeChange(CheckAlignedAffinityForUpgrade, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        int nodeId = 0;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::Restart,
            Reliability::NodeDeactivationStatus::DeactivationComplete));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            nodeId++, L"", L"",
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        volatileParentService = wformatString("{0}_VolatileParentService", testName);
        volatileChildService = wformatString("{0}_VolatileChildService", testName);
        persistedParentService = wformatString("{0}_PersistedParentService", testName);
        persistedChildService = wformatString("{0}_PersistedChildService", testName);
        plb.UpdateService(CreateServiceDescription(volatileParentService, parentType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(volatileChildService, childType, true, volatileParentService, CreateMetrics(L""), false, 1, false));
        plb.UpdateService(CreateServiceDescription(persistedParentService, parentType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(persistedChildService, childType, true, persistedParentService, CreateMetrics(L""), false, 1, true));
    }

    void TestPLBPlacement::PlacementMissingReplicasWithQuorumBasedDomainHelper(PlacementAndLoadBalancing & plb, bool fdTreeExists)
    {
        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        Reliability::FailoverUnitFlags::Flags noneFlag_;

        // stateFull services - target 7
        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 7));
        // replica difference is 2 - we expect to add 2 secondary replicas in the first domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/4,S/5, S/6"), 2));

        // stateFull services - target 7, non-packing
        serviceName = L"StatefulNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 7));
        // replica difference is 5
        //  - we expect to add 1 secondary replicas in the second domain if FD tree exists
        //  - otherwise we expect to add 4 replicas, 2 in the first and 2 in the second domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0, S/6"), 5));

        // stateless services- target 7
        serviceName = L"StatelessPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 7));
        // replica difference is 2 - we expect to add 2 secondary replicas in the first domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4,I/5, I/6"), 2));

        // stateless services- target 5, non-packing
        serviceName = L"StatelessNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, false, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 7));
        // replica difference is 5
        //  - we expect to add 1 secondary replicas in the second domain if FD tree exists
        //  - otherwise we expect to add 4 replicas, 2 in the first and 2 in the second domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"I/0, I/6"), 5));

        // stateless services- target -1
        serviceName = L"StatelessOnEveryNodeService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), true, INT_MAX));
        // replica difference is INT_MAX - we expect to add 3 secondary replicas in the first domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4,I/5, I/6"), INT_MAX));

        // stateFull services - target 6
        serviceName = L"StatefulPackingService6";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 6));
        // replica difference is 2 - we expect to add 1 secondary replicas in the first domain
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(L"P/0, S/4,S/5, S/6"), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        if (fdTreeExists)
        {
            VERIFY_ARE_EQUAL(10u, actionList.size());
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add secondary 1|2|3", value)));
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 4|5", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add instance 1|2|3", value)));
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 add instance 4|5", value)));
            VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"4 add instance 1|2|3", value)));
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add secondary 1|2|3", value)));
        }
        else
        {
            VERIFY_ARE_EQUAL(16u, actionList.size());
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 add secondary 1|2|3", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add secondary 1|2|3", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add secondary 4|5", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add instance 1|2|3", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"3 add instance 1|2|3", value)));
            VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"3 add instance 4|5", value)));
            VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"4 add instance 1|2|3", value)));
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 add secondary 1|2|3", value)));
        }
    }

    void TestPLBPlacement::NodeBufferPercentageWithPrimaryToBePlacedTest()
    {
        Trace.WriteInfo("PLBPlacementTestSource", "NodeBufferPercentageWithPrimaryToBePlacedTest");
        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap , nodeBufferPercentageMap);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // J is the flag used by FM to mark PrimaryUpgradeLocation
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1/J"), 0, upgradingFlag_));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Secondary replica on node 1 should be upgraded to primary although it will violate NodeBufferPercentage
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1", value)));
    }

    void TestPLBPlacement::UseHeuristicDuringPlacementOfHugeBatchTest()
    {
        wstring testName = L"UseHeuristicDuringPlacementOfHugeBatchTest";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        uint nodeCapacity = 200;
        uint replicaLoad = 1;
        uint loadWeight = 1;
        int numOfNodes = 100;

        double percentOfFullNodes = 0.6;
        int numOfNewReplicas = 1000;
        int numOfFullNodes = (int)(percentOfFullNodes * (double)numOfNodes);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring defragMetric = L"DefragMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyDoubleValueMap placementHeuristicIncomingLoadFactor;
        placementHeuristicIncomingLoadFactor.insert(make_pair(defragMetric, 1.3));
        PLBConfigScopeChange(PlacementHeuristicIncomingLoadFactor,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicIncomingLoadFactor);

        PLBConfig::KeyDoubleValueMap placementHeuristicEmptySpacePercent;
        placementHeuristicEmptySpacePercent.insert(make_pair(defragMetric, 0.0));
        PLBConfigScopeChange(PlacementHeuristicEmptySpacePercent,
            PLBConfig::KeyDoubleValueMap,
            placementHeuristicEmptySpacePercent);

        for (int i = 0; i < numOfNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                i,
                wformatString("dc0/r{0}", i%10),
                wformatString("{0}/{1}",
                defragMetric,
                StringUtility::ToWString<uint>(nodeCapacity))));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        for (int i = 0; i < numOfFullNodes; i++)
        {
            for (int j = 0; j < (int)nodeCapacity; j++)
            {
                plb.UpdateService(CreateServiceDescription(
                    wformatString("I_AM_SERVICE_ON_NODE_{0}_RB_{1}", i + 1, j + 1),
                    L"TestType",
                    true,
                    CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                    defragMetric,
                    StringUtility::ToWString<uint>(loadWeight),
                    StringUtility::ToWString<uint>(replicaLoad)))));
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(i * nodeCapacity + j + 1),
                    wformatString("I_AM_SERVICE_ON_NODE_{0}_RB_{1}", i + 1, j + 1),
                    0,
                    CreateReplicas(wformatString("P/{0}", StringUtility::ToWString<int>(i))),
                    0));
                fm_->FuMap.insert(make_pair<>(CreateGuid(i + 1),
                    move(FailoverUnitDescription(
                    CreateGuid(i * nodeCapacity + j + 1),
                    wformatString("I_AM_SERVICE_ON_NODE_{0}_RB_{1}", i + 1, j + 1),
                    0,
                    CreateReplicas(wformatString("P/{0}", StringUtility::ToWString<int>(i))),
                    0))));
            }
        }
        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        for (int i = 0; i < numOfNewReplicas; i++)
        {
            plb.UpdateService(CreateServiceDescription(
                wformatString("I_AM_PLACEMENT_SERVICE_{0}", i),
                L"TestType",
                true,
                CreateMetrics(wformatString("{0}/{1}/{2}/{2}",
                defragMetric,
                StringUtility::ToWString<uint>(loadWeight),
                StringUtility::ToWString<uint>(replicaLoad)))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i + numOfNodes*nodeCapacity),
                wformatString("I_AM_PLACEMENT_SERVICE_{0}", i),
                0,
                CreateReplicas(L""),
                1));
            fm_->FuMap.insert(make_pair<>(CreateGuid(i + numOfNodes*nodeCapacity),
                move(FailoverUnitDescription(
                CreateGuid(i + numOfNodes*nodeCapacity),
                wformatString("I_AM_PLACEMENT_SERVICE_{0}", i),
                0,
                CreateReplicas(L""),
                1))));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), numOfNewReplicas);
        fm_->ApplyActions();
        fm_->UpdatePlb();

        //Get plb snapshot
        fm_->RefreshPLB(Stopwatch::Now());

        int numOfEmptyNodes = 0;
        for (int i = 0; i < numOfNodes; i++)
        {
            ServiceModel::NodeLoadInformationQueryResult queryResult;
            ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(i), queryResult);
            VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
            VERIFY_ARE_EQUAL(queryResult.NodeLoadMetricInformation.size(), 1);
            if (queryResult.NodeLoadMetricInformation[0].NodeLoad == 0)
            {
                numOfEmptyNodes++;
            }
        }

        int expectedNumberOfFreeNodes = (numOfNodes - static_cast<int>(numOfNodes*percentOfFullNodes)) - static_cast<int>(ceil((numOfNewReplicas / static_cast<double>(nodeCapacity)) * 1.3));
        VERIFY_IS_TRUE(numOfEmptyNodes == expectedNumberOfFreeNodes);
    }
}
