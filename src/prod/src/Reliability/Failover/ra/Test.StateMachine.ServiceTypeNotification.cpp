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

class TestStateMachine_ServiceTypeNotification
{
protected:
    TestStateMachine_ServiceTypeNotification() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup) ;
    ~TestStateMachine_ServiceTypeNotification() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup) ;

    void SetFSRSuccess(wstring const & shortName);
    void SetFSRRetryable(wstring const & shortName);
    
    void PerformAddPrimary(wstring const & shortName);
    
    void PerformAddPrimaryWithFSRSuccess(wstring const & shortName);
    void PerformAddPrimaryWithFSRRetryable(wstring const & shortName);

    void PerformRemove(wstring const & shortName);

    void ValidateMessage(wstring const & shortName1);
    void ValidateMessage(wstring const & shortName1, wstring const & shortName2);
    void ValidateNoMessage();

    void ProcessReply(wstring const & shortName);

    void RequestRetry();

    Infrastructure::BackgroundWorkManagerWithRetry & GetBGMR()
    {
        return GetScenarioTest().RA.HostingAdapterObj.BGMR;
    }

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_ServiceTypeNotification::TestSetup()
{
    holder_ = ScenarioTestHolder::Create(); 

    auto context = Default::GetInstance().SP1_FTContext;
    context.FUID = FailoverUnitId();
    context.CUID = ConsistencyUnitId();
    context.ShortName = L"SP11";

    GetScenarioTest().AddFTContext(L"SP11", context);

    auto & config = holder_->ScenarioTestObj.UTContext.Config;
    config.PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::Zero;
    config.PerNodeMinimumIntervalBetweenServiceTypeMessageToFM = TimeSpan::Zero;

    return true;
}

bool TestStateMachine_ServiceTypeNotification::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_ServiceTypeNotification::PerformAddPrimaryWithFSRSuccess(wstring const & shortName)
{
    SetFSRSuccess(shortName);
    PerformAddPrimary(shortName);
}

void TestStateMachine_ServiceTypeNotification::PerformAddPrimaryWithFSRRetryable(wstring const & shortName)
{
    SetFSRRetryable(shortName);
    PerformAddPrimary(shortName);
}

void TestStateMachine_ServiceTypeNotification::PerformAddPrimary(wstring const & shortName)
{
    auto & test = holder_->ScenarioTestObj;
    test.ProcessFMMessageAndDrain<MessageType::AddPrimary>(shortName, L"311/422 [P/P IB U 1:1]");
}

void TestStateMachine_ServiceTypeNotification::RequestRetry()
{
    auto & test = holder_->ScenarioTestObj;
    GetBGMR().Request(L"a");
    test.DrainJobQueues();
}

void TestStateMachine_ServiceTypeNotification::PerformRemove(wstring const & shortName)
{
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::DeleteReplica>(shortName, L"000/422 [N/P RD U 1:1]");
    GetScenarioTest().ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(shortName, L"000/422 [N/N RD U 1:1] Success D");
}

void TestStateMachine_ServiceTypeNotification::SetFSRSuccess(wstring const & shortName)
{
    GetScenarioTest().SetFindServiceTypeRegistrationSuccess(shortName);
}

void TestStateMachine_ServiceTypeNotification::SetFSRRetryable(wstring const & shortName)
{
    GetScenarioTest().SetFindServiceTypeRegistrationState(shortName, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpenAndReopenAndDrop);
}

void TestStateMachine_ServiceTypeNotification::ValidateNoMessage()
{
    GetScenarioTest().ValidateNoFMMessagesOfType<MessageType::ServiceTypeNotification>();
}

void TestStateMachine_ServiceTypeNotification::ValidateMessage(wstring const & shortName1)
{
    ValidateMessage(shortName1, L"");
}

void TestStateMachine_ServiceTypeNotification::ProcessReply(wstring const & shortName)
{
    GetScenarioTest().ProcessFMMessageAndDrain<MessageType::ServiceTypeNotificationReply>(wformatString("[{0}]", shortName));
}

void TestStateMachine_ServiceTypeNotification::ValidateMessage(wstring const & shortName1, wstring const & shortName2)
{
    wstring body;
    if (shortName2.empty())
    {
        body = wformatString("[{0}]", shortName1);
    }
    else
    {
        body = wformatString("[{0}] [{1}]", shortName1, shortName2);
    }

    GetScenarioTest().ValidateFMMessage<MessageType::ServiceTypeNotification>(body);
}

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ServiceTypeNotificationSuite, TestStateMachine_ServiceTypeNotification)

BOOST_AUTO_TEST_CASE(AddServiceType_MessageIsSent)
{
    PerformAddPrimaryWithFSRSuccess(L"SP1");

    ValidateMessage(L"SP1");
}

