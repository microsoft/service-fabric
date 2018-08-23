// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsPC'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Common;

    class CheckpointStressTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        CheckpointStressTest()
        {
            Setup(1);

            maxKey_ = 0;
        }

        ~CheckpointStressTest()
        {
            Cleanup();
        }

        Random GetRandom()
        {
            auto seed = Stopwatch::Now().Ticks;
            Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        KString::SPtr CreateString(__in ULONG32 index)
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 9) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return key;
        }

        KString::SPtr CreateString(__in LPCWSTR value)
        {
            KString::SPtr result;
            auto status = KString::Create(result, GetAllocator(), value);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return result;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in byte fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, this->GetAllocator());

            byte* buffer = static_cast<byte *>(resultSPtr->GetBuffer());
            memset(buffer, fillValue, sizeInBytes);

            return resultSPtr;
        }

        Awaitable<void> AddItemsAsync(
            __in ULONG32 startCount, 
            __in ULONG32 endCount)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());

            for (ULONG32 i = startCount; i < endCount; i++)
            {
                {
                    auto key = CreateString(i);
                    auto value = CreateBuffer(128, static_cast<byte>(i & 0xff));

                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                InterlockedIncrement64(&maxKey_);
            }
        }

        Awaitable<void> AddItemsAsync(
            __in ULONG32 startCount, 
            __in ULONG32 endCount,
            __in ULONG32 numTasks)
        {
            ULONG32 countPerTask = (endCount - startCount + 1) / numTasks;
            ULONG32 startKey = startCount;

            KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>;
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks->Append(AddItemsAsync(startKey, startKey+countPerTask));
                startKey += countPerTask;
            }
            
            co_await StoreUtilities::WhenAll(*tasks, GetAllocator());
        }

        Awaitable<void> AddItemsAsync(__in CancellationToken const & token, __in ULONG32 throttle=0)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            auto value = CreateBuffer(64 * 1024, 10);

            while (!token.IsCancellationRequested)
            {
                LONG64 index = maxKey_;
                auto key = CreateString(static_cast<ULONG32>(index));

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                InterlockedIncrement64(&maxKey_);
                
                if (throttle > 0)
                {
                    co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, throttle, nullptr);
                }
            }
        }

        Awaitable<void> AddOrUpdateItemsAsync(
            __in ULONG32 startCount, 
            __in ULONG32 endCount,
            __in ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr> & expectedState,
            __in Random & random)
        {

            ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr>::SPtr expectedStateSPtr = &expectedState;

            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool()); // Immediately background

            for (ULONG32 i = startCount; i <= endCount; i++)
            {
                    auto key = CreateString(i);
                    auto value = CreateBuffer(128, static_cast<byte>(random.Next(0, 0xff)));

                    {
                        auto txn = CreateWriteTransaction();
                        bool updated = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                        if (!updated)
                        {
                            co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                        }

                        expectedStateSPtr->AddOrUpdate(key, value);

                        co_await txn->CommitAsync();
                        txn = nullptr;
                    }
            }

            co_return;
        }

        Awaitable<void> AddItemsAsync(
            __in AwaitableCompletionSource<void> & startTcs, 
            __in ULONG32 startCount, 
            __in ULONG32 endCount)
        {
            co_await startTcs.GetAwaitable();
            co_await AddItemsAsync(startCount, endCount);
        }

        Awaitable<void> ReadItemsAsync(
            __in AwaitableCompletionSource<void> & startTcs, 
            __in ULONG32 count,
            __in StoreTransactionReadIsolationLevel::Enum isolationLevel, 
            __in CancellationToken cancellationToken)
        {
            Random random = GetRandom();
            co_await startTcs.GetAwaitable();

            for (ULONG32 i = 1; i < count; i++)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                ULONG32 keySeed = static_cast<ULONG32>(random.Next(0, static_cast<int>(maxKey_)));
                KString::SPtr key = CreateString(keySeed);

                {
                    auto txn = CreateWriteTransaction();
                    txn->StoreTransactionSPtr->ReadIsolationLevel = isolationLevel;

                    ULONG32 readChoice = random.Next(2);

                    if (readChoice == 0) // ContainsKeyAsync
                    {
                        bool exists = co_await Store->ContainsKeyAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                        CODING_ERROR_ASSERT(exists);
                    }
                    else if (readChoice == 1) // ConditionalGetAsync
                    {
                        //KBuffer::SPtr expectedValue = CreateBuffer(128, keySeed & 0xff);
                        KeyValuePair<LONG64, KBuffer::SPtr> result(-1, nullptr);

                        bool exists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, result, CancellationToken::None);
                        CODING_ERROR_ASSERT(exists);
                        CODING_ERROR_ASSERT(result.Value != nullptr);

                        //KBuffer::SPtr actualValue = result.Value;
                        //CODING_ERROR_ASSERT(KBufferComparer::Equals(actualValue, expectedValue));
                    }
                    else
                    {
                        ASSERT_IFNOT(false, "Invalid choice={0}", readChoice);
                    }

                    // Do not commit
                    co_await txn->AbortAsync();
                }
            }
        }

        Awaitable<void> GetItemsAsync(
            __in AwaitableCompletionSource<void> & startTcs,
            __in ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr> & expectedState,
            __in Random & random,
            __in CancellationToken const & token)
        {
            co_await startTcs.GetAwaitable();

            while (!token.IsCancellationRequested)
            {
                ULONG32 keySeed = static_cast<ULONG32>(random.Next(0, static_cast<ULONG32>(maxKey_)));
                auto key = CreateString(keySeed);

                {
                    auto txn = CreateWriteTransaction();

                    KeyValuePair<LONG64, KBuffer::SPtr> result(-1, nullptr);
                    bool exists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, result, CancellationToken::None);
                    if (expectedState.ContainsKey(key))
                    {
                        CODING_ERROR_ASSERT(exists);

                        KBuffer::SPtr actualValue = result.Value;

                        KBuffer::SPtr expectedValue = nullptr;
                        expectedState.TryGetValue(key, expectedValue);
                        CODING_ERROR_ASSERT(KBufferComparer::Equals(expectedValue, actualValue));
                    }
                    else
                    {
                        CODING_ERROR_ASSERT(!exists);
                    }

                    co_await txn->AbortAsync();
                }
            }
        }

        Awaitable<void> UpdateItemsAsync(
            __in AwaitableCompletionSource<void> & startTcs,
            __in ULONG32 count)
        {
            Random random = GetRandom();
            co_await startTcs.GetAwaitable();

            for (ULONG32 i = 0; i < count; i++)
            {
                ULONG32 keySeed = static_cast<ULONG32>(random.Next(0, static_cast<ULONG32>(maxKey_)));
                auto key = CreateString(keySeed);
                auto value = CreateBuffer(128, keySeed & 0xff);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }
        }

        Awaitable<void> RepeatedlyCheckpointAsync(
            __in AwaitableCompletionSource<void> & startTcs,
            __in CancellationToken & cancellationToken)
        {
            co_await startTcs.GetAwaitable();

            while (!cancellationToken.IsCancellationRequested)
            {
                co_await CheckpointAsync();
                co_await CheckpointAsync();
                co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 100, nullptr);
            }
        }

        Awaitable<void> ReadOrUpdateUsingOracle(
            __in Random & random,
            __in ULONG32 keySeed,
            __in ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr> & oracle)
        {
            KString::SPtr key = CreateString(keySeed);

            KBuffer::SPtr existingValue = nullptr;
            bool keyExistsInOracle = oracle.TryGetValue(key, existingValue);

            // Randomly read, AddOrUpdate or remove
            ULONG32 choice = random.Next(0, 3);
            bool shouldCommit = random.Next(0, 2) > 0;

            KBuffer::SPtr updateValue = CreateBuffer(128, (keySeed + 1) & 0xff);

            {
                auto txn = CreateWriteTransaction();
                switch (choice)
                {
                case 0: // Read
                {
                    Data::KeyValuePair<LONG64, KBuffer::SPtr> result(-1, nullptr);
                    bool keyExistsInStore = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, result, CancellationToken::None);
                    if (keyExistsInOracle)
                    {
                        KBuffer::SPtr actualValue = result.Value;
                        CODING_ERROR_ASSERT(keyExistsInStore);
                        CODING_ERROR_ASSERT(KBufferComparer::Equals(existingValue, actualValue));
                    }
                    else
                    {
                        CODING_ERROR_ASSERT(keyExistsInStore == false);
                    }

                    break;
                }
                case 1: // AddOrUpdate
                {
                    bool updated = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                    if (!updated)
                    {
                        CODING_ERROR_ASSERT(!keyExistsInOracle); // Check that the key actually shouldn't exist
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                    }

                    if (shouldCommit)
                    {
                        oracle.AddOrUpdate(key, updateValue);
                    }

                    break;
                }
                case 2: // Remove
                    bool removed = co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                    if (!removed)
                    {
                        CODING_ERROR_ASSERT(!keyExistsInOracle); // Check that the key actually shouldn't exist
                        shouldCommit = false;
                    }

                    if (removed && shouldCommit)
                    {
                        oracle.Remove(key);
                    }

                    break;
                }

                if (choice != 0 && shouldCommit)
                {
                    co_await txn->CommitAsync();
                }
                else
                {
                    co_await txn->AbortAsync();
                }
            }
        }

        void TestConsolidation(
            __in ULONG32 count,
            __in bool enableBackgroundSweep,
            __in bool allowReaders)
        {
            cout << "Consolidation test starting. Count: " << count << " EnableBackgroundSweep: " << enableBackgroundSweep << " AllowReaders: " << allowReaders << endl;

            Store->EnableBackgroundConsolidation = enableBackgroundSweep;
            Store->EnableSweep = true;
            Store->StartBackgroundSweep();

            Common::Random random = GetRandom();

            ULONG32 totalCount = 1'000'000;
            ULONG32 countPerTask = 5'000;

            maxKey_ = totalCount;

            for (ULONG32 i = 0; i < count; i++)
            {
                ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr>::SPtr expectedState = nullptr;
                NTSTATUS status = ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr>::Create(
                    100,
                    K_DefaultHashFunction,
                    *Store->KeyComparerSPtr,
                    GetAllocator(),
                    expectedState);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                KSharedArray<Awaitable<void>>::SPtr addOrUpdateTasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

                for (ULONG32 j = 1; j <= (totalCount / countPerTask); j++)
                {
                    ULONG32 start = countPerTask * (j - 1);
                    ULONG32 end = countPerTask * j;
                    addOrUpdateTasks->Append(AddOrUpdateItemsAsync(start, end, *expectedState, random));
                }

                AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
                AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

                CancellationTokenSource::SPtr source = nullptr;
                CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
                CancellationToken token = source->Token;

                auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);

                if (allowReaders)
                {
                    auto readTask = GetItemsAsync(*startTcs, *expectedState, random, token);
                    startTcs->Set();
                    
                    SyncAwait(StoreUtilities::WhenAll(*addOrUpdateTasks, GetAllocator()));
                    source->Cancel();
                    SyncAwait(checkpointTask);
                    SyncAwait(readTask);
                }
                else
                {
                    startTcs->Set();
                    SyncAwait(StoreUtilities::WhenAll(*addOrUpdateTasks, GetAllocator()));
                    source->Cancel();
                    SyncAwait(checkpointTask);
                }

                addOrUpdateTasks = nullptr;

                RemoveStateAndReopenStore();
                cout << "Finished iteration " << i + 1 << endl;
            }

            cout << "Consolidation test complete" << endl;
        }

        LONG64 GetKeysAndMetadataSize(
            __in LONG64 estimatedKeySize,
            __in ULONG32 numDifferential,
            __in ULONG32 numNonDifferential)
        {
            return numDifferential * (2 * Constants::ValueMetadataSizeBytes + estimatedKeySize) + numNonDifferential * (estimatedKeySize + Constants::ValueMetadataSizeBytes);
        }

    private:
        LONG64 maxKey_;

        CommonConfig config; // load the config object as it's needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointStressTestSuite, CheckpointStressTest)

    BOOST_AUTO_TEST_CASE(Add_Read_Checkpoint_ConcurrentOverlapping)
    {
        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        // Add some keys to start for the read task
        SyncAwait(AddItemsAsync(0, 1));

        ULONG32 maxCount = 1'000'000;

        auto addTask = AddItemsAsync(*startTcs, 1, maxCount);
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::ReadRepeatable, token);

        startTcs->Set();
        
        cout << "Add_Read_Checkpoint_ConcurrentOverlapping - started" << endl;

        // Wait for add to finish and then stop the read and checkpoint tasks
        SyncAwait(addTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        SyncAwait(readTask);

        cout << "Add_Read_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }

    BOOST_AUTO_TEST_CASE(Add_Read_Checkpoint_Sweep_ConcurrentOverlapping)
    {
        Store->EnableSweep = true;
        Store->SweepManagerSPtr->TimeoutInMS = 30'000;
        Store->StartBackgroundSweep();

        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        auto memoryTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(10), true, token);

        // Add some keys to start for the read task
        SyncAwait(AddItemsAsync(0, 1));

        ULONG32 maxCount = 1'000'000;

        auto addTask = AddItemsAsync(*startTcs, 1, maxCount);
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::ReadRepeatable, token);

        startTcs->Set();
        
        cout << "Add_Read_Checkpoint_Sweep_ConcurrentOverlapping - started" << endl;

        // Wait for add to finish and then stop the read and checkpoint tasks
        SyncAwait(addTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        SyncAwait(readTask);
        SyncAwait(memoryTask);

        cout << "Add_Read_Checkpoint_Sweep_ConcurrentOverlapping - completed" << endl;
    }

