// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Federation;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;

    class FailoverUnitTest
    {
    protected:

        FailoverUnitTest() { BOOST_REQUIRE(ClassSetup()); }
        ~FailoverUnitTest() { BOOST_REQUIRE(ClassCleanup()); }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);

        void TestSafeReplicaCloseCount(wstring const& testName, wstring const& failoverUnitStr, int expectedCount);

        void SetupConfigsForHealthReportTest(int replicaLimit, int reconfigurationTimeLimit);
        void SetTimeBackAndUpdateHealthState(FailoverUnit & ft);

    private:

        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;
    };


    BOOST_FIXTURE_TEST_SUITE(FailoverUnitTestSuite, FailoverUnitTest)

    BOOST_AUTO_TEST_CASE(TestFailoverUnit)
    {
        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"3 2 SP 000/111");

        failoverUnit->VerifyConsistency();
        VERIFY_ARE_EQUAL(PersistenceState::ToBeInserted, failoverUnit->PersistenceState);

        for (int i = 0; i < 10; i++)
        {
            NodeInstance nodeInstance = TestHelper::CreateNodeInstance(i, 1);
            failoverUnit->CreateReplica(TestHelper::CreateNodeInfo(nodeInstance));
            failoverUnit->VerifyConsistency();
        }

        VERIFY_ARE_EQUAL(10u, failoverUnit->UpReplicaCount);
        VERIFY_ARE_EQUAL(0u, failoverUnit->PreviousConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(0u, failoverUnit->CurrentConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(PersistenceState::ToBeInserted, failoverUnit->PersistenceState);

        failoverUnit->PostUpdate(DateTime::Now());
        failoverUnit->PostCommit(failoverUnit->OperationLSN + 1);
        VERIFY_ARE_EQUAL(PersistenceState::NoChange, failoverUnit->PersistenceState);

        int i = 0;
        for (ReplicaIterator it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; it++, i++)
        {
            if (i == 0)
            {
                failoverUnit->UpdateReplicaPreviousConfigurationRole(it, Reliability::ReplicaRole::Secondary); // only update the first replica
                failoverUnit->UpdateReplicaCurrentConfigurationRole(it, Reliability::ReplicaRole::Primary); // only update the first replica
            }
            else if (i == 1)
            {
                failoverUnit->UpdateReplicaPreviousConfigurationRole(it, Reliability::ReplicaRole::Primary); // only update the first replica
                failoverUnit->UpdateReplicaCurrentConfigurationRole(it, Reliability::ReplicaRole::Secondary); // only update the first replica
            }
            else
            {
                failoverUnit->UpdateReplicaPreviousConfigurationRole(it, Reliability::ReplicaRole::Secondary);
                failoverUnit->UpdateReplicaCurrentConfigurationRole(it, Reliability::ReplicaRole::Secondary);
            }
            failoverUnit->VerifyConsistency();
        }

        VERIFY_ARE_EQUAL(10u, failoverUnit->UpReplicaCount);
        VERIFY_ARE_EQUAL(10u, failoverUnit->PreviousConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(9u, failoverUnit->PreviousConfiguration.SecondaryCount);
        VERIFY_ARE_EQUAL(10u, failoverUnit->CurrentConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(9u, failoverUnit->CurrentConfiguration.SecondaryCount);

        i = 0;
        for (ReplicaIterator it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; it++, i++)
        {
            if (i >= 5)
            {
                failoverUnit->UpdateReplicaCurrentConfigurationRole(it, Reliability::ReplicaRole::Idle);
                failoverUnit->UpdateReplicaPreviousConfigurationRole(it, Reliability::ReplicaRole::Idle);
            }
            failoverUnit->VerifyConsistency();
        }

        failoverUnit->PostUpdate(DateTime::Now());
        failoverUnit->PostCommit(failoverUnit->OperationLSN + 1);

        VERIFY_ARE_EQUAL(10u, failoverUnit->UpReplicaCount);
        VERIFY_ARE_EQUAL(5u, failoverUnit->PreviousConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(4u, failoverUnit->PreviousConfiguration.SecondaryCount);
        VERIFY_ARE_EQUAL(5u, failoverUnit->CurrentConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(4u, failoverUnit->CurrentConfiguration.SecondaryCount);

        // copy to a new partition
        FailoverUnitUPtr failoverUnit2 = make_unique<FailoverUnit>(FailoverUnit(*failoverUnit));
        failoverUnit2->VerifyConsistency();
        VERIFY_ARE_EQUAL(10u, failoverUnit2->UpReplicaCount);
        VERIFY_ARE_EQUAL(5u, failoverUnit2->PreviousConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(4u, failoverUnit2->PreviousConfiguration.SecondaryCount);
        VERIFY_ARE_EQUAL(5u, failoverUnit2->CurrentConfiguration.ReplicaCount);
        VERIFY_ARE_EQUAL(4u, failoverUnit2->CurrentConfiguration.SecondaryCount);

        failoverUnit2->SetToBeDeleted();
        VERIFY_ARE_EQUAL(true, failoverUnit2->IsToBeDeleted);
    }

    BOOST_AUTO_TEST_CASE(HealthReportShowDroppedReplicasOnStatefulFt)
    {
        //Limit should not count for Stateful Fts
        SetupConfigsForHealthReportTest(0, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"4 2 S 111/222 [1 S/P RD - Up] [2 S/S DD - Down] [3 S/S IB - Up] [4 P/S RD - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/P Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/S Dropped  2");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/S InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"P/S Ready  4");
    }

    BOOST_AUTO_TEST_CASE(HealthReportDontShowDroppedReplicasNotOnConfigurationOnStatefulFt)
    {
        //Limit should not count for Stateful Fts
        SetupConfigsForHealthReportTest(0, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"4 2 S 111/222 [1 S/P RD - Up] [2 I/I DD - Down] [3 S/S IB - Up] [4 P/S RD - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/P Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/S InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"P/S Ready  4");
    }

    BOOST_AUTO_TEST_CASE(HealthReportShowNotInConfigurationNotDroppedReplicaStatefulFt)
    {
        //Limit should not count for Stateful Fts
        SetupConfigsForHealthReportTest(0, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"3 2 S 111/222 [1 S/P RD - Up] [2 P/S RD - Up] [3 I/I SB - Down]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"S/P Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"P/S Ready  2");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"I/I Down  3");
    }

    BOOST_AUTO_TEST_CASE(HealthReportLimitReplicasStatelessFt)
    {
        SetupConfigsForHealthReportTest(1, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N DD - Down] [3 N/N IB - Up] [4 N/N RD - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 1 out of 3 instances. Total available instances: 2)");
    }

    BOOST_AUTO_TEST_CASE(HealthReportIBReplicaCountStatelessFt)
    {
        SetupConfigsForHealthReportTest(2, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"5 0 - 000/111 [1 N/N IB - Up] [2 N/N IB - Up] [3 N/N DD - Down] [4 N/N RD - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  2");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 2 out of 3 instances. Total available instances: 1)");
    }

    BOOST_AUTO_TEST_CASE(HealthReportFillAvailableLimitStatelessFt)
    {
        SetupConfigsForHealthReportTest(3, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"7 0 - 000/111 [1 N/N RD - Up] [2 N/N DD - Down] [3 N/N IB - Up] [4 N/N RD - Up] [5 N/N IB - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  5");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 3 out of 4 instances. Total available instances: 2)");
    }

    BOOST_AUTO_TEST_CASE(HealthReportLimitHigherThanAvailableReplicasStatelessFt)
    {
        SetupConfigsForHealthReportTest(10, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"7 0 - 000/111 [1 N/N RD - Up] [2 N/N DD - Down] [3 N/N IB - Up] [4 N/N RD - Up] [5 N/N IB - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  5");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  4");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 4 out of 4 instances. Total available instances: 2)");
    }

    BOOST_AUTO_TEST_CASE(HealthReportNoAvailableReplicasStatelessFt)
    {
        SetupConfigsForHealthReportTest(10, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"7 0 - 000/111 [1 N/N DD - Down] [2 N/N DD - Down] [3 N/N IB - Up] [4 N/N DD - Down] [5 N/N IB - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"InBuild  5");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 2 out of 2 instances. Total available instances: 0)");
    }

    BOOST_AUTO_TEST_CASE(HealthReportNoUnavailableReplicasStatelessFt)
    {
        SetupConfigsForHealthReportTest(10, 10);

        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(L"7 0 - 000/111 [1 N/N RD - Up] [2 N/N DD - Down] [3 N/N RD - Up] [4 N/N DD - Down] [5 N/N RD - Up]");

        SetTimeBackAndUpdateHealthState(*failoverUnit);

        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  1");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  3");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"Ready  5");
        TestHelper::VerifyStringContains(failoverUnit->ExtraDescription, L"(Showing 3 out of 3 instances. Total available instances: 3)");
    }

    BOOST_AUTO_TEST_CASE(StatelessSafeReplicaCloseCountTest)
    {
        FailoverConfig::GetConfig().IsStrongSafetyCheckEnabled = false;

        TestSafeReplicaCloseCount(
            L"Target=-1, Up=1",
            L"-1 0 - 000/111 [1 N/N RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=-1, Up=2",
            L"-1 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=1, Up=1",
            L"1 0 - 000/111 [1 N/N RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=2, Up=1",
            L"2 0 - 000/111 [1 N/N RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=2, Up=2",
            L"2 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Up=1",
            L"3 0 - 000/111 [1 N/N RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=3, Up=2",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Up=3",
            L"3 0 - 000/111 [1 N/N RD - Up] [2 N/N RD - Up] [3 N/N RD - Up]",
            2);

        TestSafeReplicaCloseCount(
            L"Target=2, Up=2, Available=1",
            L"2 0 - 000/111 [1 N/N IB - Up] [2 N/N RD - Up]",
            1);
    }

    BOOST_AUTO_TEST_CASE(StatefulSafeReplicaCloseCountTest)
    {
        FailoverConfig::GetConfig().IsStrongSafetyCheckEnabled = false;

        TestSafeReplicaCloseCount(
            L"Target=1, Min=1, Up=1",
            L"1 1 S 000/111 [1 N/P RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=2, Min=1, Up=1",
            L"2 1 S 000/111 [1 N/P RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=2, Min=1, Up=2",
            L"2 1 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=1, Up=1",
            L"3 1 S 000/111 [1 N/P RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=1, Up=2",
            L"3 1 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=1, Up=3",
            L"3 1 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=2, Up=1",
            L"3 2 S 000/111 [1 N/P RD - Up] [2 N/S DD - Down]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=2, Up=2",
            L"3 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=2, Up=3",
            L"3 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=1",
            L"3 3 S 000/111 [1 N/P RD - Up] [2 N/S DD - Down] [3 N/S DD - Down]",
            -2);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=2",
            L"3 3 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S DD - Down]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=3",
            L"3 3 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=2, Up=3",
            L"4 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=2, Up=4",
            L"4 2 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=3, Up=4",
            L"4 3 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=5, Min=3, Up=4",
            L"5 3 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=5, Min=3, Up=5",
            L"5 3 S 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up] [4 N/S RD - Up] [5 N/S RD - Up]",
            2);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=3, Available=2",
            L"3 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD P Up] [3 N/S IB - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=3, Up=4, ccUp=3, ccAvailable=2",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S IB - Up] [4 N/I IB - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=3, Up=3, ccUp=3, ccAvailable=2",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S IB - Up] [4 N/I RD - Down]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=1, Up=1, Available=0",
            L"3 1 SP 000/111 [1 N/P IB - Up]",
            0);

        TestSafeReplicaCloseCount(
            L"Target=1, Min=1, Up=1, Available=0",
            L"1 1 SP 000/111 [1 N/P IB - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=3, Available=3, Reconfiguring",
            L"3 3 SP 111/122 [1 P/S RD - Up] [2 S/P RD - Up] [3 S/S RD - Up]",
            1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=2, Up=2, Available=2, Reconfiguring",
            L"3 2 SP 111/122 [1 P/S RD - Down] [2 S/P RD - Up] [3 S/S RD - Up]",
            0);
    }

    BOOST_AUTO_TEST_CASE(StrongSafetyCheckEnabledTest)
    {
        FailoverConfig::GetConfig().IsStrongSafetyCheckEnabled = true;

        TestSafeReplicaCloseCount(
            L"Target=3, Min=3, Up=3, Available=2",
            L"3 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD P Up] [3 N/S IB - Up]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=3, Up=4, ccUp=3, ccAvailable=2",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S IB - Up] [4 N/I IB - Up]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=4, Min=3, Up=3, ccUp=3, ccAvailable=2",
            L"4 3 SP 000/111 [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S IB - Up] [4 N/I RD - Down]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=3, Min=1, Up=1, Available=0",
            L"3 1 SP 000/111 [1 N/P IB - Up]",
            -1);

        TestSafeReplicaCloseCount(
            L"Target=1, Min=1, Up=1, Available=0",
            L"1 1 SP 000/111 [1 N/P IB - Up]",
            -1);

        TestSafeReplicaCloseCount(
            L"Stateless: Target=2, Up=2, Available=1",
            L"2 0 - 000/111 [1 N/N IB - Up] [2 N/N RD - Up]",
            0);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void FailoverUnitTest::TestSafeReplicaCloseCount(wstring const& testName, wstring const& failoverUnitStr, int expectedCount)
    {
        FailoverUnitUPtr failoverUnit = TestHelper::FailoverUnitFromString(failoverUnitStr);

        int actualCount = failoverUnit->GetSafeReplicaCloseCount(*fm_, *failoverUnit->ServiceInfoObj->ServiceType->Application);

        TestHelper::AssertEqual(expectedCount, actualCount, testName);
    }

    bool FailoverUnitTest::ClassSetup()
    {
        FailoverConfig::Test_Reset();
        FailoverConfig & failoverConfig = FailoverConfig::GetConfig();
        failoverConfig.IsTestMode = true;
        failoverConfig.DummyPLBEnabled = true;
        failoverConfig.PeriodicStateScanInterval = TimeSpan::MaxValue;

        Reliability::LoadBalancingComponent::PLBConfig::Test_Reset();
        auto & plbConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
        plbConfig.IsTestMode = true;
        plbConfig.DummyPLBEnabled = true;
        plbConfig.PLBRefreshInterval = (TimeSpan::MaxValue);
        plbConfig.ProcessPendingUpdatesInterval = (TimeSpan::MaxValue);

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        return true;
    }

    bool FailoverUnitTest::ClassCleanup()
    {
        fm_->Close(true /* isStoreCloseNeeded */);

        FailoverConfig::Test_Reset();
        Reliability::LoadBalancingComponent::PLBConfig::Test_Reset();

        return true;
    }

    void FailoverUnitTest::SetupConfigsForHealthReportTest(int replicaLimit, int timeLimit)
    {
        FailoverConfig::GetConfig().ReconfigurationTimeLimit = Common::TimeSpan::FromSeconds(timeLimit);
        FailoverConfig::GetConfig().PlacementTimeLimit = Common::TimeSpan::FromSeconds(timeLimit);
        FailoverConfig::GetConfig().MaxReplicasInHealthReportDescription = replicaLimit;

    }
    void FailoverUnitTest::SetTimeBackAndUpdateHealthState(FailoverUnit & ft)
    {
        auto time = DateTime::Now() - TimeSpan::FromMinutes(1);
        ft.Test_SetReconfigurationStartTime(time);
        ft.PlacementStartTime = time;

        ft.UpdateHealthState();
    }
}
