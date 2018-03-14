// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Timer.h"

using namespace std;

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(TimerTests)

    void BasicTest(TimeSpan dueTime)
    {
        Common::atomic_long callCounter(0);
        ManualResetEvent callbackReadyToComplete(false);
        ManualResetEvent callbackAllowedToComplete(false);
        ManualResetEvent callbackCompleted(false);

        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [&callCounter, &callbackReadyToComplete, &callbackAllowedToComplete, &callbackCompleted]
            (TimerSPtr const &)
            {
                ++ callCounter;
                callbackReadyToComplete.Set();
                VERIFY_IS_TRUE(callbackAllowedToComplete.WaitOne(TimeSpan::FromSeconds(30)));
                callbackCompleted.Set();
            });

        timer->Change(dueTime);

        callbackReadyToComplete.WaitOne();
        Trace.WriteInfo(TraceType, "callbackReadyToComplete.WaitOne() returned");
        VERIFY_IS_TRUE(callCounter.load() == 1);

        callbackAllowedToComplete.Set();

        timer->Cancel();
        timer.reset();

        VERIFY_IS_TRUE(callbackCompleted.WaitOne(TimeSpan::FromSeconds(30)));
    }

    void TestCancelInCallback()
    {
        Common::ManualResetEvent callbackStartSignal(false);
        Common::ManualResetEvent callbackCompleted(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [&](TimerSPtr const & timerSPtr)
            {
                Trace.WriteNoise(TraceType, "in timer callback refCount = {0}", timerSPtr.use_count());
                callbackStartSignal.WaitOne();
                Trace.WriteNoise(TraceType, "Cancelling timer in callback ...");
                timerSPtr->Cancel();

                Trace.WriteNoise(TraceType, "in timer callback refCount = {0}", timerSPtr.use_count());
                Trace.WriteInfo(TraceType, "Timer canceled in callback");
                callbackCompleted.Set();
            });
        timer->Change(TimeSpan::Zero);
        callbackStartSignal.Set();
        callbackCompleted.WaitOne();
        Trace.WriteInfo(TraceType, "callbackCompleted.WaitOne() returned");
        timer.reset();
        LONG refCount = timer.use_count();
        Trace.WriteNoise(TraceType, "timer refCount = {0}", refCount);
        VERIFY_IS_TRUE(refCount <= 1);
    }

    void TestRecurringNoConcurrency()
    {
        const LONG total = 10;
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const &)
            {
                if (callCounter->load() < total)
                {
                    ++ (*callCounter);
                    if (callCounter->load() == total)
                    {
                        taskCompleted->Set();
                    }
                }
            }, false);

        timer->Change(Common::TimeSpan::Zero, Common::TimeSpan::FromMilliseconds(1));

        taskCompleted->WaitOne();
        Trace.WriteNoise(TraceType, "taskCompleted.WaitOne() returned");
        VERIFY_IS_TRUE(callCounter->load() == total);

        timer->Cancel();
    }

    void TestRecurringNoConcurrencyCancelInCallback()
    {
        const LONG total = 10;
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const & timerSPtr)
            {
                if (callCounter->load() < total)
                {
                    Trace.WriteNoise(TraceType, "incrementing callCounter ...");
                    auto value = ++ (*callCounter);
                    Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());

                    if (value == total)
                    {
                        Trace.WriteNoise(TraceType, "Cancelling timer in callback ...");
                        timerSPtr->Cancel();
                        Trace.WriteNoise(TraceType, "timer cancelled");
                        taskCompleted->Set();
                        Trace.WriteNoise(TraceType, "taskCompleted set");
                    }
                }
            }, false);
        timer->Change(Common::TimeSpan::Zero, Common::TimeSpan::FromMilliseconds(1));
        VERIFY_IS_TRUE(taskCompleted->WaitOne(TimeSpan::FromSeconds(30)));

#ifdef PLATFORM_UNIX
        // POSIX timer does not support wait for callback completion
        VERIFY_IS_TRUE(callCounter->load() >= total);
#else
        VERIFY_IS_TRUE(callCounter->load() == total);
