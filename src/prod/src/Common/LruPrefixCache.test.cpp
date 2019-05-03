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

    StringLiteral const TraceComponent("LruPrefixCacheTest");

    class LruPrefixCacheTest
    {
    public:

        class TestCacheEntry : public LruCacheEntryBase<NamingUri>
        {
        public:
            explicit TestCacheEntry(NamingUri const & key) 
                : LruCacheEntryBase(key)
                , value_(-1)
            { 
            }

            explicit TestCacheEntry(NamingUri && key) 
                : LruCacheEntryBase(move(key))
                , value_(-1)
            { 
            }

            TestCacheEntry(NamingUri const & key, int value) 
                : LruCacheEntryBase(key)
                , value_(value)
            { }

            static size_t GetHash(NamingUri const & key) { return key.GetHash(); }
            static bool AreEqualKeys(NamingUri const & left, NamingUri const & right) { return left == right; }
            static bool ShouldUpdateUnderLock(TestCacheEntry const &, TestCacheEntry const &) { return true; } 

            int GetValue() const { return value_; }

        private:
            int value_;
        };

    protected:
        typedef shared_ptr<TestCacheEntry> EntrySPtr;
        typedef LruCacheWaiterAsyncOperation<TestCacheEntry> WaiterType;
        typedef shared_ptr<WaiterType> WaiterSPtr;
        typedef LruPrefixCache<NamingUri, TestCacheEntry> PrefixCache;

        LruPrefixCacheTest() { BOOST_REQUIRE(TestcaseSetup()); }
        TEST_METHOD_SETUP(TestcaseSetup)

        void GetAndVerify(
            __in PrefixCache &,
            NamingUri const & name,
            ErrorCodeValue::Enum expectedError,
            bool expectedIsFirstWaiter,
            int expectedEntry);
        void PutAndVerify(
            __in PrefixCache &,
            NamingUri const & name, 
            int entry);
        void PutAndVerify(
            __in PrefixCache &,
            NamingUri const & name, 
            int entry, 
            bool expectedPutResult, 
            int expectedEntry);

        void WaitUntil(atomic_long const & counter, long expected);
        void WaitNot(atomic_long const & counter, long expected);
        long WaitWhileNot(atomic_long const & counter, long expected);
    };

    bool LruPrefixCacheTest::TestcaseSetup()
    {
        Config cfg;
        return true;
    }

    void LruPrefixCacheTest::GetAndVerify(
        __in PrefixCache & prefixCache,
        NamingUri const & name,
        ErrorCodeValue::Enum expectedError,
        bool expectedIsFirstWaiter,
        int expectedEntry)
    {
        Trace.WriteInfo(TraceComponent, "GetAndVerify({0})", name);
       
        ManualResetEvent event(false);

        auto operation = prefixCache.BeginTryGet(
            name, 
            TimeSpan::MaxValue,
            [&event](AsyncOperationSPtr const &) { event.Set(); },
            AsyncOperationSPtr());

        event.WaitOne();

        bool isFirstWaiter = false;
        EntrySPtr entry;
        auto error = prefixCache.EndTryGet(operation, isFirstWaiter, entry);

        VERIFY_IS_TRUE_FMT(error.IsError(expectedError), "error={0} expected={1}", error, expectedError);
        VERIFY_IS_TRUE_FMT(isFirstWaiter == expectedIsFirstWaiter, "isFirstWaiter={0} expectedIsFirstWaiter={1}", isFirstWaiter, expectedIsFirstWaiter);

        if (expectedEntry < 0)
        {
            VERIFY_IS_TRUE_FMT(entry == nullptr, "entry={0} expected=nullptr", (entry ? wformatString("{0}", entry->GetValue()) : L"nullptr")); 
        }
        else
        {
            if (entry == nullptr)
            {
                VERIFY_IS_TRUE_FMT(entry != nullptr, "entry=nullptr expected={0}", expectedEntry);
            }
            else
            {
                VERIFY_IS_TRUE_FMT(entry->GetValue() == expectedEntry, "entry={0} expected={1}", (entry ? wformatString("{0}", entry->GetValue()) : L"nullptr"), expectedEntry);
            }
        }
    }

    void LruPrefixCacheTest::PutAndVerify(
        __in PrefixCache & prefixCache,
        NamingUri const & name, 
        int entry)
    {
        PutAndVerify(prefixCache, name, entry, true, entry);
    }

    void LruPrefixCacheTest::PutAndVerify(
        __in PrefixCache & prefixCache,
        NamingUri const & name, 
        int entry, 
        bool expectedPutResult, 
        int expectedEntry)
    {
        Trace.WriteInfo(TraceComponent, "PutAndVerify({0})", name);

        auto entrySPtr = make_shared<TestCacheEntry>(name, entry);
        auto putResult = prefixCache.TryPutOrGet(name, entrySPtr);
        auto entryPtr = entrySPtr.get();

        VERIFY_IS_TRUE_FMT(putResult == expectedPutResult, "putResult={0} expected={1}", putResult, expectedPutResult);
        VERIFY_IS_TRUE_FMT(entrySPtr->GetValue() == expectedEntry, "entry={0} expected={1}", entrySPtr->GetValue(), expectedEntry);

        if (putResult)
        {
            VERIFY_IS_TRUE_FMT(entryPtr == entrySPtr.get(), "ptr={0} expected={1}", static_cast<void*>(entryPtr), static_cast<void*>(entrySPtr.get()));
        }
    }

    void LruPrefixCacheTest::WaitUntil(atomic_long const & counter, long expected)
    {
        auto value = WaitWhileNot(counter, expected);
        VERIFY_IS_TRUE_FMT(value == expected, "counter={0} expected={1}", value, expected);
    } 

    void LruPrefixCacheTest::WaitNot(atomic_long const & counter, long expected)
    {
        auto value = WaitWhileNot(counter, expected);
        VERIFY_IS_TRUE_FMT(value != expected, "counter={0} expected={1}", value, expected);
    } 

    long LruPrefixCacheTest::WaitWhileNot(atomic_long const & counter, long expected)
    {
        int retryCount = 5 * 5;
        while (counter.load() != expected && retryCount > 0)
        {
            --retryCount;

            Sleep(200);
        }

        return counter.load();
    }

    BOOST_FIXTURE_TEST_SUITE(LruPrefixCacheTestSuite, LruPrefixCacheTest)

    // Tests basic prefix-matching on cache and results of updating cache
    //
    BOOST_AUTO_TEST_CASE(PrefixCacheTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "*** PrefixCacheTest");

        LruCache<NamingUri, TestCacheEntry> lruCache(numeric_limits<size_t>::max());
        PrefixCache prefixCache(lruCache);

        // Exact match
        {
            NamingUri name(L"a/b/c/d/e");

            GetAndVerify(prefixCache, name, ErrorCodeValue::Success, true, -1);

            PutAndVerify(prefixCache, name, 1);

            GetAndVerify(prefixCache, name, ErrorCodeValue::Success, false, 1);
        }
        
        // Prefix match (root segment)
        {
            NamingUri authName(L"a");

            PutAndVerify(prefixCache, authName, 2);

            GetAndVerify(prefixCache, NamingUri(L"a/b/c/d/e"), ErrorCodeValue::Success, false, 1);

            GetAndVerify(prefixCache, NamingUri(L"a/b/c/d/f"), ErrorCodeValue::Success, false, 2);

            GetAndVerify(prefixCache, NamingUri(L"a/b/c/g"), ErrorCodeValue::Success, false, 2);

            GetAndVerify(prefixCache, NamingUri(L"a/i"), ErrorCodeValue::Success, false, 2);
        }
        
        // Update prefix match result via inner cache
        {
            NamingUri name1(L"a/b/x");

            GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, false, 2);

            NamingUri name2(L"a/c/y");

            GetAndVerify(prefixCache, name2, ErrorCodeValue::Success, false, 2);

            NamingUri insertName1(L"a/b");

            PutAndVerify(prefixCache, insertName1, 3);

            GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, false, 3);

            GetAndVerify(prefixCache, name2, ErrorCodeValue::Success, false, 2);

            NamingUri insertName2(L"a/c/y");

            PutAndVerify(prefixCache, insertName2, 4);

            GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, false, 3);

            GetAndVerify(prefixCache, name2, ErrorCodeValue::Success, false, 4);
        }
    }

    // Tests completion of waiters with both success and failure 
    //
    BOOST_AUTO_TEST_CASE(PrefixCacheWaitersTest)
    {
        Trace.WriteInfo(
            TraceComponent,
            "*** PrefixCacheWaitersTest");

        LruCache<NamingUri, TestCacheEntry> lruCache(numeric_limits<size_t>::max());
        PrefixCache prefixCache(lruCache);

        atomic_long completionCount(0);

        // Single waiter
        {
            NamingUri name(L"ab/cd/ef");

            GetAndVerify(prefixCache, name, ErrorCodeValue::Success, true, -1);

            Threadpool::Post([this, &prefixCache, &name, &completionCount]
            { 
                GetAndVerify(prefixCache, name, ErrorCodeValue::Success, false, 10);

                ++completionCount;
            });

            WaitNot(completionCount, 1);

            PutAndVerify(prefixCache, NamingUri(L"ab"), 11);

            WaitNot(completionCount, 1);

            PutAndVerify(prefixCache, NamingUri(L"ab/cd"), 12);

            WaitNot(completionCount, 1);

            PutAndVerify(prefixCache, name, 10);

            WaitUntil(completionCount, 1);

            GetAndVerify(prefixCache, name, ErrorCodeValue::Success, false, 10);
        }

        // Multiple waiters
        {
            NamingUri name1(L"x/y/z");

            GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, true, -1);

            Threadpool::Post([this, &prefixCache, &name1, &completionCount]
            { 
                GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, false, 20);

                ++completionCount;
            });

            Threadpool::Post([this, &prefixCache, &name1, &completionCount]
            { 
                GetAndVerify(prefixCache, name1, ErrorCodeValue::Success, false, 20);

                ++completionCount;
            });

            NamingUri name2(L"x/y/z/a");

            GetAndVerify(prefixCache, name2, ErrorCodeValue::Success, true, -1);

            Threadpool::Post([this, &prefixCache, &name2, &completionCount]
            { 
                GetAndVerify(prefixCache, name2, ErrorCodeValue::Success, false, 21);

                ++completionCount;
            });

            NamingUri name3(L"x");

            GetAndVerify(prefixCache, name3, ErrorCodeValue::Success, true, -1);

            Threadpool::Post([this, &prefixCache, &name3, &completionCount]
            { 
                GetAndVerify(prefixCache, name3, ErrorCodeValue::UserServiceNotFound, false, -1);

                ++completionCount;
            });

            Threadpool::Post([this, &prefixCache, &name3, &completionCount]
            { 
                GetAndVerify(prefixCache, name3, ErrorCodeValue::UserServiceNotFound, false, -1);

                ++completionCount;
            });
            
            WaitNot(completionCount, 2);

            PutAndVerify(prefixCache, name1, 20);

            WaitUntil(completionCount, 3);

            WaitNot(completionCount, 4);

            PutAndVerify(prefixCache, name2, 21);

            WaitUntil(completionCount, 4);

            WaitNot(completionCount, 5);

            prefixCache.FailWaiters(name3, ErrorCodeValue::UserServiceNotFound);

            WaitUntil(completionCount, 6);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}

