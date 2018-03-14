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


    class TestPLBBalancingWithConstraintViolation
    {
    protected:
        TestPLBBalancingWithConstraintViolation() { 
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBBalancingWithConstraintViolation()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;
    };



    BOOST_FIXTURE_TEST_SUITE(TestPLBBalancingWithConstraintViolationSuite,TestPLBBalancingWithConstraintViolation)

    BOOST_AUTO_TEST_CASE(BalancingWithBlockListViolationsTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithBlockListViolationsTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        blockList.insert(CreateNodeId(3));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService0"), 0, CreateReplicas(L"P/1"), 0));

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        fm_->Clear();

        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"* move * *=>3", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDomainViolationsTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithDomainViolationsTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(5, L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescription(6, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(7, L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescription(8, L"dc1/rack2"));
        plb.UpdateNode(CreateNodeDescription(9, L"dc1/rack2"));
        plb.UpdateNode(CreateNodeDescription(10, L"dc1/rack2"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2, S/4, S/6, S/8"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/4, S/6, S/8"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/4, S/6, S/8"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"I/0, I/1, I/2, I/4, I/6, I/8"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"I/1, I/5, I/7, I/9"), 0));

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityViolationsTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithAffinityViolationsTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);
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

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(metricStr)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService2", CreateMetrics(metricStr)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/3, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/2"), 0));

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithAffinityViolationsChildTargetLargerTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithAffinityViolationsChildTargetLargerTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);
        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(ServiceMetric::PrimaryCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::ReplicaCountName, 1.0));
        globalWeights.insert(make_pair(ServiceMetric::CountName, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 6; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        wchar_t metricStr[] = L"PrimaryCount/0.0/0/0,ReplicaCount/0.0/0/0,Count/0.0/0/0";
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true, CreateMetrics(metricStr), FABRIC_MOVE_COST_LOW, false, 3));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService0", CreateMetrics(metricStr), false, 4));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService0", CreateMetrics(metricStr), false, 4));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/2, S/3, S/4"), 0));

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        // We should observe some replica balancing while improving (or not making worse) affinity violations
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithCapacityViolationTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithCapacityViolationTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::FromTicks(1));
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::FromTicks(1));
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Count/25"));
        plb.UpdateNode(CreateNodeDescription(1));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"BalancingWithCapacityViolationTest_TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"BalancingWithCapacityViolationTest_TestService", L"BalancingWithCapacityViolationTest_TestType", false));

        for (int i = 0; i < 100; i++)
        {
            if (i < 60)
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(i), wstring(L"BalancingWithCapacityViolationTest_TestService"), 0, CreateReplicas(L"I/0"), 0));
            }
            else
            {
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(i), wstring(L"BalancingWithCapacityViolationTest_TestService"), 0, CreateReplicas(L"I/1"), 0));
            }
        }

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(10u, actionList.size());
        VERIFY_ARE_EQUAL(10, CountIf(actionList, ActionMatch(L"* move instance 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragWithCapacityViolationTest)
    {
        wstring testName = L"DefragWithCapacityViolationTest";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

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
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/50/50", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        for (int i = 1; i < 5; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/10/10", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        // Node0 contains 2 replicas and load is over capacity (110). Other nodes have one primary replica each with load 10.
        // PLB should do the defragmentation (balancing phase) but it should not fix Node0 capacity violation.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"* move primary 0=>*", value)));
    }

    BOOST_AUTO_TEST_CASE(DefragWithAffinityViolationTest)
    {
        wstring testName = L"DefragWithAffinityViolationTest";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventIntermediateOvercommit, bool, true);
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, false);

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

        for (int i = 0; i < 2; i++)
        {
            serviceName = wformatString("{0}{1}", testName, index);
            plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/70/70", defragMetric))));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(wformatString("P/{0}", i)), 0));
        }

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1/20/20", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));

        wstring parentService = serviceName;

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            serviceName, testType, true, parentService, CreateMetrics(wformatString("{0}/1/20/20", defragMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1"), 0));

        // First two nodes have load 90 (capacity is 100).
        // Each one of them contains 2 partitions with 1 replica - loads are 70 and 20.
        // Partitions with load 20 are affinitized but they are on two different nodes.
        // PLB should not create node chain violation during constraint check phase.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        /* uncomment with bug 6721174
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"2 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"3 move primary 1=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 0=>*", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 1=>*", value)));
        */
    }

    BOOST_AUTO_TEST_CASE(AffinityViolationWithoutIntermediateOvercommitTest)
    {
        wstring testName = L"AffinityViolationWithoutIntermediateOvercommitTest";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PreventIntermediateOvercommit, bool, true);
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, false);
        std::wstring defragMetric = L"MyMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        int parentFUindex, childFUindex;
        wstring serviceName;
        //parent service...
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, false, CreateMetrics(wformatString("{0}/1/0/0", defragMetric))));
        parentFUindex = index;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/0, I/2"), 0));

        wstring parentService = serviceName;

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            serviceName, testType, false, parentService, CreateMetrics(wformatString("{0}/1/0/0", defragMetric))));
        childFUindex = index;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/1, I/2"), 0));

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(0), 80));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(parentFUindex, parentService, defragMetric, 0, loads));

        //plb.ProcessPendingUpdatesPeriodicTask();
        map<NodeId, uint> loads2;
        loads2.insert(make_pair(CreateNodeId(2), 70));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(parentFUindex, parentService, defragMetric, 0, loads2));
        //plb.ProcessPendingUpdatesPeriodicTask();
        map<NodeId, uint> loads3;
        loads3.insert(make_pair(CreateNodeId(1), 30));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(childFUindex, serviceName, defragMetric, 0, loads3));
        //plb.ProcessPendingUpdatesPeriodicTask();
        map<NodeId, uint> loads4;
        loads4.insert(make_pair(CreateNodeId(2), 20));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(childFUindex, serviceName, defragMetric, 0, loads4));


        plb.ProcessPendingUpdatesPeriodicTask();

        // Loads are 80, 30, 90 (capacity is 100).
        // First and second node contain torn parent and child respectively, but these cannot be brought together because intermediate overcommits are prevented.
        // Third node contains both, and is stable.
        // PLB should not create node chain violation during constraint check phase. No Movements can fix this AffinityViolation
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //VERIFY_ARE_EQUAL(2u, actionList.size());
        //VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"14 move primary 0=>1", value)));
        //VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"15 move primary 1=>0", value)));
        //VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"14 move primary 0=>*", value)));
        //VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"15 move primary 1=>*", value)));

        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(AffinityViolationWithoutIntermediateOvercommitTest2)
    {
        wstring testName = L"AffinityViolationWithoutIntermediateOvercommitTest2";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(PreventIntermediateOvercommit, bool, true);
        PLBConfigScopeChange(MoveParentToFixAffinityViolation, bool, false);

        std::wstring defragMetric = L"MyMetric";
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(defragMetric, true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(defragMetric, 1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0/r0", wformatString("{0}/100", defragMetric)));
        }

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int index = 0;
        int parentFUindex, childFUindex, placeholderFUindex;
        wstring parentService, childService, placeholderService;
        wstring serviceName;
        //parent service...
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, false, CreateMetrics(wformatString("{0}/1/0/0", defragMetric))));
        parentFUindex = index;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/0"), 0));
        parentService = serviceName;
        //child service
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(
            serviceName, testType, false, parentService, CreateMetrics(wformatString("{0}/1/0/0", defragMetric))));
        childFUindex = index;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/1"), 0));
        childService = serviceName;
        //Placeholder service...
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, false, CreateMetrics(wformatString("{0}/1/0/0", defragMetric))));
        placeholderFUindex = index;
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"I/0, I/1"), 0));
        placeholderService = serviceName;

        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(0), 80));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(parentFUindex, parentService, defragMetric, 0, loads));

        map<NodeId, uint> loads2;
        loads2.insert(make_pair(CreateNodeId(1), 20));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(childFUindex, childService, defragMetric, 0, loads2));

        map<NodeId, uint> loads3;
        loads3.insert(make_pair(CreateNodeId(0), 10));
        loads3.insert(make_pair(CreateNodeId(1), 10));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(placeholderFUindex, placeholderService, defragMetric, 0, loads3));


        plb.ProcessPendingUpdatesPeriodicTask();

        // Loads are 90, 30, 0 (capacity is 100).
        // First and second node contain torn parent and child respectively, but these can only be brought together on third node because intermediate overcommits are prevented.
        // PLB should not create node chain violation during constraint check phase.
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        /* uncomment with bug 6721174
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"0 move instance 0=>1", value)));
        VERIFY_ARE_EQUAL(0u, CountIf(actionList, ActionMatch(L"1 move instance 1=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move instance 0=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move instance 1=>2", value)));
        */
    }

    BOOST_AUTO_TEST_CASE(BalancingWithMultipleViolationsTest)
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "BalancingWithMultipleViolationsTest");
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(1, L"dc0"));
        plb.UpdateNode(CreateNodeDescription(2, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(3, L"dc1"));
        plb.UpdateNode(CreateNodeDescription(4, L"dc2"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService0", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(1));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService0"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService0"), 0, CreateReplicas(L"P/1, S/0"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService1"), 0, CreateReplicas(L"P/1, S/0"), 0)); // P/2, S/1
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/2"), 0)); // P/1, S/2
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService1"), 0, CreateReplicas(L"P/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService1"), 0, CreateReplicas(L"P/3, S/2"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService2"), 0, CreateReplicas(L"P/4, S/3"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(L"TestService3"), 0, CreateReplicas(L"P/4, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), wstring(L"TestService3"), 0, CreateReplicas(L"P/3, S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(9), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/4"), 0)); // P/0, S/4

        Sleep(1); // add a slight delay so we can control the searches PLB performs

        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_TRUE(0u < actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithUnderloadedFaultDomainTest2)
    {
        wstring testName = L"BalancingWithUnderloadedFaultDomainTest2";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);

        wstring myMetric = L"MyMetric";
        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, wformatString("dc0/left/r{0}", i), wformatString("{0}/50", myMetric)));
        }
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(3, wformatString("dc0/right/r{0}", 3), wformatString("{0}/100", myMetric)));
        plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(4, wformatString("dc0/right/r{0}", 4), wformatString("{0}/50", myMetric)));
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
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"S/0,S/1,P/3,S/4,S/5"), 0));

        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(wformatString("{0}/1.0/60/60", myMetric))));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/3"), 0));

        // First service cannot be moved as partition will remain in violation
        // Second service cannot be moved to the first three nodes - not enough node capacity
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(BalancingWithScaleoutCountViolationsTest1)
    {
        wstring testName = L"BalancingWithScaleoutCountViolationsTest1";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
        plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName = wformatString("{0}Application", testName);
        int appScaleoutCount = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(appName), std::map<std::wstring, ApplicationCapacitiesDescription>(), appScaleoutCount));
        wstring serviceName = testName;
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, appName, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/2, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //Secondary on 1 goes standby, new secondary on 3
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"P/0, S/3, SB/1"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        //We should have an assert here if something is wrong....
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(BalancingMakesAffinityViolation)
    {
        wstring testName = L"BalancingMakesAffinityViolation";
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "{0}", testName);
        PLBConfigScopeChange(IsTestMode, bool, false);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App",
            0,  // MinimumNodes - no reservation
            61,  // MaximumNodes
            CreateApplicationCapacities(L"SurprizeMetric/61/1/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType1"), set<NodeId>()));
        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));
        blockList.insert(CreateNodeId(2));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType2"), move(blockList)));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"Service1",
            L"TestType1",
            wstring(L"App"),
            true,
            CreateMetrics(L"SurprizeMetric/1/1/0,Dummy/1/1/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"Service2", L"TestType1", true, L"App", L"Service1", CreateMetrics(L"Dummy/1/1/0")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"Service3",
            L"TestType2",
            wstring(L"App"),
            true,
            CreateMetrics(L"SurprizeMetric/1/1/0,Dummy/1/1/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"Service3"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(actionList.size(), 0u);

        fm_->RefreshPLB(Stopwatch::Now());

        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(actionList.size(), 0u);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestPLBBalancingWithConstraintViolation::ClassSetup()
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBBalancingWithConstraintViolation::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBBalancingWithConstraintViolation::ClassCleanup()
    {
        Trace.WriteInfo("PLBBalancingWithConstraintViolationTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}

    /*
    void TestPLBBalancingWithConstraintViolation::BalancingWithScaleoutCountViolationsTest1()
    {
        wstring testName = L"BalancingWithScaleoutCountViolationsTest1";
        Log::Comment(String().Format(L"{0}", testName));
        PLBConfigScopeChange(ConstraintCheckSearchTimeout, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName = wformatString("{0}Application", testName);
        int appScaleoutCount = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(appName), std::map<std::wstring, ApplicationCapacitiesDescription>(), appScaleoutCount));

        plb.UpdateService(CreateServiceDescriptionWithApplication(testName, testType, appName, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService"), 0, CreateReplicas(L"P/2, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService"), 0, CreateReplicas(L"P/2, S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService"), 0, CreateReplicas(L"P/3, S/2"), 0));


        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);
        fm_->Clear();
        now += PLBConfig::GetConfig().MaxMovementExecutionTime;
        fm_->RefreshPLB(now);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_IS_LESS_THAN(0u, actionList.size());
    }
    */


