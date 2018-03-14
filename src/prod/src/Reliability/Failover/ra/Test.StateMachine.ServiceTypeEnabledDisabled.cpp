// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestStateMachine_ServiceTypeEnabledDisabled
{
protected:
    TestStateMachine_ServiceTypeEnabledDisabled() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup) ;
    ~TestStateMachine_ServiceTypeEnabledDisabled() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup) ;

    void PerformAddPrimary(wstring const & shortName) const;

    void ProcessServiceTypeEnabled(wstring const& shortName, uint64 sequenceNumber) const;
    void ProcessServiceTypeDisabled(wstring const& shortName, uint64 sequenceNumber) const;

    void ValidateNoEnabledMessage() const;
    void ValidateNoDisabledMessage() const;

    void ValidateEnabledMessage(wstring const& shortName, uint64 sequenceNumber) const;
    void ValidateDisabledMessage(wstring const& shortName, uint64 sequenceNumber) const;

    void ProcessEnabledReply(uint64 sequenceNumber) const;
    void ProcessDisabledReply(uint64 sequenceNumber) const;

    void ResetAll() const;
    void RequestWork() const;
    void Drain() const;
    void RequestAndDrain() const;

    ScenarioTest & GetScenarioTest() const
    {
        return holder_->ScenarioTestObj;
    }

    FailoverConfig & GetConfig() const
    {
        return GetScenarioTest().UTContext.Config;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_ServiceTypeEnabledDisabled::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();

    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromMinutes(1);
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::FromMinutes(1);

    return true;
}

bool TestStateMachine_ServiceTypeEnabledDisabled::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_ServiceTypeEnabledDisabled::PerformAddPrimary(wstring const & shortName) const
{
    auto & test = GetScenarioTest();
    test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(shortName, L"311/422 [P/P IB U 1:1]");
}

void TestStateMachine_ServiceTypeEnabledDisabled::ProcessServiceTypeEnabled(wstring const & shortName, uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessServiceTypeEnabled(shortName, sequenceNumber);
}

void TestStateMachine_ServiceTypeEnabledDisabled::ProcessServiceTypeDisabled(wstring const & shortName, uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessServiceTypeDisabled(shortName, sequenceNumber);
}

void TestStateMachine_ServiceTypeEnabledDisabled::ValidateNoEnabledMessage() const
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::ServiceTypeEnabled>();
}

void TestStateMachine_ServiceTypeEnabledDisabled::ValidateNoDisabledMessage() const
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::ServiceTypeDisabled>();
}

void TestStateMachine_ServiceTypeEnabledDisabled::ValidateEnabledMessage(wstring const & shortName, uint64 sequenceNumber) const
{
    auto serviceTypeId = GetScenarioTest().GetSingleFTContext(shortName).STInfo.ServiceTypeId;

    auto body = wformatString(
        "[{0}] {1}",
        shortName,
        sequenceNumber);

     GetScenarioTest().ValidateFMMessage<MessageType::ServiceTypeEnabled>(body);
}

void TestStateMachine_ServiceTypeEnabledDisabled::ValidateDisabledMessage(wstring const & shortName, uint64 sequenceNumber) const
{
    auto serviceTypeId = GetScenarioTest().GetSingleFTContext(shortName).STInfo.ServiceTypeId;

    auto body = wformatString(
        "[{0}] {1}",
        shortName,
        sequenceNumber);

    GetScenarioTest().ValidateFMMessage<MessageType::ServiceTypeDisabled>(body);
}

void TestStateMachine_ServiceTypeEnabledDisabled::ProcessEnabledReply(uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessFMMessage<MessageType::ServiceTypeEnabledReply>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_ServiceTypeEnabledDisabled::ProcessDisabledReply(uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessFMMessage<MessageType::ServiceTypeDisabledReply>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_ServiceTypeEnabledDisabled::ResetAll() const
{
    GetScenarioTest().ResetAll();
}

void TestStateMachine_ServiceTypeEnabledDisabled::RequestWork() const
{
    GetScenarioTest().StateItemHelpers.MessageRetryHelper.RequestWork();
}

void TestStateMachine_ServiceTypeEnabledDisabled::Drain() const
{
    GetScenarioTest().DrainJobQueues();
}

void TestStateMachine_ServiceTypeEnabledDisabled::RequestAndDrain() const
{
    RequestWork();
    Drain();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ServiceTypeEnabledDisabledSuite, TestStateMachine_ServiceTypeEnabledDisabled)

/*
    Test messages are/aren't sent
*/

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageIsSent_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    Drain();

    ValidateDisabledMessage(L"SP1", 2);
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageIsSent_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1); // msg sn: 1
    Drain();

    ValidateEnabledMessage(L"SP1", 2);
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_EnabledMessageNotSent_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1);
    Drain();

    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_DisabledMessageNotSent_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1);
    Drain();

    ValidateNoDisabledMessage();
}

