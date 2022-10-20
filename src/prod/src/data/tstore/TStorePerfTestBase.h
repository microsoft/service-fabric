// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STOREPERFTESTBASE_TAG 'pbTS'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::TStore;
    using namespace TxnReplicator;

#define TRACE_TEST() \
        auto & test = boost::unit_test::framework::current_test_case(); \
        auto & suite = boost::unit_test::framework::get<boost::unit_test::test_suite>(test.p_parent_id); \
        Tracer t(suite.p_name.get() + "/" + test.p_name.get());

    class Tracer
    {
    public:
        Tracer(__in const string testName) : testName_(testName)
        {
            Trace.WriteInfo("Test", "*****Starting test {0}*****", testName_);
            stopwatch_.Start();
        }
        
        ~Tracer()
        {
            stopwatch_.Stop();
            Trace.WriteInfo("Test", "*****Finishing test {0}. Time taken={1} ms*****", testName_, stopwatch_.ElapsedMilliseconds);
            Trace.WriteInfo("Test", "################################################");
        }
    private:
        string testName_;
        Common::Stopwatch stopwatch_;
    };

    template<typename TKey, typename TValue, typename TKeyComparer, typename TKeySerializer, typename TValueSerializer>
    class TStorePerfTestBase : public TStoreTestBase<TKey, TValue, TKeyComparer, TKeySerializer, TValueSerializer>
    {
    public:
        const Common::TimeSpan DefaultTimeout = Common::TimeSpan::FromSeconds(4);
        typedef KDelegate<bool(TValue & one, TValue & two)> ValueEqualityFunctionType;

        void Setup(
            ULONG replicaCount = 1,
            KDelegate<ULONG(const TKey & Key)> hashFunc = DefaultHash,
            KString::CSPtr startDirectory = nullptr,
            bool hasPersistedState = true) override
        {
            TStoreTestBase<TKey, TValue, TKeyComparer, TKeySerializer, TValueSerializer>::Setup(replicaCount, hashFunc, startDirectory, hasPersistedState);
            this->Replicator->ShouldSynchronizePrepareAndApply = false;
        }

        ktl::Awaitable<LONG64> AddKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 offset,
            __in ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = &items;

            auto txn = this->CreateWriteTransaction();

            Common::Stopwatch stopwatch;

            for (ULONG32 k = offset; k < offset + count; k++)
            {
                auto item = (*itemsSPtr)[k];
                auto key = item.Key;
                auto value = item.Value;
                stopwatch.Start();
                co_await this->Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                stopwatch.Stop();
                stopwatch.Reset();
            }

            stopwatch.Start();
            co_await txn->CommitAsync();
            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> AddKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 numTasks = 1)
        {
            CODING_ERROR_ASSERT(items.Count() % numTasks == 0);
            KSharedArray<ktl::Awaitable<LONG64>>::SPtr addTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<LONG64>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = items.Count() / numTasks;


            Common::Stopwatch stopwatch;
            stopwatch.Start();
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                addTasksSPtr->Append(AddKeysAsync(items, offset, countPerTask));
                offset += countPerTask;
            }

            co_await StoreUtilities::WhenAll(*addTasksSPtr, this->GetAllocator());
            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> VerifyKeyValueAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 offset,
            __in ULONG32 count,
            __in ValueEqualityFunctionType equalityFunction = DefaultEqualityFunction)
        {
            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = &items;
            auto txn = this->CreateWriteTransaction();

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 k = offset; k < offset + count; k++)
            {
                auto item = (*itemsSPtr)[k];
                auto key = item.Key;
                auto value = item.Value;
                TValue defaultValue = TValue();
                co_await this->VerifyKeyExistsAsync(*(this->Store), *txn->StoreTransactionSPtr, key, defaultValue, value, equalityFunction);
            }

            co_await txn->CommitAsync();

            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> VerifyKeyValueAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 numTasks = 1,
            __in ValueEqualityFunctionType equalityFunction = DefaultEqualityFunction)
        {
            CODING_ERROR_ASSERT(items.Count() % numTasks == 0);
            KSharedArray<ktl::Awaitable<LONG64>>::SPtr verifyTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<LONG64>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = items.Count() / numTasks;

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                verifyTasksSPtr->Append(VerifyKeyValueAsync(items, offset, countPerTask, equalityFunction));
                offset += countPerTask;
            }

            co_await StoreUtilities::WhenAll(*verifyTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> GetKeyAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 offset,
            __in ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = &items;
            TValue defaultValue = TValue();

            auto txn = this->CreateWriteTransaction();

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 k = offset; k < offset + count; k++)
            {
                auto item = (*itemsSPtr)[k];
                auto key = item.Key;

                KeyValuePair<LONG64, TValue> output(-1, defaultValue);
                co_await this->Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, output, ktl::CancellationToken::None);
            }

            co_await txn->AbortAsync();
            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> GetKeyAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 numTasks = 1)
        {
            CODING_ERROR_ASSERT(items.Count() % numTasks == 0);
            KSharedArray<ktl::Awaitable<LONG64>>::SPtr getTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<LONG64>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = items.Count() / numTasks;

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                getTasksSPtr->Append(GetKeyAsync(items, offset, countPerTask));
                offset += countPerTask;
            }

            co_await StoreUtilities::WhenAll(*getTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> UpdateKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 offset,
            __in ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = &items;

            auto txn = this->CreateWriteTransaction();

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 k = offset; k < offset + count; k++)
            {
                auto item = (*itemsSPtr)[k];
                auto key = item.Key;
                auto value = item.Value;

                co_await this->Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
            }

            co_await txn->CommitAsync();
            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> RemoveKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 offset,
            __in ULONG32 count)
        {
            co_await CorHelper::ThreadPoolThread(this->GetAllocator().GetKtlSystem().DefaultThreadPool());

            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = &items;

            auto txn = this->CreateWriteTransaction();

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 k = offset; k < offset + count; k++)
            {
                auto item = (*itemsSPtr)[k];
                auto key = item.Key;

                co_await this->Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            }

            co_await txn->CommitAsync();
            stopwatch.Stop();

            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> RemoveKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 numTasks = 1)
        {
            CODING_ERROR_ASSERT(items.Count() % numTasks == 0);
            KSharedArray<ktl::Awaitable<LONG64>>::SPtr removeTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<LONG64>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = items.Count() / numTasks;

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                removeTasksSPtr->Append(RemoveKeysAsync(items, offset, countPerTask));
                offset += countPerTask;
            }

            co_await StoreUtilities::WhenAll(*removeTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            co_return stopwatch.ElapsedMilliseconds;
        }

        ktl::Awaitable<LONG64> UpdateKeysAsync(
            __in KSharedArray<KeyValuePair<TKey, TValue>> & items,
            __in ULONG32 numTasks = 1)
        {
            CODING_ERROR_ASSERT(items.Count() % numTasks == 0);
            KSharedArray<ktl::Awaitable<LONG64>>::SPtr updateTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<LONG64>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = items.Count() / numTasks;

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                updateTasksSPtr->Append(UpdateKeysAsync(items, offset, countPerTask));
                offset += countPerTask;
            }

            co_await StoreUtilities::WhenAll(*updateTasksSPtr, this->GetAllocator());

            stopwatch.Stop();
            co_return stopwatch.ElapsedMilliseconds;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG32 fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, this->GetAllocator());

            ULONG32* buffer = static_cast<ULONG32 *>(resultSPtr->GetBuffer());

            auto numElements = sizeInBytes / sizeof(ULONG32);
            for (ULONG32 i = 0; i < numElements; i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

        static bool DefaultEqualityFunction(TValue & one, TValue & two)
        {
            return one == two;
        }

        virtual TKey CreateKey(__in ULONG32 index)
        {
            UNREFERENCED_PARAMETER(index);
            return TKey();
        }

        virtual TValue CreateValue(__in ULONG32 index)
        {
            UNREFERENCED_PARAMETER(index);
            return TValue();
        }

        void CRUDPerf_UniqueKeys(__in ULONG32 totalKeys, __in ULONG32 numTasks)
        {
            TRACE_TEST();
            // Create items ahead of time
            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<KeyValuePair<TKey, TValue>>();
            for (ULONG32 i = 0; i < totalKeys; i++)
            {
                TKey key = CreateKey(i);
                TValue value = CreateValue(i);
                KeyValuePair<TKey, TValue> pair(key, value);
                itemsSPtr->Append(pair);
            }

            LONG64 addTime = SyncAwait(AddKeysAsync(*itemsSPtr, numTasks));

            Trace.WriteInfo(
                "Perf",
                "CRUD_UniqueKeys Add {0} keys with {1} tasks: {2} ms",
                totalKeys,
                numTasks,
                addTime);

            LONG64 readTime = SyncAwait(GetKeyAsync(*itemsSPtr, numTasks));

            Trace.WriteInfo(
                "Perf",
                "CRUD_UniqueKeys Read {0} keys with {1} tasks: {2} ms",
                totalKeys,
                numTasks,
                readTime);

            for (ULONG32 i = 0; i < totalKeys; i++)
            {
                KeyValuePair<TKey, TValue> & item = (*itemsSPtr)[i];
                item.Value = CreateValue(i + 10);
            }

            LONG64 updateTime = SyncAwait(UpdateKeysAsync(*itemsSPtr, numTasks));

            Trace.WriteInfo(
                "Perf",
                "CRUD_UniqueKeys Update {0} keys with {1} tasks: {2} ms",
                totalKeys,
                numTasks,
                updateTime);

            LONG64 removeTime = SyncAwait(RemoveKeysAsync(*itemsSPtr, numTasks));

            Trace.WriteInfo(
                "Perf",
                "CRUD_UniqueKeys Remove {0} keys with {1} tasks: {2} ms",
                totalKeys,
                numTasks,
                removeTime);
        }

        LONG64 GetAsyncCallTime = 0;

        ktl::Awaitable<void> ReadSingleKeyAsync(
            __in TKey key,
            __in ULONG32 numReads,
            __in ULONG32 numTransactions)
        {
            CODING_ERROR_ASSERT(numReads % numTransactions == 0);
            ULONG32 numReadsPerTxn = numReads / numTransactions;

            for (ULONG32 t = 0; t < numTransactions; t++)
            {
                auto txn = this->CreateWriteTransaction();
                for (ULONG32 k = 0; k < numReadsPerTxn; k++)
                {
                    KeyValuePair<LONG64, TValue> output(-1, TValue());

                    Common::Stopwatch stopwatch;
                    stopwatch.Start();
                    co_await this->Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, output, ktl::CancellationToken::None);
                    stopwatch.Stop();
                    InterlockedAdd64(&GetAsyncCallTime, stopwatch.ElapsedTicks);
                }
                co_await txn->AbortAsync();
            }
        }

        void SingleKeyReadTest(
            __in ULONG32 totalNumReads,
            __in ULONG32 numTasks,
            __in bool singleTransaction,
            __in bool fromConsolidated)
        {
            TRACE_TEST()

            this->Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            CODING_ERROR_ASSERT(totalNumReads % numTasks == 0);
            KSharedArray<ktl::Awaitable<void>>::SPtr getTasksSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            ULONG32 offset = 0;
            ULONG32 countPerTask = totalNumReads / numTasks;

            auto key = CreateKey(8);
            auto value = CreateValue(8);

            {
                auto txn = this->CreateWriteTransaction();
                SyncAwait(this->Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            if (fromConsolidated)
            {
                this->Checkpoint();
            }


            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                getTasksSPtr->Append(ReadSingleKeyAsync(key, countPerTask, singleTransaction ? 1 : countPerTask));
                offset += countPerTask;
            }

            SyncAwait(StoreUtilities::WhenAll(*getTasksSPtr, this->GetAllocator()));

            stopwatch.Stop();

            Trace.WriteInfo(
                "Perf",
                "SingleKeyReadTest Read {0} times with {1} tasks: {2} ms",
                totalNumReads,
                numTasks,
                stopwatch.ElapsedMilliseconds);
        }

        void CheckpointAndReadKeysFromDiskTest(__in ULONG32 numKeys, __in ULONG32 parallelism = 200)
        {
            TRACE_TEST();
            // Create items ahead of time
            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<KeyValuePair<TKey, TValue>>();
            for (ULONG32 i = 0; i < numKeys; i++)
            {
                TKey key = CreateKey(i);
                TValue value = CreateValue(i);
                KeyValuePair<TKey, TValue> pair(key, value);
                itemsSPtr->Append(pair);
            }

            SyncAwait(AddKeysAsync(*itemsSPtr, parallelism));

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            this->Checkpoint();
            stopwatch.Stop();

            Trace.WriteInfo(
                "Perf",
                "CheckpointAndReadKeysFromDiskTest Checkpoint {0} keys: {1} ms",
                numKeys,
                stopwatch.ElapsedMilliseconds);

            this->Store->ShouldLoadValuesOnRecovery = false;
            LONG64 recoveryTime = this->CloseAndReOpenStore();

            Trace.WriteInfo(
                "Perf",
                "CheckpointAndReadKeysFromDiskTest Recover {0} keys: {1} ms",
                numKeys,
                recoveryTime);
        }

        void CheckpointAndReadFromDiskTest(
            __in ULONG32 numKeys,
            __in ULONG32 numReaders,
            __in ULONG32 parallelism = 200)
        {
            // Create items ahead of time
            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<KeyValuePair<TKey, TValue>>();
            for (ULONG32 i = 0; i < numKeys; i++)
            {
                TKey key = CreateKey(i);
                TValue value = CreateValue(i);
                KeyValuePair<TKey, TValue> pair(key, value);
                itemsSPtr->Append(pair);
            }

            SyncAwait(AddKeysAsync(*itemsSPtr, parallelism));

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            this->Checkpoint();
            stopwatch.Stop();

            Trace.WriteInfo(
                "Perf",
                "CheckpointAndReadFromDiskTest Checkpoint {0} keys: {1} ms",
                numKeys,
                stopwatch.ElapsedMilliseconds);

            this->Store->ShouldLoadValuesOnRecovery = false;
            LONG64 recoveryTime = this->CloseAndReOpenStore();

            Trace.WriteInfo(
                "Perf",
                "CheckpointAndReadFromDiskTest Recover {0} keys: {1} ms",
                numKeys,
                recoveryTime);

            LONG64 readTime = SyncAwait(GetKeyAsync(*itemsSPtr, numReaders));

            Trace.WriteInfo(
                "Perf",
                "CheckpointAndReadFromDiskTest Read {0} keys with {1} tasks: {2} ms",
                numKeys,
                numReaders,
                readTime);
        }

        void MultipleCheckpointTest(__in ULONG numKeys, __in ULONG numFiles, __in ULONG numReaders)
        {
            // Create items ahead of time
            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> itemsSPtr = _new(STOREPERFTESTBASE_TAG, this->GetAllocator()) KSharedArray<KeyValuePair<TKey, TValue>>();
            for (ULONG32 i = 0; i < numKeys; i++)
            {
                TKey key = CreateKey(i);
                TValue value = CreateValue(i);
                KeyValuePair<TKey, TValue> pair(key, value);
                itemsSPtr->Append(pair);
            }

            const ULONG keysPerFile = numKeys / numFiles;
            for (ULONG32 offset = 0; offset < numKeys; offset += keysPerFile)
            {
                SyncAwait(AddKeysAsync(*itemsSPtr, offset, keysPerFile));

                Common::Stopwatch checkpointStopwatch;

                checkpointStopwatch.Start();
                this->Checkpoint();
                checkpointStopwatch.Stop();

                Trace.WriteInfo(
                    "Perf",
                    "Checkpoint: {0} ms",
                    checkpointStopwatch.ElapsedMilliseconds);
            }

            this->Store->ShouldLoadValuesOnRecovery = false;
            LONG64 recoveryTime = this->CloseAndReOpenStore();

            Trace.WriteInfo(
                "Perf",
                "Recover: {0} ms",
                recoveryTime);

            LONG64 readTime = SyncAwait(GetKeyAsync(*itemsSPtr, numReaders));

            Trace.WriteInfo(
                "Perf",
                "Read: {0} ms",
                readTime);

        }
        
        template <typename KeyType>
        static ULONG DefaultHash(__in KeyType const & key)
        {
            return K_DefaultHashFunction(key);
        }

        template <>
        static ULONG DefaultHash(__in int const & key)
        {
            return K_DefaultHashFunction(LONGLONG(key));
        }

        template <>
        static ULONG DefaultHash(__in KBuffer::SPtr const & key)
        {
            return KBufferComparer::Hash(key);
        }
    };
}