#endif
    }

    void TestRecurringWithConcurrency()
    {
        const LONG teamSize = 10;
        shared_ptr<RwLock> lock = make_shared<RwLock>();
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        shared_ptr<ManualResetEvent> allCallbackReadySignal = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const &)
            {
                bool teamFull = false;
                bool needToWaitForOthers = false;
                {
                    AcquireWriteLock grab(*lock);
                    if (callCounter->load() < teamSize)
                    {
                        Trace.WriteNoise(TraceType, "incrementing callCounter ...");
                        ++ (*callCounter);
                        Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                        needToWaitForOthers = callCounter->load() < teamSize;
                    }
                    else
                    {
                        teamFull = true;
                    }
                }

                if (!teamFull)
                {
                    if (needToWaitForOthers)
                    {
                        Trace.WriteNoise(TraceType, "waiting for other callback instances to be called ...");
                        allCallbackReadySignal->WaitOne();
                        Trace.WriteNoise(TraceType, "all callback instances are ready now.");
                    }
                    else
                    {
                        Trace.WriteNoise(TraceType, "setting allCallbacksReadySignal ...");
                        allCallbackReadySignal->Set();
                        Trace.WriteNoise(TraceType, "allCallbacksReadySignal set");
                    }

                    bool setTaskCompleted;
                    {
                        AcquireWriteLock grab(*lock);
                        Trace.WriteNoise(TraceType, "concurrently incrementing callCounter ...");
                        ++ (*callCounter);
                        Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                        setTaskCompleted = (callCounter->load() == teamSize + teamSize);
                    }

                    if (setTaskCompleted)
                    {
                        Trace.WriteNoise(TraceType, "setting taskCompleted");
                        taskCompleted->Set();
                        Trace.WriteNoise(TraceType, "taskCompleted set");
                    }
                }
            },
            true);
        timer->Change(Common::TimeSpan::Zero, Common::TimeSpan::FromMilliseconds(1));
        taskCompleted->WaitOne();
        Trace.WriteNoise(TraceType, "taskCompleted.WaitOne() returned");
        VERIFY_IS_TRUE(callCounter->load() == teamSize + teamSize);
        timer->Cancel();
        Trace.WriteNoise(TraceType, "timer cancelled");
    }

    void TestRecurringWithConcurrencyCancelInCallback()
    {
        const LONG teamSize = 10;
        shared_ptr<RwLock> lock = make_shared<RwLock>();
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        shared_ptr<ManualResetEvent> allCallbackReadySignal = make_shared<ManualResetEvent>(false);
        shared_ptr<atomic_bool> timerCanceled = make_shared<atomic_bool>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const & timerSPtr)
            {
                bool teamFull = false;
                bool needToWaitForOthers = false;
                {
                    AcquireWriteLock grab(*lock);
                    if (callCounter->load() < teamSize)
                    {
                        Trace.WriteNoise(TraceType, "incrementing callCounter ...");
                        ++ (*callCounter);
                        Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                        needToWaitForOthers = callCounter->load() < teamSize;
                    }
                    else
                    {
                        teamFull = true;
                    }
                }

                if (!teamFull)
                {
                    if (needToWaitForOthers)
                    {
                        Trace.WriteNoise(TraceType, "waiting for other callback instances to be called ...");
                        allCallbackReadySignal->WaitOne();
                        Trace.WriteNoise(TraceType, "all callback instances are ready now.");
                    }
                    else
                    {
                        Trace.WriteNoise(TraceType, "setting allCallbacksReadySignal ...");
                        allCallbackReadySignal->Set();
                        Trace.WriteNoise(TraceType, "allCallbacksReadySignal set");
                    }

                    bool setTaskCompleted;
                    {
                        AcquireWriteLock grab(*lock);
                        Trace.WriteNoise(TraceType, "concurrently incrementing callCounter ...");
                        ++ (*callCounter);
                        Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                        setTaskCompleted = (callCounter->load() == teamSize + teamSize);
                    }

                    if (setTaskCompleted)
                    {
                        timerCanceled->store(true);
                        Trace.WriteNoise(TraceType, "Cancelling timer in callback ...");
                        timerSPtr->Cancel();
                        Trace.WriteNoise(TraceType, "timer got canceled in callback");
                        taskCompleted->Set();
                        Trace.WriteNoise(TraceType, "taskCompleted set");
                    }
                }
            },
            true);
        timer->Change(Common::TimeSpan::Zero, Common::TimeSpan::FromMilliseconds(1));
        taskCompleted->WaitOne();
        Trace.WriteNoise(TraceType, "taskCompleted.WaitOne() returned");
        VERIFY_IS_TRUE(callCounter->load() == teamSize + teamSize);
        VERIFY_IS_TRUE(timerCanceled->load());
    }


