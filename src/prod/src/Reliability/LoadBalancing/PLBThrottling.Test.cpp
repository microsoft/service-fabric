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


    class TestPLBThrottling
    {
    protected:
        TestPLBThrottling() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBThrottling()
        {
            BOOST_REQUIRE(ClassCleanup());
            Trace.WriteInfo("PLBThrottlingTestSource", "Destructor called.");
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;

        void SimplePerNodeThrottlingTestHelper(int inBuildCount, int expectedMoveCount);

        void PerNodeThrottlingTestHelper(int phase,
            int mode,
            int inBuildCount,
            int expectedMoveCount,
            bool throttlePlacement = true,
            bool throttleConstraintCheck = true,
            bool throttleBalancing = true,
            int throttlingConstraintPriority = 0);

        void PlacementTestHelper(bool primaryBuild);

        void DomainsTestHelper(bool isFD);
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBThrottlingSuite, TestPLBThrottling)

    BOOST_AUTO_TEST_CASE(CheckInBuildCountsBasicTest)
    {
        // Checks if IB counts per node are correct when adding/updating/deleting replicas.
        Trace.WriteInfo("PLBThrottlingTestSource", "CheckInBuildCountsBasicTest");

        PlacementAndLoadBalancingTestHelper & plb = fm_->PLBTestHelper;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        // Add a few partitions with IB replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B,S/3/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(3, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(3));

        // Delete one partition
        plb.DeleteFailoverUnit(wstring(L"TestService"), CreateGuid(2));

        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3));

        // Update one partition, remove one IB replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 1, CreateReplicas(L"P/0,S/1,S/2/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3));

        // Add back one IB replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 2, CreateReplicas(L"P/0,S/1/B,S/2/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3));

        // Remove all IB replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 3, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3));

        // Add a few partitions where primary is getting build (should not influence IB count)
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(10), wstring(L"TestService"), 3, CreateReplicas(L"P/0/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(11), wstring(L"TestService"), 3, CreateReplicas(L"P/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(12), wstring(L"TestService"), 3, CreateReplicas(L"P/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(13), wstring(L"TestService"), 3, CreateReplicas(L"P/3/B"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3));
    }

    BOOST_AUTO_TEST_CASE(CheckInBuildCountsWithDomainMergeAndSplit)
    {
        // Checks if IB counts per node are correct when merging and splitting domains.
        Trace.WriteInfo("PLBThrottlingTestSource", "CheckInBuildCountsWithDomainMergeAndSplit");

        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        PlacementAndLoadBalancingTestHelper & plb = fm_->PLBTestHelper;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Add a few partitions with IB replicas to first service domain
        plb.UpdateService(CreateServiceDescription(L"BlueTestService", L"TestType", true, CreateMetrics(L"BlueMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"BlueTestService"), 0, CreateReplicas(L"P/0,S/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"BlueTestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"BlueTestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B,S/3/B"), 0));

        // Add a few partitions with IB replicas to second service domain
        plb.UpdateService(CreateServiceDescription(L"GreenTestService", L"TestType", true, CreateMetrics(L"GreenMetric/1.0/1/1")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"GreenTestService"), 0, CreateReplicas(L"P/3,S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"GreenTestService"), 0, CreateReplicas(L"P/3,S/2/B,S/1/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"GreenTestService"), 0, CreateReplicas(L"P/3,S/2/B,S/1/B,S/0/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0, L"BlueMetric"));
        VERIFY_ARE_EQUAL(3, plb.GetInBuildCountPerNode(1, L"BlueMetric"));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(2, L"BlueMetric"));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(3, L"BlueMetric"));

        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(0, L"GreenMetric"));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(1, L"GreenMetric"));
        VERIFY_ARE_EQUAL(3, plb.GetInBuildCountPerNode(2, L"GreenMetric"));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3, L"GreenMetric"));

        // Merge the domains (takes effect immediately)
        plb.UpdateService(CreateServiceDescription(L"GreenBlueTestService", L"TestType", true, CreateMetrics(L"BlueMetric/1.0/1/1,GreenMetric/1.0/1/1")));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(0));
        VERIFY_ARE_EQUAL(5, plb.GetInBuildCountPerNode(1));
        VERIFY_ARE_EQUAL(5, plb.GetInBuildCountPerNode(2));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(3));

        // Split the domains again (takes effect immediately)
        plb.DeleteService(L"GreenBlueTestService");
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(0, L"BlueMetric"));
        VERIFY_ARE_EQUAL(3, plb.GetInBuildCountPerNode(1, L"BlueMetric"));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(2, L"BlueMetric"));
        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(3, L"BlueMetric"));

        VERIFY_ARE_EQUAL(1, plb.GetInBuildCountPerNode(0, L"GreenMetric"));
        VERIFY_ARE_EQUAL(2, plb.GetInBuildCountPerNode(1, L"GreenMetric"));
        VERIFY_ARE_EQUAL(3, plb.GetInBuildCountPerNode(2, L"GreenMetric"));
        VERIFY_ARE_EQUAL(0, plb.GetInBuildCountPerNode(3, L"GreenMetric"));
    }

    BOOST_AUTO_TEST_CASE(PerNodeThrottlesPhasesTest)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "PerNodeThrottlesPhasesTest");
        // 0 - placement (helper will create 20 partitions with primary on node 0 and with 1 new replica each).
        // 1 - constraint check (helper will put 20 replicas on node 0, node capacity is 10).
        // 2 - balancing (helper will put 20 replicas on node 0, nothing on node 1, no node capacity)
        for (int phase = 0; phase < 3; ++phase)
        {
            // Helper will always limit the number of in-build replicas per node to 2.
            // mode: 0 - global throttling limit is smaller than per-phase limit.
            // mode: 1 - per-phase limit is smaller than global limit
            for (int mode = 0; mode < 2; ++mode)
            {
                // we will vary number of in-build replicas per node from 1 (below limit) to 3 (above limit)
                for (int ibCount = 1; ibCount < 4; ++ibCount)
                {
                    // we can add/move only if there is 1 slot.
                    int expectedMoves = ibCount == 1 ? 1 : 0;
                    fm_->Clear();
                    fm_->Load();
                    Trace.WriteInfo("PLBThrottlingTestSource",
                        "CheckInBuildCountsWithDomainMergeAndSplit: phase={0} mode={1} ibCount={2} expectedMoves={3}",
                        phase,
                        mode,
                        ibCount,
                        expectedMoves);
                    PerNodeThrottlingTestHelper(phase, mode, ibCount, expectedMoves);
                }
            }
        }
        
    }

    BOOST_AUTO_TEST_CASE(PlacementPerNodeThrottlingKillSwitch)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementPerNodeThrottlingKillSwitch");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // 20 new replicas need to be placed, only possible target is node 1 (primaries on node 0)
        // All 20 adds should be executed because config is false to throttle placement.
        PerNodeThrottlingTestHelper(0, 1, 4, 20, false, true, true);
    }

    BOOST_AUTO_TEST_CASE(PlacementPerNodeThrottlingWithLowPriority)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementPerNodeThrottlingWithLowPriority");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // 20 new replicas need to be placed, only possible target is node 1 (primaries on node 0)
        // Throttling constraint priority is 1 (soft), and all replicas should be placed.
        PerNodeThrottlingTestHelper(0, 1, 4, 20, true, true, true, 1);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPerNodeThrottlingKillSwitch)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "ConstraintCheckPerNodeThrottlingKillSwitch");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // Node 0 is 10 over capacity, only possible target is node 1
        // 10 moves should be executed because config is false to throttle constraint check.
        PerNodeThrottlingTestHelper(1, 1, 4, 10, true, false, true);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckPerNodeThrottlingWithLowPriority)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "ConstraintCheckPerNodeThrottlingWithLowPriority");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // Node 0 is 10 over capacity, only possible target is node 1
        // Throttling is enabled, but throttling constraint priority is 1 (soft)
        // 10 moves should be executed because node capacity has higher priority than throttling.
        PerNodeThrottlingTestHelper(1, 1, 4, 10, true, true, true, 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingPerNodeThrottlingKillSwitch)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "ConstraintCheckPerNodeThrottlingKillSwitch");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // Node 0 has 20 replicas (load 20), and node 1 has load 0
        // 10 moves should be executed because config is false to throttle balancing.
        PerNodeThrottlingTestHelper(2, 1, 4, 10, true, true, false);
    }

    BOOST_AUTO_TEST_CASE(BalancingPerNodeThrottlingWithLowPriority)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingPerNodeThrottlingWithLowPriority");
        // Node 1 has 4 IB replicas, throttling limit is 2.
        // Node 0 has 20 replicas (load 20), and node 1 has load 0
        // 10 moves should be executed because throttling constraint priority is 2 (optimization)
        PerNodeThrottlingTestHelper(2, 1, 4, 10, true, true, true, 2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithPerNodeThrottlingBasic)
    {
        // 2 nodes, 1 with 20 replicas, 1 empty. Throttling at 2 IB replicas per node, only 2 can be moved.
        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingWithPerNodeThrottlingBasic");

        SimplePerNodeThrottlingTestHelper(0, 2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithPerNodeThrottlingWithIBReplicasWithinQuota)
    {
        // 2 nodes, 1 with 21 replicas, 1 with 1 IB replicas.
        // Throttling is set to 2 IB replicas per node, and node 1 already has 1 IB, so only one can be moved.
        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingWithPerNodeThrottlingWithIBReplicasWithinQuota");

        SimplePerNodeThrottlingTestHelper(1, 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithPerNodeThrottlingWithIBReplicasOnQuota)
    {
        // 2 nodes, 1 with 22 replicas, 1 with 2 IB replicas.
        // Throttling is set to 2 IB replicas per node, and node 1 already has 2 IB, so nothing can be moved.
        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingWithPerNodeThrottlingWithIBReplicasWithinQuota");

        SimplePerNodeThrottlingTestHelper(2, 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithPerNodeThrottlingWithIBReplicasOverQuota)
    {
        // 2 nodes, 1 with 23 replicas, 1 with 3 IB replicas.
        // Throttling is set to 2 IB replicas per node, and node 1 already has 3, so nothing can be moved.
        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingWithPerNodeThrottlingWithIBReplicasOverQuota");

        SimplePerNodeThrottlingTestHelper(3, 0);
    }

    BOOST_AUTO_TEST_CASE(ThrottlingConstraintShouldNotReportViolation)
    {
        // One node with 3 IB replicas, limit is 2. Load is zero on all nodes, no new replicas.
        // We should not report violation or start the CC run.

        Trace.WriteInfo("PLBThrottlingTestSource", "ThrottlingConstraintShouldNotReportViolation");

        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);
        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"StatefulService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/0/0")));

        int ftIndex = 0;
        for (; ftIndex < 4; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0, S/1/B"), 0));
        }

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);

        VerifyPLBAction(plb, L"NoActionNeeded");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithMixedReplicasStatelessNotThrottledPerNode)
    {
        // Mix of stateless and stateful replica on node 0. Stateful can't be moved due to throttling.
        // Stateless can and should be moved to achieve perfect balance.

        Trace.WriteInfo("PLBThrottlingTestSource", "BalancingWithMixedReplicasStatelessNotThrottledPerNode");

        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"StatefulService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/0")));
        plb.UpdateService(CreateServiceDescription(L"StatelessService", L"TestType", false, CreateMetrics(L"ArenaMetric/1.0/1/1")));

        int ftIndex = 0;
        for (; ftIndex < 4; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex), wstring(L"StatefulService"), 0, CreateReplicas(L"P/0, S/1/B"), 0));
        }

        for (; ftIndex < 8; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex), wstring(L"StatelessService"), 0, CreateReplicas(L"I/0"), 0));
        }

        // Node 0 has load of 8, node 1 load of 0. There are 4 IB replicas on node 0.
        // 4 stateless instances should be moved to node 1.
        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(4u, CountIf(actionList, ActionMatch(L"4|5|6|7 move instance 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckNotThrottledPerNode)
    {
        // Need to move 10 replicas to resolve capacity violation. Throttled to 2 IB per node, but all should be moved.
        Trace.WriteInfo("PLBThrottlingTestSource", "ConstraintCheckNotThrottledPerNode");

        // Maximum of 2 builds per node type
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps), L"ArenaMetric/10"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/1")));

        for (int i = 0; i < 20; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        }

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(10u, actionList.size());
        VERIFY_ARE_EQUAL(10u, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithPerNodeThrottling)
    {
        // Need to move 10 replicas to resolve capacity violation. Throttled to 2 IB per node, and throttling priority is 0.
        // Throttling should kick in and limit the number of moves to 2.
        Trace.WriteInfo("PLBThrottlingTestSource", "ConstraintCheckWithPerNodeThrottling");

        // Maximum of 2 builds per node type
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);
        PLBConfigScopeChange(ThrottlingConstraintPriority, int, 0);
        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps), L"ArenaMetric/10"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/1")));

        int ftIndex = 0;
        for (; ftIndex < 20; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B,S/3/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B"), 0));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move primary 0=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(PlacementWithPerNodeThrottling)
    {
        // Need to place 10 new replicas. Throttled to 2 IB per node, and throttling priority is 0.
        // Throttling should kick in and limit the number of placements.
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementWithPerNodeThrottling");

        PlacementTestHelper(false);
    }

    BOOST_AUTO_TEST_CASE(DoNotThrottlePrimaryBuildsTest)
    {
        // Need to place 10 new primary replicas. Throttled to 2 IB per node, and throttling priority is 0.
        // Throttling should not kick in because we do not throtle primary builds
        Trace.WriteInfo("PLBThrottlingTestSource", "DoNotThrottlePrimaryBuildsTest");

        PlacementTestHelper(true);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithThrottlingAndReplicaPrioritization)
    {
        // 4 partitions need new replicas:
        //  - 2 already have target, 2 need one extra.
        //  - There are 2 slots available for throttling.
        //  - Partitions that are not on target replica count should be placed.
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementWithThrottlingAndReplicaPrioritization");

        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap placementThrottlingLimits;

        placementThrottlingLimits.insert(make_pair(nodeTypeName, 1));

        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNodePlacementThrottle, PLBConfig::KeyIntegerValueMap, placementThrottlingLimits);

        PLBConfigScopeChange(ThrottlePlacementPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(
            L"TestService",
            L"TestType",
            true,
            CreateMetrics(L""),
            FABRIC_MOVE_COST_LOW,
            false,
            2));

        int ftIndex = 0;
        // Two FTs that are on target, need 1 extra replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/2"), 1));

        // Two FTs that are not on target, need 1 extra replica
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"2|3 add secondary 1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(ThrottledBalancingWithSwapsAndMovesTest)
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "ThrottledBalancingWithPossibleSwapsTest");
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap globalThrottlingLimits;

        globalThrottlingLimits.insert(make_pair(nodeTypeName, 2));

        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, globalThrottlingLimits);

        PLBConfigScopeChange(ThrottleBalancingPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"0_fd", L"UD1", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(1, L"1_fd", L"UD2", map<wstring, wstring>(nodeProps)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"CircuitMetric/1.0/1/0,ArenaMetric/1.0/0/1")));

        int ftIndex = 0;
        // These 4 FTs will cause the load to be 4 on node 0 and 0 on node 1 for ArenaMetric
        for (int i = 0; i < 4; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1"), 0));
        }

        // 8 load on node 0 for CircuitMetric, no load for ArenaMetric (no secondaries)
        for (int i = 0; i < 8; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        }

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(ThrottledBalancingWithPossibleSwapsTest)
    {
        // This test case was caught during the development of FabricTest.
        // 5 nodes (1 recently added), and state is more-less balanced:
        //      Primary replicas are equally distributed.
        //      10+ secondaries need to be moved to node 50.
        // Test checks that attempts to swap do not count towards in-build quota, and that 2 moves can be made.
        // Seed is random on purpose to provoke different behavior.
        if (!PLBConfig::GetConfig().IsTestMode)
        {
            Trace.WriteInfo("PLBThrottlingTestSource", "Skipping slow test: ThrottledBalancingWithPossibleSwapsTest");
        }
        Trace.WriteInfo("PLBThrottlingTestSource", "ThrottledBalancingWithPossibleSwapsTest");
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap globalThrottlingLimits;

        globalThrottlingLimits.insert(make_pair(nodeTypeName, 2));

        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, globalThrottlingLimits);

        PLBConfigScopeChange(ThrottleBalancingPhase, bool, true);

        PLBConfigScopeChange(ConstraintCheckEnabled, bool, false);

        PLBConfigScopeChange(InitialRandomSeed, int, -1);
        fm_->Load();

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(10, L"10_fd", L"UD1", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(20, L"20_fd", L"UD2", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(30, L"30_fd", L"UD3", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(40, L"40_fd", L"UD3", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(50, L"50_fd", L"UD3", map<wstring, wstring>(nodeProps)));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"")));

        int ftIndex = 0;
        // Replica placement from the placement dump.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/20, S/40, S/10, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/40, S/10, S/20, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/20, S/10, S/40, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/10, S/20, S/30, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/20, S/10, S/30, P/50"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/30, S/20, S/10, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/30, S/10, S/20, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/10, S/20, S/40, P/50"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/40, S/10, S/20, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/30, S/10, S/20, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/20, S/10, S/40, P/50"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/20, S/10, S/30, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/20, S/10, S/30, P/50"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/10, S/20, S/30, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/20, S/10, S/30, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/40, S/20, S/10, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/30, S/10, S/20, S/40"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/40, S/20, S/10, S/30"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/10, S/20, S/30, S/50"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/10, S/20, S/40, S/50"), 0));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move * *=>50", value)));

        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(PlacementConstraintThrottledTest)
    {
        // Checks if fixing placement constraint violation is throttled
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementConstraintThrottledTest");
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap globalThrottlingLimits;

        globalThrottlingLimits.insert(make_pair(nodeTypeName, 2));

        map<wstring, wstring> badNodeProps;
        badNodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        badNodeProps.insert(make_pair(L"ServiceAllowed", L"No"));
        map<wstring, wstring> goodNodeProps;
        goodNodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        goodNodeProps.insert(make_pair(L"ServiceAllowed", L"Yes"));

        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, globalThrottlingLimits);

        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(badNodeProps)));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(badNodeProps)));
        plb.UpdateNode(CreateNodeDescription(2, L"", L"", map<wstring, wstring>(badNodeProps)));
        plb.UpdateNode(CreateNodeDescription(3, L"", L"", map<wstring, wstring>(goodNodeProps)));
        plb.UpdateNode(CreateNodeDescription(4, L"", L"", map<wstring, wstring>(goodNodeProps)));
        plb.UpdateNode(CreateNodeDescription(5, L"", L"", map<wstring, wstring>(goodNodeProps)));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithConstraint(
            L"TestService", L"TestType", true, L"ServiceAllowed==Yes", CreateMetrics(L""), false, 3));

        int ftIndex = 0;
        // Replica placement from the placement dump.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/0, P/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"TestService"), 0, CreateReplicas(L"S/0, S/1, P/2"), 0));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(6u, actionList.size());

        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move * 0|1|2=>3", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move * 0|1|2=>4", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move * 0|1|2=>5", value)));

        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(AffinityConstraintThrottledTest)
    {
        // In current implementation, affinity cannot be throttled.
        Trace.WriteInfo("PLBThrottlingTestSource", "PlacementConstraintThrottledTest");
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap globalThrottlingLimits;

        globalThrottlingLimits.insert(make_pair(nodeTypeName, 2));

        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));

        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, globalThrottlingLimits);

        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, true);
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"", L"", map<wstring, wstring>(nodeProps)));
        plb.UpdateNode(CreateNodeDescription(1, L"", L"", map<wstring, wstring>(nodeProps)));
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"ParentService", L"TestType", true));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"ChildService", L"TestType", true, L"ParentService"));

        int ftIndex = 0;
        // Replica placement from the placement dump.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ParentService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ChildService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ChildService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ChildService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ChildService"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftIndex++), wstring(L"ChildService"), 0, CreateReplicas(L"P/1"), 0));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(5u, CountIf(actionList, ActionMatch(L"* move primary 1=>0", value)));

        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(FDConstraintThrottledTest)
    {
        // Checks if fixing FD violation is throttled
        Trace.WriteInfo("PLBThrottlingTestSource", "FDConstraintThrottledTest");
        DomainsTestHelper(true);
    }

    BOOST_AUTO_TEST_CASE(UDConstraintThrottledTest)
    {
        // Checks if fixing FD violation is throttled
        Trace.WriteInfo("PLBThrottlingTestSource", "UDConstraintThrottledTest");
        DomainsTestHelper(false);
    }

    BOOST_AUTO_TEST_SUITE_END()


    bool TestPLBThrottling::ClassSetup()
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBThrottling::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBThrottling::ClassCleanup()
    {
        Trace.WriteInfo("PLBThrottlingTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }

    void TestPLBThrottling::DomainsTestHelper(bool isFD)
    {
        // Maximum builds: 1 for NodeType1 (nodes with even id), 2 for NodeType2 (nodes with odd id)
        wstring smallNodeTypeName = L"SmallNodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(smallNodeTypeName, 1));

        wstring largeNodeTypeName = L"LargeNodeType";
        throttlingLimits.insert(make_pair(largeNodeTypeName, 2));

        map<wstring, wstring> smallNodeProps;
        smallNodeProps.insert(make_pair(*Constants::ImplicitNodeType, smallNodeTypeName));

        map<wstring, wstring> largeNodeProps;
        largeNodeProps.insert(make_pair(*Constants::ImplicitNodeType, largeNodeTypeName));

        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);
        PLBConfigScopeChange(ThrottlingConstraintPriority, int, 0);
        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            map<wstring, wstring> nodeProps = (i % 2 == 0) ? smallNodeProps : largeNodeProps;
            wstring fdId = L"";
            wstring udId = L"";
            if (isFD)
            {
                // We are testing FD violation, there will be 2 nodes per FD, and one node per UD
                fdId = wformatString("fd:/{0}", i / 2);
                udId = wformatString("{0}", i);
            }
            else
            {
                fdId = wformatString("fd:/{0}", i);
                udId = wformatString("{0}", i / 2);
            }
            plb.UpdateNode(CreateNodeDescription(i, fdId, udId, move(nodeProps)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        // 10 FTs with replicas on first 4 nodes, creating domain (FD or UD violation). At least 10 replica need to be moved to fix.
        int ftId = 0;
        for (; ftId < 10; ++ftId)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftId), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));
        }

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move * 0|1|2|3=>4", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move * 0|1|2|3=>5", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    void TestPLBThrottling::PlacementTestHelper(bool primaryBuild)
    {
        // Maximum of 2 builds per node type
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);
        PLBConfigScopeChange(ThrottlingConstraintPriority, int, 0);
        PLBConfigScopeChange(ThrottlePlacementPhase, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps), L"ArenaMetric/10"));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/1")));

        int ftId = 0;
        for (; ftId < 20; ++ftId)
        {
            if (primaryBuild)
            {
                // New partition, primary replica does not exist
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftId), wstring(L"TestService"), 0, CreateReplicas(L""), 1));
            }
            else
            {
                // Existing partition, primary replica exists on node 1
                plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftId), wstring(L"TestService"), 0, CreateReplicas(L"P/1"), 1));
            }
        }

        // In Build replicas per node (0 - 0) (1 - 3) (2 - 2) (3 - 1)
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftId++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B,S/3/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftId++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B,S/2/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(ftId++), wstring(L"TestService"), 0, CreateReplicas(L"P/0,S/1/B"), 0));

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        if (!primaryBuild)
        {
            VERIFY_ARE_EQUAL(3u, actionList.size());
            VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* add secondary 0", value)));
            VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* add secondary 3", value)));
        }
        else
        {
            VERIFY_ARE_EQUAL(20u, actionList.size());
            VERIFY_ARE_EQUAL(20u, CountIf(actionList, ActionMatch(L"* add primary *", value)));
        }
    }

    void TestPLBThrottling::SimplePerNodeThrottlingTestHelper(int inBuildCount, int expectedMoveCount)
    {
        // Maximum of 2 builds per node type
        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap throttlingLimits;
        throttlingLimits.insert(make_pair(nodeTypeName, 2));


        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, throttlingLimits);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        // Node 0 will have 20 active replicas
        int ftIndex = 0;
        for (; ftIndex < 20; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        }

        for (auto ibc = 0; ibc < inBuildCount; ++ibc)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(++ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0/V, S/1/B"), 0));
        }
        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(expectedMoveCount, actionList.size());
        VERIFY_ARE_EQUAL(expectedMoveCount, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
    }

    void TestPLBThrottling::PerNodeThrottlingTestHelper(int phase,
        int mode,
        int inBuildCount,
        int expectedMoveCount,
        bool throttlePlacement,
        bool throttleConstraintCheck,
        bool throttleBalancing,
        int throttlingConstraintPriority)
    {
        // Maximum of 2 builds per node type:
        //  mode == 0 --> Global limit = 2, Phase limit = 3, global is enforced
        //  mode == 1 --> Global limit = 3, Phase limit = 2, phase limit is enforced
        int globalLimit = 2;
        int phaseLimit = 2;
        if (mode == 0)
        {
            globalLimit = 2;
            phaseLimit = 3;
        }
        else if (mode == 1)
        {
            globalLimit = 3;
            phaseLimit = 2;
        }

        wstring nodeTypeName = L"NodeType";
        PLBConfig::KeyIntegerValueMap globalThrottlingLimits;
        PLBConfig::KeyIntegerValueMap placementThrottlingLimits;
        PLBConfig::KeyIntegerValueMap ccThrottlingLimits;
        PLBConfig::KeyIntegerValueMap balancingThrottlingLimits;

        globalThrottlingLimits.insert(make_pair(nodeTypeName, globalLimit));

        // phase: 0 - placement, 1 - constraint check, 2 - balancing
        if (phase == 0)
        {
            placementThrottlingLimits.insert(make_pair(nodeTypeName, phaseLimit));
        }
        else if (phase == 1)
        {
            ccThrottlingLimits.insert(make_pair(nodeTypeName, phaseLimit));
        }
        else if (phase == 2)
        {
            balancingThrottlingLimits.insert(make_pair(nodeTypeName, phaseLimit));
        }

        map<wstring, wstring> nodeProps;
        nodeProps.insert(make_pair(*Constants::ImplicitNodeType, nodeTypeName));
        PLBConfigScopeChange(MaximumInBuildReplicasPerNode, PLBConfig::KeyIntegerValueMap, globalThrottlingLimits);
        PLBConfigScopeChange(MaximumInBuildReplicasPerNodePlacementThrottle, PLBConfig::KeyIntegerValueMap, placementThrottlingLimits);
        PLBConfigScopeChange(MaximumInBuildReplicasPerNodeBalancingThrottle, PLBConfig::KeyIntegerValueMap, balancingThrottlingLimits);
        PLBConfigScopeChange(MaximumInBuildReplicasPerNodeConstraintCheckThrottle, PLBConfig::KeyIntegerValueMap, ccThrottlingLimits);

        PLBConfigScopeChange(ThrottlePlacementPhase, bool, throttlePlacement);
        PLBConfigScopeChange(ThrottleBalancingPhase, bool, throttleBalancing);
        PLBConfigScopeChange(ThrottleConstraintCheckPhase, bool, throttleConstraintCheck);

        PLBConfigScopeChange(ThrottlingConstraintPriority, int, throttlingConstraintPriority);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            if (phase == 1)
            {
                // To provoke constraint check, nodes will have capacity
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps), L"ArenaMetric/10"));
            }
            else
            {
                plb.UpdateNode(CreateNodeDescription(i, L"", L"", map<wstring, wstring>(nodeProps)));
            }
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"ArenaMetric/1.0/1/0")));

        // Node 0 will have 20 active replicas
        int ftIndex = 0;
        for (; ftIndex < 20; ++ftIndex)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        }

        for (auto ibc = 0; ibc < inBuildCount; ++ibc)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(++ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0/V, S/1/B"), 0));
        }

        if (phase == 0)
        {
            ++ftIndex;
            // If we are testing placement, then we need 20 additional new replicas
            int maxFtIndex = ftIndex + 20;
            for (; ftIndex < maxFtIndex; ++ftIndex)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(ftIndex), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 1));
            }
        }

        StopwatchTime now = Stopwatch::Now();

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(expectedMoveCount, actionList.size());
        if (phase == 1 || phase == 2)
        {
            // Balancing and constraint check will move primary replicas
            VERIFY_ARE_EQUAL(expectedMoveCount, CountIf(actionList, ActionMatch(L"* move primary 0=>1", value)));
        }
        else if (phase == 0)
        {
            VERIFY_ARE_EQUAL(expectedMoveCount, CountIf(actionList, ActionMatch(L"* add secondary 1", value)));
        }

        if (phase == 0)
        {
            VerifyPLBAction(plb, L"NewReplicaPlacement");
        }
        else if (phase == 1)
        {
            VerifyPLBAction(plb, L"ConstraintCheck");
        }
        else
        {
            VerifyPLBAction(plb, L"LoadBalancing");
        }
    }
}
