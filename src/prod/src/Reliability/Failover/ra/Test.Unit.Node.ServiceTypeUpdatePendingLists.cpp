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

class TestServiceTypeUpdatePendingLists
{
protected:
    typedef ServiceTypeUpdateKind::Enum Kind;
    typedef ServiceTypeUpdatePendingLists::ProcessReplyResult ProcessReplyResult;
    typedef ServiceTypeUpdatePendingLists::MessageDescription MessageDescription;

    TestServiceTypeUpdatePendingLists() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestServiceTypeUpdatePendingLists() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    bool TryUpdateLists(
        ServiceTypeUpdateKind::Enum updateEvent,
        ServiceTypeIdentifier& serviceTypeId);

    void UpdateSequence(wstring events);

    ServiceTypeUpdatePendingLists::ProcessReplyResult TryProcessReply(
        ServiceTypeUpdateKind::Enum updateEvent,
        uint64 sequenceNumber);

    bool TryGetMessage(__out MessageDescription & updateEvent);

    void TestServiceTypeUpdatePendingLists::VerifyNextMessage(
        bool expectedHasNext,
        ServiceTypeUpdateKind::Enum updateEvent,
        size_t pendingItemsCount,
        uint64 sequenceNumber);
    void VerifyNoNextMessage();

    void VerifyHasPendingUpdates(bool expected) const;

    unique_ptr<UnitTestContext> testContext_;
    unique_ptr<ServiceTypeUpdatePendingLists> pendingLists_;

    ServiceTypeIdentifier SP1;
    ServiceTypeIdentifier SL1;
    ServiceTypeIdentifier SV1;
    map<ServiceTypeIdentifier, PartitionId> FUIDs;
};

bool TestServiceTypeUpdatePendingLists::TestSetup()
{
    testContext_ = UnitTestContext::Create();

    // Constants
    SP1 = Default::GetInstance().SP1_FTContext.STInfo.ServiceTypeId;
    SL1 = Default::GetInstance().SL1_FTContext.STInfo.ServiceTypeId;
    SV1 = Default::GetInstance().SV1_FTContext.STInfo.ServiceTypeId;
    FUIDs[SP1] = Default::GetInstance().SP1_FTContext.FUID;
    FUIDs[SL1] = Default::GetInstance().SL1_FTContext.FUID;
    FUIDs[SV1] = Default::GetInstance().SV1_FTContext.FUID;

    // Initialize
    pendingLists_ = make_unique<ServiceTypeUpdatePendingLists>();

    return true;
}

bool TestServiceTypeUpdatePendingLists::TryUpdateLists(
    ServiceTypeUpdateKind::Enum updateEvent,
    ServiceTypeIdentifier & serviceTypeId)
{
    if (updateEvent == Kind::PartitionDisabled || updateEvent == Kind::PartitionEnabled)
    {
        return pendingLists_->TryUpdatePartitionLists(updateEvent, FUIDs[serviceTypeId]);
    }

    return pendingLists_->TryUpdateServiceTypeLists(updateEvent, serviceTypeId);
}

void TestServiceTypeUpdatePendingLists::UpdateSequence(wstring events)
{
    // Usage:    Takes a string consisting of tokens separated by spaces.
    //           Tokens should be in the format [Action][ServiceType], 
    //           where Action can be E (Enabled), D (Disabled),
    //           PD (Partition Disabled), or PE (Partition Enabled)
    //           and ServiceType can be 1, 2, or 3, where each corresponds
    //           to a different ServiceType.
    //
    // Example:  UpdateSequence("E1 D2 D1 PD3")
    std::vector<wstring> tokens;
    wstring delimiter(L" ");
    StringUtility::SplitOnString(events, tokens, delimiter);

    for (auto it = tokens.cbegin(); it != tokens.cend(); it++)
    {
        auto const& token = *it;

        auto const& serviceTypeId = token.back();
        ServiceTypeIdentifier serviceType;
        switch (serviceTypeId)
        {
        case L'1':
            serviceType = SP1;
            break;
        case L'2':
            serviceType = SL1;
            break;
        case L'3':
            serviceType = SV1;
            break;
        default:
            Assert::TestAssert("Invalid update sequence service type id");
            break;
        }

        auto const& action = token.front();
        auto const& actionPart = token.at(1);
        switch (action)
        {
        case L'D':
            TryUpdateLists(Kind::Disabled, serviceType);
            break;
        case L'E':
            TryUpdateLists(Kind::Enabled, serviceType);
            break;
        case L'P':
            switch (actionPart)
            {
            case L'D':
                TryUpdateLists(Kind::PartitionDisabled, serviceType);
                break;
            case L'E':
                TryUpdateLists(Kind::PartitionEnabled, serviceType);
                break;
            default:
                Assert::TestAssert("Invalid update sequence partition action");
                break;
            }
            break;
        default:
            Assert::TestAssert("Invalid update sequence action");
            break;
        }
    }
}

