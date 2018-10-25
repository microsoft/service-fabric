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

    StringLiteral const TraceComponent("LruCacheTest");

    class LruCacheTest
    {
    protected:
        static const int MinKeyLength = 64;
        static const int MaxKeyLength = 128;
        static const int SmallTimeoutInMilliseconds = 1000;
        static const int LargeTimeoutInMilliseconds = 10 * 1000;

        class TestCacheEntry;
        class MultiThreadWorkerAsyncOperation;

        typedef shared_ptr<TestCacheEntry> TestCacheEntrySPtr;
        typedef LruCache<wstring, TestCacheEntry> TestCache;

        LruCacheTest() { BOOST_REQUIRE(TestcaseSetup()); }
        TEST_METHOD_SETUP(TestcaseSetup)

        void PerfTestHelper(
            int operationCount, 
            size_t capacity, 
            bool enableTrim);

        static wstring GetRandomKey(Random &);
        void SyncTryPut(TestCache &, wstring const & key, bool expectedSuccess);
        void SyncTryPut(TestCache &, wstring const & key, int version, bool expectedSuccess);
        void SyncTryGet(TestCache &, wstring const & key, bool expectedSuccess);
        void SyncTryRemove(TestCache &, wstring const & key, bool expectedSuccess);
        void SyncAddRemoveHelper(TestCache &, int keyCount);
        void ClearTestHelper(int keyCount);
    };

    // *** Testcase helper classes

    class LruCacheTest::TestCacheEntry : public LruCacheEntryBase<wstring>
    {
    public:
        explicit TestCacheEntry(wstring const & key) 
            : LruCacheEntryBase(key)
            , version_(0) 
            , value_()
        { 
        }

        explicit TestCacheEntry(wstring && key) 
            : LruCacheEntryBase(move(key))
            , version_(0) 
            , value_()
        { 
        }

        TestCacheEntry(wstring const & key, int version) 
            : LruCacheEntryBase(key)
            , version_(version) 
            , value_(key)
        { }

        TestCacheEntry(wstring const & key, wstring const & value, int version) 
            : LruCacheEntryBase(key)
            , version_(version) 
            , value_(value)
        { }

        static size_t GetHash(wstring const & key) { return StringUtility::GetHash(key); }
        static bool AreEqualKeys(wstring const & left, wstring const & right) { return left == right; }
        static bool ShouldUpdateUnderLock(TestCacheEntry const & existing, TestCacheEntry const & incoming) 
        { 
            if (incoming.version_ == 0 && existing.version_ == 0)
            {
                return true;
            }

            return (incoming.version_ > existing.version_); 
        }

        wstring GetValue() const { return value_; }

    private:
        int version_;
        wstring value_;
    };

    class LruCacheTest::MultiThreadWorkerAsyncOperation : public AsyncOperation
    {
    public:
        MultiThreadWorkerAsyncOperation(
            TestCache & cache,
            Random & rand,
            atomic_long & nextVersion,
            int workerId,
            wstring const & key,
            int targetWaiterCount,
            int targetIterations,
            atomic_long & iteration,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
            , cache_(cache)
            , rand_(rand)
            , nextVersion_(nextVersion)
            , workerId_(workerId)
            , key_(key)
            , targetWaiterCount_(targetWaiterCount)
            , targetIterations_(targetIterations)
            , iteration_(iteration)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            return AsyncOperation::End<MultiThreadWorkerAsyncOperation>(operation)->Error;
        }

        virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            this->StartTryGet(thisSPtr);
        }

    private:
        static const int FirstWaiterDelayInMilliseconds = 300;
        static const int MinWaiterTimeoutInMilliseconds = 10;
        static const int MaxWaiterTimeoutInMilliseconds = 1000;

        TimeSpan GetRandomTimeout()
        {
            auto timeout = TimeSpan::MaxValue;
            if (rand_.Next() % 2 == 0)
            {
                timeout = TimeSpan::FromMilliseconds(rand_.Next(
                    MinWaiterTimeoutInMilliseconds, 
                    MaxWaiterTimeoutInMilliseconds));
            }

            return timeout;
        }

        void StartTryGet(AsyncOperationSPtr const & thisSPtr)
        {
            auto timeout = this->GetRandomTimeout();

            Trace.WriteInfo(
                TraceComponent, 
                "BeginTryGet(key={0}, worker={1}): timeout={2}",
                key_,
                workerId_,
                timeout);

            auto operation = cache_.BeginTryGet(
                key_,
                timeout,
                [this](AsyncOperationSPtr const & operation){ this->OnTryGetCompleted(operation, false); },
                thisSPtr);
            this->OnTryGetCompleted(operation, true);
        }

        void OnTryGetCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto const & thisSPtr = operation->Parent;

            bool isFirstWaiter = false;
            TestCacheEntrySPtr entry;

            auto error = cache_.EndTryGet(operation, isFirstWaiter, entry);

            Trace.WriteInfo(
                TraceComponent, 
                "EndTryGet(key={0} worker={1}): error={2} isFirst={3}",
                key_,
                workerId_,
                error,
                isFirstWaiter);

            if (error.IsSuccess())
            {
                if (isFirstWaiter)
                {
                    this->ProcessFirstWaiterOnGet(thisSPtr);
                }
                else
                {
                    this->ProcessEntryOnGet(thisSPtr, entry);
                }
            }
            else if (error.IsError(ErrorCodeValue::Timeout))
            {
                this->StartTryGet(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, error);
            }
        }

        void ProcessFirstWaiterOnGet(AsyncOperationSPtr const & thisSPtr)
        {
            auto iteration = iteration_.load();
            auto waiterCount = cache_.GetWaiterCount(key_);
            if (iteration < targetIterations_ && waiterCount < targetWaiterCount_)
            {
                Trace.WriteInfo(
                    TraceComponent, 
                    "Wait-OnGet(key={0} worker={1}): waiters={2}/{3} iteration={4}",
                    key_,
                    workerId_,
                    waiterCount,
                    targetWaiterCount_,
                    iteration);

                Sleep(FirstWaiterDelayInMilliseconds);
                Threadpool::Post([this, thisSPtr]{ this->ProcessFirstWaiterOnGet(thisSPtr); });
                return;
            }

            auto entry = make_shared<TestCacheEntry>(key_, wformatString("worker-{0}-OnGet", workerId_), ++nextVersion_);

            bool success = cache_.TryPutOrGet(entry);

            Trace.WriteInfo(
                TraceComponent, 
                "TryPut-OnGet(key={0} worker={1}): value={2} success={3}",
                key_,
                workerId_,
                entry->GetValue(),
                success);

            if (success)
            {
                this->ProcessEntryOnGet(thisSPtr, entry);
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            }
        }

        void ProcessEntryOnGet(AsyncOperationSPtr const & thisSPtr, TestCacheEntrySPtr const & entry)
        {
            auto timeout = this->GetRandomTimeout();

            Trace.WriteInfo(
                TraceComponent, 
                "BeginTryInvalidate(key={0} worker={1}): value={2} timeout={3}",
                key_,
                workerId_,
                entry->GetValue(),
                timeout);

            auto operation = cache_.BeginTryInvalidate(
                entry,
                timeout,
                [this](AsyncOperationSPtr const & operation){ this->OnTryInvalidateCompleted(operation, false); },
                thisSPtr);
            this->OnTryInvalidateCompleted(operation, true);
        }

        void OnTryInvalidateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto const & thisSPtr = operation->Parent;

            bool isFirstWaiter = false;
            TestCacheEntrySPtr entry;

            auto error = cache_.EndTryInvalidate(operation, isFirstWaiter, entry);

            Trace.WriteInfo(
                TraceComponent, 
                "EndTryInvalidate(key={0} worker={1}): error={2} isFirst={3}",
                key_,
                workerId_,
                error,
                isFirstWaiter);

            if (error.IsSuccess())
            {
                if (isFirstWaiter)
                {
                    this->ProcessFirstWaiterOnInvalidate(thisSPtr);
                }
                else
                {
                    this->ProcessEntryOnInvalidate(thisSPtr, entry);
                }
            }
            else if (error.IsError(ErrorCodeValue::Timeout))
            {
                this->StartTryGet(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, error);
            }
        }

        void ProcessFirstWaiterOnInvalidate(AsyncOperationSPtr const & thisSPtr)
        {
            auto iteration = iteration_.load();
            auto waiterCount = cache_.GetWaiterCount(key_);
            if (iteration < targetIterations_ && waiterCount < targetWaiterCount_)
            {
                Trace.WriteInfo(
                    TraceComponent, 
                    "Wait-OnInvalidate(key={0} worker={1}): waiters={2}/{3} iteration={4}",
                    key_,
                    workerId_,
                    waiterCount,
                    targetWaiterCount_,
                    iteration);

                Sleep(FirstWaiterDelayInMilliseconds);
                Threadpool::Post([this, thisSPtr]{ this->ProcessFirstWaiterOnInvalidate(thisSPtr); });
                return;
            }

            auto entry = make_shared<TestCacheEntry>(key_, wformatString("worker-{0}-OnInvalidate", workerId_), ++nextVersion_);

            bool success = cache_.TryPutOrGet(entry);

            Trace.WriteInfo(
                TraceComponent, 
                "TryPut-OnInvalidate(key={0} worker={1}): value={2} success={3} iteration={4}",
                key_,
                workerId_,
                entry->GetValue(),
                success,
                iteration_.load());

            if (success)
            {
                ++iteration_;

                this->StartIterationOrComplete(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            }
        }

        void ProcessEntryOnInvalidate(AsyncOperationSPtr const & thisSPtr, TestCacheEntrySPtr const & entry)
        {
            Trace.WriteInfo(
                TraceComponent, 
                "Invalidated(key={0} worker={1}): entry={2}",
                key_,
                workerId_,
                entry->GetValue());

            this->StartIterationOrComplete(thisSPtr);
        }

        void StartIterationOrComplete(AsyncOperationSPtr const & thisSPtr)
        {
            auto iteration = iteration_.load();

            Trace.WriteInfo(
                TraceComponent, 
                "FinishIteration(key={0} worker={1}): iteration={2}/{3}",
                key_,
                workerId_,
                iteration,
                targetIterations_);

            if (iteration < targetIterations_)
            {
                this->StartTryGet(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }

        TestCache & cache_;
        Random & rand_;
        atomic_long & nextVersion_;
        int workerId_;
        wstring key_;
        size_t targetWaiterCount_;
        int targetIterations_;
        atomic_long & iteration_;
    };

    // *** Test helper functions








    // Basic add/get/remove cycle on N cache entries.
    // Verification occurs after each operation.
    //
    BOOST_FIXTURE_TEST_SUITE(LruCacheTestSuite,LruCacheTest)

    BOOST_AUTO_TEST_CASE(SyncAddRemoveTest)
    {
        size_t cacheLimit = 0;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        for (auto ix=1; ix<=32; ix*=2)
        {
            SyncAddRemoveHelper(cache, ix);
        }
    }

    // Same as SyncAddRemoveTest but with eviction
    // enabled (non-zero cache limit) and 
    // additional consistency verification
    // between the cache and eviction list counts.
    //
    BOOST_AUTO_TEST_CASE(SyncAddRemoveTest2)
    {
        size_t cacheLimit = 32;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        for (auto ix=1; ix<=32; ix*=2)
        {
            SyncAddRemoveHelper(cache, ix);
        }
    }

    // Tests versioning of cache entries using ShouldUpdateUnderLock()
    //
    BOOST_AUTO_TEST_CASE(ConditionalAddTest)
    {
        size_t cacheLimit = 0;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        auto key1 = L"aa";
        auto key2 = L"bb";
        auto key3 = L"cc";

        SyncTryPut(cache, key1, 1, true);
        SyncTryPut(cache, key1, 2, true);
        SyncTryPut(cache, key1, 1, false);
        SyncTryPut(cache, key1, 2, false);
        
        SyncTryGet(cache, key1, true);

        SyncTryPut(cache, key2, 1, true);
        SyncTryPut(cache, key2, 2, true);
        SyncTryPut(cache, key2, 2, false);
        SyncTryPut(cache, key2, 1, false);

        SyncTryGet(cache, key1, true);
        SyncTryGet(cache, key2, true);

        SyncTryPut(cache, key3, 5, true);
        SyncTryPut(cache, key3, 4, false);
        SyncTryPut(cache, key3, 3, false);
        SyncTryPut(cache, key1, 3, true);
        SyncTryPut(cache, key2, 3, true);

        SyncTryGet(cache, key1, true);
        SyncTryGet(cache, key2, true);
        SyncTryGet(cache, key3, true);
    }

    // Tests cache limit maintenance.
    // New entries go to head of LRU list.
    // Removal happens at tail of LRU list.
    //
    BOOST_AUTO_TEST_CASE(CacheLimitTest)
    {
        size_t cacheLimit = 5;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        SyncTryPut(cache, L"aa", true);
        SyncTryPut(cache, L"bb", true);
        SyncTryPut(cache, L"cc", true);
        SyncTryPut(cache, L"dd", true);
        SyncTryPut(cache, L"ee", true);

        SyncTryGet(cache, L"ee", true);
        SyncTryGet(cache, L"dd", true);
        SyncTryGet(cache, L"cc", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);

        SyncTryPut(cache, L"ff", true);
        SyncTryGet(cache, L"ff", true);
        SyncTryGet(cache, L"dd", true);
        SyncTryGet(cache, L"cc", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);
        SyncTryGet(cache, L"ee", false);

        SyncTryPut(cache, L"gg", true);
        SyncTryGet(cache, L"dd", true);
        SyncTryGet(cache, L"cc", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);
        SyncTryGet(cache, L"gg", true);
        SyncTryGet(cache, L"ee", false);
        SyncTryGet(cache, L"ff", false);

        SyncTryPut(cache, L"hh", true);
        SyncTryGet(cache, L"cc", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);
        SyncTryGet(cache, L"gg", true);
        SyncTryGet(cache, L"hh", true);
        SyncTryGet(cache, L"ee", false);
        SyncTryGet(cache, L"ff", false);
        SyncTryGet(cache, L"dd", false);

        SyncTryPut(cache, L"ii", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);
        SyncTryGet(cache, L"gg", true);
        SyncTryGet(cache, L"hh", true);
        SyncTryGet(cache, L"ii", true);
        SyncTryGet(cache, L"ee", false);
        SyncTryGet(cache, L"ff", false);
        SyncTryGet(cache, L"dd", false);
        SyncTryGet(cache, L"cc", false);

        SyncTryPut(cache, L"jj", true);
        SyncTryPut(cache, L"kk", true);
        SyncTryPut(cache, L"ll", true);
        SyncTryGet(cache, L"hh", true);
        SyncTryGet(cache, L"ii", true);
        SyncTryGet(cache, L"jj", true);
        SyncTryGet(cache, L"kk", true);
        SyncTryGet(cache, L"ll", true);
        SyncTryGet(cache, L"ee", false);
        SyncTryGet(cache, L"ff", false);
        SyncTryGet(cache, L"dd", false);
        SyncTryGet(cache, L"cc", false);
        SyncTryGet(cache, L"bb", false);
        SyncTryGet(cache, L"aa", false);
        SyncTryGet(cache, L"gg", false);
    }
    
    // Tests cache limit maintenance.
    // Updates of existing elements should not
    // cause any eviction.
    //
    BOOST_AUTO_TEST_CASE(CacheLimitTest2)
    {
        size_t cacheLimit = 5;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        SyncTryPut(cache, L"aa", true);
        SyncTryPut(cache, L"bb", true);
        SyncTryPut(cache, L"cc", true);
        SyncTryPut(cache, L"dd", true);
        SyncTryPut(cache, L"ee", true);

        SyncTryGet(cache, L"ee", true);
        SyncTryGet(cache, L"dd", true);
        SyncTryGet(cache, L"cc", true);
        SyncTryGet(cache, L"bb", true);
        SyncTryGet(cache, L"aa", true);

        for (auto ix=0; ix<5; ++ix)
        {
            SyncTryPut(cache, L"aa", true);
            SyncTryGet(cache, L"ee", true);
            SyncTryGet(cache, L"dd", true);
            SyncTryGet(cache, L"cc", true);
            SyncTryGet(cache, L"bb", true);
            SyncTryGet(cache, L"aa", true);
        }

        for (auto ix=0; ix<5; ++ix)
        {
            SyncTryPut(cache, L"bb", true);
            SyncTryGet(cache, L"ee", true);
            SyncTryGet(cache, L"dd", true);
            SyncTryGet(cache, L"cc", true);
            SyncTryGet(cache, L"bb", true);
            SyncTryGet(cache, L"aa", true);
        }

        for (auto ix=0; ix<5; ++ix)
        {
            SyncTryPut(cache, L"cc", true);
            SyncTryGet(cache, L"ee", true);
            SyncTryGet(cache, L"dd", true);
            SyncTryGet(cache, L"cc", true);
            SyncTryGet(cache, L"bb", true);
            SyncTryGet(cache, L"aa", true);
        }

        for (auto ix=0; ix<5; ++ix)
        {
            SyncTryPut(cache, L"dd", true);
            SyncTryGet(cache, L"ee", true);
            SyncTryGet(cache, L"dd", true);
            SyncTryGet(cache, L"cc", true);
            SyncTryGet(cache, L"bb", true);
            SyncTryGet(cache, L"aa", true);
        }

        for (auto ix=0; ix<5; ++ix)
        {
            SyncTryPut(cache, L"ee", true);
            SyncTryGet(cache, L"ee", true);
            SyncTryGet(cache, L"dd", true);
            SyncTryGet(cache, L"cc", true);
            SyncTryGet(cache, L"bb", true);
            SyncTryGet(cache, L"aa", true);
        }
    }

    // Tests multi-threaded use of cache using asynchronous
    // TryGet/TryInvalidat APIs.
    //
    // Each worker runs a TryGet/(TryPut)/TryInvalidate loop
    // over N iterations in parallel with multiple other workers.
    //
    // Workers use random timeouts for each TryGet/TryInvalidate operation.
    //
    BOOST_AUTO_TEST_CASE(MultiThreadTest)
    {
        int workerCount = 18;
        int keyCount = 10;
        int targetWaiterCount = workerCount / 3;
        int targetIterations = 20;

        ManualResetEvent event(false);
        Random rand(static_cast<int>(DateTime::Now().Ticks));
        atomic_long nextVersion(0);

        vector<shared_ptr<atomic_long>> iterationCounters;
        for (auto ix=0; ix<keyCount; ++ix)
        {
            iterationCounters.push_back(make_shared<atomic_long>(0));
        }

        size_t cacheLimit = 0;
        size_t bucketCount = 128;

        TestCache cache(cacheLimit, bucketCount);

        atomic_long pendingThreadCount(workerCount * keyCount);
        atomic_long failureCount(0);

        for (auto ix=0; ix<keyCount; ++ix)
        {
            auto key = GetRandomKey(rand);

            for (auto jx=0; jx<workerCount; ++jx)
            {
                AsyncOperation::CreateAndStart<MultiThreadWorkerAsyncOperation>(
                    cache,
                    rand,
                    nextVersion,
                    jx,
                    key,
                    targetWaiterCount,
                    targetIterations,
                    *(iterationCounters[ix]),
                    [&](AsyncOperationSPtr const & operation) 
                    { 
                        auto error = MultiThreadWorkerAsyncOperation::End(operation);

                        if (!error.IsSuccess())
                        {
                            ++failureCount;
                        }

                        if (--pendingThreadCount == 0)
                        {
                            event.Set(); 
                        }
                    },
                    AsyncOperationSPtr());
            }
        }

        event.WaitOne();
        VERIFY_IS_TRUE_FMT(failureCount.load() == 0, "failures = {0}", failureCount.load());
    }

    BOOST_AUTO_TEST_CASE(ClearTest)
    {
        for (auto ix=1; ix<=20; ++ix)
        {
            ClearTestHelper(ix);
        }
    }

    // Tests that when the cache destructs, all cache
    // entries will also destruct by verifying the
    // ref-counts of cached entries
    //

    const bool IsPerfTestEnabled = true;

    BOOST_AUTO_TEST_CASE(PerfTest)
    {
        if (!IsPerfTestEnabled)
        {
            Trace.WriteWarning(
                TraceComponent, 
                "PerfTest disabled");

            return;
        }

        for (auto ix=1000; ix<=1024000; ix*=2)
        {
            PerfTestHelper(
                ix, 
                8000,
                false);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool LruCacheTest::TestcaseSetup()
    {
        Config cfg;
        return true;
    }
    wstring LruCacheTest::GetRandomKey(Random & rand)
    {
        int minDataLength = MinKeyLength;
        int maxDataLength = MaxKeyLength;

        int datalen = rand.Next(minDataLength, maxDataLength);
        wstring key;
        for (auto jx=0; jx<datalen; ++jx)
        {
            key.push_back((wchar_t)(rand.Next((int)'a', (int)'z')));
        }
        return key;
    }
    void LruCacheTest::SyncTryPut(TestCache & cache, wstring const & key, bool expectedSuccess)
    {
        SyncTryPut(cache, key, 0, expectedSuccess);
    }
    void LruCacheTest::SyncTryPut(TestCache & cache, wstring const & key, int version, bool expectedSuccess)
    {
        auto entry = make_shared<TestCacheEntry>(key, version);

        auto originalEntry = entry;

        bool success = cache.TryPutOrGet(entry);

        if (success)
        {
            VERIFY_IS_TRUE_FMT(entry == originalEntry, "TryPut({0}): {1}, {2}", key, entry.get(), originalEntry.get());
        }
        else
        {
            VERIFY_IS_TRUE_FMT(entry != originalEntry, "TryPut({0}): {1}, {2}", key, entry.get(), originalEntry.get());
        }

        VERIFY_IS_TRUE_FMT(
            key == entry->GetKey() && success == expectedSuccess, 
            "TryPut({0}, {1}): {2}, {3}", key, expectedSuccess, entry->GetKey(), success);

        if (expectedSuccess)
        {
            VERIFY_IS_TRUE_FMT(entry.use_count() == 3, "TryPut({0}) use_count({1}) != 3", key, entry.use_count());
        }
    }
    void LruCacheTest::SyncTryGet(TestCache & cache, wstring const & key, bool expectedSuccess)
    {
        TestCacheEntrySPtr entry;
        bool success = cache.TryGet(key, entry);

        if (success)
        {
            VERIFY_IS_TRUE_FMT(entry, "TryGet({0}): {1}", key, entry.get());
            VERIFY_IS_TRUE_FMT(entry->GetKey() == key, "TryGet({0}) = {1}", key, entry->GetKey());
        }
        else
        {
            VERIFY_IS_TRUE_FMT(!entry, "TryGet({0}): {1}", key, entry.get());
        }

        VERIFY_IS_TRUE_FMT(
            success == expectedSuccess,
            "TryGet({0}, {1}): {2}", key, expectedSuccess, success);

        if (expectedSuccess)
        {
            VERIFY_IS_TRUE_FMT(entry.use_count() == 2, "TryGet({0}) use_count({1}) != 2", key, entry.use_count());
        }
    }
    void LruCacheTest::SyncTryRemove(TestCache & cache, wstring const & key, bool expectedSuccess)
    {
        TestCacheEntrySPtr entry;
        bool success = cache.TryGet(key, entry);
        VERIFY_IS_TRUE_FMT(
            success == expectedSuccess,
            "TryGet->Remove({0}, {1}): {2}", key, expectedSuccess, success);

        success = cache.TryRemove(key);
        VERIFY_IS_TRUE_FMT(
            success == expectedSuccess,
            "TryRemove({0}, {1}): {2}", key, expectedSuccess, success);

        if (expectedSuccess)
        {
            VERIFY_IS_TRUE_FMT(entry.use_count() == 1, "TryRemove({0}) use_count({1}) != 1", key, entry.use_count());
        }
    }
    void LruCacheTest::SyncAddRemoveHelper(TestCache & cache, int keyCount)
    {
        Trace.WriteInfo(
            TraceComponent, 
            "Add/Remove {0} keys",
            keyCount);

        VERIFY_IS_TRUE_FMT(cache.Size == 0, "cache size = {0}", cache.Size);
        VERIFY_IS_TRUE_FMT(cache.CacheLimit == 0 || cache.EvictionListSize == cache.Size, "limit = {0} eviction = {1}", cache.CacheLimit, cache.EvictionListSize);

        Random rand(static_cast<int>(DateTime::Now().Ticks));

        vector<wstring> keys;
        for (auto ix=0; ix<keyCount; ++ix)
        {
            keys.push_back(GetRandomKey(rand));
        }

        for (auto ix=0; ix<keyCount; ++ix) 
        {
            SyncTryGet(cache, keys[ix], false);
        }

        Trace.WriteInfo(
            TraceComponent, 
            "Add {0} keys",
            keyCount);

        for (auto ix=0; ix<keyCount; ++ix) 
        {
            SyncTryPut(cache, keys[ix], true);

            VERIFY_IS_TRUE_FMT(cache.Size == ix + 1, "cache size = {0}", cache.Size);
            VERIFY_IS_TRUE_FMT(cache.CacheLimit == 0 || cache.EvictionListSize == cache.Size, "limit = {0} eviction = {1}", cache.CacheLimit, cache.EvictionListSize);

            SyncTryGet(cache, keys[ix], true);
        }

        for (auto ix=0; ix<keyCount; ++ix) 
        {
            SyncTryGet(cache, keys[ix], true);
        }

        Trace.WriteInfo(
            TraceComponent, 
            "Remove {0} keys",
            keyCount);

        for (auto ix=0; ix<keyCount; ++ix) 
        {
            SyncTryRemove(cache, keys[ix], true);

            VERIFY_IS_TRUE_FMT(cache.Size == keyCount - ix - 1, "cache size = {0}", cache.Size);
            VERIFY_IS_TRUE_FMT(cache.CacheLimit == 0 || cache.EvictionListSize == cache.Size, "limit = {0} eviction = {1}", cache.CacheLimit, cache.EvictionListSize);

            SyncTryGet(cache, keys[ix], false);
        }

        for (auto ix=0; ix<keyCount; ++ix) SyncTryGet(cache, keys[ix], false);
        for (auto ix=0; ix<keyCount; ++ix) SyncTryRemove(cache, keys[ix], false);

        VERIFY_IS_TRUE_FMT(cache.Size == 0, "cache size = {0}", cache.Size);
        VERIFY_IS_TRUE_FMT(cache.CacheLimit == 0 || cache.EvictionListSize == cache.Size, "limit = {0} eviction = {1}", cache.CacheLimit, cache.EvictionListSize);
    }
    void LruCacheTest::ClearTestHelper(int keyCount)
    {
        size_t cacheLimit = keyCount;
        size_t bucketCount = 128;

        auto cacheSPtr = make_shared<TestCache>(cacheLimit, bucketCount);

        for (auto ix=0; ix<keyCount; ++ix)
        {
            auto key = wformatString("key{0}", ix);
            SyncTryPut(*cacheSPtr, key, true);
            SyncTryGet(*cacheSPtr, key, true);
        }

        vector<weak_ptr<TestCacheEntry>> weakPtrs;

        for (auto ix=0; ix<keyCount; ++ix)
        {
            auto key = wformatString("key{0}", ix);

            TestCacheEntrySPtr entry;
            cacheSPtr->TryGet(key, entry);

            weak_ptr<TestCacheEntry> weakPtr(entry);

            entry.reset();

            VERIFY_IS_TRUE_FMT(weakPtr.use_count() == 1, "{0}: wptr use_count {1} != 1", key, weakPtr.use_count());
            VERIFY_IS_TRUE_FMT(weakPtr.lock().get() != nullptr, "{0}: wptr lock() failed", key);
            VERIFY_IS_TRUE_FMT(!weakPtr.expired(), "{0}: wptr expired {1}", key, weakPtr.expired());
            weakPtr.reset();

            cacheSPtr->TryGet(key, entry);
            weakPtrs.push_back(weak_ptr<TestCacheEntry>(entry));
        }

        cacheSPtr.reset();

        for (auto ix=0; ix<keyCount; ++ix)
        {
            auto key = wformatString("key{0}", ix);

            auto & weakPtr = weakPtrs[ix];

            VERIFY_IS_TRUE_FMT(weakPtr.use_count() == 0, "{0}: wptr use_count {1} != 0", key, weakPtr.use_count());
            VERIFY_IS_TRUE_FMT(weakPtr.lock().get() == nullptr, "{0}: wptr lock() succeeded", key);
            VERIFY_IS_TRUE_FMT(weakPtr.expired(), "{0}: wptr !expired", key, weakPtr.expired());
        }
    }

    void LruCacheTest::PerfTestHelper(
        int operationCount,
        size_t capacity,
        bool enableTrim)
    {
        //SetVerifyOutput localVerifySettings(VerifyOutputSettings::LogOnlyFailures);

        size_t bucketCount = 10240;

        LruCache<wstring, TestCacheEntry> cache(enableTrim ? capacity : 0, bucketCount);

        Random rand(static_cast<int>(DateTime::Now().Ticks));
        vector<TestCacheEntrySPtr> entries;
        for (auto ix=0; ix<operationCount; ++ix)
        {
            entries.push_back(make_shared<TestCacheEntry>(GetRandomKey(rand)));
        }

        Stopwatch stopwatch;

        // Write
        
        stopwatch.Restart();
        for (auto it=entries.begin(); it!=entries.end(); ++it)
        {
            auto entry = *it;
            bool success = cache.TryPutOrGet(entry);
            VERIFY_IS_TRUE(success);
        }
        stopwatch.Stop();

        Trace.WriteInfo(
            TraceComponent, 
            "LruCache-put: operations={0} elapsed={1} throughput={2} ops/s", 
            cache.Size, 
            stopwatch.Elapsed,
            (double)cache.Size / stopwatch.ElapsedMilliseconds * 1000);

        // Read

        stopwatch.Restart();
        for (auto it=entries.begin(); it!=entries.end(); ++it)
        {
            shared_ptr<TestCacheEntry> result;
            bool success = cache.TryGet((*it)->GetKey(), result);
            VERIFY_IS_TRUE(success);
        }
        stopwatch.Stop();

        Trace.WriteInfo(
            TraceComponent, 
            "LruCache-get: operations={0} elapsed={1} throughput={2} ops/s", 
            cache.Size, 
            stopwatch.Elapsed,
            (double)cache.Size / stopwatch.ElapsedMilliseconds * 1000);

        Trace.WriteInfo(
            TraceComponent, 
            "CacheSize: {0}",
            cache.Size);
    }
}
