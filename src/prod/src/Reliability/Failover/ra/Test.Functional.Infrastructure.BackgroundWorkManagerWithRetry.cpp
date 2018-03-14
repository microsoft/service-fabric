// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Common;
using namespace std;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;

class TestBackgroundWorkManagerWithRetry
{
protected:
    TestBackgroundWorkManagerWithRetry() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestBackgroundWorkManagerWithRetry() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    unique_ptr<BackgroundWorkManagerWithRetry> bgmr_;
    UnitTestContextUPtr utContext_;

    int workFunctionCount_;
    void Request();

    class MyThreadpoolStub : public ThreadpoolStub
    {
    public:
        MyThreadpoolStub(ReconfigurationAgent & ra) : ThreadpoolStub(ra, false) {}

        virtual void ExecuteOnThreadpool(ThreadpoolCallback callback)
        {
            callback();
        }
    };
};

bool TestBackgroundWorkManagerWithRetry::TestSetup()
{
	utContext_ = UnitTestContext::Create(UnitTestContext::Option::StubJobQueueManager);
    workFunctionCount_ = 0;

    IThreadpoolUPtr threadpoolStub(new MyThreadpoolStub(utContext_->RA));
    utContext_->UpdateThreadpool(move(threadpoolStub));

    utContext_->Config.MinimumIntervalBetweenPeriodicStateCleanup = TimeSpan::Zero;

	// The large timer retry interval is to make sure that the timer does not fire
	// These tests validate that the timer was set using the IsSet property on the timer
	// and cancel the timer before it fires
	utContext_->Config.FMMessageRetryInterval = TimeSpan::FromMilliseconds(1000);

    bgmr_ = make_unique<BackgroundWorkManagerWithRetry>(
		L"id",
		L"aidprefix",
        [this](wstring const &, ReconfigurationAgent&, BackgroundWorkManagerWithRetry&)
        {
            workFunctionCount_++;
        },
        utContext_->Config.MinimumIntervalBetweenPeriodicStateCleanupEntry,
		utContext_->Config.FMMessageRetryIntervalEntry,
		utContext_->RA);

	return true;
}

bool TestBackgroundWorkManagerWithRetry::TestCleanup()
{
	bgmr_->Close();
	bgmr_.reset();

	return utContext_->Cleanup();
}

void TestBackgroundWorkManagerWithRetry::Request()
{
	bgmr_->Request(L"a");
}

BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(TestBackgroundWorkManagerWithRetrySuite,TestBackgroundWorkManagerWithRetry)

BOOST_AUTO_TEST_CASE(RequestCausesCallbackToExecute)
{
    Request();

    // the cb should execute
    VERIFY_ARE_EQUAL(1, workFunctionCount_);

    // compelte the work function
    bgmr_->OnWorkComplete(false);
}

BOOST_AUTO_TEST_CASE(SetTimerArmsTheTimer)
{
    bgmr_->SetTimer();

    // Timer sequence number should change i.e. it is armed
    VERIFY_IS_TRUE(bgmr_->RetryTimerObj.IsSet);

    // the cb should execute eventually
    BusyWaitUntil([&] { return workFunctionCount_ == 1;});
}

BOOST_AUTO_TEST_CASE(AskingForRetryAlsoArmsTimer)
{       
    Request();

    // Complete cb and ask for retry
    bgmr_->OnWorkComplete(true);

    // Timer sequence number should change i.e. it is armed
    VERIFY_IS_TRUE(bgmr_->RetryTimerObj.IsSet);

    // the cb should execute eventually
    BusyWaitUntil([&] { return workFunctionCount_ == 2;});
}

BOOST_AUTO_TEST_CASE(NotAskingForRetryDoesNotRearmTimer)
{
    Request();

    // Complete cb and do not ask for retry
    bgmr_->OnWorkComplete(false);

    // Timer sequence number should change i.e. it is armed
    VERIFY_IS_FALSE(bgmr_->RetryTimerObj.IsSet);
}

BOOST_AUTO_TEST_CASE(TimerIsNotCancelledIfSetCallComesWhileCbRuns)
{
    Request();

    // while the cb is running set the timer again
    bgmr_->SetTimer();

    // Complete cb and do not ask for retry
    bgmr_->OnWorkComplete(false);

    VERIFY_IS_TRUE(bgmr_->RetryTimerObj.IsSet);
}

BOOST_AUTO_TEST_CASE(CompletingWorkWithImmediateCausesRetry)
{
    Request();

    bgmr_->OnWorkComplete(BackgroundWorkRetryType::Immediate);

    VERIFY_ARE_EQUAL(2, workFunctionCount_);

    VERIFY_IS_FALSE(bgmr_->RetryTimerObj.IsSet);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