ServiceTypeUpdatePendingLists::ProcessReplyResult TestServiceTypeUpdatePendingLists::TryProcessReply(
    ServiceTypeUpdateKind::Enum updateEvent,
    uint64 sequenceNumber)
{
    return pendingLists_->TryProcessReply(updateEvent, sequenceNumber);
}

bool TestServiceTypeUpdatePendingLists::TryGetMessage(__out MessageDescription & updateEvent)
{
    return pendingLists_->TryGetMessage(updateEvent);
}

void TestServiceTypeUpdatePendingLists::VerifyNextMessage(
    bool expectedHasNext,
    ServiceTypeUpdateKind::Enum updateEvent,
    size_t pendingItemsCount,
    uint64 sequenceNumber)
{
    MessageDescription description;
    bool hasNext = TryGetMessage(description);

    VERIFY_ARE_EQUAL(expectedHasNext, hasNext);
    VERIFY_ARE_EQUAL(updateEvent, description.UpdateEvent);
    VERIFY_ARE_EQUAL(sequenceNumber, description.SequenceNumber);

    if (description.UpdateEvent == Kind::Disabled ||
        description.UpdateEvent == Kind::Enabled)
    {
        VERIFY_ARE_EQUAL(pendingItemsCount, description.ServiceTypes.size());
    }
    else if (description.UpdateEvent == Kind::PartitionDisabled ||
             description.UpdateEvent == Kind::PartitionEnabled)
    {
        VERIFY_ARE_EQUAL(pendingItemsCount, description.Partitions.size());
    }
}

void TestServiceTypeUpdatePendingLists::VerifyNoNextMessage()
{
    MessageDescription description;
    bool hasNext = TryGetMessage(description);
    VERIFY_ARE_EQUAL(false, hasNext);
}

void TestServiceTypeUpdatePendingLists::VerifyHasPendingUpdates(bool expected) const
{
    bool hasPending = pendingLists_->HasPendingUpdates();
    VERIFY_ARE_EQUAL(expected, hasPending);
}

bool TestServiceTypeUpdatePendingLists::TestCleanup()
{
    testContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestServiceTypeUpdatePendingListsSuite, TestServiceTypeUpdatePendingLists)

