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
#include <boost/mpl/vector.hpp>
#include "Common/boost-taef.h"
#include "Reliability/Failover/common/FailoverConfig.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Reliability::LoadBalancingComponent;

    class TestPLBUpgrade
    {
    protected:
        TestPLBUpgrade() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestPLBUpgrade()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        struct SingleReplicaUpgradeTestHelper
        {
            // Test flavor
            std::wstring flavor_;

            //Node setup
            int numberOfNodes_;
            std::vector<std::wstring> nodeCapacities_;
            std::vector<std::wstring> nodeFDs_;
            std::vector<std::wstring> nodeUDs_;
            std::vector<Reliability::NodeDeactivationIntent::Enum> nodeDeactivationIntent_;
            std::vector<Reliability::NodeDeactivationStatus::Enum> nodeDeactivationStatus_;

            // Application setup
            bool needApplication_;
            bool singleApplication_;
            bool isAppGroup_;
            int appScaleout_;
            int appMinNodes_;
            std::vector<std::wstring> applications_;
            std::vector<std::wstring> appCapacities_;
            bool isAppUpgradeInProgress_;
            std::set<std::wstring> appCompletedUDs_;


            // Service setup
            int numberOfServices_;
            bool hasParentService_;
            std::vector<std::wstring> serviceTypes_;
            std::vector<std::wstring> services_;
            std::vector<bool> isStateful_;
            std::vector<bool> isPersisted_;
            std::vector<int> targetReplicaCount_;
            std::vector<std::wstring> defaultMetrics_;

            // Replica setup
            bool createReplicas_;
            std::vector<bool> replicaInUpgrade_;
            std::vector<int> replicaPlacement_;

            // SF upgrade setup
            bool clusterUpgradeInProgress_;
            std::set<std::wstring> clusterCompletedUDs_;

            // Run and Verify
            bool runAndVerify;

            SingleReplicaUpgradeTestHelper(std::wstring flavor = L"") :
                flavor_(flavor),
                numberOfNodes_(2),
                nodeCapacities_(),
                nodeFDs_(),
                nodeUDs_(),
                nodeDeactivationIntent_(),
                nodeDeactivationStatus_(),
                needApplication_(false),
                singleApplication_(true),
                isAppGroup_(false),
                appScaleout_(0),
                appMinNodes_(0),
                applications_(),
                appCapacities_(),
                isAppUpgradeInProgress_(false),
                appCompletedUDs_(),
                numberOfServices_(2),
                hasParentService_(false),
                serviceTypes_(),
                services_(),
                isStateful_(),
                isPersisted_(),
                targetReplicaCount_(),
                defaultMetrics_(),
                createReplicas_(false),
                replicaInUpgrade_(),
                replicaPlacement_(),
                clusterUpgradeInProgress_(false),
                clusterCompletedUDs_(),
                runAndVerify(false)
            {
                if (flavor == L"CheckAffinity")
                {
                    hasParentService_ = true;
                }
                else if (flavor == L"RelaxedScaleout")
                {
                    needApplication_ = true;
                    isAppGroup_ = true;
                    appScaleout_ = 1;
                    appMinNodes_ = 1;
                }
            };
        };

        shared_ptr<TestFM> fm_;
        Reliability::FailoverUnitFlags::Flags upgradingFlag_;
        Reliability::FailoverUnitFlags::Flags swappingPrimaryFlag_;

        void RunTestHelper(
            PlacementAndLoadBalancing & plb,
            SingleReplicaUpgradeTestHelper & testHelper);
    };

    // Define multiple test flavors that will be run on the same scenario
    struct CheckAffinityFlavor {
        wstring value;
        CheckAffinityFlavor() :value(L"CheckAffinity") {};
    };

    struct RelaxedScaleoutFlavor {
        wstring value;
        RelaxedScaleoutFlavor() :value(L"RelaxedScaleout") {};
    };

    typedef boost::mpl::vector<CheckAffinityFlavor, RelaxedScaleoutFlavor> TestFlavors;

    BOOST_FIXTURE_TEST_SUITE(TestPLBUpgradeSuite, TestPLBUpgrade)

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeBasicTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Parent will be in upgrade and will have +1 replica difference,
        // child will not have it set. Expectation is that we add one replica for and move other
        // RelaxedScaleout: One replica will be in upgrade while other is not
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeBasicTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgrade,
            [&]()->bool{return flavor==L"CheckAffinity";},
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradePlacementNegativeTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Parent and two child partitions in upgrade with replica diff +1
        // Node load and capacity is set so that we can't add new primaries for all 3 partitions
        // Expectation: do not do anything because we can't add all of them together.
        // Then, we set the flag to false and expect to add 2 new primaries (1 parent + 1 child).
        // RelaxedScaleout: The same scenario, but replicas are in application scaleout 1 correlation
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradePlacementNegativeTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradePlacementNegativeTest,
            [&]()->bool{return flavor==L"CheckAffinity";},
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 2;
        // Node 0
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeDeactivationIntent_.push_back(Reliability::NodeDeactivationIntent::Enum::Restart);
        testHelper.nodeDeactivationStatus_.push_back(Reliability::NodeDeactivationStatus::Enum::DeactivationSafetyCheckInProgress);
        // Node 1
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/20");
        testHelper.nodeDeactivationIntent_.push_back(Reliability::NodeDeactivationIntent::Enum::None);
        testHelper.nodeDeactivationStatus_.push_back(Reliability::NodeDeactivationStatus::Enum::None);
        testHelper.numberOfServices_ = 3;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/10/10");
        }
        RunTestHelper(plb, testHelper);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(testHelper.services_[2]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Reset the optimization config
        PLBConfigCondScopeModify(SingleReplicaUpgradePlacementNegativeTest, false);
        fm_->RefreshPLB(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);
        actionList = GetActionListString(fm_->MoveActions);

        // CheckAffinity: Check that torn affinity is happening
        // RelaxedScaleout: Check that upgrade is still blocked
        if (flavor == L"CheckAffinity")
        {
            VERIFY_ARE_EQUAL(2u, actionList.size());
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        }
        else
        {
            VERIFY_ARE_EQUAL(0u, actionList.size());
        }
    }

    BOOST_AUTO_TEST_CASE(UpgradeWithSvt1AffinityParentWithTwoReplicasTest)
    {
        // Basic test for CheckAffinityForUpgradePlacement with negative outcome:
        // Parent will be in upgrade, will have 2 replicas and replica difference 0, child will have replica difference +1
        // Expectation is that we add one replica for the child without moving the parent
        wstring testName = L"UpgradeWithSvt1AffinityParentWithTwoReplicasTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_ParentService", testName);
        wstring childService = wformatString("{0}_ChildService", testName);
        plb.UpdateService(CreateServiceDescription(parentService, parentType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 2, false));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(childService, childType, true, parentService, CreateMetrics(L""), false, 1, false, false));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0,S/1"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradePlacementWithRelaxedAffinityOrScaleoutTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // CheckAffinityForUpgradePlacement will be set to true, and application upgrade is in progress
        // Cluster upgrade is also in progress - balancing is disabled and constraint check is disabled as well.
        // Node load and capacity is set so that we can't move both parent and child.
        // But, affinity is relaxed during cluster upgrade so we will be able to move one of them.
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        // Also, primary and secondary default loads are different, and expectation is that primary def load will be used,
        // for both primary and secondary replicas
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradePlacementWithRelaxedAffinityOrScaleoutTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradePlacementWithRelaxedAffinityOrScaleoutTest,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/20");
        // Secondary has lower default load specified,
        // this needs to be aligned with primary load during service update
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/15/5");
        }
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        // Node 2 cannot accept both services, so no movement.
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Then upgrade starts, and affinity is relaxed
        if (flavor == L"CheckAffinity")
        {
            plb.SetMovementEnabled(false, false);
            plb.Refresh(Stopwatch::Now() + PLBConfig::GetConfig().MinPlacementInterval);

            actionList = GetActionListString(fm_->MoveActions);
            VERIFY_ARE_EQUAL(1u, actionList.size());
            VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        }
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradePlacementWithNodeCapacityTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // Only one node will have enough capacity to accommodate add+move
        // Expectation is that we add one replica for parent and move child to a correct node
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        // Also, primary replica load got updated, and expectation is that primary load will be used,
        // for both primary and secondary replicas
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradePlacementWithNodeCapacityTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        // To force placement on node 2
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradePlacementWithNodeCapacityTest,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 3;
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/20");
        testHelper.isAppUpgradeInProgress_ = true;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/5/5");
        }
        RunTestHelper(plb, testHelper);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        // Update the load of singleton replica services, with misaligned primary and secondary loads.
        // Expectation is that both primary and secondary load will be aligned for singletons (i.e. 16)
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(0, wstring(testHelper.services_[0]), L"NoNameMetric", 15, 5));
        plb.UpdateLoadOrMoveCost(CreateLoadOrMoveCost(1, wstring(testHelper.services_[1]), L"NoNameMetric", 15, 5));

        // Node 3 should be eliminated because is has capacity of 20.
        // If we don't eliminate it while searching dummy PLB will force add/move to 3
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradePlacementWithReservationTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // Loads are zero for both replicas, but reservation is set so that only one node can accommodate both.
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradePlacementWithReservationTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradePlacementWithReservationTest,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 4;
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/20");
        testHelper.nodeCapacities_.push_back(L"NoNameMetric/30");
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/5/5");
        }
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        testHelper.isAppGroup_ = true;
        // In relaxed scaleout, application capacity should also be relaxed
        // hence expectation is that upgrade will not be blocked
        if (flavor == L"RelaxedScaleout")
        {
            testHelper.appCapacities_.push_back(L"NoNameMetric/30/30/30");
        }
        else
        {
            testHelper.appCapacities_.push_back(L"NoNameMetric/300/30/30");
        }
        testHelper.createReplicas_ = true;
        testHelper.replicaInUpgrade_.push_back(true); // first service replica in upgrade
        testHelper.replicaInUpgrade_.push_back(false); // second service replica not in upgrade
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Add one more service to the application and place it on node 3 to use load of 10
        wstring otherServiceType = wformatString("{0}_OtherType", testName);
        ServiceTypeDescription otherServiceTypeDescription = ServiceTypeDescription(wstring(otherServiceType), set<NodeId>());
        plb.UpdateServiceType(move(otherServiceTypeDescription));
        wstring otherServiceName = wformatString("{0}_OtherService", testName);
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            otherServiceType,
            wstring(L""),
            true,
            CreateMetrics(L"NoNameMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(otherServiceName), 0, CreateReplicas(L"P/3"), 0, upgradingFlag_));

        // Node 3 should be eliminated because two replicas can't be placed on it due to application capacity.
        // If we don't eliminate it while searching dummy PLB will force add/move to 3
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(CheckAffinityForUpgradePlacementWithOneApplicationTest)
    {
        // Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // Due to application capacity only one node will have enough capacity to accommodate add+move
        // Expectation is that we add one replica for parent and move child to a correct node
        wstring testName = L"CheckAffinityForUpgradePlacementWithOneApplicationTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(L"CheckAffinity");
        testHelper.numberOfServices_ = 2;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/15/15");
        }
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        testHelper.isAppGroup_ = true;
        testHelper.appCapacities_.push_back(L"NoNameMetric/300/30/0");
        testHelper.createReplicas_ = true;
        testHelper.replicaInUpgrade_.push_back(true); // parent service replica in upgrade
        testHelper.replicaInUpgrade_.push_back(false); // child service replica not in upgrade
        RunTestHelper(plb, testHelper);

        // Add one more service to the application and place it on node 2 to use load of 10
        wstring otherServiceType = wformatString("{0}_OtherType", testName);
        ServiceTypeDescription otherServiceTypeDescription = ServiceTypeDescription(wstring(otherServiceType), set<NodeId>());
        plb.UpdateServiceType(move(otherServiceTypeDescription));
        wstring otherServiceName = wformatString("{0}_OtherService", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            otherServiceType,
            wstring(testHelper.applications_[0]),
            true,
            CreateMetrics(L"NoNameMetric/1.0/10/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(otherServiceName), 0, CreateReplicas(L"P/2"), 0, upgradingFlag_));

        // Node 3 should be eliminated because two replicas can't be placed on it due to application capacity.
        // If we don't eliminate it while searching dummy PLB will force add/move to 3
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(CheckAffinityForUpgradePlacementWithTwoApplicationsTest)
    {
        // Parent will be in upgrade and will have +1 replica difference, child will not have it set.
        // Parent and child belong to different applications in this case
        // Due to application capacity only one node will have enough capacity to accommodate add+move
        // Each application should eliminate one node
        // Expectation is that we add one replica for parent and move child to a correct node
        wstring testName = L"CheckAffinityForUpgradePlacementWithTwoApplicationsTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        // To force placement on highest node ID
        PLBConfigScopeChange(DummyPLBEnabled, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(L"CheckAffinity");
        testHelper.numberOfServices_ = 2;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"NoNameMetric/1.0/15/15");
            testHelper.appCapacities_.push_back(L"NoNameMetric/300/30/0");
        }
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = false;
        testHelper.isAppGroup_ = true;
        testHelper.createReplicas_ = true;
        testHelper.replicaInUpgrade_.push_back(true); // first service replica in upgrade
        testHelper.replicaInUpgrade_.push_back(false); // second service replica not in upgrade
        RunTestHelper(plb, testHelper);

        // Add one more node
        plb.UpdateNode(CreateNodeDescription(3));

        // Add one more service to parent application and place it on node 3 to use load of 10
        wstring otherServiceType = wformatString("{0}_OtherParentType", testName);
        ServiceTypeDescription otherServiceTypeDescription = ServiceTypeDescription(wstring(otherServiceType), set<NodeId>());
        plb.UpdateServiceType(move(otherServiceTypeDescription));
        wstring otherServiceName = wformatString("{0}_OtherParentService", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            otherServiceType,
            wstring(testHelper.applications_[0]),
            true,
            CreateMetrics(L"NoNameMetric/1.0/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(3), wstring(otherServiceName), 0, CreateReplicas(L"P/3"), 0, upgradingFlag_));

        // Add one more service to child application and place it on node 2 to use load of 10
        otherServiceType = wformatString("{0}_OtherChildType", testName);
        otherServiceTypeDescription = ServiceTypeDescription(wstring(otherServiceType), set<NodeId>());
        plb.UpdateServiceType(move(otherServiceTypeDescription));
        otherServiceName = wformatString("{0}_OtherChildService", testName);

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            otherServiceName,
            otherServiceType,
            wstring(testHelper.applications_[1]),
            true,
            CreateMetrics(L"NoNameMetric/1.0/20/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(4), wstring(otherServiceName), 0, CreateReplicas(L"P/2"), 0, upgradingFlag_));

        // Node 3 should be eliminated because it already uses 20 load in parent application
        // Node 2 should be eliminated because it already uses 20 load in child application
        // If we don't eliminate it while searching dummy PLB will force add/move to 3
        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 move primary 0=>1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE(CheckAffinityForUpgradePlacementInitialAffinityViolation)
    {
        // Both parent and child will be in upgrade and will have replica difference of +1
        // Total application capacity will be breached if both new replicas are added.
        // Per-node capacity is set so that it won't be violated and that we can find target nodes
        // Expectation is that we do not do anything
        wstring testName = L"CheckAffinityForUpgradePlacementInitialAffinityViolation";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));
        plb.UpdateNode(CreateNodeDescription(2));

        wstring parentApplicationName = wformatString("{0}_Application", testName);
        wstring childApplicationName = parentApplicationName;
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(parentApplicationName, 0, CreateApplicationCapacities(L"NoNameMetric/60/30/0")));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);

        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = wformatString("{0}_ParentService", testName);
        wstring childService = wformatString("{0}_ChildService", testName);

        plb.UpdateService(ServiceDescription(
            wstring(parentService),             // Service Name
            move(parentType),                   // Parent service type
            wstring(parentApplicationName),     // Application name
            true,                               // Stateful
            wstring(L""),                       // Placement constraints
            wstring(L""),                       // Parent service
            true,                               // Aligned affinity
            CreateMetrics(L"NoNameMetric/1.0/10/10"),// Service metrics
            FABRIC_MOVE_COST_LOW,               // Default move cost
            false,                              // On every node?
            0,                                  // Partition count
            1,                                  // Target replica set size
            false));                            // Has persisted state

        plb.UpdateService(ServiceDescription(
            wstring(childService),              // Service Name
            move(childType),                    // Parent service type
            wstring(childApplicationName),      // Application name
            true,                               // Stateful
            wstring(L""),                       // Placement constraints
            wstring(parentService),             // Parent service
            true,                               // Aligned affinity
            CreateMetrics(L"NoNameMetric/1.0/10/10"), // Service metrics
            FABRIC_MOVE_COST_LOW,               // Default move cost
            false,                              // On every node?
            0,                                  // Partition count
            1,                                  // Target replica set size
            false));                            // Has persisted state

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));

        // Affinity teardown - but we will still attempt placement for upgrade
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(childService), 0, CreateReplicas(L"P/1/LI"), 1, upgradingFlag_));

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(2), wstring(childService), 0, CreateReplicas(L"P/1/LI"), 1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Total application capacity is 40, we're already using 30 and trying to add 30 more
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"* add secondary 2", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradePlacementAllCombinationsTest, TestFlavor, TestFlavors)
    {
        // CheckAffinity: We are testing all possible combinations for CheckAffinityForUpgradePlacement:
        //  Parent and child can be stateful  or stateless
        //  We can request +1 for parent and/or child
        //  Parent and child can belong to the same application or to different applications
        // For each of these combinations we verify:
        //  That partition with replica difference of 1 has one replica added.
        //  That partition with replica difference of 0 has primary moved
        //  That target node is the same for both partitions
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradePlacementAllCombinationsTest{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        if (!PLBConfig::GetConfig().IsTestMode)
        {
            // This test is slow, and tests an edge case - execute only in test mode (functional)
            Trace.WriteInfo("PLBUpgradeTestSource", "Skipping {0} (IsTestMode == false).", testName);
            return;
        }

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradePlacementAllCombinationsTest,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        for (bool firstReplicaInUpgrade : {false, true})
        {
            for (bool firstReplicaIsStateful : {false, true})
            {
                for (bool secondReplicaInUpgrade : {false, true})
                {
                    for (bool secondReplicaIsStateful : {false, true})
                    {
                        // If there is no replica in upgrade,
                        // or only stateless ones are in upgrade
                        if ((!firstReplicaInUpgrade && !secondReplicaInUpgrade) ||
                            (!secondReplicaIsStateful && !firstReplicaIsStateful) ||
                            (!firstReplicaInUpgrade && secondReplicaInUpgrade && !secondReplicaIsStateful) ||
                            (firstReplicaInUpgrade && !secondReplicaInUpgrade && !firstReplicaIsStateful))
                            {
                                continue;   // Nothing to test here
                            }
                        for (bool inSameApplication : {false, true})
                        {
                            if (!inSameApplication && flavor == L"RelaxedScaleout")
                            {
                                continue;
                            }
                            wstring testCaseName = wformatString("{0}_{1}-{2}-{3}-{4}-{5}",
                                testName,
                                firstReplicaIsStateful,
                                firstReplicaIsStateful,
                                secondReplicaInUpgrade,
                                secondReplicaIsStateful,
                                inSameApplication);

                            fm_->Load();
                            SingleReplicaUpgradeTestHelper testHelper(flavor);
                            testHelper.needApplication_ = true;
                            testHelper.singleApplication_ = inSameApplication;
                            testHelper.isStateful_.push_back(firstReplicaIsStateful);
                            testHelper.isStateful_.push_back(secondReplicaIsStateful);
                            testHelper.createReplicas_ = true;
                            testHelper.replicaInUpgrade_.push_back(firstReplicaInUpgrade);
                            testHelper.replicaInUpgrade_.push_back(secondReplicaInUpgrade);
                            testHelper.runAndVerify = true;
                            testHelper.isAppUpgradeInProgress_ = true;
                            RunTestHelper(fm_->PLB, testHelper);
                        }
                    }
                }
            }
        }
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeNewServiceCreation, TestFlavor, TestFlavors)
    {
        // CheckAffinity: Creations of new service in the middle of Application upgrade,
        // which contains affinitized services and CheckAffinityForUpgradePlacement is set
        // Expected behavior is that new replica will not move already moved application replicas
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeNewServiceCreation{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(0));
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeNewServiceCreation,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 3;
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(L"M1/100");
        }
        testHelper.numberOfServices_ = 6;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
        }
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        RunTestHelper(plb, testHelper);

        // Application is in upgrade (UD0 is done)
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            testHelper.applications_[0] ,
            testHelper.appMinNodes_,
            testHelper.appScaleout_,
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)));

        // Instantiate all partitions except two with application in upgrade flag ("B")
        Reliability::FailoverUnitFlags::Flags replicaFlags(true);
        replicaFlags.SetUpgrading(true);
        for (int i = 0; i < testHelper.numberOfServices_ - 2; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wstring(testHelper.services_[i]), 1, CreateReplicas(L"P/1"), 0, replicaFlags));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        // Initial placement (CheckAffinity flavor):
        // Nodes:            N0(M1:100)    N1(M1:100)   N2(M1: 100)
        // Parent:                         P(M1:10)
        // Children:                       3xP(M1:10)

        // New (child) service has been created in the middle of upgrade process (no replicas in upgrade),
        // it is expected that it should not initiate movements of other replicas
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 1, CreateReplicas(L""), 1, replicaFlags));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add primary 1", value)));
        // Apply the movement
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 2, CreateReplicas(L"P/1"), 0, replicaFlags));
        plb.ProcessPendingUpdatesPeriodicTask();

        // New (child) service has been created in the middle of upgrade process (some replicas are in upgrade),
        // it is expected that all replicas are added/moved to the upgraded domain (node0)
        for (int i = 0; i < testHelper.numberOfServices_ - 1; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wstring(testHelper.services_[i]),
                2,
                CreateReplicas(i % 2 == 0 ? L"P/1/LI" : L"P/1"),
                i % 2 == 0 ? 1 : 0,
                replicaFlags));
        }
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(testHelper.services_[5]), 0, CreateReplicas(L""), 1, replicaFlags));
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(6u, actionList.size());
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"* add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5 add primary 0", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"* move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeToBePromotedReplicas, TestFlavor, TestFlavors)
    {
        // CheckAffinity: During application upgrade, FM drops one of the generated movements and send +1 again,
        // but other replicas has started building process
        // Expected behavior is that new movement will be generated only of replica that got +1,
        // and leave others to finish building process
        // RelaxedScaleout: The same scenario, except replicas are in application scaleout 1 correlation
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeToBePromotedReplicas{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeNewServiceCreation,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 10;
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(L"M1/100");
        }
        testHelper.numberOfServices_ = 4;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
        }
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        RunTestHelper(plb, testHelper);

        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(
                CreateGuid(i),
                wstring(testHelper.services_[i]),
                0,
                CreateReplicas(L"P/1"),
                0)));
        }
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:            N0(M1:50)    N1(M1:40)   N2(M1: 50)
        // Parent:                        P(M1:10)
        // Children:                      3xP(M1:10)

        // Application is in upgrade (UD0 is done)
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            testHelper.applications_[0],
            testHelper.appMinNodes_,
            testHelper.appScaleout_,
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)));

        // All partitions have got application in-upgrade flag ("B")
        // Partition that is not in upgrade (e.g. only set of service packages are in upgrade)
        // or partition which update is just late, will not get +1 (child1 and child3)
        // Other partitions (parent and child2), will receive update with actual difference +1
        Reliability::FailoverUnitFlags::Flags replicaFlags(true);
        replicaFlags.SetUpgrading(true);
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"P/1/LI"), 1, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"P/1"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"P/1/LI"), 1, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 1, CreateReplicas(L"P/1"), 0, replicaFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0|2 add secondary 0", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1|3 move primary 1=>0", value)));
        fm_->Clear();

        // Now, all replicas, except one, have started building process on node 0.
        // FM had rejected movement for child3 service (from some reason), and it receives late +1
        // Expectation is that no movement will be generated until there is some non-movable replicas
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 2, CreateReplicas(L"S/0/B, P/1/LI"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 2, CreateReplicas(L"S/0/B, P/1/LI"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 2, CreateReplicas(L"S/0/B, P/1/LI"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 2, CreateReplicas(L"P/1/LI"), 1, replicaFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Then, all replicas are build on node 0, and swap primary is initiated.
        // Expectation is that no movement will be generated until there is some non-movable replicas
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 2, CreateReplicas(L"P/0/P, S/1/L"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 2, CreateReplicas(L"P/0/P, S/1/L"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 2, CreateReplicas(L"P/0/P, S/1/L"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 2, CreateReplicas(L"P/1/LI"), 1, replicaFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // At the end, all replicas are movable, and we should move the pending one to the node 0
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 3, CreateReplicas(L"P/0"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 3, CreateReplicas(L"P/0"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 3, CreateReplicas(L"P/0"), 0, replicaFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 3, CreateReplicas(L"P/1/LI"), 1, replicaFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeWithMovementDrop, TestFlavor, TestFlavors)
    {
        // Scenario is testing what happens when during single replica upgrade,
        // which is in Affinity or AppGroup with scaleout one correlations,
        // some movements are rejected/dropped by the FM, and replica receives new +1 request.
        // Also, test verifies that creation of new replica doesn't move replicas that are not movable
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeWithMovementDrop{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeWithMovementDrop,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 3;
        testHelper.nodeCapacities_.push_back(L"M1/50");
        testHelper.nodeCapacities_.push_back(L"M1/40");
        testHelper.nodeCapacities_.push_back(L"M1/50");
        // Application setup
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        // Service setup
        testHelper.numberOfServices_ = 6;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
            switch(i)
            {
                case 0:
                case 2:
                case 5:
                {
                    testHelper.isStateful_.push_back(true);
                    testHelper.isPersisted_.push_back(false);
                    break;
                }
                case 1:
                {
                    testHelper.isStateful_.push_back(false);
                    testHelper.isPersisted_.push_back(false);
                    break;
                }

                case 3:
                case 4:
                {
                    testHelper.isStateful_.push_back(true);
                    testHelper.isPersisted_.push_back(true);
                    break;
                }
                default:
                    break;
            }
        }
        RunTestHelper(plb, testHelper);

        // Replica setup
        for (int i = 0; i < testHelper.numberOfServices_ - 1; i++)
        {
            fm_->FuMap.insert(make_pair(CreateGuid(i), FailoverUnitDescription(
                CreateGuid(i),
                wstring(testHelper.services_[i]),
                0,
                CreateReplicas(i == 1 ? L"I/2" : L"P/2"),
                0)));
        }
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:            N0(M1:50)    N1(M1:40)   N2(M1: 50)
        // App1, S, FU1:                               P(M1:10)
        // App1, -, FU2:                               S(M1:10)
        // App1, S, FU3:                               P(M1:10)
        // App1, SP, FU4:                              P(M1:10)
        // App1, SP, FU5:                              P(M1:10)

        // Application is in upgrade (first two UDs are done)
        //
        std::set<std::wstring> completedUDs;
        completedUDs.insert(L"UD0");
        completedUDs.insert(L"UD1");
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            testHelper.applications_[0],
            testHelper.appMinNodes_,
            testHelper.appScaleout_,
            CreateApplicationCapacities(L""),
            true,
            move(completedUDs)));

        // Partitions that do not get +1 during pre-upgrade phase (stateless),
        // or partitions that are not in upgrade (only set of code packages are in upgrade),
        // or partition which update with +1 is just late
        Reliability::FailoverUnitFlags::Flags statelessFlags;
        Reliability::FailoverUnitFlags::Flags statefullFlags(true);
        statelessFlags.SetUpgrading(true);
        statefullFlags.SetUpgrading(true);
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"I/2"), 0, statelessFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"P/2"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 1, CreateReplicas(L"P/2"), 0, statefullFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        // Initiate primary swap-out for stateful replicas in upgrade
        //
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"P/2/LI"), 1, statefullFlags));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 1, CreateReplicas(L"P/2/LI"), 1, statefullFlags));
        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 add secondary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move instance 2=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 move primary 2=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 2=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"4 add secondary 0", value)));

        // Current placement:
        // Nodes:            N0(M1:50)    N1(M1:40)   N2(M1: 50)
        // App1, S, FU1:      S/B                      P/LI
        // App1, -, FU2:      I/B
        // App1, S, FU3:      S/B                      P/LI
        // App1, SP, FU4:                              P/LI
        // App1, SP, FU5:     S/B                      P/LI
        // App1, S, FU6: +1

        // FM accept all the actions, except for the partition 4 (which was dropped)
        // and new failover unit update is sent for it with +1
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 2, CreateReplicas(L"P/2/LI,S/0/B"), 0, statefullFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 2, CreateReplicas(L"I/0/B"), 0, statelessFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 2, CreateReplicas(L"P/2/LI,S/0/B"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 2, CreateReplicas(L"P/2/LI"), 1, statefullFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(4), FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 2, CreateReplicas(L"P/2/LI,S/0/B"), 0, statefullFlags)));
        fm_->UpdatePlb();
        fm_->Clear();

        // Add a couple of nodes to introduce more randomness
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(3, L"FD3", L"UD3", L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(4, L"FD4", L"UD4", L"M1/100"));
        plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(5, L"FD5", L"UD5", L"M1/100"));

        // Expectation is that no replicas will be moved as only 1 is movable
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // One new partition is created in the meanwhile, and all replicas that are in upgrade are movable now,
        // but expectation is that new replica will not initiate movement of other replica in upgrade
        fm_->FuMap.insert(make_pair(CreateGuid(0), FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 3, CreateReplicas(L"P/0"), 0, statefullFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(1), FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 3, CreateReplicas(L"I/0"), 0, statelessFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(2), FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 3, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3), FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 3, CreateReplicas(L"P/0"), 0, statefullFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(4), FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 3, CreateReplicas(L"P/0"), 0, statefullFlags)));
        fm_->FuMap.insert(make_pair(CreateGuid(5), FailoverUnitDescription(CreateGuid(5), wstring(testHelper.services_[5]), 0, CreateReplicas(L""), 1)));
        fm_->UpdatePlb();
        fm_->Clear();

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeWhileMovementIsInProgress, TestFlavor, TestFlavors)
    {
        // Scenario is testing what happens when upgrade starts while some replica,
        // in an affinity or scaleout one correlation, is currently in movement state
        // Expectations is that none of the replicas is moved
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeWhileMovementIsInProgress{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeWhileMovementIsInProgress,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 3;
        testHelper.numberOfServices_ = 4;
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/LI"), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 0, CreateReplicas(L"S/0/B,P/1/V"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 0, CreateReplicas(L"P/0/LI"), 1)));
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:  N0         N1          N2
        //   +1    P/LI
        //         P
        //         S/B       P/V
        //   +1    P/LI

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeResotreLocationAfterUpgrade, TestFlavor, TestFlavors)
    {
        // Scenario is testing atomic movements of related replicas after upgrade,
        // (in an affinity or scaleout one correlation), due to RestoreReplicaLocationAfterUpgrade flag
        // Expectations is that all replicas will be moved to the preferred location
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeResotreLocationAfterUpgrade{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeResotreLocationAfterUpgrade,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 10;
        testHelper.appCapacities_.push_back(L"M1/50/50/0");
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(L"M1/50");
        }
        // Service setup
        testHelper.numberOfServices_ = 5;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
            testHelper.isStateful_.push_back(i != 3 ? true : false);
            testHelper.isPersisted_.push_back((i == 1 || i == 4) ? true : false);
        }
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        std::vector<bool> isReplicaUpVolatile;
        isReplicaUpVolatile.push_back(false);
        isReplicaUpVolatile.push_back(true);
        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"D/0/ZLJ,P/1", isReplicaUpVolatile), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 0, CreateReplicas(L"P/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 0, CreateReplicas(L"I/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 0, CreateReplicas(L"SB/0/LJ,P/1"), 1)));
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:         N0         N1          N2
        // Volatile(+1)   D/ZLJ      P
        // Persisted                 P
        // Volatile                  P
        // Stateless                 I
        // Persisted(+1)  SB/LJ      P

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0|4 add primary 0", value)));
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1|2 move primary 1=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move instance 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeIncludeAllStandByInAppCapacity, TestFlavor, TestFlavors)
    {
        // Scenario is testing atomic movements of related replicas after upgrade,
        // (in an affinity or scaleout one correlation), due to RestoreReplicaLocationAfterUpgrade flag
        // Expectations is that all replicas will be moved to the preferred location
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeIncludeAllStandByInAppCapacity{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeResotreLocationAfterUpgrade,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 10;
        testHelper.appCapacities_.push_back(L"M1/50/50/0");
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(L"M1/50");
        }
        // Service setup
        testHelper.numberOfServices_ = 5;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
            testHelper.isStateful_.push_back(i != 1 ? true : false);
            testHelper.isPersisted_.push_back((i > 1) ? true : false);
        }
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        std::vector<bool> isReplicaUpVolatile;
        isReplicaUpVolatile.push_back(false);
        isReplicaUpVolatile.push_back(true);
        fm_->FuMap.insert(make_pair(CreateGuid(0),
            FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"D/0/ZLJ,P/1", isReplicaUpVolatile), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(1),
            FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"S/1"), 0)));
        fm_->FuMap.insert(make_pair(CreateGuid(2),
            FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 0, CreateReplicas(L"SB/0/LJ,P/1"), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(3),
            FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 0, CreateReplicas(L"SB/0/LJ,P/1"), 1)));
        fm_->FuMap.insert(make_pair(CreateGuid(4),
            FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 0, CreateReplicas(L"SB/0/LJ,P/1"), 1)));
        fm_->UpdatePlb();
        fm_->Clear();

        // Initial placement:
        // Nodes:         N0         N1          N2
        // Volatile(+1)   D/ZLJ      P
        // Persisted                 P
        // Volatile                  P
        // Stateless                 I
        // Persisted(+1)  SB/LJ      P

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(5u, actionList.size());
        VERIFY_ARE_EQUAL(4u, CountIf(actionList, ActionMatch(L"0|2|3|4 add primary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 move instance 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeMoveToDifferentUD, TestFlavor, TestFlavors)
    {
        // Scenario is testing that upgrade domain constraint is respected during single replica upgrades,
        // i.e. other upgrade domain is chosen when moving single replicas in upgrade
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeMoveToDifferentUD{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeMoveToDifferentUD,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 10;
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(L"M1/100");
            testHelper.nodeFDs_.push_back(wformatString("DF{0}", i));
            testHelper.nodeUDs_.push_back(i == 5 ? L"UD1" : L"UD0");
        }
        // Application setup
        testHelper.needApplication_ = true;
        testHelper.singleApplication_ = true;
        // Service setup
        testHelper.numberOfServices_ = 10;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
            testHelper.isStateful_.push_back((i == 0 || i % 3 != 0) ? true : false);
            testHelper.isPersisted_.push_back(i % 2 == 0);
        }
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wstring(testHelper.services_[i]),
                1,
                CreateReplicas(wformatString("{0}", (i > 0 && i % 3 == 0) ? L"S/0" : (i % 4 == 0 ? L"P/0/LI" : L"P/0"))),
                (i % 4 == 0) ? 1 : 0));
        }

        // Initial placement:
        // Nodes:     N0:UD0  N1:UD0...  N5:UD1  ... N9:UD0
        // Stateful:   7xP
        // Stateless:  3xI

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(10u, actionList.size());
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"0|4|8 add secondary 5", value)));
        VERIFY_ARE_EQUAL(4u, CountIf(actionList, ActionMatch(L"1|2|5|7 move primary 0=>5", value)));
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"3|6|9 move instance 0=>5", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeMultipleAppUpgrades, TestFlavor, TestFlavors)
    {
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeMultipleAppUpgrades{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeMultipleAppUpgrades,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 4;
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            testHelper.nodeCapacities_.push_back(i == 0 ? L"M1/20" : L"M1/60");
        }
        // Application setup
        testHelper.singleApplication_ = false;
        for (int i = 0; i < 2; i++)
        {
            std::set<std::wstring> completedUDApp;
            completedUDApp.insert(L"UD0");
            completedUDApp.insert(L"UD1");
            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                wformatString(L"TestApp{0}", i),
                flavor == L"CheckAffinity" ? 0 : 1,
                flavor == L"CheckAffinity" ? 0 : 1,
                CreateApplicationCapacities(
                    flavor == L"CheckAffinity" ?
                    L"M1/60/30/0" :
                    L"M1/30/30/0"), // check relaxed capacity
                true,
                move(completedUDApp)));
        }
        // Service setup
        testHelper.numberOfServices_ = 6;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.applications_.push_back(wformatString(L"TestApp{0}", i % 2));
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
        }
        RunTestHelper(plb, testHelper);

        // Replica setup
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(
                CreateGuid(i),
                wstring(testHelper.services_[i]),
                1,
                CreateReplicas(i < 3 ? L"P/2/LI" : L"P/2"),
                i < 3 ? 1 : 0));
        }

        // Initial placement:
        // Nodes:  N0(M1:20)  N1(M1:60)   N2(M1: 60)  N3(M1:60)
        // App1:                          3xP(M1:10)
        // App2:                          3xP(M1:10)

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        // Expecting all partitions to be moved to the node1 in already upgraded domain UD1
        VERIFY_ARE_EQUAL(6u, actionList.size());
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"0|1|2 add secondary 1", value)));
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"3|4|5 move primary 2=>1", value)));

        // FM accepts all the movements from TestApp1, and it finish its upgrade
        // Regarding TestApp2, FM reject one movement, hence new failover unit update is sent for it with +1
        fm_->Clear();
        Reliability::FailoverUnitFlags::Flags updateFlags(true);
        updateFlags.SetUpgrading(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 2, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 2, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 2, CreateReplicas(L"P/1"), 0));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 2, CreateReplicas(L"P/2/LI,S/1/B"), 0, updateFlags));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 2, CreateReplicas(L"P/2/LI,S/1/B"), 0, updateFlags));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(testHelper.services_[5]), 2, CreateReplicas(L"P/2/LI"), 1, updateFlags));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // FM accepts all the movements from TestApp1, and it finish its upgrade
        // Regarding TestApp2, FM reject one movement, hence new failover unit update is sent for it with +1
        fm_->Clear();
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 3, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 3, CreateReplicas(L"P/1"), 0, updateFlags));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 3, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 3, CreateReplicas(L"P/1"), 0, updateFlags));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(4), wstring(testHelper.services_[4]), 3, CreateReplicas(L"P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(5), wstring(testHelper.services_[5]), 3, CreateReplicas(L"P/2/LI"), 1, updateFlags));

        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"5 add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeToBeDroppedWithoutNewReplica, TestFlavor, TestFlavors)
    {
        // Scenario is testing atomic movements of related replicas after upgrade,
        // (in an affinity or scaleout one correlation), in cases where one replica got J flag,
        // but +1 replica difference is still missing
        // Expectations is that all replicas will be moved atomically
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeToBeDroppedWithoutNewReplica{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeToBeDroppedWithoutNewReplica,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 3;
        // Service setup
        testHelper.numberOfServices_ = 3;
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        std::vector<bool> isReplicaUp;
        isReplicaUp.push_back(false);
        isReplicaUp.push_back(true);
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"D/0/LJ,P/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"D/0/ZLJ,P/1"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"D/0/ZLJ,P/1"), 1));

        // Expecting than all replicas are moved
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"1|2 add primary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeStatelessMoveInProgress, TestFlavor, TestFlavors)
    {
        // Scenario is testing atomic movements of related replicas during upgrade,
        // (in an affinity or scaleout one correlation), in cases where load balancing is disabled,
        // and persisted movements are dropped from initial run, but stateless one has already started movement
        // Expectations is that all persisted replicas are moved only when stateless replica is built
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeStatelessMoveInProgress{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeStatelessMoveInProgress,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.SetMovementEnabled(true, false);
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 3;
        testHelper.nodeCapacities_.push_back(L"M1/40");
        testHelper.nodeCapacities_.push_back(L"M1/40");
        testHelper.nodeCapacities_.push_back(L"M1/20");
        // Service setup
        testHelper.numberOfServices_ = 4;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.defaultMetrics_.push_back(L"M1/1/10/10");
            testHelper.isStateful_.push_back(i < 3 ? true : false);
            testHelper.isPersisted_.push_back(i < 3 ? true : false);
        }
        testHelper.clusterUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"P/0/LI"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"P/0/LI"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"P/0/LI"), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 2, CreateReplicas(L"I/0/V,I/1/B"), 0));

        // Expecting that non of the persisted replicas is moved
        // until stateless is build
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(0u, actionList.size());

        // Stateless replica has finished building
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 3, CreateReplicas(L"I/1"), 0));
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(3u, actionList.size());
        VERIFY_ARE_EQUAL(3u, CountIf(actionList, ActionMatch(L"* add secondary 1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeStatelessParentWithVolatileChildInUpgrade, TestFlavor, TestFlavors)
    {
        // Scenario is testing atomic movements of related replicas after the upgrade,
        // when parent replica is stateless and not in upgrade, and child is volatile replica in upgrade
        // Expectations is that all replicas are moved
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeStatelessParentWithVolatileChildInUpgrade{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeStatelessMoveInProgress,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 2;
        // Service setup
        testHelper.numberOfServices_ = 4;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.isStateful_.push_back(i < 2 ? false : true);
            testHelper.isPersisted_.push_back(i < 3 ? false : true);
        }
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"I/1"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"D/0/ZLJ,P/1",false), 1));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 1, CreateReplicas(L"P/1"), 0));

        // Expecting that non of the all replicas are moved
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());
        VERIFY_ARE_EQUAL(2u, CountIf(actionList, ActionMatch(L"0|1 move instance 1=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"2 add primary 0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 move primary 1=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(SingleReplicaUpgradeStatelessParentWithPersistedChildDroppedMove)
    {
        // Scenario is testing corner case, when stateless service is parent and persisted child movement is dropped,
        // but other children have already been moved to other node
        // Expectations is that this case will be recognized and persisted child is moved
        wstring testName = L"SingleReplicaUpgradeStatelessParentWithPersistedChildDroppedMove";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        wstring flavor = L"CheckAffinity";
        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 2;
        // Service setup
        testHelper.numberOfServices_ = 4;
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.isStateful_.push_back(i < 2 ? false : true);
            testHelper.isPersisted_.push_back(i < 3 ? false : true);
        }
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"I/0,I/1/V"), 0)); // Stateless parent
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"I/0"), 0)); // Stateless child
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"P/0,S/1/L"), 0)); // Volatile child
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(testHelper.services_[3]), 1, CreateReplicas(L"P/1/LI"), 1)); // Persisted child

        // Expecting that non of the all replicas are moved
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"3 add secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingleReplicaUpgradeVolatilePrimaryToBeSwappedOut, TestFlavor, TestFlavors)
    {
        // Scenario is testing case, when volatile service has created secondary replica,
        // and it is now ready to do primary swap out
        // Expectations is that swap out should be performed
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingleReplicaUpgradeVolatilePrimaryToBeSwappedOut{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);

        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, false);
        PLBConfigScopeChange(PLBActionRetryTimes, int, 3);
        PLBConfigCondScopeChange(
            SingleReplicaUpgradeStatelessMoveInProgress,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        SingleReplicaUpgradeTestHelper testHelper(flavor);
        // Nodes setup
        testHelper.numberOfNodes_ = 2;
        // Service setup
        testHelper.numberOfServices_ = 3;
        testHelper.isStateful_.push_back(true);  // Volatile
        testHelper.isPersisted_.push_back(false);
        testHelper.isStateful_.push_back(false); // Stateless
        testHelper.isPersisted_.push_back(false);
        testHelper.isStateful_.push_back(true);  // Persisted
        testHelper.isPersisted_.push_back(true);
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        // Replica setup
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(testHelper.services_[0]), 1, CreateReplicas(L"S/0,P/1/LI"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(testHelper.services_[1]), 1, CreateReplicas(L"I/0"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(testHelper.services_[2]), 1, CreateReplicas(L"P/0"), 0, upgradingFlag_));

        // Expecting that volatile primary will be swapped out
        fm_->RefreshPLB(Stopwatch::Now());
        VerifyPLBAction(plb, L"NewReplicaPlacement");
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 1<=>0", value)));
    }

    BOOST_AUTO_TEST_CASE(DropingReplicaDuringUpgradeMoveInProgress)
    {
        // Test simulates a possible situation during partition upgrade.
        // We want to verify that replica drop could be one of the actions.

        wstring testName = L"DropingReplicaDuringUpgradeMoveInProgress";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"S/0/J/P, S/1, P/2, S/3/V, I/4"), -1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // Primary should be swapped with secondary on node 0 and secondary on node 3 should be dropped
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>0", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 drop secondary 3", value)));
    }

    BOOST_AUTO_TEST_CASE(DropingReplicaDuringUpgradePrimaryToBeSwappedOut)
    {
        // Test simulates a possible situation during partition upgrade.
        // We want to verify that replica drop could be one of the actions.

        wstring testName = L"DropingReplicaDuringUpgradePrimaryToBeSwappedOut";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        Guid ftId = CreateGuid(0);
        plb.UpdateFailoverUnit(FailoverUnitDescription(ftId, wstring(serviceName), 0, CreateReplicas(L"S/0/P/V, S/1, P/2/I, S/3, I/4"), -1, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());

        // Secondary on node 0 should be dropped and primary should be swapped with some of the others secondaries
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 swap primary 2<=>1|3", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"0 drop secondary 0", value)));
    }

    BOOST_AUTO_TEST_CASE(SingletonReplicaUpgradeSwapMoveInProgressBasicTest)
    {
        // Check that singleton replica primary cannot be swapped out,
        // to the secondary marked with move in progress ('V') flag,
        // hence void movement is generated, to clear the flags
        wstring testName = L"SingletonReplicaUpgradeSwapMoveInProgressBasicTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        wstring parentType = wformatString("{0}_ServiceType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        wstring parentService = wformatString("{0}_TestService", testName);
        plb.UpdateService(ServiceDescription(
            wstring(parentService),
            wstring(parentType),
            wstring(L""),
            true,                   // isStateful
            wstring(L""),           // placementConstraints
            wstring(L""),           // affinitizedService
            true,                   // alignedAffinity
            CreateMetrics(L""),
            FABRIC_MOVE_COST_LOW,
            false,                  // onEveryNode
            1,                      // partitionCount
            1,                      // targetReplicaSetSize
            false));                 // hasPersistedState
 
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0/I,S/1/V"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(1u, actionList.size());

        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 void movement on 1", value)));
    }

    BOOST_AUTO_TEST_CASE_TEMPLATE(SingletonReplicaUpgradeSwapMoveInProgress, TestFlavor, TestFlavors)
    {
        // Check that singleton parent primary replica cannot be swapped out,
        // to the secondary marked with move in progress ('V') flag,
        // in both upgrade optimizations (check affinity and relaxed scaleout),
        // hence void movement is generated, to clear the flags
        TestFlavor testFlavor;
        wstring flavor = testFlavor.value;
        wstring testName = wformatString(L"SingletonReplicaUpgradeSwapMoveInProgress{0}", flavor);
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);
        PLBConfigCondScopeChange(
            SingleReplicaUpgrade,
            [&]()->bool {return flavor == L"CheckAffinity"; },
            CheckAffinityForUpgradePlacement,
            RelaxScaleoutConstraintDuringUpgrade,
            bool,
            true);

        SingleReplicaUpgradeTestHelper testHelper(flavor);
        testHelper.numberOfNodes_ = 2;
        testHelper.isAppUpgradeInProgress_ = true;
        RunTestHelper(plb, testHelper);

        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(0), wstring(testHelper.services_[0]), 0, CreateReplicas(L"P/0/I,S/1/V"), 0, upgradingFlag_));
        plb.UpdateFailoverUnit(FailoverUnitDescription(
            CreateGuid(1), wstring(testHelper.services_[1]), 0, CreateReplicas(L"P/0/I,S/1"), 0, upgradingFlag_));

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(2u, actionList.size());
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"0 void movement on 1", value)));
        VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(L"1 swap primary 0<=>1", value)));
    }

    BOOST_AUTO_TEST_CASE(UpgradePrimaryToBeSwappedOutBalancingTest1)
    {
        wstring testName = L"UpgradePrimaryToBeSwappedOutBalancingTest1";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring serviceType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        wstring serviceName = wformatString("{0}Service", testName);
        plb.UpdateService(CreateServiceDescription(serviceName, serviceType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(serviceName), 0, CreateReplicas(L"P/0, S/1, S/2, S/3, S/4"), 0, upgradingFlag_));
        for (int ftId = 1; ftId < 5; ++ftId)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(ftId), wstring(serviceName), 0, CreateReplicas(L"P/0/I, S/1, S/2, S/3, S/4"), 0, upgradingFlag_));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(4u, actionList.size());

        // Secondary on node 0 should be dropped and primary should be swapped with some of the others secondaries
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>1", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>2", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>3", value)));
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"* swap primary 0<=>4", value)));

    }

    BOOST_AUTO_TEST_CASE(SingleChildUpgradeWithCheckAffinityTest)
    {
        // Check that all affinitized replicas are moved at once,
        // when CheckAffinityForUpgradePlacement flag is enabled,
        // and only one child replica is in singleton replica upgrade
        wstring testName = L"SingleChildUpgradeWithCheckAffinityTest";
        Trace.WriteInfo("PLBUpgradeTestSource", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        PLBConfigScopeChange(MinConstraintCheckInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(MinLoadBalancingInterval, TimeSpan, TimeSpan::MaxValue);
        PLBConfigScopeChange(CheckAffinityForUpgradePlacement, bool, true);
        PLBConfigScopeChange(RelaxAffinityConstraintDuringUpgrade, bool, true);
        FailoverConfigScopeChange(IsSingletonReplicaMoveAllowedDuringUpgradeEntry, bool, true);

        plb.UpdateNode(CreateNodeDescription(0));
        plb.UpdateNode(CreateNodeDescription(1));

        wstring parentType = wformatString("{0}_ParentType", testName);
        wstring childType = wformatString("{0}_ChildType", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(parentType), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(childType), set<NodeId>()));

        wstring parentService = L"ParentService";
        plb.UpdateService(CreateServiceDescription(parentService, parentType, true, CreateMetrics(L""), FABRIC_MOVE_COST_LOW, false, 1, false));
        for (int i = 1; i < 7; ++i)
        {
            plb.UpdateService(CreateServiceDescriptionWithAffinity(wformatString("ChildService{0}", i), childType, true, parentService, CreateMetrics(L""), false, 1, false, false));
        }

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(parentService), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"ChildService1"), 0, CreateReplicas(L"P/0/LI"), 1, upgradingFlag_));
        for (int i = 2; i < 7; ++i)
        {
            plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(i), wformatString("ChildService{0}", i), 0, CreateReplicas(L"P/0"), 0));
        }

        fm_->RefreshPLB(Stopwatch::Now());

        vector<wstring> actionList = GetActionListString(fm_->MoveActions);
        VERIFY_ARE_EQUAL(7u, actionList.size());
        VERIFY_ARE_EQUAL(1u, CountIf(actionList, ActionMatch(L"1 add secondary 1", value)));
        VERIFY_ARE_EQUAL(6u, CountIf(actionList, ActionMatch(L"0|2|3|4|5|6 move primary 0=>1", value)));
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestPLBUpgrade::ClassSetup()
    {
        Trace.WriteInfo("PLBUpgradeTestSource", "Random seed: {0}", PLBConfig::GetConfig().InitialRandomSeed);

        fm_ = make_shared<TestFM>();

        upgradingFlag_.SetUpgrading(true);
        swappingPrimaryFlag_.SetSwappingPrimary(true);

        return TRUE;
    }

    bool TestPLBUpgrade::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestPLBUpgrade::ClassCleanup()
    {
        Trace.WriteInfo("PLBUpgradeTestSource", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }

    void TestPLBUpgrade::RunTestHelper(
        PlacementAndLoadBalancing & plb,
        SingleReplicaUpgradeTestHelper & testHelper)
    {
        Common::ErrorCode serviceRet;
        Common::ErrorCode applicationRet;

        bool hasNodeCapacities = testHelper.nodeCapacities_.size() == 0 ? false : true;
        bool hasNodeFDs = testHelper.nodeFDs_.size() == 0 ? false : true;
        bool hasNodeUDs = testHelper.nodeUDs_.size() == 0 ? false : true;
        bool hasNodeDeactivationIntent = testHelper.nodeDeactivationIntent_.size() == 0 ? false : true;
        bool hasNodeDeactivationStatus = testHelper.nodeDeactivationStatus_.size() == 0 ? false : true;
        bool hasAppCapacities = testHelper.appCapacities_.size() == 0 ? false : true;
        bool hasTargetReplicaCount = testHelper.targetReplicaCount_.size() == 0 ? false : true;
        bool hasDefaultMetrics = testHelper.defaultMetrics_.size() == 0 ? false : true;
        bool hasStatefulnessInfo = testHelper.isStateful_.size() == 0 ? false : true;
        bool hasPersistedInfo = testHelper.isPersisted_.size() == 0 ? false : true;
        bool hasApplications = testHelper.applications_.size() == 0 ? false : true;

        // Setup test nodes
        for (int i = 0; i < testHelper.numberOfNodes_; i++)
        {
            if (!hasNodeCapacities)
            {
                testHelper.nodeCapacities_.push_back(L"");
            }
            if (!hasNodeFDs)
            {
                testHelper.nodeFDs_.push_back(wformatString(L"FD{0}", i));
            }
            if (!hasNodeUDs)
            {
                testHelper.nodeUDs_.push_back(wformatString(L"UD{0}", i));
            }
            if (!hasNodeDeactivationIntent)
            {
                testHelper.nodeDeactivationIntent_.push_back(Reliability::NodeDeactivationIntent::Enum::None);
            }
            if (!hasNodeDeactivationStatus)
            {
                testHelper.nodeDeactivationStatus_.push_back(Reliability::NodeDeactivationStatus::Enum::None);
            }

            plb.UpdateNode(CreateNodeDescriptionWithDomainsAndCapacity(
                i,
                testHelper.nodeFDs_[i],
                testHelper.nodeUDs_[i],
                testHelper.nodeCapacities_[i],
                testHelper.nodeDeactivationIntent_[i],
                testHelper.nodeDeactivationStatus_[i]));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        // Setup test applications
        if (!hasApplications)
        {
            if (testHelper.needApplication_)
            {
                for (int i = 0; i < testHelper.numberOfServices_; i++)
                {
                    testHelper.applications_.push_back(wformatString(L"Application{0}{1}", testHelper.flavor_, i));
                    if (testHelper.isAppGroup_)
                    {
                        if (!hasAppCapacities)
                        {
                            testHelper.appCapacities_.push_back(L"");
                        }
                        applicationRet = plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                            testHelper.applications_[0],
                            testHelper.appMinNodes_,
                            testHelper.appScaleout_,
                            CreateApplicationCapacities(testHelper.appCapacities_[0]),
                            testHelper.isAppUpgradeInProgress_,
                            move(testHelper.appCompletedUDs_)));
                    }
                    else
                    {
                        applicationRet = plb.UpdateApplication(ApplicationDescription(std::wstring(testHelper.applications_[0])));
                    }
                    VERIFY_ARE_EQUAL(applicationRet.IsSuccess(), true);

                    // If it is a single application (not per service) break
                    if (testHelper.singleApplication_)
                    {
                        break;
                    }
                }
            }
            else
            {
                testHelper.applications_.push_back(L"");
            }
        }


        // Setup test services
        for (int i = 0; i < testHelper.numberOfServices_; i++)
        {
            testHelper.serviceTypes_.push_back(wformatString(L"ServiceType{0}{1}", testHelper.flavor_, i));
            plb.UpdateServiceType(ServiceTypeDescription(wstring(testHelper.serviceTypes_[i]), set<NodeId>()));

            testHelper.services_.push_back(wformatString(L"Service{0}{1}", testHelper.flavor_, i));

            if (!hasTargetReplicaCount)
            {
                testHelper.targetReplicaCount_.push_back(1);
            }

            if (!hasDefaultMetrics)
            {
                testHelper.defaultMetrics_.push_back(L"");
            }

            if (!hasStatefulnessInfo)
            {
                testHelper.isStateful_.push_back(true);
            }

            if (!hasPersistedInfo)
            {
                testHelper.isPersisted_.push_back(false);
            }

            if (i == 0)
            {
                serviceRet = plb.UpdateService(ServiceDescription(
                    wstring(testHelper.services_[0]),
                    wstring(testHelper.serviceTypes_[0]),
                    wstring(testHelper.applications_[0]),
                    testHelper.isStateful_[0],                  // isStateful
                    wstring(L""),                               // placementConstraints
                    wstring(L""),                               // affinitizedService
                    true,                                       // alignedAffinity
                    CreateMetrics(testHelper.defaultMetrics_[0]),
                    FABRIC_MOVE_COST_LOW,
                    false,                                      // onEveryNode
                    1,                                          // partitionCount
                    testHelper.targetReplicaCount_[0],          // targetReplicaSetSize
                    testHelper.isPersisted_[0]));               // hasPersistedState
            }
            else
            {
                serviceRet = plb.UpdateService(ServiceDescription(
                    wstring(testHelper.services_[i]),
                    wstring(testHelper.serviceTypes_[i]),
                    wstring(testHelper.singleApplication_ ?
                        testHelper.applications_[0] :
                        testHelper.applications_[i]),
                    testHelper.isStateful_[i],                  // isStateful
                    wstring(L""),                               // placementConstraints
                    wstring(testHelper.hasParentService_ ?      // affinitizedService
                        testHelper.services_[0] :
                        wstring(L"")),
                    true,                                       // alignedAffinity
                    CreateMetrics(testHelper.defaultMetrics_[i]),
                    FABRIC_MOVE_COST_LOW,
                    false,                                      // onEveryNode
                    1,                                          // partitionCount
                    testHelper.targetReplicaCount_[i],          // targetReplicaSetSize
                    testHelper.isPersisted_[i]));               // hasPersistedState
            }
            VERIFY_ARE_EQUAL(serviceRet.IsSuccess(), true);
        }

        // Setup test partitions
        if (testHelper.createReplicas_)
        {
            bool hasReplicaInUpgrade = testHelper.replicaInUpgrade_.size() == 0 ? false : true;
            bool hasReplicaPlacement = testHelper.replicaPlacement_.size() == 0 ? false : true;

            for (int i = 0; i < testHelper.numberOfServices_; i++)
            {
                if (!hasReplicaInUpgrade)
                {
                    testHelper.replicaInUpgrade_.push_back(true);
                }

                if (!hasReplicaPlacement)
                {
                    testHelper.replicaPlacement_.push_back(0);
                }

                if (testHelper.isStateful_[i])
                {
                    if (testHelper.replicaInUpgrade_[i])
                    {
                        plb.UpdateFailoverUnit(FailoverUnitDescription(
                            CreateGuid(i),                              // GUID
                            wstring(testHelper.services_[i]),           // Service name
                            0,                                          // Version
                            CreateReplicas(wformatString(L"P/{0}/LI", testHelper.replicaPlacement_[i])),
                            1,                                          // Replica difference
                            upgradingFlag_));                           // Partition flags
                    }
                    else
                    {
                        plb.UpdateFailoverUnit(FailoverUnitDescription(
                            CreateGuid(i),                              // GUID
                            wstring(testHelper.services_[i]),           // Service name
                            0,                                          // Version
                            CreateReplicas(wformatString(L"P/{0}", testHelper.replicaPlacement_[i])),
                            0,                                          // Replica difference
                            testHelper.singleApplication_ ?             // Partition flags
                                upgradingFlag_ :
                                Reliability::FailoverUnitFlags::Flags::None));
                    }
                }
                else
                {
                    plb.UpdateFailoverUnit(FailoverUnitDescription(
                        CreateGuid(i),                              // GUID
                        wstring(testHelper.services_[i]),           // Service name
                        0,                                          // Version
                        CreateReplicas(wformatString(L"S/{0}", testHelper.replicaPlacement_[i])),
                        0,                                          // Replica difference
                        upgradingFlag_));                           // Partition flags
                }

            }
        }

        // If there is cluster upgrade in progress,
        // update the status
        if (testHelper.clusterUpgradeInProgress_)
        {
            plb.UpdateClusterUpgrade(true, move(testHelper.clusterCompletedUDs_));
        }

        // Run and verify
        if (testHelper.runAndVerify)
        {
            fm_->RefreshPLB(Stopwatch::Now());

            vector<wstring> actionList = GetActionListString(fm_->MoveActions);
            size_t expectedNumberOfMoves = testHelper.numberOfServices_;
            VERIFY_ARE_EQUAL(expectedNumberOfMoves, actionList.size());

            size_t targetOne = 0;
            size_t targetTwo = 0;

            for (int i = 0; i < testHelper.numberOfServices_; i++)
            {
                if (testHelper.replicaInUpgrade_[i] && testHelper.isStateful_[i])
                {
                    VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(wformatString(L"{0} add secondary 1|2", i), value)));
                    targetOne += CountIf(actionList, ActionMatch(wformatString(L"{0} add secondary 1", i), value));
                    targetTwo += CountIf(actionList, ActionMatch(wformatString(L"{0} add secondary 2", i), value));
                }
                else
                {
                    VERIFY_ARE_EQUAL(1, CountIf(actionList, ActionMatch(wformatString(L"{0} move {1} {2}=>1|2", i,
                        testHelper.isStateful_[i] == true ? L"primary" : L"instance", testHelper.replicaPlacement_[i]), value)));
                    targetOne += CountIf(actionList, ActionMatch(wformatString(L"{0} move {1} {2}=>1", i,
                        testHelper.isStateful_[i] == true ? L"primary" : L"instance", testHelper.replicaPlacement_[i]), value));
                    targetTwo += CountIf(actionList, ActionMatch(wformatString(L"{0} move {1} {2}=>2", i,
                            testHelper.isStateful_[i] == true ? L"primary" : L"instance", testHelper.replicaPlacement_[i]), value));
                }
            }
            VERIFY_IS_TRUE((targetOne == expectedNumberOfMoves && targetTwo == 0) || (targetOne == 0 && targetTwo == expectedNumberOfMoves));
        }
    }

}
