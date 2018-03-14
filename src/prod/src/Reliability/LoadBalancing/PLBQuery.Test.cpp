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


    class TestPLBQuery
    {
    protected:
        TestPLBQuery() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBQuery()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;
    };





    BOOST_FIXTURE_TEST_SUITE(TestPLBQuerySuite,TestPLBQuery)

    BOOST_AUTO_TEST_CASE(QueryingBeforeRefresh)
    {
        // We do not do anything in this test, just make sure that PLB returns an error if it is not ready
        PlacementAndLoadBalancing & plb = fm_->PLB;
        ServiceModel::ClusterLoadInformationQueryResult queryResult;

        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::PLBNotReady, result.ReadValue());

        // Now, refresh the PLB
        fm_->RefreshPLB(Stopwatch::Now());
        result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesBasicTest)
    {
        wstring testName = L"LoadQueriesBasicTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // First, put 50 of Metric2 to each node
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"Metric1/1.0/100/50")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        // Then, 50 of Metric2
        serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"Metric2/1.0/20/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1,S/0,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, L"Metric1", 200, false);
        VerifyLoad(queryResult, L"Metric2", 40, false);

        VerifyNodeLoadQuery(plb, 0, L"Metric1", 100);
        VerifyNodeLoadQuery(plb, 1, L"Metric1", 50);
        VerifyNodeLoadQuery(plb, 2, L"Metric1", 50);

        VerifyNodeLoadQuery(plb, 0, L"Metric2", 10);
        VerifyNodeLoadQuery(plb, 1, L"Metric2", 20);
        VerifyNodeLoadQuery(plb, 2, L"Metric2", 10);
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesDownNodes)
    {
        wstring testName = L"LoadQueriesDownNodes";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // Create a service (query won't return any load if metric load is 0)
        wstring serviceName = wformatString("{0}_0", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"Metric1/1.0/100/50")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyCapacity(queryResult, L"Metric1", 400);

        // Take one node down
        plb.UpdateNode(NodeDescription(CreateNodeInstance(2, 0),
            false,
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"Metric1/100")));

        // Deactivate one node and leave it UP
        plb.UpdateNode(NodeDescription(CreateNodeInstance(3, 0),
            true,
            Reliability::NodeDeactivationIntent::RemoveData,
            Reliability::NodeDeactivationStatus::DeactivationComplete,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"Metric1/100")));

        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyCapacity(queryResult, L"Metric1", 200);

        // Down and deactivated nodes should still list their capacities
        VerifyNodeCapacity(plb, 2, L"Metric1", 100);
        VerifyNodeCapacity(plb, 3, L"Metric1", 100);
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesWithoutServices)
    {
        // Metrics do have capacity, but do not belong to any service domain.
        wstring testName = L"LoadQueriesWithoutServices";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 4, 512));;

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyCapacity(queryResult, *ServiceModel::Constants::SystemMetricNameCpuCores, 6);
        VerifyCapacity(queryResult, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1536);

        VerifyNodeCapacity(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeCapacity(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 4);
        VerifyNodeCapacity(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024);
        VerifyNodeCapacity(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesMultipleNodeAndServiceTypes)
    {
        wstring testName = L"LoadQueriesMultipleNodeAndServiceTypes";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::map<wstring, wstring> node0Properties;
        node0Properties.insert(make_pair(L"NodeType", L"DB"));
        std::map<wstring, wstring> node1Properties;
        node1Properties.insert(make_pair(L"NodeType", L"DB"));
        std::map<wstring, wstring> node2Properties;
        node2Properties.insert(make_pair(L"NodeType", L"DB"));
        std::map<wstring, wstring> node3Properties;
        node3Properties.insert(make_pair(L"NodeType", L"WF"));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(0, L"Metric1/150,Metric2/100", move(node0Properties)));
        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(1, L"Metric1/150,Metric2/100", move(node1Properties)));

        // One down DB node
        plb.UpdateNode(NodeDescription(CreateNodeInstance(2, 0),
            false,
            Reliability::NodeDeactivationIntent::None,
            Reliability::NodeDeactivationStatus::None,
            map<wstring, wstring>(),
            NodeDescription::DomainId(),
            wstring(),
            map<wstring, uint>(),
            CreateCapacities(L"Metric1/150,Metric2/100")));

        plb.UpdateNode(CreateNodeDescriptionWithPlacementConstraintAndCapacity(3, L"Metric1/150", move(node3Properties)));

        fm_->RefreshPLB(Stopwatch::Now());

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // On every node
        wstring serviceName1 = wformatString("{0}_0", testName);
        plb.UpdateService(CreateServiceDescription(serviceName1, testType, false, CreateMetrics(L"Metric1/1.0/15/15"), FABRIC_MOVE_COST_LOW, true, -1));

        wstring serviceName2 = wformatString("{0}_DB", testName);
        plb.UpdateService(ServiceDescription(
            wstring(serviceName2),
            wstring(testType),
            wstring(L""),
            true,                                                  //bool isStateful,
            wstring(L"NodeType==DB"),                              //placementConstraints,
            wstring(),                                             //affinitizedService,
            false,                                                 //alignedAffinity,
            CreateMetrics(L"Metric1/1.0/100/50,Metric2/1.0/40/30"),//metrics
            FABRIC_MOVE_COST_ZERO,
            false,
            1,                                                     // partition count
            3,                                                     // target replica set size
            false));                                               // persisted

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"I/0,I/1,I/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyCapacity(queryResult, L"Metric1", 450);
        VerifyCapacity(queryResult, L"Metric2", 200);
        VerifyLoad(queryResult, L"Metric1", 195, false);
        VerifyLoad(queryResult, L"Metric2", 70, false);

        // Down node should still list its capacity
        VerifyNodeCapacity(plb, 2, L"Metric1", 150);
        VerifyNodeCapacity(plb, 2, L"Metric2", 100);
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesAfterConstraintCheck)
    {
        wstring testName = L"LoadQueriesAfterNoActionNeeded";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One FailoverUnit, loads: Node0: 100, Node1: 10, Node2: 10, Node 3: 0
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"Metric1/1.0/100/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        // With second FailoverUnit, loads: Node0: 100 + 10, Node1: 10 + 100, Node2: 10 + 10, Node 3: 0
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/1,S/0,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, L"Metric1", 240);

        VerifyNodeLoadQuery(plb, 0, L"Metric1", 110);
        VerifyNodeLoadQuery(plb, 1, L"Metric1", 110);
        VerifyNodeLoadQuery(plb, 2, L"Metric1", 20);
        VerifyNodeLoadQuery(plb, 3, L"Metric1", 0);
    }

    BOOST_AUTO_TEST_CASE(LoadQueriesAfterPlacement)
    {
        wstring testName = L"LoadQueriesAfterNoActionNeeded";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"Metric1/100"));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // One FailoverUnit, loads: Node0: 10, Node1: 10, Node2: 10, Node 3: 10
        int index = 0;
        wstring serviceName = wformatString("{0}{1}", testName, index);
        plb.UpdateService(CreateServiceDescription(serviceName, testType, true, CreateMetrics(L"Metric1/1.0/10/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 1));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, L"Metric1", 40);

        VerifyNodeLoadQuery(plb, 0, L"Metric1", 10);
        VerifyNodeLoadQuery(plb, 1, L"Metric1", 10);
        VerifyNodeLoadQuery(plb, 2, L"Metric1", 10);
        VerifyNodeLoadQuery(plb, 3, L"Metric1", 10);

        // With second FailoverUnit, loads: Node0: 10 + 10, Node1: 10 + 10, Node2: 10 + 10, Node 3: 10 + 10
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(index++), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, L"Metric1", 80);

        VerifyNodeLoadQuery(plb, 0, L"Metric1", 20);
        VerifyNodeLoadQuery(plb, 1, L"Metric1", 20);
        VerifyNodeLoadQuery(plb, 2, L"Metric1", 20);
        VerifyNodeLoadQuery(plb, 3, L"Metric1", 20);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadClusterWideMultipleDomainsNoMetricsTest)
    {
        wstring testName = L"ApplicationLoadClusterWideMultipleDomainsNoMetricsTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Create dummy service that will just be there
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"DummyTestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"DummyTestService", L"DummyTestType", true, CreateMetrics(L"MyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"DummyTestService"), 0, CreateReplicas(L"P/1, S/0"), 0)); // P/2, S/0 is optimal placement


        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;
        uint minNodes = 0;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring, ApplicationCapacitiesDescription> (),
            minNodes,
            maxNodes));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

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

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"I/0, I/1, I/2"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(minNodes, queryResult.MinimumNodes);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);
    }

    BOOST_AUTO_TEST_CASE(PLBApplicationLoadTest)
    {
        wstring testName = L"PLBApplicationLoadTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        int fuIndex = 0;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Create our services
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"NamingStoreService"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ClusterManagerServiceType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"NamingService", L"NamingStoreService", true, CreateMetrics(L"__NamingServicePrimaryCount__/1/1/0,__NamingServiceReplicaCount__/0.3/1/1")));
        plb.UpdateService(CreateServiceDescription(L"ClusterManagerServiceName", L"ClusterManagerServiceType", true, CreateMetrics(L"__ClusterManagerServicePrimaryCount__/1/1/0,__ClusterManagerServiceReplicaCount__/0.3/1/1")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"NamingService"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"NamingService"), 0, CreateReplicas(L"P/3,S/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(L"ClusterManagerServiceName"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;
        uint minNodes = 0;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            minNodes,
            maxNodes));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

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

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(serviceName), 0, CreateReplicas(L"I/0, I/1, I/2"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(minNodes, queryResult.MinimumNodes);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);
    }

    BOOST_AUTO_TEST_CASE(PLBApplicationClusterLoadTest)
    {
        wstring testName = L"PLBApplicationClusterLoadTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        int fuIndex = 0;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;
        uint minNodes = 2;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName),
            std::map<std::wstring, ApplicationCapacitiesDescription>(),
            minNodes,
            maxNodes));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(applicationName), minNodes, maxNodes, CreateApplicationCapacities(L"Memory/1000/100/50")));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

        plb.UpdateService(ServiceDescription(
            wstring(serviceName),
            wstring(testType),
            wstring(applicationName),
            false,          // isStateful
            wstring(L""),   // placementConstraints
            wstring(L""),   // affinitizedService
            true,           // alignedAffinity
            CreateMetrics(L"Memory/10/10/0"),
            FABRIC_MOVE_COST_LOW,
            true,           // onEveryNode
            1,              // partitionCount
            -1));           // targetReplicaSetSize

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(serviceName), 0, CreateReplicas(L"I/0, I/1, I/2"), INT_MAX));

        fm_->RefreshPLB(Stopwatch::Now());

        //Initial placement (Memory)
        //Nodes:        N0      N1      N2      N3
        //S1,Par0       I(20)   I(20)   I(20)
        //--------------  RESERVATION ------------------
        // App1:        (30)    (30)    (30)    -
        //--------------- EXPECTED TOTAL LOADS ---------
        // Nodes:      (50)    (50)     (50)    (0)
        // Cluster:   (150)

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(minNodes, queryResult.MinimumNodes);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);

        ServiceModel::ClusterLoadInformationQueryResult queryResult2;
        ErrorCode result2 = plb.GetClusterLoadInformationQueryResult(queryResult2);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result2.ReadValue());

        VerifyLoad(queryResult2, L"Memory", 150);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadBasicTest)
    {
        wstring testName = L"ApplicationLoadBasicTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.UpdateNode(CreateNodeDescription(4));

        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;
        plb.UpdateApplication(ApplicationDescription(wstring(applicationName), std::map<std::wstring, ApplicationCapacitiesDescription>(), maxNodes));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, applicationName, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(2, queryResult.NodeCount);

        // Now expand to 1 more node
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);

        // And now scaleout violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"P/3, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        fm_->RefreshPLB(Stopwatch::Now());
        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(4, queryResult.NodeCount);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadWithMetricsTest)
    {
        wstring testName = L"ApplicationLoadWithMetricsTest";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.UpdateNode(CreateNodeDescription(4));

        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(applicationName), maxNodes, CreateApplicationCapacities(L"CPU/30/10/0,Memory/6000/1000/0")));

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, applicationName, true, CreateMetrics(L"CPU/1.0/2/1,Memory/1.0/1000/500")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1, S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);
        VerifyApplicationLoadQuery(queryResult, L"CPU", 6);
        VerifyApplicationLoadQuery(queryResult, L"Memory", 3000);

        // Report load for two of two primaries
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, serviceName, L"CPU", 7, std::map<Federation::NodeId, uint>()));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, serviceName, L"Memory", 999, std::map<Federation::NodeId, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());
        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);
        VerifyApplicationLoadQuery(queryResult, L"CPU", 11);
        VerifyApplicationLoadQuery(queryResult, L"Memory", 2999);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadAfterUpdate)
    {
        // Start without application in PLB, but services do have non-empty application name.
        // Then update scaleout counts and capacities and verify load.
        wstring testName = L"ApplicationLoadAfterUpdate";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));
        plb.UpdateNode(CreateNodeDescription(3));
        plb.UpdateNode(CreateNodeDescription(4));

        wstring applicationName = wformatString("{0}Application", testName);
        uint maxNodes = 3;
        // There is no application at the beginning.

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceName, testType, applicationName, true, CreateMetrics(L"CPU/1.0/2/1,Memory/1.0/1000/500")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/1, S/2"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Basic verification steps. Application Load should be empty.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(0, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(0, queryResult.MinimumNodes);
        VERIFY_ARE_EQUAL(0, queryResult.NodeCount);

        // Now update the application, so that it does exist.
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(applicationName), maxNodes, CreateApplicationCapacities(L"CPU/30/10/0,Memory/6000/1000/0")));

        // Report load for two of two primaries
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetApplicationLoadInformationResult(applicationName, queryResult);
        Trace.WriteInfo("PLBQueryTestSource", "Error = {0}, QueryResult = {1}", result, queryResult);

        // Application should be returning load now.
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VERIFY_ARE_EQUAL(applicationName, queryResult.ApplicationName);
        VERIFY_ARE_EQUAL(maxNodes, queryResult.MaximumNodes);
        VERIFY_ARE_EQUAL(3, queryResult.NodeCount);
        VerifyApplicationLoadQuery(queryResult, L"CPU", 6);
        VerifyApplicationLoadQuery(queryResult, L"Memory", 3000);
    }

    BOOST_AUTO_TEST_CASE(AccountReservationInTotalLoad)
    {
        // Check that Application reservation is accounted in Node and Cluster PLB load info queries
        // Test stress following scenarios:
        //   - Application which reports load for one metric which have reservation on it,
        //     and metric which doesn't have active load but only reservation
        //   - Application that contains explicit zero reservation metric
        //   - Application that contains reservation on metric for which node capacity is not specified
        //   - Service without application name and reported loads for metrics which have reservations in cluster.
        wstring testName = L"AccountReservationInTotalLoad";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100,M3/100,M4/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100,M3/100,M4/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100,M2/100,M3/100,M4/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/100,M2/100,M3/100,M4/100"));
        plb.ProcessPendingUpdatesPeriodicTask();
        
        // Application which contains reservation for metric M1 and metric M2 that is not reported from service replicas
        // Also, first service has two partitions, hence local load balancing domain is created
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp1",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M1/400/100/40,M2/400/100/40"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType1", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType1", L"TestApp1", true, CreateMetrics(L"M1/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType1", L"TestApp1", true, CreateMetrics(L"M1/1.0/20/10")));

        // Application which contains reservation for metric M3 and report zero reservation for metric M4
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp2",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M3/400/100/20,M4/400/100/0"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType2", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType2", L"TestApp2", true, CreateMetrics(L"M3/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestServiceType2", L"TestApp2", true, CreateMetrics(L"M4/1.0/20/10")));

        // Application which contains reservation for metric M1 and M5 (M5 has no node capacity).
        // Metrics M1 and M3 are shared with first two applications
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp3",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M1/400/100/20,M3/400/100/10,M5/400/100/20"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType3", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestServiceType3", L"TestApp3", true, CreateMetrics(L"M1/1.0/10/5,M5/1.0/20/10")));

        // Service without application
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType4", set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService6", L"TestServiceType4", true, CreateMetrics(L"M1/1.0/5/5")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), L"TestService1", 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), L"TestService1", 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), L"TestService2", 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), L"TestService3", 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), L"TestService4", 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), L"TestService4", 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), L"TestService5", 0, CreateReplicas(L"P/1,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), L"TestService5", 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(8), L"TestService6", 0, CreateReplicas(L"P/0,S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        
        //Initial placement
        //Nodes:        N0(cap:100)         N1(cap:100)         N2(cap:100)         N3(cap:100)
        //S1,Par0       P(M1:20)            S(M1:10)            S(M1:10)
        //S1,Par1       P(M1:20)            S(M1:10)            S(M1:10)
        //S2,Par2       P(M1:20)            S(M1:10)
        //S3,Par3       P(M3:20)            S(M3:10)
        //S4,Par4                                               P(M4:20)            S(M4:10)
        //S4,Par5                                               P(M4:20)            S(M4:10)
        //S5,Par6                           P(M1:10,M5:20)                          S(M1:5,M5:10)
        //S5,Par7                           P(M1:10,M5:20)      S(M1:5,M5:10)                    
        //S6,Par8      P(M1:5)                                  S(M1:5)
        //----------------------------------  RESERVATION ----------------------------------------
        // App1:      (M1:0,M2:40)          (M1:10,M2:40)       (M1:20,M2:40)       (M1:0,M2:0)
        // App2:      (M3:0,M4:0)           (M3:10,M4:0)        (M3:20,M4:0)        (M3:20,M4:0)
        // App3:      (M1:0,M3:0,M5:0)      (M1:0,M3:10,M5:0)   (M1:15,M3:10,M5:10) (M1:15,M3:10,M5:10)
        // Total:     (M1:0,M2:40,M3:0,     (M1:10,M2:40,M3:20, (M1:35,M2:40,M3:30, (M1:15,M2:0,M3:30,
        //             M4:0,M5:0)           M4:0,M5:0)          M4:0,M5:10)         M4:0,M5:10)
        //------------------------------- EXPECTED TOTAL LOADS ------------------------------------
        // Nodes:     (M1:65,M2:40,M3:20,   (M1:60,M2:40,M3:30, (M1:65,M2:40,M3:30, (M1:20,M2:0,M3:30,
        //            M4:0,M5:0)            M4:0,M5:40)         M4:40,M5:20)        M4:20,M5:20)
        // Cluster:   (M1:210,M2:120,M3:110,M4:60,M5:80)
        // App1:      (M1:140,M2:120)
        // App2:      (M3:80,M4:60)
        // App3:      (M1:60,M3:30,M5:80)

        ServiceModel::ClusterLoadInformationQueryResult clusterLoadQueryResult;
        ServiceModel::ApplicationLoadInformationQueryResult appLoadQueryResult;
        
        // Cluster total load check
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(clusterLoadQueryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());
        VerifyLoad(clusterLoadQueryResult, L"M1", 210, false);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[0].MinNodeLoadValue, 20);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[0].MaxNodeLoadValue, 65);
        VerifyLoad(clusterLoadQueryResult, L"M2", 120, false);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[1].MinNodeLoadValue, 0);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[1].MaxNodeLoadValue, 40);
        VerifyLoad(clusterLoadQueryResult, L"M3", 110, false);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[2].MinNodeLoadValue, 20);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[2].MaxNodeLoadValue, 30);
        VerifyLoad(clusterLoadQueryResult, L"M4", 60, false);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[3].MinNodeLoadValue, 0);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[3].MaxNodeLoadValue, 40);
        VerifyLoad(clusterLoadQueryResult, L"M5", 80, false);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[4].MinNodeLoadValue, 0);
        VERIFY_ARE_EQUAL(clusterLoadQueryResult.LoadMetric[4].MaxNodeLoadValue, 40);
        
        // Per node total load check
        // Node 0
        VerifyNodeLoadQuery(plb, 0, L"M1", 65);
        VerifyNodeLoadQuery(plb, 0, L"M2", 40);
        VerifyNodeLoadQuery(plb, 0, L"M3", 20);
        VerifyNodeLoadQuery(plb, 0, L"M4", 0);
        VerifyNodeLoadQuery(plb, 0, L"M5", 0);

        // Node 1
        VerifyNodeLoadQuery(plb, 1, L"M1", 60);
        VerifyNodeLoadQuery(plb, 1, L"M2", 40);
        VerifyNodeLoadQuery(plb, 1, L"M3", 30);
        VerifyNodeLoadQuery(plb, 1, L"M4", 0);
        VerifyNodeLoadQuery(plb, 1, L"M5", 40);

        // Node 2
        VerifyNodeLoadQuery(plb, 2, L"M1", 65);
        VerifyNodeLoadQuery(plb, 2, L"M2", 40);
        VerifyNodeLoadQuery(plb, 2, L"M3", 30);
        VerifyNodeLoadQuery(plb, 2, L"M4", 40);
        VerifyNodeLoadQuery(plb, 2, L"M5", 20);

        // Node 3
        VerifyNodeLoadQuery(plb, 3, L"M1", 20);
        VerifyNodeLoadQuery(plb, 3, L"M2", 0);
        VerifyNodeLoadQuery(plb, 3, L"M3", 30);
        VerifyNodeLoadQuery(plb, 3, L"M4", 20);
        VerifyNodeLoadQuery(plb, 3, L"M5", 20);

        // Swap primary is called, 
        // hence full partition closure is created
        Federation::NodeId newPrimaryDest = CreateNodeId(1);
        result = plb.TriggerSwapPrimary(
            L"TestService3",
            CreateGuid(3),      // partition GUID
            CreateNodeId(0),    // source node
            newPrimaryDest);    // target node
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        // Cluster total load consistency check after swap primary move
        ServiceModel::LoadMetricInformation metricLoadInfo;
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M1");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 210);
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M2");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 120);
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M3");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 110);
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M4");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 60);
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M5");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 80);

        // Node total load consistency check after swap primary move
        ServiceModel::NodeLoadMetricInformation nodeMetricLoadInfo;
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 0, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 65);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 1, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 40);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 2, L"M3");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 30);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 3, L"M4");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 20);
    }

    BOOST_AUTO_TEST_CASE(AccountReservationInTotalLoadNodeBufferedCapacity)
    {
        // Check that Application reservation is accounted in Node and Cluster PLB load info queries,
        // when there is node buffered capacity specified
        // Test stress following scenarios:
        //   - Application which have only reservation (no active load is reported)
        //   - Two service domains for M1 and M2 metrics
        //   - Specific percentage of cluster is reserved as buffered capacity
        wstring testName = L"AccountReservationInTotalLoadNodeBufferedCapacity";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap nodeBufferPercentageMap;
        nodeBufferPercentageMap.insert(make_pair(L"M1", 0.2));
        nodeBufferPercentageMap.insert(make_pair(L"M2", 0.2));
        PLBConfigScopeChange(NodeBufferPercentage, PLBConfig::KeyDoubleValueMap, nodeBufferPercentageMap);

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(3, L"M1/100,M2/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Application which only contains reservation load for metric M1
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp1",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M1/400/100/40"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType1", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType1", L"TestApp1", true, CreateMetrics(L"")));

        // Application which contains active loads and reservation for metric M1
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp2",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M1/400/100/20"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType2", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType2", L"TestApp2", true, CreateMetrics(L"M1/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestServiceType2", L"TestApp2", true, CreateMetrics(L"M1/1.0/20/10")));

        // Application which contains active loads and reservation for metric M2
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp3",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M2/400/100/60"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType3", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"TestServiceType3", L"TestApp3", true, CreateMetrics(L"M2/1.0/45/20")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService5", L"TestServiceType3", L"TestApp3", true, CreateMetrics(L"M2/1.0/45/20")));

        // Service without application reporting metric M1
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType4", set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService6", L"TestServiceType4", true, CreateMetrics(L"M1/1.0/30/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), L"TestService1", 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), L"TestService2", 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), L"TestService3", 0, CreateReplicas(L"P/0,S/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), L"TestService4", 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), L"TestService5", 0, CreateReplicas(L"P/0,S/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), L"TestService6", 0, CreateReplicas(L"P/2,S/3"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        //Initial placement
        //Nodes:        N0(cap:100)         N1(cap:100)         N2(cap:100)         N3(cap:100)
        //S1,Par0       P                   S                   S
        //S2,Par1       P(M1:20)            S(M1:10)            S(M1:10)
        //S3,Par2       P(M1:20)                                S(M1:10)            S(M1:10)
        //S4,Par3       P(M2:45)            S(M2:20)            S(M2:20)
        //S5,Par4       P(M2:45)                                S(M2:20)            S(M2:20)
        //S6,Par5                                               P(M1:30)            S(M1:10)
        //----------------------------------  RESERVATION ----------------------------------------
        // App1:      (M1:40)               (M1:40)             (M1:40)
        // App2:      (M1:0)                (M1:10)             (M1:0)              (M1:10)
        // App3:      (M2:0)                (M2:40)             (M2:20)             (M2:40)
        // Total:     (M1:40,M2:0)          (M1:50,M2:40)       (M1:40,M2:20)       (M1:10,M2:40)
        //------------------------------- EXPECTED TOTAL LOADS ------------------------------------
        // Nodes:     (M1:80,M2:90)         (M1:60,M2:60)       (M1:90,M2:60)       (M1:30,M2:60)
        // Cluster:   (M1:260,M2:270)

        // Cluster total load check
        ServiceModel::LoadMetricInformation metricLoadInfo;
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M1");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 260);
        VERIFY_ARE_EQUAL(metricLoadInfo.RemainingUnbufferedCapacity, 140);
        VERIFY_ARE_EQUAL(metricLoadInfo.NodeBufferPercentage, 0.2);
        VERIFY_ARE_EQUAL(metricLoadInfo.BufferedCapacity, 320);
        VERIFY_ARE_EQUAL(metricLoadInfo.RemainingBufferedCapacity, 60);

        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M2");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 270);
        VERIFY_ARE_EQUAL(metricLoadInfo.RemainingUnbufferedCapacity, 130);
        VERIFY_ARE_EQUAL(metricLoadInfo.NodeBufferPercentage, 0.2);
        VERIFY_ARE_EQUAL(metricLoadInfo.BufferedCapacity, 320);
        VERIFY_ARE_EQUAL(metricLoadInfo.RemainingBufferedCapacity, 50);

        // Per node total load check
        // Node 0
        ServiceModel::NodeLoadMetricInformation nodeMetricLoadInfo;
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 0, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 20);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 0);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);
        
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 0, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 90);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 10);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 0);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, true);

        // Node 1
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 1, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 60);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 40);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 20);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);

        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 1, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 60);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 40);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 20);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);

        // Node 2
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 2, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 90);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 10);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 0);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation,true);

        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 2, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 60);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 40);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 20);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);

        // Node 3
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 3, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 30);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 70);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 50);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);

        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 3, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 60);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingCapacity, 40);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeBufferedCapacity, 80);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeRemainingBufferedCapacity, 20);
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.IsCapacityViolation, false);

        // Swap primary is called, 
        // hence full partition closure is created
        Federation::NodeId newPrimaryDest = CreateNodeId(3);
        Common::ErrorCode result = plb.TriggerSwapPrimary(
            L"TestService6",
            CreateGuid(5),      // partition GUID
            CreateNodeId(2),    // source node
            newPrimaryDest);    // target node
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        // Cluster total load consistency check after swap primary move
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M1");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 260);
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M2");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 270);

        // Node total load consistency check after swap primary move
        // (load distribution should be the same, as PLB only proposed move to FM)
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 0, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 80);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 1, L"M2");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 60);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 2, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 90);
        GetNodeLoadMetricInformation(plb, nodeMetricLoadInfo, 3, L"M1");
        VERIFY_ARE_EQUAL(nodeMetricLoadInfo.NodeLoad, 30);
    }

    BOOST_AUTO_TEST_CASE(AccountReservationInTotalLoadMovePrimaryCommand)
    {
        // Check that Application reservation is accounted in Cluster capacity,
        // and it is consisted after move primary command is issued
        wstring testName = L"AccountReservationInTotalLoadMovePrimaryCommand";
        Trace.WriteInfo("PLBQueryTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithCapacity(0, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(1, L"M1/100,M2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"M1/100,M2/100"));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Application which only contains reservation load for metric M1
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"TestApp1",
            1,
            4,
            CreateApplicationCapacities(wformatString(L"M1/300/100/40,M2/300/100/40"))));
        plb.UpdateServiceType(ServiceTypeDescription(L"TestServiceType1", set<NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestServiceType1", L"TestApp1", true, CreateMetrics(L"M1/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestServiceType1", L"TestApp1", true, CreateMetrics(L"M2/1.0/20/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), L"TestService1", 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), L"TestService1", 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), L"TestService2", 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), L"TestService2", 0, CreateReplicas(L"P/0,S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        //Initial placement
        //Nodes:        N0(cap:100)         N1(cap:100)         N2(cap:100)
        //S1,Par0       P(M1:20)            S(M1:10)
        //S1,Par1       P(M1:20)            S(M1:10)
        //S2,Par2                           P(M1:20)            S(M1:10)
        //S2,Par3       P(M2:20)            S(M2:20)
        //----------------------------------  RESERVATION ----------------------------------------
        // Total:      (M1:0,M2:20)         (M1:20,M2:0)       (M1:40,M2:30)
        //------------------------------- EXPECTED TOTAL LOADS ------------------------------------
        // Nodes:      (M1:40,M2:40)        (M1:40,M2:40)      (M1:40,M2:40)
        // Cluster:    (M1:120,M2:120)

        // Swap primary is called
        Federation::NodeId newPrimaryDest = CreateNodeId(1);
        Common::ErrorCode result = plb.TriggerSwapPrimary(
            L"TestService1",
            CreateGuid(0),  // partition GUID
            CreateNodeId(0),  // source node
            newPrimaryDest); // target node
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        // Cluster total load check
        ServiceModel::LoadMetricInformation metricLoadInfo;
        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M1");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 120);

        GetClusterLoadMetricInformation(plb, metricLoadInfo, L"M2");
        VERIFY_ARE_EQUAL(metricLoadInfo.ClusterLoad, 120);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestPLBQuery::ClassSetup()
    {
        Trace.WriteInfo("PLBQueryTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBQuery::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBQuery::ClassCleanup()
    {
        Trace.WriteInfo("PLBQueryTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
