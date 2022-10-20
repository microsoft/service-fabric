// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"
#include "TestFM.h"
#include "PLBConfig.h"
#include "RandomDistribution.h"
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

    class TestPLBPerformance
    {
    protected:
        TestPLBPerformance() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }
        TEST_CLASS_SETUP(ClassSetup);
        TEST_METHOD_SETUP(TestSetup);

        void StaticTestHelper(std::wstring testName,
            size_t numNodes,                // Total number of nodes
            size_t numServices,             // Total number of services
            size_t numPartitionsPerService, // Number of partitions per service
            size_t numReplicasPerService,   // Number of replicas per partition
            bool useApplication,            // Every service in its own application
            size_t numIterations            // How many iterations in test
        );

        void StaticHelperCreateService(
            wstring const& testName,
            wstring const& serviceNamePrefix,
            PlacementAndLoadBalancingTestHelper & plb,
            size_t & serviceNo,
            size_t & partitionId,
            size_t numNodes,
            size_t numPartitionsPerService,
            size_t numReplicasPerPartition,
            bool useApplication);

        int64 PlacementPerfTestRunner(
            int nodeCount,
            int containerCount,
            int fdCount = 5,
            int udCount = 5,
            bool reserveNodesForSystemServices = false,
            bool useMoveCost = false,
            RandomDistribution::Enum moveCostDistribution = RandomDistribution::Enum::Exponential,
            bool useNodeCapacities = false,
            uint32_t cpuCapacity = 64,
            uint32_t memoryCapacity = 2300,
            RandomDistribution::Enum loadDistribution = RandomDistribution::Enum::Exponential,
            bool useRandomWalking = false,
            bool useDefrag = false,
            bool verboseOutput = false);

        void SetupAllFeaturesPerformanceTest(std::wstring const& testName,
            size_t numNodes,
            size_t numberOfApplications,
            size_t applicationMaxNodes,
            size_t numChildServicesPerApplication,
            size_t numPartitionsPerChildService,
            bool useDifferentLoadForPrimary,
            bool includeInFUMap = false);

        void BalancingWithManyNodesHelper(std::wstring testName, bool onEveryNode, bool placementConstraints, int createApplications = 0);

        // 0 - No changes
        // 1 - No application (app name == L"")
        // 2 - Many app groups (scaleout == 1000) - no violations ever
        void StressTestHelper(std::wstring testName, uint type);

        shared_ptr<TestFM> fm_;
        PlacementAndLoadBalancingTestHelperUPtr plb_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBPerformanceSuite, TestPLBPerformance)

    /*************************************************************************/
    //                                STATIC TESTS
    /*************************************************************************/
    BOOST_AUTO_TEST_CASE(BatchPlacementWithHighNumberOfDownNodes)
    {
        wstring testName = L"BatchPlacementWithHighNumberOfDownNodes";
        Trace.WriteInfo("BatchPlacementWithHighNumberOfDownNodes", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        
        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        const int upNodeCount = 150;
        const int NodeCount = 20000;
        const int partitionCount = 10;
        const int instanceCount = 100;
        const int fdCount = 10;
        const int udCount = 10;
        const int numIterations = 10;

        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        // Setup UP nodes
        for (int i = 0; i < upNodeCount; i++)
        {
            NodeDescription::DomainId fdId = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", wformatString("{0}", i % fdCount)).Segments;
            plb.UpdateNode(NodeDescription(
                CreateNodeInstance(i, 0),
                true, // Up
                Reliability::NodeDeactivationIntent::Enum::None,
                Reliability::NodeDeactivationStatus::Enum::None,
                map<wstring, wstring>(),
                move(fdId),
                wformatString("{0}", i % udCount),
                map<wstring, uint>(),
                map<wstring, uint>()));
        }

        // Setup Down nodes
        for (int i = upNodeCount; i < NodeCount; i++)
        {
            NodeDescription::DomainId fdId = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", wformatString("{0}", i % fdCount)).Segments;
            plb.UpdateNode(NodeDescription(
                CreateNodeInstance(i, 0),
                false, // Down
                Reliability::NodeDeactivationIntent::Enum::None,
                Reliability::NodeDeactivationStatus::Enum::DeactivationComplete,
                map<wstring, wstring>(),
                move(fdId),
                wformatString("{0}", i % udCount),
                map<wstring, uint>(),
                CreateCapacities(L"")));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        // Prepare PLB for querying (create snapshots)
        StopwatchTime now = Stopwatch::Now();
        fm_->RefreshPLB(now);

        // Setup services
        int fuId = 0;
        wstring serviceName1 = wformatString("{0}_Service1", testName);
        plb.UpdateService(CreateServiceDescription(serviceName1, testType, false, CreateMetrics(L"MyMetric1/1.0/0/0")));
        for (int j = 0; j < partitionCount; j++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount));
            fm_->FuMap.insert(make_pair<>(CreateGuid(fuId), move(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount))));
            fuId++;
        }
        wstring serviceName2 = wformatString("{0}_Service2", testName);
        plb.UpdateService(CreateServiceDescription(serviceName2, testType, false, CreateMetrics(L"MyMetric2/1.0/0/0")));
        for (int j = 0; j < partitionCount; j++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName2), 0, CreateReplicas(L""), instanceCount));
            fm_->FuMap.insert(make_pair<>(CreateGuid(fuId), move(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName2), 0, CreateReplicas(L""), instanceCount))));
            fuId++;
        }
        now += PLBConfig::GetConfig().MinPlacementInterval;
        fm_->RefreshPLB(now);

        // Check that all services are placed
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2000u, actionList.size());
        fm_->Clear();

        // Run multiple iterations and check the plb refresh timers 
        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);
            fm_->Clear();

            // Just add the service to have a change (to avoid throttle).
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount));
            fm_->FuMap.insert(make_pair<>(CreateGuid(fuId), move(FailoverUnitDescription(CreateGuid(fuId), wstring(serviceName1), 0, CreateReplicas(L""), instanceCount))));
            fuId++;

            // Go 10 days ahead
            now += TimeSpan::FromSeconds(10 * 24 * 60 * 60);

            plb.Refresh(now);

            // Check that engine run is not less than 40% of overall time
            uint64 engineTime = plb.RefreshTimers.msTimeCountForEngine;
            uint64 refreshTime = plb.RefreshTimers.msRefreshTime;
            if ((engineTime * 1.0 / refreshTime) < 0.4)
            {
                VERIFY_FAIL(L"Engine runtime is below 40% of overall refresh runtime");
            }
        }

        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(BasicPerformanceTest)
    {
        StaticTestHelper(
            L"BasicPerformanceTest",
            40,         // Total number of nodes
            500000,     // Total number of services
            1,          // Number of partitions per service
            4,          // Number of replicas per partition
            false,      // Every service in its own application
            10          // How many iterations in test
        );
    }

    BOOST_AUTO_TEST_CASE(MultiPartitionPerformanceTest)
    {
        // Number of replicas remains the same as in BasicPerformanceTest
        // Difference is that there are 10x less services, each with 10 partitions
        StaticTestHelper(
            L"MultiPartitionPerformanceTest",
            40,         // Total number of nodes
            50000,      // Total number of services
            10,         // Number of partitions per service
            4,          // Number of replicas per partition
            false,      // Every service in its own application
            10          // How many iterations in test
        );
    }

    BOOST_AUTO_TEST_CASE(ManyNodesPerformanceTest)
    {
        // Same as BasicPerformanceTest, just spread out over 1000 nodes.
        StaticTestHelper(
            L"ManyNodesPerformanceTest",
            1000,       // Total number of nodes
            500000,     // Total number of services
            1,          // Number of partitions per service
            4,          // Number of replicas per partition
            false,      // Every service in its own application
            10          // How many iterations in test
        );
    }

    BOOST_AUTO_TEST_CASE(AllFeaturesPerformanceTestStatic)
    {
        auto & plb = fm_->PLBTestHelper;

        std::wstring testName = L"AllFeaturesPerformanceTestStatic";
        size_t numIterations = 10;

        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        SetupAllFeaturesPerformanceTest(testName,
            20,         // Number of nodes
            80000,      // Number of applications
            4,          // Application max nodes
            1,          // Number of child services per application
            2,          // Number of partitions per application
            false);     // false ==> Both primary and secondary replicas will have the same load

        StopwatchTime now = Stopwatch::Now();

        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);
            fm_->Clear();

            // Just add the service to have a change (to avoid throttle).
            plb.UpdateService(CreateServiceDescription(wformatString("{0}_Dummy_{1}", testName, iteration), wstring(L"MyServiceType"), true));
            plb.DeleteService(wformatString("{0}_Dummy_{1}", testName, iteration));

            plb.Refresh(now);

            // Go 10 days ahead (all timers expire)
            now += TimeSpan::FromSeconds(10 * 24 * 60 * 60);
        }
        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(CompareNodeForPromotionSpeedTest)
    {
        auto & plb = fm_->PLBTestHelper;

        std::wstring testName = L"CompareNodeForPromotionSpeedTest";
        size_t numIterations = 10;

        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        SetupAllFeaturesPerformanceTest(testName,
            20,         // Number of nodes
            80000,      // Number of applications
            4,          // Application max nodes
            1,          // Number of child services per application
            2,          // Number of partitions per application
            false,      // false ==> Both primary and secondary replicas will have the same load
            true);      // Update FT map in test FM

        Stopwatch comparisonStopwatch;
        size_t numComparisons = 0;

        comparisonStopwatch.Reset();
        comparisonStopwatch.Start();

        for (auto iteration = 0; iteration < numIterations; ++iteration)
        {
            for (auto const& guidFUPair : fm_->FuMap)
            {
                FailoverUnitDescription const& fuDesc = guidFUPair.second;
                size_t numReplicas = fuDesc.Replicas.size();
                for (size_t first = 0; first < numReplicas; ++first)
                {
                    if (fuDesc.PrimaryReplicaIndex == first)
                    {
                        continue;
                    }
                    for (size_t second = 0; second < numReplicas; ++second)
                    {
                        if (first == second || second == fuDesc.PrimaryReplicaIndex)
                        {
                            continue;
                        }

                        plb.CompareNodeForPromotion(fuDesc.ServiceName,
                            guidFUPair.first,
                            fuDesc.Replicas[first].NodeId,
                            fuDesc.Replicas[second].NodeId);
                        numComparisons++;
                    }
                }
            }
        }

        comparisonStopwatch.Stop();

        Trace.WriteInfo("PLBPerformanceTestSource",
            "CompareNodeForPromotion_Time={0} CompareNodeForPromotion_Count={1}",
            comparisonStopwatch.ElapsedMilliseconds,
            numComparisons);

        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(PlacementDensityTest)
    {
        wstring testName = L"PlacementDensityTest";
        size_t numIterations = 1;

        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        PLBConfigScopeChange(PlacementSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(FastBalancingSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));

        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 0);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 0);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        PLBConfigScopeChange(IsTestMode, bool, false);
        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        const int nodeCount = PLBConfig::GetConfig().PerfTestNumberOfNodes; // 3000;
        const int partitionedServiceCount = 1;
        const int partitionCount = PLBConfig::GetConfig().PerfTestNumberOfPartitions; // 100000;
        const int instanceCount = PLBConfig::GetConfig().PerfTestNumberOfInstances; // 10
        const int fdCount = 5;
        const int udCount = 5;

        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);

            fm_->Load();
            PlacementAndLoadBalancing & plb = fm_->PLB;

            {
                map<wstring, wstring> nodeProperties;
                nodeProperties.insert(make_pair(L"a", L"0"));

                plb.UpdateNode(CreateNodeDescription(0, L"fd:/0", L"0", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(1, L"fd:/1", L"1", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(2, L"fd:/2", L"2", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(3, L"fd:/3", L"3", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(4, L"fd:/4", L"4", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(5, L"fd:/0", L"1", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(6, L"fd:/1", L"2", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(7, L"fd:/2", L"3", move(nodeProperties)));
                plb.UpdateNode(CreateNodeDescription(8, L"fd:/3", L"4", move(nodeProperties)));
            }

            map<wstring, wstring> nodeProperties;
            nodeProperties.insert(make_pair(L"a", L"1"));

            for (int i = 9; i < nodeCount + 1; i++)
            {
                plb.UpdateNode(CreateNodeDescription(
                    i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), map<wstring, wstring>(nodeProperties)));
            }

            // Force processing of pending updates so that service can be created.
            plb.ProcessPendingUpdatesPeriodicTask();

            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

            // Create one system service
            plb.UpdateService(CreateServiceDescriptionWithConstraint(L"TestService", L"TestType", true,
                L"a==0", CreateMetrics(L"MyPCount/1.0/1/0, MySCount/0.3/1/1")));

            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0,
                CreateReplicas(L"P/8"), 6));

            int fuId = 1;

            int i = 0;
            for (; i < partitionedServiceCount; i++)
            {
                wstring serviceName = wstring(L"PartitionedService") + StringUtility::ToWString(i);
                plb.UpdateService(CreateServiceDescriptionWithConstraint(serviceName, L"TestType", false, L"a==1"));
                for (int j = 0; j < partitionCount; j++)
                {
                    plb.UpdateFailoverUnit(FailoverUnitDescription(
                        CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L""), instanceCount));
                }
            }

            Stopwatch timer;

            printf("Starting...\n");

            timer.Start();

            plb.Refresh(Stopwatch::Now());

            timer.Stop();

            printf("Ended.\n\n");

            printf("Total time taken: %lld\n", timer.ElapsedMilliseconds);
        }

        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(StatelessServicesHugePlacementBatchWithFdUdTest)
    {
        // Measuring the time for the INITIAL WALK only - placement random walk and simulated annealing are turned OFF.
        // Placing 1 stateless service with PerfTestNumberOfPartitions partitions, where every partition has PerfTestNumberOfInstances instances.
        // Number of nodes is PerfTestNumberOfNodes.
        // PLB active constrains:
        //   - FaultDomain
        //   - UpgradeDomain
        wstring testName = L"StatelessServicesHugePlacementBatchWithFdUdTest";
        size_t numIterations = 1;

        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        PLBConfigScopeChange(PlacementSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(FastBalancingSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));

        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, 0);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 0);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 0);

        PLBConfigScopeChange(IsTestMode, bool, false);
        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        const int nodeCount = PLBConfig::GetConfig().PerfTestNumberOfNodes; // 3000;
        const int partitionedServiceCount = 1;
        const int partitionCount = PLBConfig::GetConfig().PerfTestNumberOfPartitions; // 100000;
        const int instanceCount = PLBConfig::GetConfig().PerfTestNumberOfInstances; // 10
        const int fdCount = 10;
        const int udCount = 10;

        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);

            fm_->Load();
            PlacementAndLoadBalancing & plb = fm_->PLB;

            for (int i = 0; i < nodeCount + 1; i++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                    i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), L""));
            }

            // Force processing of pending updates so that service can be created.
            plb.ProcessPendingUpdatesPeriodicTask();

            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

            int fuId = 0;

            int i = 0;
            for (; i < partitionedServiceCount; i++)
            {
                wstring serviceName = wstring(L"PartitionedService") + StringUtility::ToWString(i);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false));
                for (int j = 0; j < partitionCount; j++)
                {
                    plb.UpdateFailoverUnit(FailoverUnitDescription(
                        CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L""), instanceCount));
                }
            }

            Stopwatch timer;

            printf("Starting...\n");

            timer.Start();

            plb.Refresh(Stopwatch::Now());

            timer.Stop();

            printf("Ended.\n\n");

            printf("Total time taken: %lld\n", timer.ElapsedMilliseconds);
        }

        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(BatchPlacementTimeUpperBoundTest)
    {
        // This test is to verify the placement time isn't be much greater than the configed placement timeout

        wstring testName = L"BatchPlacementTimeUpperBoundTest";
        size_t numIterations = 1;

        Stopwatch counter;
        counter.Start(); 
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        PLBConfigScopeChange(PlacementSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(FastBalancingSearchTimeout, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(PlacementReplicaCountPerBatch, int, 1000);


        PLBConfigScopeChange(IsTestMode, bool, false);
        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        const int nodeCount = PLBConfig::GetConfig().PerfTestNumberOfNodes / 10; // 300;
        const int partitionedServiceCount = 1;
        const int partitionCount = PLBConfig::GetConfig().PlacementReplicaCountPerBatch; // 1000;
        const int instanceCount = 2; // To make sure batch placement is used in the test
        const int fdCount = 10;
        const int udCount = 10;

        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);

            fm_->Load();
            PlacementAndLoadBalancing & plb = fm_->PLB;

            for (int i = 0; i < nodeCount + 1; i++)
            {
                plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                    i, wformatString("{0}", i % fdCount), wformatString("{0}", i % udCount), L""));
            }

            plb.ProcessPendingUpdatesPeriodicTask();

            plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

            int fuId = 0;

            int i = 0;
            for (; i < partitionedServiceCount; i++)
            {
                wstring serviceName = wstring(L"PartitionedService") + StringUtility::ToWString(i);
                plb.UpdateService(CreateServiceDescription(serviceName, L"TestType", false));
                for (int j = 0; j < partitionCount; j++)
                {
                    plb.UpdateFailoverUnit(FailoverUnitDescription(
                        CreateGuid(fuId++), wstring(serviceName), 0, CreateReplicas(L""), instanceCount));
                }
            }

            Stopwatch timer;

            printf("Starting...\n");

            timer.Start();

            plb.Refresh(Stopwatch::Now());

            timer.Stop();

            printf("Ended.\n\n");

            printf("Total time taken: %lld\n", timer.ElapsedMilliseconds);

            Common::TimeSpan placementTimeout = PLBConfig::GetConfig().PlacementSearchTimeout;

            if (timer.ElapsedMilliseconds > placementTimeout.TotalMilliseconds() * 2)
            {
                VERIFY_FAIL(L"Run time is longer than placement timeout in BatchPlacementTimeUpperBoundTest");
            }

        }

        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(SeaBreezePlacementPerfTest)
    {
        wstring testName = L"SeaBreezePlacementPerfTest";
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        PlacementPerfTestRunner(1500,
            5000,
            5,
            5,
            false,
            false,
            RandomDistribution::Enum::Exponential,
            false,
            64,
            2300,
            RandomDistribution::Enum::Exponential,
            false,
            false,
            true);

        PlacementPerfTestRunner(1500,
            5000,
            5,
            5,
            true,
            true,
            RandomDistribution::Enum::Exponential,
            true,
            64,
            2300,
            RandomDistribution::Enum::Exponential,
            false,
            true,
            true);
    }

    BOOST_AUTO_TEST_CASE(PlacementWithNodeCapacitiesAndScopedDefragScaleTest)
    {
        wstring testName = L"PlacementWithNodeCapacitiesAndScopedDefragScaleTest";
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);

        int    fdUdCounts[] = {   1,    5,    10,    15 };
        int    nodeCounts[] = {  10,  100,   500,  1500 };
        int serviceCounts[] = { 320, 3200, 10000, 20000 };

        size_t nodeCountsSize = sizeof(nodeCounts) / sizeof(int);
        size_t serviceCountsSize = sizeof(serviceCounts) / sizeof(int);
        size_t fdUdCountsSize = sizeof(fdUdCounts) / sizeof(int);

        TESTASSERT_IF(nodeCountsSize != serviceCountsSize, "Size mismatch between nodeCounts and serviceCounts.");
        TESTASSERT_IF(nodeCountsSize != fdUdCountsSize, "Size mismatch between nodeCounts and fdUdCounts.");

        for (size_t i = 0; i < nodeCountsSize; i++)
        {
            PlacementPerfTestRunner(nodeCounts[i],
                serviceCounts[i],
                fdUdCounts[i],
                fdUdCounts[i],
                false,
                false,
                RandomDistribution::Enum::AllOnes,
                true,
                64,
                2300,
                RandomDistribution::Enum::AllOnes,
                false,
                true);
        }
    }

    BOOST_AUTO_TEST_CASE(MemoryIncreaseDuringPLBRefreshTest)
    {
        // Testing regression, made in PLB Diagnostics, where PLB increased memory usage with every PLB Refresh, if there were metrics with capacity but without service domain.
        // New domain would be created for such metrics with every PLB Refresh, and added to PLB Diagnostics domain map. This increases size of the map and PLB increase memory usage with time.
        // There could be memory difference between 2 PLB Refreshes, due to pending actions..., but constant increase of memory usage was caught. Without fix there were about 4MBs constant increase
        // of process memory usage after 1800 PLB refreshes. To allow smaller spikes, only 2MBs are allowed in this test.
        wstring testName = L"MemoryIncreaseDuringPLBRefreshTest";
        Trace.WriteInfo("PLBPerformanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring RGmetric1 = *ServiceModel::Constants::SystemMetricNameCpuCores;
        std::wstring RGmetric2 = *ServiceModel::Constants::SystemMetricNameMemoryInMB;
        std::wstring metric = L"metric";
        for (int i = 0; i < 20; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithResources(i, 2, 2048, map<wstring, wstring>(), wformatString("{0}/100000", metric), L"", L"dc1/rack1"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();


        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<Federation::NodeId>()));

        int fuIndex = 0;
        for (int i = 0; i < 1; i++)
        {
            plb.UpdateService(CreateServiceDescription(wformatString("Service_{0}", i), L"TestType", true, CreateMetrics(L"metric/0.0/2/0")));
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wformatString("Service_{0}", i), 0, CreateReplicas(L"P/{0}", i), 0));
        }
        bool result;
        PROCESS_MEMORY_COUNTERS memCounter;
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 1);
        PLBConfigScopeChange(MinPlacementInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(ProcessPendingUpdatesInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        uint64 startMemory = 0;
        uint64 endMemory = 0;

        for (int i = 0; i <= 1800; i++)
        {
            plb.ProcessPendingUpdatesPeriodicTask();
            fm_->RefreshPLB(Stopwatch::Now());
            result = GetProcessMemoryInfo(GetCurrentProcess(),
                &memCounter,
                sizeof(memCounter));
            if (result && i == 100)
            {
                startMemory = memCounter.WorkingSetSize;
            }
            if (result && i == 1800)
            {
                endMemory = memCounter.WorkingSetSize;
            }
        }
        if (result)
        {
            VERIFY_IS_TRUE(endMemory <= startMemory || endMemory - startMemory < 2000000); //~2MB increase
        }
    }
    /*
    BOOST_AUTO_TEST_CASE(BalancingStressTest)
    {
        StressTestHelper(L"BalancingStressTest", 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingStressTestWithNoApplication)
    {
        StressTestHelper(L"BalancingStressTestWithNoApplication", 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingStressTestWithManyApplications)
    {
        StressTestHelper(L"BalancingStressTestWithManyApplications", 2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodes)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodes", false, false);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWService)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWService", true, false);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWServiceAndConstraints)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWServiceAndConstraints", true, true);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithConstraints)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithConstraints", false, true);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWServiceAndConstraints1App)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWServiceAndConstraints", true, true, 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWServiceAndConstraints2Apps)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWServiceAndConstraints", true, true, 2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWServiceAndConstraints10Apps)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWServiceAndConstraints", true, true, 10);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWService1App)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWService", true, false, 1);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWService2Apps)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWService", true, false, 2);
    }

    BOOST_AUTO_TEST_CASE(BalancingWithManyNodesWithCWService10Apps)
    {
        BalancingWithManyNodesHelper(L"BalancingWithManyNodesWithCWService", true, false, 10);
    }
    */


    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBPerformance::StaticTestHelper(std::wstring testName,
        size_t numNodes,                // Total number of nodes
        size_t numServices,             // Total number of services
        size_t numPartitionsPerService, // Number of partitions per service
        size_t numReplicasPerService,   // Number of replicas per partition
        bool useApplication,            // Every service in its own application
        size_t numIterations            // How many iterations of refresh
    )
    {
        Stopwatch counter;
        counter.Start();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestStart {0}", testName);
        // Use test helper! (TODO: Check if we want to do this!)
        auto & plb = fm_->PLBTestHelper;

        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        for (int i = 0; i < numNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithResources(i, i % 3 + 1, 1024 * (i + 1), map<wstring, wstring>(), L"Random/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyServiceType"), set<NodeId>()));

        wstring serviceNamePrefix = wformatString("{0}Service", testName);

        plb.ProcessPendingUpdatesPeriodicTask();

        size_t partitionId = 0;
        size_t serviceId = 0;
        for (size_t serviceNo = 0; serviceNo < numServices; ++serviceNo)
        {
            StaticHelperCreateService(
                testName,
                serviceNamePrefix,
                plb,
                serviceId,
                partitionId,
                numNodes,
                numPartitionsPerService,
                numReplicasPerService,
                useApplication);
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // Add a node and then remove it so that we force full closure for constraint check.
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(static_cast<int>(numNodes), 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Node down!
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(static_cast<int>(numNodes), 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now() + PLBConfig::GetConfig().BalancingDelayAfterNewNode;

        for (int iteration = 0; iteration < numIterations; iteration++)
        {
            Trace.WriteInfo("PLBPerformanceTestSource", "TestIteration {0} {1}", testName, iteration);
            fm_->Clear();

            // Just add the service to have a change (to avoid throttle).
            StaticHelperCreateService(
                testName,
                serviceNamePrefix,
                plb,
                serviceId,
                partitionId,
                numNodes,
                0,
                0,
                useApplication);

            plb.Refresh(now);

            // Go 10 days ahead
            now += TimeSpan::FromSeconds(10 * 24 * 60 * 60);

            // TODO: Reenable when 6437297 is done.
            //std::wcout << testName << ",BeginRefresh, Iteration " << iteration << ", " << plb.BeginRefreshTime << std::endl;
        }
        counter.Stop();
        Trace.WriteInfo("PLBPerformanceTestSource", "TestEnd {0} {1}", testName, counter.ElapsedMilliseconds);
    }

    void TestPLBPerformance::StaticHelperCreateService(
        wstring const& testName,
        wstring const& serviceNamePrefix,
        PlacementAndLoadBalancingTestHelper & plb,
        size_t & serviceId,
        size_t & partitionId,
        size_t numNodes,
        size_t numPartitionsPerService,
        size_t numReplicasPerPartition,
        bool useApplication)
    {
        wstring applicationName = L"";
        ServiceModel::ServicePackageIdentifier spId1;
        if (useApplication)
        {
            // Create application (if needed)
            applicationName = wformatString("{0}Application_{1}", testName, serviceId);

            vector<ServicePackageDescription> packages;
            packages.push_back(CreateServicePackageDescription(L"ServicePackageName", L"", applicationName, 2, 128, spId1));

            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                wstring(applicationName),
                static_cast<int>(numNodes),   // Scaleout never violated!
                map<std::wstring, ApplicationCapacitiesDescription>(),
                packages));
        }

        // Create the service itself
        wstring serviceName = wformatString("{0}_{1}", serviceNamePrefix, serviceId);
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            wstring(serviceName),
            wstring(L"MyServiceType"),
            wstring(applicationName),    // ApplicationName
            true,
            vector<ServiceMetric>(),
            ServiceModel::ServicePackageIdentifier(spId1)));

        for (int partitionNo = 0; partitionNo < numPartitionsPerService; partitionNo++)
        {
            size_t node = partitionId % numNodes;
            wstring replicaList = wformatString("P/{0}", node);
            for (size_t rIndex = 0; rIndex < numReplicasPerPartition - 1; ++rIndex)
            {
                node = (node + 1) % numNodes;
                replicaList = wformatString("{0},S/{1}", replicaList, node);
            }
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(static_cast<int>(partitionId)),
                wstring(serviceName),
                1,
                CreateReplicas(replicaList),
                0));
            partitionId++;
        }
        serviceId++;
    }

    void TestPLBPerformance::BalancingWithManyNodesHelper(std::wstring testName, bool onEveryNode, bool placementConstraints, int createApplication)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::wstring placementConstraintString = placementConstraints ? L"NodeType==Tenant" : L"";

        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        int numNodes = 1000;
        int numApplications = 100;
        int numServicesPerApplication = 16;
        int partitionId = 0;

        for (int i = 0; i < numNodes; i++)
        {
            map<wstring, wstring> nodeProperties;
            if (placementConstraints)
            {
                nodeProperties.insert(make_pair(L"NodeType", L"Tenant"));
            }
            plb.UpdateNode(CreateNodeDescriptionWithResources(i, i % 3 + 1, 1024 * (i + 1), move(nodeProperties), L"Random/100"));
        }

        if (placementConstraints)
        {
            // Add 7 more nodes that are WF type
            int otherNodeStartIndex = numNodes + 100;
            for (int i = otherNodeStartIndex; i < otherNodeStartIndex + 7; ++i)
            {
                map<wstring, wstring> nodeProperties;
                nodeProperties.insert(make_pair(L"NodeType", L"WF"));
                plb.UpdateNode(CreateNodeDescriptionWithResources(i, i % 3 + 1, 1024 * (i + 1), move(nodeProperties), L"Random/100"));
            }
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyServiceType"), set<NodeId>()));

        wstring serviceNamePrefix = wformatString("{0}Service", testName);

        plb.ProcessPendingUpdatesPeriodicTask();

        int appNumber = 0;
        int serviceNumber = 0;

        // One app group per service, no constraints ever violated
        for (; appNumber < numApplications; appNumber++)
        {
            wstring applicationName = wformatString("fabric:/{0}Application_{1}", testName, appNumber);
            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                wstring(applicationName),
                0,   // No Application Capacity
                map<std::wstring, ApplicationCapacitiesDescription>()));
            for (int s = 0; s < numServicesPerApplication; ++s)
            {
                wstring serviceName = wformatString("fabric:/{0}/{1}_{2}", applicationName, serviceNamePrefix, serviceNumber);

                plb.UpdateService(ServiceDescription(
                    wstring(serviceName),
                    wstring(L"MyServiceType"),
                    wstring(applicationName),
                    true,                               //bool isStateful,
                    wstring(placementConstraintString), //placementConstraints,
                    wstring(),                          //affinitizedService,
                    true,                               //alignedAffinity,
                    vector<ServiceMetric>(),
                    FABRIC_MOVE_COST_LOW,
                    false));                    //bool onEveryNode,

                int node = partitionId % numNodes;
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(partitionId++),
                    wstring(serviceName),
                    1,
                    CreateReplicas(wformatString("P/{0}", node)), 0));
                serviceNumber++;
            }
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        if (onEveryNode)
        {
            int numClusterWideServices = 4;
            int clusterWideStart = numApplications * numServicesPerApplication + 1000;
            for (int clS = 0; clS < numClusterWideServices; ++clS)
            {
                int cwServiceNumber = clusterWideStart + clS;
                wstring applicationName = wformatString("fabric:/{0}Application_{1}", testName, cwServiceNumber);
                wstring serviceName = wformatString("fabric:/{0}/{1}_{2}", applicationName, serviceNamePrefix, 0);

                plb.UpdateService(ServiceDescription(
                    wstring(serviceName),
                    wstring(L"MyServiceType"),
                    wstring(applicationName),
                    false,
                    wstring(placementConstraintString),
                    wstring(L""),
                    true, vector<ServiceMetric>(),
                    FABRIC_MOVE_COST_LOW,
                    true));

                vector<ReplicaDescription> replicaVector;
                for (int replicaNo = 0; replicaNo < numNodes; ++replicaNo)
                {
                    replicaVector.push_back(ReplicaDescription(
                        CreateNodeInstance(replicaNo, 0),
                        ReplicaRole::Secondary,
                        Reliability::ReplicaStates::Ready,
                        true));
                }

                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(partitionId++),
                    wstring(serviceName),
                    0,
                    move(replicaVector),
                    INT_MAX));
            }
        }

        // Add a node and then remove it so that we force full closure for constraint check.
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(numNodes, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Node down!
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(numNodes, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now() + PLBConfig::GetConfig().BalancingDelayAfterNewNode;

        for (int iteration = 0; iteration < 20; iteration++)
        {
            fm_->Clear();

            if (createApplication > 0)
            {
                for (int a = 0; a < createApplication; a++)
                {
                    wstring applicationName = wformatString("fabric:/{0}Application_{1}", testName, appNumber);
                    plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                        wstring(applicationName),
                        0,   // No Application Capacity
                        map<std::wstring, ApplicationCapacitiesDescription>()));
                    for (int s = 0; s < numServicesPerApplication; ++s)
                    {
                        wstring serviceName = wformatString("fabric:/{0}/{1}_{2}", applicationName, serviceNamePrefix, serviceNumber);

                        plb.UpdateService(ServiceDescription(
                            wstring(serviceName),
                            wstring(L"MyServiceType"),
                            wstring(applicationName),
                            true,                               //bool isStateful,
                            wstring(placementConstraintString), //placementConstraints,
                            wstring(),                          //affinitizedService,
                            true,                               //alignedAffinity,
                            vector<ServiceMetric>(),
                            FABRIC_MOVE_COST_LOW,
                            false));                    //bool onEveryNode,

                        int node = partitionId % numNodes;
                        plb.UpdateFailoverUnit(FailoverUnitDescription(
                            CreateGuid(partitionId++),
                            wstring(serviceName),
                            1,
                            CreateReplicas(wformatString("P/{0}", node)), 0));
                        serviceNumber++;
                    }
                    appNumber++;
                }
            }
            else
            {
                plb.UpdateService(CreateServiceDescription(wformatString("{0}{1}", serviceNamePrefix, serviceNumber), wstring(L"MyServiceType"), true));
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    CreateGuid(partitionId++),
                    wformatString("{0}{1}", serviceNamePrefix, serviceNumber),
                    1,
                    CreateReplicas(wformatString("")), 0));
                serviceNumber++;
            }

            plb.Refresh(now);

            // Go 10 days ahead
            now += TimeSpan::FromSeconds(10 * 24 * 60 * 60);

            // TODO: Reenable when 6437297 is done.
            //std::wcout << testName << ",BeginRefresh, Iteration " << iteration << ", " << plb.BeginRefreshTime << std::endl;
        }
    }

    // 0 - No changes
    // 1 - No application (app name == L"")
    // 2 - Many app groups (scaleout == 1000) - no violations ever
    void TestPLBPerformance::StressTestHelper(std::wstring testName, uint type)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        int numNodes = 20;
        int numServices = 275000;

        int numPartitions = 1;

        for (int i = 0; i < numNodes; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithResources(i, i % 3 + 1, 1024 * (i + 1), map<wstring, wstring>(), L"Random/100"));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyServiceType"), set<NodeId>()));

        wstring serviceNamePrefix = wformatString("{0}Service", testName);

        plb.ProcessPendingUpdatesPeriodicTask();

        int serviceNumber = 0;
        if (type == 0)
        {
            // Everything default!
            for (; serviceNumber < numServices; serviceNumber++)
            {
                plb.UpdateService(CreateServiceDescription(wformatString("{0}_{1}", serviceNamePrefix, serviceNumber), wstring(L"MyServiceType"), true));
            }
        }
        else if (type == 1)
        {
            // No application for service (turn app groups off effectively)
            for (; serviceNumber < numServices; serviceNumber++)
            {
                plb.UpdateService(CreateServiceDescriptionWithApplication(
                    wformatString("{0}_{1}", serviceNamePrefix, serviceNumber),
                    wstring(L"MyServiceType"),
                    L"",    // ApplicationName
                    true,
                    vector<ServiceMetric>()));
            }
        }
        else if (type == 2)
        {
            // One app group per service, no constraints ever violated
            for (; serviceNumber < numServices; serviceNumber++)
            {
                wstring applicationName = wformatString("{0}Application_{1}", testName, serviceNumber);
                plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                    wstring(applicationName),
                    1000,   // Scaleout never violated!
                    map<std::wstring, ApplicationCapacitiesDescription>()));

                plb.UpdateService(CreateServiceDescriptionWithApplication(
                    wformatString("{0}_{1}", serviceNamePrefix, serviceNumber),
                    wstring(L"MyServiceType"),
                    wstring(applicationName),    // ApplicationName
                    true,
                    vector<ServiceMetric>()));
            }
        }
        else
        {
            VERIFY_FAIL(L"Never, ever, do this!");
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        int partitionId = 0;
        for (int serviceNo = 0; serviceNo < numServices; serviceNo++)
        {
            for (int partitionNo = 0; partitionNo < numPartitions; partitionNo++)
            {
                int node = partitionId % numNodes;
                plb.UpdateFailoverUnit(FailoverUnitDescription(
                    //Guid::NewGuid(),
                    CreateGuid(partitionId++),
                    //wformatString("{0}", serviceNo),
                    wformatString("{0}_{1}", serviceNamePrefix, serviceNo),
                    1,
                    CreateReplicas(wformatString("P/{0},S/{1},S/{2}", node, (node + 1) % numNodes, (node + 2) % numNodes)), 0));
            }
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        // Add a node and then remove it so that we force full closure for constraint check.
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(numNodes, 0),
            true,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Node down!
        plb.UpdateNode(NodeDescription(
            CreateNodeInstance(numNodes, 0),
            false,
            Reliability::NodeDeactivationIntent::Enum::None,
            Reliability::NodeDeactivationStatus::Enum::None,
            std::map<wstring, wstring>(),
            NodeDescription::DomainId(),
            std::wstring(),
            map<wstring, uint>(),
            map<wstring, uint>()));
        plb.ProcessPendingUpdatesPeriodicTask();

        StopwatchTime now = Stopwatch::Now() + PLBConfig::GetConfig().BalancingDelayAfterNewNode;

        for (int iteration = 0; iteration < 20; iteration++)
        {
            fm_->Clear();

            plb.UpdateService(CreateServiceDescription(wformatString("{0}{1}", serviceNamePrefix, serviceNumber), wstring(L"MyServiceType"), true));
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(partitionId++),
                wformatString("{0}{1}", serviceNamePrefix, serviceNumber),
                1,
                CreateReplicas(wformatString("")), 0));
            serviceNumber++;

            plb.Refresh(now);

            // Go 10 days ahead
            now += TimeSpan::FromSeconds(10 * 24 * 60 * 60);
        }
    }

    int64 TestPLBPerformance::PlacementPerfTestRunner(int nodeCount,
        int containerCount,
        int fdCount,
        int udCount,
        bool reserveNodesForSystemServices,
        bool useMoveCost,
        RandomDistribution::Enum moveCostDistribution,
        bool useNodeCapacities,
        uint32_t cpuCapacity,
        uint32_t memoryCapacity,
        RandomDistribution::Enum loadDistribution,
        bool useRandomWalking,
        bool useDefrag,
        bool verboseOutput)
    {
        fm_->Load(); // Destruct and renew PLB object
        PlacementAndLoadBalancing & plb = fm_->PLB;

        int seed = 224;

        PLBConfigScopeChange(InitialRandomSeed, int, seed);

        Common::TimeSpan placementSearchTimeout = Common::TimeSpan::FromSeconds(0);
        int placementSearchIterationsPerRound = 0;
        int simulatedAnnealingIterationsPerRound = 0;
        int maxSimulatedAnnealingIterations = 0;

        if (useRandomWalking)
        {
            placementSearchTimeout = TimeSpan::MaxValue;
            placementSearchIterationsPerRound = 1000;
            simulatedAnnealingIterationsPerRound = 1000;
            maxSimulatedAnnealingIterations = 20;
        }

        PLBConfigScopeChange(PlacementSearchTimeout, Common::TimeSpan, placementSearchTimeout);
        PLBConfigScopeChange(PlacementSearchIterationsPerRound, int, placementSearchIterationsPerRound);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, simulatedAnnealingIterationsPerRound);
        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, maxSimulatedAnnealingIterations);

        PLBConfigScopeChange(IsTestMode, bool, false);
        PLBConfigScopeChange(UseRGInBoost, bool, true);
        PLBConfigScopeChange(PrintRefreshTimers, bool, true);

        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);

        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;

        if (useDefrag)
        {
            int freeNodesTarget = static_cast<int>(0.01 * nodeCount);
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(
                make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores, freeNodesTarget));
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(
                make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB, freeNodesTarget));
        }
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyDoubleValueMap globalWeights;
        globalWeights.insert(make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores, 1.0));
        globalWeights.insert(make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 1.0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, globalWeights);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores, useDefrag));
        defragMetricMap.insert(make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB, useDefrag));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores, useDefrag));
        scopedDefragMetricMap.insert(make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB, useDefrag));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        PLBConfig::KeyDoubleValueMap scopedDefragEmptyWeight;
        scopedDefragEmptyWeight.insert(make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores, 1.0));
        scopedDefragEmptyWeight.insert(make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 1.0));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, scopedDefragEmptyWeight);

        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(*ServiceModel::Constants::SystemMetricNameCpuCores,
            Metric::DefragDistributionType::NumberOfEmptyNodes));
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(*ServiceModel::Constants::SystemMetricNameMemoryInMB,
            Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        int nodesPerFdUd = nodeCount / (fdCount * udCount);
        int remainingNodes = nodeCount % (fdCount * udCount);

        for (int i = 0, nodeId = 0; i < fdCount; i++)
        {
            for (int j = 0; j < udCount; j++)
            {
                int totalNodesPerFdUd = nodesPerFdUd;
                if (remainingNodes > 0)
                {
                    remainingNodes--;
                    totalNodesPerFdUd++;
                }
                for (int k = 0; k < totalNodesPerFdUd; k++)
                {
                    map<wstring, wstring> nodeProperties;
                    if (reserveNodesForSystemServices)
                    {
                        nodeProperties.insert(make_pair(L"NodeType", L"User"));
                    }
                    if (useNodeCapacities)
                    {
                        plb.UpdateNode(CreateNodeDescriptionWithResources(
                            nodeId++,
                            cpuCapacity,
                            memoryCapacity,
                            move(nodeProperties),
                            L"",
                            wformatString("{0}", j),
                            wformatString("{0}", i)));
                    }
                    else
                    {
                        plb.UpdateNode(CreateNodeDescription(
                            nodeId++,
                            wformatString("{0}", i),
                            wformatString("{0}", j),
                            move(nodeProperties)));
                    }
                }
            }
        }

        wstring constraintString = L"";
        if (reserveNodesForSystemServices)
        {
            // We are placing only on User nodes
            constraintString = L"NodeType==User";
            for (int i = 0; i < 12; i++)
            {
                map<wstring, wstring> nodeProperties;
                nodeProperties.insert(make_pair(L"NodeType", L"System"));
                if (useNodeCapacities)
                {
                    plb.UpdateNode(CreateNodeDescriptionWithResources(
                        nodeCount + i + 1,
                        cpuCapacity,
                        memoryCapacity,
                        move(nodeProperties),
                        L"",
                        wformatString("{0}", i % udCount),
                        wformatString("{0}", i % fdCount)));
                }
                else
                {
                    plb.UpdateNode(CreateNodeDescription(
                        nodeCount + i + 1,
                        wformatString("{0}", i % fdCount),
                        wformatString("{0}", i % udCount),
                        move(nodeProperties)));
                }
            }
        }

        plb.ProcessPendingUpdatesPeriodicTask();
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Generating ServicePackage loads
        int64 cpuLoad = 0;
        int64 memoryLoad = 0;

        RandomDistribution generator(seed);

        DistributionMap distributionCpu =
            generator.GenerateDistribution(containerCount, loadDistribution, 1, cpuCapacity);

        VERIFY_ARE_EQUAL((int)generator.GetNumberOfSamples(distributionCpu), containerCount);

        DistributionMap distributionMemory =
            generator.GenerateDistribution(containerCount, loadDistribution, 1, memoryCapacity);

        VERIFY_ARE_EQUAL((int)generator.GetNumberOfSamples(distributionMemory), containerCount);

        int ftId = 0;
        vector<pair<int, int> > flattenMap(containerCount);
        for (auto it : distributionCpu)
        {
            for (uint i = 0; i < it.second; i++)
            {
                cpuLoad += it.first;
                flattenMap[ftId++].first = it.first;
            }
        }
        ftId = 0;
        for (auto it : distributionMemory)
        {
            for (uint i = 0; i < it.second; i++)
            {
                memoryLoad += it.first;
                flattenMap[ftId++].second = it.first;
            }
        }

        wstring applicationType = wstring(L"AppTestType");

        // SeaBreeze container is a stateless service with one replica
        for (int i = 0; i < containerCount; i++)
        {
            wstring serviceNameBase = wstring(L"Container");
            wstring applicationName = wstring(L"Application") + StringUtility::ToWString(i);
            wstring servicePackageName = wstring(L"ServicePackage") + StringUtility::ToWString(i);

            vector<ServicePackageDescription> packages;
            ServiceModel::ServicePackageIdentifier spId;

            packages.push_back(CreateServicePackageDescription(
                servicePackageName,
                applicationType,
                applicationName,
                flattenMap[i].first,
                flattenMap[i].second,
                spId));

            plb.UpdateApplication(CreateApplicationWithServicePackages(applicationType, applicationName, packages));

            plb.UpdateService(ServiceDescription(
                move(serviceNameBase + StringUtility::ToWString(i)),
                L"TestType",
                wformatString(L"fabric:/{0}", applicationName),
                false,
                move(constraintString),
                wstring(L""),
                true,
                move(CreateMetrics(L"")),
                FABRIC_MOVE_COST_LOW,
                false,
                0,
                0,
                true,
                ServiceModel::ServicePackageIdentifier(spId),
                ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i), serviceNameBase + StringUtility::ToWString(i), 0, CreateReplicas(L""), 1));
        }

        if (useMoveCost)
        {
            DistributionMap distribution =
                generator.GenerateDistribution(containerCount, moveCostDistribution, 1, 3);

            VERIFY_ARE_EQUAL((int)generator.GetNumberOfSamples(distribution), containerCount);

            ftId = 0;
            for (auto it : distribution)
            {
                for (uint i = 0; i < it.second; i++)
                {
                    wstring serviceName = wstring(L"Container") + StringUtility::ToWString(ftId);
                    std::map<Federation::NodeId, uint> secondaryMoveCost;
                    plb.UpdateLoadOrMoveCost(
                        CreateLoadOrMoveCost(ftId++,
                            serviceName,
                            *Reliability::LoadBalancingComponent::Constants::MoveCostMetricName,
                            it.first,
                            secondaryMoveCost));
                }
            }
        }

        wcout << L"Nodes to place = " << nodeCount << endl;
        wcout << L"Services to place = " << containerCount << endl;
        wcout << L"FD count = " << fdCount << L" | UD count = " << udCount << endl;
        if (verboseOutput)
        {
            if (useNodeCapacities)
            {
                wcout << "Node CPU capacity = " << cpuCapacity << endl;
                wcout << "Node Memory capacity = " << memoryCapacity << endl;
                wcout << "Node CPU utilization = " << 100. * cpuLoad / (cpuCapacity * nodeCount) << "%" << endl;
                wcout << "Node Memory utilization = " << 100. * memoryLoad / (memoryCapacity * memoryCapacity) << "%" << endl;
            }
            wcout << "Using scope defrag in placement = " << useDefrag << endl;
            wcout << L"Using move cost = " << useMoveCost << endl;
            wcout << L"Using constraint for system nodes = " << reserveNodesForSystemServices << endl;
            wcout << L"Nodes have capacities = " << useNodeCapacities << endl;
            wcout << L"Using random walking = " << useRandomWalking << endl;
        }

        Stopwatch RefreshTimer;

        RefreshTimer.Start();
        plb.Refresh(Stopwatch::Now());
        RefreshTimer.Stop();

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(actionList.size(), containerCount);

        fm_->Load(); // Destruct and renew PLB object
        return RefreshTimer.ElapsedMilliseconds;
    }

    void TestPLBPerformance::SetupAllFeaturesPerformanceTest(std::wstring const& testName,
        size_t numNodes,
        size_t numberOfApplications,
        size_t applicationMaxNodes,
        size_t numChildServicesPerApplication,
        size_t numPartitionsPerChildService,
        bool useDifferentLoadForPrimary,
        bool includeInFUMap)
    {
        // Two node types: left and right
        //   Each node has capacity defined for two metrics
        // Multiple applications:
        //   Number of services per application.
        //   Number of paritions per service.
        // Affinity related services:
        //   Number
        auto & plb = fm_->PLB;

        Stopwatch counter;
        counter.Start();

        for (int nodeId = 0; nodeId < numNodes; nodeId++)
        {
            wstring faultDomain = wformatString("fd:/rack/switch{0}", nodeId);
            wstring upgradeDomain = wformatString("{0}", nodeId);

            // Left node (node ID: nodeId)
            map<wstring, wstring> leftNodeProperties;
            leftNodeProperties.insert(make_pair(L"Direction", L"Left"));

            plb.UpdateNode(CreateNodeDescriptionWithResources(
                nodeId,
                nodeId % 3 + 1,
                1024 * (nodeId + 1),
                move(leftNodeProperties),
                L"ForwardMetric/1000000",
                upgradeDomain,
                faultDomain));
            // Right node (node ID: nodeId + numNodes)
            map<wstring, wstring> rightNodeProperties;
            rightNodeProperties.insert(make_pair(L"Direction", L"Right"));

            plb.UpdateNode(CreateNodeDescriptionWithResources(
                nodeId + static_cast<int>(numNodes),
                (nodeId + static_cast<int>(numNodes)) % 3 + 1,
                1024 * (nodeId + static_cast<int>(numNodes) + 1),
                move(rightNodeProperties), L"ForwardMetric/1000000",
                upgradeDomain, faultDomain));
        }

        // Blocklisted left node
        map<wstring, wstring> leftNodeProperties;
        leftNodeProperties.insert(make_pair(L"Direction", L"Left"));

        plb.UpdateNode(CreateNodeDescriptionWithResources(
            2 * static_cast<int>(numNodes),
            (2 * static_cast<int>(numNodes)) % 3 + 1,
            1024 * (2 * static_cast<int>(numNodes) + 1),
            move(leftNodeProperties),
            L"ForwardMetric/1000000"));

        // Right node (node ID: nodeId + numNodes)
        map<wstring, wstring> rightNodeProperties;
        rightNodeProperties.insert(make_pair(L"Direction", L"Right"));

        plb.UpdateNode(CreateNodeDescriptionWithResources(
            2 * static_cast<int>(numNodes) + 1,
            (2 * static_cast<int>(numNodes) + 1) % 3 + 1,
            1024 * (2 * static_cast<int>(numNodes) + 2),
            move(rightNodeProperties),
            L"ForwardMetric/1000000"));

        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(2 * static_cast<int>(numNodes) + 1));
        blockList.insert(CreateNodeId(2 * static_cast<int>(numNodes)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"MyServiceType"), move(blockList)));

        int direction = 0;
        // Total: 10M, Per node: 1M, Reserved: 2
        std::wstring applicationCapacityString = L"ForwardMetric/10000000/1000000/2";
        std::wstring serviceMetricString = useDifferentLoadForPrimary ? L"ForwardMetric/1.0/2/1" : L"ForwardMetric/1.0/1/1";
        int applicationIndex = 0;
        int serviceIndex = 0;
        int partitionIndex = 0;
        while (direction < 2)
        {
            std::wstring placementConstraints = direction == 0 ? L"Direction == Left" : L"Direction == Right";
            std::wstring directionString = direction == 0 ? L"Left" : L"Right";
            int startNode = direction == 0 ? 0 : static_cast<int>(numNodes);
            int applicationStartNode = 0;
            for (int applicationNo = 0; applicationNo < numberOfApplications; ++applicationNo)
            {
                std::wstring applicationName = wformatString("fabric:/{0}_Application_{1}_{2}", testName, directionString, applicationIndex);

                vector<ServicePackageDescription> packages;
                ServiceModel::ServicePackageIdentifier spId;
                packages.push_back(CreateServicePackageDescription(L"ServicePackageName", L"", applicationName, 2, 128, spId));

                // Create the application
                plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                    applicationName,
                    //static_cast<int>(applicationMaxNodes),  // MinimumNodes
                    static_cast<int>(applicationMaxNodes),  // MaximumNodes
                    CreateApplicationCapacities(applicationCapacityString),
                    packages));

                // Calculate where replicas of this application will go
                int node = applicationStartNode;
                wstring replicaPlacementString = wformatString("P/{0}", startNode + node);
                for (int rNo = 0; rNo < applicationMaxNodes - 1; ++rNo)
                {
                    node = (node + 1) % numNodes;
                    replicaPlacementString = wformatString("{0},S/{1}", replicaPlacementString, startNode + node);
                }

                // Create parent service and FT
                wstring parentServiceName = wformatString("{0}/ParentService_{1}", applicationName, serviceIndex);
                plb.UpdateService(ServiceDescription(
                    wstring(parentServiceName),
                    wstring(L"MyServiceType"),
                    wstring(applicationName),
                    true,                               //bool isStateful,
                    wstring(placementConstraints), //placementConstraints,
                    wstring(),                          //affinitizedService,
                    true,                               //alignedAffinity,
                    CreateMetrics(serviceMetricString),
                    FABRIC_MOVE_COST_LOW,
                    false));
                FailoverUnitDescription fuDescription(
                    CreateGuid(partitionIndex++),
                    wstring(parentServiceName),
                    1,
                    CreateReplicas(replicaPlacementString),
                    0);
                if (includeInFUMap)
                {
                    fm_->FuMap.insert(make_pair(fuDescription.FUId, fuDescription));
                }
                plb.UpdateFailoverUnit(move(fuDescription));
                serviceIndex++;

                // Create child services
                for (int csNo = 0; csNo < numChildServicesPerApplication; ++csNo)
                {
                    wstring childServiceName = wformatString("{0}/ChildService_{1}", applicationName, serviceIndex);
                    plb.UpdateService(ServiceDescription(
                        wstring(childServiceName),
                        wstring(L"MyServiceType"),
                        wstring(applicationName),
                        true,                               //bool isStateful,
                        wstring(placementConstraints), //placementConstraints,
                        wstring(parentServiceName),                   //affinitizedService,
                        true,                               //alignedAffinity,
                        CreateMetrics(serviceMetricString),
                        FABRIC_MOVE_COST_LOW,
                        false));
                    serviceIndex++;

                    for (int cpNo = 0; cpNo < numPartitionsPerChildService; ++cpNo)
                    {
                        FailoverUnitDescription fuDesc(
                            CreateGuid(partitionIndex++),
                            wstring(childServiceName),
                            1,
                            CreateReplicas(replicaPlacementString),
                            0);
                        if (includeInFUMap)
                        {
                            fm_->FuMap.insert(make_pair(fuDesc.FUId, fuDesc));
                        }
                        plb.UpdateFailoverUnit(move(fuDesc));
                    }
                }

                applicationIndex++;
                applicationStartNode = (applicationStartNode + static_cast<int>(applicationMaxNodes)) % static_cast<int>(numNodes);
            }
            direction++;
        }   // Left and Right direction

        plb.ProcessPendingUpdatesPeriodicTask();
    }

    bool TestPLBPerformance::ClassSetup()
    {
        Trace.WriteInfo("PLBPerformanceTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBPerformance::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }
}
