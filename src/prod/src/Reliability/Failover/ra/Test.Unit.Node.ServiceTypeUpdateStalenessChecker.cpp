// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Node;
using namespace Infrastructure;
using namespace ServiceModel;
using namespace Common;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestServiceTypeUpdateStalenessChecker
{
protected:
    TestServiceTypeUpdateStalenessChecker() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestServiceTypeUpdateStalenessChecker() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    bool TryUpdateSeqNum(
        ServiceTypeIdentifier & serviceTypeId,
        uint64 sequenceNumber);
    bool TryUpdateSeqNum(
        PartitionId & partitionId,
        uint64 sequenceNumber);

    void AdvanceClock(int seconds);

    void PerformCleanup();

    unique_ptr<ServiceTypeUpdateStalenessChecker> stalenessChecker_;
    unique_ptr<UnitTestContext> testContext_;

    int KeepDuration;

    ServiceTypeIdentifier SP1;
    ServiceTypeIdentifier SL1;
    PartitionId SP1_FUID;
    PartitionId SL1_FUID;

    TimeSpanConfigEntry & GetConfigEntry() const;
};

TimeSpanConfigEntry & TestServiceTypeUpdateStalenessChecker::GetConfigEntry() const
{
    return testContext_->Config.ServiceTypeUpdateStalenessEntryKeepDurationEntry;
}

bool TestServiceTypeUpdateStalenessChecker::TestSetup()
{
    testContext_ = UnitTestContext::Create();

    // Constants
    KeepDuration = 10;
    SP1 = Default::GetInstance().SP1_FTContext.STInfo.ServiceTypeId;
    SL1 = Default::GetInstance().SL1_FTContext.STInfo.ServiceTypeId;
    SP1_FUID = Default::GetInstance().SP1_FTContext.FUID;
    SL1_FUID = Default::GetInstance().SL1_FTContext.FUID;

    // Config
    testContext_->Clock.SetManualMode();
    GetConfigEntry().Test_SetValue(TimeSpan::FromSeconds(KeepDuration));

    // Initialize
    stalenessChecker_ = make_unique<ServiceTypeUpdateStalenessChecker>(
        testContext_->Clock,
        GetConfigEntry());

    return true;
}

bool TestServiceTypeUpdateStalenessChecker::TryUpdateSeqNum(
    ServiceTypeIdentifier & serviceTypeId,
    uint64 sequenceNumber)
{
    return stalenessChecker_->TryUpdateServiceTypeSequenceNumber(serviceTypeId, sequenceNumber);
}

bool TestServiceTypeUpdateStalenessChecker::TryUpdateSeqNum(
    PartitionId & partitionId,
    uint64 sequenceNumber)
{
    return stalenessChecker_->TryUpdatePartitionSequenceNumber(partitionId, sequenceNumber);
}

void TestServiceTypeUpdateStalenessChecker::AdvanceClock(int seconds)
{
    testContext_->Clock.AdvanceTimeBySeconds(seconds);
}

void TestServiceTypeUpdateStalenessChecker::PerformCleanup()
{
    stalenessChecker_->PerformCleanup();
}

bool TestServiceTypeUpdateStalenessChecker::TestCleanup()
{
    testContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestServiceTypeUpdateStalenessCheckerSuite, TestServiceTypeUpdateStalenessChecker)

BOOST_AUTO_TEST_CASE(ServiceType_Insert_New)
{
    bool res = TryUpdateSeqNum(SP1, 0);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Insert_New)
{
    bool res = TryUpdateSeqNum(SP1_FUID, 0);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Insert_OtherServiceType)
{
    TryUpdateSeqNum(SP1, 0);

    bool res = TryUpdateSeqNum(SL1, 1);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Insert_OtherServiceType)
{
    TryUpdateSeqNum(SP1_FUID, 0);

    bool res = TryUpdateSeqNum(SL1_FUID, 1);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Insert_HigherSeqNum)
{
    TryUpdateSeqNum(SP1, 0);
    bool res = TryUpdateSeqNum(SP1, 1);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Insert_HigherSeqNum)
{
    TryUpdateSeqNum(SP1_FUID, 0);
    bool res = TryUpdateSeqNum(SP1_FUID, 1);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Insert_LowerSeqNum)
{
    TryUpdateSeqNum(SP1, 1);
    bool res = TryUpdateSeqNum(SP1, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Insert_LowerSeqNum)
{
    TryUpdateSeqNum(SP1_FUID, 1);
    bool res = TryUpdateSeqNum(SP1_FUID, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Insert_EqualSeqNum)
{
    TryUpdateSeqNum(SP1, 1);
    bool res = TryUpdateSeqNum(SP1, 1);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Insert_EqualSeqNum)
{
    TryUpdateSeqNum(SP1_FUID, 1);
    bool res = TryUpdateSeqNum(SP1_FUID, 1);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Cleanup_NotExpired)
{
    TryUpdateSeqNum(SP1, 1);

    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Cleanup_NotExpired)
{
    TryUpdateSeqNum(SP1_FUID, 1);

    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1_FUID, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Cleanup_Expired)
{
    TryUpdateSeqNum(SP1, 1);

    AdvanceClock(KeepDuration + 1);
    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1, 0);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Cleanup_Expired)
{
    TryUpdateSeqNum(SP1_FUID, 1);

    AdvanceClock(KeepDuration + 1);
    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1_FUID, 0);
    VERIFY_IS_TRUE(res);
}

BOOST_AUTO_TEST_CASE(ServiceType_Cleanup_SomeExpired)
{
    TryUpdateSeqNum(SP1, 1);
    AdvanceClock(KeepDuration/2 + 1);

    TryUpdateSeqNum(SL1, 2);
    AdvanceClock(KeepDuration/2);

    // SP1 entry is (KeepDuration + 1) old
    // SL1 entry is (KeepDuration / 2) old
    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1, 0);
    VERIFY_IS_TRUE(res);

    res = TryUpdateSeqNum(SL1, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_CASE(Partition_Cleanup_SomeExpired)
{
    TryUpdateSeqNum(SP1_FUID, 1);
    AdvanceClock(KeepDuration/2 + 1);

    TryUpdateSeqNum(SL1_FUID, 2);
    AdvanceClock(KeepDuration/2);

    // SP1 entry is (KeepDuration + 1) old
    // SL1 entry is (KeepDuration / 2) old
    PerformCleanup();

    bool res = TryUpdateSeqNum(SP1_FUID, 0);
    VERIFY_IS_TRUE(res);

    res = TryUpdateSeqNum(SL1_FUID, 0);
    VERIFY_IS_FALSE(res);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