BOOST_AUTO_TEST_CASE(AddServiceType_MessageIsNotSent)
{
    // SP2 is pending. SP1 is complete. SP1 gets another retry. No message should be sent
    PerformAddPrimaryWithFSRSuccess(L"SP2");
    PerformAddPrimaryWithFSRSuccess(L"SP1");
    ProcessReply(L"SP1");

    GetScenarioTest().ResetAll();
    PerformAddPrimaryWithFSRSuccess(L"SP11");
    ValidateNoMessage();
}

BOOST_AUTO_TEST_CASE(RemoveServiceType_ContinuesToRetryExistingItems)
{
    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformAddPrimaryWithFSRSuccess(L"SP2");

    PerformRemove(L"SP1");
    GetScenarioTest().ResetAll();

    RequestRetry();
    ValidateMessage(L"SP2");
}

BOOST_AUTO_TEST_CASE(RemoveServiceType_CleansUpTheEntity)
{
    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformRemove(L"SP1");

    PerformAddPrimaryWithFSRSuccess(L"SP1");
    ValidateMessage(L"SP1");
}

BOOST_AUTO_TEST_CASE(RemoveServiceType_ReplyIsReceived)
{
    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformRemove(L"SP1");

    ProcessReply(L"SP1");
    GetScenarioTest().ResetAll();

    RequestRetry();
    ValidateNoMessage();
}

BOOST_AUTO_TEST_CASE(RemoveServiceType_DoesNotRemoveTheEntityIfNotRequested)
{
    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformAddPrimaryWithFSRSuccess(L"SP11");

    PerformRemove(L"SP11");

    GetScenarioTest().ResetAll();
    RequestRetry();
    ValidateMessage(L"SP1");
}

BOOST_AUTO_TEST_CASE(Retry_MessageIsRetried)
{
    auto & test = holder_->ScenarioTestObj;
    PerformAddPrimaryWithFSRSuccess(L"SP1");

    test.ResetAll();

    RequestRetry();
    ValidateMessage(L"SP1");
}

BOOST_AUTO_TEST_CASE(Retry_MultiplePendingItems)
{
    auto & test = holder_->ScenarioTestObj;

    PerformAddPrimaryWithFSRRetryable(L"SP1");
    PerformAddPrimaryWithFSRRetryable(L"SP2");

    test.ProcessServiceTypeRegisteredAndDrain(L"SP1");
    test.ProcessServiceTypeRegisteredAndDrain(L"SP2");

    test.ResetAll();

    RequestRetry();
    ValidateMessage(L"SP1", L"SP2");
}

BOOST_AUTO_TEST_CASE(Reply_RetryIsStoppedIfAllEntitesAreUpdated)
{
    auto & test = holder_->ScenarioTestObj;

    PerformAddPrimaryWithFSRSuccess(L"SP1");

    test.ResetAll();
    ProcessReply(L"SP1");

    RequestRetry();

    ValidateNoMessage();
    Verify::IsTrue(!GetBGMR().RetryTimerObj.IsSet, L"BGMR must no longer be retry");
}

BOOST_AUTO_TEST_CASE(Reply_RetryHandlesDeletedEntities)
{

    ProcessReply(L"SP1");

    RequestRetry();

    ValidateNoMessage();
    Verify::IsTrue(!GetBGMR().RetryTimerObj.IsSet, L"BGMR must no longer be retry");
}

BOOST_AUTO_TEST_CASE(Reply_RetryContinuesIfSomeEntriesAreMissing)
{
    auto & test = holder_->ScenarioTestObj;

    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformAddPrimaryWithFSRSuccess(L"SP2");

    ProcessReply(L"SP1");

    test.ResetAll();
    RequestRetry();
    ValidateMessage(L"SP2");
}

BOOST_AUTO_TEST_CASE(Retry_MessageRespectsEntityCountAndRetryPolicy)
{
    auto & test = holder_->ScenarioTestObj;

    auto & config = test.UTContext.Config;
    config.MaxNumberOfServiceTypeInMessageToFM = 0;
    config.PerServiceTypeMinimumIntervalBetweenMessageToFM = TimeSpan::FromMinutes(1);

    PerformAddPrimaryWithFSRSuccess(L"SP1");
    PerformAddPrimaryWithFSRSuccess(L"SP2");

    test.ResetAll();
    config.MaxNumberOfServiceTypeInMessageToFM = 1;

    RequestRetry();
    ValidateMessage(L"SP1");

    test.ResetAll();
    RequestRetry();
    ValidateMessage(L"SP2");
}

BOOST_AUTO_TEST_CASE(FMIsSkipped)
{
    auto & test = holder_->ScenarioTestObj;
    PerformAddPrimaryWithFSRSuccess(L"FM");

    test.ValidateNoFMMessagesOfType<MessageType::ServiceTypeNotification>();

    PerformRemove(L"FM");
    test.ValidateNoFMMessagesOfType<MessageType::ServiceTypeNotification>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
