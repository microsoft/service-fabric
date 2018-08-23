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

    StringLiteral const PLBAssertTestSource("PLBAssertTest");

    class TestPLBAssert
    {
    protected:
        TestPLBAssert() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBAssert()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        void FailoverUnitWithoutService();

        shared_ptr<TestFM> fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestPLBAssertSuite, TestPLBAssert)

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithoutServiceType)
    {
        wstring testName = L"ConstraintCheckWithoutServiceType";
        Trace.WriteInfo(PLBAssertTestSource, "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        plb.ProcessPendingUpdatesPeriodicTask();

        set<NodeId> blockList;
        blockList.insert(CreateNodeId(0));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), move(blockList)));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"DummyTestType"), std::set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"")));

        // FT in violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        plb.DeleteServiceType(wstring(L"TestType"));

        VerifySystemErrorThrown(fm_->RefreshPLB(Stopwatch::Now()));

        {
            // Disable test assert and make sure that PLB engine run passes
            Assert::DisableTestAssertInThisScope disableAssert;

            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
        }
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithoutApplication)
    {
        wstring testName = L"ConstraintCheckWithoutApplication";
        Trace.WriteInfo(PLBAssertTestSource, "{0}", testName);

        PLBConfigScopeChange(UseAppGroupsInBoost, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        map<wstring, wstring> nodeProperties0;
        nodeProperties0.insert(make_pair(L"NodeType", L"NOTOK"));
        plb.UpdateNode(CreateNodeDescription(0, L"fd:/0", L"0", move(nodeProperties0)));
        map<wstring, wstring> nodeProperties1;
        nodeProperties1.insert(make_pair(L"NodeType", L"OK"));
        plb.UpdateNode(CreateNodeDescription(1, L"fd:/1", L"1", move(nodeProperties1)));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateApplication(ApplicationDescription(wstring(L"TestApplication")));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(ServiceDescription(wstring(L"TestService"), wstring(L"TestType"), wstring(L"TestApplication"), true, wstring(L"NodeType==OK"),
            wstring(L""), false, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false));

        // FT in violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        // One more application/service/FT to make sure that application entry dictionary is not empty
        plb.UpdateService(CreateServiceDescription(L"OtherTestService", L"TestType", true, CreateMetrics(L"")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"OtherTestService"), 0, CreateReplicas(L"P/1"), 0));

        // Process the FT update before application is deleted
        plb.ProcessPendingUpdatesPeriodicTask();

        // Now, delete the application - we should be able to survive this...
        plb.DeleteApplication(L"TestApplication");

        // In test mode an assert should be thrown because we don't have the application
        VerifySystemErrorThrown(fm_->RefreshPLB(Stopwatch::Now()));

        {
            // Disable test assert and make sure that PLB engine run passes
            Assert::DisableTestAssertInThisScope disableAssert;
            fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);
            VERIFY_ARE_EQUAL(actionList.size(), 1u);
            VerifyPLBAction(plb, L"ConstraintCheck");
        }
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckAffinityViolationsNoParent)
    {
        Trace.WriteInfo(PLBAssertTestSource, "ConstraintCheckAffinityViolationsNoParent");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeParent"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceParent", L"TestTypeParent", true, CreateMetrics(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestTypeChild"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestServiceChild", L"TestTypeChild", true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestServiceParent"), 0, CreateReplicas(L"P/0, S/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestServiceChild"), 0, CreateReplicas(L"P/0, S/1"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        plb.DeleteFailoverUnit(L"TestServiceParent", CreateGuid(0));
        plb.DeleteService(L"TestServiceParent");
        plb.DeleteServiceType(L"TestTypeParent");

        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinConstraintCheckInterval);
    }

    BOOST_AUTO_TEST_CASE(UseSeparateSecondaryLoadChange)
    {
        Trace.WriteInfo(PLBAssertTestSource, "UseSeparateSecondaryLoadChange");

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 4; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"CPU/1.0/100/100")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0, S/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        map<NodeId, uint> loads;
        loads.insert(make_pair(CreateNodeId(1), 50));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, L"TestService", L"CPU", 100, loads));
        fm_->RefreshPLB(Stopwatch::Now());
        //we create the new entry with average load
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        fm_->RefreshPLB(Stopwatch::Now());

        PLBConfigScopeChange(UseSeparateSecondaryLoad, bool, false);

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 2, CreateReplicas(L"P/0, S/1"), 0));

        //we should not assert here even though UseSeparateSecondaryLoad changed
        fm_->RefreshPLB(Stopwatch::Now());
    }

    BOOST_AUTO_TEST_CASE(PlacementAffinityParentMultiplePartitions)
    {
        wstring testName = L"PlacementAffinitParentMultiplePartitions";
        Trace.WriteInfo(PLBAssertTestSource, "{0}", testName);
        PLBConfigScopeChange(PlaceChildWithoutParent, bool, false);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        wstring testType = wformatString("{0}_Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        auto now = Stopwatch::Now();

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", testType, wstring(L""), true, CreateMetrics(L""), ServiceModel::ServicePackageIdentifier(), true, 2));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", testType, true, L"TestService1", CreateMetrics(L"")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", testType, wstring(L""), true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService1"), 0, CreateReplicas(L"P/0,S/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService2"), 0, CreateReplicas(L""), 2));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"TestService3"), 0, CreateReplicas(L""), 2));

        fm_->RefreshPLB(now);
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        //we should not place the child but we should not assert
        VERIFY_ARE_EQUAL(0, CountIf(actionList, ActionMatch(L"2 add * 0|1|2", value)));
        VERIFY_ARE_EQUAL(2, CountIf(actionList, ActionMatch(L"3 add * 0|1|2", value)));
    }

    BOOST_AUTO_TEST_CASE(CompareNodeForPromotionAsserts)
    {
        wstring testName = L"CompareNodeForPromotionAsserts";
        Trace.WriteInfo(PLBAssertTestSource, "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceTypeName = wformatString("{0}_ServiceType", testName);
        wstring serviceName = wformatString("{0}_Service", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceTypeName), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(wstring(serviceName), wstring(serviceTypeName), true, CreateMetrics(L"")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestServiceParent"), 0, CreateReplicas(L"P/0,S/1,S/2"), 0));

        // This was asserting before.
        VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(serviceName, CreateGuid(0), CreateNodeId(2), CreateNodeId(2)));

        // Service 3.14 does not exist, assert in test mode
        VerifySystemErrorThrown(plb.CompareNodeForPromotion(L"fabric:/3.14", CreateGuid(22), CreateNodeId(1), CreateNodeId(2)));

        {
            Assert::DisableTestAssertInThisScope disableAssert;
            // Just return 0 if some of the nodes is not present
            VERIFY_IS_TRUE(0 == plb.CompareNodeForPromotion(L"fabric:/3.14", CreateGuid(22), CreateNodeId(1), CreateNodeId(2)));
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestPLBAssert::FailoverUnitWithoutService()
    {
        // This test will assert in ServiceDomain::GetService() that is called from multiple places
        // This should be enabled when asserts from ServiceDomain class are processed.
        wstring testName = L"FailoverUnitWithoutService";
        Trace.WriteInfo(PLBAssertTestSource, "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), std::set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"")));
        // To avoid asserting on non-empty FT table when last service is deleted
        plb.UpdateService(CreateServiceDescription(L"PointlessService", L"TestType", true, CreateMetrics(L"")));

        // FT in violation
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));

        fm_->RefreshPLB(Stopwatch::Now());

        plb.DeleteService(wstring(L"TestService"));

        fm_->RefreshPLB(Stopwatch::Now());
    }


    bool TestPLBAssert::ClassSetup()
    {
        Trace.WriteInfo("PLBAssertTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestPLBAssert::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBAssert::ClassCleanup()
    {
        Trace.WriteInfo("PLBAssertTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }
}