/*
    Test for handling of updates with different sequence numbers
*/

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_HigherSequenceNumEnable_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    ProcessServiceTypeEnabled(L"SP1", 2);  // msg sn: 2
    Drain();

    ValidateEnabledMessage(L"SP1", 3);
    ValidateNoDisabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_HigherSequenceNumDisable_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1);  // msg sn: 1
    ProcessServiceTypeDisabled(L"SP1", 2); // msg sn: 2
    Drain();

    ValidateDisabledMessage(L"SP1", 3);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_LowerSequenceNumEnable_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 2); // msg sn: 1
    ProcessServiceTypeEnabled(L"SP1", 1);  // msg sn: 1
    Drain();

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_LowerSequenceNumDisable_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 2);  // msg sn: 1
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    Drain();

    ValidateEnabledMessage(L"SP1", 2);
    ValidateNoDisabledMessage();
}

/*
    Tests for message retries
*/

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_RetriesMessage_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1

    ResetAll();
    RequestAndDrain();

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_RetriesMessage_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1); // msg sn: 1

    ResetAll();
    RequestAndDrain();

    ValidateEnabledMessage(L"SP1", 2);
    ValidateNoDisabledMessage();
}

/*
    Tests for reply handling
*/

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_ClearsPendingAfterReply_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    ProcessDisabledReply(1);

    ResetAll();
    RequestAndDrain();

    ValidateNoDisabledMessage();
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_ClearsPendingAfterReply_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1); // msg sn: 1
    ProcessEnabledReply(1);

    ResetAll();
    RequestAndDrain();

    ValidateNoDisabledMessage();
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_LowerSequenceNumberReplyIgnored_Disabled)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    ProcessDisabledReply(0);
    Drain();

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_LowerSequenceNumberReplyIgnored_Enabled)
{
    ProcessServiceTypeEnabled(L"SP1", 1);  // msg sn: 1
    ProcessEnabledReply(0);
    Drain();

    ValidateEnabledMessage(L"SP1", 2);
    ValidateNoDisabledMessage();
}

/*
    Tests for message short-circuiting
*/

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageShortCircuit_SendsOnlyDisabledMessageFirst)
{
    ProcessServiceTypeEnabled(L"SP2", 1);  // msg sn: 1
    ProcessServiceTypeDisabled(L"SP1", 2); // msg sn: 2
    Drain();

    ValidateDisabledMessage(L"SP1", 3);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageShortCircuit_SendsOnlyDisabledMessageFirst2)
{
    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    ProcessServiceTypeEnabled(L"SP2", 2);  // msg sn: 1
    Drain();

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageShortCircuit_SendsEnabledAfterDisabledCleared)
{
    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromSeconds(0);
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::FromSeconds(0);

    ProcessServiceTypeEnabled(L"SP1", 1);  // msg sn: 1
    ProcessServiceTypeDisabled(L"SP2", 2); // msg sn: 2
    ResetAll();

    ProcessDisabledReply(2);               // msg sn: 2

    ResetAll();
    RequestAndDrain();                     // msg sn: 3

    ValidateEnabledMessage(L"SP1", 3);
    ValidateNoDisabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageShortCircuit_SendsDisabledAfterEnabledUpdate)
{

    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    ProcessServiceTypeEnabled(L"SP2", 2);  // msg sn: 1
    ResetAll();

    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromSeconds(0);
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::FromSeconds(0);

    RequestAndDrain();                     // msg sn: 2

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
}

BOOST_AUTO_TEST_CASE(ServiceTypeUpdate_MessageShortCircuit_DoesNotIncrementWhenUnnecessary)
{
    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromSeconds(0);
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::FromSeconds(0);

    ProcessServiceTypeDisabled(L"SP1", 1); // msg sn: 1
    Drain();                               // msg sn: 2

    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();

    ResetAll();

    // The Enabled update should not increment the seq num on enqueue and
    // should not request work (so the seq num should not increment).
    ProcessServiceTypeEnabled(L"SP2", 2);  // msg sn: 2
    Drain();                               // msg sn: 2

    ValidateNoDisabledMessage();
    ValidateNoEnabledMessage();

    ProcessDisabledReply(2);

    RequestAndDrain();                     // msg sn: 3

    ValidateEnabledMessage(L"SP2", 3);
    ValidateNoDisabledMessage();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
