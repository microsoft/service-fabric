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

class TestStateMachine_PartitionEnabledDisabled
{
protected:
    TestStateMachine_PartitionEnabledDisabled() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup) ;
    ~TestStateMachine_PartitionEnabledDisabled() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup) ;

    void PerformAddPrimary(wstring const & shortName) const;

    void ProcessPartitionEnabled(wstring const& shortName, uint64 sequenceNumber) const;
    void ProcessPartitionDisabled(wstring const& shortName, uint64 sequenceNumber) const;

    void ValidateNoPartitionMessage() const;
    void ValidateNoEnabledMessage() const;
    void ValidateNoDisabledMessage() const;

    void ValidateEmptyDisabledMessage(uint64 sequenceNumber) const;
    void ValidatePartitionMesssage(std::wstring const& shortName, uint64 sequenceNumber) const;
    void ValidatePartitionMesssage(vector<wstring> const& shortName, uint64 sequenceNumber) const;
    void ValidateEnabledMessage(wstring const& shortName, uint64 sequenceNumber) const;
    void ValidateDisabledMessage(wstring const& shortName, uint64 sequenceNumber) const;

    void ProcessReply(uint64 sequenceNumber) const;
    void ProcessEnabledReply(uint64 sequenceNumber) const;
    void ProcessDisabledReply(uint64 sequenceNumber) const;

    void ResetAll() const;
    void RequestWork() const;
    void Drain() const;
    void RequestAndDrain() const;

    Infrastructure::BackgroundWorkManagerWithRetry & GetBGMR() const
    {
        return GetScenarioTest().RA.ServiceTypeUpdateProcessorObj.MessageRetryWorkManager;
    }

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

bool TestStateMachine_PartitionEnabledDisabled::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();

    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromMinutes(1);
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::FromMinutes(1);

    return true;
}

bool TestStateMachine_PartitionEnabledDisabled::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_PartitionEnabledDisabled::PerformAddPrimary(wstring const & shortName) const
{
    auto & test = holder_->ScenarioTestObj;
    test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(shortName, L"311/422 [P/P IB U 1:1]");
}

void TestStateMachine_PartitionEnabledDisabled::ProcessPartitionEnabled(wstring const & shortName, uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessServiceTypeEnabled(
        shortName,
        sequenceNumber,
        ServiceModel::ServicePackageActivationMode::ExclusiveProcess);
}

void TestStateMachine_PartitionEnabledDisabled::ProcessPartitionDisabled(wstring const & shortName, uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessServiceTypeDisabled(
        shortName,
        sequenceNumber,
        ServiceModel::ServicePackageActivationMode::ExclusiveProcess);
}

void TestStateMachine_PartitionEnabledDisabled::ValidateNoPartitionMessage() const
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::PartitionNotification>();
}

void TestStateMachine_PartitionEnabledDisabled::ValidateEmptyDisabledMessage(uint64 sequenceNumber) const
{
    GetScenarioTest().ValidateFMMessage<MessageType::PartitionNotification>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_PartitionEnabledDisabled::ValidateNoEnabledMessage() const
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::ServiceTypeEnabled>();
}

void TestStateMachine_PartitionEnabledDisabled::ValidateNoDisabledMessage() const
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::ServiceTypeDisabled>();
}

void TestStateMachine_PartitionEnabledDisabled::ValidatePartitionMesssage(std::wstring const& shortName, uint64 sequenceNumber) const
{
    vector<std::wstring> shortNames;
    shortNames.push_back(shortName);
    ValidatePartitionMesssage(shortNames, sequenceNumber);
}

void TestStateMachine_PartitionEnabledDisabled::ValidateEnabledMessage(wstring const & shortName, uint64 sequenceNumber) const
{
    auto serviceTypeId = GetScenarioTest().GetSingleFTContext(shortName).STInfo.ServiceTypeId;

    auto body = wformatString(
        "[{0}] {1}",
        shortName,
        sequenceNumber);

    GetScenarioTest().ValidateFMMessage<MessageType::ServiceTypeEnabled>(body);
}

void TestStateMachine_PartitionEnabledDisabled::ValidateDisabledMessage(wstring const & shortName, uint64 sequenceNumber) const
{
    auto serviceTypeId = GetScenarioTest().GetSingleFTContext(shortName).STInfo.ServiceTypeId;

    auto body = wformatString(
        "[{0}] {1}",
        shortName,
        sequenceNumber);

    GetScenarioTest().ValidateFMMessage<MessageType::ServiceTypeDisabled>(body);
}