/*
    BOOST_AUTO_TEST_CASE(Update_Read_Checkpoint_ConcurrentOverlapping)
    {
        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        ULONG32 maxCount = 1'000'000;

        SyncAwait(AddItemsAsync(0, maxCount, 12));
        
        auto updateTask = UpdateItemsAsync(*startTcs, maxCount);
        auto checkpointTask = CheckpointAndSweepAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::ReadRepeatable, token);

        startTcs->Set();
        
        cout << "Update_Read_Checkpoint_ConcurrentOverlapping - started" << endl;

        // Wait for update to finish and then stop the read and checkpoint tasks
        SyncAwait(updateTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        SyncAwait(readTask);
        
        CloseAndReOpenStore();

        cout << "Update_Read_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }
*/

    BOOST_AUTO_TEST_CASE(Update_ReadWithSnapshot_Checkpoint_Sweep_ConcurrentOverlapping)
    {
        Store->EnableSweep = true;
        Store->SweepManagerSPtr->TimeoutInMS = 30'000;
        Store->StartBackgroundSweep();

        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        auto memoryTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(10), true, token);

        ULONG32 maxCount = 1'000'000;

        SyncAwait(AddItemsAsync(0, maxCount-1, 12));
        
        auto updateTask = UpdateItemsAsync(*startTcs, maxCount);
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::Snapshot, token);

        startTcs->Set();
        
        cout << "Update_ReadWithSnapshot_Checkpoint_Sweep_ConcurrentOverlapping - started" << endl;

        // Wait for update to finish and then stop the read and checkpoint tasks
        SyncAwait(updateTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        SyncAwait(readTask);
        
        CloseAndReOpenStore();

        cout << "Update_ReadWithSnapshot_Checkpoint_Sweep_ConcurrentOverlapping - completed" << endl;

        SyncAwait(memoryTask);
    }

    BOOST_AUTO_TEST_CASE(Update_ReadWithSnapshot_Checkpoint_ConcurrentOverlapping)
    {
        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        ULONG32 maxCount = 1'000'000;

        SyncAwait(AddItemsAsync(0, maxCount, 12));
        
        auto updateTask = UpdateItemsAsync(*startTcs, maxCount);
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::Snapshot, token);

        startTcs->Set();
        
        cout << "Update_ReadWithSnapshot_Checkpoint_ConcurrentOverlapping - started" << endl;

        // Wait for update to finish and then stop the read and checkpoint tasks
        SyncAwait(updateTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        SyncAwait(readTask);
        
        CloseAndReOpenStore();

        cout << "Update_ReadWithSnapshot_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }

    BOOST_AUTO_TEST_CASE(Read_Checkpoint_ConcurrentOverlapping)
    {
        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        ULONG32 maxCount = 1'000'000;

        SyncAwait(AddItemsAsync(0, maxCount, 12));
        
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, maxCount, StoreTransactionReadIsolationLevel::ReadRepeatable, token);
        startTcs->Set();
        
        cout << "Read_Checkpoint_ConcurrentOverlapping - started" << endl;

        // Wait for update to finish and then stop the read and checkpoint tasks
        SyncAwait(readTask);
        source->Cancel();
        SyncAwait(checkpointTask);
        
        cout << "Read_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }

    BOOST_AUTO_TEST_CASE(Read_Checkpoint_Sweep_ConcurrentOverlapping)
    {
        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        auto memoryTask = TraceMemoryUsagePeriodically(Common::TimeSpan::FromSeconds(15), true, token);

        ULONG32 maxCount = 1'000'000;

        SyncAwait(AddItemsAsync(0, maxCount, 12));
        
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, maxCount, StoreTransactionReadIsolationLevel::ReadRepeatable, token);
        startTcs->Set();
        
        cout << "Read_Checkpoint_Sweep_ConcurrentOverlapping - started" << endl;

        // Wait for update to finish and then stop the read and checkpoint tasks
        SyncAwait(readTask);
        source->Cancel();
        SyncAwait(checkpointTask);

        SyncAwait(memoryTask);
        
        cout << "Read_Checkpoint_Sweep_ConcurrentOverlapping - completed" << endl;
    }

    BOOST_AUTO_TEST_CASE(Add_Checkpoint_Sweep_VerifySize)
    {
        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        ULONG32 noOfItemsPerIteration = 1000;
        ULONG32 valueSize = 128;

        for (ULONG32 i = 0; i < 4; i++)
        {
            ULONG32 start = i * noOfItemsPerIteration;
            ULONG32 end = start + noOfItemsPerIteration;
            SyncAwait(AddItemsAsync(start, end));
            SyncAwait(CheckpointAsync());

            // Check size
            LONG64 expectedKeysAndMetadatasSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, (i + 1) * noOfItemsPerIteration);
            LONG64 expectedValuesSize = (i + 1) * noOfItemsPerIteration * (valueSize + sizeof(ULONG32));
            LONG64 expectedSize = expectedKeysAndMetadatasSize + expectedValuesSize;

            LONG64 consolidatedComponentSize = Store->ConsolidationManagerSPtr->GetMemorySize(Store->GetEstimatedKeySize());
            CODING_ERROR_ASSERT(consolidatedComponentSize == expectedSize);
        }

        // Throw things out from the cache.
        Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);
        Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);

        LONG64 expectedKeysAndMetadatasSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 4000);
        LONG64 consolidatedComponentSize = Store->ConsolidationManagerSPtr->GetMemorySize(Store->GetEstimatedKeySize());
        CODING_ERROR_ASSERT(consolidatedComponentSize == expectedKeysAndMetadatasSize);
    }

    BOOST_AUTO_TEST_CASE(SweepStressTest_ShouldSucceed)
    {
        Store->EnableSweep = false; // Will manually call sweep
        Store->EnableBackgroundConsolidation = false;
        Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::All;

        ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr>::SPtr expectedState = nullptr;
        NTSTATUS status = ConcurrentDictionary2<KString::SPtr, KBuffer::SPtr>::Create(
            100,
            K_DefaultHashFunction,
            *Store->KeyComparerSPtr,
            GetAllocator(),
            expectedState);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        SyncAwait(AddItemsAsync(1, 502));

        for (ULONG32 i = 1; i <= 500; i++)
        {
            bool added = expectedState->TryAdd(CreateString(i), CreateBuffer(128, i & 0xff));
            CODING_ERROR_ASSERT(added);
        }

        cout << "SweepStressTest - started" << endl;

        Random random = GetRandom();

        ULONG32 maxCount = 10'000;

        for (ULONG32 i = 1; i <= maxCount; i++)
        {
            Checkpoint();
            Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);

            // Update items 1 to 100
            for (ULONG32 j = 1; j <= 100; j++)
            {
                SyncAwait(ReadOrUpdateUsingOracle(random, j, *expectedState));
            }

            if (i % 2 == 0)
            {
                // Update items 101 to 200
                for (int j = 101; j <= 200; j++)
                {
                    SyncAwait(ReadOrUpdateUsingOracle(random, j, *expectedState));
                }
            }

            if (i % 3 == 0)
            {
                // Update items 201 to 300
                for (int j = 201; j <= 300; j++)
                {
                    SyncAwait(ReadOrUpdateUsingOracle(random, j, *expectedState));
                }
            }

            if (i % 4 == 0)
            {
                // Update items 301 to 400
                for (int j = 301; j <= 400; j++)
                {
                    SyncAwait(ReadOrUpdateUsingOracle(random, j, *expectedState));
                }
            }

            if (i % 5 == 0)
            {
                // Update items 401 to 500
                for (int j = 401; j <= 500; j++)
                {
                    SyncAwait(ReadOrUpdateUsingOracle(random, j, *expectedState));
                }
            }
        }

        cout << "SweepStressTest - completed" << endl;
    }

    // TODO: Following tests fail because of negative size assert
    BOOST_AUTO_TEST_CASE(ConsolidationStress_RepeatableRead)
    {
        TestConsolidation(3, false, true);
    }
    
    BOOST_AUTO_TEST_CASE(ConsolidationStress_NoReaders)
    {
        TestConsolidation(3, false, false);
    }

    BOOST_AUTO_TEST_CASE(ConsolidationStress_InBackground_RepeatableRead)
    {
        TestConsolidation(3, true, true);
    }

    BOOST_AUTO_TEST_CASE(ConsolidationStress_InBackground_NoReaders)
    {
        TestConsolidation(3, true, false);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_4GB_ShouldSucceed)
    {
        cout << "Checkpoint 4GB start" << endl;

        try
        {
            // Setup - Add 4K items of size 1MB
            for (ULONG32 i = 0; i < 4 * 1024; i++)
            {
                {
                    auto txn = CreateWriteTransaction();

                    KString::SPtr key = CreateString(i);
                    KBuffer::SPtr value = CreateBuffer(1024 * 1024, static_cast<byte>(i & 0xff));

                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));

                    SyncAwait(txn->CommitAsync());
                }
            }

            Stopwatch stopwatch;
            stopwatch.Start();
            Checkpoint();
            cout << "Checkpoint: " << stopwatch.ElapsedMilliseconds << " ms" << endl;

            LONG64 recoveryMS = CloseAndReOpenStore();
            cout << "Recover: " << recoveryMS << " ms" << endl;
            CODING_ERROR_ASSERT(Store->Count == 4096);

            cout << "Verify keys of 4GB checkpoint - start" << endl;

            for (ULONG32 i = 0; i < 4 * 1024; i++)
            {
                KString::SPtr readKey = CreateString(i);
                KBuffer::SPtr expectedValue = CreateBuffer(1024 * 1024, static_cast<byte>(i & 0xff));
                SyncAwait(VerifyKeyExistsAsync(*Store, readKey, nullptr, expectedValue, KBufferComparer::Equals));
            }

            cout << "Verify keys of 4GB checkpoint - completed" << endl;
        }
        catch (Exception const & e)
        {
            cout << e.GetStatus() << endl;
            CODING_ERROR_ASSERT(false);
        }

        cout << "Checkpoint 4GB end";
    }

    BOOST_AUTO_TEST_CASE(Merge_Two4GBFiles_ShouldSucceed)
    {
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::FileCount;
        Store->MergeHelperSPtr->FileCountMergeConfigurationSPtr->FileCountMergeThreshold = 2;
        Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;

        cout << "Checkpoint & Merge 4GB+4GB start" << endl;

        try
        {
            // Setup - Add 8K items of size 1MB, checkpointing twice

            byte checkpointCount = 0;

            // Second checkpoint will trigger a merge
            for (ULONG32 i = 0; i < 4 * 1024; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    KString::SPtr key = CreateString(i);
                    KBuffer::SPtr value = CreateBuffer(1024 * 1024, static_cast<byte>(i & 0xff));
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }

            }

            Stopwatch stopwatch;
            stopwatch.Start();
            Checkpoint();
            cout << "Checkpoint: " << stopwatch.ElapsedMilliseconds << " ms" << endl;
            checkpointCount++;

            for (ULONG32 i = 4 * 1024; i < 8 * 1024; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    KString::SPtr key = CreateString(i);
                    KBuffer::SPtr value = CreateBuffer(1024 * 1024, static_cast<byte>(i & 0xff));
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }

            stopwatch.Restart();
            Checkpoint();
            cout << "Checkpoint & Merge: " << stopwatch.ElapsedMilliseconds << " ms" << endl;
            checkpointCount++;

            // A precaution for catching changes to the test
            CODING_ERROR_ASSERT(checkpointCount == 2);

            LONG64 recoveryMS = CloseAndReOpenStore();
            cout << "Recover: " << recoveryMS << " ms" << endl;
            CODING_ERROR_ASSERT(Store->Count == 8192);

            cout << "Verify keys of 8GB checkpoint - start" << endl;

            for (ULONG32 i = 0; i < 8 * 1024; i++)
            {
                KString::SPtr readKey = CreateString(i);
                KBuffer::SPtr expectedValue = CreateBuffer(1024 * 1024, static_cast<byte>(i & 0xff));
                SyncAwait(VerifyKeyExistsAsync(*Store, readKey, nullptr, expectedValue, KBufferComparer::Equals));
            }

            cout << "Verify keys of 8GB checkpoint - completed" << endl;
        }
        catch (Exception const & e)
        {
            cout << e.GetStatus() << endl;
            CODING_ERROR_ASSERT(false);
        }

        cout << "Checkpoint & Merge 4GB+4GB start" << endl;
    }
    
    /* Disabling since these are very long running tests (12 hours each)

    // Following test runs into negative size assert
    BOOST_AUTO_TEST_CASE(Add_Checkpoint_Read_Long_Time)
    {
        Store->EnableSweep = true;

        // Add 100 keys to start
        SyncAwait(AddItemsAsync(0, 100));

        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        auto addTask = AddItemsAsync(token, 300); // Add ~3 items/sec
        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::ReadRepeatable, token);

        startTcs->Set();
        
        cout << "Add_Read_Checkpoint_ConcurrentOverlapping - started" << endl;

        for (ULONG32 hours = 0; hours < 12; hours++)
        {
            cout << hours << ": ";
            for (ULONG32 minutes = 0; minutes < 60; minutes++)
            {
                if (minutes % 10 == 0)
                {
                    if (minutes == 0) cout << "00 ";
                    else cout << minutes << " ";
                }

                SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 60 * 1000, nullptr));
            }

            cout << endl;
        }
        // Wait for add to finish and then stop the read and checkpoint tasks
        source->Cancel();

        SyncAwait(addTask);
        SyncAwait(checkpointTask);
        SyncAwait(readTask);

        cout << "Add_Read_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_Read_Long_Time)
    {
        Store->EnableSweep = true;

        // Add 100 keys to start
        SyncAwait(AddItemsAsync(0, 100));

        AwaitableCompletionSource<void>::SPtr startTcs = nullptr;
        AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, startTcs);

        CancellationTokenSource::SPtr source = nullptr;
        CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, source);
        CancellationToken token = source->Token;

        auto checkpointTask = RepeatedlyCheckpointAsync(*startTcs, token);
        auto readTask = ReadItemsAsync(*startTcs, MAXULONG32, StoreTransactionReadIsolationLevel::ReadRepeatable, token);

        startTcs->Set();
        
        cout << "Read_Checkpoint_ConcurrentOverlapping - started" << endl;

        for (ULONG32 hours = 0; hours < 12; hours++)
        {
            cout << hours << ": ";
            for (ULONG32 minutes = 0; minutes < 60; minutes++)
            {
                if (minutes % 10 == 0)
                {
                    if (minutes == 0) cout << "00 ";
                    else cout << minutes << " ";
                }

                SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 60 * 1000, nullptr));
            }

            cout << endl;
        }
        // Wait for add to finish and then stop the read and checkpoint tasks
        source->Cancel();

        SyncAwait(checkpointTask);
        SyncAwait(readTask);

        cout << "Read_Checkpoint_ConcurrentOverlapping - completed" << endl;
    }
    */

    BOOST_AUTO_TEST_SUITE_END();
}
