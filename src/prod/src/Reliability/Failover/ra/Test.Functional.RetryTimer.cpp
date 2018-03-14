// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Common;
using namespace std;
using namespace ReliabilityUnitTest;
using namespace Infrastructure;

namespace
{
    std::wstring DefaultActivityIdPrefix(L"foo");
}

class TimerHolder
{
    DENY_COPY(TimerHolder);
public:
    TimerHolder(unique_ptr<RetryTimer> && timer)
    : timer_(move(timer))
    {
    }

    ~TimerHolder()
    {
        if (timer_)
        {
            timer_->Close();
        }
    }

    __declspec(property(get=get_Timer)) RetryTimer & Timer;
    RetryTimer & get_Timer() { return *timer_; }

private:
    unique_ptr<RetryTimer> timer_;
};

DWORD ShortTimerDurationInMs = 10;
DWORD MiddleTimerDurationInMs = 100;

class TestRetryTimer
{
protected:
    TestRetryTimer() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestRetryTimer() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    unique_ptr<TimerHolder> CreateTimerHolderEx(DWORD intervalInMs, RetryTimer::RetryTimerCallback const & callback)
    {
        testContext_->Config.FMMessageRetryInterval = TimeSpan::FromMilliseconds(intervalInMs);
        auto timer = make_unique<RetryTimer>(
            L"a", 
            testContext_->RA, 
            testContext_->Config.FMMessageRetryIntervalEntry,
            DefaultActivityIdPrefix, 
            callback);

        return make_unique<TimerHolder>(move(timer));
    }

    unique_ptr<TimerHolder> CreateTimerHolder(DWORD intervalInMs, std::function<void ()> const & callback)
    {
        return CreateTimerHolderEx(intervalInMs, [=] (wstring const&, ReconfigurationAgent&) { callback(); });
    }

    unique_ptr<UnitTestContext> testContext_;
};

bool TestRetryTimer::TestSetup()
{
	testContext_ = UnitTestContext::Create();
	return true;
}

bool TestRetryTimer::TestCleanup()
{
	testContext_->Cleanup();
	return true;
}

BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(TestRetryTimerSuite,TestRetryTimer)

BOOST_AUTO_TEST_CASE(OnConstructSequenceNumberIsZero)
{
    auto holder = CreateTimerHolder(ShortTimerDurationInMs, [] { });
    RetryTimer & timer = holder->Timer;
        
    VERIFY_ARE_EQUAL(0, timer.SequenceNumber);
    timer.Close();
}

BOOST_AUTO_TEST_CASE(OnConstructTimerIsNotSet)
{
    auto holder = CreateTimerHolder(ShortTimerDurationInMs, [] { });
    RetryTimer & timer = holder->Timer;

    VERIFY_IS_FALSE(timer.IsSet);
    timer.Close();
}

BOOST_AUTO_TEST_CASE(SetTimerInvokesCallback)
{
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(ShortTimerDurationInMs, [&isInvoked] 
                                    {
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Set();
        
    BusyWaitUntil([&isInvoked] { return isInvoked.load(); });

    timer.Close();
}

BOOST_AUTO_TEST_CASE(SetTimerIncrementsSequenceNumber)
{
    auto holder = CreateTimerHolder(ShortTimerDurationInMs, []  {});

    RetryTimer & timer = holder->Timer;

    auto initialSequenceNumber = timer.SequenceNumber;

    timer.Set();

    auto finalSequenceNumber = timer.SequenceNumber;

    timer.Close();

    VERIFY_ARE_EQUAL(1, finalSequenceNumber - initialSequenceNumber);
}

BOOST_AUTO_TEST_CASE(SetTimerSetsIsSetToTrue)
{
    auto holder = CreateTimerHolder(ShortTimerDurationInMs, []  {});

    RetryTimer & timer = holder->Timer;

    timer.Set();
    VERIFY_IS_TRUE(timer.IsSet);
    timer.Close();
}

BOOST_AUTO_TEST_CASE(TimerIsNotSetInCallback)
{
    RetryTimer* retryTimer = nullptr;
    bool timerSetValueInCallback = false;
        
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&timerSetValueInCallback, &retryTimer, &isInvoked] 
                                    {
                                        timerSetValueInCallback = retryTimer->IsSet;
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    retryTimer = &timer;
    timer.Set();

    BusyWaitUntil([&] () { return isInvoked.load(); });

    VERIFY_IS_FALSE(timerSetValueInCallback);

    timer.Close();
}

BOOST_AUTO_TEST_CASE(CallbackIsInvokedOnlyOnceRegardlessOfSetCount)
{
    Common::atomic_long value(0);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&value] 
                                    {
                                        value++;
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Set();
    timer.Set();
        
    BusyWaitUntil([&] () { return 1 == value.load(); });
        
    VERIFY_ARE_EQUAL(1, value.load());
    timer.Close();
}

BOOST_AUTO_TEST_CASE(TryCancelWithEqualSequenceNumberCancelsCallback)
{
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&isInvoked] 
                                    {
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Set();
    VERIFY_IS_TRUE(timer.IsSet);
    VERIFY_IS_TRUE(timer.TryCancel(timer.SequenceNumber));
    VERIFY_IS_FALSE(timer.IsSet);
    Sleep(2 * MiddleTimerDurationInMs);

    // the isInvoked should be false
    VERIFY_IS_FALSE(isInvoked.load());
        
    timer.Close();
}

BOOST_AUTO_TEST_CASE(TryCancelWithLowerSequenceNumberDoesNotCancelCallback)
{
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&isInvoked] 
                                    {
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Set();
    VERIFY_IS_TRUE(timer.IsSet);
    VERIFY_IS_FALSE(timer.TryCancel(timer.SequenceNumber - 1));
    VERIFY_IS_TRUE(timer.IsSet);
        
    BusyWaitUntil([&isInvoked] { return isInvoked.load(); }, 20, MiddleTimerDurationInMs * 10);

    timer.Close();
}

BOOST_AUTO_TEST_CASE(SettingTheTimerInTheCallbackWorks)
{
    RetryTimer* timerPtr = nullptr;
        
    Common::atomic_long count(0);        

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&count, &timerPtr] 
                                    {
                                        count++;
                                        if (count.load() == 1)
                                        {
                                            timerPtr->Set();
                                        }
                                    });

    RetryTimer & timer = holder->Timer;

    timerPtr = &timer;

    timer.Set();

    BusyWaitUntil([&count] { return count.load() == 2; });

    timer.Close();
}

