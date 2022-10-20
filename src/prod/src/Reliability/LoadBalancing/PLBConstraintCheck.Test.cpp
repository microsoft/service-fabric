// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"
#include "TestFM.h"
#include "PLBConfig.h"
#include "PLBDiagnostics.h"
#include "PlacementAndLoadBalancing.h"
#include "IFailoverManager.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "math.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability::LoadBalancingComponent;


    class TestPLBConstraintCheck
    {
    protected:
        TestPLBConstraintCheck() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBConstraintCheck()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        void ConstraintCheckWithQuorumBasedDomainNoViolationsHelper(PlacementAndLoadBalancing & plb);
        void ConstraintCheckWithQuorumBasedDomainWithViolationsHelper(PlacementAndLoadBalancing & plb);

        void ConstraintCheckWithShouldDisappearLoadAndReservationTestHelper(wstring const& testName, bool featureSwitch);
        void ConstraintCheckWithShouldDisappearLoadTestHelper(wstring const& testName, bool featureSwitch);

        shared_ptr<TestFM> fm_;

        void CheckLoadValue(std::wstring &serviceName, int fuId, std::wstring &metric, ReplicaRole::Enum role,
            uint load, int nodeId = -1, bool GetSecondaryLoad = false)
        {
            Trace.WriteInfo("PLBTest.CheckLoadValue", "CheckLoadValue: replica {0}:{1}:{2} node: {3} metric: {4} load: {5}",
                serviceName, CreateGuid(fuId), role, CreateNodeId(nodeId), metric, load);

            NodeId nodeIdGuid = CreateNodeId(nodeId == -1 ? 0 : nodeId);
            VERIFY_IS_TRUE(fm_->PLBTestHelper.CheckLoadValue(serviceName, CreateGuid(fuId), metric, role, load, nodeIdGuid, GetSecondaryLoad));
        }
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBConstraintCheckSuite, TestPLBConstraintCheck)

        BOOST_AUTO_TEST_CASE(ConstraintCheckServiceTypeBlockListTest)
    {
            Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckServiceTypeBlockListTest: service type disabled on some nodes, but existing replicas should not be moved out");

            PlacementAndLoadBalancing & plb = fm_->PLB;

            for (int i = 0; i < 5; i++)
            {
                plb.UpdateNode(CreateNodeDescription(i));
            }

            set<NodeId> blockList;
            blockList.insert(CreateNodeId(1));
            blockList.insert(CreateNodeId(3));
            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
            plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType1", true));
            set<NodeId> blockList2;
            blockList2.insert(CreateNodeId(2));
            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
            plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType2", false));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, P/1, S/2, S/3, S/4"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, S/1, P/2, S/3, S/4"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, S/1, S/2, P/3, S/4"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"S/0, S/1, S/2, S/3, P/4"), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1, I/2, I/3, I/4"), 0));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VERIFY_ARE_EQUAL(0u, actionList.size());
        }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/50", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));
        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);

        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/100/0", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        // Secondary on 2 becomes StandBy replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1,SB/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 secondary default load is 0 and secondary load on node 1 is increased to 100, over node capacity,
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 100));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 100, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 replicas should be moved out - in order to make artificial move from node 1 to node where StandBy replica is.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Memory structures check before invalidating the move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 100, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 100, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);

        // Invalidate the move and drop StandBy replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/1,D/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Refresh - in order to get the assert.
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest2)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest2";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/50", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/100/0", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 secondary default load is 0 and secondary load on node 1 is increased to 100, over node capacity,
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 100));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 100, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 replicas should be moved out - in order to make artificial move from node 1 to node where StandBy replica is.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Memory structures check before invalidating the move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 100, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 100, 1);
        // Secondary map should still has only one entry for node 1
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        // Check GetSecondaryLoad value
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 100, 2, true);

        // Move in progress
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1,IB/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Refresh - in order to get the assert.
        fm_->RefreshPLB(Stopwatch::Now());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/1,D/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        actionList = GetActionListString(fm_->MoveActions);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);

        // Refresh - in order to get the assert.
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest3)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndStandByReplicasLoadTest3";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/50", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/100/100", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 secondary default load is 0 and secondary load on node 1 is increased to 50, over node capacity,
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 60));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 100, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 replicas should be moved out - in order to make artificial move from node 1 to node where StandBy replica is.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Memory structures check before invalidating the move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 100, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 60, 1);

        // Move Completes, source on standby
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,SB/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Refresh - to update that replica is up.
        fm_->RefreshPLB(Stopwatch::Now());

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 60, 2);

        // Target on standby
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,SB/1,SB/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();


        // Refresh - to update that replica is SB.
        fm_->RefreshPLB(Stopwatch::Now());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 3, CreateReplicas(L"P/0,SB/1,D/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();


        // Refresh - in order to try and get the assert.
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckServiceTypeBlockListTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckServiceTypeBlockListTest2: service type disabled on some nodes, so constraint check shouldn't move existing replicas to those nodes");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"Count/1"));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(5));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType1", true));
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(5));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType2", false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"I/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * *=>5", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/r3", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/r2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/r3", L"ud1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainViolationsTest3)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainViolationsTest3");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/r3", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/r1", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/r2", L"ud1"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/r3", L"ud1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/4"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VERIFY_ARE_EQUAL(1u, actionList.size());
        }

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndOtherViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndOtherViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodePropertiesRed0, nodePropertiesRed1, nodePropertiesRed2, nodePropertiesRed3,
            nodePropertiesBlue0, nodePropertiesBlue1, nodePropertiesBlue2, nodePropertiesBlue3, nodePropertiesBlue4;

        //Service domain needs to be initialized and created for the updatenodes to register the new node up
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        fm_->RefreshPLB(Stopwatch::Now());

        nodePropertiesRed0.insert(make_pair(L"NodeType", L"red"));
        nodePropertiesRed1.insert(make_pair(L"NodeType", L"red"));
        nodePropertiesRed2.insert(make_pair(L"NodeType", L"red"));
        nodePropertiesRed3.insert(make_pair(L"NodeType", L"red"));
        nodePropertiesBlue0.insert(make_pair(L"NodeType", L"blue"));
        nodePropertiesBlue1.insert(make_pair(L"NodeType", L"blue"));
        nodePropertiesBlue2.insert(make_pair(L"NodeType", L"blue"));
        nodePropertiesBlue3.insert(make_pair(L"NodeType", L"blue"));
        nodePropertiesBlue4.insert(make_pair(L"NodeType", L"blue"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0", L"ud0", move(nodePropertiesRed0)));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r1", L"ud0", move(nodePropertiesRed1)));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r2", L"ud1", move(nodePropertiesRed2)));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/r3", L"ud1", move(nodePropertiesRed3)));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r0", L"ud0", move(nodePropertiesBlue0)));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/r1", L"ud0", move(nodePropertiesBlue1)));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/r2", L"ud1", move(nodePropertiesBlue2)));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/r3", L"ud1", move(nodePropertiesBlue3)));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());


        plb.UpdateService(ServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType"),
            wstring(L""),
            true,                               //bool isStateful,
            wstring(L"NodeType==blue"),       //placementConstraints,
            wstring(),                          //affinitizedService,
            true,                               //alignedAffinity,
            vector<ServiceMetric>(),
            FABRIC_MOVE_COST_ZERO,
            false,
            1,                                // partition count
            4,                                  // target replica set size
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));

        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/4"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0)));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(4u, actionList0.size());
        }

        //Updating FailoverUnit and Adding a node to force a clear of the the moveplan, so the next refresh cycle doesn't result in a skip
        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->ClearMoveActions();

        plb.ProcessPendingUpdatesPeriodicTask();

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            VERIFY_ARE_EQUAL(1u, actionList.size());
        }
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainViolationsTest2)
{
    Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainViolationsTest2");
    PlacementAndLoadBalancing & plb = fm_->PLB;

    int index = 0;
    plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
    plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));

    set<NodeId> blockList;
    blockList.insert(CreateNodeId(8));
    blockList.insert(CreateNodeId(9));
    blockList.insert(CreateNodeId(10));
    blockList.insert(CreateNodeId(11));
    blockList.insert(CreateNodeId(12));
    blockList.insert(CreateNodeId(13));
    blockList.insert(CreateNodeId(14));
    blockList.insert(CreateNodeId(15));

    plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));
    plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
    plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0"));

    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/17, S/4, S/5"), 0));
    plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/17, S/4, S/5"), 0));

    fm_->RefreshPLB(Stopwatch::Now());

    vector<wstring> actionList = GetActionListString(fm_->MoveActions);
    VERIFY_ARE_EQUAL(2u, actionList.size());
}

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainViolationsTest4)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainViolationsTest4");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4"));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(8));
        blockList.insert(CreateNodeId(9));
        blockList.insert(CreateNodeId(10));
        blockList.insert(CreateNodeId(11));
        blockList.insert(CreateNodeId(12));
        blockList.insert(CreateNodeId(13));
        blockList.insert(CreateNodeId(14));
        blockList.insert(CreateNodeId(15));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/17, S/4, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/17, S/4, S/5"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckUpgradeDomainViolationWithFaultDomainTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckUpgradeDomainViolationWithFaultDomainTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 5);

        //plb.UpdateNode(CreateNodeDescription(0, L"0/0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"0/10", L"1"));
        plb.UpdateNode(CreateNodeDescription(2, L"0/15", L"6"));
        plb.UpdateNode(CreateNodeDescription(3, L"0/20", L"2"));
        plb.UpdateNode(CreateNodeDescription(4, L"0/25", L"7"));
        plb.UpdateNode(CreateNodeDescription(5, L"0/5", L"5"));
        plb.UpdateNode(CreateNodeDescription(6, L"1/1", L"1"));
        plb.UpdateNode(CreateNodeDescription(7, L"1/11", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"1/16", L"7"));
        //plb.UpdateNode(CreateNodeDescription(9, L"1/21", L"3"));
        plb.UpdateNode(CreateNodeDescription(10, L"1/26", L"8"));
        plb.UpdateNode(CreateNodeDescription(11, L"1/6", L"6"));
        plb.UpdateNode(CreateNodeDescription(12, L"2/12", L"3"));
        plb.UpdateNode(CreateNodeDescription(13, L"2/17", L"8"));
        plb.UpdateNode(CreateNodeDescription(14, L"2/2", L"2"));
        plb.UpdateNode(CreateNodeDescription(15, L"2/22", L"4"));
        plb.UpdateNode(CreateNodeDescription(16, L"2/27", L"9"));
        plb.UpdateNode(CreateNodeDescription(17, L"2/7", L"7"));
        plb.UpdateNode(CreateNodeDescription(18, L"3/13", L"4"));
        plb.UpdateNode(CreateNodeDescription(19, L"3/18", L"9"));
        plb.UpdateNode(CreateNodeDescription(20, L"3/23", L"5"));
        //plb.UpdateNode(CreateNodeDescription(21, L"3/28", L"0"));
        plb.UpdateNode(CreateNodeDescription(22, L"3/3", L"3"));
        plb.UpdateNode(CreateNodeDescription(23, L"3/8", L"8"));
        plb.UpdateNode(CreateNodeDescription(24, L"4/14", L"5"));
        //plb.UpdateNode(CreateNodeDescription(25, L"4/19", L"0"));
        plb.UpdateNode(CreateNodeDescription(26, L"4/24", L"6"));
        plb.UpdateNode(CreateNodeDescription(27, L"4/29", L"1"));
        plb.UpdateNode(CreateNodeDescription(28, L"4/4", L"4"));
        plb.UpdateNode(CreateNodeDescription(29, L"4/9", L"9"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/7, S/3, S/17, S/23, S/27"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckUpgradeDomainViolationWithFaultDomainTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckUpgradeDomainViolationWithFaultDomainTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 5);

        //plb.UpdateNode(CreateNodeDescription(0, L"0/0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"0/10", L"1"));
        plb.UpdateNode(CreateNodeDescription(2, L"0/15", L"6"));
        plb.UpdateNode(CreateNodeDescription(3, L"0/20", L"2"));
        plb.UpdateNode(CreateNodeDescription(4, L"0/25", L"7"));
        plb.UpdateNode(CreateNodeDescription(5, L"0/5", L"5"));
        plb.UpdateNode(CreateNodeDescription(6, L"1/1", L"1"));
        plb.UpdateNode(CreateNodeDescription(7, L"1/11", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"1/16", L"7"));
        //plb.UpdateNode(CreateNodeDescription(9, L"1/21", L"3"));
        plb.UpdateNode(CreateNodeDescription(10, L"1/26", L"8"));
        plb.UpdateNode(CreateNodeDescription(11, L"1/6", L"6"));
        plb.UpdateNode(CreateNodeDescription(12, L"2/12", L"3"));
        plb.UpdateNode(CreateNodeDescription(13, L"2/17", L"8"));
        plb.UpdateNode(CreateNodeDescription(14, L"2/2", L"2"));
        plb.UpdateNode(CreateNodeDescription(15, L"2/22", L"4"));
        plb.UpdateNode(CreateNodeDescription(16, L"2/27", L"9"));
        plb.UpdateNode(CreateNodeDescription(17, L"2/7", L"7"));
        plb.UpdateNode(CreateNodeDescription(18, L"3/13", L"4"));
        plb.UpdateNode(CreateNodeDescription(19, L"3/18", L"9"));
        plb.UpdateNode(CreateNodeDescription(20, L"3/23", L"5"));
        //plb.UpdateNode(CreateNodeDescription(21, L"3/28", L"0"));
        plb.UpdateNode(CreateNodeDescription(22, L"3/3", L"3"));
        plb.UpdateNode(CreateNodeDescription(23, L"3/8", L"8"));
        plb.UpdateNode(CreateNodeDescription(24, L"4/14", L"5"));
        //plb.UpdateNode(CreateNodeDescription(25, L"4/19", L"0"));
        plb.UpdateNode(CreateNodeDescription(26, L"4/24", L"6"));
        plb.UpdateNode(CreateNodeDescription(27, L"4/29", L"1"));
        plb.UpdateNode(CreateNodeDescription(28, L"4/4", L"4"));
        plb.UpdateNode(CreateNodeDescription(29, L"4/9", L"9"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/7, S/3, S/17, S/23, S/27"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainBlockList)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainBlockList");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r4"));


        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/5,S/6"), 0));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));

        fm_->RefreshPLB(Stopwatch::Now());

        //replicas violating placement constraints or are placed on block listed nodes should not cause fault domain violations
        //replicas on block listed nodes cannot be moved so placement constraint wont do anything
        //the idea is for replicas on invalid nodes placement constraint should fix them not fault domain
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainShouldRun)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainShouldRun");
        PLBConfigScopeChange(PlacementConstraintPriority, int, -1);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/r1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/r2"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/r3"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/r4"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/r2"));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //fault domain constraint should trigger here because the ration is 4 to 1, the replica to be marked as invalid should be on the node 2
        VerifyPLBAction(plb, L"ConstraintCheck");
        //we cannot move blocklisted replicas so no moves are generated here!
        VERIFY_ARE_EQUAL(actionList.size(), 0u);
    }


    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainEmptyNonpacking)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainEmptyNonpacking");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PlacementConstraintPriority, int, -1);

        map<wstring, wstring> nodeProperties1;
        nodeProperties1.insert(make_pair(L"Color", L"blue"));

        map<wstring, wstring> nodeProperties2;
        nodeProperties2.insert(make_pair(L"Color", L"red"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/r1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L""));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateNode(CreateNodeDescription(1, L"dc1/r0", L"", map<wstring, wstring>(nodeProperties1)));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/r1", L"", map<wstring, wstring>(nodeProperties1)));
        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0", L"", move(nodeProperties2)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true, L"Color==red&&FDPolicy~Nonpacking"));

        fm_->RefreshPLB(Stopwatch::Now());

        //nonpacking should not consider empty nodes (the blue ones here)
        //placement constraint is disabled so no constaint check should run
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultUpgradeDomainEmptyDeactivation)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultUpgradeDomainEmptyDeactivation");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0", L"ud0"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(index++, L"dc1", L"ud1", L"", Reliability::NodeDeactivationIntent::Enum::Restart, Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        //fault and upgrade domain constraints should neglect domains with zero nodes
        //nodes that are deactivated should not be considered for FD/UD
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSingleReplicaBlockList)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSingleReplicaBlockList");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));

        fm_->RefreshPLB(Stopwatch::Now());

        //singleton cannot cause fault domain violation
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckRandomBlocklistNodesFaultDomain)
    {
        wstring testName = L"ConstraintCheckRandomBlocklistNodesFaultDomain";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        Random random;
        PLBConfigScopeChange(InitialRandomSeed, int, random.Next());

        fm_->Load();
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = 0;
        int numIterations = 50;
        int placement[5][3];
        int nodes[15];
        int noReplica = 0, validReplica = 1, invalidReplica = 2;

        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/rack2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2/rack1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc2/rack2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3/rack0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3/rack1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc3/rack2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4/rack0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4/rack1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc4/rack2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        auto now = Stopwatch::Now();
        auto FUversion = 0;
        for (int i = 0; i < numIterations; ++i)
        {
            set<NodeId> blockList;

            for (int j = 0; j < 5; ++j)
            {
                for (int k = 0; k < 3; ++k)
                {
                    placement[j][k] = noReplica;
                }
            }

            for (int j = 0; j < index; ++j)
            {
                nodes[j] = j;
            }

            int numOfReplicas = random.Next() % index;
            int currentNodeMax = 15;

            for (int j = 0; j < numOfReplicas; ++j)
            {
                int position = random.Next() % currentNodeMax;
                int node = nodes[position];
                int temp = nodes[currentNodeMax - 1];
                if (random.Next() % 2 == 0)
                {
                    placement[node / 3][node % 3] = validReplica;
                }
                else
                {
                    blockList.insert(CreateNodeId(node));
                    placement[node / 3][node % 3] = invalidReplica;
                }
                nodes[position] = temp;
                --currentNodeMax;
            }

            wstring replicas = L"";
            int currentReplicaCount = numOfReplicas;
            for (int j = 0; j < 5; ++j)
            {
                for (int k = 0; k < 3; ++k)
                {
                    if (placement[j][k] != noReplica)
                    {
                        replicas += wformatString("I/{0}", j * 3 + k);
                        if (currentReplicaCount > 1)
                        {
                            replicas += L",";
                        }
                        --currentReplicaCount;
                    }
                }
            }

            int totalCnt = 0;
            int domainCnt = 0;
            int domains[5];
            bool validDomains[5];

            for (int j = 0; j < 5; ++j)
            {
                domains[j] = 0;
            }

            for (int j = 0; j < 5; ++j)
            {
                int cnt = 0;
                bool isValid = false;
                validDomains[j] = false;
                for (int k = 0; k < 3; ++k)
                {
                    if (placement[j][k] != invalidReplica)
                    {
                        isValid = true;
                    }
                    if (placement[j][k] != noReplica)
                    {
                        ++cnt;
                        ++domains[j];
                    }
                }
                if (isValid)
                {
                    validDomains[j] = true;
                    totalCnt += cnt;
                    ++domainCnt;
                }

            }
            int base = totalCnt / domainCnt;

            int overflowCnt = 0;
            int underflowCnt = 0;
            int removeCnt = 0;
            for (int j = 0; j < 5; ++j)
            {
                if (!validDomains[j])
                {
                    continue;
                }
                if (domains[j] > base + 1)
                {
                    overflowCnt += domains[j] - base - 1;
                }
                if (domains[j] < base)
                {
                    underflowCnt += base - domains[j];
                }
            }

            removeCnt = max(underflowCnt, overflowCnt);

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), FUversion++, CreateReplicas(replicas), 0));
            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));

            plb.PlbDiagnosticsSPtr->SchedulersDiagnostics->setKeepHistory(true);
            now += PLBConfig::GetConfig().MinConstraintCheckInterval;
            fm_->RefreshPLB(now);

            auto domainId = plb.GetServiceDomains().at(0).domainId_;
            domainId += L"_PrimaryCount";
            auto serviceDomainData = plb.PlbDiagnosticsSPtr->SchedulersDiagnostics->GetLatestStageDiagnostics(domainId);

            vector<int> badReplicas;
            if (serviceDomainData.get() != nullptr)
            {
                auto constraintsData = serviceDomainData->constraintsDiagnosticsData_;
                for (auto constraint : constraintsData)
                {
                    for (auto & info : constraint->basicDiagnosticsInfo_)
                    {
                        for (auto & node : info.Nodes)
                        {
                            int nodeIndex = static_cast<int>(node.first.IdValue.Low);
                            badReplicas.push_back(nodeIndex);
                        }
                    }
                }

                auto compareFunc = [&placement](int a, int b) {return placement[a / 3][a % 3] > placement[b / 3][b % 3]; };
                //process invalid and then valid replicas
                sort(badReplicas.begin(), badReplicas.end(), compareFunc);

                for (int replica : badReplicas)
                {
                    int domainIdIndex = replica / 3;
                    ASSERT_IFNOT(domains[domainIdIndex] > base, "Count must be greater than base");
                    ASSERT_IFNOT(validDomains[domainIdIndex], "Replica should not be invalid if the whole domain is invalid");
                    int status = placement[domainIdIndex][replica % 3];
                    ASSERT_IF(status == noReplica, "This cannot be an invalid replica");
                    if (status == validReplica)
                    {
                        for (int k = 0; k < 3; ++k)
                        {
                            ASSERT_IF(placement[domainIdIndex][k] == invalidReplica, "We should remove invalid replicas first");
                        }
                    }
                    placement[domainIdIndex][replica % 3] = noReplica;
                    --domains[domainIdIndex];
                    --removeCnt;
                    ASSERT_IF(removeCnt < 0, "We removed too many replicas");
                }

            }
            plb.PlbDiagnosticsSPtr->SchedulersDiagnostics->setKeepHistory(false);
            plb.PlbDiagnosticsSPtr->SchedulersDiagnostics->CleanupStageDiagnostics();
            ASSERT_IFNOT(removeCnt == 0, "We have not removed all invalid replicas");

            now += PLBConfig::GetConfig().MinPlacementInterval;
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), FUversion++, CreateReplicas(replicas), random.Next() % 5));
            //check that we do not assert here
            fm_->RefreshPLB(now);
        }

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDuringApplicationUpgradeTest)
    {
        wstring testName = L"ConstraintCheckDuringApplicationUpgradeTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(AllowConstraintCheckFixesDuringApplicationUpgrade, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring metric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/20", metric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/120", metric)));

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        int index = 0;
        // Regular service on node with P/I replica
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, true, CreateMetrics(wformatString("{0}/1.0/30/30", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        // Service with I primary and with inTransition and inUpgrade partition
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, true, CreateMetrics(wformatString("{0}/1.0/30/30", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag));

        // Service with J secondary and with inTransition and inUpgrade partition
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, true, CreateMetrics(wformatString("{0}/1.0/30/30", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0/J,P/1,S/2"), 0, upgradingFlag));

        // Service with J secondary and with inTransition and inUpgrade partition
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceTypeName, true, CreateMetrics(wformatString("{0}/1.0/30/30", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/J,S/2"), 0, upgradingFlag));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 1=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 2=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move secondary 2=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainViolationWithDownReplica)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainViolationWithDownReplica");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int index = 0;
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc0/r3"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r0"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r1"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r2"));
        plb.UpdateNode(CreateNodeDescription(index++, L"dc1/r2"));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));
        blockList.insert(CreateNodeId(3));
        blockList.insert(CreateNodeId(4));
        blockList.insert(CreateNodeId(5));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"N/3, N/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityNoParentPartition)
    {
        wstring testName = L"ConstraintCheckAffinityNoParentPartition";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", testType, wstring(L""), true, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", testType, true, L"TestService1", CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService2", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/3, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 1=>0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 3=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 1=>4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move primary 0=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsTest2");
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
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary *<=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsTest3)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsTest3: affinity violation with all node exceeding capacity");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/120/120")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L"Dummy/0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* swap primary 0|1<=>1|0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsTest4)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsTest4");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(metricStr)));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType1", true, L"TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsNonPrimaryAlignmentTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsNonPrimaryAlignmentTest1");
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

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(metricStr)));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(wstring(L"TestService1"), wstring(L"TestType1"), wstring(L""), true, wstring(L""),
            wstring(L"TestService0"), false, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));
        plb.UpdateService(ServiceDescription(wstring(L"TestService2"), wstring(L"TestType1"), wstring(L""), true, wstring(L""),
            wstring(L"TestService0"), false, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, S/2, P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService2"), 0, CreateReplicas(L"P/2, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Verify that there should be no movement generated for the NonAlignment affinity
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsNonPrimaryAlignmentTest2)
    {
        wstring testName = L"ConstraintCheckAffinityViolationsNonPrimaryAlignmentTest2";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBConstraintCheckTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 50);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(metricStr)));

        // Service1 is NonAlignment scheme and Service2 is Alignment
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(wstring(L"TestService1"), wstring(L"TestType1"), wstring(L""), true, wstring(L""),
            wstring(L"TestService0"), false, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));
        plb.UpdateService(ServiceDescription(wstring(L"TestService2"), wstring(L"TestType1"), wstring(L""), true, wstring(L""),
            wstring(L"TestService0"), true, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0, P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"S/0, P/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 2=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 swap primary 3=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsWithNewReplicas)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsWithNewReplicas");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/90"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/120/120")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", false, L"TestService0", CreateMetrics(L"Dummy/0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/4, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 9));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 9));


        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        VERIFY_IS_TRUE(fm_->MoveActions.empty());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsDeleteServiceTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityViolationsTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L"Dummy/0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        Sleep(1);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0")));
        plb.DeleteService(L"TestService0");

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        Sleep(5);

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 1, CreateReplicas(L"P/5, S/0, S/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsDeleteFailoverUnitTest)
    {
        wstring testName = L"ConstraintCheckAffinityViolationsDeleteFailoverUnitTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentServiceName = wformatString("{0}_ParentService", testName);
        wstring childServiceName = wformatString("{0}_ChildService", testName);

        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, wstring(parentServiceName)));

        // One parent FailoverUnit
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentServiceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        // 3 child FUs, one with guid 2 is in violation of affinity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(childServiceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(childServiceName), 0, CreateReplicas(L"P/0, S/1, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(childServiceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 3=>2", value)));

        // Now fix the violation and delete 1st FailoverUnit
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(childServiceName), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.DeleteFailoverUnit(wstring(childServiceName), CreateGuid(1));

        fm_->Clear();

        // Refresh again to check if constraint check is needed. We only updated FUs, so fullConstraintCheck_ is false.
        // If FailoverUnitwith GUID 1 is not properly deleted, then PLB will assert here.
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityChildTargetLarger)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityChildTargetLarger)");

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
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Since we allow children to have more target replicas, there should be no constraint violation detected
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityChildTargetLargerInvalidReplicas)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityChildTargetLargerInvalidReplicas");
        // Turning off simulated annealing to simplify verification by preventing the case when parent secondary randomly moves to node 3 or 4
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 0);

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
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Affinity constraint check should force one of the non-colocated child replicas for each child service to move to the node with the parent
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 3|4=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 3|4=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityChildTargetLargerPreventExcessiveMoves)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityChildTargetLargerPreventExcessiveMoves)");
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 9));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4, S/5, S/6, S/7, S/8, S/9"), 0));


        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // We expect one move to fix affinity, but not any additional ones
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary *=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityParentTargetLarger)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityParentTargetLarger)");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // There should be no constraint violation detected
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityParentTargetLargerInvalidReplica)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityParentTargetLargerInvalidReplica)");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Affinity constraint check should force one of the non-colocated child replicas to move to the node with the parent
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 4=>1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckBufferedCapacityChange)
    {
        wstring testName = L"ConstraintCheckBufferedCapacityChange";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        //this test checks whether the closure is properly updated when we change node buffer capacity dynamicly
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring metric = L"MyMetric";
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/50", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/550", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/200", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/600", metric)));

        wstring testType = wformatString("{0}Type", testName);
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), move(blockList)));

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"service1", testType, true, CreateMetrics(wformatString("{0}/1.0/150/100", metric))));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"service2", testType, true, CreateMetrics(wformatString("{0}/1.0/150/100", metric))));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"service1"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"service2"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>3", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"service2"), 1, CreateReplicas(L"P/3"), 0));

        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"MyMetric", 0.5));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);

        fm_->Clear();
        //the closure should be updated now taking into account node buffer precentage changes
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 2=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsPrefferedPrimary)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", " ConstraintCheckCapacityViolationsPrefferedPrimary ");

        //this test check whether we are considering proper loads during upgrade
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/5"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/30/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/25/0")));

        Reliability::FailoverUnitFlags::Flags upgradingFlag;
        upgradingFlag.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 0));
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(10.0));

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"S/0/L,P/1"), 1, upgradingFlag));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        actionList = GetActionListString(fm_->MoveActions);
        //secondary that is preferred primary should report secondary load
        //so we can only move to node 0 to fix violation
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move primary 2=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest: capacity violation on node0 but enough capacity on node1");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Count/25"));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false));

        for (int i = 0; i < 50; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));
        }

        for (int i = 0; i < 50; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i + 60), wstring(L"TestService"), 0, CreateReplicas(L"I/1"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //should have 25 move
        VERIFY_ARE_EQUAL(25u, actionList.size());
        VERIFY_ARE_EQUAL(25, CountIf(actionList, ActionMatch(L"* move instance 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest2: capacity violation on node0 and can be corrected by the only one replica");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/90"));

        map<wstring, uint> capacities2;
        capacities2.insert(make_pair(wstring(L"MyMetric"), 99));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/99"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, CreateMetrics(L"MyMetric/1.0/0/0")));

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i, L"TestService", L"MyMetric", 10));

            if (i == 5)
            {
                plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i, L"TestService", L"MyMetric", 9));
            }
        }

        for (int i = 0; i < 10; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i + 10), wstring(L"TestService"), 0, CreateReplicas(L"I/1"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i + 10, L"TestService", L"MyMetric", 9));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest3)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest3: capacity defined on one metric but not another, only related failover unit should be moved");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric1/50"));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", false, CreateMetrics(L"MyMetric1/1.0/0/0")));

        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", false, CreateMetrics(L"MyMetric2/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1, S/0"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 100));
        // Update the load of both primary and secondary of service 0 (100)
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService1", L"MyMetric1", 100, loads));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest4)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest4: capacity violation on node0 for the parent service while affinity exist");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, uint> capacities;
        capacities.insert(make_pair(wstring(L"MyMetric"), 100));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0"));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService2"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService0", L"MyMetric", 70, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService2", L"MyMetric", 70, 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest5)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest5: capacity violation can only be corrected by swap primary/secondary");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/150"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/100/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationsTest6)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationsTest6: capacity violation with affinity");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, uint> capacities;
        capacities.insert(make_pair(wstring(L"MyMetric"), 100));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/200"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric/1.0/110/10")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L"MyMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // VERIFY_ARE_EQUAL(2u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckCapacityViolationWithFaultDomain)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckCapacityViolationWithFaultDomain");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0", L"EndPointCount/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0", L"EndPointCount/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1", L"EndPointCount/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1", L"EndPointCount/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2", L"EndPointCount/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc2", L"EndPointCount/10"));

        vector<ServiceMetric> metrics = CreateMetrics(L"EndPointCount/0.0/5/0,PC/1.0/1/0,RC/1.0/0/1");

        vector<ServiceMetric> dummyMetrics = CreateMetrics(L"Dummy/0/0/0");

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

        int index = 0;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P00"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C00"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P01"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C01"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P02"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C02"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P03"), 0, CreateReplicas(L"P/1, S/3, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C03"), 0, CreateReplicas(L"P/1, S/3, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_P04"), 0, CreateReplicas(L"P/1, S/3, S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(L"GroupA_C04"), 0, CreateReplicas(L"P/1, S/3, S/5"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* swap primary 0<=>2|4", value)));

        fm_->Clear();
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainViolationWithCapacity)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainViolationWithCapacity");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1", L"MyMetric/10"));


        vector<ServiceMetric> metrics = CreateMetrics(L"MyMetric/1.0/0/0");

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, vector<ServiceMetric>(metrics)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"MyMetric", 9, 9));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService2", L"MyMetric", 9, 9));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move * *<=>*", value)));

        fm_->Clear();
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainViolationWithCapacityWithDelay)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainViolationWithCapacityWithDelay");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1", L"MyMetric/10"));


        vector<ServiceMetric> metrics = CreateMetrics(L"MyMetric/1.0/0/0");

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, vector<ServiceMetric>(metrics)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, vector<ServiceMetric>(metrics)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"MyMetric", 9, 9));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, L"TestService2", L"MyMetric", 9, 9));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move * *<=>*", value)));

        fm_->Clear();
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainViolationWithCapacity2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainViolationWithCapacity2: when we have limited time to do search, the correction of fault domain violation will not violate capacity");
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc1", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1", L"MyMetric/10"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc2", L"MyMetric/10"));

        vector<ServiceMetric> metrics = CreateMetrics(L"MyMetric/1.0/5/5");

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, vector<ServiceMetric>(metrics)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/2, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/1, S/2, S/4"), 0));

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        VERIFY_IS_TRUE(fm_->MoveActions.empty());

        fm_->Clear();
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainViolationWithCapacity3)
    {
        wstring testName = L"ConstraintCheckFaultDomainViolationWithCapacity3";
        Trace.WriteInfo("PLBConstraintCheckTestSource",
            "{0}: when we are correcting fault domain, we should not vioalte capacity even if the total remaining violations are smaller than original",
            testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc0", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc1", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc1", L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc1", L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/100/100")));

        for (int i = 0; i < 100; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(100), wstring(L"TestService2"), 0, CreateReplicas(L"P/3/B, S/4/B, S/5/B"), 0));
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        VERIFY_ARE_EQUAL(0u, fm_->MoveActions.size());

        fm_->Clear();
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckMultipleViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckMultipleViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType2", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0"), 0)); // P/2, S/1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0)); // P/1, S/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService2"), 0, CreateReplicas(L"P/4, S/3"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService3"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService3"), 0, CreateReplicas(L"P/1, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService3"), 0, CreateReplicas(L"P/2, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1=>2|3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1=>2|3|4", value)));

        // verification for service3
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"9|10 move * 1=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckMultipleViolationsTestWithDelay)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckMultipleViolationsTestWithDelay");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType2", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0"), 0)); // P/2, S/1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0)); // P/1, S/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService2"), 0, CreateReplicas(L"P/4, S/3"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService3"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService3"), 0, CreateReplicas(L"P/1, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService3"), 0, CreateReplicas(L"P/2, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/2"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1=>2|3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1=>2|3|4", value)));

        // verification for service3
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"9|10 move * 1=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckBestEffortTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckBestEffortTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType1", true));

        blockList.insert(CreateNodeId(3));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType2", true));

        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType3", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0)); // fault domain violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/2, S/3"), 0)); // fault domain violation

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0"), 0)); // scaleout count violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0)); // and blocklist violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0)); // and fault domain violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService2"), 0, CreateReplicas(L"P/4, N/3"), 0)); //block list violation, with one down replica

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService3"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/4"), 0)); // scaleout count violation

        fm_->RefreshPLB(Stopwatch::Now());

        // TODO: verify it automatically
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndBlockListViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndBlockListViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc1/rack0"));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(6));
        blockList.insert(CreateNodeId(7));
        blockList.insert(CreateNodeId(8));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/3, S/5, S/6, S/8"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndCapacityViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndCapacityViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(CapacityConstraintPriority, int, 1);

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"dc0/rack0", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"dc0/rack0", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"dc0/rack1", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"dc0/rack1", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"dc1/rack0", L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"dc1/rack0", L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, L"dc1/rack1", L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7, L"dc1/rack1", L"MyMetric/30"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/5"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(4), 40));
        loads.insert(make_pair(CreateNodeId(5), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 50, loads));

        // We have hard and soft constraint violation, violation of soft nodeCapacity constraint cannot be fixed.
        // Expectation is that FD constraint violation is fixed.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 0|1|2|3=>6|7", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithServiceOnEveryNodeDefaultLoadConfigFalse)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckWithServiceOnEveryNodeDefaultLoadConfigFalse");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/25"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, CreateMetrics(L"CPU/1.0/25/25"), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(0), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"CPU", 0, newLoads, false));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"I/0,I/1"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        //the new replica is using the average so we are in violation on node 1
        VerifyNodeLoadQuery(plb, 0, L"CPU", 40);
        VerifyNodeLoadQuery(plb, 1, L"CPU", 40);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithServiceOnEveryNodeDefaultLoadConfigTrue)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckWithServiceOnEveryNodeDefaultLoadConfigTrue");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(UseDefaultLoadForServiceOnEveryNode, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/25"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", false, CreateMetrics(L"CPU/1.0/25/25"), FABRIC_MOVE_COST_LOW, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"I/0"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        map<NodeId, uint> newLoads;
        newLoads.insert(make_pair(CreateNodeId(0), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"CPU", 0, newLoads, false));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"I/0,I/1"), -1));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VerifyNodeLoadQuery(plb, 0, L"CPU", 40);
        VerifyNodeLoadQuery(plb, 1, L"CPU", 25);
        //the new replica is using the default so there is no violation and we will go into creation (for -1 we always go into creation)
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndCapacityViolationsTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndCapacityViolationsTest2");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(CapacityConstraintPriority, int, 1);

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0,  L"dc0/rack0", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1,  L"dc0/rack0", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2,  L"dc0/rack1", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3,  L"dc0/rack1", L"MyMetric/50"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4,  L"dc1/rack0", L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5,  L"dc1/rack0", L"MyMetric/30"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6,  L"dc1/rack1", L"MyMetric/40"));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7,  L"dc1/rack1", L"MyMetric/40"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/5"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(4), 40));
        loads.insert(make_pair(CreateNodeId(5), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"MyMetric", 50, loads));

        // We have hard and soft constraint violation, violation of soft nodeCapacity constraint can be fixed.
        // Expectation is that nodeCapacity constraint violation is fixed (although it is soft) because of hardcoded constraint order.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move secondary 4|5=>6|7", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndNodePropertyViolationsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndNodePropertyViolationsTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties;
        nodeProperties.insert(make_pair(L"a", L"0"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack0", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1", L"", map<wstring, wstring>(nodeProperties)));

        nodeProperties[L"a"] = L"1";
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/rack0", L"", map<wstring, wstring>(nodeProperties)));
        plb.UpdateNode(CreateNodeDescription(8, L"dc1/rack0", L"", map<wstring, wstring>(nodeProperties)));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));
        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService0", L"TestType", true, wstring(L"a==0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/3, S/5, S/6, S/8"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move * *=>1|4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndServicePlacementTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndServicePlacementTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc1/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService0", L"TestType", true, L"FaultDomain ^ fd:/dc0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/3, S/5, S/6, S/8"), 0));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService1", L"TestType", true, L"FaultDomain ^ fd:/dc0/rack0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/2, S/3"), 0));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService2", L"TestType", true, L"FaultDomain !^ fd:/dc0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/6, S/5, S/7"), 0));

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService3", L"TestType", true, L"FaultDomain !^ fd:/dc0/rack1"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * *=>1|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 3=>0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 5=>8", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move secondary 3=>*", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDomainAndPrimaryReplicaPlacementTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDomainAndPrimaryReplicaPlacementTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/rack0"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService0", L"TestType", true, L"FaultDomain ^P fd:/dc1"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2, S/3, S/5, S/6"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>6", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainBasicPackingDistributionTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainBasicPackingDistributionTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd:/1", L"1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd:/1", L"2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd:/2", L"3", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd:/2", L"4", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd:/2", L"5", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"fd:/3", L"6", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(7, L"fd:/3", L"7", L""));

        // One more down node per FD, to make sure that we will not move to them!
        plb.UpdateNode(NodeDescription(CreateNodeInstance(8, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"fd:/1").Segments),
            wstring(L"8"),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(9, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"fd:/2").Segments),
            wstring(L"9"),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.UpdateNode(NodeDescription(CreateNodeInstance(10, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            vector<wstring>(Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"fd:/3").Segments),
            wstring(L"10"),
            map<wstring, uint>(),
            map<wstring, uint>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/1,S/3,S/4,S/5,S/6"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move * 3|4|5=>2|7", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainPackingTwoLevelesDistributionTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainPackingTwoLevelesDistributionTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd:/0/0", L"1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd:/0/1", L"2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd:/1/0", L"3", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd:/1/1", L"4", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd:/2/0", L"5", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"fd:/2/1", L"6", L""));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentServiceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentServiceName), 0, CreateReplicas(L"P/1,S/4,S/6,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainPackingDifferentLevelsDistributionTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainPackingDifferentLevelsDistributionTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // 4 nodes are in 1 datacenter that has no FDs
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd:/dc1", L"1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd:/dc1", L"2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd:/dc1", L"3", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd:/dc1", L"4", L""));
        // 4 nodes are in second datacenter that has subdomains
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd:/dc2/fd1", L"5", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(6, L"fd:/dc2/fd1", L"6", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(7, L"fd:/dc2/fd2", L"7", L""));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(8, L"fd:/dc2/fd2", L"8", L""));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One FailoverUnit, 4 replicas in datacenter 1
        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/1,S/2,S/3,S/4"), 0));

        // Point of the test is to check if PLB will see that replicas on nodes 2 and 5 are in FD/UD violation.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move * 1|2|3|4=>5|6", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move * 1|2|3|4=>7|8", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainPackingVariousRandomCasesTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainPackingVariousRandomCasesTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        srand(static_cast<unsigned int>(time(NULL)));

        size_t randomCases = 5;
        size_t badRuns = 0;
        for (int randomCase = 0; randomCase < randomCases; randomCase++)
        {
            PlacementAndLoadBalancing & plb = fm_->PLB;

            size_t maxReplicas = 30;
            size_t maxDomains = 10;

            // Making sure that we have more than 1 replica and 1 domain:
            size_t replicas = rand() % (maxReplicas - 1) + 2;
            size_t domains = rand() % (min(maxDomains, replicas) - 1) + 2;
            size_t maxReplicasPerDomain = static_cast<size_t>(ceil((double)replicas / domains));
            size_t minReplicasPerDomain = static_cast<size_t>(floor((double)replicas / domains));

            // Making sure that we have enough nodes per domain to make domain violations:
            size_t nodes = maxReplicasPerDomain * domains * 2;

            map<size_t, size_t> nodesPerDomain;
            map<int, size_t> nodeDomain;
            map<size_t, size_t> replicasPerDomain;
            for (size_t domainId = 0; domainId < domains; domainId++)
            {
                nodesPerDomain.insert(make_pair(domainId, nodes / domains));
                replicasPerDomain.insert(make_pair(domainId, 0));
            }

            size_t tempReplicas = replicas;
            size_t currentDomainId = 0;
            size_t iterations = 0;
            while (tempReplicas > 0)
            {
                iterations++;
                currentDomainId %= domains;
                size_t availableSlots = nodesPerDomain[currentDomainId] - replicasPerDomain[currentDomainId];

                if (availableSlots == 0)
                {
                    currentDomainId++;
                    continue;
                }

                size_t replicasToAdd = tempReplicas;
                if (tempReplicas > 1)
                {
                    replicasToAdd = rand() % tempReplicas + 1;
                }

                replicasToAdd = min(replicasToAdd, availableSlots);
                replicasPerDomain[currentDomainId] += replicasToAdd;
                tempReplicas -= replicasToAdd;
                currentDomainId++;
            }

            int globalNodeId = 0;
            for (size_t domainId = 0; domainId < domains; domainId++)
            {
                for (size_t nodeId = 0; nodeId < nodesPerDomain[domainId]; nodeId++)
                {
                    wstring faultDomain = wformatString("fd:/{0}/{1}", domainId, nodeId);
                    plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(globalNodeId, faultDomain, wformatString("{0}", globalNodeId), L""));
                    nodeDomain.insert(make_pair(globalNodeId, domainId));
                    globalNodeId++;
                }
            }

            plb.ProcessPendingUpdatesPeriodicTask();

            wstring testType = wformatString("{0}Type", testName);
            plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

            wstring serviceName = wformatString("{0}_Service", testName);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true));

            wstring replicasPlacement = L"";
            wstring replicasDistributionForPrint = L"";
            wstring nodesDistributionForPrint = L"";
            bool isPrimaryAdded = false;
            size_t lastNodeIdInPreviousDomain = 0;
            size_t violationsBefore = 0;

            for (size_t domainId = 0; domainId < domains; domainId++)
            {
                for (size_t nodeId = 0; nodeId < replicasPerDomain[domainId]; nodeId++)
                {
                    if (!isPrimaryAdded)
                    {
                        replicasPlacement.append(wformatString("P/{0}", lastNodeIdInPreviousDomain + nodeId));
                        isPrimaryAdded = true;
                    }
                    else
                    {
                        replicasPlacement.append(wformatString(",S/{0}", lastNodeIdInPreviousDomain + nodeId));
                    }
                }

                lastNodeIdInPreviousDomain += nodesPerDomain[domainId];

                if (replicasPerDomain[domainId] > maxReplicasPerDomain)
                {
                    violationsBefore += replicasPerDomain[domainId] - maxReplicasPerDomain;
                }

                if (replicasPerDomain[domainId] < minReplicasPerDomain)
                {
                    violationsBefore += minReplicasPerDomain - replicasPerDomain[domainId];
                }

                replicasPlacement.append(L" ");
                nodesDistributionForPrint.append(wformatString("\t{0}", nodesPerDomain[domainId]));
                replicasDistributionForPrint.append(wformatString("\t{0}", replicasPerDomain[domainId]));
            }
            violationsBefore /= 2;

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(replicasPlacement), 0));

            plb.ProcessPendingUpdatesPeriodicTask();

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);

            // Update number of replicas per domain
            for (auto const& actionPair : fm_->MoveActions)
            {
                for (FailoverUnitMovement::PLBAction currentAction : actionPair.second.Actions)
                {
                    if (currentAction.Action == FailoverUnitMovementType::MoveSecondary ||
                        currentAction.Action == FailoverUnitMovementType::MoveInstance ||
                        currentAction.Action == FailoverUnitMovementType::MovePrimary)
                    {
                        replicasPerDomain[nodeDomain[static_cast<int>(currentAction.SourceNode.IdValue.Low)]]--;
                        replicasPerDomain[nodeDomain[static_cast<int>(currentAction.TargetNode.IdValue.Low)]]++;
                    }
                }
            }

            replicasDistributionForPrint.clear();
            size_t violationsAfter = 0;
            for (size_t domainId = 0; domainId < domains; domainId++)
            {
                if (replicasPerDomain[domainId] > maxReplicasPerDomain)
                {
                    violationsAfter += replicasPerDomain[domainId] - maxReplicasPerDomain;
                }

                if (replicasPerDomain[domainId] < minReplicasPerDomain)
                {
                    violationsAfter += minReplicasPerDomain - replicasPerDomain[domainId];
                }

                replicasDistributionForPrint.append(wformatString("\t{0}", replicasPerDomain[domainId]));
            }

            if (violationsAfter != 0)
            {
                badRuns++;
            }

            fm_->Load();
        }

        VERIFY_ARE_EQUAL(badRuns, 0);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainNonpackingDistributionTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainNonpackingDistributionTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2/rack1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType123"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType123", true, L"FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/4, S/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 0|1=>2|3", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainNonpackingDistributionTestWithDelay)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckFaultDomainNonpackingDistributionTestWithDelay");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc2/rack1"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType123"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType123", true, L"FDPolicy ~ Nonpacking"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/4, S/0, S/1"), 0));

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(9999));

            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList0 = GetActionListString(fm_->MoveActions);

            //System is bootstrapping so NewNode Throttling partially delays FD/UD violation-fixes.
            VERIFY_ARE_EQUAL(0u, actionList0.size());
        }

        {
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
            PLBConfigScopeChange(ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));

            //Simulating system already stabilizing after NewNode/NodeDown.
            fm_->RefreshPLB(Stopwatch::Now());
        }

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 0|1=>2|3", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedFaultDomainNoViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedFaultDomainNoViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
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

        ConstraintCheckWithQuorumBasedDomainNoViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedUpgradeDomainNoViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedUpgradeDomainNoViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"0"));

        plb.UpdateNode(CreateNodeDescription(4, L"", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"", L"2"));

        plb.UpdateNode(CreateNodeDescription(9, L"", L"3"));

        ConstraintCheckWithQuorumBasedDomainNoViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedFaultAndUpgradeDomainNoViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedFaultAndUpgradeDomainNoViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc2/rack0", L"2"));

        plb.UpdateNode(CreateNodeDescription(9, L"dc3/rack0", L"3"));

        ConstraintCheckWithQuorumBasedDomainNoViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedFaultDomainViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedFaultDomainViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
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

        ConstraintCheckWithQuorumBasedDomainWithViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedUpgradeDomainViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedUpgradeDomainViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"0"));

        plb.UpdateNode(CreateNodeDescription(4, L"", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"", L"2"));

        plb.UpdateNode(CreateNodeDescription(9, L"", L"3"));

        ConstraintCheckWithQuorumBasedDomainWithViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedFaultAndUpgradeDomainViolationToFixTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedFaultAndUpgradeDomainViolationToFixTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0"));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc2/rack0", L"2"));

        plb.UpdateNode(CreateNodeDescription(9, L"dc3/rack0", L"3"));

        ConstraintCheckWithQuorumBasedDomainWithViolationsHelper(plb);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedIntraDomainMoveTest0)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedIntraDomainMoveTest0");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties0;
        nodeProperties0.insert(make_pair(L"NodeType", L"red"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));

        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"1", move(map<wstring, wstring>(nodeProperties0))));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2", move(map<wstring, wstring>(nodeProperties0))));

        plb.ProcessPendingUpdatesPeriodicTask();

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // service in placement constraint violation but only intra domain move is possible
        wstring serviceType = L"TestType0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));
        wstring serviceName = L"StatefulPackingService0";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"NodeType==red", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/5,S/6, S/7"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1 => 2|3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedIntraDomainMoveTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedIntraDomainMoveTest1");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties0;
        nodeProperties0.insert(make_pair(L"NodeType", L"red"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));

        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2", move(map<wstring, wstring>(nodeProperties0))));

        plb.ProcessPendingUpdatesPeriodicTask();

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // service in placement constraint and FD constraint violation but only intra domain move is possible
        set<NodeId> blockList1;
        blockList1.insert(CreateNodeId(2));
        blockList1.insert(CreateNodeId(6));
        wstring serviceType = L"TestType1";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));
        wstring serviceName = L"StatefulPackingService1";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"NodeType==red", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2, S/5, S/7"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 2 => 3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedIntraDomainMoveTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedIntraDomainMoveTest2");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties0;
        nodeProperties0.insert(make_pair(L"NodeType", L"red"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1", L"0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/rack1", L"0", move(map<wstring, wstring>(nodeProperties0))));

        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1", move(map<wstring, wstring>(nodeProperties0))));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack0", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2"));

        plb.ProcessPendingUpdatesPeriodicTask();

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // service in placement constraint and FD constraint violation but only intra domain move is possible
        wstring serviceType = L"TestType2";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));
        wstring serviceName = L"StatefulPackingService2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"NodeType==red", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3, S/5"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move secondary 3 => 4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedDeactivationSafetyCheckInProgressDomainTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedDeactivationSafetyCheckInProgressDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"dc0/rack0", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"dc0/rack0", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"dc0/rack1", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"dc0/rack1", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack1", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(9, L"dc2/rack1", L"2"));

        plb.UpdateNode(CreateNodeDescription(10, L"dc3/rack0", L"3"));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // whole domain is deactivated but replicas are movable, we should fix FD/UD violation if exists
        wstring serviceType = L"TestType0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));
        wstring serviceName = L"StatefulPackingService0";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,     S/4,S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,     S/4,     S/7"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,         S/4,S/5, S/7"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * 0|1|2|3 => 5|6|7|8|9|10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1|2 => 7|8|9|10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move * 0|1|2 => 5|6|8|9|10", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"3 move * * => *", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckQuorumBasedDeactivationCompleteDomainTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckQuorumBasedDeactivationCompleteDomainTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerUpgradeDomains, bool, true);
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"dc0/rack0", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"dc0/rack0", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"dc0/rack1", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"dc0/rack1", L"0", L"",
            Reliability::NodeDeactivationIntent::Enum::Restart,
            Reliability::NodeDeactivationStatus::Enum::DeactivationComplete));

        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0", L"1"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack1", L"1"));

        plb.UpdateNode(CreateNodeDescription(7, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc2/rack0", L"2"));
        plb.UpdateNode(CreateNodeDescription(9, L"dc2/rack1", L"2"));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // whole domain is deactivated but replicas are movable, we should fix FD/UD violation if exists
        wstring serviceType = L"TestType0";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));
        wstring serviceName = L"StatefulPackingService0";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 5));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,     S/4,S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,     S/4,     S/7"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,         S/4,S/5, S/7"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckTwoQuorumBasedFaultDomainsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckTwoQuorumBasedFaultDomainsTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"l0/l1/dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"l0/l1/dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(3, L"l0/l1/dc0/rack3"));

        plb.UpdateNode(CreateNodeDescription(4, L"l0/l1/dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"l0/l1/dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(6, L"l0/l1/dc1/rack2"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0, S/4,S/5"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1|2 => 4|5|6", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckThreeQuorumBasedFaultDomainsTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckThreeQuorumBasedFaultDomainsTest");
        PLBConfigScopeChange(QuorumBasedReplicaDistributionPerFaultDomains, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"l0/l1/dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"l0/l1/dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"l0/l1/dc0/rack2"));
        plb.UpdateNode(CreateNodeDescription(3, L"l0/l1/dc0/rack3"));
        plb.UpdateNode(CreateNodeDescription(4, L"l0/l1/dc0/rack4"));

        plb.UpdateNode(CreateNodeDescription(5, L"l0/l1/dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"l0/l1/dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(7, L"l0/l1/dc1/rack2"));

        plb.UpdateNode(CreateNodeDescription(8, L"l0/l1/dc2/rack0"));
        plb.UpdateNode(CreateNodeDescription(9, L"l0/l1/dc2/rack1"));
        plb.UpdateNode(CreateNodeDescription(10, L"l0/l1/dc2/rack2"));

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 4));

        // partition can have max 2 replicas per domain, as we have 3 domains in total
        // replica distribution among domains 4, 0, 0 - violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        // replica distribution among domains 3, 1, 0 - violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2, S/5"), 0));

        // replica distribution among domains 2, 2, 0 - no violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/5,S/6"), 0));

        // replica distribution among domains 2, 1, 1 - no violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/5, S/8"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * 0|1|2|3 => 5|6|7|8|9|10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1|2 => 6|7|8|9|10", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckNodePropertyAndServiceTypeBlockList)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckNodePropertyAndServiceTypeBlockList");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            map<wstring, wstring> nodeProperties;
            // this makes 2,3,4 as invalid nodes
            nodeProperties.insert(make_pair(L"a", i >= 2 ? L"1" : L"0"));
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", move(nodeProperties)));
        }

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        blockList.insert(CreateNodeId(3));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(wstring(L"TestService0"), wstring(L"TestType1"), true, wstring(L"a==0")));
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList2)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(wstring(L"TestService1"), wstring(L"TestType2"), false, wstring(L"a==0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"P/4"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"I/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService1"), 0, CreateReplicas(L"I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService1"), 0, CreateReplicas(L"I/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService1"), 0, CreateReplicas(L"I/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService1"), 0, CreateReplicas(L"I/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2|4 move * 2|4=>0", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"8|9 move * 3|4=>0|1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPlacementObjectCreationVerificationTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckPlacementObjectCreationVerificationTest");
        PLBConfigScopeChange(MaxMovementExecutionTime, TimeSpan, TimeSpan::FromTicks(0));

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"Parent", L"TestType", true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Parent"), 0, CreateReplicas(L"P/0"), 0));

        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"Child", L"TestType", true, L"Parent"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Child"), 0, CreateReplicas(L"P/1"), 0));

        // this will do constraint check
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        // due to our MaxMovementExecutionTime setting, this will again do constraint check. So we shouldn't verify there is no placement creation.
        fm_->RefreshPLB(now);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPartitionAffinityTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckPartitionAffinityTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0")));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0", CreateMetrics(L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDirectionalAffinityDisabledBasicTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDirectionalAffinityDisabledBasicTest");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(metricStr)));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentBasicTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentBasicTest1");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //Either parent(Service0) or child(Service1) can be moved to fix the affinity, but we should prefer moving the child...
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 2=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentBasicTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentBasicTest2");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //Service 1(child) has blocklisted Node 1, therefore Service 0(parent) will be moved to fix the affinity...
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentBasicTest3)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentBasicTest3");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //Now both parent and child have respective opposite nodes blocklisted, hence we will move both replicas to node 3...
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        set<NodeId> blockList2;
        blockList2.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), move(blockList2)));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 2=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentChildTargetLargerTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentChildTargetLargerTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType1", true, L"TestService0", CreateMetrics(L""), false, 3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //Service 1(child) has blocklisted Node 1, therefore Service 0(parent) will be moved to fix the affinity...
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), move(blockList)));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckDirectionalAffinityEnabledTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckDirectionalAffinityEnabledTest1");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric2/50"));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(L"MyMetric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(L"MyMetric2/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 100));
        // Update the load of both primary and secondary of service 0 (100)
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"TestService1", L"MyMetric2", 100, loads));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move secondary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPrimaryWithStandByTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckPrimaryWithStandByTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/51")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/51/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, SB/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"S/1, P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* swap primary 2<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSecondaryWithStandByTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSecondaryWithStandByTest");
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/100"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/51")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/51")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, SB/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* swap primary 1<=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(TestClusterCapacity)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "TestClusterCapacity");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Cluster capacity is zero if there are no nodes
        VERIFY_ARE_EQUAL(0, plb.GetTotalClusterCapacity(L"MyMetric01"));

        // Next, add three nodes and verify that capacity increases accordingly
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric01/10"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(10, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric01/10"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(20, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric01/10"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(30, plb.GetTotalClusterCapacity(L"MyMetric01"));

        // Increase and then decrease capacity of one node
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric01/3"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(23, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric01/10"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(30, plb.GetTotalClusterCapacity(L"MyMetric01"));

        // Capacity becomes "Inifinite" on one node, and then becomes finite again
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L""));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(INT64_MAX, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric01/10"));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(30, plb.GetTotalClusterCapacity(L"MyMetric01"));

        // Bring the node down, and then up again
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric01/10")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(20, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric01/10")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(30, plb.GetTotalClusterCapacity(L"MyMetric01"));

        // Deactivate the node and then activate it
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::Pause,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric01/10")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(20, plb.GetTotalClusterCapacity(L"MyMetric01"));

        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"MyMetric01/10")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(30, plb.GetTotalClusterCapacity(L"MyMetric01"));
    }

    BOOST_AUTO_TEST_CASE(DynamicConstraintCheckTest1)
    {
        wstring testName = L"DynamicConstraintCheckTest1";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MaxMovementExecutionTime, TimeSpan, TimeSpan::FromTicks(0));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<Federation::NodeId>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring serviceName = wformatString("{0}_service", testName);
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, testType, true, L"FaultDomain ^ fd:/dc0"));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        // Dynamically update constraint with another faultdomain
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, testType, true, L"FaultDomain ^ fd:/dc1"));
        // In the next run, PLB should move replica to new domain
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>2|3|4", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2|3|4", value)));

        fm_->Clear();
        // Change it back. No constraint violation any more
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, testType, true, L"FaultDomain ^ fd:/dc0"));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DynamicAffinityTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "DynamicAffinityTest1");
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.5/0/0,ReplicaCount/0.5/0/0,Count/0.5/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Dynamically update affinity
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 2=>0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 3=>1", value)));

        // Dynamically remove affinity
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(metricStr)));
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DynamicCapacityTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "DynamicCapacityTest1: capacity violation on node0 but enough capacity on node1");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/30"));

        vector<ServiceMetric> metrics1a = CreateMetrics(L"Metric1/1.0/30/30");
        vector<ServiceMetric> metrics1b = CreateMetrics(L"Metric1/1.0/25/25"); // change the default load to lower value
        vector<ServiceMetric> metrics1c = CreateMetrics(L"Metric1/1.0/30/25"); // change it back
        vector<ServiceMetric> metrics1d = CreateMetrics(L"Metric1/1.0/25/25, Metric2/1.0/30/25"); // additional metric
        vector<ServiceMetric> metrics1e = CreateMetrics(L"Metric2/1.0/30/25"); // replace metric

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1a)));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>2|3", value)));
        fm_->Clear();

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1b)));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1c)));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>2|3", value)));
        fm_->Clear();

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1d)));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1e)));
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DynamicNodePropertyTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "DynamicNodePropertyTest1");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/30"));

        vector<ServiceMetric> metrics1a = CreateMetrics(L"Metric1/1.0/30/30");
        vector<ServiceMetric> metrics1b = CreateMetrics(L"Metric1/1.0/25/25");
        vector<ServiceMetric> metrics1c = CreateMetrics(L"Metric1/1.0/30/25");

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, vector<ServiceMetric>(metrics1a)));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2|3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>2|3", value)));
        fm_->Clear();

        // increaset the node capacity dynamically
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/30"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/30"));
        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSeparateSecondaryLoadTest1)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSeparateSecondaryLoadTest1");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/20"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/20"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ConstraintCheckSeparateSecondaryLoadTest1ServiceType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"ConstraintCheckSeparateSecondaryLoadTest1Service0",
            L"ConstraintCheckSeparateSecondaryLoadTest1ServiceType", true, CreateMetrics(L"MyMetric/1.0/5/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ConstraintCheckSeparateSecondaryLoadTest1Service0"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(2), 11));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"ConstraintCheckSeparateSecondaryLoadTest1Service0", L"MyMetric", 5, loads));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 2=>3|4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSeparateSecondaryLoadTest2)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSeparateSecondaryLoadTest2");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/10"));
        }

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/20"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ConstraintCheckSeparateSecondaryLoadTest2_ServiceType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"ConstraintCheckSeparateSecondaryLoadTest2_Service0",
            L"ConstraintCheckSeparateSecondaryLoadTest2_ServiceType", true, CreateMetrics(L"MyMetric/1.0/5/5")));
        plb.UpdateService(CreateServiceDescription(L"ConstraintCheckSeparateSecondaryLoadTest2_Service1",
            L"ConstraintCheckSeparateSecondaryLoadTest2_ServiceType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ConstraintCheckSeparateSecondaryLoadTest2_Service0"),
            0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"ConstraintCheckSeparateSecondaryLoadTest2_Service1"),
            0, CreateReplicas(L"P/4"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(2), 15));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"ConstraintCheckSeparateSecondaryLoadTest2_Service0", L"MyMetric", 5, loads));

        // after the update, node 2 would be over capacity, S/2 can only be moved to node 4
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 2=>4", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ConstraintCheckSeparateSecondaryLoadTest2_Service0"),
            1, CreateReplicas(L"P/0, S/1, S/4"), 0));

        loads.clear();
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, L"ConstraintCheckSeparateSecondaryLoadTest2_Service1", L"MyMetric", 9, loads));

        // after the update, node 4 would be over capacity
        plb.ProcessPendingUpdatesPeriodicTask();

        now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 4=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSeparateSecondaryLoadTest3)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSeparateSecondaryLoadTest3");
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/10"));
        for (int i = 1; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"MyMetric/20"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ConstraintCheckSeparateSecondaryLoadTest3_ServiceType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"ConstraintCheckSeparateSecondaryLoadTest3_Service0",
            L"ConstraintCheckSeparateSecondaryLoadTest3_ServiceType", true, CreateMetrics(L"MyMetric/1.0/20/20")));
        plb.UpdateService(CreateServiceDescription(L"ConstraintCheckSeparateSecondaryLoadTest3_Service1",
            L"ConstraintCheckSeparateSecondaryLoadTest3_ServiceType", true, CreateMetrics(L"MyMetric/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ConstraintCheckSeparateSecondaryLoadTest3_Service0"),
            0, CreateReplicas(L"P/0, S/3, S/4"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(3), 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"ConstraintCheckSeparateSecondaryLoadTest3_Service0", L"MyMetric", 20, loads));

        // node 0 would be over capacity, primary can only be swapped with S/3 to correct the capacity violation
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 0=>3", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"ConstraintCheckSeparateSecondaryLoadTest3_Service0"),
            1, CreateReplicas(L"P/3, S/0, S/4"), 0));

        // after the update, everything should be fine
        now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadUpdatePrimaryLoad)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadUpdatePrimaryLoad";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/150", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/105", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/50/50", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 100));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 50, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        loads.clear();
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 45, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Node 0 load: 105/100
        // FU0: 50 -> 45 - updated primary load and FU1: 60
        // Node 1 load: 100/150
        // FU0: 50 -> 100 - updated secondary load
        // Node 2 load: 50/105
        // FU0: secondary default load 50
        //
        // There should be no place for FU1 primary replica on nodes 1 and 2
        // Instead of just moving FU1 primary replica out, we should first swap FU0 primary and secondary replica
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 swap primary 0<=>1|2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithNodeDeactivationTest)
    {
        wstring testName = L"ConstraintCheckWithNodeDeactivationTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int nodeId = 0;
        for (; nodeId < 3; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId, L"", L"MyMetric/200"));
        }

        for (; nodeId < 6; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId, L"", L"MyMetric/250"));
        }

        // Currently we have:
        // Reliability::NodeDeactivationIntent::Enum::{None, Pause, Restart, RemoveData, RemoveNode}
        // Reliability::NodeDeactivationStatus::Enum::{
        //  DeactivationSafetyCheckInProgress, DeactivationSafetyCheckComplete, DeactivationComplete, ActivationInProgress, None}
        // First batch deactivated nodes are in constraint check violations as they have capacity 100 and will have replicas of 150.
        // Second batch will have capacity 150 and will be empty.
        int batchNodes = 0;
        for (int batch = 0; batch < 2; batch++)
        {
            for (int status = Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress;
                status <= Reliability::NodeDeactivationStatus::Enum::None;
                status++)
            {
                for (int intent = Reliability::NodeDeactivationIntent::Enum::None;
                    intent <= Reliability::NodeDeactivationIntent::Enum::RemoveNode;
                    intent++)
                {
                    plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                        nodeId++, L"", batch == 0 ? L"MyMetric/100" : L"MyMetric/150",
                        static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                        static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
                    batchNodes++;
                }
            }
        }
        batchNodes /= 2;

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));

        wstring replicas = L"P/0,S/1,S/2";
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 200));
        loads.insert(make_pair(CreateNodeId(2), 200));
        for (int i = 6; i < 6 + batchNodes; ++i)
        {
            loads.insert(make_pair(CreateNodeId(i), 150));
            replicas.append(wformatString(",S/{0}", i));
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(replicas), 0));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, L"MyMetric", 200, loads));

        // Adding temp FailoverUnit on possible target nodes in order to check that balancing inside of constraint check phase
        // will not place replicas on empty deactivating nodes instead of half-filled regular nodes 3, 4 or 5.
        wstring tempServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(L"MyMetric/1.0/100/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/3,S/4,S/5"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // All replicas on nodes 6-14 are in node capacity violation.
        // Only secondary replicas from nodes with Restart/RemoveData intent with DeactivationSafetyCheckComplete status could be moved.
        // Replicas from 0-2 nodes cannot be moved out due to other nodes capacity.
        // Replicas from nodes with Pause intent should not be moved.
        // Replicas from nodes with status different than DeactivationSafetyCheckComplete should not be moved.
        // Replicas should not be moved to the deactivated nodes.
        // Nodes:
        // NodeId   Replicas    Load    Capacity    ConstraintFix  DeactivationIntent  DeactivationStatus
        // 0        P_0         200     200                        None                None
        // 1        S_0         200     200                        None                None
        // 2        S_0         200     200                        None                None
        // 3            P_1     100     250                        None                None
        // 4            S_1     100     250                        None                None
        // 5            S_1     100     250                        None                None
        // 6        S_0        [150]    100         Allow          None                DeactivationSafetyCheckInProgress
        // 7        S_0        [150]    100         Don�t Allow    Pause               DeactivationSafetyCheckInProgress
        // 8        S_0        [150]    100         Allow          Restart             DeactivationSafetyCheckInProgress
        // 9        S_0        [150]    100         Allow          RemoveData          DeactivationSafetyCheckInProgress
        // 10       S_0        [150]    100         Allow          RemoveNode          DeactivationSafetyCheckInProgress
        // 11       S_0        [150]    100         Allow          None                DeactivationSafetyCheckComplete
        // 12       S_0        [150]    100         Don�t Allow    Pause               DeactivationSafetyCheckComplete
        // 13       S_0        [150]    100         Don�t Allow    Restart             DeactivationSafetyCheckComplete
        // 14       S_0        [150]    100         Don�t Allow    RemoveData          DeactivationSafetyCheckComplete
        // 15       S_0        [150]    100         Don�t Allow    RemoveNode          DeactivationSafetyCheckComplete
        // 16       S_0        [150]    100         Allow          None                DeactivationComplete
        // 17       S_0        [150]    100         Don't Allow    Pause               DeactivationComplete
        // 18       S_0        [150]    100         Don't Allow    Restart             DeactivationComplete
        // 19       S_0        [150]    100         Don't Allow    RemoveData          DeactivationComplete
        // 20       S_0        [150]    100         Don't Allow    RemoveNode          DeactivationComplete
        // 21       S_0        [150]    100         Allow          None                ActivationInProgress
        // 22       S_0        [150]    100         Don�t Allow    Pause               ActivationInProgress
        // 23       S_0        [150]    100         Don�t Allow    Restart             ActivationInProgress
        // 24       S_0        [150]    100         Don�t Allow    RemoveData          ActivationInProgress
        // 25       S_0        [150]    100         Don�t Allow    RemoveNode          ActivationInProgress
        // 26       S_0        [150]    100         Allow          None                None
        // 27       S_0        [150]    100         Don�t Allow    Pause               None
        // 28       S_0        [150]    100         Don�t Allow    Restart             None
        // 29       S_0        [150]    100         Don�t Allow    RemoveData          None
        // 30       S_0        [150]    100         Don�t Allow    RemoveNode          None
        // 31                  0        150         Allow          None                DeactivationSafetyCheckInProgress
        // 32                  0        150         Don�t Allow    Pause               DeactivationSafetyCheckInProgress
        // 33                  0        150         Don�t Allow    Restart             DeactivationSafetyCheckInProgress
        // 34                  0        150         Don�t Allow    RemoveData          DeactivationSafetyCheckInProgress
        // 35                  0        150         Don�t Allow    RemoveNode          DeactivationSafetyCheckInProgress
        // 36                  0        150         Allow          None                DeactivationSafetyCheckComplete
        // 37                  0        150         Don�t Allow    Pause               DeactivationSafetyCheckComplete
        // 38                  0        150         Don�t Allow    Restart             DeactivationSafetyCheckComplete
        // 39                  0        150         Don�t Allow    RemoveData          DeactivationSafetyCheckComplete
        // 40                  0        150         Don�t Allow    RemoveNode          DeactivationSafetyCheckComplete
        // 41                  0        150         Allow          None                DeactivationComplete
        // 42                  0        150         Don't Allow    Pause               DeactivationComplete
        // 43                  0        150         Don't Allow    Restart             DeactivationComplete
        // 44                  0        150         Don't Allow    RemoveData          DeactivationComplete
        // 45                  0        150         Don't Allow    RemoveNode          DeactivationComplete
        // 46                  0        150         Allow          None                ActivationInProgress
        // 47                  0        150         Don�t Allow    Pause               ActivationInProgress
        // 48                  0        150         Don�t Allow    Restart             ActivationInProgress
        // 49                  0        150         Don�t Allow    RemoveData          ActivationInProgress
        // 50                  0        150         Don�t Allow    RemoveNode          ActivationInProgress
        // 51                  0        150         Allow          None                None
        // 52                  0        150         Don�t Allow    Pause               None
        // 53                  0        150         Don�t Allow    Restart             None
        // 54                  0        150         Don�t Allow    RemoveData          None
        // 55                  0        150         Don�t Allow    RemoveNode          None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 6=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 8=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 9=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 10=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 11=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 16=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 21=>*", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 26=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithNodeDeactivationAndNoViolationsTest)
    {
        wstring testName = L"ConstraintCheckWithNodeDeactivationAndNoViolationsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);

        int nodeId = 0;
        for (; nodeId < 3; nodeId++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(nodeId, wformatString("dc{0}", nodeId), L"MyMetric/100"));
        }

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
                    nodeId, wformatString("dc{0}", nodeId), L"MyMetric/100",
                    static_cast<Reliability::NodeDeactivationIntent::Enum>(intent),
                    static_cast<Reliability::NodeDeactivationStatus::Enum>(status)));
                nodeId++;
            }
        }

        for (int i = nodeId; i < nodeId + 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc{0}", i), L"MyMetric/100"));
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
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(
            parentServiceName, testType, true, CreateMetrics(L"MyMetric/1.0/20/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(replicas), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            childServiceName, testType, true, parentServiceName, CreateMetrics(L"MyMetric/1.0/20/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(replicas), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // There is no replica with affinity, node capacity or FD violation.
        // No replica should be moved as balancing cannot make any improvement.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithOneConstraintViolationAndNodeDeactivationWithNoViolationsTest)
    {
        wstring testName = L"ConstraintCheckWithOneConstraintViolationAndNodeDeactivationWithNoViolationsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
            2, L"", L"", Reliability::NodeDeactivationIntent::Enum::Pause));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"S/0,S/2"), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(L"S/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Only child replica from node 1 should be moved to the node 0 where parent replica is.
        // Replicas from deactivated node should not be moved.
        // Nodes:
        // NodeId   Replicas    DeactivationIntent  DeactivationStatus
        // 0        S_0         None                None
        // 1             S_1    None                None
        // 2        S_0  S_1    Pause               None
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndPreferredLocationTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndPreferredLocationTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 9; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,SB/1,S/2"), 1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3,SB/4/D,S/5"), 1));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/6,SB/7/V,S/8"), 1));

        // StandBy replica shouldn't be marked with MoveInProgress
        // Although SB replicas is with D flag - marked as ToBeDropped, PLB should use it as a preferred location for new replica
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 4", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add secondary 7", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndReplicaExclusionStaticTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndReplicaExclusionStaticTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);

        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/D,S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/V,S/1"), 0));

        // FailoverUnit 0 has two secondary replicas on a node 1, one of them is ToBeDropped
        // PLB should move the other secondary as this state can occure only during PLB run
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndAffinityChildDontMoveToTBDTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndAffinityChildDontMoveToTBDTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2/D,S/4/V"), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/3"), 0));

        // PLB should not fix affinity violation if the child replica is ToBeDropped/MoveInProgress
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndAffinityParentDontMoveToTBDTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndAffinityParentDontMoveToTBDTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        for (int i = 0; i < 7; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1,S/2/D,S/4,S/5/V"), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/3/D,S/6/V"), 0));

        // ToBeDropped/MoveInProgress replicas from parent and from child are teared down and also parent contains one extra replica.
        // Parent replica should not be moved.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndAffinityChildMoveFromTBDTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndAffinityChildMoveFromTBDTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring parentServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(parentServiceName), 0, CreateReplicas(L"P/0,S/1/D,S/2,S/3/V,S/4"), 0));

        wstring childServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childServiceName, testType, true, parentServiceName));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(childServiceName), 0, CreateReplicas(L"P/0,S/1,S/3"), 0));

        // Secondary child replicas should be moved out from ToBeDropped/MoveInProgress parent replica
        // if there is unpaired parent secondary replica
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 1=>2|4", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 3=>2|4", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndDomainsTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndDomainsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/r{0}", i % 3), wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/4/D,S/2,S/5/V"), 0));

        // PLB should not fix upgrade/fault domain violation if its active replica is
        // on the same domain where ToBeDropped/MoveInProgress replica is
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndReplicaExclusionDynamicTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearAndReplicaExclusionDynamicTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("")));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        int index = 0;
        wstring serviceName;

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FailoverUnit 0 has primary and secondary replica on the same node
        //      PLB should move out one replica
        // FailoverUnit 1 has primary and ToBeDropped secondary replica on the same node
        //      PLB should move out primary
        // FailoverUnit 2 has primary and secondary and ToBeDropped secondary replica on the same node
        //      PLB should move out primary and secondary
        // FailoverUnit 3 has primary and secondary replica on node 0 and ToBeDropped secondary replica on node 1
        //      PLB should move out primary or secondary replica to node 2
        // FailoverUnit 4 has two secondary replicas on node 0 and ToBeDropped primary replica on node 1
        //      PLB should move out one of the secondary replicas to node 2
        // FailoverUnit 5 - 8 are in the same states as FailoverUnit 1 - 4 except those have MoveInProgress replicas
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0,S/0/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0,S/1/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/0,P/1/D"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0,S/0/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/0,S/1/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/0,P/1/V"), 0));

        // PLB should fix violation if it is caused by ToBeDropped/MoveInProgress repica
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(11u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move * 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"2 move * 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move * 0=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 move secondary 0=>2", value)));

        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5 move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"6 move * 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"7 move * 0=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"8 move secondary 0=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearAndCapacityTest)
    {
        // Temporary workaround for bug 13150802 (not present in 6.5).
        PLBConfigScopeChange(CountDisappearingLoadForSimulatedAnnealing, bool, false);
        PLBConfigScopeChange(EnablePreferredSwapSolutionInConstraintCheck, bool, false);

        wstring testName = L"ConstraintCheckWithShouldDisappearAndCapacityTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(IsTestMode, bool, false); // TODO: remove with fix for 3422258

        std::wstring metric = L"MyMetric";

        for (int i = 0; i < 7; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", metric)));
        }
        for (int i = 7; i < 9; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"dc0/r0"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        // Service with standBy replica ToBeDroppedByFM and secondary replica ToBeDroppedByPLB
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/60/60", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"SB/0/D,S/1/V,S/2/V,S/3/R"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/40/50", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/110/110", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/2,S/3,S/4/V,S/5/R"), 0));

        // Node 0:  50 + 60 SB ToBeDropped - should NOT move anything
        // Node 1:  50 + 60 MoveInProgress - should NOT move anything
        // Node 2: 110 + 60 MoveInProgress - should move 110 to one of the nodes with unlimited capacity - 7 or 8
        // Node 3: 110 + 60 ToBeDropped    - should move 110 out to one of the nodes with unlimited capacity - 7 or 8
        // Node 4: 110 MoveInProgress      - should NOT move it
        // Node 5: 110 ToBeDropped         - should NOT move it
        // Node 6: empty, capacity is 100
        // Node 7: empty, capacity unlimited
        // Node 8: empty, capacity unlimited
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 2=>7|8", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move secondary 3=>7|8", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPreventOvercommitMoveBackAffinity)
    {
        wstring testName = L"ConstraintCheckPreventOvercommitMoveBackAffinity";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wstring(L"MyMetric/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wstring(L"MyMetric/100")));

        plb.UpdateService(CreateServiceDescription(L"MyService1", L"MyType", true, CreateMetrics(wformatString("MyMetric/1.0/200/200"))));
        plb.UpdateService(CreateServiceDescription(L"MyService2", L"MyType", true, CreateMetrics(wformatString("MyMetric/1.0/1/1"))));
        plb.UpdateService(CreateServiceDescription(L"MyService3", L"MyType", true, CreateMetrics(wformatString(""))));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"MyService4", L"MyType", true, L"MyService3", CreateMetrics(wformatString(""))));

        //violating capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"MyService1"), 0, CreateReplicas(L"P/0"), 0));
        for (int i = 1; i < 100; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"MyService2"), 0, CreateReplicas(L"P/0"), 0));
        }
        //violating affinity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(100), wstring(L"MyService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(101), wstring(L"MyService4"), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"101 move primary 2=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPreventOvercommitMoveBackApplicationCapacity)
    {
        wstring testName = L"ConstraintCheckPreventOvercommitMoveBackApplicationCapacity";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric1/100,MyMetric2/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wstring(L"MyMetric1/100,MyMetric2/100")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"MyApp",
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"MyMetric2/1000/50/0")));

        plb.UpdateService(CreateServiceDescription(L"MyService1", L"MyType", true, CreateMetrics(wformatString("MyMetric1/1.0/200/200"))));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"MyService2",
            L"MyType",
            L"MyApp",
            true,
            CreateMetrics(L"MyMetric1/1.0/0/0,MyMetric2/1.0/50/0")));

        //violating capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"MyService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"MyService1"), 0, CreateReplicas(L"P/0"), 0));

        //violating application capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"MyService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"MyService2"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2|3 move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPreventOvercommitMoveBackApplicationScaleout)
    {
        wstring testName = L"ConstraintCheckPreventOvercommitMoveBackApplicationScaleout";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric1/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wstring(L"MyMetric1/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wstring(L"MyMetric1/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wstring(L"MyMetric1/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, wstring(L"MyMetric1/100")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"MyApp",
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateService(CreateServiceDescription(L"MyService1", L"MyType", true, CreateMetrics(wformatString("MyMetric1/1.0/200/200"))));
        plb.UpdateService(CreateServiceDescription(L"MyService2", L"MyType", true, CreateMetrics(wformatString("MyMetric1/1.0/200/200"))));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"MyService3",
            L"MyType",
            L"MyApp",
            true,
            CreateMetrics(L"MyMetric1/1.0/0/0")));

        //violating capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"MyService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"MyService2"), 0, CreateReplicas(L"P/1,S/0"), 0));

        //violating application capacity
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"MyService3"), 0, CreateReplicas(L"P/3,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"MyService3"), 0, CreateReplicas(L"P/4,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 3=>4", value)) + CountIf(actionList, ActionMatch(L"3 move primary 4=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearReplicasAndSeparateSecondaryLoadTest)
    {
        wstring testName = L"ConstraintCheckWithShouldDisappearReplicasAndSeparateSecondaryLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100", metric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/40", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/50/50", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/3"), 0));
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(3), 45));
        // Replica 3 load 45 will be added into secondary load map
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, metric, 50, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 3=>2", value)));

        // ToBeDropped replica on node 3 will be added to the node 3 load (value of updated load - 45)
        // but replica 3 in secondary load map should not be replaced as movement is still in progress.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"P/0,S/1,S/2,S/3/D"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Update with removing ToBeDropped replica from partition reconfiguration should remove load from replica on node 3 (saved in map).
        // This is checking are we going to remove load greater than ToBeDropped replica load - default load as 50.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 2, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/55/55", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L""), 1));

        // There should be 55 free capacity only on node 2
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime + PLBConfig::GetConfig().MinPlacementInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndPlbRestartsTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndPlbRestartsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/150", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 deafult secondary load is 10, but it is increased to 140
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 140));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 10, loads));

        // FU1
        wstring tempServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Increased FU0 secondary replica should be moved to node 2
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Updating FU0 with the in-progress move
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1/D,I/2/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Increase FU1 primary replica load
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, tempServiceName, metric, 110, 110));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/150", metric)));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before PLB restart:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 10, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 110, 2);

        // Primary replica from FU1 should be moved out as its load is increased and FU0 is in transition
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 2=>3", value)));

        // FM and PLB restart
        fm_->Load();
        PlacementAndLoadBalancing & newPlb = fm_->PLB;

        // Resend all node updates:
        newPlb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        newPlb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        newPlb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/150", metric)));
        newPlb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/150", metric)));

        // Resend service type:
        newPlb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // Resend all the updates for FU0 (inBuild replica doesn't report the load):
        newPlb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        newPlb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1/D,I/2/B"), 0));
        loads.clear();
        loads.insert(make_pair(CreateNodeId(1), 140));
        newPlb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 10, loads));

        // Resend all the updates for FU1:
        newPlb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        newPlb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/2"), 0));
        newPlb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(2, tempServiceName, metric, 110, 110));

        newPlb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after PLB restart:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 10, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        CheckLoadValue(tempServiceName, 2, metric, ReplicaRole::Primary, 110, 2);

        // Primary replica from FU1 should be moved out as its load is increased and FU0 is in transition
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        // VERIFY_ARE_EQUAL(1u, actionList.size());
        // VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 2=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndInterruptedMoveAndExistingReplicaLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndInterruptedMoveAndExistingReplicaLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/150", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 deafult load on nodes 0 and 1 is 10, but it is increased to 55
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 55));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 55, loads));

        // FU1
        wstring tempServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Default load of FU1 on node 1 is 10, but it is updated to 555
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, tempServiceName, metric, 555, 555));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 replicas from node 1 should be moved out to the node 2 (as FU1 replica has huge load)
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Updating FU0 with the in-progress move (like it was chosen to move FU0)
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1/D,I/2/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before rejecting move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 55, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 55, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 55, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 555, 1);

        // Updating FU0 - interrupted move, but replica on node 1 should still have load 55
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after rejecting move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 55, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 55, 1);
        // load map only has entry for node 1
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 555, 1);

        // Default load of FU1 on node 1 is 10, but it is updated to 55
        // PLB can choose which replica to move out of node 1
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, tempServiceName, metric, 55, 55));
        plb.ProcessPendingUpdatesPeriodicTask();

        // One of two replicas on node 1 should be moved to the node 2
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move * 1=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndInterruptedMoveAndNewReplicaLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndInterruptedMoveAndNewReplicaLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/150", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 default load is 10, primary on node 0 is increased to 60, secondary on node 1 is increased to 140
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 140));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 60, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU1
        wstring tempServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Increased FU0 secondary replica should be moved to node 2
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Updating FU0 with the in-progress move
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1/D,I/2/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before rejecting move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 60, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 10, 2);

        // Updating FU0 - interrupted move and new replica should be added, with the deafult load
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/1"), 1));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after rejecting move:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 60, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        // Move is cancelled, load map only has node 1
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 10, 2);

        // New replica should be added, with the deafult load
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 2", value)));

        // Updating FU0 with the created secondary replica - load should be average secondary load of 140
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 3, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after adding new replica:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 60, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        // Node 2 has new replica, it should use average load and update the load map (140/1)
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 10, 2);

        // Default load of FU1 on node 2 is 10, but lets updated it to 15 in order to check
        // is it going to be moved out of the node 2 when additional replica of the FU0 cames
        // (additional FU0 replica should not come with the load 140).
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, tempServiceName, metric, 15, 15));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/150", metric)));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Nothing should be moved out of the node 2
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary *=>3", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndUpdateLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndUpdateLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/150", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 default load is 10, primary load on node 0 is increased to 10, secondary load on node 1 is increased to 140
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 140));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 50, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU1
        wstring tempServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(tempServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(tempServiceName), 0, CreateReplicas(L"P/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Increased FU0 secondary replica should be moved to node 2
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>2", value)));

        // Updating FU0 with the in-progress move
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1/D,I/2/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before moving replica load update:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 50, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 10, 2);

        // Reducing the FU0 source replica load that is beeing moved - target replica load should also be reduced
        loads.clear();
        loads.insert(make_pair(CreateNodeId(1), 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 50, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after moving replica load update:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 50, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 50, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 140, 2);
        CheckLoadValue(tempServiceName, 1, metric, ReplicaRole::Primary, 10, 2);

        // Increasing the FU1 primary replica load on a node 2 node from 10 to 100
        // Total load on node 2 should be 150
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 100, 100));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/150", metric)));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Nothing should be moved out
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndUpdateLoadAssertTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndUpdateLoadAssertTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, wformatString("{0}/100", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, false, CreateMetrics(wformatString("{0}/1.0/0/0", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index), wstring(mainServiceName), 0, CreateReplicas(L"S/0,S/1"), 0));

        // FU0 default load is 0, secondary load for both replicas is increased to 90
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(0), 90));
        loads.insert(make_pair(CreateNodeId(1), 90));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 0, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before move 1->2:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        // load map has node 0 and 1
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 3);

        // FU0: Trigger move from 1 to 2
        NodeId newSecondary = CreateNodeId(2);
        VERIFY_IS_TRUE(!fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(1), newSecondary, false).IsSuccess());
        newSecondary = CreateNodeId(2);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(1), newSecondary, true).IsSuccess());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move instance 1=>2", value)));

        // Memory structures check after move 1->2:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 3);

        // FU0: FM rejected the move
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"S/0,S/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after FM rejected move 1->2:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 3);

        // FU0: Trigger move from 1 to 3
        fm_->Clear();
        newSecondary = CreateNodeId(3);
        VERIFY_IS_TRUE(!fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(1), newSecondary, false).IsSuccess());
        newSecondary = CreateNodeId(3);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(1), newSecondary, true).IsSuccess());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move instance 1=>3", value)));

        // Memory structures check after move 1->3:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 3);

        // Apply move 1->3 - start
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index), wstring(mainServiceName), 2, CreateReplicas(L"S/0,S/1/D,I/3/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after applying move 1->3:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // Apply move 1->3 - end, and adding one more replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index), wstring(mainServiceName), 3, CreateReplicas(L"S/0,S/3"), 1));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after applying move 1->3:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // Missing secondary replica should be created on node 2
        StopwatchTime now = Stopwatch::Now();
        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add instance 2", value)));

        // Memory structures check after adding replica on a node 2 - replica 2 should have default load:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // Apply adding one replica to node 2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index), wstring(mainServiceName), 4, CreateReplicas(L"S/0,S/2,S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after applying adding replica on a node 2 - replica 2 should have average load:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 1);
        // node 2 should use average (90 + 90) / 2
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // FU0: Trigger move from 0 to 1
        fm_->Clear();

        newSecondary = CreateNodeId(1);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(0), newSecondary).IsSuccess());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move instance 0=>1", value)));

        // Memory structures check after move 0->1:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // Apply move 0->1 - start
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index), wstring(mainServiceName), 5, CreateReplicas(L"S/0/D,S/2,I/1/B,S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after applying move 0->1 - start:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);

        // Apply move 0->1 - end
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index), wstring(mainServiceName), 6, CreateReplicas(L"S/2,S/1,S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after applying move 0->1 - end:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 90, 3);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndResetPartitionLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndResetPartitionLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100", metric)));
        }

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i + 2, wformatString("{0}/150", metric)));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        int index = 0;
        wstring mainServiceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // FU0 default load is 10 and both primary on node 0 and secondary load on node 1 are increased to 150
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 150, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 replicas should be moved out - to the nodes with higher total capacity
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>*", value)));

        // Memory structures check before replica load reset:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 150, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 150, 1);

        // Reset to default load
        plb.ResetPartitionLoad(Reliability::FailoverUnitId(CreateGuid(0)), mainServiceName, true);
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after replica load reset:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 10, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 1);

        // No more violations, nothing should be moved out
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->Clear();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndStaleSwapTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndStaleSwapTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wformatString("{0}/30", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, wformatString("{0}/100", metric)));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/20/60", metric))));
        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"S/0,S/1,P/2"), 0)));
        fm_->UpdatePlb();

        // FU0: primary load 20, secondary on node 0 has load 30, secondary on node 1 has load 50 (in violation)
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(0), 30));
        loads.insert(make_pair(CreateNodeId(1), 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 20, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 20, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 30, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 50, 1);
        // node 2 should not be in secondary load map, checking load value should return default load
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 60, 2);

        // FU0 primary should be swapped out to fix constraint violation
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>1", value)));

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->ClearMoveActions();

        // FU0: primary load increased from 20 to 40 so node 1 is again in violation
        loads.clear();
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 40, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 40, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 30, 0);
        // node 1 should not be in secondary load map, checking load value should return default load
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 60, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 50, 2);

        // Node 1 in violation, only secondary from node 0 (with load 30) can fit on node 1
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 1<=>0", value)));

        fm_->ApplyActions();
        fm_->UpdatePlb();
        fm_->ClearMoveActions();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 40, 0);
        // node 0 should not be in secondary load map, checking load value should return default load
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 60, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 30, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 50, 2);

        // No violation expected
        now += PLBConfig::GetConfig().MaxMovementExecutionTime +
            PLBConfig::GetConfig().MinPlacementInterval +
            PLBConfig::GetConfig().MinConstraintCheckInterval;
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndMissingReplicasTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndMissingReplicasTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        // 100      100     100     100     100     150
        int i = 0;
        for (; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100", metric)));
        }

        for (; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/150", metric)));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        // FU0 default load is 10 and both primary on node 0 and secondary load on node 1 are increased to 150
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 150));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 15, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 15, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 150, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 3);

        // FU0 replicas should be moved out - to the nodes with higher total capacity
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 1=>5", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/2,S/3,S/5"), 1));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before replica load reset:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 150, 5);

        fm_->Clear();

        now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary *", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/2,S/3,S/4,S/5"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before replica load reset:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 3);
        // node 4 should use average
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 57, 4);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 150, 5);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(6, wformatString("{0}/100", metric)));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(7, wformatString("{0}/100", metric)));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();

        NodeId newSecondary = CreateNodeId(6);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(2), newSecondary).IsSuccess());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 2=>6", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 3, CreateReplicas(L"P/0,S/3,S/4,S/5,S/6"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 3);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 57, 4);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 150, 5);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 6);

        fm_->Clear();
        newSecondary = CreateNodeId(7);
        VERIFY_IS_TRUE(fm_->PLBTestHelper.TriggerMoveSecondary(mainServiceName, CreateGuid(0), CreateNodeId(4), newSecondary).IsSuccess());

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move secondary 4=>7", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 4, CreateReplicas(L"P/0,S/3,S/5,S/6,S/7"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Move is 4->7, so node 7 should use old average. The entry in load map on node 4 should be removed.
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 10, 4);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 57, 7);

        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/20/20", metric))));

        // Reset to default load
        plb.ResetPartitionLoad(Reliability::FailoverUnitId(CreateGuid(0)), mainServiceName, true);
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check after replica load reset:
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Primary, 20, 0);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 20, 3);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 20, 5);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 20, 6);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 20, 7);

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadMultiMetricsTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadMultiMetricsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring m1 = L"MyMetric_1";
        std::wstring m2 = L"MyMetric_2";
        std::wstring m3 = L"MyMetric_3";

        int i = 0;
        for (; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100,{1}/100,{2}/100", m1, m2, m3)));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true,
            CreateMetrics(wformatString("{0}/1.0/10/10,{1}/1.0/10/10,{2}/1.0/10/10", m1, m2, m3))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // FU0 default load is 10 and both primary on node 0 and secondary load on node 1 are increased to 150
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, m1, 10, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        loads.clear();
        loads.insert(make_pair(CreateNodeId(2), 40));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, m2, 10, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 50, 1);
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 10, 3);

        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 40, 2);
        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 10, 3);

        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 3);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Memory structures check before replica load reset:
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 50, 1);
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 10, 3);
        CheckLoadValue(mainServiceName, 0, m1, ReplicaRole::Secondary, 23, 4);

        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 40, 2);
        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 10, 3);
        CheckLoadValue(mainServiceName, 0, m2, ReplicaRole::Secondary, 20, 4);

        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 1);
        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 2);
        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 3);
        CheckLoadValue(mainServiceName, 0, m3, ReplicaRole::Secondary, 10, 4);

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithSeparateSecondaryLoadAndNewReplicaAverageLoadTest)
    {
        wstring testName = L"ConstraintCheckWithSeparateSecondaryLoadAndNewReplicaAverageLoadTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::wstring metric = L"MyMetric";

        // 100      100     100     10     10
        int i = 0;
        for (; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/100", metric)));
        }

        for (; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/10", metric)));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // FU0
        wstring mainServiceName = wformatString("{0}{1}", testName, 0);
        plb.UpdateService(CreateServiceDescription(mainServiceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/0", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        // Secondary load updats 1(50), 2(30)
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 50));
        loads.insert(make_pair(CreateNodeId(2), 30));

        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, mainServiceName, metric, 15, loads));
        plb.ProcessPendingUpdatesPeriodicTask();

        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 50, 1);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 30, 2);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 0, 3);
        CheckLoadValue(mainServiceName, 0, metric, ReplicaRole::Secondary, 40, 3, true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 1, CreateReplicas(L"P/0,S/1,S/2"), 1));

        // New replica shouldn't be placed, since the load is 40 if average load used
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, wformatString("{0}/50", metric)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(mainServiceName), 2, CreateReplicas(L"P/0,S/1,S/2"), 2));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->Clear();

        now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 5", value)));

    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithDomainViolationDueToPlacement)
    {
        wstring testName = L"ConstraintCheckWithDomainViolationDueToPlacement";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"fd:/3", L"5", L"", Reliability::NodeDeactivationIntent::Enum::None));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"fd:/1", L"2", L"", Reliability::NodeDeactivationIntent::Enum::Restart));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"fd:/4", L"7", L"", Reliability::NodeDeactivationIntent::Enum::None));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"fd:/2", L"4", L"", Reliability::NodeDeactivationIntent::Enum::None));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"fd:/4", L"2", L"", Reliability::NodeDeactivationIntent::Enum::Restart));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring parentServiceName = wformatString("{0}_Parent", testName);
        plb.UpdateService(CreateServiceDescription(parentServiceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentServiceName), 0, CreateReplicas(L"P/3,S/1,S/2,S/4"), 0));

        wstring childServiceName = wformatString("{0}_Child", testName);
        plb.UpdateService(CreateServiceDescriptionWithNonAlignedAffinity(childServiceName, testType, true, wstring(parentServiceName)));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(childServiceName), 0, CreateReplicas(L"P/2,S/5,S/1,S/4"), 0));

        // Point of the test is to check if PLB will see that replicas on nodes 2 and 5 are in FD/UD violation.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        // The verify below would be true when constraint check is allowed for deactivated nodes in future
        // VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move secondary 5=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWith4LevelFaultDomains331Test)
    {
        wstring testName = L"ConstraintCheckWith4LevelFaultDomains331Test";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int floor = 0; floor < 3; floor++)
        {
            for (int rack = 0; rack < 5; rack++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(
                    floor * 5 + rack, wformatString("fd/cluster{0}/floor{0}/rack{1}", floor, rack), L""));
            }
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/5,S/6,S/7,S/10"), 0));

        // fd/clusterX has only one child fault domain floorX
        // fd is in problem - child fault domains have: 3, 3, 1
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1|2|5|6|7=>11|12|13|14", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithStandaloneFaultDomainTest)
    {
        wstring testName = L"ConstraintCheckWithStandaloneFaultDomainTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // without setting these weights to 0, this test repros bug #8056489 with violation becomes worse
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"fd/left/sub0/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"fd/left/sub0/1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"fd/left/sub1/2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"fd/left/sub1/3", L""));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"fd/mid/sub2/4", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"fd/mid/sub3/5", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, L"fd/mid/sub3/6", L""));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7, L"fd/right/sub4/7", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(8, L"fd/right/sub4/8", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(9, L"fd/right/sub4/9", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(10, L"fd/right/sub4/10", L""));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/4,S/5,S/6,S/7"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/7,S/8"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/4,S/5,S/6,S/7,S/8,S/9"), 0));

        // fault domain fd/right has only one child - sub4.
        // 0. partition: fd is in problem - children sub-domains have: 3, 3, 1
        // 1. partition: fd is in problem - children sub-domains have: 4, 1, 2
        // 2. partition: fd is in problem - children sub-domains have: 1, 3, 3
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1|5|6=>8|9|10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1|2|3=>5|6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move * 5|6|7|8|9=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithoutStandaloneFaultDomainTest)
    {
        wstring testName = L"ConstraintCheckWithoutStandaloneFaultDomainTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // without setting these weights to 0, this test repros bug #8056489 with violation becomes worse
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"fd/left/sub0/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"fd/left/sub0/1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"fd/left/sub1/2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"fd/left/sub1/3", L""));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, L"fd/mid/sub2/4", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, L"fd/mid/sub3/5", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, L"fd/mid/sub3/6", L""));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7, L"fd/right-sub4/7", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(8, L"fd/right-sub4/8", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(9, L"fd/right-sub4/9", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(10, L"fd/right-sub4/10", L""));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/4,S/5,S/6,S/7"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/7,S/8"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/4,S/5,S/6,S/7,S/8,S/9"), 0));

        // 0. partition: fd is in problem - children sub-domains have: 3, 3, 1
        // 1. partition: fd is in problem - children sub-domains have: 4, 1, 2
        // 2. partition: fd is in problem - children sub-domains have: 1, 3, 3
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1|5|6=>8|9|10", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1|2|3=>5|6", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move * 5|6|7|8|9=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPlacePartiallyDistributionTest)
    {
        wstring testName = L"ConstraintCheckPlacePartiallyDistributionTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"MyMetric/25"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"MyMetric/12"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"MyMetric/0"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"MyMetric/0"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"MyMetric/12"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(5, L"MyMetric/12"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"PartiallyTestService", L"TestType", true, CreateMetrics(L"MyMetric/1.0/10/5")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"NonPartiallyTestService1", L"TestType", true, L"PlacePolicy ~ NonPartially", CreateMetrics(L"MyMetric/1.0/10/5")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"NonPartiallyTestService2", L"TestType", true, L"PlacePolicy ~ NonPartially", CreateMetrics(L"MyMetric/1.0/10/5")));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"NonPartiallyTestService3", L"TestType", true, L"PlacePolicy ~ NonPartially", CreateMetrics(L"MyMetric/1.0/100/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"PartiallyTestService"), 0, CreateReplicas(L""), 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"NonPartiallyTestService1"), 0, CreateReplicas(L""), 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"NonPartiallyTestService2"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"NonPartiallyTestService3"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(7u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary *", value)));
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"0 add secondary *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 add primary *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"1 add secondary *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add primary *", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 add secondary *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"3 add primary *", value)));
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"3 add secondary *", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithFDViolationThatShouldBeFixedOnAFirstLevelTest)
    {
        wstring testName = L"ConstraintCheckWithFDViolationThatShouldBeFixedOnAFirstLevelTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring myMetric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/left/r{0}", i), wformatString("{0}/100", myMetric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/right/r{0}", 3), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/40", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/4,S/5,S/6"), 0));

        // Replica distribution accross domains is left: 1, 1, 0 and right: 1, 3
        // One replica from domain right/r4 should be moved to domain left/2
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 4|5|6=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithUnderloadedFaultDomainTest)
    {
        wstring testName = L"ConstraintCheckWithUnderloadedFaultDomainTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring myMetric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/left/r{0}", i), wformatString("{0}/100", myMetric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/right/r{0}", 3), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/40", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/3,S/4,S/5"), 0));

        // Replica distribution accross domains is left: 1, 1, 0 and right: 1, 2
        // All sub-domains are balanced, nothing should be moved.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithMoveToUnderloadedFaultDomainTest)
    {
        wstring testName = L"ConstraintCheckWithMoveToUnderloadedFaultDomainTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring myMetric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/left/r{0}", i), wformatString("{0}/100", myMetric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/right/r{0}", 3), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, wformatString("dc0/right/r{0}", 4), wformatString("{0}/50", myMetric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/40", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/70/70", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4"), 0));

        // Both FU0 and FU1: Replica distribution accross domains is left: 1, 1, 1 and right: 1, 1
        // Only replicas from FU0 can be moved to the node 5 because of node capacity and it should be allowed according to FD constraint
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * *=>5", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithUnderloadedFaultDomainAndInTransitionReplicasTest)
    {
        wstring testName = L"ConstraintCheckWithUnderloadedFaultDomainAndInTransitionReplicasTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring myMetric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/left/r{0}", i), wformatString("{0}/100", myMetric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/right/r{0}", 3), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, wformatString("dc0/right/r{0}", 4), wformatString("{0}/50", myMetric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/40", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,P/1/B,S/2,S/3/D,S/4,S/5"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/4,S/5"), 0));

        // FU0 has replica on every node - it is not in FD violation
        // FU1 has two replicas in the same domain right/4 - one should be moved to left domain
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 4|5=>0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithFaultDomainViolationAndBalancedTargetDomainTest)
    {
        wstring testName = L"ConstraintCheckWithFaultDomainViolationAndBalancedTargetDomainTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        wstring myMetric = L"MyMetric";
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, wformatString("dc0/left/r{0}", 1), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, wformatString("dc0/left/r{0}", 2), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, wformatString("dc0/left/r{0}", 2), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/left/r{0}", 2), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/left/r{0}", 3), wformatString("{0}/100", myMetric)));

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(5, wformatString("dc0/right/r{0}", 4), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(6, wformatString("dc0/right/r{0}", 5), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(7, wformatString("dc0/right/r{0}", 6), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(8, wformatString("dc0/right/r{0}", 6), wformatString("{0}/100", myMetric)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        wstring serviceName;
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/40/40", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3,S/4,S/5,S/6,S/7"), 0));

        // Replica distribution accross domains is left: 1, 3, 1 and right: 1, 1, 1
        // Domains left and right are not balanced, one replica from domain left/r2 should be moved to the node 8
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 1|2|3=>8", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithApplicationScaleoutCountTest1)
    {
        wstring testName = L"ConstraintCheckWithApplicationScaleoutCountTest1";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Down node- just to make sure that nothing gets moved here.
        plb.UpdateNode(NodeDescription(CreateNodeInstance(5, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        int appScaleoutCount = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(appName1), std::map<std::wstring, ApplicationCapacitiesDescription>(), appScaleoutCount));


        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // Nothing gets moved to down node:
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * *=>5", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithApplicationScaleoutCountTest2)
    {
        wstring testName = L"ConstraintCheckWithApplicationScaleoutCountTest2";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

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

        // Do placement first
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"0 add * 0|1|2", value)));

        fm_->Clear();

        wstring serviceWithScaleoutCount2 = wformatString("{0}_Service{1}", testName, L"2");
        // service1 can be placed on any node because no capacity defined
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceWithScaleoutCount1), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceWithScaleoutCount2), 0, CreateReplicas(L"P/3, S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Do constraint check second
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 move * *=>0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainWithApplicationTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainWithApplicationTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(ScaleoutCountConstraintPriority, int, 0);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 3;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"fd:/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"fd:/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"fd:/1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"fd:/2", L""));

        wstring applicationName = wformatString("{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, vector<ServiceMetric>()));

        // FD violation: two replicas are in fd:/0, no replicas are in fd:/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));

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

        // We need to fix FD constraint (final outcome will be that scaleout count is respected)!
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move secondary 0|1=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFaultDomainWithApplicationTwoPartitionsTest)
    {
        wstring testName = L"ConstraintCheckFaultDomainWithApplicationTwoPartitionsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 3;

        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(0, L"fd:/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(1, L"fd:/0", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(2, L"fd:/1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, L"fd:/2", L""));

        wstring applicationName = wformatString("{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, vector<ServiceMetric>()));

        // FD violation: each partition has two replicas are in fd:/0, no replicas are in fd:/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/2,S/1,S/0"), 0));

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

        // We need to fix FD constraint (final outcome will be that scaleout count is respected)!
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        int64 movesFrom0 = CountIf(actionList, ActionMatch(L"0|1 move secondary 0=>*", value));
        int64 movesFrom1 = CountIf(actionList, ActionMatch(L"0|1 move secondary 1=>*", value));
        int64 movesTo3 = CountIf(actionList, ActionMatch(L"* move * *=>3", value));
        bool situationOK = ((movesFrom0 == 2 && movesFrom1 == 0) || (movesFrom0 == 0 && movesFrom1 == 2)) && (movesTo3 == 2);
        VERIFY_IS_TRUE(situationOK);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAfterApplicationUpdate)
    {
        wstring testName = L"ConstraintCheckAfterApplicationUpdate";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // First, add service with application name but without an application
        wstring applicationName = wformatString("{0}_Application", testName);
        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, CreateMetrics(L"CPU/1.0/10/10")));

        // Update failover units, so that nodes 0,1,2 have load of 20 and nodes 3,4,5 have 10
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/3,S/4,S/5"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        // Refresh the PLB and verify that nothing happens (no moves)
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Now add application with scaleout count of 3
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            3));

        // Refresh the PLB and check if application will be moved to 3 nodes
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"1 move * 3|4|5=>0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckofAppCapacityWithMultiMetrics)
    {
        wstring testName = L"ConstraintCheckofAppCapacityWithMultiMetrics";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Down node- just to make sure that nothing gets moved here.
        plb.UpdateNode(NodeDescription(CreateNodeInstance(6, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));

        // First, add service with application name but without an application
        wstring applicationName = wformatString("{0}_Application", testName);
        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName0 = wformatString("{0}_Service_0", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName0), testType, wstring(applicationName), true,
            CreateMetrics(L"Metric1/1.0/10/10,Metric2/1.0/10/10")));

        wstring serviceName1 = wformatString("{0}_Service_1", testName);
        plb.UpdateService(CreateServiceDescription(wstring(serviceName1), testType, true, CreateMetrics(L"Metric2/1.0/10/10,Metric3/1.0/10/10")));

        // Update failover units, so that nodes 0,1,2 have load of 20 for service with application and nodes 3,4,5 have 10 for a regular service
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName0), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName0), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName1), 0, CreateReplicas(L"P/3,S/4,S/5"), 0));

        // Refresh the PLB and verify that nothing happens (no moves)
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Update the app with 15 per node cap, so 3 replica has to be moved
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            wstring(applicationName),
            6, // scaleout is not a constraint in this test
            CreateApplicationCapacities(L"Metric2/90/15/0")));

        // Refresh the PLB and check if application will be moved to 3 nodes
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);

        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * 0=>3|4|5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * 1=>3|4|5", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"* move * 2=>3|4|5", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithApplicationScaleoutCountE2ETest)
    {
        wstring testName = L"ConstraintCheckWithApplicationScaleoutCountE2ETest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        // PLB should be able to fix this situation in the first attempt.
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int numNodes = 9;

        int numFUs = 101;

        for (int i = 0; i < numNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"NoSleepMetric/300"));
        }

        // First, add service with application name but without an application
        wstring applicationName = wformatString("{0}_Application", testName);
        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, CreateMetrics(L"NoSleepMetric/1.0/1/1")));

        for (int fuIndex = 0; fuIndex < numFUs; fuIndex++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex),
                wstring(serviceName),
                0,
                CreateReplicas(wformatString("P/{0},S/{1},S/{2}",
                fuIndex % numNodes,
                (fuIndex + 1) % numNodes,
                (fuIndex + 2) % numNodes)),
                0));
        }

        // Refresh the PLB and verify that nothing happens (no moves)
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Now add application with scaleout count of 4 (and min node count of 1)
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            1, 4));

        // Refresh the PLB and check if application will be moved to 3 nodes
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);

        // Verify that we are moving something...
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_NOT_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckApplicationAndDissapearingReplicasNotNeeded)
    {
        wstring testName = L"ConstraintCheckApplicationAndDissapearingReplicasNotNeeded";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        int appScaleoutCount = 2;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));

        wstring applicationName = wformatString("{0}_Application", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(applicationName), true, vector<ServiceMetric>()));

        // Two FUs are placed on nodes 0 and 1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1,S/0"), 0));
        // This FailoverUnit is moving - S is moving from Node2 (MiP) to Node1 (S/IB), P has moved from node 3 (S/TBD) to Node 0 (P)
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1/B,S/2/V,S/3/D"), 0));

        // Scaleout count == 2:
        // Node0: 1P + 1S + 1InBuild
        // Node1: 2P + 1S
        // Node2: 1 MoveInProgress
        // Node3: 1ToBeDropped
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckApplicationCapacityAndDissapearingReplicasNotNeeded)
    {
        wstring testName = L"ConstraintCheckApplicationCapacityAndDissapearingReplicasNotNeeded";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

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

        // One FailoverUnit with
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0/D,S/1/V"), 0));

        // Scaleout count == 2, MaxNodeCapacity == 10 - NO VIOLATION:
        // Node0: Load: 10, Dissapearing load: 10
        // Node1: Load: 10, Dissapearing load: 10
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckOnDownNode)
    {
        wstring testName = L"ConstraintCheckOnDownNode";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(), L"CPU/100", false));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(), L"CPU/150", true));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(serviceName), testType, wstring(L""), true, CreateMetrics(L"CPU/1.0/120/120")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0", false), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithBigLoads)
    {
        wstring testName = L"ConstraintCheckWithBigLoads";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"SurprizeMetric/2147483647"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"SurprizeMetric/2147483647"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceSmall", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestServiceBig", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/2147483647/2147483647")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestServiceSmall"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestServiceBig"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Big load for two services
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestServiceSmall", L"SurprizeMetric", INT_MAX, std::map<Federation::NodeId, uint>()));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)), 1u);
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(CapacityConstraintCheckWithDownNodes)
    {
        wstring testName = L"CapacityConstraintCheckWithDownNodes";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 30);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"SurprizeMetric/30"));
        // Down node!
        plb.UpdateNode(NodeDescription(CreateNodeInstance(1, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"SurprizeMetric/1.0/20/20")));

        // 2 x 20 load on node 0
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), 0u);
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAppCapacityComplexFix)
    {
        wstring testName = L"ConstraintCheckAppCapacityComplexFix";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 1);

        for (int i = 0; i < 250; ++i)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App",
            0,  // MinimumNodes - no reservation
            61,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/61/1/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        for (int i = 0; i < 60; ++i)
        {
            plb.UpdateService(CreateServiceDescriptionWithApplication(
                wformatString("Parent{0}", i),
                L"TestType",
                wstring(L"App"),
                true,
                CreateMetrics(L"SurprizeMetric/61/1/0")));
        }
        for (int i = 0; i < 60; ++i)
        {
            plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(wformatString("Child{0}", i), L"TestType", true, L"App", wformatString("Parent{0}", i), CreateMetrics(L"")));
        }

        for (int i = 0; i < 43; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wformatString("Parent{0}", i), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i + 1000), wformatString("Child{0}", i), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        for (int i = 43; i < 55; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wformatString("Parent{0}", i), 0, CreateReplicas(wformatString("P/{0}", i - 43)), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i + 1000), wformatString("Child{0}", i), 0, CreateReplicas(wformatString("P/{0}", i - 43)), 0));
        }
        for (int i = 55; i < 60; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wformatString("Parent{0}", i), 0, CreateReplicas(wformatString("P/{0}", i - 55)), 0));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i + 1000), wformatString("Child{0}", i), 0, CreateReplicas(wformatString("P/{0}", i - 55)), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        // there are 17 violations
        // 5 nodes have 3 primary replicas - Nodes[0..4]
        // 12 nodes have 2 primary replicas (5 of them are previous one) -> Nodes[0..11]
        // 12 + 5 = 17 -> plb has to make 17 movements and x2 for both child and parent
        VERIFY_ARE_EQUAL(actionList.size(), 34u);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckFixAffinityOverNodeCapacityImprovement)
    {
        wstring testName = L"ConstraintCheckFixAffinityOverNodeCapacityImprovement";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, Common::TimeSpan::FromSeconds(0.5));
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, -1);
        PLBConfigScopeChange(AffinityConstraintPriority, int, 0);
        PLBConfigScopeChange(CapacityConstraintPriority, int, 0);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(0, L"FD0", L"UD0", L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(1, L"FD1", L"UD1", L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(2, L"FD2", L"UD2", L"M1/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(L"TestApp", 1, 3, CreateApplicationCapacities(L"")));
        set<NodeId> parentServiceBlocklist;
        parentServiceBlocklist.insert(CreateNodeId(1));
        parentServiceBlocklist.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ParentServiceType"), move(parentServiceBlocklist)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"ParentService", L"ParentServiceType", L"TestApp", true, CreateMetrics(L"")));

        set<NodeId> childServiceBlocklist;
        childServiceBlocklist.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ChildServiceType"), move(childServiceBlocklist)));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"ChildService", L"ChildServiceType", true, L"TestApp", L"ParentService", CreateMetrics(L"M1/1/10/0")));

        set<NodeId> otherServiceBlocklist;
        otherServiceBlocklist.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"OtherServiceType"), move(otherServiceBlocklist)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"OtherService", L"OtherServiceType", L"TestApp", true, CreateMetrics(L"M1/1/60/0")));

        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(L"ParentService"), 0, CreateReplicas(L"P/0"), 0)));
        for (int i = 1; i < 11; i++)
        {
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(L"ChildService"), 0, CreateReplicas(wformatString("P/{0}", i > 4 ? 1 : 0)), 0)));
        }
        for (int i = 11; i < 14; i++)
        {
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(L"OtherService"), 0, CreateReplicas(L"P/2"), 0)));
        }
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:   | N0(M1: 40/100) | N1(M1: 60/100) | N2(M1: 180/100) |
        // Parent:  |       P        |  blocklisted   |  blocklisted    |
        // Child:   |     4xP        |     6xP        |  blocklisted    |
        // Other:   |                |  blocklisted   |      3xP        |

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(6u, actionList.size());
        VERIFY_ARE_EQUAL(6u, CountIf(actionList, ActionMatch(L"* move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentAndNodeCapacityConstraintTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentAndNodeCapacityConstraintTest");
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100"));
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
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType2", true, CreateMetrics(L"M1/1.0/50/30")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService3",
            L"TestType3",
            true,
            L"TestService2",
            CreateMetrics(L"M1/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"1 move primary 0=>2", value)), 1u);

        // Initial placement:
        // Nodes:       |   N0(M1:50)    |  N1(M1:100)    |   N2(M1:100)    |
        // Parent (P0): |        P       |                |                 |
        // Parent (P2): |                |       P        |                 |
        // Child  (C1): |                |                |      P(P0)      |
        // Child  (C3): |                |       P(P2)    |                 |
        // Other:       |                |                |                 |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckRespectMoveCostMediumPositive)
    {
        wstring testName = L"ConstraintCheckRespectMoveCostMediumPositive";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(L"CPU", 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/50"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int i = 0; i < 30; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/0/1/1")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i, serviceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_LOW, std::map<Federation::NodeId, uint>()));
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        wstring largeServiceName = L"TestServiceLarge";
        plb.UpdateService(CreateServiceDescription(largeServiceName, L"TestType", true, CreateMetrics(L"CPU/0/30/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(30), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(30, largeServiceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_MEDIUM, std::map<Federation::NodeId, uint>()));
        fm_->FuMap.insert(make_pair(CreateGuid(30), FailoverUnitDescription(CreateGuid(30), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0)));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Verify that 10 small instances (cost 10) get moved instead of the large instance (cost 15)
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"30 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckRespectMoveCostMediumNegative)
    {
        wstring testName = L"ConstraintCheckRespectMoveCostMediumNegative";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(L"CPU", 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/50"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int i = 0; i < 40; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/0/1/1")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i, serviceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_LOW, std::map<Federation::NodeId, uint>()));
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        wstring largeServiceName = L"TestServiceLarge";
        plb.UpdateService(CreateServiceDescription(largeServiceName, L"TestType", true, CreateMetrics(L"CPU/0/30/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(40), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(40, largeServiceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_MEDIUM, std::map<Federation::NodeId, uint>()));
        fm_->FuMap.insert(make_pair(CreateGuid(40), FailoverUnitDescription(CreateGuid(30), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0)));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Verify that 1 large instance (cost 15) is moved instead of 20 small instances (cost 20)
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"40 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckRespectMoveCostHigh)
    {
        wstring testName = L"ConstraintCheckRespectMoveCostHigh";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(L"CPU", 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/50"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        for (int i = 0; i < 40; i++)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/0/1/1")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(i, serviceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_LOW, std::map<Federation::NodeId, uint>()));
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        wstring largeServiceName = L"TestServiceLarge";
        plb.UpdateService(CreateServiceDescription(largeServiceName, L"TestType", true, CreateMetrics(L"CPU/0/30/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(40), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(40, largeServiceName, wstring(*Constants::MoveCostMetricName), FABRIC_MOVE_COST::FABRIC_MOVE_COST_HIGH, std::map<Federation::NodeId, uint>()));
        fm_->FuMap.insert(make_pair(CreateGuid(40), FailoverUnitDescription(CreateGuid(30), wstring(largeServiceName), 0, CreateReplicas(L"P/0"), 0)));

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        // Verify that 10 small instances (cost 20) get moved instead of the large instance (cost 40)
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"40 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(20, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckRespectBalancingScore)
    {
        wstring testName = L"ConstraintCheckRespectBalancingScore";
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
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, PLBConfig::KeyDoubleValueMap, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(metric, 1.0));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        const int nodeCount = 10;
        for (int i = 0; i < nodeCount; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"CPU/100"));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        int serviceCount = 30;
        for (int i = 0; i < serviceCount; ++i)
        {
            wstring serviceName = wformatString("TestService{0}", i);
            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", true, CreateMetrics(L"CPU/1.0/5/5")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(CreateGuid(i), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        int emptyNodesRemaining = 0;
        for (int i = 1; i < nodeCount; ++i)
        {
            if (CountIf(actionList, ActionMatch(wformatString(L"* move primary *=>{0}", i), value)) == 0)
            {
                ++emptyNodesRemaining;
            }
        }

        VERIFY_IS_TRUE(emptyNodesRemaining >= numberOfNodesRequested);
    }

    // Affinity test with one parent replicas and two child replicas that are not on the same node as parent
    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentWithMoreChildReplicaThanParentReplicaTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentWithMoreChildReplicaThanParentReplicaTest");
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100"));
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
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary 0=>2", value)), 1u);

        // Initial placement:
        // Nodes:   |    N0(M1:50)   |   N1(M1:100)   |   N2(M1:100)   |
        // Parent:  |       P        |                |                |
        // Child:   |                |       S        |       P        |
        // Other:   |                |                |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentWithNodeCapacityConstraintAndNoActionTakenTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentWithNodeCapacityConstraintAndNoActionTakenTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100"));
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

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService2",
            L"TestType2",
            true,
            L"TestService0",
            CreateMetrics(L"M1/1.0/50/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Initial placement:
        // Nodes:        |   N0(M1:100)  |   N1(M1:100)   |
        // Parent (P0):  |       P       |                |
        // Child  (C1):  |       P       |                |
        // Child  (C2):  |               |        P       |
        // Other:        |               |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentWithNodeCapacityConstraintTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentWithNodeCapacityConstraintTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/150"));
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

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService2",
            L"TestType2",
            true,
            L"TestService0",
            CreateMetrics(L"M1/1.0/50/30")));


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType3", true, CreateMetrics(L"M1/1.0/30/30")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType4"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService4",
            L"TestType4",
            true,
            L"TestService3",
            CreateMetrics(L"M1/1.0/30/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary 0=>2", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move primary 1=>2", value)), 1u);

        // Initial placement:
        // Nodes:        |   N0(M1:50)   |   N1(M1:150)   |   N2(M1:150)   |
        // Parent (P0):  |       P       |                |                |
        // Parent (P3):  |               |        P       |                |
        // Child  (C1):  |               |                |      P(P0)     |
        // Child  (C2):  |               |        P(P0)   |                |
        // Child  (C4):  |               |        P(P3)   |                |
        // Other:        |               |                |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentWithApplicationGroupsAndScaleoutTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentWithApplicationGroupsAndScaleoutTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/150"));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"Application1",
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"Application2",
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(
            wstring(L"TestService0"),
            wstring(L"TestType0"),
            wstring(L"Application1"),
            true,                                //bool isStateful,
            wstring(L""),                        //placementConstraints,
            wstring(L""),                        //affinitizedService,
            false,                               //alignedAffinity,
            vector<ServiceMetric>(CreateMetrics(L"M1/1.0/50/30")),
            false,
            false));                             // persisted

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(
            wstring(L"TestService1"),
            wstring(L"TestType1"),
            wstring(L"Application2"),
            true,                               //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(L"TestService0"),           //affinitizedService,
            true,                               //alignedAffinity,
            vector<ServiceMetric>(CreateMetrics(L"M1/1.0/50/30")),
            false,
            false));                            // persisted

        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType2", true, CreateMetrics(L"M1/1.0/50/30")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(
            wstring(L"TestService2"),
            wstring(L"TestType2"),
            wstring(L"Application1"),
            true,                               //bool isStateful,
            wstring(L""),                       //placementConstraints,
            wstring(L""),                       //affinitizedService,
            false,                              //alignedAffinity,
            vector<ServiceMetric>(CreateMetrics(L"M1/1.0/50/30")),
            false,
            false));                            // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move secondary 0=>2", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move primary 0=>2", value)), 1u);

        // Initial placement:
        // Nodes:   |   N0(M1:100)  |   N1(M1:150)   |   N2(M1:150)   |
        // Parent:  |       S       |        P       |                |
        // Child:   |               |        P       |        S       |
        // Other:   |       P       |        S       |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentWithMoreParentReplicaThanChildReplicaTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentWithMoreParentReplicaThanChildReplicaTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/140"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/150"));
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

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService2",
            L"TestType2",
            true,
            L"TestService0",
            CreateMetrics(L"M1/1.0/50/30")));


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType3"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType3", true, CreateMetrics(L"M1/1.0/30/30")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType4"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService4",
            L"TestType4",
            true,
            L"TestService3",
            CreateMetrics(L"M1/1.0/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary 0=>3", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"1 move primary 2=>3", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move primary 1=>3", value)), 1u);

        // Initial placement:
        // Nodes:       |   N0(M1:50)   |   N1(M1:150)   |   N2(M1:50)    |   N3(M1:150)   |
        // Parent (P0): |       P       |        S       |                |                |
        // Parent (P3): |               |        P       |                |                |
        // Child  (C1): |               |                |      P(P0)     |                |
        // Child  (C2): |               |        P(P0)   |                |                |
        // Child  (C4): |               |        P(P3)   |                |                |
        // Other:       |               |                |                |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityWithMovableParentAndBlockListAndPlacementConstraintTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckAffinityWithMovableParentAndBlockListAndPlacementConstraintTest");

        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, true);
        PLBConfigScopeChange(EnablePreferredSwapSolutionInConstraintCheck, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        fm_->RefreshPLB(Stopwatch::Now());

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/150"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(4, L"M1/150"));


        int nodeIndex = 0;
        map<wstring, wstring> nodeProperties;
        nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
        plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties)));
        for (nodeIndex = 1; nodeIndex < 3; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties1;
            nodeProperties1.insert(make_pair(L"NodeType", L"Other"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties1)));
        }
        for (; nodeIndex < 5; ++nodeIndex)
        {
            map<wstring, wstring> nodeProperties1;
            nodeProperties1.insert(make_pair(L"NodeType", L"Tenant"));
            plb.UpdateNode(CreateNodeDescription(nodeIndex, L"", L"", move(nodeProperties1)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType0"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType0", true, CreateMetrics(L"M1/1.0/50/30")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(
            wstring(L"TestService1"),
            wstring(L"TestType1"),
            wstring(L""),
            true,                               //bool isStateful,
            wstring(L"NodeType==Tenant"),       //placementConstraints,
            wstring(L"TestService0"),           //affinitizedService,
            false,                               //alignedAffinity,
            vector<ServiceMetric>(CreateMetrics(L"M1/1.0/50/30")),
            false,
            false));                            // persisted

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            L"TestService2",
            L"TestType2",
            true,
            L"TestService0",
            CreateMetrics(L"M1/1.0/50/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/3, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move primary 0=>3|4", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"0 move secondary 1=>3|4", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"1 move primary 2=>3|4", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"1 move secondary 1=>3|4", value)), 1u);
        VERIFY_ARE_EQUAL(CountIf(actionList, ActionMatch(L"2 move secondary 2=>3|4", value)), 1u);

        // Initial placement:
        // Nodes:  |   N0(M1:150)  |   N1(M1:150)   |   N2(M1:150)   |   N3(M1:150)   |   N4(M1:150)   |
        // Parent: |       P       |        S       |                |                |                |
        // Child:  |               |        S       |        P       |                |                |
        // Child:  |               |                |        S       |        P       |                |
        // Other:  |               |                |                |                |                |
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckNodeCapacitySwap)
    {
        wstring testName = L"ConstraintCheckNodeCapacitySwap";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBConstraintCheckTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(EnablePreferredSwapSolutionInConstraintCheck, bool, false);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wstring(L"MyMetric/10000000")));
        for (int i = 2; i < 50; ++i)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric/0")));
        }

        //we need to swap all the primary replicas of this service from node1
        plb.UpdateService(CreateServiceDescription(L"MyService1", L"MyType", true, CreateMetrics(wformatString("MyMetric/1.0/101/0"))));
        for (int i = 0; i < 300; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"MyService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we cannot find a solution to all violations
        VERIFY_ARE_NOT_EQUAL(300u, actionList.size());

        PLBConfig::GetConfig().EnablePreferredSwapSolutionInConstraintCheck = true;

        fm_->Clear();
        for (int i = 0; i < 300; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(L"MyService1"), 1, CreateReplicas(L"P/0,S/1"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        actionList = GetActionListString(fm_->MoveActions);
        //we can fix all violations
        VERIFY_ARE_EQUAL(300u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckSwapCost)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "ConstraintCheckSwapCost");

        //swaps are a LOOOT better than moves
        PLBConfigScopeChange(MoveCostLowValue, int, 100000000);
        PLBConfigScopeChange(SwapCost, double, 1);
        PLBConfigScopeChange(UseMoveCostReports, bool, true);
        PLBConfigScopeChange(PreferredLocationConstraintPriority, int, -1);

        PlacementAndLoadBalancing & plb = fm_->PLB;


        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, wstring(L"MyMetric/100")));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, wstring(L"MyMetric/100000")));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"MyService1", L"MyType", true, CreateMetrics(wformatString("MyMetric/1.0/50/20")), FABRIC_MOVE_COST_LOW));
        plb.UpdateService(CreateServiceDescription(L"MyService2", L"MyType", true, CreateMetrics(wformatString("MyMetric/1.0/60/0")), FABRIC_MOVE_COST_LOW));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"MyService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"MyService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"MyService2"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we should make 2 swaps here because that actually makes the solution better energy
        VERIFY_ARE_EQUAL(2u, fm_->MoveActions.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(QuorumLogicSwitchOnOffTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "QuorumLogicSwitchOnOffTest");

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 100);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> nodeProperties1, nodeProperties2;;

        nodeProperties1.insert(make_pair(L"NodeType", L"red"));
        nodeProperties2.insert(make_pair(L"NodeType", L"blue"));

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/r0/c0", L"ud0", move(nodeProperties1)));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/r0/c1", L"ud1", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/r0/c2", L"ud2", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/r0/c3", L"ud3", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(4, L"dc0/r0/c4", L"ud4", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(5, L"dc0/r0/c0", L"ud1", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(6, L"dc0/r0/c1", L"ud2", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(7, L"dc0/r0/c2", L"ud3", move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescription(8, L"dc0/r0/c3", L"ud4", move(nodeProperties2)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", false, L"", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), L"TestService", 0, CreateReplicas(L"S/1, S/2, S/3, S/4, S/8"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        // Due to the number of domains and target replica count, quorum based logic is used and there is no constraint violation
        VerifyPLBAction(plb, L"NoActionNeeded");
        std::vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", false, L"NodeType==blue", CreateMetrics(L""), false, 5));

        fm_->RefreshPLB(Stopwatch::Now());

        // Due to the placement constaint, number of domains is reduced so +1 logic is used now so there is constraint violation
        VerifyPLBAction(plb, L"ConstraintCheck");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move instance 8=>5", value)));

        fm_->ClearMoveActions();

        plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", false, L"", CreateMetrics(L""), false, 5));

        fm_->RefreshPLB(Stopwatch::Now());

        // Domains are like they were at the begining, quorum based logic is turned on again so there is no violation
        VerifyPLBAction(plb, L"NoActionNeeded");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(StatelessServicesPlacementAndScaleDownTest)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "StatelessServicesPlacementAndScaleDownTest");

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1); //number of rounds

        const int partitionCount = 1;

        Common::Random random;

        int fdCount = random.Next(2, 6);
        int udCount = fdCount == 2 ? random.Next(3, 6) : random.Next(2, 6);
        int nodeCount = random.Next(6, fdCount * udCount + 1);
        int instanceCount = random.Next(5, nodeCount);

        fm_->Load(random.Next());
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int fuVersion = 0;
        std::vector<std::vector<bool>> placed(fdCount, vector<bool>(udCount, false));

        std::map<int, std::pair<int, int>> mapFdUd;
        std::vector<int> sumFd(fdCount, 0);
        std::vector<int> sumUd(udCount, 0);

        // Node placement
        for (int nodeId = 0, i = 0, j = 0; nodeId < nodeCount; ++nodeId)
        {
            while (placed[i % fdCount][j % udCount])
            {
                j++;
            }

            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(nodeId, wformatString("{0}", i % fdCount), wformatString("{0}", j % udCount), L"MyMetric/100"));

            mapFdUd[nodeId] = std::make_pair(i % fdCount, j % udCount);
            sumFd[i % fdCount]++;
            sumUd[j % udCount]++;

            placed[i++ % fdCount][j++ % udCount] = true;

        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        wstring serviceName = wstring(L"PartitionedService");
        plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false, CreateMetrics(wformatString("MyMetric/1.0/50/50")), 1, false, instanceCount));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(L""), instanceCount));

        // Replica placement
        plb.Refresh(Stopwatch::Now());

        std::vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        int countAgain = 1;
        while (actionList.size() != instanceCount && countAgain <= 5)
        {
            PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
            fm_->ClearMoveActions();
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(L""), instanceCount));
            plb.Refresh(Stopwatch::Now());
            actionList = GetActionListString(fm_->MoveActions);
        }

        VERIFY_ARE_EQUAL(instanceCount, CountIf(actionList, ActionMatch(L"0 add * *", value)));

        fm_->ClearMoveActions();

        wstring distribution = L"";
        std::vector<wstring> usedNodes;

        for (int i = 0; i < instanceCount; ++i)
        {
            std::vector<wstring> splited;
            StringUtility::Split<wstring>(actionList[i], splited, L" ");
            distribution += wstring(L"S/");
            distribution += splited[3];
            distribution += wstring(L",");

            usedNodes.push_back(splited[3]);
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(distribution)), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->ClearMoveActions();

        int limitFd = (int)nodeCount / fdCount + (nodeCount % fdCount != 0 ? 1 : 0);
        int limitUd = (int)nodeCount / udCount + (nodeCount % udCount != 0 ? 1 : 0);

        // Do the scale down for every node (that has replica) by deactivating
        for (int i = 0; i < instanceCount; ++i)
        {
            PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

            int currentNode;

            StringUtility::TryFromWString<int>(usedNodes[i], currentNode);

            if (sumFd[mapFdUd[currentNode].first] < limitFd || sumUd[mapFdUd[currentNode].second] < limitUd)
            {
                continue;
            }

            PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
            PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);

            Trace.WriteInfo("PLBStatelessServicesPlacementAndScaleDownTestSource",
                "No LoadBalancing actions: FD count: {0}, UD count: {1}, Node count: {2}, Instance count: {3}, Distribution: {4}, Node: {5}",
                fdCount, udCount, nodeCount, instanceCount, distribution, currentNode);

            wstring newDistribution(distribution);
            wstring oldInstance = L"S/" + StringUtility::ToWString(currentNode) + L",";
            wstring newInstance = L"S/" + StringUtility::ToWString(currentNode) + L"/T,";
            StringUtility::Replace(newDistribution, oldInstance, newInstance);

            // Deactivate node
            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(currentNode,
                wformatString("{0}", mapFdUd[currentNode].first), wformatString("{0}", mapFdUd[currentNode].second),
                L"MyMetric/100", Reliability::NodeDeactivationIntent::RemoveNode, Reliability::NodeDeactivationStatus::DeactivationSafetyCheckInProgress));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(newDistribution)), 1));
            plb.Refresh(Stopwatch::Now());

            actionList = GetActionListString(fm_->MoveActions);

            countAgain = 1;
            while (actionList.size() == 0 && countAgain <= 5)
            {
                PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
                plb.Refresh(Stopwatch::Now());
                actionList = GetActionListString(fm_->MoveActions);

                // If NewReplicaPlacement is not good enough, try NewReplicaPlacementWithMove
                if (actionList.size() == 0)
                {
                    plb.Refresh(Stopwatch::Now());
                    actionList = GetActionListString(fm_->MoveActions);
                }
            }

            VERIFY_ARE_NOT_EQUAL(0, actionList.size());

            fm_->ClearMoveActions();

            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(currentNode,
                wformatString("{0}", mapFdUd[currentNode].first), wformatString("{0}", mapFdUd[currentNode].second),
                L"MyMetric/100", Reliability::NodeDeactivationIntent::None, Reliability::NodeDeactivationStatus::None));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(distribution)), 0));
            plb.ProcessPendingUpdatesPeriodicTask();
        }
    }

    BOOST_AUTO_TEST_CASE(PartialPlacementConstraintFixTest)
    {
        // Placement constraint violation on node 0, 2 replicas need to move.
        // Node capacity allows the move of only 1 replica.
        // We should be able to make the partial fix.
        wstring testName = L"PartialPlacementConstraintFixTest";
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", testName);
 
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
 
        PlacementAndLoadBalancing & plb = fm_->PLB;
        std::map<wstring, wstring> node1Properties;
        node1Properties.insert(make_pair(L"Property", L"NotOK"));
        std::map<wstring, wstring> node2Properties;
        node2Properties.insert(make_pair(L"Property", L"OK"));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(0, L"CPU/200", move(node1Properties)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(1, L"CPU/100", move(node2Properties)));
 
        plb.ProcessPendingUpdatesPeriodicTask();
 
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
 
        wstring service1Name = wformatString("{0}_Service1", testName);
        wstring service2Name = wformatString("{0}_Service2", testName);
 
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"TestType"),
            wstring(L""),
            true,
            wstring(L"Property==OK"),
            wstring(L""),
            true,
            move(CreateMetrics(L"CPU/1.0/100/100")),
            FABRIC_MOVE_COST_LOW,
            false));
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"TestType"),
            wstring(L""),
            true,
            wstring(L"Property==OK"),
            wstring(L""),
            true,
            move(CreateMetrics(L"CPU/1.0/100/100")),
            FABRIC_MOVE_COST_LOW,
            false));
 
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));
 
        // Add one more node to force full closure. Irrelevant for this test because it has no capacity.
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"CPU/0"));
 
        plb.Refresh(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    // This test is the simulation that systematically checks if scale down can be performed.
    // Simulation shows that auto switching from '+1' to quorum based logic also helps scaling down
    // It's not 'classic' boost test so shouldn't be used on regular basis. 
    // Be aware that it can execute for hours in debug mode, so run it in retail mode (~10min with current constants) if necessary.
    /*
    BOOST_AUTO_TEST_CASE(StatelessServicesPlacementAndScaleDownSimulation)
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "StatelessServicesPlacementAndScaleDownSimulation");

        PLBConfigScopeChange(FaultDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(UpgradeDomainConstraintPriority, int, 0);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 1); //number of rounds

        const int fdCount = 5;
        const int udCount = 5;
        const int partitionCount = 1;
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

                            int fuVersion = 0;
                            std::vector<std::vector<bool>> placed(fd, vector<bool>(ud, false));

                            std::map<int, std::pair<int, int>> mapFdUd;
                            std::vector<int> sumFd(fd, 0);
                            std::vector<int> sumUd(ud, 0);

                            // Node placement
                            for (int nodeId = 0, i = 0, j = 0; nodeId < nodeCount; ++nodeId)
                            {
                                while (placed[i % fd][j % ud])
                                {
                                    j++;
                                }

                                plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(nodeId, wformatString("{0}", i % fd), wformatString("{0}", j % ud), L"MyMetric/100"));

                                mapFdUd[nodeId] = std::make_pair(i % fd, j % ud);
                                sumFd[i % fd]++;
                                sumUd[j % ud]++;

                                placed[i++ % fd][j++ % ud] = true;

                            }

                            // Force processing of pending updates so that service can be created.
                            plb.ProcessPendingUpdatesPeriodicTask();

                            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

                            wstring serviceName = wstring(L"PartitionedService");
                            plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false, CreateMetrics(wformatString("MyMetric/1.0/50/50")), 1, false, instance));

                            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(L""), instance));

                            // Replica placement
                            plb.Refresh(Stopwatch::Now());

                            std::vector<wstring> actionList = GetActionListString(fm_->MoveActions);

                            int countAgain = 1;
                            while (actionList.size() != instance && countAgain <= 5)
                            {
                                PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
                                fm_->ClearMoveActions();
                                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(L""), instance));
                                plb.Refresh(Stopwatch::Now());
                                actionList = GetActionListString(fm_->MoveActions);
                            }

                            VERIFY_ARE_EQUAL(instance, CountIf(actionList, ActionMatch(L"0 add * *", value)));

                            fm_->ClearMoveActions();

                            wstring distribution = L"";
                            std::vector<wstring> usedNodes;

                            for (int i = 0; i < instance; ++i)
                            {
                                std::vector<wstring> splited;
                                StringUtility::Split<wstring>(actionList[i], splited, L" ");
                                distribution += wstring(L"S/");
                                distribution += splited[3];
                                distribution += wstring(L",");
                                usedNodes.push_back(splited[3]);
                            }

                            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(distribution)), 0));

                            plb.ProcessPendingUpdatesPeriodicTask();

                            fm_->ClearMoveActions();

                            int limitFd = (int)nodeCount / fd + (nodeCount % fd != 0 ? 1 : 0);
                            int limitUd = (int)nodeCount / ud + (nodeCount % ud != 0 ? 1 : 0);

                            // Do the scale down for every node (that has replica) by deactivating
                            for (int i = 0; i < instance; ++i)
                            {
                                PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

                                int currentNode;

                                StringUtility::TryFromWString<int>(usedNodes[i], currentNode);

                                if (sumFd[mapFdUd[currentNode].first] < limitFd || sumUd[mapFdUd[currentNode].second] < limitUd)
                                {
                                    continue;
                                }

                                PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
                                PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);

                                wstring newDistribution(distribution);
                                wstring oldInstance = L"S/" + StringUtility::ToWString(currentNode) + L",";
                                wstring newInstance = L"S/" + StringUtility::ToWString(currentNode) + L"/T,";
                                StringUtility::Replace(newDistribution, oldInstance, newInstance);

                                // Deactivate node
                                plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(currentNode,
                                    wformatString("{0}", mapFdUd[currentNode].first), wformatString("{0}", mapFdUd[currentNode].second),
                                    L"MyMetric/100", Reliability::NodeDeactivationIntent::RemoveNode, Reliability::NodeDeactivationStatus::DeactivationSafetyCheckInProgress));
                                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(newDistribution)), 1));
                                plb.Refresh(Stopwatch::Now());

                                actionList = GetActionListString(fm_->MoveActions);

                                countAgain = 1;
                                while (actionList.size() == 0 && countAgain <= 5)
                                {
                                    PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 500 * countAgain++);
                                    plb.Refresh(Stopwatch::Now());
                                    actionList = GetActionListString(fm_->MoveActions);

                                    // If NewReplicaPlacement is not good enough, try NewReplicaPlacementWithMove
                                    if (actionList.size() == 0)
                                    {
                                        plb.Refresh(Stopwatch::Now());
                                        actionList = GetActionListString(fm_->MoveActions);
                                    }
                                }

                                // VERIFY_ARE_NOT_EQUAL(0, actionList.size());

                                if (actionList.size() == 0)
                                {
                                    printf("No actions\n");
                                    printf("FD count: %d\n", fd);
                                    printf("UD count: %d\n", ud);
                                    printf("Node count: %d\n", nodeCount);
                                    printf("Instance count: %d\n", instance);
                                    printf("Current node: %d\n", currentNode);
                                    printf("------------------------------\n");

                                    Trace.WriteInfo("PLBStatelessServicesPlacementAndScaleDownSimulationSource",
                                        "No LoadBalancing actions: FD count: {0}, UD count: {1}, Node count: {2}, Instance count: {3}, Distribution: {4}, Node: {5}",
                                        fd, ud, nodeCount, instance, distribution, currentNode);
                                }

                                fm_->ClearMoveActions();

                                plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(currentNode,
                                    wformatString("{0}", mapFdUd[currentNode].first), wformatString("{0}", mapFdUd[currentNode].second),
                                    L"MyMetric/100", Reliability::NodeDeactivationIntent::None, Reliability::NodeDeactivationStatus::None));
                                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), fuVersion++, CreateReplicas(wstring(distribution)), 0));
                                plb.ProcessPendingUpdatesPeriodicTask();

                            }
                        }
                    }
                }
            }
        }
    }
    */

    BOOST_AUTO_TEST_CASE(DiagnosticsConstraintCheckScaleoutCountWithStandByReplicas)
    {
        wstring testName = L"DiagnosticsConstraintCheckScaleoutCountWithStandByReplicas";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Decrease ConstraintViolationHealthReportLimit and set test mode so that we can test diagnostics
        PLBConfigScopeChange(ConstraintViolationHealthReportLimit, int, 1);
        PLBConfigScopeChange(IsTestMode, bool, true);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int appScaleoutCount1 = 1;
        wstring appName1 = wformatString("{0}Application1", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount1));

        wstring service1 = wformatString("{0}_Service{1}", testName, L"1");
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            service1,
            testType,
            appName1,
            true,
            CreateMetrics(L"MyMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1), 0, CreateReplicas(L"SB/0, P/1"), 0));

        // Check that we won't assert
        fm_->RefreshPLB(Stopwatch::Now());

        // Check that we couldn't move anything
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(DiagnosticsConstraintCheckScaleoutCountAndAffinityWithStandByReplicas)
    {
        wstring testName = L"DiagnosticsConstraintCheckScaleoutCountAndAffinityWithStandByReplicas";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Decrease ConstraintViolationHealthReportLimit and set test mode so that we can test diagnostics
        PLBConfigScopeChange(ConstraintViolationHealthReportLimit, int, 1);
        PLBConfigScopeChange(IsTestMode, bool, true);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int appScaleoutCount1 = 1;
        wstring appName1 = wformatString("{0}Application1", testName);
        plb.UpdateApplication(ApplicationDescription(wstring(appName1),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            appScaleoutCount1));

        wstring appName2 = wformatString("{0}Application2", testName);
        plb.UpdateApplication(ApplicationDescription(
            wstring(appName2),
            std::map<std::wstring,
            ApplicationCapacitiesDescription>(),
            0));

        wstring parentService = wformatString("{0}_ParentService{1}", testName, L"1");
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            parentService,
            testType,
            appName2,
            true,
            CreateMetrics(L"MyMetric/1.0/1/1")));

        wstring childService = wformatString("{0}_ChildService{1}", testName, L"1");
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(
            childService,
            testType,
            true,
            appName1,
            parentService,
            CreateMetrics(L"MyMetric/1.0/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(childService), 0, CreateReplicas(L"SB/0,P/1"), 0));

        // Check that we won't assert
        fm_->RefreshPLB(Stopwatch::Now());

        // Check that we couldn't move anything
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPreferExistingReplicaLocationsTest)
    {
        wstring testName = L"ConstraintCheckPreferExistingReplicaLocationsTest";
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreferExistingReplicaLocations, bool, false);

        std::wstring metric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, wformatString("{0}/10", metric)));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("service_{0}", testName);

        // Node0 has capacity violation. Swap can fix the violation, but move primary of FT0 to node 2 make load more balanced
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/6/4", metric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move primary 0<=>2", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearLoadTest)
    {
        ConstraintCheckWithShouldDisappearLoadTestHelper(L"ConstraintCheckWithShouldDisappearLoadTest", true);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearLoadTestFeatureSwitchOff)
    {
        ConstraintCheckWithShouldDisappearLoadTestHelper(L"ConstraintCheckWithShouldDisappearLoadTestFeatureSwitchOff", false);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearLoadAndReservationTest)
    {
        ConstraintCheckWithShouldDisappearLoadAndReservationTestHelper(L"ConstraintCheckWithShouldDisappearLoadTest", true);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithShouldDisappearLoadAndReservationTestFeatureSwitchOff)
    {
        ConstraintCheckWithShouldDisappearLoadAndReservationTestHelper(L"ConstraintCheckWithShouldDisappearLoadAndReservationTestFeatureSwitchOff", false);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBConstraintCheck::ConstraintCheckWithShouldDisappearLoadTestHelper(wstring const& testName, bool featureSwitch)
    {
        // Node 0 has total load of 100, capacity of 50 - 50 needs to be moved.
        // Node 1 has disappearing load of 100, load of 0 and capacity of 100.
        // Feature switch on: No moves should be made into node 1, since disappearing load prevents it.
        // Feature switch off: Moves should be made into node 1, since disappearing load prevents it.
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        PLBConfigScopeChange(CountDisappearingLoadForSimulatedAnnealing, bool, featureSwitch);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
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
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    void TestPLBConstraintCheck::ConstraintCheckWithShouldDisappearLoadAndReservationTestHelper(wstring const& testName, bool featureSwitch)
    {
        // Node 0 has total load of 0 and reservation of 100, capacity of 50 - 50 reservation needs to be moved.
        // Node 1 has disappearing load of 100, load of 0 and capacity of 100.
        // In case when feature switch is on: No moves should be made into node 1, since disappearing load prevents it.
        // In case when feature switch is off: Moves should be made into node 1, since disappearing load prevents it.
        Trace.WriteInfo("PLBConstraintCheckTestSource", "{0}", testName);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PLBConfigScopeChange(CountDisappearingLoadForSimulatedAnnealing, bool, featureSwitch);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"CPU/50"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"CPU/100"));

        plb.ProcessPendingUpdatesPeriodicTask();

        // This service is used to add disappearing load to node 1.
        wstring serviceName = wformatString("{0}_Service", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            serviceName,
            L"TestType",
            wstring(L""),
            true,
            CreateMetrics(L"CPU/1.0/50/50")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/1/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1/D"), 0));

        // Two applications with reservation of 50 each to create node capacity violation on node 0.
        wstring app1Name = wformatString("{0}_App1", testName);
        wstring service1Name = wformatString("{0}_App1_Service", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            app1Name,
            1,
            2,
            CreateApplicationCapacities(wformatString(L"CPU/100/50/50"))));
        // This one reports the metric!
        plb.UpdateService(CreateServiceDescriptionWithApplication(service1Name, L"TestType", app1Name, true, CreateMetrics(L"CPU/1/0/0")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));

        wstring app2Name = wformatString("{0}_App2", testName);
        wstring service2Name = wformatString("{0}_App2_Service", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            app2Name,
            1,
            2,
            CreateApplicationCapacities(wformatString(L"CPU/100/50/50"))));
        // This one reports the metric!
        plb.UpdateService(CreateServiceDescriptionWithApplication(service2Name, L"TestType", app2Name, true, CreateMetrics(L"CPU/1/0/0")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));

        plb.Refresh(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        size_t expectedMoves = featureSwitch ? 0 : 1;
        VERIFY_ARE_EQUAL(expectedMoves, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    void TestPLBConstraintCheck::ConstraintCheckWithQuorumBasedDomainNoViolationsHelper(PlacementAndLoadBalancing & plb)
    {
        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // stateFull services - target 5
        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/4,S/5, S/7"), 0));

        // stateFull services - target 3 non-packing
        serviceName = L"StatefulNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0, S/4, S/7"), 0));

        // stateless services - target 3
        serviceName = L"StatelessPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4, I/7"), 0));

        // stateless services - target 4 non-packing
        serviceName = L"StatelessNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, false, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4, I/7, I/9"), 0));

        // stateless services - target -1
        serviceName = L"StatelessOnEveryNodeService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), true, INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1,I/2,I/3, I/4,I/5,I/6, I/7,I/8, I/9"), INT_MAX));

        // stateFull - target 1
        serviceName = L"Stateful1";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        // stateLess - target 1
        serviceName = L"Stateless1";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(serviceName), 0, CreateReplicas(L"I/0"), 0));

        // stateFull - target 2
        serviceName = L"Stateful2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(serviceName), 0, CreateReplicas(L"P/0,S/4"), 0));

        // stateLess - target 2
        serviceName = L"Stateless2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(serviceName), 0, CreateReplicas(L"I/0,I/4"), 0));

        // stateFull - target 4
        serviceName = L"Stateful4";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(serviceName), 0, CreateReplicas(L"P/0,S/4,S/7,S/9"), 0));

        // stateLess - target 4
        serviceName = L"Stateless4";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(serviceName), 0, CreateReplicas(L"I/0,I/4,I/7,I/9"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    void TestPLBConstraintCheck::ConstraintCheckWithQuorumBasedDomainWithViolationsHelper(PlacementAndLoadBalancing & plb)
    {
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 0.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 0.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        wstring serviceType = L"TestType";
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        Reliability::FailoverUnitFlags::Flags noneFlag;

        // stateFull services - target 5
        wstring serviceName = L"StatefulPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 5));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2, S/4, S/7"), 0));

        // stateFull services - target 3 non-packing
        serviceName = L"StatefulNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, true, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1, S/4"), 0));

        // stateless services - target 3
        serviceName = L"StatelessPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4,I/5"), 0));

        // stateless services - target 4 non-packing
        serviceName = L"StatelessNonPackingService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            serviceName, serviceType, false, L"FDPolicy ~ Nonpacking", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"I/0, I/4, I/7,I/8"), 0));

        // stateless services - target -1
        serviceName = L"StatelessOnEveryNodeService";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), true, INT_MAX));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1,I/2,I/3, I/4,I/5,I/6, I/7,I/8, I/9"), INT_MAX));

        // stateFull - target 2
        serviceName = L"Stateful2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        // stateLess - target 2
        serviceName = L"Stateless2";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1"), 0));

        // stateFull - target 4
        serviceName = L"Stateful4";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, true, L"", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/4,S/7"), 0));

        // stateLess - target 4
        serviceName = L"Stateless4";
        plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, serviceType, false, L"", CreateMetrics(L""), false, 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(serviceName), 0, CreateReplicas(L"I/0,I/1,I/4,I/7"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(8u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move * 0|1|2 => *", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move * 0|1 => 7|8|9", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move instance 4|5 => 7|8|9", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"3 move instance 7|8 => 9", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"5 move * 0|1 => 4|5|6|7|8|9", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"6 move instance 0|1 => 4|5|6|7|8|9", value)));

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"7 move * 0|1 => 9", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"8 move instance 0|1 => 9", value)));
    }

    bool TestPLBConstraintCheck::ClassSetup()
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBConstraintCheck::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBConstraintCheck::ClassCleanup()
    {
        Trace.WriteInfo("PLBConstraintCheckTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
