// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    StringLiteral const TraceComponent("LruCacheWaiterListTest");

    class LruCacheWaiterListTest
    {
    protected:
        typedef shared_ptr<int> EntrySPtr;
        typedef LruCacheWaiterAsyncOperation<int> WaiterType;
        typedef shared_ptr<WaiterType> WaiterSPtr;
        typedef LruCacheWaiterList<int> WaiterList;

        static const int TestWaiterCount = 32;
        static const int SmallTimeoutInMilliseconds = 1000;
        static const int LargeTimeoutInMilliseconds = 10 * 1000;
        static const int TimeoutFactor = 3;

        LruCacheWaiterListTest() { BOOST_REQUIRE(TestcaseSetup()); }
        TEST_METHOD_SETUP(TestcaseSetup)

        typedef function<void(WaiterList &, int)> TestCaseHelperAction;
        void TestCaseHelper(wstring const & testcase, TestCaseHelperAction const &);

        static void AddWaiters(WaiterList &, int waiterCount, ManualResetEvent &, atomic_long &, vector<WaiterSPtr> &, TimeSpan const timeout);
        static void AddAndCompleteWaiters(WaiterList &, int waiterCount);
        static void AddAndFailWaiters(WaiterList &, int waiterCount);
        static void AddAndTimeoutWaiters(WaiterList &, int waiterCount);
        static void AddMixedWaiters(WaiterList &, int waiterCount);
        static TimeSpan GetRandomTimeout(Random &, wstring const & traceTag);
        static void VerifyWaiterUseCounts(vector<WaiterSPtr> &);
    };

    bool LruCacheWaiterListTest::TestcaseSetup()
    {
        Config cfg;
        return true;
    }

    void LruCacheWaiterListTest::AddWaiters(
        WaiterList & list, 
        int waiterCount,
        ManualResetEvent & waiterEvent,
        atomic_long & pendingWaiters,
        vector<WaiterSPtr> & waiters,
        TimeSpan const timeout)
    {
        Random rand(static_cast<int>(DateTime::Now().Ticks));

        waiterEvent.Reset();

        AsyncOperationSPtr root;

        for (auto ix=0; ix<waiterCount; ++ix)
        {
            auto waiterTimeout = timeout;

            if (waiterTimeout == TimeSpan::Zero)
            {
                waiterTimeout = GetRandomTimeout(rand, wformatString("waiter_timeout_{0}", ix));
            }

            auto waiter = list.AddWaiter(
                waiterTimeout,
                [&](AsyncOperationSPtr const&) 
                { 
                    if (--pendingWaiters == 0) 
                    { 
                        waiterEvent.Set(); 
                    } 
                },
                root);
            waiters.push_back(move(waiter));
        }

        VERIFY_IS_TRUE_FMT(list.GetSize() == waiters.size(), "waiters = {0}", list.GetSize());

        // Waiters should not be completing until they are started with StartOutsideLock()

        bool success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(SmallTimeoutInMilliseconds));
        VERIFY_IS_TRUE_FMT(!success, "waiterEvent = {0}", success);
        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == waiters.size(), "pending = {0}", pendingWaiters.load());

        for (auto it=waiters.begin(); it!=waiters.end(); ++it)
        {
            (*it)->StartOutsideLock(*it);
        }

        bool isFirstWaiter;
        EntrySPtr entry;

        auto error = WaiterType::End(waiters.front(), isFirstWaiter, entry);

        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "error = {0}", error);
        VERIFY_IS_TRUE_FMT(isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
        VERIFY_IS_TRUE_FMT(!entry, "entry = {0}", entry.get());

        // Only the first waiter will complete immediately when started.
        // All subsequent waiters will either timeout or wait for external
        // completion by the first waiter.
        //
        if (waiterCount == 1)
        {
            success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(SmallTimeoutInMilliseconds));
            VERIFY_IS_TRUE_FMT(success, "waiterEvent = {0}", success);
        }
        else
        {
            success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(SmallTimeoutInMilliseconds));
            VERIFY_IS_TRUE_FMT(!success, "waiterEvent = {0}", success);
        }

        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == waiters.size() - 1, "pending = {0}", pendingWaiters.load());
    }

    void LruCacheWaiterListTest::AddAndCompleteWaiters(__in WaiterList & list, int waiterCount)
    {
        ManualResetEvent waiterEvent(false);
        atomic_long pendingWaiters(waiterCount);
        vector<WaiterSPtr> waiters;

        AddWaiters(list, waiterCount, waiterEvent, pendingWaiters, waiters, TimeSpan::MaxValue);

        // First waiter completes successfully
        //
        auto expectedValue = Random(static_cast<int>(DateTime::Now().Ticks)).Next();

        list.CompleteWaiters(make_shared<int>(expectedValue));
        VERIFY_IS_TRUE_FMT(list.GetSize() == 0, "waiters = {0}", list.GetSize());

        bool success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(LargeTimeoutInMilliseconds));
        VERIFY_IS_TRUE_FMT(success, "waiterEvent = {0}", success);
        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == 0, "pending = {0}", pendingWaiters.load());

        for (auto it=(waiters.begin() + 1); it!=waiters.end(); ++it)
        {
            bool isFirstWaiter;
            EntrySPtr entry;

            auto error = WaiterType::End(*it, isFirstWaiter, entry);

            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "error = {0}", error);
            VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
            VERIFY_IS_TRUE_FMT(entry, "entry = {0}", entry.get());
            VERIFY_IS_TRUE_FMT(*entry == expectedValue, "entry = {0}", *entry);
        }

        VerifyWaiterUseCounts(waiters);
    }

    void LruCacheWaiterListTest::AddAndFailWaiters(__in WaiterList & list, int waiterCount)
    {
        ManualResetEvent waiterEvent(false);
        atomic_long pendingWaiters(waiterCount);
        vector<WaiterSPtr> waiters;

        AddWaiters(list, waiterCount, waiterEvent, pendingWaiters, waiters, TimeSpan::MaxValue);

        // First waiter completes with an error
        //
        ErrorCode expectedError(ErrorCodeValue::OperationCanceled);

        list.CompleteWaiters(expectedError);
        VERIFY_IS_TRUE_FMT(list.GetSize() == 0, "waiters = {0}", list.GetSize());

        bool success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(LargeTimeoutInMilliseconds));
        VERIFY_IS_TRUE_FMT(success, "waiterEvent = {0}", success);
        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == 0, "pending = {0}", pendingWaiters.load());

        for (auto it=(waiters.begin() + 1); it!=waiters.end(); ++it)
        {
            bool isFirstWaiter;
            EntrySPtr entry;

            auto error = WaiterType::End(*it, isFirstWaiter, entry);

            VERIFY_IS_TRUE_FMT(error.IsError(expectedError.ReadValue()), "error = {0}", error);
            VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
            VERIFY_IS_TRUE_FMT(!entry, "entry = {0}", entry.get());
        }

        VerifyWaiterUseCounts(waiters);
    }
    void LruCacheWaiterListTest::AddAndTimeoutWaiters(__in WaiterList & list, int waiterCount)
    {
        ManualResetEvent waiterEvent(false);
        atomic_long pendingWaiters(waiterCount);
        vector<WaiterSPtr> waiters;

        AddWaiters(list, waiterCount, waiterEvent, pendingWaiters, waiters, 
            TimeSpan::FromMilliseconds(SmallTimeoutInMilliseconds * TimeoutFactor ));

        ErrorCode expectedError(ErrorCodeValue::Timeout);

        // First waiter does not complete, waiters will eventually timeout
        //
        bool success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(LargeTimeoutInMilliseconds));
        VERIFY_IS_TRUE_FMT(success, "waiterEvent = {0}", success);
        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == 0, "pending = {0}", pendingWaiters.load());

        for (auto it=(waiters.begin() + 1); it!=waiters.end(); ++it)
        {
            bool isFirstWaiter;
            EntrySPtr entry;

            auto error = WaiterType::End(*it, isFirstWaiter, entry);

            VERIFY_IS_TRUE_FMT(error.IsError(expectedError.ReadValue()), "error = {0}", error);
            VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
            VERIFY_IS_TRUE_FMT(!entry, "entry = {0}", entry.get());
        }

        // When the first waiter finally completes, it must still remove
        // itself from the list.
        //
        list.CompleteWaiters(ErrorCodeValue::Timeout);
        VERIFY_IS_TRUE_FMT(list.GetSize() == 0, "waiters = {0}", list.GetSize());

        VerifyWaiterUseCounts(waiters);
    }

    void LruCacheWaiterListTest::AddMixedWaiters(__in WaiterList & list, int waiterCount)
    {
        ManualResetEvent waiterEvent(false);
        atomic_long pendingWaiters(waiterCount);
        vector<WaiterSPtr> waiters;

        // Generate random timeouts
        AddWaiters(list, waiterCount, waiterEvent, pendingWaiters, waiters, TimeSpan::Zero);

        // Delay a random amount of time before first waiter completes.
        //
        auto rand = Random(static_cast<int>(DateTime::Now().Ticks));

        auto expectedValue = rand.Next();

        auto delay = GetRandomTimeout(rand, L"first_waiter_delay").TotalMilliseconds();

        Sleep(static_cast<DWORD>(delay));

        list.CompleteWaiters(make_shared<int>(expectedValue));
        VERIFY_IS_TRUE_FMT(list.GetSize() == 0, "waiters = {0}", list.GetSize());

        // Expect random number of successful completions and timeouts
        //
        bool success = waiterEvent.WaitOne(TimeSpan::FromMilliseconds(LargeTimeoutInMilliseconds));
        VERIFY_IS_TRUE_FMT(success, "waiterEvent = {0}", success);
        VERIFY_IS_TRUE_FMT(pendingWaiters.load() == 0, "pending = {0}", pendingWaiters.load());

        int successCount = 0;
        int timeoutCount = 0;

        for (auto it=(waiters.begin() + 1); it!=waiters.end(); ++it)
        {
            bool isFirstWaiter;
            EntrySPtr entry;

            auto error = WaiterType::End(*it, isFirstWaiter, entry);

            // Waiters must either timeout or complete successfully.
            //
            if (error.IsSuccess())
            {
                VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
                VERIFY_IS_TRUE_FMT(entry, "entry = {0}", entry.get());
                VERIFY_IS_TRUE_FMT(*entry == expectedValue, "entry = {0}", *entry);

                ++successCount;
            }
            else
            {
                VERIFY_IS_TRUE_FMT(error.IsError(ErrorCodeValue::Timeout), "error = {0}", error);
                VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
                VERIFY_IS_TRUE_FMT(!entry, "entry = {0}", entry.get());

                ++timeoutCount;
            }
        }

        VERIFY_IS_TRUE_FMT(
            successCount + timeoutCount + 1 == waiterCount, 
            "success = {0} timeout = {1}", 
            successCount, 
            timeoutCount);

        VerifyWaiterUseCounts(waiters);

        Trace.WriteInfo(
            TraceComponent, 
            "Counts: delay = {0}ms, success = {1} timeout = {2}",
            delay,
            successCount,
            timeoutCount);
    }

    TimeSpan LruCacheWaiterListTest::GetRandomTimeout(Random & rand, wstring const & traceTag)
    {
        auto min = SmallTimeoutInMilliseconds * TimeoutFactor;
        auto max = min * 2;

        auto timeout = TimeSpan::FromMilliseconds(rand.Next(min, max));

        Trace.WriteInfo(
            TraceComponent, 
            "GetRandomTimeout({0}) = {1}",
            traceTag,
            timeout);

        return timeout;
    }

    void LruCacheWaiterListTest::VerifyWaiterUseCounts(vector<WaiterSPtr> & waiters)
    {
        int maxRetries = 10;
        int ix = 0;
        for (auto const & waiter : waiters)
        {
            auto useCount = waiter.use_count();
            for (auto retry=0; retry<maxRetries && useCount != 1; ++retry)
            {
                Trace.WriteInfo(
                    TraceComponent, 
                    "waiter {0}/{1} use_count retry {2}/{3}: {4} != 1", 
                    ix, 
                    waiters.size(), 
                    retry,
                    maxRetries,
                    useCount);

                Sleep(200);
                useCount = waiter.use_count();
            }

            VERIFY_IS_TRUE_FMT(useCount == 1, "waiter {0}/{1} use_count: {2} != 1 ({3})", ix, waiters.size(), useCount, waiter.use_count());

            ++ix;
        }
    }

    void LruCacheWaiterListTest::TestCaseHelper(
        wstring const & testcase,
        TestCaseHelperAction const & action)
    {
        WaiterList list;

        for (auto ix = 1; ix <= TestWaiterCount; ix *= 2)
        {
            Trace.WriteInfo(
                TraceComponent,
                "Testcase: {0} with {1} waiter(s)",
                testcase,
                ix);

            action(list, ix);
        }
    }

    BOOST_FIXTURE_TEST_SUITE(LruCacheWaiterListTestSuite, LruCacheWaiterListTest)

        // Test completion of pending waiters on success
        //
        BOOST_AUTO_TEST_CASE(CompleteWaitersTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "CompleteWaitersTest");

        TestCaseHelper(L"CompleteWaitersTest", AddAndCompleteWaiters);
    }

    // Test completion of pending waiters on failure
    //
    BOOST_AUTO_TEST_CASE(FailWaitersTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "FailWaitersTest");

        TestCaseHelper(L"FailWaitersTest", AddAndFailWaiters);
    }

    // Test timeout of pending waiters
    //
    BOOST_AUTO_TEST_CASE(TimeoutWaitersTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "TimeoutWaitersTest");

        TestCaseHelper(L"TimeoutWaitersTest", AddAndTimeoutWaiters);
    }

    // Test successful completion of pending waiters with some waiters already timed out.
    //
    BOOST_AUTO_TEST_CASE(MixedWaitersTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "MixedWaitersTest");

        TestCaseHelper(L"MixedWaitersTest", AddMixedWaiters);
    }

    BOOST_AUTO_TEST_CASE(CreateCompletedTest)
    {
        auto expected = make_shared<int>(42);

        auto operation = WaiterType::Create(
            expected,
            [](AsyncOperationSPtr const&) {},
            AsyncOperationSPtr());

        operation->StartOutsideLock(operation);

        bool isFirstWaiter;
        EntrySPtr entry;

        auto error = WaiterType::End(operation, isFirstWaiter, entry);

        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "error = {0}", error);
        VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
        VERIFY_IS_TRUE_FMT(entry, "entry = {0}", entry.get());
        VERIFY_IS_TRUE_FMT(*entry == *expected, "value = {0}", *entry);
    }

    BOOST_AUTO_TEST_CASE(CreateFailedTest)
    {
        auto expected = ErrorCodeValue::OperationFailed;

        auto operation = WaiterType::Create(
            expected,
            [](AsyncOperationSPtr const&) {},
            AsyncOperationSPtr());

        operation->StartOutsideLock(operation);

        bool isFirstWaiter;
        EntrySPtr entry;

        auto error = WaiterType::End(operation, isFirstWaiter, entry);

        VERIFY_IS_TRUE_FMT(error.IsError(expected), "error = {0}", error);
        VERIFY_IS_TRUE_FMT(!isFirstWaiter, "isFirstWaiter = {0}", isFirstWaiter);
        VERIFY_IS_TRUE_FMT(!entry, "entry = {0}", entry.get());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