/*
    Tests for TryUpdateLists shouldSend return value
*/

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_Disabled)
{
    bool shouldSend = TryUpdateLists(Kind::Disabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_Enabled)
{
    bool shouldSend = TryUpdateLists(Kind::Enabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_Disabled)
{
    bool shouldSend = TryUpdateLists(Kind::PartitionDisabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_Enabled)
{
    bool shouldSend = TryUpdateLists(Kind::PartitionEnabled, SP1);
    VERIFY_IS_FALSE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_DisabledEnabled)
{
    TryUpdateLists(Kind::Disabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::Enabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_EnabledDisabled)
{
    TryUpdateLists(Kind::Enabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::Disabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_DisabledEnabled)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::PartitionEnabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_EnabledDisabled)
{
    TryUpdateLists(Kind::PartitionEnabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::PartitionDisabled, SP1);
    VERIFY_IS_TRUE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_DisabledDisabled)
{
    TryUpdateLists(Kind::Disabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::Disabled, SP1);
    VERIFY_IS_FALSE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_ServiceType_EnabledEnabled)
{
    TryUpdateLists(Kind::Enabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::Enabled, SP1);
    VERIFY_IS_FALSE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_DisabledDisabled)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::PartitionDisabled, SP1);
    VERIFY_IS_FALSE(shouldSend);
}

BOOST_AUTO_TEST_CASE(TryUpdate_ShouldSend_Partition_EnabledEnabled)
{
    TryUpdateLists(Kind::PartitionEnabled, SP1);
    bool shouldSend = TryUpdateLists(Kind::PartitionEnabled, SP1);
    VERIFY_IS_FALSE(shouldSend);
}

/*
    Tests for HasPending
*/

BOOST_AUTO_TEST_CASE(HasPending_ServiceType_Disabled)
{
    TryUpdateLists(Kind::Disabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_ServiceType_Enabled)
{
    TryUpdateLists(Kind::Enabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_Partition_Disabled)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_Partition_Enabled)
{
    TryUpdateLists(Kind::PartitionEnabled, SP1);

    VerifyHasPendingUpdates(false);
}

BOOST_AUTO_TEST_CASE(HasPending_ServiceType_DisabledEnabled)
{
    TryUpdateLists(Kind::Disabled, SP1);
    TryUpdateLists(Kind::Enabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_ServiceType_EnabledDisabled)
{
    TryUpdateLists(Kind::Enabled, SP1);
    TryUpdateLists(Kind::Disabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_Partition_DisabledEnabled)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1);
    TryUpdateLists(Kind::PartitionEnabled, SP1);

    VerifyHasPendingUpdates(true);
}

BOOST_AUTO_TEST_CASE(HasPending_Partition_EnabledDisabled)
{
    TryUpdateLists(Kind::PartitionEnabled, SP1);
    TryUpdateLists(Kind::PartitionDisabled, SP1);

    VerifyHasPendingUpdates(true);
}

/*
    Tests for ProcessReply result
*/

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Disabled_EqualSeqNum)
{
    TryUpdateLists(Kind::Disabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Disabled, 1);
    VERIFY_IS_TRUE(res == ProcessReplyResult::NoPending);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Enabled_EqualSeqNum)
{
    TryUpdateLists(Kind::Enabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Enabled, 1);
    VERIFY_IS_TRUE(res == ProcessReplyResult::NoPending);
}

BOOST_AUTO_TEST_CASE(ProcessReply_Partition_Disabled_EqualSeqNum)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::PartitionDisabled, 1);
    VERIFY_IS_TRUE(res == ProcessReplyResult::NoPending);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Disabled_HigherSeqNum)
{
    TryUpdateLists(Kind::Disabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Disabled, 10);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Enabled_HigherSeqNum)
{
    TryUpdateLists(Kind::Enabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Enabled, 10);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_Partition_Disabled_HigherSeqNum)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::PartitionDisabled, 10);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Disabled_LowerSeqNum)
{
    TryUpdateLists(Kind::Disabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Disabled, 0);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Enabled_LowerSeqNum)
{
    TryUpdateLists(Kind::Enabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::Enabled, 0);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_Partition_Disabled_LowerSeqNum)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1); // msg sn: 1

    auto res = TryProcessReply(Kind::PartitionDisabled, 0);
    VERIFY_IS_TRUE(res == ProcessReplyResult::Stale);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Disabled_HasOtherPending)
{
    TryUpdateLists(Kind::Disabled, SP1); // msg sn: 1
    TryUpdateLists(Kind::Enabled, SL1);  // msg sn: 1

    auto res = TryProcessReply(Kind::Disabled, 1);
    VERIFY_IS_TRUE(res == ProcessReplyResult::HasPending);
}

BOOST_AUTO_TEST_CASE(ProcessReply_ServiceType_Enabled_HasOtherPending)
{
    TryUpdateLists(Kind::Enabled, SP1);  // msg sn: 1
    TryUpdateLists(Kind::Disabled, SL1); // msg sn: 2

    auto res = TryProcessReply(Kind::Enabled, 2);
    VERIFY_IS_TRUE(res == ProcessReplyResult::HasPending);
}

BOOST_AUTO_TEST_CASE(ProcessReply_Partition_Disabled_HasOtherPending)
{
    TryUpdateLists(Kind::PartitionDisabled, SP1); // msg sn: 1
    TryUpdateLists(Kind::Disabled, SL1);          // msg sn: 2

    auto res = TryProcessReply(Kind::PartitionDisabled, 2);
    VERIFY_IS_TRUE(res == ProcessReplyResult::HasPending);
}