#ifdef PLATFORM_UNIX

#include <unistd.h>
#include <sys/syscall.h>

    BOOST_AUTO_TEST_CASE(BasicTest_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        BasicTest(TimeSpan::Zero);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestOneTickDueTime_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        BasicTest(TimeSpan::FromTicks(1));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerCancelInCallback_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        TestCancelInCallback();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringNoConcurrency_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        TestRecurringNoConcurrency();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringNoConcurrencyCancelInCallback_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        TestRecurringNoConcurrencyCancelInCallback();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringWithConcurrency_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        TestRecurringWithConcurrency();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringWithConcurrencyCancelInCallback_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        TestRecurringWithConcurrencyCancelInCallback();

        LEAVE;
    }

    void ScaleTestFunc(TimeSpan dueTime, bool limitToOneShot)
    {
        auto timerCountBefore = Timer::ObjCount.load();
        Trace.WriteInfo(TraceType, "timerCount before = {0}", timerCountBefore);

        auto testStart = Stopwatch::Now();

        vector<TimerSPtr> timers;
        Common::AutoResetEvent allCompleted(false);
        atomic_long callCount(0);
        const uint timerCount = 10000;
        for(uint i = 0; i < timerCount; ++i)
        {
            TimerSPtr timer = Timer::Create(
                TimerTagDefault,
                [&](TimerSPtr const & timer)
                {
                    auto value = ++callCount;
                    Trace.WriteInfo(TraceType, "tid = {0:x}, callCount = {1}", syscall(SYS_gettid), value); 
                    if (value == timerCount)
                    {
                        allCompleted.Set();
                    }
                });

            if (limitToOneShot) timer->LimitToOneShot();
            timers.emplace_back(move(timer));
        }

        for(auto const & timer : timers)
        {
            timer->Change(dueTime);
        }

        const TimeSpan CompletionWait = TimeSpan::FromSeconds(30);
        bool expectComplete = (dueTime < CompletionWait); 
        if (expectComplete)
        {
            BOOST_REQUIRE(allCompleted.WaitOne(CompletionWait));
        }
        auto afterWait = Stopwatch::Now();

        auto timerDisposeDelay = CommonConfig::GetConfig().TimerDisposeDelay;
        auto extraWait = timerDisposeDelay + timerDisposeDelay - (afterWait - testStart);
        if (extraWait > TimeSpan::Zero)
        {
            Trace.WriteInfo(TraceType, "wait {0} for timers created before this test to dispose", extraWait);
            Sleep(extraWait.TotalPositiveMilliseconds());
        }

        // clean up timers created in this test
        for(auto const & timer : timers)
        {
            timer->Cancel();
        }
        timers.clear();

        // wait for TimerFinalizer to dispose all timers
        // config Common/TimerDisposeDelay is set to 1 seconds in Common.Test.exe.cfg
        auto checkInterval = TimeSpan::FromMilliseconds(300);
        TimeSpan totalWait = TimeSpan::Zero;
        while((Timer::ObjCount.load() > timerCountBefore) && (totalWait < TimeSpan::FromSeconds(60)))
        {
            Trace.WriteInfo(TraceType, "Timer::ObjCount = {0}", Timer::ObjCount.load());
            Sleep(checkInterval.TotalMilliseconds());
            totalWait = totalWait + checkInterval;
        }

        auto timerCountAfter = Timer::ObjCount.load();
        Trace.WriteInfo(TraceType, "timerCount after = {0}", timerCountAfter);
        VERIFY_IS_TRUE(timerCountBefore >= timerCountAfter);

        if (expectComplete)
        {
            BOOST_REQUIRE(callCount.load() == timerCount); //make sure callback is not called more than requested
        }
    }

    BOOST_AUTO_TEST_CASE(ScaleTest)
    {
        ENTER;

        ScaleTestFunc(TimeSpan::Zero, false);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ScaleTest_AlwaysQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 0;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        ScaleTestFunc(TimeSpan::Zero, false);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ScaleTest_MayQueue)
    {
        ENTER;

        auto saved = CommonConfig::GetConfig().PosixTimerLimit;
        CommonConfig::GetConfig().PosixTimerLimit = 10;
        KFinally([=] { CommonConfig::GetConfig().PosixTimerLimit = saved; });

        ScaleTestFunc(TimeSpan::Zero, false);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ScaleTest_LimitToOneShot)
    {
        ENTER;

        ScaleTestFunc(TimeSpan::Zero, true);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ScaleTest_LimitToOneShot2)
    {
        ENTER;

        ScaleTestFunc(TimeSpan::FromSeconds(100), true);

        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(TestTimerWaitOnCancel)
    {
        ENTER;

        Common::atomic_long callCounter(0);
        Common::AutoResetEvent callbackStarted(false);
        TimerSPtr timer = Timer::Create(
            "TestTimerWaitOnCancel",
            [&callCounter, &callbackStarted](TimerSPtr const &)
            {
                callbackStarted.Set();
                ::Sleep(100); // Wait for Cancel() to be called on the main test thread
                ++ callCounter;
            },
            false);

        timer->SetCancelWait();
        timer->Change(Common::TimeSpan::Zero);
        VERIFY_IS_TRUE(callbackStarted.WaitOne(Common::TimeSpan::FromSeconds(30)));
        timer->Cancel();
        timer.reset();

        auto callCounterValue = callCounter.load();
        Trace.WriteInfo(TraceType, "callCounter = {0}", callCounterValue);
        BOOST_REQUIRE_EQUAL(callCounterValue, 1); 

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerWaitOnCancel2)
    {
        ENTER;

        Common::atomic_long callCounter(0);
        Common::AutoResetEvent callbackStarted(false);
        TimerSPtr timer = Timer::Create(
            "TestTimerWaitOnCancel2",
            [&callCounter, &callbackStarted](TimerSPtr const &)
            {
                callbackStarted.Set();
                ::Sleep(100); // Wait for Cancel() to be called on the main test thread
                ++ callCounter;
            },
            false);

        timer->SetCancelWait();
        timer->Change(TimeSpan::Zero, TimeSpan::FromMilliseconds(1));
        VERIFY_IS_TRUE(callbackStarted.WaitOne(Common::TimeSpan::FromSeconds(30)));
        timer->Cancel();
        timer.reset();

        auto callCounterValue = callCounter.load();
        Trace.WriteInfo(TraceType, "callCounter = {0}", callCounterValue);
        BOOST_REQUIRE(callCounterValue >= 1);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerWaitOnCancelFromCallback)
    {
        ENTER;

        Common::AutoResetEvent callbackCompleted(false);
        TimerSPtr timer = Timer::Create(
            __FUNCTION__,
            [&callbackCompleted](TimerSPtr const & timer)
            {
                // This cancel will be scheduled on threadpool, so no deadlock will occur
                timer->Cancel();
                callbackCompleted.Set();
            },
            false);

        timer->SetCancelWait();
        timer->Change(Common::TimeSpan::Zero);
        VERIFY_IS_TRUE(callbackCompleted.WaitOne(Common::TimeSpan::FromSeconds(30)));
        timer.reset();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestOneTickDueTime)
    {
        ENTER;
        BasicTest(TimeSpan::FromTicks(1));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerCancelInCallback)
    {
        ENTER;

        TestCancelInCallback();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringNoConcurrency)
    {
        ENTER;

        TestRecurringNoConcurrency();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringNoConcurrencyCancelInCallback)
    {
        ENTER;

        TestRecurringNoConcurrencyCancelInCallback();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringWithConcurrency)
    {
        ENTER;

        TestRecurringWithConcurrency();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerRecurringWithConcurrencyCancelInCallback)
    {
        ENTER;

        TestRecurringWithConcurrencyCancelInCallback();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerChangingDueTimeBeforeFiring)
    {
        ENTER;

        Common::atomic_long callCounter(0);
        shared_ptr<ManualResetEvent> callbackReadyToComplete = make_shared<ManualResetEvent>(false);
        shared_ptr<ManualResetEvent> callbackAllowedToComplete = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [&callCounter, callbackReadyToComplete, callbackAllowedToComplete](TimerSPtr const &)
            {
                ++ callCounter;
                callbackReadyToComplete->Set();
                VERIFY_IS_TRUE(callbackAllowedToComplete->WaitOne(TimeSpan::FromSeconds(30)));
            });

        timer->Change(Common::TimeSpan::FromSeconds(3));
        timer->Change(Common::TimeSpan::Zero);

        callbackAllowedToComplete->Set();

        VERIFY_IS_TRUE(callbackReadyToComplete->WaitOne(TimeSpan::FromSeconds(30)));
        VERIFY_IS_TRUE(callCounter.load() == 1);

        timer->Cancel();
        timer.reset();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerChangingDueTimeInCallback)
    {
        ENTER;

        const LONG totalExpected = 10;
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const & timerSPtr)
            {
                if (callCounter->load() < totalExpected)
                {
                    Trace.WriteNoise(TraceType, "incrementing callCounter ...");
                    ++ (*callCounter);
                    Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                    timerSPtr->Change(Common::TimeSpan::FromMilliseconds(1));
                }
                else
                {
                    taskCompleted->Set();
                    Trace.WriteNoise(TraceType, "taskCompleted set");
                }
            },
            true);
        timer->Change(Common::TimeSpan::Zero);
        taskCompleted->WaitOne();
        timer->Cancel();
        Trace.WriteInfo(TraceType, "callCounter = {0}", callCounter->load());
        VERIFY_IS_TRUE(callCounter->load() == totalExpected);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(TestTimerChangingDueTimeInCallbackCancelInCallback)
    {
        ENTER;

        const LONG totalExpected = 10;
        shared_ptr<atomic_long> callCounter = make_shared<atomic_long>(0);
        shared_ptr<ManualResetEvent> taskCompleted = make_shared<ManualResetEvent>(false);
        TimerSPtr timer = Timer::Create(
            TimerTagDefault,
            [=](TimerSPtr const & timerSPtr)
            {
                if (callCounter->load() == totalExpected)
                {
                    Trace.WriteNoise(TraceType, "Cancelling timer in callback ...");
                    timerSPtr->Cancel();
                    Trace.WriteNoise(TraceType, "timer got canceled in callback");
                    taskCompleted->Set();
                    Trace.WriteNoise(TraceType, "taskCompleted set");
                }
                else
                {
                    Trace.WriteNoise(TraceType, "incrementing callCounter ...");
                    ++ (*callCounter);
                    Trace.WriteNoise(TraceType, "callCounter incremented to {0}", callCounter->load());
                    timerSPtr->Change(Common::TimeSpan::FromMilliseconds(1));
                }
            },
            true);
        timer->Change(Common::TimeSpan::Zero);
        taskCompleted->WaitOne();
        Trace.WriteInfo(TraceType, "callCounter = {0}", callCounter->load());
        VERIFY_IS_TRUE(callCounter->load() == totalExpected);

        LEAVE;
    }


    BOOST_AUTO_TEST_CASE(CancelWithoutStart)
    {
        ENTER;

        TimerSPtr timer = Timer::Create(TimerTagDefault, [](TimerSPtr const &) {});
        timer->Cancel();

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