void TestStateMachine_PartitionEnabledDisabled::ValidatePartitionMesssage(vector<std::wstring> const& shortNames, uint64 sequenceNumber) const
{
    std::wstring partitionList;
    for (auto it = shortNames.begin(); it != shortNames.end(); ++it)
    {
        partitionList += GetScenarioTest().GetSingleFTContext(*it).FUID.Guid.ToString();

        if (it != shortNames.end())
        {
            partitionList += L" ";
        }
    }

    auto body = wformatString(
        "[{0}] {1}",
        partitionList,
        sequenceNumber);

     GetScenarioTest().ValidateFMMessage<MessageType::PartitionNotification>(body);
}

void TestStateMachine_PartitionEnabledDisabled::ProcessReply(uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::PartitionNotificationReply>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_PartitionEnabledDisabled::ProcessEnabledReply(uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessFMMessage<MessageType::ServiceTypeEnabledReply>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_PartitionEnabledDisabled::ProcessDisabledReply(uint64 sequenceNumber) const
{
    GetScenarioTest().ProcessFMMessage<MessageType::ServiceTypeDisabledReply>(wformatString("{0}", sequenceNumber));
}

void TestStateMachine_PartitionEnabledDisabled::ResetAll() const
{
    GetScenarioTest().ResetAll();
}

void TestStateMachine_PartitionEnabledDisabled::RequestWork() const
{
    GetScenarioTest().StateItemHelpers.MessageRetryHelper.RequestWork();
}

void TestStateMachine_PartitionEnabledDisabled::Drain() const
{
    GetScenarioTest().DrainJobQueues();
}

void TestStateMachine_PartitionEnabledDisabled::RequestAndDrain() const
{
    RequestWork();
    Drain();
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_PartitionEnabledDisabledSuite, TestStateMachine_PartitionEnabledDisabled)

BOOST_AUTO_TEST_CASE(PartitionEnabled_MessageIsSent)
{
    ProcessPartitionEnabled(L"SP1", 1); // msg sn: 1

    Drain();                            // msg sn: 2
    ValidateEnabledMessage(L"SP1", 2);
    ValidateNoDisabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_MessageIsSent)
{
    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1

    Drain();                             // msg sn: 2
    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionEnabled_HigherSequenceNumDisable)
{
    ProcessPartitionEnabled(L"SP1", 1);  // msg sn: 1
    ProcessPartitionDisabled(L"SP1", 2); // msg sn: 2

    Drain();                             // msg sn: 3
    ValidateDisabledMessage(L"SP1", 3);
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_HigherSequenceNumEnable)
{
    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1
    ProcessPartitionEnabled(L"SP1", 2);  // msg sn: 2

    Drain();                             // msg sn: 3
    ValidateEnabledMessage(L"SP1", 3);
    ValidateNoDisabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionEnabled_LowerSequenceNumDisable)
{
    ProcessPartitionEnabled(L"SP1", 2);  // msg sn: 1
    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1

    Drain();                             // msg sn: 2
    ValidateEnabledMessage(L"SP1", 2);
    ValidateNoDisabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_LowerSequenceNumEnable)
{
    ProcessPartitionDisabled(L"SP1", 2); // msg sn: 1
    ProcessPartitionEnabled(L"SP1", 1);  // msg sn: 1

    Drain();                             // msg sn: 2
    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_ReplyClearsPending)
{
    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1
    ProcessDisabledReply(1);

    RequestAndDrain();
    ValidateNoDisabledMessage();
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_LowerSequenceNumReplyIgnored)
{
    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1
    ProcessDisabledReply(0);

    RequestAndDrain();                   // msg sn: 2
    ValidateDisabledMessage(L"SP1", 2);
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_CASE(PartitionDisabled_RetriesMessage)
{
    GetConfig().PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::Zero;
    GetConfig().PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::Zero;

    ProcessPartitionDisabled(L"SP1", 1); // msg sn: 1
    Drain();                             // msg sn: 2
    
    ResetAll();
    RequestAndDrain();                   // msg sn: 3

    ValidateDisabledMessage(L"SP1", 3);
    ValidateNoEnabledMessage();
    ValidateNoPartitionMessage();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