/*
    Test TryGetMessage has next
*/

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_NoUpdates)
{
    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_ServiceType_Disabled)
{
    UpdateSequence(L"D1"); // msg sn: 1

    VerifyNextMessage(true, Kind::Disabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_ServiceType_Enabled)
{
    UpdateSequence(L"E1"); // msg sn: 1

    VerifyNextMessage(true, Kind::Enabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_Partition_Disabled)
{
    UpdateSequence(L"PD1"); // msg sn: 1

    VerifyNextMessage(true, Kind::PartitionDisabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_Partition_Enabled)
{
    UpdateSequence(L"PE1"); // msg sn: 1

    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_ServiceType_DisabledEnabled)
{
    UpdateSequence(L"D1 E1"); // msg sn: 2

    VerifyNextMessage(true, Kind::Enabled, 1, 3);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_ServiceType_EnabledDisabled)
{
    UpdateSequence(L"E1 D1"); // msg sn: 2

    VerifyNextMessage(true, Kind::Disabled, 1, 3);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_Partition_DisabledEnabled)
{
    UpdateSequence(L"PD1 PE1"); // msg sn: 2

    VerifyNextMessage(true, Kind::PartitionDisabled, 0, 3);
}

BOOST_AUTO_TEST_CASE(GetMessage_HasNext_Partition_EnabledDisabled)
{
    UpdateSequence(L"PE1 PD1"); // msg sn: 1

    VerifyNextMessage(true, Kind::PartitionDisabled, 1, 2);
}

/*
    Test TryGetMessage next event kind and sequence number
*/

BOOST_AUTO_TEST_CASE(GetMessage_Order_D1E2)
{
    UpdateSequence(L"D1 E2"); // msg sn: 1

    VerifyNextMessage(true, Kind::Disabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_Order_D1PD2)
{
    UpdateSequence(L"D1 PD2"); // msg sn: 1

    VerifyNextMessage(true, Kind::Disabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_Order_E1D2)
{
    UpdateSequence(L"E1 D2"); // msg sn: 2

    VerifyNextMessage(true, Kind::Disabled, 1, 3);
}

BOOST_AUTO_TEST_CASE(GetMessage_Order_E1PD2)
{
    UpdateSequence(L"E1 PD2"); // msg sn: 1

    VerifyNextMessage(true, Kind::Enabled, 1, 2);
}

BOOST_AUTO_TEST_CASE(GetMessage_Order_PD1D2)
{
    UpdateSequence(L"PD1 D2"); // msg sn: 2

    VerifyNextMessage(true, Kind::Disabled, 1, 3);
}

BOOST_AUTO_TEST_CASE(GetMessage_Order_PD1E2)
{
    UpdateSequence(L"PD1 E2"); // msg sn: 2

    VerifyNextMessage(true, Kind::Enabled, 1, 3);
}

/*
    Test TryGetMessage with replies
*/

BOOST_AUTO_TEST_CASE(GetMessage_Replies_D1E2)
{
    UpdateSequence(L"D1 E2");                     // msg sn: 1
    TryProcessReply(Kind::Disabled, 1);

    VerifyNextMessage(true, Kind::Enabled, 1, 2); // msg sn: 2

    TryProcessReply(Kind::Enabled, 2);
    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_CASE(GetMessage_Replies_E1D2)
{
    UpdateSequence(L"E1 D2");                     // msg sn: 2
    TryProcessReply(Kind::Disabled, 2);

    VerifyNextMessage(true, Kind::Enabled, 1, 3); // msg sn: 3

    TryProcessReply(Kind::Enabled, 3);
    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_CASE(GetMessage_Replies_D1E2PD3)
{
    UpdateSequence(L"D1 E2 PD3"); // msg sn: 1
    TryProcessReply(Kind::Disabled, 1);

    VerifyNextMessage(true, Kind::Enabled, 1, 2);           // msg sn: 2

    TryProcessReply(Kind::Enabled, 2);
    VerifyNextMessage(true, Kind::PartitionDisabled, 1, 3); // msg sn: 3

    TryProcessReply(Kind::PartitionDisabled, 3);
    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_CASE(GetMessage_Replies_PD1E2D3)
{
    UpdateSequence(L"PD1 E2 D3");                           // msg sn: 3
    TryProcessReply(Kind::Disabled, 3);

    VerifyNextMessage(true, Kind::Enabled, 1, 4);           // msg sn: 4

    TryProcessReply(Kind::Enabled, 4);
    VerifyNextMessage(true, Kind::PartitionDisabled, 1, 5); // msg sn: 5

    TryProcessReply(Kind::PartitionDisabled, 5);
    VerifyNoNextMessage();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
