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
    using namespace Reliability::LoadBalancingComponent;

    // Represents ServiceModel::ServicePackageResourceGovernanceDescription as it was prior to v6.0.
    // It is used to test if CpuCores will be properly read from the new class.
    class ServicePackageResourceGovernanceDescriptionMockup : public Serialization::FabricSerializable
    {
    public:
        FABRIC_FIELDS_03(IsGoverned, CpuCores, MemoryInMB);

        bool IsGoverned;
        uint CpuCores; 
        uint MemoryInMB;
    };

    class TestResorceGovernance
    {
    protected:
        TestResorceGovernance() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestResorceGovernance()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;

        void VerifyAllApplicationReservedLoadPerNode(wstring const& metricName, int64 expectedReservedLoadUsed, Federation::NodeId nodeId);
        void VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier const& servicePackageIdentifier,
            int exclusiveRegularReplicaCount,
            int sharedRegularReplicaCount,
            int sharedDisappearReplicaCount,
            Federation::NodeId nodeId);
    };

    BOOST_FIXTURE_TEST_SUITE(TestResorceGovernanceTestSuite, TestResorceGovernance)

    BOOST_AUTO_TEST_CASE(RGAppGroupsAppCapacity)
    {
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGAppGroupsAppCapacity";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 2048, map<wstring, wstring>(), L"Random/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 1024, map<wstring, wstring>(), L"Random/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 1, 2048, map<wstring, wstring>(), L"Random/200"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 3, 4096, map<wstring, wstring>(), L"Random/50"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 2, 4096, map<wstring, wstring>(), L"Random/50"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 512, map<wstring, wstring>(), L"Random/50"));

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<ServicePackageDescription> packages1;
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName1", L"AppType1", L"App1", 1, 1024, spId1));
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName2", L"AppType2", L"App2", 0, 1024, spId2));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            4,
            CreateApplicationCapacities(wformatString(L"Random/10000/500/30")),
            packages1,
            L"AppType1"));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            4,
            CreateApplicationCapacities(wformatString(L"Random/10000/40/0")),
            packages2,
            L"AppType2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type2"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"Type1", L"App1", true, CreateMetrics(L""), spId1, false));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"Type1", L"App1", true, CreateMetrics(L""), spId1, false));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"Type2", L"App2", true, CreateMetrics(L"Random/1.0/25/25"), spId2, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L""), 2));
        fm_->RefreshPLB(Stopwatch::Now());

        // Verify statistics
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(2, 2));
        // SP with non-governed CPU (0) is not included in statistics, hence minimum is 1
        // All hosts are exclusive, so each replica contributes to load
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 6, 1, 1, 14));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 7168, 1024, 1024, 13824));

        VERIFY_IS_TRUE(plbTestHelper.CheckDefragStatistics(6, 0, 0, 0, 0)); //Count: Random metric, SystemCpuCores, SystemMetricMB and 3 default metrics
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        //second replica cannot be placed on node 1 because of app group capacity and cannot be placed on other nodes due to node capacity
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 add primary 3", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 add primary 2", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 1, CreateReplicas(L"P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 1, CreateReplicas(L"P/5"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"ConstraintCheck");
        //we are violation node capacity on this node
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4 move primary 5=>0|2|3|4", value)));

        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 7, 1, 1, 14));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 9216, 1024, 1024, 13824));
    }

    BOOST_AUTO_TEST_CASE(RGAppGroupsScaleout)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGAppGroupsScaleout";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        vector<ServicePackageDescription> packages1;
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName1", L"AppType1", L"App1", 3, 4096, spId1));
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName2", L"AppType1", L"App1", 1, 1024, spId2));
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName3", L"AppType2", L"App2", 1, 1024, spId3));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App1",
            2,
            CreateApplicationCapacities(wformatString(L"")),
            packages1,
            L"AppType1"));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            L"App2",
            2,
            CreateApplicationCapacities(wformatString(L"")),
            packages2,
            L"AppType2"));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type3"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"Type1", L"App1", true, CreateMetrics(L""), spId1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"Type1", L"App1", true, CreateMetrics(L""), spId1, true));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"Type2", L"App2", true, CreateMetrics(L""), spId2, false));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService4", L"Type3", L"App2", true, CreateMetrics(L""), spId3, true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1/D,S/2/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L""), 3));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/4,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        //down replica are not considered in scaleout
        //TestService4 cannot be placed on nodes 3,4 due to node capacity and others are invalid due to scaloeut
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add * 0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(RGWithAffinity)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGWithAffinity";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 1, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 1, 2000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(6, 10, 0));

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 3, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"Type2"), set<Federation::NodeId>()));

        wstring parentService1 = wformatString("{0}_ParentService1", testName);
        wstring childService1 = wformatString("{0}_ChildService1", testName);
        plb.UpdateService(CreateServiceDescription(parentService1, L"Type1", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false, spId1));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService1, L"Type1", true, parentService1, CreateMetrics(L""), false, 1, false, true, spId1));

        wstring parentService2 = wformatString("{0}_ParentService2", testName);
        wstring childService2 = wformatString("{0}_ChildService2", testName);
        plb.UpdateService(CreateServiceDescription(parentService2, L"Type2", true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false, spId2, false));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService2, L"Type2", true, parentService2, CreateMetrics(L""), false, 1, false, true, spId2, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService1), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService1), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(parentService2), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(childService2), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(8u, actionList.size());

        //we need 2 CPUs on the node because resource is not shared
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"0|1 add * 0|1", value)));
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"2|3 add * 2|3", value)));

        wstring childService3 = wformatString("{0}_ChildService3", testName);
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService3, L"Type2", true, parentService2, CreateMetrics(L""), false, 1, false, true, spId2, true));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService1), 1, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService1), 1, CreateReplicas(L""), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(parentService2), 1, CreateReplicas(L"P/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(childService2), 1, CreateReplicas(L"P/6"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(childService3), 0, CreateReplicas(L"P/5"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"ConstraintCheck");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2|3 move primary 6=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(RGWithPlacementConstraint)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGWithPlacementConstraint";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        map<wstring, wstring> node1Prop;
        node1Prop[L"Color"] = L"Blue";
        map<wstring, wstring> node2Prop;
        node2Prop[L"Color"] = L"Blue";
        map<wstring, wstring> node3Prop;
        node3Prop[L"Color"] = L"Red";
        map<wstring, wstring> node4Prop;
        node4Prop[L"Color"] = L"Green";

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 1024, move(node1Prop)));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 1024, move(node2Prop)));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 2048, move(node3Prop)));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 6, 2048, move(node4Prop)));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 2048, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L""), true, appName, L"Color==Blue"));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L""), false, appName, L"Color!=Green"));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        //TestService2 cannot be placed on 0,1,2 because of node capacity and cannot be on node 3 due to placement constraint
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1 add * 0|1", value)));

        // Verify statistics
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(2, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 2, 1, 1, 14));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 3072, 1024, 2048, 6144));
    }

    BOOST_AUTO_TEST_CASE(RGWithFD)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGWithFD";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc0/rack0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc0/rack1"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc0/rack2"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc1/rack0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc1/rack1"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc2/rack0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(6, 2, 2048, map<wstring, wstring>(), L"", L"", L"dc2/rack1"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 2048, spId2));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName3", appTypeName, appName, 2, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType3", set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L""), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L""), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L""), false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3,S/5"), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/6"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //node 4 and 6 are invalid due to node capacity and we need to spread accross domains
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(RGWithUD)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGWithUD";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048, map<wstring, wstring>(), L"", L"ud1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 1024, map<wstring, wstring>(), L"", L"ud1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 1024, map<wstring, wstring>(), L"", L"ud1", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048, map<wstring, wstring>(), L"", L"ud2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 0, 2048, map<wstring, wstring>(), L"", L"ud2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 2048, map<wstring, wstring>(), L"", L"ud2", L""));
        plb.UpdateNode(CreateNodeDescriptionWithResources(6, 2, 2048, map<wstring, wstring>(), L"", L"ud2", L""));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1500, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L""), false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/5,S/6"), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // we must place in ud1 and 0 is the only one with enough capacity
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 0", value)));
        VerifyPLBAction(plb, L"NewReplicaPlacement");

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/4,S/5"), 0));
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 4=>0", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(RGDropTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGDropTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 2, 2014, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/0,S/1,S/2"), -1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //this move will improve the balance situation as it will make the cluster perfectly balanced
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 drop secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(RGBasicPlacement)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGBasicPlacement";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        // Only node 0 has enough resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 1, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 2, 2014, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        // Both new replicas can fit into a single SP on the node 0.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(RGPlacementWithServiceOnEveryNode)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGPlacementWithServiceOnEveryNode";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        //ServicePackage cannot be placed on node 0 or node 1
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 4096));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 4, 4096, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            false,                              // Stateful?
            wstring(L""),                       // Placement constrinats
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            true,                               // On every node
            1,                                  // Partition count
            INT_MAX,                            // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* add * 2", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"I/2"), 0));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 4, 4096));

        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* add * 2", value)));
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"* add * 3", value)));
    }

    BOOST_AUTO_TEST_CASE(RGPlacementTwoServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGPlacementTwoServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 4, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 2, 2048, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 3, 2048, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service1", L"ServiceType1", true, spId1));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service2", L"ServiceType2", true, spId2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L""), 2));

        //we can only place 1 replica
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(RGPlacementInssuficientResources)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGPlacementInssuficientResources";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service1", L"ServiceType1", true, spId1));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service2", L"ServiceType2", true, spId2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L""), 1));

        //we cannot place any replica
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE(RGPlacementWithShouldDisappear)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGPlacementWithShouldDisappear";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 1024));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 2, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service1", L"ServiceType1", true, spId1));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service2", L"ServiceType2", true, spId2));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L"P/0,S/1/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L""), 2));

        //we cannot place any replica on either of the nodes as we account for should disappear load
        fm_->RefreshPLB(Stopwatch::Now());      
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 3, 1024));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"Service1"), 0, CreateReplicas(L"P/2,S/3/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"Service1"), 0, CreateReplicas(L"S/2/D,P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 1, CreateReplicas(L"P/0,S/1"), 0));

        fm_->Clear();
        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 add * 2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicPlacementMultipleAppWithMultipleMetric)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicPlacementMultipleAppWithMultipleMetric";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 1024, std::map<wstring, wstring>(), L"Custom/400,Custom2/0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 1024, std::map<wstring, wstring>(), L"Custom/200,Custom2/0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 1024, std::map<wstring, wstring>(), L"Custom/200,Custom2/0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 3, 1024, std::map<wstring, wstring>(), L"Custom/100,Custom2/0"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 9, 512, std::map<wstring, wstring>(), L"Custom/200,Custom2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 1024, std::map<wstring, wstring>(), L"Custom/200,Custom2/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(6, 9, 1024, std::map<wstring, wstring>(), L"Custom/10,Custom2/10"));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(ApplicationDescription(L"ApplicationName1"));
        plb.UpdateApplication(ApplicationDescription(L"ApplicationName2"));

        wstring appType1Name = wformatString("{0}_AppType1", testName);
        wstring app1Name = wformatString("{0}_Application1", testName);
        vector<ServicePackageDescription> packages1;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName1", appType1Name, app1Name, 5, 0, spId1));
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName2", appType1Name, app1Name, 3, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appType1Name, app1Name, packages1));

        wstring appType2Name = wformatString("{0}_AppType2", testName);
        wstring app2Name = wformatString("{0}_Application2", testName);
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName3",appType2Name, app2Name, 1, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appType2Name, app2Name, packages2));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType3", set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service1", L"ServiceType1", true, spId1, CreateMetrics(L"Custom2/1.0/100/50")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service2", L"ServiceType2", true, spId2, CreateMetrics(L"Custom/1.0/200/200")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"Service3", L"ServiceType3", true, spId3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"Service2"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"Service3"), 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"Service3"), 0, CreateReplicas(L"P/1,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // for Service1 - only Node4 is candidate since its service package needs 5 CPUcores and it can place only one replica
        // for Service2 - only Node0 is candidate since: 
        // Node1 & Node2 eliminated due to Service3 since its package use 1 CPUCore
        // Node3 & Node6 - not enough capacity for Custom metric
        // Node4 - not enough MemoryInMB
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1|2 add primary 0", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add primary 4", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicPlacementSharedAndExclusiveServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicPlacementSharedAndExclusiveServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 5, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 0, 50000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 20, 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 4, 2048, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 4096, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L""), 1));

        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType2"),       // Service type
            wstring(appName),               // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constraints
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            2,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/1"), 1));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        auto actionList = GetActionListString(fm_->MoveActions);
        //we can place service1 becuase it is using shared model but not service 2 because it is using exclusive
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add primary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicPlacementNoEnoughRecources)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicPlacementNoEnoughRecources";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 4, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 8192));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 20, 0));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 0, 30000));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 4, 4096, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),               // Application name
            false,                          // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            true,                           // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0 add * 0|1", value)));

        fm_->Clear();

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 1, CreateReplicas(L"I/0,I/1"), 0));

        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),               // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            2,                              // Partition count
            2,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::Enum::SharedProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L""), 2));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
    }

    BOOST_AUTO_TEST_CASE(PlacementWithDefaultMetrics)
    {
        // Test case: 4 replicas with shared host need to be placed on 2 nodes.
        //      When we account for default metrics, each node needs to receive 2 primaries.
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        wstring testName = L"PlacementWithDefaultMetrics";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 4, 2048, spId));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            4,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId),
            ServiceModel::ServicePackageActivationMode::SharedProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L""), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L""), 1));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* add primary 0", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* add primary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(BalancingWithDefaultMetrics)
    {
        // Test case: 4 replicas with shared host: 3 on node 0, 1 on node 1.
        // Looking only at resources, system is perfectly balanced.
        // Balancing should kick in to balances instances so that distribution is 2-2.
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        wstring testName = L"BalancingWithDefaultMetrics";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 4, 2048, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            false,                              // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            4,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::SharedProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName), 0, CreateReplicas(L"S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName), 0, CreateReplicas(L"S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        auto actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move instance 0=>1", value)));

        //Clear actions and check will default metrics stay after RG metrics update
        fm_->ClearMoveActions();

        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 20, 2048, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, packages2));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

        actionList.clear();
        actionList = GetActionListString(fm_->MoveActions);
        VerifyPLBAction(plb, L"LoadBalancing");
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* move instance 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicUpgradeWithAffinity)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicUpgradeWithAffinity";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 768));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 1, 2048));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 1, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<Federation::NodeId>()));

        wstring parentService = wformatString("{0}_ParentService", testName);
        wstring childService = wformatString("{0}_ChildService", testName);
        plb.UpdateService(CreateServiceDescription(parentService, parentType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false, spId1));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService, childType, true, parentService, CreateMetrics(L""), false, 1, false, true, spId2));

        Reliability::FailoverUnitFlags::Flags upgradingFlag_;
        upgradingFlag_.SetUpgrading(true);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        // Add primary for the parent and move child to the new node
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithNodeCapacity)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckWithNodeCapacity";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        // Only node 0 has enough resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 2, 2048, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 2, 2048, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType2"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));  // Service package
                                                // Both new replicas can fit into a single SP on the node 0.
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service2Name), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Services are shared, but there is not enough node capacity for both to be on one node
        // move either 2 replicas of the first service or 2 replicas of the other
        VERIFY_ARE_EQUAL(2u, actionList.size());
        size_t movesSP1 = CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value));
        size_t movesSP2 = CountIf(actionList, ActionMatch(L"2|3 move primary 0=>1", value));
        VERIFY_IS_TRUE(movesSP1 == 2 || movesSP2 == 2);
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithPlacementConstraints)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckWithPlacementConstraints";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        std::map<std::wstring, std::wstring> nodeProperties1;
        std::map<std::wstring, std::wstring> nodeProperties2;
        std::map<std::wstring, std::wstring> nodeProperties3;

        nodeProperties1[L"Color"] = L"Red";
        nodeProperties2[L"Color"] = L"Red";
        nodeProperties3[L"Color"] = L"Blue";
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048, move(nodeProperties1)));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 2048, move(nodeProperties2)));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 2048, move(nodeProperties3)));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 3, 1024, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),               // Service type
            wstring(appName),                       // Application name
            true,                                   // Stateful?
            wstring(L"Color==Red"),                 // Placement constraints
            wstring(L""),                           // Parent
            true,                                   // Aligned affinity
            move(CreateMetrics(L"")),               // Default metrics
            FABRIC_MOVE_COST_LOW,                   // Move cost
            false,                                  // On every node
            1,                                      // Partition count
            1,                                      // Target replica set size
            true,                                   // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));      // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType1"),               // Service type
            wstring(appName),                       // Application name
            true,                                   // Stateful?
            wstring(L""),                           // Placement constraints
            wstring(L""),                           // Parent
            true,                                   // Aligned affinity
            move(CreateMetrics(L"")),               // Default metrics
            FABRIC_MOVE_COST_LOW,                   // Move cost
            false,                                  // On every node
            1,                                      // Partition count
            1,                                      // Target replica set size
            true,                                   // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));      // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we are in violation due to being over capacity for CPU on node 0 but we cannot move both services to a different node
        //service1 has bad placement constraints
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithTwoServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckWithTwoServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 5, 8192));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 2, 4096, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 3, 2048, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType2"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"* move primary 0|1=>2", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckNoActionNeeded)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckNoActionNeeded";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 2, 2048, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 2, 2048, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType2"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/0/V,S/1/B"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"NoActionNeeded");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintMemoryNodeCapacityViolation)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintMemoryNodeCapacityViolation";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 3000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 3, 3000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 8192));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 3, 8192));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appType1Name = wformatString("{0}_AppType1", testName);
        wstring app1Name = wformatString("{0}_Application1", testName);
        vector<ServicePackageDescription> packages1;
        ServiceModel::ServicePackageIdentifier spId1;
        packages1.push_back(CreateServicePackageDescription(L"ServicePackageName1", appType1Name, app1Name, 0, 2048, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appType1Name, app1Name, packages1));

        wstring appType2Name = wformatString("{0}_AppType2", testName);
        wstring app2Name = wformatString("{0}_Application2", testName);
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName2", appType2Name, app2Name, 0, 2048, spId2));
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName3", appType2Name, app2Name, 0, 2048, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appType2Name, app2Name, packages2));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType3", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),           // Service type
            wstring(app1Name),                  // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraint
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType2"),           // Service type
            wstring(app2Name),                  // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));  // Service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType3"),           // Service type
            wstring(app2Name),                  // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constrinats
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3)));  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"S/0,P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service1Name), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service2Name), 0, CreateReplicas(L"P/0,S/1/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service3Name), 0, CreateReplicas(L"P/0,S/1/D"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"* move primary 0=>2|3", value)));
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckMultipleServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckMultipleServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Give it enough time to find solution.
        PLBConfigScopeChange(ConstraintCheckIterationsPerRound, int, 200);

        // Only node 0 has enough resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 1, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 1, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 4, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 4, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 4096, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 4, 2048, spId2));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName3", appTypeName, appName, 2, 2048, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType3", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType1"),               // Service type
            wstring(appName),                       // Application name
            true,                                   // Stateful?
            wstring(L""),                           // Placement constraints
            wstring(L""),                           // Parent
            true,                                   // Aligned affinity
            move(CreateMetrics(L"")),               // Default metrics
            FABRIC_MOVE_COST_LOW,                   // Move cost
            false,                                  // On every node
            1,                                      // Partition count
            1,                                      // Target replica set size
            true,                                   // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess));   // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType2"),       // Service type
            wstring(appName),               // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constraints
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess));   // Service package
    
        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType3"),       // Service type
            wstring(appName),               // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constraints
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),
            ServiceModel::ServicePackageActivationMode::Enum::SharedProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service3Name), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(6u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 move * 0|2=>4|5", value)));
        VERIFY_ARE_EQUAL(4, CountIf(actionList, ActionMatch(L"2|3 move * 0|1=>2|3", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckMultipleSharedServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckMultipleSharedServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);

        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 5, 2048, std::map<wstring, wstring>(), L"Random/200"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 4, 2048, std::map<wstring, wstring>(), L"Random/50"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048, std::map<wstring, wstring>(), L"Random/50"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048, std::map<wstring, wstring>(), L"Random/50"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 2, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 3, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName3", appTypeName, appName, 2, 4096, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType3", set<Federation::NodeId>()));

        wstring serviceName = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),                            // Service type
            wstring(appName),                                   // Application name
            true,                                               // Stateful?
            wstring(L""),                                       // Placement constraints
            wstring(L""),                                       // Parent
            true,                                               // Aligned affinity
            move(CreateMetrics(L"Random/1.0/100/100")),         // Default metrics
            FABRIC_MOVE_COST_LOW,                               // Move cost
            false,                                              // On every node
            1,                                                  // Partition count
            1,                                                  // Target replica set size
            true,                                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));                   // Service package

        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType2"),                           // Service type
            wstring(appName),                                   // Application name
            true,                                               // Stateful?
            wstring(L""),                                       // Placement constraints
            wstring(L""),                                       // Parent
            true,                                               // Aligned affinity
            move(CreateMetrics(L"")),                           // Default metrics
            FABRIC_MOVE_COST_LOW,                               // Move cost
            false,                                              // On every node
            1,                                                  // Partition count
            1,                                                  // Target replica set size
            true,                                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));                  // Service package

        wstring serviceName3 = L"ServiceName3";
        plb.UpdateService(ServiceDescription(wstring(serviceName3),
            wstring(L"ServiceType3"),                           // Service type
            wstring(appName),                                   // Application name
            true,                                               // Stateful?
            wstring(L""),                                       // Placement constraints
            wstring(L""),                                       // Parent
            true,                                               // Aligned affinity
            move(CreateMetrics(L"")),                           // Default metrics
            FABRIC_MOVE_COST_LOW,                               // Move cost
            false,                                              // On every node
            1,                                                  // Partition count
            1,                                                  // Target replica set size
            true,                                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3)));                  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName2), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"ConstraintCheck");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // move replica since PLB is in node capacity violation for metric Random on Node2
        // Node0 is the only one which has enough capacity for this metric
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move primary 2=>0", value)));

        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName), 1, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName3), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        VerifyPLBAction(plb, L"ConstraintCheck");
        actionList = GetActionListString(fm_->MoveActions);
        // ServiceName3 is in node capacity violation and it can not be fixed (since we do not have a node with this amount of Memory)
        // ServiceName cannot be moved since there is no node with enough capacity for Random
        // move ServiceName2 to the Node1 since it's the only one with enough CPUcores
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckFixNodeCapacityViolation)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckFixNodeCapacityViolation";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 4, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Move TestService1 to the Node1 (only one with enough CpuCores)
        // Then move secondary replica to Node2 to fix node capacity violation
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 1=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithPreventTrainsientOverCommit)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckWithPreventTrainsientOverCommit";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 3, 2048, map<wstring, wstring>(), L"Random/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 2048, map<wstring, wstring>(), L"Random/100"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 5, 2048, map<wstring, wstring>(), L"Random/10"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"Random/1.0/50/50")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType1", true, spId1, CreateMetrics(L""), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType2", true, spId2, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // On Node0 we're in capacity violation for CpuCores
        // TestService3 primary replica cannot be moved because of replica exclusion
        // Need to move primary replica of the TestService1 and the only candidate node is Node1
        // Move secondary replica of the TestService2 to Node2 to avoid node capacity violation for CpuCores on Node1
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 1=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 1, CreateReplicas(L"P/0,S/1,S/2"), 0));

        fm_->Clear();
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        actionList = GetActionListString(fm_->MoveActions);
        // when PreventTransientOvercommit we cannot fix any violation
        VERIFY_ARE_EQUAL(0u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingCpuCores)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingCpuCores";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 10, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 3, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Balance cluster for CpuCores metric
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * 0|1=>2|3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingTestCpuCoresAndMemory)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingTestCpuCoresAndMemory";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.1));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 1.1));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 10, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 10, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 10, 4096));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        ServiceModel::ServicePackageIdentifier spId4;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 1, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 1024, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 1024, spId3));
        packages.push_back(CreateServicePackageDescription(L"Package4", appTypeName, appName, 3, 2048, spId4));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType4"), set<Federation::NodeId>()));
        // Do not add default metrics: we want to test if PLB will move smalles number of replicas in order to move the entire SP.
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService4", L"ServiceType4", true, spId4, CreateMetrics(L"DummyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        //the CPU cannot be balanced any better but the disk can
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2 move * 2|3=>0|1", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingMultipleServicePackage)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingMultipleServicePackage";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.1));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 90, 3000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 50, 4000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 60, 4000));
        
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 70, 1000, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 10, 900, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 10, 900, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        // DummyMetric is here to avoid balancing on default metrics. We want to test if entire SPs will be moved.
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService4", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService5", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService4"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(L"TestService5"), 0, CreateReplicas(L"P/0"), 0));

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 3000);
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Move SP2 or SP3 to another node -> SP2 cannot due to replica exclusion
        // Move SP3 -> partition 2, 4 & 6 to Node1 / Node2
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"2|4|6 move primary 0=>1|2", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingCpuCoresMetric)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingCpuCoresMetric";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 10, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 3, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * 0|1=>2|3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingCpuCoresSharedServicePackages)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingCpuCoresSharedServicePackages";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 9, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 9, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 3, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 6, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        // We want to test that PLB will move correct replicas to move the entire SP, so adding dummy metrics.
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0"), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0"), true));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0"), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService4", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0"), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService5", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0"), true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService4"), 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService5"), 0, CreateReplicas(L"P/2,S/3"), 0));

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 3000);
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Move TestService2 from Node0 & Node 1 to Node2 & Node3 to balance CPUcore metric
        // since its shared service and won't increase CpuCores on those nodes
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"1 move * 0|1=>2|3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingCPUAndMemoryPreventTransientOverCommit)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingCPUAndMemoryPreventTransientOverCommit";

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBAppGroupsTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.5));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 10, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 4, 1, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 0, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        // The intent is to test if preventing overcommit works, so no balancing on default metrics is required.
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0"), false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/3"), 0));

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 3000);
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Balance for both metric:
        // Move one of the partition 0/1 from Node0->Node1
        // Move one of the partition 2/3 from Node1->Node2
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2|3 move * 1=>2", value)));
        VerifyPLBAction(plb, L"LoadBalancing");

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 1, CreateReplicas(L"P/0,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService2"), 1, CreateReplicas(L"P/1,S/3"), 0));

        fm_->Clear();
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        actionList = GetActionListString(fm_->MoveActions);
        // Move only one of the partition 2/3 from Node1->Node2
        // because PreventTransientOvercommit doesn't allow to increase temporarily Memory with moving partitions 0 or 1
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2|3 move * 1=>2", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckAndBalancing)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicConstraintCheckAndBalancing";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1.5));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 9, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 12, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 9, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 5, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 0, 1024, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 1024, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L""), false));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L""), true));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L""), true));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService4", L"ServiceType3", true, spId3, CreateMetrics(L""), true));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"S/0,S/1,P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(0u, actionList.size());
        // ConstraintCheck phase, but cannot fix node capacity violation since Service1 that violates is exclusive service
        VerifyPLBAction(plb, L"ConstraintCheck");

        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 1, CreateReplicas(L"S/0,S/1,P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 1, CreateReplicas(L"P/1"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        actionList = GetActionListString(fm_->MoveActions);
        // LoadBalancing phase -> balance and move to Node0/Node2
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2|3 move primary 1=>0|2", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicBalancingMultipleMetrics)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicBalancingMultipleMetrics";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 0.0));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0));
        balancingThresholds.insert(make_pair(L"Random", 1.5));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 6, 0, map<wstring, wstring>(), L"Random/10000"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 6, 0, map<wstring, wstring>(), L"Random/10000"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 1024, map<wstring, wstring>(), L"Random/10000"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 22, 1024, map<wstring, wstring>(), L"Random/10000"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        ServiceModel::ServicePackageIdentifier spId4;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 1, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 10, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 1, 1024, spId3));
        packages.push_back(CreateServicePackageDescription(L"Package4", appTypeName, appName, 1, 0, spId4));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType4"), set<Federation::NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"Random/1.0/50/50"), false, appName));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"Random/1.0/0/0"), false, appName));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"Random/1.0/200/200"), true, appName));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService4", L"ServiceType4", true, spId4, CreateMetrics(L"Random/1.0/200/200"), true, appName));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService5", L"ServiceType4", true, spId4, CreateMetrics(L"Random/1.0/200/200"), true, appName));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1,S/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L"P/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(L"TestService4"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(L"TestService5"), 0, CreateReplicas(L"P/0,S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"4|5 move * 0|1=>2", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicDefragTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicDefragTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBResourceGovernanceTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), true));
        defragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameMemoryInMB), true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        double metricBalancingThreshold = 1;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), metricBalancingThreshold));
        balancingThresholds.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameMemoryInMB), metricBalancingThreshold));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 8192, std::map<wstring, wstring>(), L"", L"ud1"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 8192, std::map<wstring, wstring>(), L"", L"ud2"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 7, 8192, std::map<wstring, wstring>(), L"", L"ud3"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 7, 8192, std::map<wstring, wstring>(), L"", L"ud4"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 7, 8192, std::map<wstring, wstring>(), L"", L"ud4"));

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 4096, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 4096, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 3, 1024, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2,S/3"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // N2, N3 and N4 have enough CpuCores
        // N4 eliminate because additional moves
        // Move partition 0 and partition 1 to Node2 or Node3 since there is not enough resources on single node for all replicas
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move primary 0|1=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move primary 0|1=>3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicDefragTestSlowBalancing)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BasicDefragTestSlowBalancing";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);

        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), true));
        defragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameMemoryInMB), true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 1);
        PLBConfigScopeChange(LocalDomainWeight, double, 0.0);
        PLBConfigScopeChange(DefragmentationFdsStdDevFactor, double, 0.0);
        PLBConfigScopeChange(DefragmentationUdsStdDevFactor, double, 0.0);

        double metricBalancingThreshold = 1;

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), metricBalancingThreshold));
        balancingThresholds.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameMemoryInMB), metricBalancingThreshold));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 4096, std::map<wstring, wstring>(), L"Custom/10", L"ud1"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 4096, std::map<wstring, wstring>(), L"Custom/10", L"ud2"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 4096, std::map<wstring, wstring>(), L"Custom/15", L"ud3"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 4096, std::map<wstring, wstring>(), L"Custom/15", L"ud4"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 2, 4096, std::map<wstring, wstring>(), L"Custom/15", L"ud5"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 2, 4096, std::map<wstring, wstring>(), L"Custom/15", L"ud6"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(6, 2, 4096, std::map<wstring, wstring>(), L"Custom/15", L"ud6"));

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 4096, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"Custom/0/10/5")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType1", true, spId1, CreateMetrics(L"Custom/0/10/5")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/1,S/2"), 0));

        //fast balancing cannot find solution
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        PLBConfigScopeChange(MaxSimulatedAnnealingIterations, int, 3000);
        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinLoadBalancingInterval);
        actionList = GetActionListString(fm_->MoveActions);
        //slow can
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|1 move primary 0|1=>2|3", value)));
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(RGLoadQueries)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGLoadQueries";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(5, 20, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 2, 100, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 2, 100, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType1", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/3,S/1,S/2/V,S/0/B"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameCpuCores, 8);
        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 400);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);

        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType1"),           // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/3,S/4/V,S/5/B"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/5"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        fm_->RefreshPLB(Stopwatch::Now());

        result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameCpuCores, 14);
        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 700);

        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameCpuCores, 4);
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameCpuCores, 4);

        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 200);
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 200);

        plb.DeleteFailoverUnit(wstring(serviceName2), CreateGuid(2));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);

        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName2), 0, CreateReplicas(L"P/4,S/5/D"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);

        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 5, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/0,S/1/D,S/2/D"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);

        plb.DeleteFailoverUnit(wstring(serviceName1), CreateGuid(0));
        plb.DeleteFailoverUnit(wstring(serviceName1), CreateGuid(1));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 2, CreateReplicas(L"P/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 100);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
    }

    //
    // Tests for updating service packages.
    //
    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithSPUpdateSharedTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        // Place one SP on the node where there are enough resources.
        // Do the refresh, make sure that nothing happens.
        // Update the SP so that it needs to be moved, make sure that it does.
        wstring testName = L"BasicConstraintCheckWithSPUpdateSharedTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 10, 2048, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),               // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        // Now increase CPU cores requested so that it cannot fit to a single node.
        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 20, 2048, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, packages2));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 1, CreateReplicas(L"P/1"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));
    }

    BOOST_AUTO_TEST_CASE(BasicConstraintCheckWithSPUpdateExclusiveTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        // Place one SP on the node where there are enough resources.
        // Do the refresh, make sure that nothing happens.
        // Update the SP so that it needs to be moved, make sure that it does.
        wstring testName = L"BasicConstraintCheckWithSPUpdateTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 5, 1024, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();
        // Now increase CPU cores requested so that it cannot fit to a single node.
        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 10, 1024, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, packages2));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move primary 0=>1", value)));
        VERIFY_IS_FALSE(fm_->IsSafeToUpgradeApp(appId));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 1, CreateReplicas(L"P/1"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));
    }

    BOOST_AUTO_TEST_CASE(RGApplicationUpgradeServicePackageMoveOutBoth)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGApplicationUpgradeServicePackageMoveOutBoth";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 20, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 10, 1024, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Now increase CPU cores and Memory requested so that it cannot fit to a single node.
        vector<ServicePackageDescription> packages2;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 20, 2048, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, packages2));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"0 move * 0|1=>2|3", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/2,S/3"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));
    }

    BOOST_AUTO_TEST_CASE(RGUpgradeCanProceedShared)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGUpgradeCanProceedShared";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 1024));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 5, 1024, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<ServicePackageDescription> package2;
        package2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 10, 1024, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, package2));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));
    }

    BOOST_AUTO_TEST_CASE(RGUpgradeCanProceedExclusive)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGUpgradeCanProceedExclusive";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 10, 1024));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 1024));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 5, 1024, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        vector<ServicePackageDescription> package2;
        package2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 10, 1024, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages, package2));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));
    }

    BOOST_AUTO_TEST_CASE(RGUpdateApplicationWithAppGroupReservation)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGUpdateApplicationWithAppGroupReservation";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 11, 3072));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 4, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 7, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 5, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 6, 2048, spId2));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackagesAndCapacity(
            appTypeName, 
            appName,
            0,
            3,
            CreateApplicationCapacities(wformatString(L"M1/300/40/6")), 
            packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName1";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M1/1.0/3/3")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M1/1.0/2/2")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyAllApplicationReservedLoadPerNode(L"M1", 1, CreateNodeId(0));

        vector<ServicePackageDescription> package2;
        package2.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 4, 2048, spId1));
        package2.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 7, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackagesAndCapacity(
            appTypeName, 
            appName,
            0,
            3,
            CreateApplicationCapacities(wformatString(L"M1/300/25/3")),
            packages, package2));

        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),                // Service type
            wstring(appName),                       // Application name
            true,                                   // Stateful?
            wstring(L""),                           // Placement constraints
            wstring(L""),                           // Parent
            true,                                   // Aligned affinity
            move(CreateMetrics(L"M1/1.0/25/25")),   // Default metrics
            FABRIC_MOVE_COST_LOW,                   // Move cost
            false,                                  // On every node
            1,                                      // Partition count
            1,                                      // Target replica set size
            true,                                   // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));      // Service package

        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),                // Service type
            wstring(appName),                       // Application name
            true,                                   // Stateful?
            wstring(L""),                           // Placement constraints
            wstring(L""),                           // Parent
            true,                                   // Aligned affinity
            move(CreateMetrics(L"M1/1.0/20/20")),   // Default metrics
            FABRIC_MOVE_COST_LOW,                   // Move cost
            false,                                  // On every node
            1,                                      // Partition count
            1,                                      // Target replica set size
            true,                                   // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));      // Service package

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0|1 move * 0=>1|2", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 1, CreateReplicas(L"P/2"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId));

        VerifyAllApplicationReservedLoadPerNode(L"M1", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"M1", 0, CreateNodeId(2));
    }

    BOOST_AUTO_TEST_CASE(RGMultipleApplicationWithMultipleSPQuery)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGMultipleApplicationWithMultipleSPQuery";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 20, 40960));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 40960));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 20, 40960));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 20, 40960));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 20, 40960));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 1, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName, appName, 2, 4096, spId2));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackagesAndCapacity(
            appTypeName,
            appName,
            1,
            5,
            CreateApplicationCapacities(wformatString(L"M1/300/20/15,M2/100/25/10")),
            packages));

        wstring appTypeName2 = wformatString("{0}_AppType2", testName);
        wstring appName2 = wformatString("{0}_Application2", testName);
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId3;
        ServiceModel::ServicePackageIdentifier spId4;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName3", appTypeName, appName, 3, 2048, spId3));
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName4", appTypeName, appName, 4, 1024, spId4));
        ServiceModel::ApplicationIdentifier appId2(appTypeName2, 1);
        plb.UpdateApplication(CreateApplicationWithServicePackagesAndCapacity(
            appTypeName2,
            appName2,
            1,
            5,
            CreateApplicationCapacities(wformatString(L"M1/300/25/15,M3/500/50/20")),
            packages2));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName1";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M1/1.0/5/3")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M1/1.0/10/7,M2/1.0/15/10")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

        wstring serviceName3 = L"ServiceName3";
        plb.UpdateService(ServiceDescription(wstring(serviceName3),
            wstring(L"ServiceType"),            // Service type
            wstring(appName2),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M3/1.0/6/4")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            3,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

        wstring serviceName4 = L"ServiceName4";
        plb.UpdateService(ServiceDescription(wstring(serviceName4),
            wstring(L"ServiceType"),            // Service type
            wstring(appName2),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"M2/1.0/10/5")),// Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            2,                                  // Partition count
            3,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId4)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/2,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName3), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName3), 0, CreateReplicas(L"P/0,S/2,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(6), wstring(serviceName4), 0, CreateReplicas(L"P/0,S/3,S/4"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(7), wstring(serviceName4), 0, CreateReplicas(L"P/1,S/2,S/4"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        //Initial placement
        //Nodes:        N0                  N1                  N2                  N3                  N4
        //S1,Par0       P(M1:5)                                 S(M1:3)
        //S1,Par1                                               P(M1:5)                                 S(M1:3)
        //S2,Par2                                               P(M1:10,M2:15)
        //S2,Par3       P(M1:10,M2:15)
        //S3,Par4       P(M3:6)
        //S3,Par5       P(M3:6)                                 S(M3:4)                                 S(M3:4)
        //S4,Par6       P(M2:10)                                                    S(M2:5)             S(M2:5)
        //S4,Par7                          P(M2:10)             S(M2:5)                                 S(M2:5)
        //----------------------------------  RESERVATION ------------------------------------------------------
        // App1:      (M1:0,M2:0)           (M1:0,M2:0)         (M1:0,M2:0)         (M1:0,M2:0)         (M1:12,M2:10)
        // App2:      (M1:15,M3:8)          (M1:15,M3:20)       (M1:15,M3:16)       (M1:15,M3:20)       (M1:15,M3:16)
        // Total:     (M1:15,M2:0,M3:8)     (M1:15,M2:0,M3:20)  (M1:15,M2:0,M3:16)  (M1:15,M2:0,M3:20)  (M1:27,M2:10,M3:16)
        //------------------------------- EXPECTED TOTAL LOADS -------------------------------------------------
        // Nodes:     (M1:30,M2:25,M3:20)   (M1:15,M2:10,M3:20) (M1:33,M2:20,M3:20) (M1:15,M2:5,M3:20)  (M1:30,M2:20,M3:20)
        // Cluster:   (M1:210,M2:120,M3:110)

        fm_->RefreshPLB(Stopwatch::Now());           

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameCpuCores, 39);
        VerifyLoad(queryResult, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 24576);

        //Node0
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 13);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 10240);
        VerifyNodeLoadQuery(plb, 0, L"M1", 30);
        VerifyNodeLoadQuery(plb, 0, L"M2", 25);
        VerifyNodeLoadQuery(plb, 0, L"M3", 20);

        VerifyAllApplicationReservedLoadPerNode(L"M1", 15, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"M2", 0, CreateNodeId(0));
        VerifyAllApplicationReservedLoadPerNode(L"M3", 8, CreateNodeId(0));

        //Node1
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 4);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024);
        VerifyNodeLoadQuery(plb, 1, L"M1", 15);
        VerifyNodeLoadQuery(plb, 1, L"M2", 10);
        VerifyNodeLoadQuery(plb, 1, L"M3", 20);

        VerifyAllApplicationReservedLoadPerNode(L"M1", 15, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"M2", 0, CreateNodeId(1));
        VerifyAllApplicationReservedLoadPerNode(L"M3", 20, CreateNodeId(1));

        //Node2
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 10);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 8192);
        VerifyNodeLoadQuery(plb, 2, L"M1", 33);
        VerifyNodeLoadQuery(plb, 2, L"M2", 20);
        VerifyNodeLoadQuery(plb, 2, L"M3", 20);

        VerifyAllApplicationReservedLoadPerNode(L"M1", 15, CreateNodeId(2));
        VerifyAllApplicationReservedLoadPerNode(L"M2", 0, CreateNodeId(2));
        VerifyAllApplicationReservedLoadPerNode(L"M3", 16, CreateNodeId(2));

        //Node3
        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameCpuCores, 4);
        VerifyNodeLoadQuery(plb, 3, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024);
        VerifyNodeLoadQuery(plb, 3, L"M1", 15);
        VerifyNodeLoadQuery(plb, 3, L"M2", 5);
        VerifyNodeLoadQuery(plb, 3, L"M3", 20);

        VerifyAllApplicationReservedLoadPerNode(L"M1", 15, CreateNodeId(3));
        VerifyAllApplicationReservedLoadPerNode(L"M2", 0, CreateNodeId(3));
        VerifyAllApplicationReservedLoadPerNode(L"M3", 20, CreateNodeId(3));

        //Node4
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameCpuCores, 8);
        VerifyNodeLoadQuery(plb, 4, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 4096);
        VerifyNodeLoadQuery(plb, 4, L"M1", 30);
        VerifyNodeLoadQuery(plb, 4, L"M2", 20);
        VerifyNodeLoadQuery(plb, 4, L"M3", 20);

        VerifyAllApplicationReservedLoadPerNode(L"M1", 27, CreateNodeId(4));
        VerifyAllApplicationReservedLoadPerNode(L"M2", 10, CreateNodeId(4));
        VerifyAllApplicationReservedLoadPerNode(L"M3", 16, CreateNodeId(4));
    }

    BOOST_AUTO_TEST_CASE(RGUpdateApplicationSPWithNoRGSettings)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGUpdateApplicationSPWithNoRGSettings";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 9, 2560));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 7, 4096));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 11, 5120));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 5, 1024, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(
            appTypeName,
            appName,
            packages));

        wstring appTypeName2 = wformatString("{0}_AppType2", testName);
        wstring appName2 = wformatString("{0}_Application2", testName);
        vector<ServicePackageDescription> packages2;
        ServiceModel::ServicePackageIdentifier spId2;
        packages2.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName2, appName2, 0, 0, spId2));
        ServiceModel::ApplicationIdentifier appId2(appTypeName2, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(
            appTypeName2,
            appName2,
            packages2));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName1";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));  // Service package

        wstring serviceName2 = L"ServiceName2";
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 0, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 0, CreateReplicas(L"P/2,S/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<ServicePackageDescription> package3;
        ServiceModel::ServicePackageIdentifier spId3;
        package3.push_back(CreateServicePackageDescription(L"ServicePackageName1", appTypeName, appName, 0, 0, spId1));
        package3.push_back(CreateServicePackageDescription(L"ServicePackageName3", appTypeName, appName, 2, 1024, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(
            appTypeName,
            appName,
            packages, package3));

        vector<ServicePackageDescription> package4;
        ServiceModel::ServicePackageIdentifier spId4;
        package4.push_back(CreateServicePackageDescription(L"ServicePackageName2", appTypeName2, appName2, 1, 1024, spId2));
        package4.push_back(CreateServicePackageDescription(L"ServicePackageName4", appTypeName2, appName2, 2, 512, spId4));
        plb.UpdateApplication(CreateApplicationWithServicePackages(
            appTypeName2,
            appName2,
            packages2, package4));

        wstring serviceName3 = L"ServiceName3";
        plb.UpdateService(ServiceDescription(wstring(serviceName3),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3)));  // Service package

        wstring serviceName4 = L"ServiceName4";
        plb.UpdateService(ServiceDescription(wstring(serviceName4),
            wstring(L"ServiceType"),            // Service type
            wstring(appName2),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId4),    // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 1, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 1, CreateReplicas(L"P/2,S/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName3), 1, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName4), 1, CreateReplicas(L"P/2,S/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VerifyPLBAction(plb, L"ConstraintCheck");
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"2|3|4|5 move * 0=>1", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 2, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName1), 2, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName2), 2, CreateReplicas(L"P/1,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName2), 2, CreateReplicas(L"P/2,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(serviceName3), 2, CreateReplicas(L"P/0,S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(serviceName4), 2, CreateReplicas(L"P/2,S/0"), 0));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyPLBAction(plb, L"NoActionNeeded");
        VERIFY_IS_TRUE(fm_->IsSafeToUpgradeApp(appId2));
    }

    BOOST_AUTO_TEST_CASE(RGUpgradeRemoveRG)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"RGUpgradeRemoveRG";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Do not do any balancing
        plb.SetMovementEnabled(true, false);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 1, 1024));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        wstring appTypeName2 = wformatString("{0}_AppType2", testName);
        wstring appName2 = wformatString("{0}_Application2", testName);

        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 5, 512, spId1));
        ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        vector<ServicePackageDescription> packagesApp2;
        ServiceModel::ServicePackageIdentifier spId2;
        packagesApp2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName2, appName2, 1, 1024, spId2));
        ServiceModel::ApplicationIdentifier appId2(appTypeName2, 0);
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName2, appName2, packagesApp2));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        wstring serviceName1 = L"ServiceName";
        wstring serviceName2 = L"ServiceName2";
        wstring serviceName3 = L"ServiceName3";
        wstring serviceName4 = L"ServiceName4";
        plb.UpdateService(ServiceDescription(wstring(serviceName1),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(serviceName4),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package
        plb.UpdateService(ServiceDescription(wstring(serviceName2),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(serviceName4),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // Service package
        plb.UpdateService(ServiceDescription(wstring(serviceName3),
            wstring(L"ServiceType"),        // Service type
            wstring(appName),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(serviceName4),          // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1), // Service package
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess)); //activation mode
        plb.UpdateService(ServiceDescription(wstring(serviceName4),
            wstring(L"ServiceType2"),        // Service type
            wstring(appName2),    // Application name
            true,                           // Stateful?
            wstring(L""),                   // Placement constrinats
            wstring(L""),                   // Parent
            true,                           // Aligned affinity
            move(CreateMetrics(L"")),       // Default metrics
            FABRIC_MOVE_COST_LOW,           // Move cost
            false,                          // On every node
            1,                              // Partition count
            1,                              // Target replica set size
            true,                           // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2)));   // Service package
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName3), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName4), 0, CreateReplicas(L""), 3));
        fm_->RefreshPLB(Stopwatch::Now());
        auto actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(6, CountIf(actionList, ActionMatch(L"0|1|2 add * 0|1", value)));
        VERIFY_ARE_EQUAL(3, CountIf(actionList, ActionMatch(L"3 add * 0|1|2", value)));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName1), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(serviceName2), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(serviceName3), 1, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(serviceName4), 1, CreateReplicas(L"P/2,S/1,S/0"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 11);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 2048);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 11);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 2048);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024);

        vector<ServicePackageDescription> package2;
        package2.push_back(CreateServicePackageDescription(L"ServicePackageName", appTypeName, appName, 0, 0, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, package2));

        fm_->Clear();
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024, false);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 1, false);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024, false);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 1, false);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024, false);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 1, false);
        VerifyNodeLoadQuery(plb, 0, ServiceMetric::PrimaryCountName, 3);
    }

    BOOST_AUTO_TEST_CASE(RGPreventOvercommit)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        Trace.WriteInfo("PLBResourceGovernanceTestSource", "RGPreventOvercommit");
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 5, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 3, 2048));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = L"apptype";
        wstring appName = L"app1";
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 4, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 2, 1024, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0,S/1"), 0));
        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move secondary 1=>2", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 0=>1", value)));
        VerifyPLBAction(plb, L"ConstraintCheck");
    }

    BOOST_AUTO_TEST_CASE(SubCoreOldNewSerializationInterop)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"SubCoreOldNewSerializationInterop";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 8, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        ServicePackageResourceGovernanceDescriptionMockup spRGMockup;
        spRGMockup.CpuCores = 7;
        spRGMockup.MemoryInMB = 1024;
        spRGMockup.IsGoverned = true;

        vector<byte> buffer;

        // First serialize the old version to buffer.
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&spRGMockup, buffer).IsSuccess());

        ServiceModel::ServicePackageResourceGovernanceDescription spRG;

        // Now deserialize the new version to buffer.
        VERIFY_IS_TRUE(FabricSerializer::Deserialize(spRG, buffer).IsSuccess());

        // Verify that old and new version are the same.
        VERIFY_ARE_EQUAL(spRG.IsGoverned, spRGMockup.IsGoverned);
        VERIFY_ARE_EQUAL(spRG.MemoryInMB, spRGMockup.MemoryInMB);
        // Don't compare doubles directly, expect reasonable difference.
        VERIFY_IS_TRUE(fabs(spRG.CpuCores - spRGMockup.CpuCores) < 0.0000001);
    }

    BOOST_AUTO_TEST_CASE(SubCoreLoadQueryBasicTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"SubCoreNodeLoadQueryTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Only node 0 has enough resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0.1, 512, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 1.6, 512, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::SharedProcess));   // Service package
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQueryWithDoubleValues(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 1, 0.1, 1, 1.9);
        VerifyNodeLoadQueryWithDoubleValues(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 2, 1.6, 0, 0.4);

        ServiceModel::ClusterLoadInformationQueryResult queryResult;
        ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
        VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

        VerifyClusterLoadDouble(queryResult,
            *ServiceModel::Constants::SystemMetricNameCpuCores,
            2,  // Load (int)
            1.7,// Load (double)
            4,  // Capacity (int)
            2,  // Remaining (int)
            2.3,// Remaining (double)
            false); // Not in violation

        // Verify statistics
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(2, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 1.7, 0.1, 1.6, 4));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024, 512, 512, 4096));
    }

    BOOST_AUTO_TEST_CASE(RGStatisticsAvailableResourcesTest)
    {
        // Verifies that available resources are correct in RG statistics when nodes are added, removed or updated.
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        wstring testName = L"RGStatisticsAvailableResourcesTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;

        // Statistics should be empty at the beginning
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 0));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 0));

        // Add one node without resources - statistics should still be empty
        plb.UpdateNode(CreateNodeDescription(1));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 0));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 0));

        // Add one node with some resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 2048));

        // Add one more node
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 4));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 4096));

        // Decrease the amount of available resources (can't happen, just testing the logic).
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 1, 1024));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 3));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 3072));

        // Increase the amount of available resources (can't happen, just testing the logic).
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 4096));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 6));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 6144));

        // Bring one node down
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 4096, std::map<std::wstring, std::wstring>(), L"", L"", L"", false));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 2048));

        // Bring the node back up
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 4096, std::map<std::wstring, std::wstring>(), L"", L"", L"", true));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 6));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 6144));

        // Bring both nodes down
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 4, 4096, std::map<std::wstring, std::wstring>(), L"", L"", L"", false));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048, std::map<std::wstring, std::wstring>(), L"", L"", L"", false));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 0));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 0));
    }

    BOOST_AUTO_TEST_CASE(RGStatisticsServicePackageTest)
    {
        // Verifies that number of service packages is correct in RG statistics.
        // Cases: app without RG, adding app with RG, modifying app to have and not to have RG.
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        wstring testName = L"RGStatisticsServicePackageTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;

        // Add one node with some resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 2048));

        // Add one application without RG
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0, 0, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 0, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        // 2 SPs total, no governed SPs - maximum and minimum are zero for both metrics
        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(2, 0));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 2048));

        // Add one more application with 1 governed SP
        wstring appTypeName1 = wformatString("{0}_AppType1", testName);
        wstring appName1 = wformatString("{0}_Application1", testName);
        vector<ServicePackageDescription> packages1;
        ServiceModel::ServicePackageIdentifier spId11;
        packages.push_back(CreateServicePackageDescription(L"SP11", appTypeName1, appName1, 0.5, 1024, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        // 3 SPs total, 1 governed SP - maximum and minimum are from that SP
        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(3, 1));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.5, 0.5, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 1024, 1024, 2048));

        // Update first application so that both packages are governed (1 will increase max CPU, other will decrease min memory).
        packages.clear();
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0.75, 1024, spId1));
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 0.5, 512, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(3, 3));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.5, 0.75, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 512, 1024, 2048));

        // Update first application to remove RG from both packages. Minimum and maximum should remain
        packages.clear();
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 0, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(3, 1));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.5, 0.75, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 512, 1024, 2048));

        // Remove 1 SP from first application
        packages.clear();
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0, 0, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        VERIFY_IS_TRUE(plbTestHelper.CheckRGSPStatistics(2, 1));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.5, 0.75, 2));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 512, 1024, 2048));
    }

    BOOST_AUTO_TEST_CASE(RGStatisticsServicesTest)
    {
        // Verifies that number of services with shared and exclusive activation is correct in RG statistics.
        PLBConfigScopeChange(UseRGInBoost, bool, false);

        wstring testName = L"RGStatisticsServicesTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;

        // Add one node with some resources
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 32, 2048));
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameCpuCores, 0.0, 0.0, 0.0, 32));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGMetricStatistics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, 0.0, 0.0, 0.0, 2048));

        // Add one application 
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 1, 128, spId1));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(0, 0));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"Type1", L"App1", true, CreateMetrics(L""), spId1, false));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(0, 1));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"Type1", L"App1", true, CreateMetrics(L""), spId1, false));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(0, 2));

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"Type2", L"App2", true, CreateMetrics(L""), spId1, true));
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(1, 2));

        plb.DeleteService(L"TestService1");
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(1, 1));

        plb.DeleteService(L"TestService2");
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(1, 0));

        plb.DeleteService(L"TestService3");
        plb.ProcessPendingUpdatesPeriodicTask();
        VERIFY_IS_TRUE(plbTestHelper.CheckRGServiceStatistics(0, 0));
    }

    BOOST_AUTO_TEST_CASE(DroppedReplicaSharedSPLoadTest)
    {
        wstring testName = L"DroppedReplicaSharedSPLoadTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 1, 512, spId1));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        // Scenario1: Check Regular, None and Dropped replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"D/1,P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"D/1,P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"N/0,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);

        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 2, 0, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 0, CreateNodeId(1));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 1, 0, CreateNodeId(2));

        // Scenario2: check StandBy and MoveInProgress replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 1, CreateReplicas(L"S/0/V"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 1, CreateReplicas(L"SB/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 1, CreateReplicas(L"D/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);

        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 1, 0, CreateNodeId(1));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 0, CreateNodeId(2));

        // Scenario3: check ToBeDropped(by FM, by PLB and nodeDeactivation) replicas as they are considered as ShouldDissapear
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 2, CreateReplicas(L"S/0/D"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 2, CreateReplicas(L"P/1/T"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 2, CreateReplicas(L"S/2/R"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(1));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(2));
    }

    BOOST_AUTO_TEST_CASE(DroppedReplicaSharedExclusiveSPLoadTest)
    {
        wstring testName = L"DroppedReplicaSharedExclusiveSPLoadTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 1, 512, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 1, 512, spId2));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // exclusive service package

        // Scenario1: Check Regular, None and Dropped replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"D/1,P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"D/0,P/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"D/1,S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        //Scenario1:
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);

        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 1024);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 2);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 1, 0, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 1, 0, CreateNodeId(2));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId2), 1, 0, 0, CreateNodeId(2));

        // Scenario2: check Dropped and ShouldDissapear replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 2, CreateReplicas(L"S/0/D,D/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 2, CreateReplicas(L"P/1/T,D/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 2, CreateReplicas(L"S/2/R,D/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 1, CreateNodeId(1));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId2), 1, 0, 0, CreateNodeId(2));
    }

    BOOST_AUTO_TEST_CASE(NoneReplicaWithResourceGovernanceSPIncludeLoadTest)
    {
        wstring testName = L"NoneReplicaWithResourceGovernanceSPIncludeLoadTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 2, 1024, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 1, 512, spId2));
        ServiceModel::ServicePackageIdentifier spIdNoRG;
        packages.push_back(CreateServicePackageDescription(L"SPNoRG", appTypeName, appName, 0, 0, spIdNoRG));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1)));   // shared service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // exclusive service package

        wstring service4Name = L"Service4";
        plb.UpdateService(ServiceDescription(wstring(service4Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"Dummy/1.0/5/5")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spIdNoRG),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // exclusive service package

        // Config IncludeResourceGovernanceNoneReplicaLoad is on - so include load for none replicas
        // Scenario1: Check load for None replica that have RG load
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"N/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"N/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"N/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(service4Name), 0, CreateReplicas(L"N/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 512);
        VerifyNodeLoadQuery(plb, 1, *ServiceModel::Constants::SystemMetricNameCpuCores, 1);

        VerifyNodeLoadQuery(plb, 2, L"Dummy", 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 2, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);

        // expectedExclusiveRegularReplicaCount, expectedSharedRegularReplicaCount, expectedSharedDisappearReplicaCount
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId1), 0, 0, 0, CreateNodeId(0));
        VerifyServicePackageReplicaNumber(ServiceModel::ServicePackageIdentifier(spId2), 0, 0, 0, CreateNodeId(1));
    }

    BOOST_AUTO_TEST_CASE(NoneReplicaWithResourceGovernanceSPLoadTest)
    {
        PLBConfigScopeChange(IncludeResourceGovernanceNoneReplicaLoad, bool, false);

        wstring testName = L"NoneReplicaWithResourceGovernanceSPLoadTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId;
        packages.push_back(CreateServicePackageDescription(L"SP", appTypeName, appName, 2, 1024, spId));

        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));
        wstring serviceName = L"Service";
        plb.UpdateService(ServiceDescription(wstring(serviceName),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // exclusive service package

        // Config IncludeResourceGovernanceNoneReplicaLoad is off - so do not include load for none replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"N/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameMemoryInMB, 0);
        VerifyNodeLoadQuery(plb, 0, *ServiceModel::Constants::SystemMetricNameCpuCores, 0);
    }

    BOOST_AUTO_TEST_CASE(BalancingCpuCoresActivityThreshold)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        // set activity threshold so that we don't balance
        wstring testName = L"BalancingCpuCoresActivityThreshold";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PLBConfig::KeyIntegerValueMap activityThresholdsNoBalancing;
        activityThresholdsNoBalancing.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholdsNoBalancing);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0.5, 0, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 0.4, 0, spId2));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // No metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // No metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        // check that we didn't generate a move
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());
        VerifyPLBAction(plb, L"NoActionNeeded");

        // Update activity threshold so that we do balancing
        PLBConfig::KeyIntegerValueMap activityThresholdsBalance;
        activityThresholdsBalance.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 0));
        ScopeChangeObjectMetricActivityThresholds.SetValue(activityThresholdsBalance);

        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        // check that we generated a move
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_IS_TRUE(actions.size() > 0u);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BalancingCpuCoresBalancingThreshold)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"BalancingCpuCoresBalancingThreshold";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);
        PLBConfig::KeyDoubleValueMap balancingThresholdsNoBalancing;
        balancingThresholdsNoBalancing.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 5.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholdsNoBalancing);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 2, 2048));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 2, 2048));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 0.5, 0, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 0.4, 0, spId2));
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"SP3", appTypeName, appName, 0.2, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")),
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")),
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"P/0,S/2"), 0));

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        // Check no movements will be generated as balancing threshold is high
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());
        VerifyPLBAction(plb, L"NoActionNeeded");

        PLBConfig::KeyIntegerValueMap activityThresholdsNoBalancing;
        activityThresholdsNoBalancing.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 2));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholdsNoBalancing);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 1, CreateReplicas(L"P/1,S/2"), 0));

        // Check no movements will be generated as activity threshold is high
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());
        VerifyPLBAction(plb, L"NoActionNeeded");

        PLBConfig::KeyIntegerValueMap activityThresholdsBalance;
        activityThresholdsBalance.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1));
        ScopeChangeObjectMetricActivityThresholds.SetValue(activityThresholdsBalance);

        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        // Check that movements will happen as activity threshold is low
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_IS_TRUE(actions.size() > 0u);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(ScopedDefragActivityThresholdTest)
    {
        // There are 5 nodes with loads and capacity as below for SystemCpuMetric
        // 1: 0/4   2: 2/4   3: 8/11   4: 3/4   5: 0/4
        // Strategy is ReservationWithBalancing; 2 nodes with 4 Cpu's should be reserved
        // If activity threshold is set to 9, no balancing is triggered because there are enough empty nodes
        // If activity threshold is set to 7, balancing should trigger and free up some space on node 3 to reach balanced state

        PLBConfigScopeChange(UseRGInBoost, bool, true);

        wstring testName = L"ScopedDefragActivityThresholdTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow - execute only in test mode (functional)
            Trace.WriteInfo("PLBResourceGovernanceTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        // enable scoped defrag so that activity threshold gets involved in triggering
        PLBConfig::KeyBoolValueMap defragMetricMap;
        defragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), true));
        PLBConfigScopeChange(DefragmentationMetrics, PLBConfig::KeyBoolValueMap, defragMetricMap);

        PLBConfig::KeyBoolValueMap scopedDefragMetricMap;
        scopedDefragMetricMap.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), true));
        PLBConfigScopeChange(DefragmentationScopedAlgorithmEnabled, PLBConfig::KeyBoolValueMap, scopedDefragMetricMap);

        // Change defrag triggering mechanism to not distribuite empty nodes
        PLBConfig::KeyIntegerValueMap defragmentationEmptyNodeDistributionPolicy;
        defragmentationEmptyNodeDistributionPolicy.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores),
            Metric::DefragDistributionType::NumberOfEmptyNodes));
        PLBConfigScopeChange(DefragmentationEmptyNodeDistributionPolicy,
            PLBConfig::KeyIntegerValueMap,
            defragmentationEmptyNodeDistributionPolicy);

        int numberOfNodesRequested = 2;
        PLBConfig::KeyDoubleValueMap defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
        defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores),
            numberOfNodesRequested));
        PLBConfigScopeChange(DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
            PLBConfig::KeyDoubleValueMap,
            defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold);

        PLBConfig::KeyIntegerValueMap activityThresholdsNoBalancing;
        activityThresholdsNoBalancing.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 9));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholdsNoBalancing);

        PLBConfig::KeyIntegerValueMap reservationLoad;
        reservationLoad.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 4));
        PLBConfigScopeChange(ReservedLoadPerNode, PLBConfig::KeyIntegerValueMap, reservationLoad);

        PLBConfig::KeyDoubleValueMap emptyNodeWeight;
        emptyNodeWeight.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 0.5));
        PLBConfigScopeChange(DefragmentationEmptyNodeWeight, PLBConfig::KeyDoubleValueMap, emptyNodeWeight);

        PLBConfigScopeChange(BalancingDelayAfterNodeDown, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(BalancingDelayAfterNewNode, TimeSpan, TimeSpan::Zero);
        PLBConfigScopeChange(SimulatedAnnealingIterationsPerRound, int, 3000);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        // Both FD and UD's should exist but distribution isn't important when reservation is done by number of empty nodes
        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 4, 2048, std::map<wstring, wstring>(), L"", L"ud1", L"fd/1"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 4, 2048, std::map<wstring, wstring>(), L"", L"ud1", L"fd/2"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(2, 11, 2048, std::map<wstring, wstring>(), L"", L"ud2", L"fd/3"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(3, 4, 2048, std::map<wstring, wstring>(), L"", L"ud3", L"fd/4"));
        plb.UpdateNode(CreateNodeDescriptionWithResources(4, 4, 2048, std::map<wstring, wstring>(), L"", L"ud4", L"fd/5"));

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        ServiceModel::ServicePackageIdentifier spId2;
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"Package1", appTypeName, appName, 2, 0, spId1));
        packages.push_back(CreateServicePackageDescription(L"Package2", appTypeName, appName, 3, 0, spId2));
        packages.push_back(CreateServicePackageDescription(L"Package3", appTypeName, appName, 5, 0, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType1"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType2"), set<Federation::NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType3"), set<Federation::NodeId>()));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService1", L"ServiceType1", true, spId1, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService2", L"ServiceType2", true, spId2, CreateMetrics(L"DummyMetric/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithServicePackage(L"TestService3", L"ServiceType3", true, spId3, CreateMetrics(L"DummyMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/2,S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/2"), 0));

        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;

        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_ARE_EQUAL(0u, actions.size());
        VerifyPLBAction(plb, L"NoActionNeeded");

        PLBConfig::KeyDoubleValueMap balancingThresholds;
        balancingThresholds.insert(make_pair(*(ServiceModel::Constants::SystemMetricNameCpuCores), 1.0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PLBConfig::KeyIntegerValueMap activityThresholdsBalance;
        activityThresholdsBalance.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 7));
        ScopeChangeObjectMetricActivityThresholds.SetValue(activityThresholdsBalance);

        // force PLB to recalculate whether the services are balanced
        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType2", set<Federation::NodeId>()));

        // Check that movements will happen as activity threshold is low
        fm_->RefreshPLB(Stopwatch::Now());
        VERIFY_IS_TRUE(actions.size() > 0u);
        VerifyPLBAction(plb, L"LoadBalancing");
    }

    BOOST_AUTO_TEST_CASE(BasicRGMetricWeightBalancingTest)
    {
        PLBConfigScopeChange(UseRGInBoost, bool, true);

        // lower weight for RG memory metric
        PLBConfigScopeChange(MemoryInMBMetricWeight, double, 0.3);

        wstring testName = L"BasicRGMetricWeightBalancingTest";
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescriptionWithResources(0, 20, 2000));
        plb.UpdateNode(CreateNodeDescriptionWithResources(1, 20, 2000));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appTypeName = wformatString("{0}_AppType", testName);
        wstring appName = wformatString("{0}_Application", testName);
        vector<ServicePackageDescription> packages;
        ServiceModel::ServicePackageIdentifier spId1;
        packages.push_back(CreateServicePackageDescription(L"SP1", appTypeName, appName, 10, 5, spId1));
        ServiceModel::ServicePackageIdentifier spId2;
        packages.push_back(CreateServicePackageDescription(L"SP2", appTypeName, appName, 5, 5, spId2));
        ServiceModel::ServicePackageIdentifier spId3;
        packages.push_back(CreateServicePackageDescription(L"SP3", appTypeName, appName, 5, 10, spId3));
        plb.UpdateApplication(CreateApplicationWithServicePackages(appTypeName, appName, packages));

        plb.UpdateServiceType(ServiceTypeDescription(L"ServiceType", set<Federation::NodeId>()));

        wstring service1Name = L"Service1";
        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service2Name = L"Service2";
        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        wstring service3Name = L"Service3";
        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"")),           // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 0, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 0, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        // Move replica of the service1 in order to balance cluster for CPU metric as it has higher weight 
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 move primary 1=>0", value)));
        VerifyPLBAction(plb, L"LoadBalancing");

        fm_->Clear();

        // Change the scenario so that cpu metric has now low weight
        PLBConfigScopeChange(CpuCoresMetricWeight, double, 0.3);
        ScopeChangeObjectMemoryInMBMetricWeight.SetValue(1.0);

        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 1, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1, actionList.size());
        // Move replica of the service2 in order to balance cluster for MemoryInMB metric as it has higher weight 
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"2 move primary 1=>0", value)));
        VerifyPLBAction(plb, L"LoadBalancing");

        fm_->Clear();

        // Change the scenario so that both metric have low weight -> No balancing will be triggered
        ScopeChangeObjectCpuCoresMetricWeight.SetValue(0);
        ScopeChangeObjectMemoryInMBMetricWeight.SetValue(0);

        plb.UpdateService(ServiceDescription(wstring(service1Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId1),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateService(ServiceDescription(wstring(service2Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            1,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId2),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateService(ServiceDescription(wstring(service3Name),
            wstring(L"ServiceType"),            // Service type
            wstring(appName),                   // Application name
            true,                               // Stateful?
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent
            true,                               // Aligned affinity
            move(CreateMetrics(L"DummyMetric/1.0/0/0")), // Default metrics
            FABRIC_MOVE_COST_LOW,               // Move cost
            false,                              // On every node
            1,                                  // Partition count
            2,                                  // Target replica set size
            true,                               // Persisted?
            ServiceModel::ServicePackageIdentifier(spId3),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess));   // Service package

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(service1Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(service2Name), 1, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(service3Name), 1, CreateReplicas(L"P/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0, actionList.size());
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestResorceGovernance::VerifyServicePackageReplicaNumber(
        ServiceModel::ServicePackageIdentifier const& servicePackageIdentifier,
        int expectedExclusiveRegularReplicaCount,
        int expectedSharedRegularReplicaCount,
        int expectedSharedDisappearReplicaCount,
        Federation::NodeId nodeId)
    {
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        int exclusiveRegularReplicaCount(0);
        int sharedRegularReplicaCount(0);
        int sharedDisappearReplicaCount(0);

        plbTestHelper.Test_GetServicePackageReplicaCountForGivenNode(
            servicePackageIdentifier,
            exclusiveRegularReplicaCount,
            sharedRegularReplicaCount,
            sharedDisappearReplicaCount,
            nodeId);

        Trace.WriteInfo("PLBResourceGovernanceTestSource", "VerifyServicePackageReplicaNumber: All replicas for servicePackageIdentifier {0} on node {1} : [exclusiveRegularReplicaCount = {2} (expected {3})],\
        [sharedRegularReplicaCount = {4} (expected {5})], [sharedDisappearReplicaCount = {6} (expected {7})]",
            servicePackageIdentifier,
            nodeId,
            exclusiveRegularReplicaCount,
            expectedExclusiveRegularReplicaCount,
            sharedRegularReplicaCount,
            expectedSharedRegularReplicaCount,
            sharedDisappearReplicaCount,
            expectedSharedDisappearReplicaCount);

        VERIFY_ARE_EQUAL(exclusiveRegularReplicaCount, expectedExclusiveRegularReplicaCount);
        VERIFY_ARE_EQUAL(sharedRegularReplicaCount, expectedSharedRegularReplicaCount);
        VERIFY_ARE_EQUAL(sharedDisappearReplicaCount, expectedSharedDisappearReplicaCount);
    }

    void TestResorceGovernance::VerifyAllApplicationReservedLoadPerNode(wstring const& metricName, int64 expectedReservedLoad, Federation::NodeId nodeId)
    {
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        int64 reservedLoad(0);

        plbTestHelper.GetReservedLoadNode(metricName, reservedLoad, nodeId);

        Trace.WriteInfo("PLBResourceGovernanceTestSource", "VerifyAllApplicationReservedLoadPerNode: All reservations on node {0} : {1} (expected {2})",
            nodeId,
            reservedLoad,
            expectedReservedLoad);

        VERIFY_ARE_EQUAL(reservedLoad, expectedReservedLoad);
    }

    bool TestResorceGovernance::ClassSetup()
    {
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestResorceGovernance::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestResorceGovernance::ClassCleanup()
    {
        Trace.WriteInfo("PLBResourceGovernanceTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