BOOST_AUTO_TEST_CASE(SetTimerThenCancelAndThenSetFiresTimer)
{
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&isInvoked] 
                                    {
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    // Set the timer
    timer.Set();

    // Cancel the timer
    timer.TryCancel(timer.SequenceNumber);

    // call back cant have been invoked
    VERIFY_IS_FALSE(isInvoked.load());

    // Set the timer again
    timer.Set();

    // callback should be invoked

    BusyWaitUntil([&isInvoked] { return isInvoked.load();});
    timer.Close();
}

BOOST_AUTO_TEST_CASE(SetIsNoOpAfterClosingTimer)
{
    Common::atomic_bool isInvoked(false);

    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [&isInvoked] 
                                    {
                                        isInvoked.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    // Close the timer
    timer.Close();

    // set
    timer.Set();
    VERIFY_IS_FALSE(timer.IsSet);

    // wait 
    Sleep(MiddleTimerDurationInMs * 4);

    // assert that cb was not invoked
    VERIFY_IS_FALSE(isInvoked.load());
}

BOOST_AUTO_TEST_CASE(CloseSetsTimerToNull)
{
    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [] 
                                    {            
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Close();

    VERIFY_IS_NULL(timer.Test_GetRawTimer());
}

BOOST_AUTO_TEST_CASE(SuccessfulTryCancelAfterCloseDoesNotRecreateTimer)
{
    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [] 
                                    {            
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Close();

    timer.TryCancel(timer.SequenceNumber);
    VERIFY_IS_FALSE(timer.IsSet);

    VERIFY_IS_NULL(timer.Test_GetRawTimer());
}

BOOST_AUTO_TEST_CASE(MultipleCloseIsNoOp)
{
    auto holder = CreateTimerHolder(MiddleTimerDurationInMs, [] 
                                    {            
                                    });

    RetryTimer & timer = holder->Timer;

    timer.Close();

    timer.Close();
    VERIFY_IS_FALSE(timer.IsSet);
}

BOOST_AUTO_TEST_CASE(CloseTimerWhileCallbackIsRunningDoesNotBlowUp)
{
    ManualResetEvent ev;
    Common::atomic_bool hasCallbackStarted(false);
    Common::atomic_bool hasCallbackEnded(false);

    auto holder = CreateTimerHolder(ShortTimerDurationInMs, [&] 
                                    {            
                                        hasCallbackStarted.store(true);
                                        ev.WaitOne();
                                        hasCallbackEnded.store(true);
                                    });

    RetryTimer & timer = holder->Timer;

    // reset the event
    ev.Reset();
        
    // Set the timer
    timer.Set();

    // Wait for the callback to start and it to wait on the event
    BusyWaitUntil([&] { return hasCallbackStarted.load(); });

    // Close the timer while callback is running
    timer.Close();

    // Timer should be null
    VERIFY_IS_TRUE(nullptr == timer.Test_GetRawTimer());

    // Set the event - this resumes the cb
    ev.Set();

    // Wait for callback to end
    BusyWaitUntil([&] { return hasCallbackEnded.load(); });

    // Set again - this should not AV either
    timer.Set();
}

BOOST_AUTO_TEST_CASE(ActivityIdPrefixTestHelper)
{
    ExclusiveLock lock;
    vector<wstring> activityIds;

    size_t totalTries = 3;

    auto holder = CreateTimerHolderEx(ShortTimerDurationInMs, [&] (wstring const & aid, ReconfigurationAgent&)
                                        {
                                            AcquireExclusiveLock grab(lock);
                                            activityIds.push_back(aid);
                                        });

    RetryTimer & timer = holder->Timer;

    // let callbacks happen
    for(size_t i = 0; i < totalTries; i++)
    {
        size_t expected = i + 1;
        timer.Set();
        BusyWaitUntil([&] () -> bool
            {
                AcquireExclusiveLock grab(lock);
                return expected == activityIds.size();
            });
    }

    // close timer
    timer.Close();

    // assert three callbacks
    VERIFY_ARE_EQUAL(totalTries, activityIds.size());

    // each aid should start with the correct prefix
    for(size_t i = 0; i < activityIds.size(); i++)
    {
        VERIFY_ARE_EQUAL(0, activityIds[i].compare(0, DefaultActivityIdPrefix.size(), DefaultActivityIdPrefix.c_str()));
    }

    // each aid should be unique
    for(size_t i = 0; i < activityIds.size(); i++)
    {
        for(size_t j = 0; j < activityIds.size(); j++)
        {
            if (i == j) 
            {
                continue;
            }

            VERIFY_ARE_NOT_EQUAL(activityIds[i], activityIds[j]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
